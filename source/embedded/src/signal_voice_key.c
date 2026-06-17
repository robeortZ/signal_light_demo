/**
 * @file signal_voice_key.c
 * @brief P17 TDL: single-click hotkey, double-click Enter, long-press hold-to-talk
 * @version 1.5
 * @date 2026-06-17
 * @copyright Copyright (c) Tuya Inc.
 */
#include "signal_voice_key.h"
#include "signal_config.h"
#include "signal_transport.h"
#include "tal_api.h"

#if defined(ENABLE_BUTTON) && (ENABLE_BUTTON == 1)

#include "ai_manage_mode.h"
#include "tdl_button_manage.h"
#include "tdd_button_gpio.h"

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static TDL_BUTTON_HANDLE s_voice_button = NULL;
static bool s_cloud_hold_active = false;

/**
 * @brief Forward button event to AI hold-talk handler (P29 equivalent)
 * @param[in] event TDL button event
 * @return none
 */
static void __voice_key_cloud_event(TDL_BUTTON_TOUCH_EVENT_E event)
{
    OPERATE_RET rt = ai_mode_handle_key(event, NULL);
    if (rt != OPRT_OK) {
        PR_DEBUG("[voice_key] ai_mode_handle_key(%d) -> %d", event, rt);
    }
}

/**
 * @brief Send one PC hotkey pulse (VOICE down then up)
 * @return none
 */
static void __voice_key_hotkey_pulse(void)
{
    signal_transport_send_voice_key(true);
    signal_transport_send_voice_key(false);
    PR_DEBUG("[voice_key] PC hotkey pulse");
}

/**
 * @brief TDL button callback for P17
 * @param[in] name button name
 * @param[in] event button event
 * @param[in] arg repeat count or hold time (driver-specific)
 * @return none
 */
static void __voice_key_button_cb(char *name, TDL_BUTTON_TOUCH_EVENT_E event, void *arg)
{
    (void)name;
    (void)arg;

    switch (event) {
    case TDL_BUTTON_PRESS_SINGLE_CLICK:
        __voice_key_hotkey_pulse();
        break;

    case TDL_BUTTON_PRESS_DOUBLE_CLICK:
        signal_transport_send_enter_key();
        PR_DEBUG("[voice_key] PC Enter");
        break;

    case TDL_BUTTON_LONG_PRESS_START:
        s_cloud_hold_active = true;
        PR_DEBUG("[voice_key] cloud hold-talk start");
        __voice_key_cloud_event(TDL_BUTTON_LONG_PRESS_START);
        break;

    case TDL_BUTTON_PRESS_UP:
        if (s_cloud_hold_active) {
            s_cloud_hold_active = false;
            PR_DEBUG("[voice_key] cloud hold-talk stop");
            __voice_key_cloud_event(TDL_BUTTON_PRESS_UP);
        }
        break;

    default:
        break;
    }
}

/**
 * @brief Register GPIO17 with TDD and create TDL button instance
 * @return OPRT_OK on success
 */
static OPERATE_RET __voice_key_button_register(void)
{
    OPERATE_RET rt = OPRT_OK;
    BUTTON_GPIO_CFG_T hw_cfg = {
        .pin = SIGNAL_VOICE_HOTKEY_GPIO,
        .level = SIGNAL_VOICE_HOTKEY_ACTIVE_LV,
        .mode = BUTTON_IRQ_MODE,
        .pin_type.irq_edge = TUYA_GPIO_IRQ_FALL,
    };
    TDL_BUTTON_CFG_T btn_cfg = {
        .long_start_valid_time = SIGNAL_VOICE_KEY_LONG_PRESS_MS,
        .long_keep_timer = 0,
        .button_debounce_time = 50,
        .button_repeat_valid_count = 2,
        .button_repeat_valid_time = SIGNAL_VOICE_KEY_DOUBLE_CLICK_MS,
    };

    TUYA_CALL_ERR_RETURN(tdd_gpio_button_register(SIGNAL_VOICE_BUTTON_NAME, &hw_cfg));
    TUYA_CALL_ERR_RETURN(tdl_button_create(SIGNAL_VOICE_BUTTON_NAME, &btn_cfg, &s_voice_button));

    tdl_button_event_register(s_voice_button, TDL_BUTTON_PRESS_SINGLE_CLICK, __voice_key_button_cb);
    tdl_button_event_register(s_voice_button, TDL_BUTTON_PRESS_DOUBLE_CLICK, __voice_key_button_cb);
    tdl_button_event_register(s_voice_button, TDL_BUTTON_LONG_PRESS_START, __voice_key_button_cb);
    tdl_button_event_register(s_voice_button, TDL_BUTTON_PRESS_UP, __voice_key_button_cb);

    return rt;
}

#endif /* ENABLE_BUTTON */

/**
 * @brief Init P17 TDL button
 * @return OPRT_OK on success
 */
OPERATE_RET signal_voice_key_init(void)
{
#if defined(ENABLE_BUTTON) && (ENABLE_BUTTON == 1)
    OPERATE_RET rt = OPRT_OK;

    if (s_voice_button != NULL) {
        return OPRT_OK;
    }

    PR_INFO("[voice_key] P17: click=hotkey | dbl-click=Enter | long-press=cloud talk");

    TUYA_CALL_ERR_RETURN(__voice_key_button_register());
    return rt;
#else
    PR_WARN("[voice_key] ENABLE_BUTTON off, P17 not initialized");
    return OPRT_NOT_SUPPORTED;
#endif
}
