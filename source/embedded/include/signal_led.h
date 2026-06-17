/**
 * @file signal_led.h
 * @brief RGB signal LED pattern driver
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_LED_H__
#define __SIGNAL_LED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

/**
 * @brief Initialize GPIO LEDs and start pattern task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_led_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_LED_H__ */
