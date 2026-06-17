/**
 * @file signal_oled.h
 * @brief SSD1306 OLED UI for agent status and link icons
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_OLED_H__
#define __SIGNAL_OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "stdbool.h"

/**
 * @brief Link icon state for OLED header
 */
typedef struct {
    bool ble_connected;
    bool uart_active;
    bool ws_active;
} SIGNAL_OLED_LINK_T;

/**
 * @brief Initialize SSD1306 display and start refresh task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_oled_init(void);

/**
 * @brief Update link state used for top-right BT/UART tag on OLED
 * @param[in] link link state
 * @return OPRT_OK on success
 */
OPERATE_RET signal_oled_set_link(const SIGNAL_OLED_LINK_T *link);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_OLED_H__ */
