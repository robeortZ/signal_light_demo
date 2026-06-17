/**
 * @file signal_transport.c
 * @brief BLE and UART transport for agent status updates
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#include "signal_transport.h"
#include "signal_config.h"
#include "agent_status.h"
#include "tal_api.h"
#include "tal_cli.h"
#include "tkl_pinmux.h"
#if SIGNAL_TRANSPORT_BLE_ENABLE
#include "tal_bluetooth.h"
#endif
#if SIGNAL_TRANSPORT_WS_ENABLE
#include "signal_ws.h"
#include "signal_discovery.h"
#endif
#include <string.h>
#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define SIGNAL_ASR_LINE_MAX 512

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static bool s_ble_connected = false;
static bool s_uart_active = false;
static bool s_ws_active = false;
static uint32_t s_uart_last_rx_ms = 0;
static uint32_t s_ws_last_rx_ms = 0;
#if !SIGNAL_UART_SHARE_WITH_CLI
static THREAD_HANDLE s_uart_thread = NULL;
static char s_uart_line_buf[SIGNAL_LINE_MAX];
static uint32_t s_uart_line_len = 0;
#endif
static THREAD_HANDLE s_link_thread = NULL;

#if SIGNAL_TRANSPORT_BLE_ENABLE
static uint8_t s_adv_data[31] = {
    0x02, 0x01, 0x06, 0x03, 0x02, 0x50, 0xFD, 0x14, 0x09, 0x53, 0x69, 0x67, 0x4E, 0x61, 0x6C, 0x69, 0x67, 0x68, 0x74,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static uint8_t s_scan_rsp[31] = {
    0x11, 0x09, 'S', 'i', 'g', 'n', 'a', 'l', 'L', 'i', 'g', 'h', 't', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif

/**
 * @brief Parse status name for CLI / SIGNAL command
 * @param[in] name status name string
 * @param[out] status parsed status
 * @return OPRT_OK on success
 */
static OPERATE_RET __parse_status_name(const char *name, AGENT_STATUS_E *status)
{
    uint8_t buf[32] = {0};
    size_t len = 0;

    if (name == NULL || status == NULL) {
        return OPRT_INVALID_PARM;
    }

    len = strlen(name);
    if (len >= sizeof(buf)) {
        return OPRT_INVALID_PARM;
    }
    memcpy(buf, name, len);
    return agent_status_parse(buf, (uint32_t)len, status);
}

/**
 * @brief Mark UART link active after a valid frame
 * @return none
 */
static void __uart_mark_active(void)
{
    s_uart_last_rx_ms = tal_system_get_millisecond();
    s_uart_active = true;
}

/**
 * @brief Mark LAN WebSocket link active after a valid frame
 * @return none
 */
static void __ws_mark_active(void)
{
    s_ws_last_rx_ms = tal_system_get_millisecond();
    s_ws_active = true;
}

/**
 * @brief Apply parsed status with LAN/WS priority over UART
 * @param[in] status parsed status
 * @param[in] source transport source
 * @return none
 */
static void __apply_status(AGENT_STATUS_E status, SIGNAL_SOURCE_E source)
{
#if SIGNAL_TRANSPORT_BLE_ENABLE
    if (source == SIGNAL_SRC_UART && s_ble_connected) {
        PR_DEBUG("[transport] uart frame ignored (ble active)");
        return;
    }
#endif
#if SIGNAL_TRANSPORT_WS_ENABLE
    if (source == SIGNAL_SRC_UART && (s_ws_active || signal_ws_is_connected())) {
        PR_DEBUG("[transport] uart frame ignored (ws active)");
        return;
    }
#else
    (void)source;
#endif
    agent_status_set(status, source);
}

/**
 * @brief CLI: SIGNAL <idle|working|attention|urgent>  (PC bridge protocol)
 * @param[in] argc argument count
 * @param[in] argv arguments
 * @return none
 */
static void __cmd_signal_frame(int argc, char *argv[])
{
    AGENT_STATUS_E status = AGENT_STATUS_IDLE;

    if (argc < 2) {
        PR_INFO("usage: SIGNAL <idle|working|attention|urgent>");
        return;
    }

    if (__parse_status_name(argv[1], &status) != OPRT_OK) {
        PR_WARN("[transport] SIGNAL unknown: %s", argv[1]);
        return;
    }

    __uart_mark_active();
    __apply_status(status, SIGNAL_SRC_UART);
}

#if SIGNAL_UART_SHARE_WITH_CLI
static cli_cmd_t s_signal_frame_cli[] = {
    { "SIGNAL", "cursor agent status (UART0)", __cmd_signal_frame },
};
#endif

#if SIGNAL_TRANSPORT_BLE_ENABLE || SIGNAL_TRANSPORT_WS_ENABLE || !SIGNAL_UART_SHARE_WITH_CLI
/**
 * @brief Process one complete line from UART or BLE
 * @param[in] line line buffer
 * @param[in] len line length
 * @param[in] source transport source
 * @return none
 */
static void __handle_line(const uint8_t *line, uint32_t len, SIGNAL_SOURCE_E source)
{
    AGENT_STATUS_E status = AGENT_STATUS_IDLE;

    while (len > 0 && (line[len - 1] == '\r' || line[len - 1] == '\n' || line[len - 1] == ' ')) {
        len--;
    }
    if (len == 0) {
        return;
    }

    if (agent_status_parse(line, len, &status) == OPRT_OK) {
        if (source == SIGNAL_SRC_WS) {
            __ws_mark_active();
        } else {
            __uart_mark_active();
        }
        __apply_status(status, source);
    } else {
        PR_WARN("[transport] bad frame (src=%d): %.*s", source, (int)len, line);
    }
}
#endif

#if SIGNAL_TRANSPORT_WS_ENABLE
/**
 * @brief WebSocket line callback from signal_ws
 * @param[in] line line buffer
 * @param[in] len line length
 * @param[in] user unused
 * @return none
 */
static void __on_ws_line(const uint8_t *line, uint32_t len, void *user)
{
    (void)user;
    __handle_line(line, len, SIGNAL_SRC_WS);
}
#endif

#if !SIGNAL_UART_SHARE_WITH_CLI
/**
 * @brief Feed bytes into UART line assembler
 * @param[in] byte incoming byte
 * @return none
 */
static void __uart_feed_byte(uint8_t byte)
{
    if (byte == '\n' || byte == '\r') {
        if (s_uart_line_len > 0) {
            s_uart_line_buf[s_uart_line_len] = '\0';
            PR_DEBUG("[transport] uart line: %s", s_uart_line_buf);
            __handle_line((const uint8_t *)s_uart_line_buf, s_uart_line_len, SIGNAL_SRC_UART);
            s_uart_line_len = 0;
        }
        return;
    }

    if (s_uart_line_len < SIGNAL_LINE_MAX - 1) {
        s_uart_line_buf[s_uart_line_len++] = (char)byte;
    } else {
        PR_WARN("[transport] uart line overflow, reset");
        s_uart_line_len = 0;
    }
}
#endif

#if SIGNAL_TRANSPORT_BLE_ENABLE
/**
 * @brief BLE event handler
 * @param[in] p_event BLE event
 * @return none
 */
static void __ble_event_callback(TAL_BLE_EVT_PARAMS_T *p_event)
{
    if (p_event == NULL) {
        return;
    }

    switch (p_event->type) {
    case TAL_BLE_STACK_INIT: {
        if (p_event->ble_event.init == 0) {
            TAL_BLE_DATA_T adv = { .p_data = s_adv_data, .len = sizeof(s_adv_data) };
            TAL_BLE_DATA_T rsp = { .p_data = s_scan_rsp, .len = sizeof(s_scan_rsp) };

            tal_ble_advertising_data_set(&adv, &rsp);
            tal_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
            PR_INFO("[transport] BLE advertising as SignalLight");
        }
        break;
    }
    case TAL_BLE_EVT_PERIPHERAL_CONNECT: {
        if (p_event->ble_event.connect.result == 0) {
            s_ble_connected = true;
            PR_INFO("[transport] BLE connected");
        } else {
            s_ble_connected = false;
            PR_WARN("[transport] BLE connect failed");
        }
        break;
    }
    case TAL_BLE_EVT_DISCONNECT: {
        s_ble_connected = false;
        PR_INFO("[transport] BLE disconnected, reason=0x%02x", p_event->ble_event.disconnect.reason);
        tal_ble_advertising_start(TUYAOS_BLE_DEFAULT_ADV_PARAM);
        break;
    }
    case TAL_BLE_EVT_WRITE_REQ: {
        TAL_BLE_DATA_REPORT_T *wr = &p_event->ble_event.write_report;
        if (wr->report.p_data != NULL && wr->report.len > 0) {
            PR_INFO("[transport] BLE write len=%u", wr->report.len);
            __handle_line(wr->report.p_data, wr->report.len, SIGNAL_SRC_BLE);
        }
        break;
    }
    default:
        break;
    }
}
#endif

/**
 * @brief UART read task (only when not sharing tal_cli)
 * @param[in] arg unused
 * @return none
 */
#if !SIGNAL_UART_SHARE_WITH_CLI
static void __uart_rx_task(void *arg)
{
    uint8_t rx_chunk[64];
    int read_len = 0;
    uint32_t i = 0;

    (void)arg;
#if SIGNAL_UART_SHARE_WITH_CLI
    PR_INFO("[transport] UART0 RX via tal_cli (SIGNAL / signal commands)");
#else
    PR_INFO("[transport] UART RX task started");
#endif

    while (1) {
        read_len = tal_uart_read(SIGNAL_UART_PORT, rx_chunk, sizeof(rx_chunk));
        if (read_len <= 0) {
            tal_system_sleep(20);
            continue;
        }

        s_uart_last_rx_ms = tal_system_get_millisecond();
        s_uart_active = true;

        for (i = 0; i < (uint32_t)read_len; i++) {
            __uart_feed_byte(rx_chunk[i]);
        }
    }
}
#endif

/**
 * @brief Periodically refresh UART link icon timeout
 * @param[in] arg unused
 * @return none
 */
static void __link_watch_task(void *arg)
{
    SIGNAL_OLED_LINK_T link = {0};
    uint32_t now = 0;

    (void)arg;

    while (1) {
        now = tal_system_get_millisecond();
        if (s_uart_active && s_uart_last_rx_ms > 0) {
            if ((now - s_uart_last_rx_ms) > SIGNAL_UART_LINK_TIMEOUT_MS) {
                s_uart_active = false;
                if (!s_ws_active) {
                    agent_status_set(AGENT_STATUS_IDLE, SIGNAL_SRC_UART);
                }
                PR_INFO("[transport] UART idle (no frame for %ums)", SIGNAL_UART_LINK_TIMEOUT_MS);
            }
        }

#if SIGNAL_TRANSPORT_WS_ENABLE
        if (signal_ws_is_connected()) {
            s_ws_active = true;
        } else if (s_ws_active && s_ws_last_rx_ms > 0) {
            if ((now - s_ws_last_rx_ms) > SIGNAL_WS_LINK_TIMEOUT_MS) {
                s_ws_active = false;
                if (!s_uart_active) {
                    agent_status_set(AGENT_STATUS_IDLE, SIGNAL_SRC_WS);
                }
                PR_INFO("[transport] WS idle (no frame for %ums)", SIGNAL_WS_LINK_TIMEOUT_MS);
            }
        } else if (!signal_ws_is_connected()) {
            s_ws_active = false;
        }
#endif

        link.ble_connected = s_ble_connected;
        link.uart_active = s_uart_active;
        link.ws_active = s_ws_active;
        signal_oled_set_link(&link);

        tal_system_sleep(200);
    }
}

/**
 * @brief Initialize UART2 for status frames
 * @return OPRT_OK on success
 */
static OPERATE_RET __uart_init(void)
{
#if SIGNAL_UART_SHARE_WITH_CLI
    tal_cli_cmd_register(s_signal_frame_cli, sizeof(s_signal_frame_cli) / sizeof(s_signal_frame_cli[0]));
    PR_INFO("[transport] UART0 shared with tal_cli, registered SIGNAL command");
    return OPRT_OK;
#else
    OPERATE_RET rt = OPRT_OK;
    TAL_UART_CFG_T cfg = {0};
    THREAD_CFG_T thrd = {0};

    tkl_io_pinmux_config(SIGNAL_UART_RX_PIN, TUYA_UART2_RX);
    tkl_io_pinmux_config(SIGNAL_UART_TX_PIN, TUYA_UART2_TX);

    cfg.base_cfg.baudrate = SIGNAL_UART_BAUDRATE;
    cfg.base_cfg.databits = TUYA_UART_DATA_LEN_8BIT;
    cfg.base_cfg.stopbits = TUYA_UART_STOP_LEN_1BIT;
    cfg.base_cfg.parity = TUYA_UART_PARITY_TYPE_NONE;
    cfg.rx_buffer_size = SIGNAL_UART_RX_BUF;
    cfg.open_mode = O_BLOCK;

    rt = tal_uart_init(SIGNAL_UART_PORT, &cfg);
    if (rt != OPRT_OK) {
        PR_ERR("[transport] uart init failed: %d", rt);
        return rt;
    }

    thrd.stackDepth = 3072;
    thrd.priority = THREAD_PRIO_2;
    thrd.thrdname = "signal_uart";
    thrd.psram_mode = 0;

    rt = tal_thread_create_and_start(&s_uart_thread, NULL, NULL, __uart_rx_task, NULL, &thrd);
    if (rt != OPRT_OK) {
        PR_ERR("[transport] uart thread failed: %d", rt);
        return rt;
    }

    return OPRT_OK;
#endif
}

/**
 * @brief Initialize transport (UART2; optional BLE when enabled)
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    THREAD_CFG_T thrd = {0};

#if SIGNAL_TRANSPORT_BLE_ENABLE
    rt = tal_ble_bt_init(TAL_BLE_ROLE_PERIPERAL, __ble_event_callback);
    if (rt != OPRT_OK) {
        PR_ERR("[transport] BLE init failed: %d", rt);
        return rt;
    }
#else
    PR_INFO("[transport] BLE disabled — Tuya IoT owns BLE for APP netcfg");
#endif

#if SIGNAL_TRANSPORT_WS_ENABLE
    rt = signal_discovery_init();
    if (rt != OPRT_OK) {
        PR_ERR("[transport] discovery init failed: %d", rt);
        return rt;
    }

    rt = signal_ws_init(SIGNAL_WS_BRIDGE_HOST, SIGNAL_WS_BRIDGE_PORT, __on_ws_line, NULL);
    if (rt != OPRT_OK) {
        PR_ERR("[transport] WS init failed: %d", rt);
        return rt;
    }
    PR_INFO("[transport] WS client (UDP discovery + fallback %s:%u)",
            SIGNAL_WS_BRIDGE_HOST, (unsigned)SIGNAL_WS_BRIDGE_PORT);
#endif

    rt = __uart_init();
    if (rt != OPRT_OK) {
        return rt;
    }

    thrd.stackDepth = 2048;
    thrd.priority = THREAD_PRIO_2;
    thrd.thrdname = "signal_link";
    thrd.psram_mode = 0;
    tal_thread_create_and_start(&s_link_thread, NULL, NULL, __link_watch_task, NULL, &thrd);

#if SIGNAL_TRANSPORT_BLE_ENABLE
    PR_INFO("[transport] BLE+UART+WS ready");
#elif SIGNAL_TRANSPORT_WS_ENABLE
    PR_INFO("[transport] UART0 + LAN/WS ready (SIGNAL / signal via tal_cli)");
#else
    PR_INFO("[transport] UART0 ready (SIGNAL / signal via tal_cli)");
#endif
    return OPRT_OK;
}

/**
 * @brief Get current transport link state for OLED
 * @param[out] link link state output
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_get_link(SIGNAL_OLED_LINK_T *link)
{
    if (link == NULL) {
        return OPRT_INVALID_PARM;
    }

    link->ble_connected = s_ble_connected;
    link->uart_active = s_uart_active;
    link->ws_active = s_ws_active;
    return OPRT_OK;
}

/**
 * @brief Send line to PC preferring WebSocket when connected
 * @param[in] line null-terminated line with trailing newline
 * @return OPRT_OK on success
 */
static OPERATE_RET __send_line_to_pc(const char *line)
{
    uint32_t len = 0;

    if (line == NULL) {
        return OPRT_INVALID_PARM;
    }
    len = (uint32_t)strlen(line);
    if (len == 0) {
        return OPRT_INVALID_PARM;
    }

#if SIGNAL_TRANSPORT_WS_ENABLE
    if (signal_ws_is_connected()) {
        if (signal_ws_send((const uint8_t *)line, len) == OPRT_OK) {
            return OPRT_OK;
        }
    }
#endif

    if (tal_uart_write(SIGNAL_UART_PORT, (const uint8_t *)line, len) == (int)len) {
        return OPRT_OK;
    }
    return OPRT_COM_ERROR;
}

/**
 * @brief Returns true when BLE central is connected
 * @return BLE connection flag
 */
bool signal_transport_ble_connected(void)
{
    return s_ble_connected;
}

/**
 * @brief Send ASR text line to PC via UART0
 * @param[in] text UTF-8 text
 * @param[in] len text length in bytes
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_send_asr(const char *text, uint32_t len)
{
    char line[SIGNAL_ASR_LINE_MAX];
    int written = 0;

    if (text == NULL || len == 0) {
        return OPRT_INVALID_PARM;
    }

    if (len > (SIGNAL_ASR_LINE_MAX - 6)) {
        len = SIGNAL_ASR_LINE_MAX - 6;
    }

    written = snprintf(line, sizeof(line), "ASR %.*s\n", (int)len, text);
    if (written <= 0 || (uint32_t)written >= sizeof(line)) {
        return OPRT_COM_ERROR;
    }

    if (__send_line_to_pc(line) != OPRT_OK) {
        PR_WARN("[transport] ASR send failed");
        return OPRT_COM_ERROR;
    }

    PR_INFO("[transport] ASR -> PC (%u bytes)", len);
    return OPRT_OK;
}

/**
 * @brief Send voice key edge to PC via UART0
 * @param[in] pressed true when button pressed, false on release
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_send_voice_key(bool pressed)
{
    const char *line = pressed ? "VOICE down\n" : "VOICE up\n";

    if (__send_line_to_pc(line) != OPRT_OK) {
        PR_WARN("[transport] VOICE send failed");
        return OPRT_COM_ERROR;
    }

    PR_INFO("[transport] VOICE -> PC (%s)", pressed ? "down" : "up");
    return OPRT_OK;
}

/**
 * @brief Send Enter key pulse to PC via UART0 / WebSocket
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_send_enter_key(void)
{
    const char *line = "KEY enter\n";

    if (__send_line_to_pc(line) != OPRT_OK) {
        PR_WARN("[transport] KEY enter send failed");
        return OPRT_COM_ERROR;
    }

    PR_INFO("[transport] KEY enter -> PC");
    return OPRT_OK;
}
