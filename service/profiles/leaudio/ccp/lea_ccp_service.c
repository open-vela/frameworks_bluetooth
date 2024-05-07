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
#define LOG_TAG "lea_ccp_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_ccp.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_ccp_event.h"
#include "lea_ccp_service.h"
#include "lea_server_service.h"
#include "sal_lea_ccp_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP
#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_ccp_service.started)       \
            return BT_STATUS_NOT_ENABLED; \
    }

#define CCP_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_ccp_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct
{
    bool started;
    bts_tbs_info_s tbs_info;
    bt_list_t* lea_calls;
    bearer_tele_info_t* info;
    callbacks_list_t* callbacks;
    pthread_mutex_t ccp_lock;
} lea_ccp_service_t;

static lea_ccp_service_t g_ccp_service = {
    .started = false,
    .lea_calls = NULL,
    .info = NULL,
    .callbacks = NULL,
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bool lea_ccp_call_cmp_index(void* ccp_call, void* call_index)
{
    return ((lea_tbs_call_state_t*)ccp_call)->index == *((uint8_t*)call_index);
}

static bool lea_ccp_call_cmp_state(void* ccp_call, void* call_state)
{
    return ((lea_tbs_call_state_t*)ccp_call)->state == *((uint8_t*)call_state);
}

lea_tbs_call_state_t* lea_ccp_find_call_by_index(uint8_t call_index)
{
    lea_tbs_call_state_t* ccp_call;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    ccp_call = bt_list_find(g_ccp_service.lea_calls, lea_ccp_call_cmp_index, &call_index);
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return ccp_call;
}

lea_tbs_call_state_t* lea_ccp_find_call_by_state(uint8_t call_state)
{
    lea_tbs_call_state_t* ccp_call;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    ccp_call = bt_list_find(g_ccp_service.lea_calls, lea_ccp_call_cmp_state, &call_state);
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return ccp_call;
}

static bool lea_ccp_find_call_index(uint8_t opcode, uint8_t* index)
{
    lea_tbs_call_state_t* call_states;
    bool valied = false;

    switch (opcode) {
    case ADPT_LEA_TBS_CALL_CONTROL_ACCEPT: {
        call_states = lea_ccp_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_INCOMING);

        break;
    }
    case ADPT_LEA_TBS_CALL_CONTROL_TERMINATE: {
        call_states = lea_ccp_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_INCOMING);
        if (call_states)
            break;

        call_states = lea_ccp_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_LOCALLY_HELD);
        if (call_states)
            break;

        call_states = lea_ccp_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_ACTIVE);
        if (call_states)
            break;

        call_states = lea_ccp_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_ALERTING);

        break;
    }
    case ADPT_LEA_TBS_CALL_CONTROL_LOCAL_HOLD: {
        call_states = lea_ccp_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_ACTIVE);

        break;
    }
    case ADPT_LEA_TBS_CALL_CONTROL_LOCAL_RETRIEVE: {
        call_states = lea_ccp_find_call_by_state(ADPT_LEA_TBS_CALL_STATE_LOCALLY_HELD);

        break;
    }
    default: {
        break;
    }
    }

    if (call_states) {
        valied = true;
        *index = call_states->index;
    }

    return valied;
}

lea_tbs_call_state_t* lea_ccp_add_call(lea_tbs_call_state_t* call)
{
    lea_tbs_call_state_t* ccp_call;

    ccp_call = malloc(sizeof(lea_tbs_call_state_t));
    if (!ccp_call) {
        BT_LOGE("error, malloc %s", __func__);
        return NULL;
    }
    memcpy(ccp_call, call, sizeof(lea_tbs_call_state_t));

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    bt_list_add_tail(g_ccp_service.lea_calls, ccp_call);
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return ccp_call;
}

static void lea_ccp_call_delete(lea_tbs_call_state_t* ccp_call)
{
    if (!ccp_call)
        return;
    free(ccp_call);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void lea_ccp_process_debug(lea_ccp_msg_t* msg)
{
    switch (msg->event) {
    case STACK_EVENT_READ_PROVIDER_NAME: {
        BT_LOGD("%s, event:%d, tbs_id:%d, provider_name:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_READ_UCI: {
        BT_LOGD("%s, event:%d, tbs_id:%d, uci:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_READ_TECHNOLOGY: {
        BT_LOGD("%s, event:%d, tbs_id:%d, technology:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8_0);
        break;
    }
    case STACK_EVENT_READ_URI_SCHEMES_SUPPORT_LIST: {
        BT_LOGD("%s, event:%d, tbs_id:%d, uri_schemes:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_READ_SIGNAL_STRENGTH: {
        BT_LOGD("%s, event:%d, tbs_id:%d, strength:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8_0);
        break;
    }
    case STACK_EVENT_READ_SIGNAL_STRENGTH_REPORT_INTERVAL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, interval:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8_0);
        break;
    }
    case STACK_EVENT_READ_CONTENT_CONTROL_ID: {
        BT_LOGD("%s, event:%d, tbs_id:%d, ccid:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8_0);
        break;
    }
    case STACK_EVENT_READ_STATUS_FLAGS: {
        BT_LOGD("%s, event:%d, tbs_id:%d, status_flags:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint16);
        break;
    }
    case STACK_EVENT_READ_CALL_CONTROL_OPTIONAL_OPCODES: {
        BT_LOGD("%s, event:%d, tbs_id:%d, option_opcode:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint16);
        break;
    }
    case STACK_EVENT_READ_INCOMING_CALL: {
        BT_LOGD("%s, event:%d, tbs_id:%d, uri:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_READ_INCOMING_CALL_TARGET_BEARER_URI: {
        BT_LOGD("%s, event:%d, tbs_id:%d, uri:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.dataarry);
        break;
    }
    case STACK_EVENT_READ_CALL_STATE: {
        lea_tbs_call_state_t* call_states;
        call_states = (lea_tbs_call_state_t*)msg->event_data.dataarry;
        BT_LOGD("%s, event:%d, tbs_id:%d, number:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint32);
        for (int i = 0; i < msg->event_data.valueint32; i++) {
            BT_LOGD("index:%d, state:%d, flags:%d",
                (call_states + i)->index,
                (call_states + i)->state,
                (call_states + i)->flags);
        }
        break;
    }

    case STACK_EVENT_READ_BEARER_LIST_CURRENT_CALL: {
        lea_tbs_call_list_item_t* calls;
        calls = (lea_tbs_call_list_item_t*)msg->event_data.dataarry;
        void* p = calls;
        BT_LOGD("%s, event:%d, tbs_id:%d, number:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint32);
        for (int i = 0; i < msg->event_data.valueint32; i++) {
            BT_LOGD("index:%d, state:%d, flags:%d, uri:%s",
                calls->index, calls->state,
                calls->flags, calls->call_uri);
            p += sizeof(lea_tbs_call_list_item_t) + strlen(calls->call_uri) + 1;
            calls = p;
        }
        break;
    }

    case STACK_EVENT_READ_CALL_FRIENDLY_NAME: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d, name:%s", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8_0,
            msg->event_data.dataarry);
        break;
    }

    case STACK_EVENT_TERMINATION_REASON: {
        BT_LOGD("%s, event:%d, tbs_id:%d, call_index:%d, reason:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8_0,
            msg->event_data.valueint8_1);
        break;
    }

    case STACK_EVENT_CALL_CONTROL_RESULT: {
        BT_LOGD("%s, event:%d, tbs_id:%d, opcode:%d, call_index:%d, result:%d", __func__,
            msg->event, msg->event_data.tbs_id, msg->event_data.valueint8_0,
            msg->event_data.valueint8_1, msg->event_data.valueint8_2);
        break;
    }
    }
}

static void lea_ccp_process_message(void* data)
{
    lea_ccp_service_t* service = &g_ccp_service;
    lea_ccp_msg_t* msg = (lea_ccp_msg_t*)data;

    lea_ccp_process_debug(msg);

    switch (msg->event) {
    case STACK_EVENT_READ_PROVIDER_NAME: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        strcpy(service->info->provider_name, (char*)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_UCI: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        strcpy(service->info->uci, (char*)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_TECHNOLOGY: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->technology = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_URI_SCHEMES_SUPPORT_LIST: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        strcpy(service->info->uri_schemes, (char*)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_SIGNAL_STRENGTH: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->strength = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_SIGNAL_STRENGTH_REPORT_INTERVAL: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->interval = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CONTENT_CONTROL_ID: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->ccid = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_STATUS_FLAGS: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->status_flags = msg->event_data.valueint16;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CALL_CONTROL_OPTIONAL_OPCODES: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->opcodes = msg->event_data.valueint16;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_INCOMING_CALL: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        strcpy(service->info->uri, (char*)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_INCOMING_CALL_TARGET_BEARER_URI: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        strcpy(service->info->uri, (char*)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CALL_STATE: {
        lea_tbs_call_state_t* call_states;
        lea_tbs_call_state_t* ccp_call;
        call_states = (lea_tbs_call_state_t*)msg->event_data.dataarry;

        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        for (int i = 0; i < msg->event_data.valueint32; i++) {
            ccp_call = lea_ccp_find_call_by_index((call_states + i)->index);
            if (!ccp_call) {
                lea_ccp_add_call(call_states + i);
            } else {
                memcpy(ccp_call, call_states + i, sizeof(lea_tbs_call_state_t));
            }
        }
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_BEARER_LIST_CURRENT_CALL: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CALL_FRIENDLY_NAME: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        strcpy(service->info->friendly_name, (char*)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_TERMINATION_REASON: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        bt_list_remove(g_ccp_service.lea_calls, lea_ccp_find_call_by_index(msg->event_data.valueint8_0));
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_CALL_CONTROL_RESULT: {
        pthread_mutex_lock(&g_ccp_service.ccp_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_1;
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);

        CCP_CALLBACK_FOREACH(g_ccp_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    default: {
        BT_LOGE("Idle: Unexpected stack event");
        break;
    }
    }
    lea_ccp_msg_destory(msg);
}

static bt_status_t lea_ccp_send_msg(lea_ccp_msg_t* msg)
{
    assert(msg);

    do_in_service_loop(lea_ccp_process_message, msg);

    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_ccp_on_bearer_provider_name(bt_address_t* addr, uint32_t tbs_id, size_t size,
    const char* name)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_PROVIDER_NAME, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    strcpy((char*)msg->event_data.dataarry, name);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_bearer_uci(bt_address_t* addr, uint32_t tbs_id, size_t size, const char* uci)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_UCI, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    strcpy((char*)msg->event_data.dataarry, uci);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_bearer_technology(bt_address_t* addr, uint32_t tbs_id, uint8_t technology)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_READ_TECHNOLOGY, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = technology;

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_bearer_uri_schemes_supported_list(bt_address_t* addr, uint32_t tbs_id, size_t size,
    const char* uri_schemes)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_URI_SCHEMES_SUPPORT_LIST, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    strcpy((char*)msg->event_data.dataarry, uri_schemes);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_bearer_signal_strength(bt_address_t* addr, uint32_t tbs_id, uint8_t strength)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_READ_SIGNAL_STRENGTH, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = strength;

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_bearer_signal_strength_report_interval(bt_address_t* addr, uint32_t tbs_id,
    uint8_t interval)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_READ_SIGNAL_STRENGTH_REPORT_INTERVAL, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = interval;

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_content_control_id(bt_address_t* addr, uint32_t tbs_id, uint8_t ccid)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_READ_CONTENT_CONTROL_ID, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = ccid;

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_status_flags(bt_address_t* addr, uint32_t tbs_id, uint16_t status_flags)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_READ_STATUS_FLAGS, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint16 = status_flags;

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_call_control_optional_opcodes(bt_address_t* addr, uint32_t tbs_id,
    uint16_t opcodes)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_READ_CALL_CONTROL_OPTIONAL_OPCODES, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint16 = opcodes;

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_incoming_call(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index, size_t size,
    const char* uri)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_INCOMING_CALL, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    strcpy((char*)msg->event_data.dataarry, uri);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_incoming_call_target_bearer_uri(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index,
    size_t size, const char* uri)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_INCOMING_CALL_TARGET_BEARER_URI, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    strcpy((char*)msg->event_data.dataarry, uri);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_call_state(bt_address_t* addr, uint32_t tbs_id, uint32_t number,
    lea_tbs_call_state_t* states_s)
{
    lea_ccp_msg_t* msg;

    if (number < 1) {
        BT_LOGW("%s ,the number of call state is zero!", __func__);
        return;
    }

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_CALL_STATE, addr, tbs_id,
        sizeof(lea_tbs_call_state_t) * number);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint32 = number;
    memcpy(&msg->event_data.dataarry, states_s, sizeof(lea_tbs_call_state_t) * number);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_bearer_list_current_calls(bt_address_t* addr, uint32_t tbs_id, uint32_t number, size_t size,
    lea_tbs_call_list_item_t* calls)
{
    lea_ccp_msg_t* msg;

    if (number < 1) {
        BT_LOGW("%s ,the number of bearer list current call is zero!", __func__);
        return;
    }

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_BEARER_LIST_CURRENT_CALL, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint32 = number;
    memcpy(&msg->event_data.dataarry, calls, size);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_call_friendly_name(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index, size_t size,
    const char* name)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new_ext(STACK_EVENT_READ_CALL_FRIENDLY_NAME, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    strcpy((char*)msg->event_data.dataarry, name);

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_termination_reason(bt_address_t* addr, uint32_t tbs_id, uint8_t call_index,
    lea_adpt_termination_reason_t reason)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_TERMINATION_REASON, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    msg->event_data.valueint8_1 = reason;

    lea_ccp_send_msg(msg);
}

void lea_ccp_on_call_control_result(bt_address_t* addr, uint32_t tbs_id, uint8_t opcode,
    uint8_t call_index, lea_adpt_call_control_result_t result)
{
    lea_ccp_msg_t* msg;

    msg = lea_ccp_msg_new(STACK_EVENT_CALL_CONTROL_RESULT, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = opcode;
    msg->event_data.valueint8_1 = call_index;
    msg->event_data.valueint8_2 = result;

    lea_ccp_send_msg(msg);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bt_status_t bts_lea_ccp_read_bearer_provider_name(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_bearer_provider_name(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_bearer_uci(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_bearer_uci(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_bearer_technology(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_bearer_technology(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_bearer_uri_schemes_supported_list(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_bearer_uri_schemes_supported_list(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_bearer_signal_strength(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_bearer_signal_strength(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_bearer_signal_strength_report_interval(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_bearer_signal_strength_report_interval(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_content_control_id(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_content_control_id(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_status_flags(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_status_flags(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_call_control_optional_opcodes(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_call_control_optional_opcodes(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_incoming_call(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_incoming_call(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_incoming_call_target_bearer_uri(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_incoming_call_target_bearer_uri(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_call_state(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_call_state(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_bearer_list_current_calls(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_bearer_list_current_calls(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_read_call_friendly_name(bt_address_t* addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_read_call_friendly_name(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_call_control_by_index(bt_address_t* addr, uint8_t opcode)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;
    uint8_t call_index;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }

    if (lea_ccp_find_call_index(opcode, &call_index)) {
        ret = bt_sal_lea_tbc_call_control_by_index(addr, service->tbs_info.sid, opcode, call_index);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s fail, err:%d ", __func__, ret);
            pthread_mutex_unlock(&g_ccp_service.ccp_lock);
            return BT_STATUS_FAIL;
        }
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_originate_call(bt_address_t* addr, uint8_t* uri)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_originate_call(addr, service->tbs_info.sid, uri);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccp_join_calls(bt_address_t* addr, uint8_t number,
    uint8_t* call_indexes)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccp_service_t* service = &g_ccp_service;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    ret = bt_sal_lea_tbc_join_calls(addr, service->tbs_info.sid, number, call_indexes);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        pthread_mutex_unlock(&g_ccp_service.ccp_lock);
        return BT_STATUS_FAIL;
    }
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static void* bts_ccp_register_callbacks(void* handle, lea_ccp_callbacks_t* callbacks)
{
    if (!g_ccp_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_ccp_service.callbacks, handle, (void*)callbacks);
}

static bool bts_ccp_unregister_callbacks(void** handle, void* cookie)
{
    if (!g_ccp_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_ccp_service.callbacks, handle, cookie);
}

static const lea_ccp_interface_t leaCcpInterface = {
    .size = sizeof(leaCcpInterface),
    .read_bearer_provider_name = bts_lea_ccp_read_bearer_provider_name,
    .read_bearer_uci = bts_lea_ccp_read_bearer_uci,
    .read_bearer_technology = bts_lea_ccp_read_bearer_technology,
    .read_bearer_uri_schemes_supported_list = bts_lea_ccp_read_bearer_uri_schemes_supported_list,
    .read_bearer_signal_strength = bts_lea_ccp_read_bearer_signal_strength,
    .read_bearer_signal_strength_report_interval = bts_lea_ccp_read_bearer_signal_strength_report_interval,
    .read_content_control_id = bts_lea_ccp_read_content_control_id,
    .read_status_flags = bts_lea_ccp_read_status_flags,
    .read_call_control_optional_opcodes = bts_lea_ccp_read_call_control_optional_opcodes,
    .read_incoming_call = bts_lea_ccp_read_incoming_call,
    .read_incoming_call_target_bearer_uri = bts_lea_ccp_read_incoming_call_target_bearer_uri,
    .read_call_state = bts_lea_ccp_read_call_state,
    .read_bearer_list_current_calls = bts_lea_ccp_read_bearer_list_current_calls,
    .read_call_friendly_name = bts_lea_ccp_read_call_friendly_name,
    .call_control_by_index = bts_lea_ccp_call_control_by_index,
    .originate_call = bts_lea_ccp_originate_call,
    .join_calls = bts_lea_ccp_join_calls,
    .register_callbacks = bts_ccp_register_callbacks,
    .unregister_callbacks = bts_ccp_unregister_callbacks,
};

/****************************************************************************
 * Public function
 ****************************************************************************/
static const void* get_lea_ccp_profile_interface(void)
{
    return &leaCcpInterface;
}

static bt_status_t lea_ccp_init(void)
{
    BT_LOGD("%s", __func__);
    lea_ccp_service_t* service = &g_ccp_service;
    service->tbs_info.num = 0;
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_ccp_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    pthread_mutexattr_t attr;
    lea_ccp_service_t* service = &g_ccp_service;

    BT_LOGD("%s", __func__);
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->lea_calls = bt_list_new((bt_list_free_cb_t)lea_ccp_call_delete);
    service->info = (bearer_tele_info_t*)malloc(sizeof(bearer_tele_info_t));
    service->callbacks = bt_callbacks_list_new(2);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->ccp_lock, &attr);

    service->started = true;
    return BT_STATUS_SUCCESS;

fail:
    bt_list_free(service->lea_calls);
    service->lea_calls = NULL;
    free((void*)service->info);
    bt_callbacks_list_free(service->callbacks);
    service->callbacks = NULL;
    pthread_mutex_destroy(&service->ccp_lock);
    return status;
}

static bt_status_t lea_ccp_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);
    if (!g_ccp_service.started)
        return BT_STATUS_SUCCESS;

    pthread_mutex_lock(&g_ccp_service.ccp_lock);
    g_ccp_service.started = false;

    bt_list_free(g_ccp_service.lea_calls);
    g_ccp_service.lea_calls = NULL;
    free((void*)g_ccp_service.info);
    g_ccp_service.info = NULL;
    bt_callbacks_list_free(g_ccp_service.callbacks);
    g_ccp_service.callbacks = NULL;
    pthread_mutex_unlock(&g_ccp_service.ccp_lock);
    pthread_mutex_destroy(&g_ccp_service.ccp_lock);

    return BT_STATUS_SUCCESS;
}

static void lea_ccp_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_ccp_dump(void)
{
    printf("impl leaudio ccp dump");
    return 0;
}

static const profile_service_t lea_ccp_service = {
    .auto_start = true,
    .name = PROFILE_CCP_NAME,
    .id = PROFILE_LEAUDIO_CCP,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = lea_ccp_init,
    .startup = lea_ccp_startup,
    .shutdown = lea_ccp_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_ccp_profile_interface,
    .cleanup = lea_ccp_cleanup,
    .dump = lea_ccp_dump,
};

void register_lea_ccp_service(void)
{
    register_service(&lea_ccp_service);
}

void adpt_tbs_sid_changed(uint32_t sid)
{
    lea_ccp_service_t* service = &g_ccp_service;
    BT_LOGD("%s, sid:%d", __func__, sid);
    service->tbs_info.num = CONFIG_BLUETOOTH_LEAUDIO_SERVER_CALL_CONTROL_NUMBER;
    service->tbs_info.sid = sid;
}

#endif