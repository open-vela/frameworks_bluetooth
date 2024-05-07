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
#define LOG_TAG "lea_mcp_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_lea_mcp.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_mcp_event.h"
#include "lea_mcp_service.h"
#include "media_session.h"
#include "sal_lea_mcp_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCP

#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_mcp_service.started)       \
            return BT_STATUS_NOT_ENABLED; \
    }

#define MCP_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_mcp_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct
{
    bool started;
    bts_mcs_info_s mcs_info;
    callbacks_list_t* callbacks;
    pthread_mutex_t device_lock;
    void* media_session_handle;
} mcp_service_t;

static mcp_service_t g_mcp_service = {
    .started = false,
    .callbacks = NULL,
    .media_session_handle = NULL,
};

static void lea_mcp_process_message(void* data)
{
    mcp_event_t* msg = (mcp_event_t*)data;

    switch (msg->event) {
    case MCP_MEDIA_PLAYER_NAME_CHANGED: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_MEDIA_PLAYER_ICON_OBJ_ID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_MEDIA_PLAYER_ICON_URL: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_PLAYBACK_SPEED: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_SEEKING_SPEED: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_PLAYING_ORDER: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_PLAYING_ORDER_SUPPORTED: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_MEDIA_CONTROL_OPCODES_SUPPORTED: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_TRACK_CHANGED: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_TRACK_TITLE: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_TRACK_DURATION: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_TRACK_POSITION: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_MEDIA_STATE: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_MEDIA_CONTROL_REQ: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_SEARCH_CONTROL_RESULT_REQ: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_CURRENT_TRACK_SEGMENTS_OBJ_ID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_CURRENT_TRACK_OBJ_ID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_NEXT_TRACK_OBJ_ID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_PARENT_GROUP_OBJ_ID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_CURRENT_GROUP_OBJ_ID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_SEARCH_RESULTS_OBJ_ID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    case MCP_READ_CCID: {
        MCP_CALLBACK_FOREACH(g_mcp_service.callbacks, test_cb, &msg->remote_addr, msg->event);
        break;
    }
    default:
        BT_LOGW("%s, Unknown event: %d !", __func__, msg->event);
        break;
    }
    mcp_event_destory(msg);
}

static bt_status_t lea_mcp_send_msg(mcp_event_t* msg)
{
    assert(msg);

    do_in_service_loop(lea_mcp_process_message, msg);

    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_mcp_on_media_player_name(bt_address_t* addr, uint32_t mcs_id, size_t size, char* name)
{
    mcp_event_t* event;

    event = mcp_event_new_ext(MCP_MEDIA_PLAYER_NAME_CHANGED, addr, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    strcpy((char*)event->event_data.string1, name);

    lea_mcp_send_msg(event);
}

void lea_mcp_on_media_player_icon_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_MEDIA_PLAYER_ICON_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcp_object_id));

    lea_mcp_send_msg(event);
}

void lea_mcp_on_media_player_icon_url(bt_address_t* addr, uint32_t mcs_id, size_t size, char* url)
{
    mcp_event_t* event;

    event = mcp_event_new_ext(MCP_MEDIA_PLAYER_ICON_URL, addr, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    strcpy((char*)event->event_data.string1, url);

    lea_mcp_send_msg(event);
}

void lea_mcp_on_playback_speed(bt_address_t* addr, uint32_t mcs_id, int8_t speed)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_PLAYBACK_SPEED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint8 = speed;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_seeking_speed(bt_address_t* addr, uint32_t mcs_id, int8_t speed)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_SEEKING_SPEED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint8 = speed;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_playing_order(bt_address_t* addr, uint32_t mcs_id, int8_t order)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_PLAYING_ORDER, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = order;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_playing_orders_supported(bt_address_t* addr, uint32_t mcs_id, uint16_t orders)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_PLAYING_ORDER_SUPPORTED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint16 = orders;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_media_control_opcodes_supported(bt_address_t* addr, uint32_t mcs_id, uint32_t opcodes)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_MEDIA_CONTROL_OPCODES_SUPPORTED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint32 = opcodes;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_track_changed(bt_address_t* addr, uint32_t mcs_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_TRACK_CHANGED, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    lea_mcp_send_msg(event);
}

void lea_mcp_on_track_title(bt_address_t* addr, uint32_t mcs_id, size_t size, char* title)
{
    mcp_event_t* event;

    event = mcp_event_new_ext(MCP_READ_TRACK_TITLE, addr, mcs_id, size);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    strcpy((char*)event->event_data.string1, title);

    lea_mcp_send_msg(event);
}

void lea_mcp_on_track_duration(bt_address_t* addr, uint32_t mcs_id, int32_t duration)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_TRACK_DURATION, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint32 = duration;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_track_position(bt_address_t* addr, uint32_t mcs_id, int32_t position)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_TRACK_POSITION, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueint32 = position;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_media_state(bt_address_t* addr, uint32_t mcs_id, uint8_t state)
{
    mcp_event_t* event;
    BT_LOGD("%s, media state:%d ", __func__, state);

    event = mcp_event_new(MCP_READ_MEDIA_STATE, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = state;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_media_control_result(bt_address_t* addr, uint32_t mcs_id, uint8_t opcode, uint8_t result)
{
    mcp_event_t* event;
    BT_LOGD("%s, opcode:%d ,result:%d", __func__, opcode, result);

    event = mcp_event_new(MCP_MEDIA_CONTROL_REQ, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = opcode;
    event->event_data.valueuint8_1 = result;

    lea_mcp_send_msg(event);

    if (result != LEA_MEDIA_CONTROL_SUCCESS) {
        BT_LOGW("%s, Failed to control remote device!", __func__);
        return;
    }

    if (opcode == MCP_MEDIA_CONTROL_PREVIOUS_TRACK) {
        media_session_notify(&g_mcp_service.media_session_handle, MEDIA_EVENT_PREV_SONG, 0, NULL);
    } else if (opcode == MCP_MEDIA_CONTROL_NEXT_TRACK) {
        media_session_notify(&g_mcp_service.media_session_handle, MEDIA_EVENT_NEXT_SONG, 0, NULL);
    }
}

void lea_mcp_on_search_control_result(bt_address_t* addr, uint32_t mcs_id, uint8_t result)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_SEARCH_CONTROL_RESULT_REQ, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = result;

    lea_mcp_send_msg(event);
}

void lea_mcp_on_current_track_segments_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_CURRENT_TRACK_SEGMENTS_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcp_object_id));

    lea_mcp_send_msg(event);
}

void lea_mcp_on_current_track_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_CURRENT_TRACK_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL) {
        BT_LOGD("obj_id: %02x %02x %02x %02x %02x %02x", obj_id[0],
            obj_id[1], obj_id[2], obj_id[3], obj_id[4], obj_id[5]);
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcp_object_id));
    }

    lea_mcp_send_msg(event);
}

void lea_mcp_on_next_track_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_NEXT_TRACK_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcp_object_id));

    lea_mcp_send_msg(event);
}

void lea_mcp_on_parent_group_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_PARENT_GROUP_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcp_object_id));

    lea_mcp_send_msg(event);
}

void lea_mcp_on_current_group_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_CURRENT_GROUP_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcp_object_id));

    lea_mcp_send_msg(event);
}

void lea_mcp_on_search_results_object_id(bt_address_t* addr, uint32_t mcs_id, lea_mcp_object_id obj_id)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_SEARCH_RESULTS_OBJ_ID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    if (obj_id != NULL)
        memcpy(&event->event_data.obj_id, obj_id, sizeof(lea_mcp_object_id));

    lea_mcp_send_msg(event);
}

void lea_mcp_on_content_control_id(bt_address_t* addr, uint32_t mcs_id, uint8_t ccid)
{
    mcp_event_t* event;

    event = mcp_event_new(MCP_READ_CCID, addr, mcs_id);
    if (!event) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }
    event->event_data.valueuint8_0 = ccid;

    lea_mcp_send_msg(event);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bt_status_t bts_mcp_read_media_player_name(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_media_player_name(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_player_icon_object_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_media_player_icon_object_id(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_player_icon_url(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_media_player_icon_url(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_playback_speed(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_playback_speed(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_seeking_speed(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_seeking_speed(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_playing_order(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_playing_order(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_playing_orders_supported(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_playing_orders_supported(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_control_opcodes_supported(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_media_control_opcodes_supported(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_track_title(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_track_title(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_track_duration(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_track_duration(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_track_position(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_track_position(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_media_state(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_media_state(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_current_track_object_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_current_track_object_id(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_next_track_object_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_next_track_object_id(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_parent_group_object_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_parent_group_object_id(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_current_group_object_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_current_group_object_id(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_search_results_object_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_search_results_object_id(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_content_control_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_read_content_control_id(addr, g_mcp_service.mcs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_read_remote_mcs_info(bt_address_t* addr, uint8_t opcode)
{
    CHECK_ENABLED();
    bt_status_t status;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    switch (opcode) {
    case MCP_MEDIA_PLAYER_NAME_CHANGED: {
        status = bts_mcp_read_media_player_name(addr);
        break;
    }
    case MCP_MEIDA_PLAYER_ICON_OBJECT_ID: {
        status = bts_mcp_read_media_player_icon_object_id(addr);
        break;
    }
    case MCP_MEIDA_PLAYER_ICON_URL: {
        status = bts_mcp_read_media_player_icon_url(addr);
        break;
    }
    case MCP_PLAYBACK_SPEED: {
        status = bts_mcp_read_playback_speed(addr);
        break;
    }
    case MCP_SEEKING_SPEED: {
        status = bts_mcp_read_seeking_speed(addr);
        break;
    }
    case MCP_PLAYING_ORDER: {
        status = bts_mcp_read_playing_order(addr);
        break;
    }
    case MCP_PLAYING_ORDERS_SUPPORTED: {
        status = bts_mcp_read_playing_orders_supported(addr);
        break;
    }
    case MCP_MEIDA_CONTROL_OPCODES_SUPPORTED: {
        status = bts_mcp_read_media_control_opcodes_supported(addr);
        break;
    }
    case MCP_TRACK_TITLE: {
        status = bts_mcp_read_track_title(addr);
        break;
    }
    case MCP_TRACK_DURATION: {
        status = bts_mcp_read_track_duration(addr);
        break;
    }
    case MCP_TRACK_POSITION: {
        status = bts_mcp_read_track_position(addr);
        break;
    }
    case MCP_MEDIA_STATE: {
        status = bts_mcp_read_media_state(addr);
        break;
    }
    case MCP_CURRENT_TRACK_OBJECT_ID: {
        status = bts_mcp_read_current_track_object_id(addr);
        break;
    }
    case MCP_NEXT_TRACK_OBJECT_ID: {
        status = bts_mcp_read_next_track_object_id(addr);
        break;
    }
    case MCP_PARENT_GROUP_OBJECT_ID: {
        status = bts_mcp_read_parent_group_object_id(addr);
        break;
    }
    case MCP_CURRENT_GROUP_OBJECT_ID: {
        status = bts_mcp_read_current_group_object_id(addr);
        break;
    }
    case MCP_SEARCH_RESULTS_OBJECT_ID: {
        status = bts_mcp_read_search_results_object_id(addr);
        break;
    }
    case MCP_CONTENT_CONTROL_ID: {
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
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_media_control_request(bt_address_t* addr,
    LEA_MCP_MEDIA_CONTROL_OPCODE opcode, int32_t n)
{
    CHECK_ENABLED();
    bt_status_t ret;
    BT_LOGD("%s, opcode:%d ", __func__, opcode);

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_media_control_request(addr, g_mcp_service.mcs_info.sid, opcode, n);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_mcp_search_control_request(bt_address_t* addr,
    uint8_t number, LEA_MCP_SEARCH_CONTROL_ITEM_TYPE type, uint8_t* parameter)
{
    CHECK_ENABLED();
    bt_status_t ret;

    pthread_mutex_lock(&g_mcp_service.device_lock);
    if (!g_mcp_service.mcs_info.num) {
        BT_LOGE("%s, mcs num is unexpected", __func__);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }

    ret = bt_sal_lea_mcp_search_control_request(addr, g_mcp_service.mcs_info.sid, number, type, parameter);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_mcp_service.device_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_mcp_service.device_lock);

    return BT_STATUS_SUCCESS;
}

static void* bts_mcp_set_callbacks(void* handle, lea_mcp_callbacks_t* callbacks)
{
    if (!g_mcp_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_mcp_service.callbacks, handle, (void*)callbacks);
}

static bool bts_mcp_reset_callbacks(void** handle, void* cookie)
{
    if (!g_mcp_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_mcp_service.callbacks, handle, cookie);
}

static const lea_mcp_interface_t leMcpInterface = {
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
static const void* get_lea_mcp_profile_interface(void)
{
    return &leMcpInterface;
}

static bool mcp_allocator(void** data, uint32_t size)
{
    *data = malloc(size);
    if (!(*data))
        return false;

    return true;
}

static void lea_mcs_media_seesion_event_callback(void* cookie, int event, int ret, const char* data)
{
    BT_LOGD("%s, event:%d ", __func__, event);

    bt_status_t rt;
    bt_address_t* addrs = NULL;
    int num = 0;

    rt = adapter_get_connected_devices(BT_TRANSPORT_BLE, &addrs, &num, mcp_allocator);
    if (rt != BT_STATUS_SUCCESS || num < 1) {
        BT_LOGE("%s, Le connected devices get failed", __func__);
        return;
    }

    switch (event) {
    case MEDIA_EVENT_START: {
        bts_mcp_media_control_request(addrs + num - 1, MCP_MEDIA_CONTROL_PLAY, 0); // addrs + num - 1: the latest connected device
        break;
    }
    case MEDIA_EVENT_PAUSE: {
        bts_mcp_media_control_request(addrs + num - 1, MCP_MEDIA_CONTROL_PAUSE, 0);
        break;
    }
    case MEDIA_EVENT_STOP: {
        bts_mcp_media_control_request(addrs + num - 1, MCP_MEDIA_CONTROL_STOP, 0);
        break;
    }
    case MEDIA_EVENT_PREV_SONG: {
        bts_mcp_media_control_request(addrs + num - 1, MCP_MEDIA_CONTROL_PREVIOUS_TRACK, 0);
        break;
    }
    case MEDIA_EVENT_NEXT_SONG: {
        bts_mcp_media_control_request(addrs + num - 1, MCP_MEDIA_CONTROL_NEXT_TRACK, 0);
        break;
    }
    default:
        BT_LOGW("%s, Unknown event!", __func__);
        break;
    }
}

static bt_status_t lea_mcp_init(void)
{
    BT_LOGD("%s", __func__);
    g_mcp_service.mcs_info.num = 0; // no service
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_mcp_startup(profile_on_startup_t cb)
{
    BT_LOGD("%s", __func__);
    bt_status_t status;
    pthread_mutexattr_t attr;
    mcp_service_t* service = &g_mcp_service;
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
    service->media_session_handle = media_session_register(service,
        lea_mcs_media_seesion_event_callback);
    if (!service->media_session_handle) {
        BT_LOGE("%s media session open failed.", __func__);
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
fail:
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    pthread_mutex_destroy(&service->device_lock);
    return status;
}

static bt_status_t lea_mcp_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);
    if (!g_mcp_service.started)
        return BT_STATUS_SUCCESS;
    pthread_mutex_lock(&g_mcp_service.device_lock);
    g_mcp_service.started = false;
    media_session_unregister(&g_mcp_service.media_session_handle);
    pthread_mutex_unlock(&g_mcp_service.device_lock);
    pthread_mutex_destroy(&g_mcp_service.device_lock);
    bt_callbacks_list_free(g_mcp_service.callbacks);
    g_mcp_service.callbacks = NULL;
    return BT_STATUS_SUCCESS;
}

static void lea_mcp_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_mcp_dump(void)
{
    printf("impl leaudio mcp dump");
    return 0;
}

static const profile_service_t lea_mcp_service = {
    .auto_start = true,
    .name = PROFILE_MCP_NAME,
    .id = PROFILE_LEAUDIO_MCP,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = lea_mcp_init,
    .startup = lea_mcp_startup,
    .shutdown = lea_mcp_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_mcp_profile_interface,
    .cleanup = lea_mcp_cleanup,
    .dump = lea_mcp_dump,
};

void register_lea_mcp_service(void)
{
    register_service(&lea_mcp_service);
}

void adapt_mcs_sid_changed(uint32_t sid)
{
    BT_LOGD("%s, sid:%d", __func__, sid);
    g_mcp_service.mcs_info.num = CONFIG_BLUETOOTH_LEAUDIO_SERVER_MEDIA_CONTROL_NUMBER;
    g_mcp_service.mcs_info.sid = sid;
}

#endif
