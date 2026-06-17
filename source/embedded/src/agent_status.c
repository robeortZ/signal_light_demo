/**
 * @file agent_status.c
 * @brief Agent work status model implementation
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#include "agent_status.h"
#include "tal_api.h"
#include <string.h>
#include <ctype.h>

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static AGENT_STATUS_E s_status = AGENT_STATUS_IDLE;
static MUTEX_HANDLE s_mutex = NULL;

static const char *const s_status_names[] = {
    "idle",
    "working",
    "attention",
    "urgent",
};

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */
/**
 * @brief Initialize agent status module
 * @return OPRT_OK on success
 */
OPERATE_RET agent_status_init(void)
{
    if (s_mutex == NULL) {
        if (tal_mutex_create_init(&s_mutex) != OPRT_OK) {
            PR_ERR("[agent] mutex create failed");
            return OPRT_MALLOC_FAILED;
        }
    }
    s_status = AGENT_STATUS_IDLE;
    PR_INFO("[agent] status module ready, default=idle");
    return OPRT_OK;
}

/**
 * @brief Set current agent status
 * @param[in] status new status
 * @param[in] source update source for logging
 * @return OPRT_OK on success, OPRT_INVALID_PARM if status invalid
 */
OPERATE_RET agent_status_set(AGENT_STATUS_E status, SIGNAL_SOURCE_E source)
{
    const char *src_name = "unknown";

    if (status >= AGENT_STATUS_MAX) {
        PR_WARN("[agent] invalid status %d", status);
        return OPRT_INVALID_PARM;
    }

    if (s_mutex) {
        tal_mutex_lock(s_mutex);
    }

    if (s_status != status) {
        s_status = status;
        switch (source) {
        case SIGNAL_SRC_BLE:
            src_name = "ble";
            break;
        case SIGNAL_SRC_UART:
            src_name = "uart";
            break;
        case SIGNAL_SRC_WS:
            src_name = "ws";
            break;
        case SIGNAL_SRC_CLI:
            src_name = "cli";
            break;
        case SIGNAL_SRC_MOCK:
            src_name = "mock";
            break;
        default:
            break;
        }
        PR_INFO("[agent] status -> %s (src=%s)", agent_status_name(status), src_name);
    }

    if (s_mutex) {
        tal_mutex_unlock(s_mutex);
    }

    return OPRT_OK;
}

/**
 * @brief Get current agent status
 * @return current status
 */
AGENT_STATUS_E agent_status_get(void)
{
    AGENT_STATUS_E status = AGENT_STATUS_IDLE;

    if (s_mutex) {
        tal_mutex_lock(s_mutex);
    }
    status = s_status;
    if (s_mutex) {
        tal_mutex_unlock(s_mutex);
    }

    return status;
}

/**
 * @brief Get human-readable status name
 * @param[in] status status value
 * @return status name string
 */
const char *agent_status_name(AGENT_STATUS_E status)
{
    if (status >= AGENT_STATUS_MAX) {
        return "unknown";
    }
    return s_status_names[status];
}

/**
 * @brief Compare buffer with keyword (case-insensitive)
 * @param[in] data input buffer
 * @param[in] len data length
 * @param[in] keyword keyword to match
 * @return true if matched
 */
static bool __match_keyword(const uint8_t *data, uint32_t len, const char *keyword)
{
    uint32_t kw_len = 0;
    uint32_t i = 0;

    if (data == NULL || keyword == NULL || len == 0) {
        return false;
    }

    kw_len = (uint32_t)strlen(keyword);
    if (len < kw_len) {
        return false;
    }

    for (i = 0; i < kw_len; i++) {
        if ((char)tolower((int)data[i]) != keyword[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Parse status from text or single-byte payload
 * @param[in] data input buffer
 * @param[in] len data length
 * @param[out] status parsed status
 * @return OPRT_OK if parsed, OPRT_INVALID_PARM otherwise
 */
OPERATE_RET agent_status_parse(const uint8_t *data, uint32_t len, AGENT_STATUS_E *status)
{
    uint32_t i = 0;
    uint32_t start = 0;

    if (data == NULL || status == NULL || len == 0) {
        return OPRT_INVALID_PARM;
    }

    while (start < len && (data[start] == ' ' || data[start] == '\t')) {
        start++;
    }
    if (start >= len) {
        return OPRT_INVALID_PARM;
    }

    /* Single byte 0-3 */
    if (len - start == 1 && data[start] >= '0' && data[start] <= '3') {
        *status = (AGENT_STATUS_E)(data[start] - '0');
        return OPRT_OK;
    }

    /* Skip optional "SIGNAL" prefix */
    if (__match_keyword(data + start, len - start, "signal")) {
        start += 6;
        while (start < len && (data[start] == ' ' || data[start] == ':' || data[start] == '\t')) {
            start++;
        }
    }

    if (start >= len) {
        return OPRT_INVALID_PARM;
    }

    for (i = 0; i < AGENT_STATUS_MAX; i++) {
        if (__match_keyword(data + start, len - start, s_status_names[i])) {
            *status = (AGENT_STATUS_E)i;
            return OPRT_OK;
        }
    }

    PR_DEBUG("[agent] parse failed, len=%u first='%c'", len, data[start]);
    return OPRT_INVALID_PARM;
}
