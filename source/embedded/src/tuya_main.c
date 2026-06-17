/**
 * @file tuya_main.c
 * @brief Signal light demo with Tuya IoT netcfg + AI chat (your_chat_bot stack)
 * @version 1.0.5
 * @date 2026-06-15
 * @copyright Copyright (c) Tuya Inc.
 */
#include "tuya_cloud_types.h"

#include <assert.h>
#include "cJSON.h"
#include "tal_api.h"
#include "tuya_config.h"
#include "tuya_iot.h"
#include "netmgr.h"
#include "tkl_output.h"
#include "tal_cli.h"
#include "tuya_authorize.h"

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "netconn_wifi.h"
#endif
#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
#include "lwip_init.h"
#endif

#include "board_com_api.h"
#include "reset_netcfg.h"
#include "app_chat_bot.h"

#if defined(ENABLE_COMP_AI_AUDIO) && (ENABLE_COMP_AI_AUDIO == 1)
#include "ai_audio_player.h"
#endif

#include "agent_status.h"
#include "signal_config.h"
#include "signal_led.h"
#include "signal_oled.h"
#include "signal_transport.h"
#include "signal_voice_key.h"
#include "signal_mock.h"

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static tuya_iot_client_t s_iot_client;
static tuya_iot_license_t s_license;

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */
/**
 * @brief Tuya IoT event handler
 * @param[in] client IoT client
 * @param[in] event event message
 * @return none
 */
static void user_event_handler_on(tuya_iot_client_t *client, tuya_event_msg_t *event)
{
    (void)client;

    PR_DEBUG("Tuya Event ID:%d(%s)", event->id, EVENT_ID2STR(event->id));

    switch (event->id) {
    case TUYA_EVENT_BIND_START:
        PR_INFO("Device bind start — open Smart Life, add device (PID %s)", TUYA_PRODUCT_ID);
#if defined(ENABLE_COMP_AI_AUDIO) && (ENABLE_COMP_AI_AUDIO == 1)
        ai_audio_player_alert(AI_AUDIO_ALERT_NETWORK_CFG);
#endif
        break;
    case TUYA_EVENT_MQTT_CONNECTED:
        PR_INFO("Cloud MQTT connected");
        break;
    default:
        break;
    }
}

/**
 * @brief Network link check for IoT stack
 * @return true if network is up
 */
static bool user_network_check(void)
{
    netmgr_status_e status = NETMGR_LINK_DOWN;

    netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &status);
    return status != NETMGR_LINK_DOWN;
}

/**
 * @brief Application entry after RTOS start
 * @return none
 */
static void user_main(void)
{
    OPERATE_RET rt = OPRT_OK;

#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    cJSON_InitHooks(&(cJSON_Hooks){.malloc_fn = tal_psram_malloc, .free_fn = tal_psram_free});
#else
    cJSON_InitHooks(&(cJSON_Hooks){.malloc_fn = tal_malloc, .free_fn = tal_free});
#endif

    tal_log_init(TAL_LOG_LEVEL_DEBUG, 2048, (TAL_LOG_OUTPUT_CB)tkl_log_output);

    PR_NOTICE("======== Signal Light Demo ========");
    PR_NOTICE("Project:  %s", PROJECT_NAME);
    PR_NOTICE("Version:  %s", PROJECT_VERSION);
    PR_NOTICE("Board:    %s", PLATFORM_BOARD);
    PR_NOTICE("PID:      %s", TUYA_PRODUCT_ID);
    PR_NOTICE("LED:      G=GPIO5 Y=GPIO6 R=GPIO7 (active-low)");
    PR_NOTICE("OLED:     SSD1306 I2C0 SCL=20 SDA=21");
    PR_NOTICE("Status:   LAN/WS + UART0 fallback (tal_cli SIGNAL)");
    PR_NOTICE("Bridge:   ws://<PC_IP>:%u/device (UDP discovery :%u, fallback %s)",
              (unsigned)SIGNAL_WS_BRIDGE_PORT, (unsigned)SIGNAL_DISCOVERY_PORT, SIGNAL_WS_BRIDGE_HOST);
    PR_NOTICE("AI chat:  hold GPIO29 -> cloud Agent -> LAN/WS AI reply");
    PR_NOTICE("Mac key:  hold GPIO17 -> LAN/WS VOICE down/up (Ctrl+Fn)");
    PR_NOTICE("Netcfg:   BLE+AP via Smart Life APP (RST x3 to reset)");

    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "vmlkasdh93dlvlcy",
        .key  = "dflfuap134ddlduq",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tal_time_service_init();
    tal_cli_init();
    tuya_authorize_init();
    reset_netconfig_start();

    if (OPRT_OK != tuya_authorize_read(&s_license)) {
        s_license.uuid = TUYA_OPENSDK_UUID;
        s_license.authkey = TUYA_OPENSDK_AUTHKEY;
        PR_WARN("Set UUID/AuthKey in tuya_config.h or authorize via IDE/tyutool");
    }

    rt = tuya_iot_init(&s_iot_client, &(const tuya_iot_config_t){
                                       .software_ver = PROJECT_VERSION,
                                       .productkey   = TUYA_PRODUCT_ID,
                                       .uuid         = s_license.uuid,
                                       .authkey      = s_license.authkey,
                                       .event_handler = user_event_handler_on,
                                       .network_check = user_network_check,
                                   });
    assert(rt == OPRT_OK);

#if defined(ENABLE_LIBLWIP) && (ENABLE_LIBLWIP == 1)
    TUYA_LwIP_Init();
#endif

    netmgr_type_e net_type = 0;
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    net_type |= NETCONN_WIFI;
#endif
    netmgr_init(net_type);
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_NETCFG,
                    &(netcfg_args_t){.type = NETCFG_TUYA_BLE | NETCFG_TUYA_WIFI_AP});
#endif

    rt = board_register_hardware();
    if (rt != OPRT_OK) {
        PR_ERR("board_register_hardware failed: %d", rt);
    }

    TUYA_CALL_ERR_GOTO(agent_status_init(), __EXIT);
    TUYA_CALL_ERR_GOTO(signal_led_init(), __EXIT);
    TUYA_CALL_ERR_GOTO(signal_oled_init(), __EXIT);
    TUYA_CALL_ERR_GOTO(signal_transport_init(), __EXIT);
    TUYA_CALL_ERR_GOTO(signal_voice_key_init(), __EXIT);

    rt = app_chat_bot_init();
    if (rt != OPRT_OK) {
        PR_ERR("app_chat_bot_init failed: %d", rt);
    }

    signal_mock_init();

    tuya_iot_start(&s_iot_client);
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
    tkl_wifi_set_lp_mode(0, 0);
#endif
    reset_netconfig_check();

    PR_NOTICE("Init done. Default status: idle (green)");

    while (1) {
        tuya_iot_yield(&s_iot_client);
    }

__EXIT:
    if (rt != OPRT_OK) {
        PR_ERR("user_main failed: %d", rt);
    }
}

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    user_main();
}
#else
static THREAD_HANDLE ty_app_thread = NULL;

/**
 * @brief Application thread wrapper
 * @param[in] arg unused
 * @return none
 */
static void tuya_app_thread(void *arg)
{
    (void)arg;
    user_main();
    tal_thread_delete(ty_app_thread);
    ty_app_thread = NULL;
}

/**
 * @brief TuyaOpen application entry
 * @return none
 */
void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {0};

    thrd_param.stackDepth = 8192;
    thrd_param.priority = THREAD_PRIO_1;
    thrd_param.thrdname = "tuya_app_main";
    thrd_param.psram_mode = 0;

    tal_thread_create_and_start(&ty_app_thread, NULL, NULL, tuya_app_thread, NULL, &thrd_param);
}
#endif
