/**
 * @file signal_config.h
 * @brief Hardware pin and feature configuration for signal light demo
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#ifndef __SIGNAL_CONFIG_H__
#define __SIGNAL_CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"

/* 0 = Tuya IoT ble_mgr handles BLE for APP netcfg; status via UART on Mac */
#define SIGNAL_TRANSPORT_BLE_ENABLE 0

/* ---------------------------------------------------------------------------
 * LEDs — active LOW (GPIO low = ON), GPIO 5/6/7 = green / yellow / red
 * --------------------------------------------------------------------------- */
#define SIGNAL_LED_GPIO_GREEN    TUYA_GPIO_NUM_5
#define SIGNAL_LED_GPIO_YELLOW   TUYA_GPIO_NUM_6
#define SIGNAL_LED_GPIO_RED      TUYA_GPIO_NUM_7
#define SIGNAL_LED_ACTIVE_LV     TUYA_GPIO_LEVEL_LOW
#define SIGNAL_LED_INACTIVE_LV   TUYA_GPIO_LEVEL_HIGH

/* ---------------------------------------------------------------------------
 * SSD1306 0.96" OLED — I2C0 on GPIO 20 (SCL) / 21 (SDA)
 * --------------------------------------------------------------------------- */
#define SIGNAL_OLED_I2C_PORT   TUYA_I2C_NUM_1
#define SIGNAL_OLED_I2C_SCL    TUYA_IO_PIN_14
#define SIGNAL_OLED_I2C_SDA    TUYA_IO_PIN_15
#define SIGNAL_OLED_I2C_ADDR   0x3C
#define SIGNAL_OLED_WIDTH      128
#define SIGNAL_OLED_HEIGHT     64
#define SIGNAL_DISPLAY_NAME    "signal_oled"

/* ---------------------------------------------------------------------------
 * Agent status UART — UART0 (GPIO10 RX / GPIO11 TX), shared with tal_cli
 * T5AI CORE: USB 烧录通道对应 UART0，115200；调试日志在另一路 USB（460800）。
 * PC 发: SIGNAL working\\n  或 CLI: signal set working
 * --------------------------------------------------------------------------- */
#define SIGNAL_UART_PORT             TUYA_UART_NUM_0
#define SIGNAL_UART_BAUDRATE         115200
#define SIGNAL_UART_RX_BUF           256
#define SIGNAL_UART_SHARE_WITH_CLI   1

/* UART idle fallback when PC stops sending (ms) */
#define SIGNAL_UART_LINK_TIMEOUT_MS 2000

/* LED pattern timing (ms) */
#define SIGNAL_LED_SLOW_CYCLE_MS    500
#define SIGNAL_LED_BLINK_HALF_MS    250
#define SIGNAL_LED_TASK_PERIOD_MS   50

/* OLED refresh */
#define SIGNAL_OLED_REFRESH_MS      200

/* IDLE for this long → main line shows local HH:MM instead of "IDLE" */
#define SIGNAL_OLED_IDLE_CLOCK_MS   60000

/* Unused — main OLED line shows DISCONNECT immediately when PC bridge is offline */
#define SIGNAL_OLED_DISCONNECT_TAG_MS 60000

/* ---------------------------------------------------------------------------
 * LAN WebSocket bridge — device connects to PC on same WiFi
 * Set SIGNAL_WS_BRIDGE_HOST to your Mac/PC LAN IP (see tools/signal_bridge_config.json)
 * --------------------------------------------------------------------------- */
#define SIGNAL_TRANSPORT_WS_ENABLE  1

#ifndef CONFIG_SIGNAL_BRIDGE_HOST
#define CONFIG_SIGNAL_BRIDGE_HOST   "192.168.1.100"
#endif
#ifndef CONFIG_SIGNAL_BRIDGE_PORT
#define CONFIG_SIGNAL_BRIDGE_PORT   8765
#endif

#define SIGNAL_WS_BRIDGE_HOST       CONFIG_SIGNAL_BRIDGE_HOST
#define SIGNAL_WS_BRIDGE_PORT       ((uint16_t)CONFIG_SIGNAL_BRIDGE_PORT)
#define SIGNAL_WS_LINK_TIMEOUT_MS   5000
#define SIGNAL_LINE_MAX             64

/* UDP LAN discovery — PC run_bridge.sh broadcasts bridge IP (port from Kconfig) */
#ifndef SIGNAL_DISCOVERY_PORT
#define SIGNAL_DISCOVERY_PORT       8766
#endif
#define SIGNAL_DISCOVERY_TTL_MS     15000

/* ---------------------------------------------------------------------------
 * P17 external button — TDD/TDL name "signal_voice_button" (signal_voice_key.c)
 * --------------------------------------------------------------------------- */
#define SIGNAL_VOICE_BUTTON_NAME         "signal_voice_button"
#define SIGNAL_VOICE_HOTKEY_GPIO         TUYA_GPIO_NUM_17
#define SIGNAL_VOICE_HOTKEY_ACTIVE_LV    TUYA_GPIO_LEVEL_HIGH
#define SIGNAL_VOICE_KEY_LONG_PRESS_MS   400
#define SIGNAL_VOICE_KEY_DOUBLE_CLICK_MS 300

#ifdef __cplusplus
}
#endif

#endif /* __SIGNAL_CONFIG_H__ */
