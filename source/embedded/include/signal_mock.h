/**
 * @file signal_mock.h
 * @brief CLI mock test for agent status patterns
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_MOCK_H__
#define __SIGNAL_MOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

/**
 * @brief Register CLI commands and optional auto-cycle mock task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_mock_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_MOCK_H__ */
