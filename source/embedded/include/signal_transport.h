/**
 * @file signal_transport.h
 * @brief BLE and UART transport for agent status updates
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_TRANSPORT_H__
#define __SIGNAL_TRANSPORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"
#include "signal_oled.h"

/**
 * @brief Initialize BLE peripheral and UART0 (shared tal_cli) receiver
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_init(void);

/**
 * @brief Get current transport link state for OLED
 * @param[out] link link state output
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_get_link(SIGNAL_OLED_LINK_T *link);

/**
 * @brief Returns true when BLE central is connected
 * @return BLE connection flag
 */
bool signal_transport_ble_connected(void);

/**
 * @brief Send ASR text line to PC via UART0 (ASR <text>\\n)
 * @param[in] text UTF-8 text (not null-terminated if len given)
 * @param[in] len text length in bytes
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_send_asr(const char *text, uint32_t len);

/**
 * @brief Send voice key edge to PC (VOICE down\\n / VOICE up\\n) for hotkey bridge
 * @param[in] pressed true when GPIO29 pressed, false on release
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_send_voice_key(bool pressed);

/**
 * @brief Send Enter key pulse to PC (KEY enter\\n)
 * @return OPRT_OK on success
 */
OPERATE_RET signal_transport_send_enter_key(void);

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_TRANSPORT_H__ */
