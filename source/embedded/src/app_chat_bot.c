/**
 * @file app_chat_bot.c
 * @brief app_chat_bot module is used to
 * @version 0.1
 * @date 2025-03-25
 */

#include "tal_api.h"

#include "netmgr.h"
#include <string.h>

#include "ai_chat_main.h"
#include "app_chat_bot.h"
#include "signal_transport.h"
#include "ai_user_event.h"

#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
#include "tkl_wifi.h"
#endif

#if defined(ENABLE_PRINTER) && (ENABLE_PRINTER == 1)
#include "app_printer.h"
#endif

/***********************************************************
************************macro define************************
***********************************************************/
#define PRINTF_FREE_HEAP_TTIME (10 * 1000)
#define DISP_NET_STATUS_TIME   (1 * 1000)
/* Max AI reply forwarded to PC (matches signal_transport ASR line budget) */
#define AI_REPLY_BUF_MAX       512

/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
***********************const declaration********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
static TIMER_ID sg_printf_heap_tm;
static char sg_ai_reply_buf[AI_REPLY_BUF_MAX + 1];
static uint32_t sg_ai_reply_len = 0;

#if defined(ENABLE_COMP_AI_DISPLAY) && (ENABLE_COMP_AI_DISPLAY == 1)
static AI_UI_WIFI_STATUS_E sg_wifi_status = AI_UI_WIFI_STATUS_DISCONNECTED;
static TIMER_ID            sg_disp_status_tm;
#endif

/***********************************************************
***********************function define**********************
***********************************************************/
#if defined(ENABLE_COMP_AI_DISPLAY) && (ENABLE_COMP_AI_DISPLAY == 1)
extern void app_ui_action_register(void);
#endif

static void __printf_free_heap_tm_cb(TIMER_ID timer_id, void *arg)
{
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    uint32_t free_heap       = tal_system_get_free_heap_size();
    uint32_t free_psram_heap = tal_psram_get_free_heap_size();
    PR_INFO("Free heap size:%d, Free psram heap size:%d", free_heap, free_psram_heap);
#else
    uint32_t free_heap = tal_system_get_free_heap_size();
    PR_INFO("Free heap size:%d", free_heap);
#endif
}

#if defined(ENABLE_COMP_AI_DISPLAY) && (ENABLE_COMP_AI_DISPLAY == 1)
static void __display_net_status_update(void)
{
    AI_UI_WIFI_STATUS_E wifi_status = AI_UI_WIFI_STATUS_DISCONNECTED;
    netmgr_status_e     net_status  = NETMGR_LINK_DOWN;

    netmgr_conn_get(NETCONN_AUTO, NETCONN_CMD_STATUS, &net_status);
    if (net_status == NETMGR_LINK_UP) {
#if defined(ENABLE_WIFI) && (ENABLE_WIFI == 1)
        // get rssi
        int8_t rssi = 0;
#ifndef PLATFORM_T5
        // BUG: Getting RSSI causes a crash on T5 platform
        tkl_wifi_station_get_conn_ap_rssi(&rssi);
#endif
        if (rssi >= -60) {
            wifi_status = AI_UI_WIFI_STATUS_GOOD;
        } else if (rssi >= -70) {
            wifi_status = AI_UI_WIFI_STATUS_FAIR;
        } else {
            wifi_status = AI_UI_WIFI_STATUS_WEAK;
        }
#else
        wifi_status = AI_UI_WIFI_STATUS_GOOD;
#endif
    } else {
        wifi_status = AI_UI_WIFI_STATUS_DISCONNECTED;
    }

    if (wifi_status != sg_wifi_status) {
        sg_wifi_status = wifi_status;
        ai_ui_disp_msg(AI_UI_DISP_NETWORK, (uint8_t *)&wifi_status, sizeof(AI_UI_WIFI_STATUS_E));
    }
}

static void __display_status_tm_cb(TIMER_ID timer_id, void *arg)
{
    __display_net_status_update();
}

#endif

/**
 * @brief Clear accumulated AI reply buffer
 * @return none
 */
static void __ai_reply_buf_reset(void)
{
    sg_ai_reply_len = 0;
    sg_ai_reply_buf[0] = '\0';
}

/**
 * @brief Append NLG chunk to AI reply buffer
 * @param[in] data UTF-8 text chunk
 * @param[in] len chunk length in bytes
 * @return none
 */
static void __ai_reply_buf_append(const char *data, uint32_t len)
{
    uint32_t space = 0;

    if (data == NULL || len == 0) {
        return;
    }

    space = AI_REPLY_BUF_MAX - sg_ai_reply_len;
    if (space == 0) {
        return;
    }
    if (len > space) {
        len = space;
    }

    memcpy(sg_ai_reply_buf + sg_ai_reply_len, data, len);
    sg_ai_reply_len += len;
    sg_ai_reply_buf[sg_ai_reply_len] = '\0';
}

/**
 * @brief Send full AI reply to PC input bridge
 * @return none
 */
static void __ai_reply_send_to_pc(void)
{
    if (sg_ai_reply_len == 0) {
        return;
    }

    PR_INFO("[chat_bot] AI reply -> PC (%u bytes)", sg_ai_reply_len);
    signal_transport_send_asr(sg_ai_reply_buf, sg_ai_reply_len);
    __ai_reply_buf_reset();
}

static void __ai_chat_handle_event(AI_NOTIFY_EVENT_T *event)
{
    AI_NOTIFY_TEXT_T *text = NULL;

    if (event == NULL) {
        return;
    }

    switch (event->type) {
    case AI_USER_EVT_ASR_OK:
        /* User speech only — cloud Agent handles it; PC gets NLG on TEXT_STREAM_STOP */
        break;
    case AI_USER_EVT_TEXT_STREAM_START:
        __ai_reply_buf_reset();
        text = (AI_NOTIFY_TEXT_T *)event->data;
        if (text != NULL && text->data != NULL && text->datalen > 0) {
            __ai_reply_buf_append(text->data, text->datalen);
        }
        break;
    case AI_USER_EVT_TEXT_STREAM_DATA:
        text = (AI_NOTIFY_TEXT_T *)event->data;
        if (text != NULL && text->data != NULL && text->datalen > 0) {
            __ai_reply_buf_append(text->data, text->datalen);
        }
        break;
    case AI_USER_EVT_TEXT_STREAM_STOP:
        text = (AI_NOTIFY_TEXT_T *)event->data;
        if (text != NULL && text->data != NULL && text->datalen > 0) {
            __ai_reply_buf_append(text->data, text->datalen);
        }
        __ai_reply_send_to_pc();
        break;
    case AI_USER_EVT_TEXT_STREAM_ABORT:
        __ai_reply_buf_reset();
        break;
#if defined(ENABLE_PRINTER) && (ENABLE_PRINTER == 1)
    case AI_USER_EVT_GENERATE_PICTURE:
    case AI_USER_EVT_GET_PICTURE_FROM_APP: {
#if defined(ENABLE_COMP_AI_PICTURE) && (ENABLE_COMP_AI_PICTURE == 1)
        app_print_img_from_album((const char *)event->data);
#endif
    } break;
#endif
    default:
        break;
    }
}

OPERATE_RET app_chat_bot_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    AI_CHAT_MODE_CFG_T ai_chat_cfg = {
        .default_mode = AI_CHAT_MODE_HOLD,
        .default_vol  = 70,
        .evt_cb       = __ai_chat_handle_event,
    };
    TUYA_CALL_ERR_RETURN(ai_chat_init(&ai_chat_cfg));

#if defined(ENABLE_COMP_AI_DISPLAY) && (ENABLE_COMP_AI_DISPLAY == 1)
    app_ui_action_register();
#endif

#if defined(ENABLE_COMP_AI_VIDEO) && (ENABLE_COMP_AI_VIDEO == 1)
    TUYA_CALL_ERR_LOG(ai_video_init());
#endif

#if defined(ENABLE_COMP_AI_MCP) && (ENABLE_COMP_AI_MCP == 1)
    TUYA_CALL_ERR_RETURN(ai_mcp_init());
#endif

#if defined(ENABLE_COMP_AI_PICTURE) && (ENABLE_COMP_AI_PICTURE == 1)
    TUYA_CALL_ERR_RETURN(ai_picture_init());
#endif

    // Free heap size
    tal_sw_timer_create(__printf_free_heap_tm_cb, NULL, &sg_printf_heap_tm);
    tal_sw_timer_start(sg_printf_heap_tm, PRINTF_FREE_HEAP_TTIME, TAL_TIMER_CYCLE);

#if defined(ENABLE_COMP_AI_DISPLAY) && (ENABLE_COMP_AI_DISPLAY == 1)
    ai_ui_disp_msg(AI_UI_DISP_NETWORK, (uint8_t *)&sg_wifi_status, sizeof(AI_UI_WIFI_STATUS_E));

    ai_ui_disp_msg(AI_UI_DISP_STATUS, (uint8_t *)INITIALIZING, strlen(INITIALIZING));
    ai_ui_disp_msg(AI_UI_DISP_EMOTION, (uint8_t *)EMOJI_NEUTRAL, strlen(EMOJI_NEUTRAL));

    // display status update
    tal_sw_timer_create(__display_status_tm_cb, NULL, &sg_disp_status_tm);
    tal_sw_timer_start(sg_disp_status_tm, DISP_NET_STATUS_TIME, TAL_TIMER_CYCLE);
#endif

    return OPRT_OK;
}
