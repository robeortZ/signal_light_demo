/**
 * @file signal_led.c
 * @brief RGB signal LED pattern driver implementation
 * @version 1.0
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#include "signal_led.h"
#include "signal_config.h"
#include "agent_status.h"
#include "tal_api.h"
#include "tkl_gpio.h"
#include <stdbool.h>

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static THREAD_HANDLE s_led_thread = NULL;

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */
/**
 * @brief Configure one LED GPIO as output
 * @param[in] pin GPIO number
 * @return OPRT_OK on success
 */
static OPERATE_RET __led_gpio_init(TUYA_GPIO_NUM_E pin)
{
    TUYA_GPIO_BASE_CFG_T cfg = {
        .mode = TUYA_GPIO_PUSH_PULL,
        .direct = TUYA_GPIO_OUTPUT,
        .level = SIGNAL_LED_INACTIVE_LV,
    };

    return tkl_gpio_init(pin, &cfg);
}

/**
 * @brief Set LED on/off
 * @param[in] pin GPIO number
 * @param[in] on true to turn on
 * @return none
 */
static void __led_set(TUYA_GPIO_NUM_E pin, bool on)
{
    tkl_gpio_write(pin, on ? SIGNAL_LED_ACTIVE_LV : SIGNAL_LED_INACTIVE_LV);
}

/**
 * @brief Apply LED outputs for current tick
 * @param[in] status agent status
 * @param[in] tick_ms monotonic tick for animations
 * @return none
 */
static void __led_apply_pattern(AGENT_STATUS_E status, uint32_t tick_ms)
{
    bool green_on = false;
    bool yellow_on = false;
    bool red_on = false;
    uint32_t phase = 0;

    switch (status) {
    case AGENT_STATUS_IDLE:
        green_on = true;
        break;
    case AGENT_STATUS_WORKING:
        phase = (tick_ms / SIGNAL_LED_SLOW_CYCLE_MS) % 3;
        if (phase == 0) {
            green_on = true;
        } else if (phase == 1) {
            yellow_on = true;
        } else {
            red_on = true;
        }
        break;
    case AGENT_STATUS_ATTENTION:
        yellow_on = ((tick_ms / SIGNAL_LED_BLINK_HALF_MS) % 2) == 0;
        break;
    case AGENT_STATUS_URGENT:
        red_on = ((tick_ms / SIGNAL_LED_BLINK_HALF_MS) % 2) == 0;
        break;
    default:
        break;
    }

    __led_set(SIGNAL_LED_GPIO_GREEN, green_on);
    __led_set(SIGNAL_LED_GPIO_YELLOW, yellow_on);
    __led_set(SIGNAL_LED_GPIO_RED, red_on);
}

/**
 * @brief LED pattern task entry
 * @param[in] arg unused
 * @return none
 */
static void __signal_led_task(void *arg)
{
    uint32_t tick_ms = 0;

    (void)arg;
    PR_INFO("[led] pattern task started");

    while (1) {
        AGENT_STATUS_E status = agent_status_get();
        __led_apply_pattern(status, tick_ms);
        tick_ms += SIGNAL_LED_TASK_PERIOD_MS;
        tal_system_sleep(SIGNAL_LED_TASK_PERIOD_MS);
    }
}

/**
 * @brief Initialize GPIO LEDs and start pattern task
 * @return OPRT_OK on success
 */
OPERATE_RET signal_led_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    THREAD_CFG_T thrd = {0};

    rt = __led_gpio_init(SIGNAL_LED_GPIO_GREEN);
    if (rt != OPRT_OK) {
        PR_ERR("[led] green gpio init failed: %d", rt);
        return rt;
    }
    rt = __led_gpio_init(SIGNAL_LED_GPIO_YELLOW);
    if (rt != OPRT_OK) {
        PR_ERR("[led] yellow gpio init failed: %d", rt);
        return rt;
    }
    rt = __led_gpio_init(SIGNAL_LED_GPIO_RED);
    if (rt != OPRT_OK) {
        PR_ERR("[led] red gpio init failed: %d", rt);
        return rt;
    }

    __led_set(SIGNAL_LED_GPIO_GREEN, false);
    __led_set(SIGNAL_LED_GPIO_YELLOW, false);
    __led_set(SIGNAL_LED_GPIO_RED, false);

    thrd.stackDepth = 2048;
    thrd.priority = THREAD_PRIO_2;
    thrd.thrdname = "signal_led";
    thrd.psram_mode = 0;

    rt = tal_thread_create_and_start(&s_led_thread, NULL, NULL, __signal_led_task, NULL, &thrd);
    if (rt != OPRT_OK) {
        PR_ERR("[led] thread create failed: %d", rt);
        return rt;
    }

    PR_INFO("[led] GPIO5/6/7 ready (G/Y/R, active-low)");
    return OPRT_OK;
}
