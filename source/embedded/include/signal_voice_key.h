/**
 * @file signal_voice_key.h
 * @brief GPIO17 press/release notifier for PC voice hotkey bridge
 * @version 1.1
 * @date 2026-06-16
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_VOICE_KEY_H__
#define __SIGNAL_VOICE_KEY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

/**
 * @brief Init P17: single-click hotkey, double-click Enter, long-press cloud hold-to-talk
 * @return OPRT_OK on success
 */
OPERATE_RET signal_voice_key_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_VOICE_KEY_H__ */
