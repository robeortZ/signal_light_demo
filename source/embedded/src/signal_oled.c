/**
 * @file signal_oled.c
 * @brief SSD1306 OLED UI — concise status + active link (BT or UART)
 * @version 1.1
 * @date 2026-06-16
 * @copyright Copyright (c) Tuya Inc.
 */
#include "signal_oled.h"
#include "signal_config.h"
#include "signal_oled_icon.h"
#include "agent_status.h"
#include "tal_api.h"
#include "tal_time_service.h"
#include "tkl_pinmux.h"
#include "tdd_disp_ssd1306.h"
#include "tdd_disp_type.h"
#include "u8g2_port.h"
#include "u8g2.h"
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static u8g2_t s_u8g2;
static THREAD_HANDLE s_oled_thread = NULL;
static MUTEX_HANDLE s_link_mutex = NULL;
static SIGNAL_OLED_LINK_T s_link = {0};
static AGENT_STATUS_E s_prev_status = AGENT_STATUS_IDLE;
static uint32_t s_idle_since_ms = 0;
static uint32_t s_unlink_since_ms = 0;

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */
/**
 * @brief Register SSD1306 on I2C0
 * @return OPRT_OK on success
 */
static OPERATE_RET __oled_register_display(void)
{
    DISP_I2C_OLED_DEVICE_CFG_T display_cfg;
    DISP_SSD1306_INIT_CFG_T init_cfg;

    memset(&display_cfg, 0, sizeof(display_cfg));
    memset(&init_cfg, 0, sizeof(init_cfg));

    tkl_io_pinmux_config(SIGNAL_OLED_I2C_SCL, TUYA_IIC0_SCL);
    tkl_io_pinmux_config(SIGNAL_OLED_I2C_SDA, TUYA_IIC0_SDA);

    display_cfg.bl.type = TUYA_DISP_BL_TP_NONE;
    display_cfg.width = SIGNAL_OLED_WIDTH;
    display_cfg.height = SIGNAL_OLED_HEIGHT;
    display_cfg.rotation = TUYA_DISPLAY_ROTATION_0;
    display_cfg.port = SIGNAL_OLED_I2C_PORT;
    display_cfg.addr = SIGNAL_OLED_I2C_ADDR;
    display_cfg.power.pin = TUYA_GPIO_NUM_MAX;

    init_cfg.is_color_inverse = true;
    init_cfg.com_pin_cfg = SSD1306_COM_PIN_CFG;

    return tdd_disp_i2c_oled_ssd1306_register(SIGNAL_DISPLAY_NAME, &display_cfg, &init_cfg);
}

/**
 * @brief Pick active transport label (LAN/WS preferred over UART/BT)
 * @param[in] link link state
 * @return "LAN", "BT", "UART", or NULL if no active link
 */
static const char *__active_link_label(const SIGNAL_OLED_LINK_T *link)
{
    if (link == NULL) {
        return NULL;
    }
    if (link->ws_active) {
        return "LAN";
    }
#if SIGNAL_TRANSPORT_BLE_ENABLE
    if (link->ble_connected) {
        return "BT";
    }
#endif
    if (link->uart_active) {
        return "UART";
    }
    return NULL;
}

/**
 * @brief Whether PC bridge is reachable via LAN or UART
 * @param[in] link link state
 * @return true if ws_active or uart_active
 */
static bool __link_connected(const SIGNAL_OLED_LINK_T *link)
{
    if (link == NULL) {
        return false;
    }
    if (link->ws_active) {
        return true;
    }
    if (link->uart_active) {
        return true;
    }
    return false;
}

/**
 * @brief Top-right disconnect tag after unlink timeout
 * @param[in] now_ms monotonic tick in ms
 * @return true when corner tag should show DISCONN
 */
static bool __unlink_corner_due(uint32_t now_ms)
{
    if (s_unlink_since_ms == 0) {
        return false;
    }
    return (now_ms - s_unlink_since_ms) >= SIGNAL_OLED_DISCONNECT_TAG_MS;
}

/**
 * @brief Short uppercase status label for main OLED line
 * @param[in] status agent status
 * @return status label string
 */
static const char *__status_label(AGENT_STATUS_E status)
{
    switch (status) {
    case AGENT_STATUS_IDLE:
        return "IDLE";
    case AGENT_STATUS_WORKING:
        return "WORKING";
    case AGENT_STATUS_ATTENTION:
        return "ATTENTION";
    case AGENT_STATUS_URGENT:
        return "URGENT";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Format local wall-clock time as HH:MM
 * @param[out] buf output buffer
 * @param[in] len buffer size (at least 6 bytes)
 * @return true if time is synced and formatted
 */
static bool __format_local_hhmm(char *buf, size_t len)
{
    POSIX_TM_S tm;
    TIME_T posix = 0;

    if (buf == NULL || len < 6) {
        return false;
    }
    if (tal_time_check_time_sync() != OPRT_OK) {
        return false;
    }
    posix = tal_time_get_posix();
    if (posix <= 0) {
        return false;
    }
    if (tal_time_get_local_time_custom(posix, &tm) != OPRT_OK) {
        return false;
    }
    snprintf(buf, len, "%02d:%02d", tm.tm_hour, tm.tm_min);
    return true;
}

/**
 * @brief Whether IDLE has lasted long enough to show clock on main line
 * @param[in] status current agent status
 * @param[in] now_ms monotonic tick in ms
 * @return true when clock should replace "IDLE"
 */
static bool __oled_idle_clock_due(AGENT_STATUS_E status, uint32_t now_ms)
{
    if (status != AGENT_STATUS_IDLE) {
        return false;
    }
    if (s_idle_since_ms == 0) {
        s_idle_since_ms = now_ms;
        return false;
    }
    return (now_ms - s_idle_since_ms) >= SIGNAL_OLED_IDLE_CLOCK_MS;
}

/**
 * @brief Draw agent mascot icon on the left
 * @return none
 */
static void __draw_agent_icon(void)
{
    u8g2_DrawXBMP(&s_u8g2, SIGNAL_AGENT_ICON_X, SIGNAL_AGENT_ICON_Y,
                  SIGNAL_AGENT_ICON_WIDTH, SIGNAL_AGENT_ICON_HEIGHT, SIGNAL_AGENT_ICON_BITS);
}

/**
 * @brief Draw status label to the right of the mascot icon
 * @param[in] status_label uppercase status text
 * @return none
 */
static void __draw_status_beside_icon(const char *status_label)
{
    uint8_t text_x = 0;
    uint8_t sw = 0;

    if (status_label == NULL || status_label[0] == '\0') {
        return;
    }

    text_x = SIGNAL_AGENT_ICON_X + SIGNAL_AGENT_ICON_WIDTH + 6;
    u8g2_SetFont(&s_u8g2, u8g2_font_logisoso16_tr);
    sw = u8g2_GetStrWidth(&s_u8g2, status_label);
    if (text_x + sw > SIGNAL_OLED_WIDTH) {
        u8g2_SetFont(&s_u8g2, u8g2_font_helvB10_tr);
    }
    u8g2_DrawStr(&s_u8g2, text_x, SIGNAL_AGENT_ICON_Y + 22, status_label);
}

/**
 * @brief Draw active link tag top-right (small font)
 * @param[in] label "BT" or "UART"
 * @return none
 */
static void __draw_link_tag_top_right(const char *label)
{
    uint8_t tw = 0;

    if (label == NULL || label[0] == '\0') {
        return;
    }

    u8g2_SetFont(&s_u8g2, u8g2_font_4x6_tr);
    tw = u8g2_GetStrWidth(&s_u8g2, label);
    u8g2_DrawStr(&s_u8g2, SIGNAL_OLED_WIDTH - tw, 6, label);
}

/**
 * @brief Refresh full OLED frame
 * @return none
 */
static void __oled_refresh_frame(void)
{
    AGENT_STATUS_E status = agent_status_get();
    SIGNAL_OLED_LINK_T link = {0};
    const char *main_label = NULL;
    const char *link_label = NULL;
    char clock_buf[8] = {0};
    uint32_t now_ms = tal_system_get_millisecond();
    bool linked = false;

    if (s_link_mutex) {
        tal_mutex_lock(s_link_mutex);
        link = s_link;
        tal_mutex_unlock(s_link_mutex);
    }

    linked = __link_connected(&link);
    if (linked) {
        s_unlink_since_ms = 0;
    } else if (s_unlink_since_ms == 0) {
        s_unlink_since_ms = now_ms;
    }

    if (status == AGENT_STATUS_IDLE) {
        if (s_prev_status != AGENT_STATUS_IDLE) {
            s_idle_since_ms = now_ms;
        }
    } else {
        s_idle_since_ms = now_ms;
    }
    s_prev_status = status;

    if (__oled_idle_clock_due(status, now_ms) && __format_local_hhmm(clock_buf, sizeof(clock_buf))) {
        main_label = clock_buf;
    } else {
        main_label = __status_label(status);
    }

    if (linked) {
        link_label = __active_link_label(&link);
    } else if (__unlink_corner_due(now_ms)) {
        link_label = "DISCONN";
    }

    u8g2_ClearBuffer(&s_u8g2);
    __draw_link_tag_top_right(link_label);
    __draw_agent_icon();
    __draw_status_beside_icon(main_label);

    if (status == AGENT_STATUS_URGENT) {
        u8g2_DrawFrame(&s_u8g2, 0, 14, SIGNAL_OLED_WIDTH, 36);
    }

    u8g2_SendBuffer(&s_u8g2);
}

/**
 * @brief OLED refresh task
 * @param[in] arg unused
 * @return none
 */
static void __signal_oled_task(void *arg)
{
    (void)arg;
    PR_INFO("[oled] refresh task started");

    while (1) {
        __oled_refresh_frame();
        tal_system_sleep(SIGNAL_OLED_REFRESH_MS);
    }
}

/**
 * @brief Initialize SSD1306 display and start refresh task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_oled_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    THREAD_CFG_T thrd = {0};

    if (s_link_mutex == NULL) {
        tal_mutex_create_init(&s_link_mutex);
    }

    rt = __oled_register_display();
    if (rt != OPRT_OK) {
        PR_ERR("[oled] display register failed: %d", rt);
        return rt;
    }

    rt = u8g2_Setup_tdl_display_f(&s_u8g2, SIGNAL_DISPLAY_NAME);
    if (rt != OPRT_OK) {
        PR_ERR("[oled] u8g2 setup failed: %d", rt);
        return rt;
    }

    u8g2_InitDisplay(&s_u8g2);
    u8g2_SetPowerSave(&s_u8g2, 0);

    thrd.stackDepth = 4096;
    thrd.priority = THREAD_PRIO_2;
    thrd.thrdname = "signal_oled";
    thrd.psram_mode = 0;

    rt = tal_thread_create_and_start(&s_oled_thread, NULL, NULL, __signal_oled_task, NULL, &thrd);
    if (rt != OPRT_OK) {
        PR_ERR("[oled] thread create failed: %d", rt);
        return rt;
    }

    PR_INFO("[oled] SSD1306 128x64 ready");
    return OPRT_OK;
}

/**
 * @brief Update BLE / UART link icons shown on OLED
 * @param[in] link link state
 * @return OPRT_OK on success
 */
OPERATE_RET signal_oled_set_link(const SIGNAL_OLED_LINK_T *link)
{
    if (link == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (s_link_mutex) {
        tal_mutex_lock(s_link_mutex);
        s_link = *link;
        tal_mutex_unlock(s_link_mutex);
    } else {
        s_link = *link;
    }

    return OPRT_OK;
}
