/**
 * @file signal_ws.h
 * @brief WebSocket client to PC signal bridge (plain TCP, RFC 6455)
 * @version 1.0
 * @date 2026-06-16
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_WS_H__
#define __SIGNAL_WS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include <stdbool.h>

/**
 * @brief Line received from bridge (newline-delimited text frame payload)
 * @param[in] line line buffer (not null-terminated unless len includes it)
 * @param[in] len line length in bytes
 * @param[in] user user pointer from signal_ws_init
 * @return none
 */
typedef void (*signal_ws_rx_cb_t)(const uint8_t *line, uint32_t len, void *user);

/**
 * @brief Initialize WebSocket client task (connects when WiFi is up)
 * @param[in] host bridge server IP or hostname
 * @param[in] port bridge server TCP port
 * @param[in] cb callback for incoming text lines
 * @param[in] user user pointer passed to cb
 * @return OPRT_OK on success
 */
OPERATE_RET signal_ws_init(const char *host, uint16_t port, signal_ws_rx_cb_t cb, void *user);

/**
 * @brief Send a text line to the bridge (thread-safe)
 * @param[in] data line bytes (newline optional)
 * @param[in] len data length
 * @return OPRT_OK on success, OPRT_RESOURCE_NOT_READY if not connected
 */
OPERATE_RET signal_ws_send(const uint8_t *data, uint32_t len);

/**
 * @brief Returns true when WebSocket session is established
 * @return connection flag
 */
bool signal_ws_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_WS_H__ */
