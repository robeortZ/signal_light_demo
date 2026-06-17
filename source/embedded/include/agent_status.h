/**
 * @file agent_status.h
 * @brief Agent work status model for Cursor / Claude signal light
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __AGENT_STATUS_H__
#define __AGENT_STATUS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

/* ---------------------------------------------------------------------------
 * Type definitions
 * --------------------------------------------------------------------------- */
typedef enum {
    AGENT_STATUS_IDLE = 0,
    AGENT_STATUS_WORKING,
    AGENT_STATUS_ATTENTION,
    AGENT_STATUS_URGENT,
    AGENT_STATUS_MAX
} AGENT_STATUS_E;

typedef enum {
    SIGNAL_SRC_NONE = 0,
    SIGNAL_SRC_BLE,
    SIGNAL_SRC_UART,
    SIGNAL_SRC_WS,
    SIGNAL_SRC_CLI,
    SIGNAL_SRC_MOCK
} SIGNAL_SOURCE_E;

/* ---------------------------------------------------------------------------
 * Function declarations
 * --------------------------------------------------------------------------- */
/**
 * @brief Initialize agent status module
 * @return OPRT_OK on success
 */
OPERATE_RET agent_status_init(void);

/**
 * @brief Set current agent status
 * @param[in] status new status
 * @param[in] source update source for logging
 * @return OPRT_OK on success, OPRT_INVALID_PARM if status invalid
 */
OPERATE_RET agent_status_set(AGENT_STATUS_E status, SIGNAL_SOURCE_E source);

/**
 * @brief Get current agent status
 * @return current status
 */
AGENT_STATUS_E agent_status_get(void);

/**
 * @brief Get human-readable status name
 * @param[in] status status value
 * @return status name string
 */
const char *agent_status_name(AGENT_STATUS_E status);

/**
 * @brief Parse status from text or single-byte payload
 * @param[in] data input buffer
 * @param[in] len data length
 * @param[out] status parsed status
 * @return OPRT_OK if parsed, OPRT_INVALID_PARM otherwise
 */
OPERATE_RET agent_status_parse(const uint8_t *data, uint32_t len, AGENT_STATUS_E *status);

#ifdef __cplusplus
}
#endif

#endif /* __AGENT_STATUS_H__ */
