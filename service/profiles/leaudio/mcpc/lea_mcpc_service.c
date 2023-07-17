/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
/****************************************************************************
 * Included Files
 ****************************************************************************/
#define LOG_TAG "lea_mcpc_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_mcpc.h"
#include "bt_profile.h"
#include "bt_addr.h"
#include "callbacks_list.h"
#include "lea_mcpc_service.h"
#include "lea_mcpc_event.h"
#include "sal_lea_mcpc_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCPC

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_mcpc_service.started)       \
            return BT_STATUS_NOT_ENABLED; \
    }

#define MCPC_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_mcpc_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct
{
    bool started;
    bts_mcs_info_s mcs_info;
    callbacks_list_t *callbacks;
    pthread_mutex_t device_lock;
} mcpc_service_t;

static mcpc_service_t g_mcpc_service = {
    .started = false,
    .callbacks = NULL,
};

static void lea_mcpc_process_message(void *data)
{
    mcpc_event_t *msg = (mcpc_event_t *)data;
    switch (msg->event) {
        case MCP_MEDIA_PLAYER_NAME: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_MEDIA_PLAYER_ICON_OBJ_ID:{
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_MEDIA_PLAYER_ICON_URL: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_PLAYBACK_SPEED: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_SEEKING_SPEED: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_PLAYING_ORDER: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_PLAYING_ORDER_SUPPORTED: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_MEDIA_CONTROL_OPCODES_SUPPORTED: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_TRACK_CHANGED: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_TRACK_TITLE: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_TRACK_DURATION: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_TRACK_POSITION: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_MEDIA_STATE: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_MEDIA_CONTROL_REQ: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_SEARCH_CONTROL_RESULT_REQ: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_CURRENT_TRACK_SEGMENTS_OBJ_ID: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_CURRENT_TRACK_OBJ_ID: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_NEXT_TRACK_OBJ_ID: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_PARENT_GROUP_OBJ_ID: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_CURRENT_GROUP_OBJ_ID: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_SEARCH_RESULTS_OBJ_ID: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        case MCP_READ_CCID: {
            MCPC_CALLBACK_FOREACH(g_mcpc_service.callbacks, test_cb, &msg->remote_addr, msg->event);
            break;
        }
        default:
            BT_LOGW("%s, Unknown event: %d !", __func__, msg->event);
            break;
    }
    mcpc_event_destory(msg);
}

static bt_status_t lea_mcpc_send_msg(mcpc_event_t *msg)
{
    assert(msg);

    do_in_service_loop(lea_mcpc_process_message, msg);

    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_mcpc_on_media_player_name(bt_address_t *addr, uint32_t mcs_id, size_t size, char *name)
{
    mcpc_event_t* event;

    event = mcpc_event_new_ext(MCP_MEDIA_PLAYER_NAME, addr, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    strcpy((char *)event->event_data.string1, name);

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_media_player_icon_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_MEDIA_PLAYER_ICON_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcpc_object_id));

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_media_player_icon_url(bt_address_t *addr, uint32_t mcs_id, size_t size, char *url)
{
    mcpc_event_t* event;

    event = mcpc_event_new_ext(MCP_MEDIA_PLAYER_ICON_URL, addr, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    strcpy((char *)event->event_data.string1, url);

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_playback_speed(bt_address_t *addr, uint32_t mcs_id, int8_t speed)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_PLAYBACK_SPEED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint8 = speed;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_seeking_speed(bt_address_t *addr, uint32_t mcs_id, int8_t speed)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_SEEKING_SPEED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint8 = speed;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_playing_order(bt_address_t *addr, uint32_t mcs_id, int8_t order)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_PLAYING_ORDER, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = order;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_playing_orders_supported(bt_address_t *addr, uint32_t mcs_id, uint16_t orders)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_PLAYING_ORDER_SUPPORTED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint16 = orders;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_media_control_opcodes_supported(bt_address_t *addr, uint32_t mcs_id, uint32_t opcodes)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_MEDIA_CONTROL_OPCODES_SUPPORTED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint32 = opcodes;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_track_changed(bt_address_t *addr, uint32_t mcs_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_TRACK_CHANGED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_track_title(bt_address_t *addr, uint32_t mcs_id, size_t size, char *title)
{
    mcpc_event_t* event;

    event = mcpc_event_new_ext(MCP_READ_TRACK_TITLE, addr, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    strcpy((char *)event->event_data.string1, title);

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_track_duration(bt_address_t *addr, uint32_t mcs_id, int32_t duration)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_TRACK_DURATION, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint32 = duration;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_track_position(bt_address_t *addr, uint32_t mcs_id, int32_t position)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_TRACK_POSITION, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint32 = position;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_media_state(bt_address_t *addr, uint32_t mcs_id, uint8_t state)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_MEDIA_STATE, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = state;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_media_control_result(bt_address_t *addr, uint32_t mcs_id, uint8_t opcode, uint8_t result)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_MEDIA_CONTROL_REQ, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = opcode;
    event->event_data.valueuint8_1 = result;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_search_control_result(bt_address_t *addr, uint32_t mcs_id, uint8_t result)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_SEARCH_CONTROL_RESULT_REQ, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = result;

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_current_track_segments_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_CURRENT_TRACK_SEGMENTS_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcpc_object_id));

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_current_track_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_CURRENT_TRACK_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL) {
        BT_LOGD("obj_id: %02x %02x %02x %02x %02x %02x", obj_id[0],
                obj_id[1], obj_id[2], obj_id[3], obj_id[4], obj_id[5]);
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcpc_object_id));
    }

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_next_track_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_NEXT_TRACK_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcpc_object_id));

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_parent_group_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_PARENT_GROUP_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcpc_object_id));

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_current_group_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_CURRENT_GROUP_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcpc_object_id));

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_search_results_object_id(bt_address_t *addr, uint32_t mcs_id, lea_mcpc_object_id obj_id)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_SEARCH_RESULTS_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcpc_object_id));

    lea_mcpc_send_msg(event);
}

void lea_mcpc_on_content_control_id(bt_address_t *addr, uint32_t mcs_id, uint8_t ccid)
{
    mcpc_event_t* event;

    event = mcpc_event_new(MCP_READ_CCID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = ccid;

    lea_mcpc_send_msg(event);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bt_status_t bts_mcp_read_media_player_name(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_media_player_name(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_player_icon_object_id(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_media_player_icon_object_id(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_player_icon_url(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_media_player_icon_url(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_playback_speed(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_playback_speed(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_seeking_speed(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_seeking_speed(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_playing_order(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_playing_order(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_playing_orders_supported(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_playing_orders_supported(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_control_opcodes_supported(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_media_control_opcodes_supported(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_track_title(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_track_title(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_track_duration(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_track_duration(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_track_position(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_track_position(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_state(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_media_state(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_current_track_object_id(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_current_track_object_id(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_next_track_object_id(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_next_track_object_id(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_parent_group_object_id(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_parent_group_object_id(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_current_group_object_id(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_current_group_object_id(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_search_results_object_id(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_search_results_object_id(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_content_control_id(bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_read_content_control_id(addr, g_mcpc_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_remote_mcs_info(bt_address_t *addr, uint8_t opcode)
{
    CHECK_ENABLED();
    bt_status_t status;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    switch (opcode) {
        case MCPC_MEDIA_PLAYER_NAME: {
            status = bts_mcp_read_media_player_name(addr);
            break;
        }
        case MCPC_MEIDA_PLAYER_ICON_OBJECT_ID: {
            status = bts_mcp_read_media_player_icon_object_id(addr);
            break;
        }
        case MCPC_MEIDA_PLAYER_ICON_URL: {
            status = bts_mcp_read_media_player_icon_url(addr);
            break;
        }
        case MCPC_PLAYBACK_SPEED: {
            status = bts_mcp_read_playback_speed(addr);
            break;
        }
        case MCPC_SEEKING_SPEED: {
            status = bts_mcp_read_seeking_speed(addr);
            break;
        }
        case MCPC_PLAYING_ORDER: {
            status = bts_mcp_read_playing_order(addr);
            break;
        }
        case MCPC_PLAYING_ORDERS_SUPPORTED: {
            status = bts_mcp_read_playing_orders_supported(addr);
            break;
        }
        case MCPC_MEIDA_CONTROL_OPCODES_SUPPORTED: {
            status = bts_mcp_read_media_control_opcodes_supported(addr);
            break;
        }
        case MCPC_TRACK_TITLE: {
            status = bts_mcp_read_track_title(addr);
            break;
        }
        case MCPC_TRACK_DURATION: {
            status = bts_mcp_read_track_duration(addr);
            break;
        }
        case MCPC_TRACK_POSITION: {
            status = bts_mcp_read_track_position(addr);
            break;
        }
        case MCPC_MEDIA_STATE: {
            status = bts_mcp_read_media_state(addr);
            break;
        }
        case MCPC_CURRENT_TRACK_OBJECT_ID: {
            status = bts_mcp_read_current_track_object_id(addr);
            break;
        }
        case MCPC_NEXT_TRACK_OBJECT_ID: {
            status = bts_mcp_read_next_track_object_id(addr);
            break;
        }
        case MCPC_PARENT_GROUP_OBJECT_ID: {
            status = bts_mcp_read_parent_group_object_id(addr);
            break;
        }
        case MCPC_CURRENT_GROUP_OBJECT_ID: {
            status = bts_mcp_read_current_group_object_id(addr);
            break;
        }
        case MCPC_SEARCH_RESULTS_OBJECT_ID: {
            status = bts_mcp_read_search_results_object_id(addr);
            break;
        }
        case MCPC_CONTENT_CONTROL_ID: {
            status = bts_mcp_read_content_control_id(addr);
            break;
        }
        default:
            BT_LOGW("%s, Unknown event!", __func__);
            status = BT_STATUS_FAIL;
            break;
    }

    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, status);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_media_control_request(bt_address_t *addr,
    LEA_MCC_MEDIA_CONTROL_OPCODE opcode, int32_t n)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_media_control_request(addr, g_mcpc_service.mcs_info.sid, opcode, n);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_search_control_request(bt_address_t *addr,
        uint8_t number, LEA_MCC_SEARCH_CONTROL_ITEM_TYPE type, uint8_t *parameter)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcpc_service.device_lock);
    if (!g_mcpc_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcc_search_control_request(addr, g_mcpc_service.mcs_info.sid, number, type, parameter);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcpc_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcpc_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static void *bts_mcp_set_callbacks(void *handle, lea_mcpc_callbacks_t* callbacks)
{
    if (!g_mcpc_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_mcpc_service.callbacks, handle, (void *)callbacks);
}

static bool bts_mcp_reset_callbacks(void **handle, void *cookie)
{
    if (!g_mcpc_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_mcpc_service.callbacks, handle, cookie);
}

static const lea_mcpc_interface_t leMcpInterface = {
    .size = sizeof(leMcpInterface),
    .read_remote_mcs_info = bts_mcp_read_remote_mcs_info,
    .media_control_request = bts_mcp_media_control_request,
    .search_control_request = bts_mcp_search_control_request,
    .set_callbacks = bts_mcp_set_callbacks,
    .reset_callbacks = bts_mcp_reset_callbacks,
};

/****************************************************************************
 * Public function
 ****************************************************************************/
static const void *get_lea_mcpc_profile_interface(void)
{
    return &leMcpInterface;
}
static bt_status_t lea_mcpc_init(void)
{
    BT_LOGD("%s", __func__);
    g_mcpc_service.mcs_info.num = 0; // no service
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcpc_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    pthread_mutexattr_t attr;
    mcpc_service_t *service = &g_mcpc_service;
    if (service->started)
        return BT_STATUS_SUCCESS;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    if (pthread_mutex_init(&service->device_lock, &attr) < 0)
        return BT_STATUS_FAIL;
    service->callbacks = bt_callbacks_list_new(2);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }
    service->started = true;
    return BT_STATUS_SUCCESS;
fail:
    bt_callbacks_list_free(service->callbacks);
    pthread_mutex_destroy(&service->device_lock);
    return status;
}

static bt_status_t lea_mcpc_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGE("%s", __func__);
    if (!g_mcpc_service.started)
        return BT_STATUS_SUCCESS;
    pthread_mutex_lock(&g_mcpc_service.device_lock);
    g_mcpc_service.started = false;
    pthread_mutex_unlock(&g_mcpc_service.device_lock);
    pthread_mutex_destroy(&g_mcpc_service.device_lock);
    bt_callbacks_list_free(g_mcpc_service.callbacks);
    g_mcpc_service.callbacks = NULL;
    return BT_STATUS_SUCCESS;
}

static void lea_mcpc_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_mcpc_dump(void)
{
    printf("impl leaudio mcpc dump");
    return 0;
}

static const profile_service_t lea_mcpc_service = {
    .auto_start = true,
    .name = PROFILE_MCPC_NAME,
    .id = PROFILE_LEAUDIO_MCPC,
    .transport = BT_TRANSPORT_BLE,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = lea_mcpc_init,
    .startup = lea_mcpc_startup,
    .shutdown = lea_mcpc_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_mcpc_profile_interface,
    .cleanup = lea_mcpc_cleanup,
    .dump = lea_mcpc_dump,
};

void register_lea_mcpc_service(void)
{
    register_service(&lea_mcpc_service);
}

void adapt_mcs_sid_changed(uint32_t sid)
{
    BT_LOGD("%s, sid:%d", __func__, sid);
    g_mcpc_service.mcs_info.num = CONFIG_BLUETOOTH_LEAUDIO_SERVER_MEDIA_CONTROL_NUMBER;
    g_mcpc_service.mcs_info.sid = sid;
}

#endif
