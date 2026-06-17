/**
 * @file signal_discovery.h
 * @brief UDP LAN discovery of PC signal bridge (beacon listener)
 * @version 1.0
 * @date 2026-06-16
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_DISCOVERY_H__
#define __SIGNAL_DISCOVERY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Start UDP discovery listener task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_discovery_init(void);

/**
 * @brief Get latest bridge endpoint from UDP beacons
 * @param[out] host IP string buffer
 * @param[in] host_len host buffer size
 * @param[out] port WebSocket TCP port
 * @return true if a fresh beacon was received within TTL
 */
bool signal_discovery_get_bridge(char *host, size_t host_len, uint16_t *port);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_DISCOVERY_H__ */
