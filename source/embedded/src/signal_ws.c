/**
 * @file signal_ws.c
 * @brief WebSocket client to PC signal bridge (adapted from hooks_buddy_pixel buddy_ws.c)
 * @version 1.0
 * @date 2026-06-16
 * @copyright Copyright (c) Tuya Inc.
 */
#include "signal_ws.h"
#include "signal_config.h"
#include "signal_discovery.h"
#include "tal_api.h"
#include "tal_network.h"
#include "netmgr.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define WS_OP_TEXT  0x01
#define WS_OP_CLOSE 0x08
#define WS_OP_PING  0x09
#define WS_OP_PONG  0x0A

#define SWS_RX_BUF_SIZE        2048
#define SWS_RECONNECT_MS       5000
#define SWS_SELECT_MS          200
#define SWS_UPGRADE_TIMEOUT_MS 8000
#define SWS_IDLE_TIMEOUT_MS    60000u
#define SWS_IDLE_MAX_TICKS     (SWS_IDLE_TIMEOUT_MS / SWS_SELECT_MS)
#define SWS_WIFI_POLL_MS       2000

/* ---------------------------------------------------------------------------
 * Type definitions
 * --------------------------------------------------------------------------- */
typedef enum {
    SWS_STATE_DISCONNECTED = 0,
    SWS_STATE_CONNECTED,
} sws_conn_e;

typedef struct {
    int              fd;
    sws_conn_e       conn;
    uint8_t          rx_buf[SWS_RX_BUF_SIZE];
    size_t           rx_len;
    MUTEX_HANDLE     tx_mutex;
    THREAD_HANDLE    thread;
    volatile bool    stop;
    char             host[64];
    char             fallback_host[64];
    uint16_t         port;
    uint16_t         fallback_port;
    uint32_t         idle_ticks;
    signal_ws_rx_cb_t rx_cb;
    void            *rx_user;
} sws_ctx_t;

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static sws_ctx_t s_ctx;

static void __disconnect(void);

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */
/**
 * @brief Check whether station WiFi link is up
 * @return true if network is available
 */
static bool __wifi_link_up(void)
{
    netmgr_status_e status = NETMGR_LINK_DOWN;

    netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &status);
    return status != NETMGR_LINK_DOWN;
}

/**
 * @brief Send all bytes on TCP socket
 * @param[in] fd socket fd
 * @param[in] buf data buffer
 * @param[in] len data length
 * @return OPRT_OK on success
 */
static OPERATE_RET __send_all(int fd, const uint8_t *buf, size_t len)
{
    size_t sent = 0;

    while (sent < len) {
        int n = tal_net_send(fd, buf + sent, (uint32_t)(len - sent));
        if (n == OPRT_RESOURCE_NOT_READY) {
            tal_system_sleep(5);
            continue;
        }
        if (n <= 0) {
            return OPRT_SEND_ERR;
        }
        sent += (size_t)n;
    }
    return OPRT_OK;
}

/**
 * @brief Receive bytes with timeout
 * @param[in] fd socket fd
 * @param[out] buf receive buffer
 * @param[in] len max bytes to read
 * @param[in] timeout_ms timeout in milliseconds
 * @return bytes read, 0 on timeout, -1 on error
 */
static int __recv_timeout(int fd, uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    uint32_t elapsed = 0;

    while (elapsed < timeout_ms) {
        TUYA_FD_SET_T rfds;

        TAL_FD_ZERO(&rfds);
        TAL_FD_SET(fd, &rfds);
        int ready = tal_net_select(fd + 1, &rfds, NULL, NULL, 50);
        if (ready > 0 && TAL_FD_ISSET(fd, &rfds)) {
            int n = tal_net_recv(fd, buf, (uint32_t)len);
            if (n > 0) {
                return n;
            }
            if (n != OPRT_RESOURCE_NOT_READY) {
                return -1;
            }
        }
        elapsed += 50;
    }
    return 0;
}

/**
 * @brief Generate WebSocket client mask key
 * @param[out] mask 4-byte mask
 * @return none
 */
static void __gen_mask(uint8_t mask[4])
{
    static uint32_t s_ctr = 0;
    uint32_t v = tal_system_get_millisecond() ^ (++s_ctr * 0x9e3779b9u);

    mask[0] = (uint8_t)((v >> 24) & 0xFF);
    mask[1] = (uint8_t)((v >> 16) & 0xFF);
    mask[2] = (uint8_t)((v >> 8) & 0xFF);
    mask[3] = (uint8_t)(v & 0xFF);
}

/**
 * @brief Send masked WebSocket frame (client role)
 * @param[in] fd socket fd
 * @param[in] opcode WS opcode
 * @param[in] payload frame payload
 * @param[in] plen payload length
 * @return OPRT_OK on success
 */
static OPERATE_RET __ws_send_masked(int fd, uint8_t opcode, const uint8_t *payload, size_t plen)
{
    uint8_t header[14] = {0};
    size_t hlen = 0;
    uint8_t mask[4] = {0};
    uint8_t *masked = NULL;
    OPERATE_RET rt = OPRT_OK;

    __gen_mask(mask);
    header[0] = (uint8_t)(0x80 | (opcode & 0x0F));

    if (plen <= 125) {
        header[1] = (uint8_t)(0x80 | plen);
        hlen = 2;
    } else if (plen <= 0xFFFF) {
        header[1] = (uint8_t)(0x80 | 126);
        header[2] = (uint8_t)((plen >> 8) & 0xFF);
        header[3] = (uint8_t)(plen & 0xFF);
        hlen = 4;
    } else {
        return OPRT_INVALID_PARM;
    }

    memcpy(header + hlen, mask, 4);
    hlen += 4;

    rt = __send_all(fd, header, hlen);
    if (rt != OPRT_OK || plen == 0) {
        return rt;
    }

    masked = (uint8_t *)tal_malloc(plen);
    if (masked == NULL) {
        return OPRT_MALLOC_FAILED;
    }

    for (size_t i = 0; i < plen; i++) {
        masked[i] = (uint8_t)(payload[i] ^ mask[i % 4]);
    }
    rt = __send_all(fd, masked, plen);
    tal_free(masked);
    return rt;
}

/**
 * @brief Decode one inbound WebSocket frame
 * @return OPRT_OK, OPRT_RESOURCE_NOT_READY if incomplete
 */
static OPERATE_RET __ws_decode_frame(const uint8_t *buf, size_t len, uint8_t *opcode,
                                     const uint8_t **payload, size_t *plen, size_t *consumed)
{
    uint64_t pl = 0;
    size_t off = 0;
    bool masked = false;

    if (len < 2) {
        return OPRT_RESOURCE_NOT_READY;
    }

    *opcode = (uint8_t)(buf[0] & 0x0F);
    masked = (buf[1] & 0x80) != 0;
    pl = (uint64_t)(buf[1] & 0x7F);
    off = 2;

    if (pl == 126) {
        if (len < off + 2) {
            return OPRT_RESOURCE_NOT_READY;
        }
        pl = ((uint64_t)buf[off] << 8) | buf[off + 1];
        off += 2;
    } else if (pl == 127) {
        if (len < off + 8) {
            return OPRT_RESOURCE_NOT_READY;
        }
        pl = 0;
        for (int i = 0; i < 8; i++) {
            pl = (pl << 8) | buf[off + i];
        }
        off += 8;
    }

    if (masked) {
        if (len < off + 4) {
            return OPRT_RESOURCE_NOT_READY;
        }
        off += 4;
    }

    if (pl > (uint64_t)(SWS_RX_BUF_SIZE - off)) {
        size_t full = off + (size_t)pl;
        if (len < full) {
            return OPRT_RESOURCE_NOT_READY;
        }
        *consumed = full;
        *plen = 0;
        *payload = NULL;
        return OPRT_OK;
    }

    {
        size_t frame = off + (size_t)pl;
        if (len < frame) {
            return OPRT_RESOURCE_NOT_READY;
        }
        *payload = buf + off;
        *plen = (size_t)pl;
        *consumed = frame;
    }
    return OPRT_OK;
}

/**
 * @brief Remove consumed bytes from RX buffer
 * @param[in] n bytes to drop
 * @return none
 */
static void __rx_consume(size_t n)
{
    if (n == 0 || n > s_ctx.rx_len) {
        return;
    }
    if (n < s_ctx.rx_len) {
        memmove(s_ctx.rx_buf, s_ctx.rx_buf + n, s_ctx.rx_len - n);
    }
    s_ctx.rx_len -= n;
}

/**
 * @brief Feed text payload into line assembler and invoke callback
 * @param[in] payload frame payload
 * @param[in] plen payload length
 * @return none
 */
static void __feed_text_payload(const uint8_t *payload, size_t plen)
{
    static char line_buf[SIGNAL_LINE_MAX];
    static uint32_t line_len = 0;
    uint32_t i = 0;

    if (payload == NULL || plen == 0 || s_ctx.rx_cb == NULL) {
        return;
    }

    for (i = 0; i < plen; i++) {
        char c = (char)payload[i];
        if (c == '\n' || c == '\r') {
            if (line_len > 0) {
                s_ctx.rx_cb((const uint8_t *)line_buf, line_len, s_ctx.rx_user);
                line_len = 0;
            }
            continue;
        }
        if (line_len < SIGNAL_LINE_MAX - 1) {
            line_buf[line_len++] = c;
        } else {
            line_len = 0;
        }
    }
}

/**
 * @brief Perform HTTP WebSocket upgrade handshake
 * @param[in] fd connected TCP socket
 * @return OPRT_OK on success
 */
static OPERATE_RET __ws_upgrade(int fd)
{
    char req[512] = {0};
    uint8_t resp[512] = {0};
    int n = 0;
    int got = 0;

    n = snprintf(req, sizeof(req),
                 "GET /device HTTP/1.1\r\n"
                 "Host: %s:%u\r\n"
                 "Origin: http://%s:%u\r\n"
                 "Upgrade: websocket\r\n"
                 "Connection: Upgrade\r\n"
                 "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
                 "Sec-WebSocket-Version: 13\r\n"
                 "\r\n",
                 s_ctx.host, (unsigned)s_ctx.port, s_ctx.host, (unsigned)s_ctx.port);
    if (n <= 0 || (size_t)n >= sizeof(req)) {
        return OPRT_BUFFER_NOT_ENOUGH;
    }

    if (__send_all(fd, (const uint8_t *)req, (size_t)n) != OPRT_OK) {
        return OPRT_SEND_ERR;
    }

    got = __recv_timeout(fd, resp, sizeof(resp) - 1, SWS_UPGRADE_TIMEOUT_MS);
    if (got <= 0) {
        return OPRT_RECV_ERR;
    }
    resp[got] = '\0';

    if (strstr((const char *)resp, "101") == NULL) {
        PR_ERR("[signal_ws] upgrade rejected: %.80s", (const char *)resp);
        return OPRT_COM_ERROR;
    }

    PR_INFO("[signal_ws] connected to %s:%u", s_ctx.host, (unsigned)s_ctx.port);
    return OPRT_OK;
}

/**
 * @brief Pick WS target from UDP discovery or compile-time fallback
 * @return none
 */
static void __resolve_bridge_target(void)
{
    char disc_host[64] = {0};
    uint16_t disc_port = 0;
    char new_host[64] = {0};
    uint16_t new_port = 0;

    if (signal_discovery_get_bridge(disc_host, sizeof(disc_host), &disc_port)) {
        snprintf(new_host, sizeof(new_host), "%s", disc_host);
        new_port = disc_port;
    } else {
        snprintf(new_host, sizeof(new_host), "%s", s_ctx.fallback_host);
        new_port = s_ctx.fallback_port;
    }

    if (strncmp(s_ctx.host, new_host, sizeof(s_ctx.host)) != 0 || s_ctx.port != new_port) {
        if (s_ctx.conn == SWS_STATE_CONNECTED) {
            PR_INFO("[signal_ws] bridge target changed, reconnecting");
            __disconnect();
        }
        snprintf(s_ctx.host, sizeof(s_ctx.host), "%s", new_host);
        s_ctx.port = new_port;
    }
}

/**
 * @brief Open TCP connection and upgrade to WebSocket
 * @return OPRT_OK on success
 */
static OPERATE_RET __connect(void)
{
    TUYA_IP_ADDR_T ip = 0;
    int fd = -1;
    OPERATE_RET rt = OPRT_OK;

    ip = tal_net_str2addr(s_ctx.host);
    if (ip == 0) {
        rt = tal_net_gethostbyname(s_ctx.host, &ip);
        if (rt != OPRT_OK || ip == 0) {
            PR_WARN("[signal_ws] DNS failed host=%s", s_ctx.host);
            return OPRT_NETWORK_ERROR;
        }
    }

    fd = tal_net_socket_create(PROTOCOL_TCP);
    if (fd < 0) {
        return OPRT_SOCK_ERR;
    }

    rt = tal_net_connect(fd, ip, s_ctx.port);
    if (rt != OPRT_OK) {
        tal_net_close(fd);
        return OPRT_NETWORK_ERROR;
    }

    rt = __ws_upgrade(fd);
    if (rt != OPRT_OK) {
        tal_net_close(fd);
        return rt;
    }

    s_ctx.fd = fd;
    s_ctx.conn = SWS_STATE_CONNECTED;
    s_ctx.rx_len = 0;
    s_ctx.idle_ticks = 0;
    return OPRT_OK;
}

/**
 * @brief Close WebSocket session
 * @return none
 */
static void __disconnect(void)
{
    if (s_ctx.fd >= 0) {
        tal_net_close(s_ctx.fd);
        s_ctx.fd = -1;
    }
    s_ctx.conn = SWS_STATE_DISCONNECTED;
    s_ctx.rx_len = 0;
}

/**
 * @brief WebSocket client background task
 * @param[in] arg unused
 * @return none
 */
static void __sws_task(void *arg)
{
    (void)arg;

    PR_INFO("[signal_ws] task started host=%s port=%u", s_ctx.host, (unsigned)s_ctx.port);

    while (!s_ctx.stop) {
        if (!__wifi_link_up()) {
            if (s_ctx.conn == SWS_STATE_CONNECTED) {
                __disconnect();
            }
            tal_system_sleep(SWS_WIFI_POLL_MS);
            continue;
        }

        __resolve_bridge_target();

        if (s_ctx.conn == SWS_STATE_DISCONNECTED) {
            if (__connect() != OPRT_OK) {
                tal_system_sleep(SWS_RECONNECT_MS);
                continue;
            }
        }

        {
            TUYA_FD_SET_T rfds;

            TAL_FD_ZERO(&rfds);
            TAL_FD_SET(s_ctx.fd, &rfds);
            int ready = tal_net_select(s_ctx.fd + 1, &rfds, NULL, NULL, SWS_SELECT_MS);
            if (ready <= 0) {
                s_ctx.idle_ticks++;
                if (s_ctx.idle_ticks >= SWS_IDLE_MAX_TICKS) {
                    PR_WARN("[signal_ws] idle timeout, reconnecting");
                    s_ctx.idle_ticks = 0;
                    __disconnect();
                }
                continue;
            }
            if (!TAL_FD_ISSET(s_ctx.fd, &rfds)) {
                continue;
            }
        }

        s_ctx.idle_ticks = 0;

        if (s_ctx.rx_len >= SWS_RX_BUF_SIZE) {
            PR_WARN("[signal_ws] rx overflow, reconnecting");
            __disconnect();
            continue;
        }

        {
            int n = tal_net_recv(s_ctx.fd, s_ctx.rx_buf + s_ctx.rx_len,
                                 (uint32_t)(SWS_RX_BUF_SIZE - s_ctx.rx_len));
            if (n == OPRT_RESOURCE_NOT_READY) {
                continue;
            }
            if (n <= 0) {
                PR_WARN("[signal_ws] connection closed");
                __disconnect();
                continue;
            }
            s_ctx.rx_len += (size_t)n;
        }

        while (s_ctx.rx_len > 0 && s_ctx.conn == SWS_STATE_CONNECTED) {
            uint8_t opcode = 0;
            const uint8_t *payload = NULL;
            size_t plen = 0;
            size_t consumed = 0;
            OPERATE_RET rt = __ws_decode_frame(s_ctx.rx_buf, s_ctx.rx_len, &opcode, &payload, &plen,
                                               &consumed);

            if (rt == OPRT_RESOURCE_NOT_READY) {
                break;
            }
            if (rt != OPRT_OK) {
                __disconnect();
                break;
            }

            switch (opcode) {
            case WS_OP_TEXT:
                __feed_text_payload(payload, plen);
                break;
            case WS_OP_PING:
                tal_mutex_lock(s_ctx.tx_mutex);
                (void)__ws_send_masked(s_ctx.fd, WS_OP_PONG, payload, plen);
                tal_mutex_unlock(s_ctx.tx_mutex);
                break;
            case WS_OP_CLOSE:
                __disconnect();
                break;
            default:
                break;
            }

            __rx_consume(consumed);
        }
    }

    __disconnect();
    PR_INFO("[signal_ws] task stopped");
}

/**
 * @brief Initialize WebSocket client task
 * @param[in] host bridge server IP or hostname
 * @param[in] port bridge server TCP port
 * @param[in] cb callback for incoming text lines
 * @param[in] user user pointer passed to cb
 * @return OPRT_OK on success
 */
OPERATE_RET signal_ws_init(const char *host, uint16_t port, signal_ws_rx_cb_t cb, void *user)
{
    OPERATE_RET rt = OPRT_OK;
    THREAD_CFG_T thrd = {0};

    if (host == NULL || cb == NULL || host[0] == '\0') {
        return OPRT_INVALID_PARM;
    }

    memset(&s_ctx, 0, sizeof(s_ctx));
    s_ctx.fd = -1;
    s_ctx.conn = SWS_STATE_DISCONNECTED;
    strncpy(s_ctx.fallback_host, host, sizeof(s_ctx.fallback_host) - 1);
    s_ctx.fallback_port = port;
    strncpy(s_ctx.host, host, sizeof(s_ctx.host) - 1);
    s_ctx.port = port;
    s_ctx.rx_cb = cb;
    s_ctx.rx_user = user;

    rt = tal_mutex_create_init(&s_ctx.tx_mutex);
    if (rt != OPRT_OK) {
        return rt;
    }

    thrd.stackDepth = 6144;
    thrd.priority = THREAD_PRIO_2;
    thrd.thrdname = "signal_ws";
    thrd.psram_mode = 0;

    s_ctx.stop = false;
    rt = tal_thread_create_and_start(&s_ctx.thread, NULL, NULL, __sws_task, NULL, &thrd);
    if (rt != OPRT_OK) {
        PR_ERR("[signal_ws] thread create failed: %d", rt);
        return rt;
    }

    return OPRT_OK;
}

/**
 * @brief Send a text line to the bridge
 * @param[in] data line bytes
 * @param[in] len data length
 * @return OPRT_OK on success
 */
OPERATE_RET signal_ws_send(const uint8_t *data, uint32_t len)
{
    OPERATE_RET rt = OPRT_OK;

    if (data == NULL || len == 0) {
        return OPRT_INVALID_PARM;
    }
    if (s_ctx.conn != SWS_STATE_CONNECTED || s_ctx.fd < 0) {
        return OPRT_RESOURCE_NOT_READY;
    }
    if (len > 1024) {
        return OPRT_INVALID_PARM;
    }

    tal_mutex_lock(s_ctx.tx_mutex);
    rt = __ws_send_masked(s_ctx.fd, WS_OP_TEXT, data, len);
    tal_mutex_unlock(s_ctx.tx_mutex);
    return rt;
}

/**
 * @brief Returns true when WebSocket session is established
 * @return connection flag
 */
bool signal_ws_is_connected(void)
{
    return (s_ctx.conn == SWS_STATE_CONNECTED);
}
