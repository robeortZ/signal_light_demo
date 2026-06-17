/**
 * @file signal_discovery.c
 * @brief UDP LAN discovery listener for PC signal bridge beacons
 * @version 1.0
 * @date 2026-06-16
 * @copyright Copyright (c) Tuya Inc.
 */
#include "signal_discovery.h"
#include "signal_config.h"
#include "tal_api.h"
#include "tal_network.h"
#include "netmgr.h"
#include <stdio.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define SDISC_RX_BUF_SIZE     128
#define SDISC_POLL_MS         500
#define SDISC_WIFI_POLL_MS    2000
#define SDISC_BEACON_PREFIX   "SIGNAL_BRIDGE"

/* ---------------------------------------------------------------------------
 * Type definitions
 * --------------------------------------------------------------------------- */
typedef struct {
    char host[64];
    uint16_t port;
    uint32_t last_seen_ms;
    bool valid;
} SDISC_BRIDGE_T;

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static SDISC_BRIDGE_T s_bridge = {0};
static MUTEX_HANDLE s_mutex = NULL;
static THREAD_HANDLE s_thread = NULL;
static volatile bool s_stop = false;

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
 * @brief Parse beacon text: SIGNAL_BRIDGE ip=x.x.x.x port=8765
 * @param[in] buf received datagram
 * @param[in] len datagram length
 * @param[out] host parsed IP string
 * @param[in] host_len host buffer size
 * @param[out] port parsed TCP port
 * @return true if parsed successfully
 */
static bool __parse_beacon(const char *buf, size_t len, char *host, size_t host_len, uint16_t *port)
{
    char tmp[SDISC_RX_BUF_SIZE] = {0};
    char ip[32] = {0};
    unsigned int ws_port = 0;
    int matched = 0;

    if (buf == NULL || host == NULL || port == NULL || host_len < 8 || len == 0) {
        return false;
    }

    if (len >= sizeof(tmp)) {
        len = sizeof(tmp) - 1;
    }
    memcpy(tmp, buf, len);
    tmp[len] = '\0';

    if (strstr(tmp, SDISC_BEACON_PREFIX) == NULL) {
        return false;
    }

    matched = sscanf(tmp, SDISC_BEACON_PREFIX " ip=%31s port=%u", ip, &ws_port);
    if (matched != 2 || ws_port == 0 || ws_port > 65535) {
        return false;
    }

    snprintf(host, host_len, "%s", ip);
    *port = (uint16_t)ws_port;
    return true;
}

/**
 * @brief Store parsed bridge endpoint
 * @param[in] host bridge IP
 * @param[in] port bridge TCP port
 * @return none
 */
static void __store_bridge(const char *host, uint16_t port)
{
    if (host == NULL || host[0] == '\0' || s_mutex == NULL) {
        return;
    }

    tal_mutex_lock(s_mutex);
    snprintf(s_bridge.host, sizeof(s_bridge.host), "%s", host);
    s_bridge.port = port;
    s_bridge.last_seen_ms = tal_system_get_millisecond();
    s_bridge.valid = true;
    tal_mutex_unlock(s_mutex);

    PR_INFO("[discovery] bridge %s:%u", host, (unsigned)port);
}

/**
 * @brief UDP discovery background task
 * @param[in] arg unused
 * @return none
 */
static void __discovery_task(void *arg)
{
    int fd = -1;
    uint8_t rx[SDISC_RX_BUF_SIZE];

    (void)arg;
    PR_INFO("[discovery] listening UDP :%u", (unsigned)SIGNAL_DISCOVERY_PORT);

    while (!s_stop) {
        if (!__wifi_link_up()) {
            if (fd >= 0) {
                tal_net_close(fd);
                fd = -1;
            }
            tal_system_sleep(SDISC_WIFI_POLL_MS);
            continue;
        }

        if (fd < 0) {
            fd = tal_net_socket_create(PROTOCOL_UDP);
            if (fd < 0) {
                tal_system_sleep(SDISC_WIFI_POLL_MS);
                continue;
            }

            if (tal_net_bind(fd, 0, SIGNAL_DISCOVERY_PORT) != OPRT_OK) {
                PR_WARN("[discovery] bind :%u failed", (unsigned)SIGNAL_DISCOVERY_PORT);
                tal_net_close(fd);
                fd = -1;
                tal_system_sleep(SDISC_WIFI_POLL_MS);
                continue;
            }
        }

        {
            TUYA_FD_SET_T rfds;
            TAL_FD_ZERO(&rfds);
            TAL_FD_SET(fd, &rfds);
            int ready = tal_net_select(fd + 1, &rfds, NULL, NULL, SDISC_POLL_MS);
            if (ready <= 0 || !TAL_FD_ISSET(fd, &rfds)) {
                continue;
            }
        }

        {
            TUYA_IP_ADDR_T from_addr = 0;
            uint16_t from_port = 0;
            int n = tal_net_recvfrom(fd, rx, sizeof(rx) - 1, &from_addr, &from_port);
            char host[64] = {0};
            uint16_t port = 0;

            if (n <= 0) {
                continue;
            }
            rx[n] = '\0';

            if (__parse_beacon((const char *)rx, (size_t)n, host, sizeof(host), &port)) {
                __store_bridge(host, port);
            }
        }
    }

    if (fd >= 0) {
        tal_net_close(fd);
    }
    PR_INFO("[discovery] task stopped");
}

/**
 * @brief Start UDP discovery listener task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_discovery_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    THREAD_CFG_T thrd = {0};

    if (s_thread != NULL) {
        return OPRT_OK;
    }

    rt = tal_mutex_create_init(&s_mutex);
    if (rt != OPRT_OK) {
        return rt;
    }

    s_stop = false;
    thrd.stackDepth = 3072;
    thrd.priority = THREAD_PRIO_2;
    thrd.thrdname = "signal_disc";
    thrd.psram_mode = 0;

    rt = tal_thread_create_and_start(&s_thread, NULL, NULL, __discovery_task, NULL, &thrd);
    if (rt != OPRT_OK) {
        PR_ERR("[discovery] thread create failed: %d", rt);
        return rt;
    }

    return OPRT_OK;
}

/**
 * @brief Get latest bridge endpoint from UDP beacons
 * @param[out] host IP string buffer
 * @param[in] host_len host buffer size
 * @param[out] port WebSocket TCP port
 * @return true if a fresh beacon was received within TTL
 */
bool signal_discovery_get_bridge(char *host, size_t host_len, uint16_t *port)
{
    bool ok = false;
    uint32_t now = tal_system_get_millisecond();

    if (host == NULL || port == NULL || host_len == 0 || s_mutex == NULL) {
        return false;
    }

    tal_mutex_lock(s_mutex);
    if (s_bridge.valid && (now - s_bridge.last_seen_ms) <= SIGNAL_DISCOVERY_TTL_MS) {
        snprintf(host, host_len, "%s", s_bridge.host);
        *port = s_bridge.port;
        ok = true;
    }
    tal_mutex_unlock(s_mutex);

    return ok;
}
