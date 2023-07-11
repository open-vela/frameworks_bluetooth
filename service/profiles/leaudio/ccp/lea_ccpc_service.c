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
#define LOG_TAG "lea_ccpc_service"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "bt_lea_ccpc.h"
#include "bt_profile.h"
#include "callbacks_list.h"
#include "lea_ccpc_event.h"
#include "lea_ccpc_service.h"
#include "lea_server_service.h"
#include "sal_lea_ccpc_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCPC
#define CHECK_ENABLED()                   \
    {                                     \
        if (!g_ccpc_service.started)      \
            return BT_STATUS_NOT_ENABLED; \
    }

#define CCPC_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, lea_ccpc_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct
{
    bool started;
    bts_tbs_info_s tbs_info;
    bt_list_t *lea_calls;
    bearer_tele_info_t *info;
    callbacks_list_t *callbacks;
    pthread_mutex_t ccpc_lock;
} lea_ccpc_service_t;

static lea_ccpc_service_t g_ccpc_service = {
    .started = false,
    .lea_calls = NULL,
    .info = NULL,
    .callbacks = NULL,
};

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bool lea_ccpc_call_cmp_index(void *ccp_call, void *call_index)
{
    return ((LEA_TBS_CALL_STATE_S *)ccp_call)->index == *((uint8_t *)call_index);
}

static bool lea_ccpc_call_cmp_state(void *ccp_call, void *call_state)
{
    return ((LEA_TBS_CALL_STATE_S *)ccp_call)->state == *((uint8_t *)call_state);
}

LEA_TBS_CALL_STATE_S *lea_ccpc_find_call_by_index(uint8_t call_index)
{
    LEA_TBS_CALL_STATE_S *ccp_call;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    ccp_call = bt_list_find(g_ccpc_service.lea_calls, lea_ccpc_call_cmp_index, &call_index);
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return ccp_call;
}

LEA_TBS_CALL_STATE_S *lea_ccpc_find_call_by_state(uint8_t call_state)
{
    LEA_TBS_CALL_STATE_S *ccp_call;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    ccp_call = bt_list_find(g_ccpc_service.lea_calls, lea_ccpc_call_cmp_state, &call_state);
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return ccp_call;
}

static bool lea_ccpc_find_call_index(uint8_t opcode, uint8_t *index)
{
    LEA_TBS_CALL_STATE_S *call_states;
    bool valied = false;

    switch (opcode) {
    case LEA_CCPC_CALL_CONTROL_ACCEPT: {
        call_states = lea_ccpc_find_call_by_state(LEA_CCPC_CALL_STATE_INCOMING);

        break;
    }
    case LEA_CCPC_CALL_CONTROL_TERMINATE: {
        call_states = lea_ccpc_find_call_by_state(LEA_CCPC_CALL_STATE_INCOMING);
        if (!call_states) {
            call_states = lea_ccpc_find_call_by_state(LEA_CCPC_CALL_STATE_LOCALLY_HELD);
            if (!call_states) {
                call_states = lea_ccpc_find_call_by_state(LEA_CCPC_CALL_STATE_ACTIVE);
            }
        }

        break;
    }
    case LEA_CCPC_CALL_CONTROL_LOCAL_HOLD: {
        call_states = lea_ccpc_find_call_by_state(LEA_CCPC_CALL_STATE_ACTIVE);

        break;
    }
    case LEA_CCPC_CALL_CONTROL_LOCAL_RETRIEVE: {
        call_states = lea_ccpc_find_call_by_state(LEA_CCPC_CALL_STATE_LOCALLY_HELD);

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

LEA_TBS_CALL_STATE_S *lea_ccpc_add_call(uint8_t index, uint8_t state, uint8_t flags)
{
    LEA_TBS_CALL_STATE_S *ccp_call;

    ccp_call = malloc(sizeof(LEA_TBS_CALL_STATE_S));
    if (!ccp_call) {
        BT_LOGE("error, malloc %s", __func__);
        return NULL;
    }
    ccp_call->index = index;
    ccp_call->state = state;
    ccp_call->flags = flags;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    bt_list_clear(g_ccpc_service.lea_calls);
    bt_list_add_tail(g_ccpc_service.lea_calls, ccp_call);
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return ccp_call;
}

static void lea_ccpc_call_delete(LEA_TBS_CALL_STATE_S *ccp_call)
{
    if (!ccp_call)
        return;
    free(ccp_call);
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void lea_ccpc_process_debug(lea_ccpc_msg_t *msg)
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
        LEA_TBS_CALL_STATE_S *call_states;
        call_states = (LEA_TBS_CALL_STATE_S *)msg->event_data.dataarry;
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
        LEA_TBS_CALLS_LIST_ITEM_S *calls;
        calls = (LEA_TBS_CALLS_LIST_ITEM_S *)msg->event_data.dataarry;
        BT_LOGD("%s, event:%d, tbs_id:%d, number:%d", __func__,
                msg->event, msg->event_data.tbs_id, msg->event_data.valueint32);
        for (int i = 0; i < msg->event_data.valueint32; i++) {
            BT_LOGD("index:%d, state:%d, flags:%d, uri:%s",
                    (calls + i)->index, (calls + i)->state,
                    (calls + i)->flags, (calls + i)->call_uri);
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

static void lea_ccpc_process_message(void *data)
{
    lea_ccpc_service_t *service = &g_ccpc_service;
    lea_ccpc_msg_t *msg = (lea_ccpc_msg_t *)data;

    lea_ccpc_process_debug(msg);

    switch (msg->event) {
    case STACK_EVENT_READ_PROVIDER_NAME: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        strcpy(service->info->provider_name, (char *)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_UCI: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        strcpy(service->info->uci, (char *)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_TECHNOLOGY: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->technology = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_URI_SCHEMES_SUPPORT_LIST: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        strcpy(service->info->uri_schemes, (char *)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_SIGNAL_STRENGTH: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->strength = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_SIGNAL_STRENGTH_REPORT_INTERVAL: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->interval = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CONTENT_CONTROL_ID: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->ccid = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_STATUS_FLAGS: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->status_flags = msg->event_data.valueint16;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CALL_CONTROL_OPTIONAL_OPCODES: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->opcodes = msg->event_data.valueint16;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_INCOMING_CALL: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        strcpy(service->info->uri, (char *)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_INCOMING_CALL_TARGET_BEARER_URI: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        strcpy(service->info->uri, (char *)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CALL_STATE: {
        LEA_TBS_CALL_STATE_S *call_states;
        call_states = (LEA_TBS_CALL_STATE_S *)msg->event_data.dataarry;

        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        for (int i = 0; i < msg->event_data.valueint32; i++) {
            if (lea_ccpc_find_call_by_index((call_states + i)->index) == NULL ||
                lea_ccpc_find_call_by_state((call_states + i)->state) == NULL) {
                lea_ccpc_add_call((call_states + i)->index,
                                  (call_states + i)->state,
                                  (call_states + i)->flags);
            }
        }
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_BEARER_LIST_CURRENT_CALL: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_READ_CALL_FRIENDLY_NAME: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        strcpy(service->info->friendly_name, (char *)msg->event_data.dataarry);
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_TERMINATION_REASON: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_0;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    case STACK_EVENT_CALL_CONTROL_RESULT: {
        pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
        service->info->tbs_id = msg->event_data.tbs_id;
        service->info->call_index = msg->event_data.valueint8_1;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

        CCPC_CALLBACK_FOREACH(g_ccpc_service.callbacks, test_cb, &msg->remote_addr);
        break;
    }

    default: {
        BT_LOGE("Idle: Unexpected stack event");
        break;
    }
    }
    lea_ccpc_msg_destory(msg);
}

static bt_status_t lea_ccpc_send_msg(lea_ccpc_msg_t *msg)
{
    assert(msg);

    do_in_service_loop(lea_ccpc_process_message, msg);

    return BT_STATUS_SUCCESS;
}

/****************************************************************************
 * sal callbacks
 ****************************************************************************/
void lea_ccpc_on_bearer_provider_name(bt_address_t *addr, uint32_t tbs_id, size_t size,
                                      const char *name)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_PROVIDER_NAME, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    strcpy((char *)msg->event_data.dataarry, name);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_bearer_uci(bt_address_t *addr, uint32_t tbs_id, size_t size, const char *uci)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_UCI, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    strcpy((char *)msg->event_data.dataarry, uci);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_bearer_technology(bt_address_t *addr, uint32_t tbs_id, uint8_t technology)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_READ_TECHNOLOGY, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = technology;

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_bearer_uri_schemes_supported_list(bt_address_t *addr, uint32_t tbs_id, size_t size,
                                                   const char *uri_schemes)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_URI_SCHEMES_SUPPORT_LIST, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    strcpy((char *)msg->event_data.dataarry, uri_schemes);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_bearer_signal_strength(bt_address_t *addr, uint32_t tbs_id, uint8_t strength)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_READ_SIGNAL_STRENGTH, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = strength;

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_bearer_signal_strength_report_interval(bt_address_t *addr, uint32_t tbs_id,
                                                        uint8_t interval)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_READ_SIGNAL_STRENGTH_REPORT_INTERVAL, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = interval;

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_content_control_id(bt_address_t *addr, uint32_t tbs_id, uint8_t ccid)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_READ_CONTENT_CONTROL_ID, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = ccid;

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_status_flags(bt_address_t *addr, uint32_t tbs_id, uint16_t status_flags)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_READ_STATUS_FLAGS, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint16 = status_flags;

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_call_control_optional_opcodes(bt_address_t *addr, uint32_t tbs_id,
                                               uint16_t opcodes)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_READ_CALL_CONTROL_OPTIONAL_OPCODES, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint16 = opcodes;

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_incoming_call(bt_address_t *addr, uint32_t tbs_id, uint8_t call_index, size_t size,
                               const char *uri)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_INCOMING_CALL, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    strcpy((char *)msg->event_data.dataarry, uri);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_incoming_call_target_bearer_uri(bt_address_t *addr, uint32_t tbs_id, uint8_t call_index,
                                                 size_t size, const char *uri)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_INCOMING_CALL_TARGET_BEARER_URI, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    strcpy((char *)msg->event_data.dataarry, uri);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_call_state(bt_address_t *addr, uint32_t tbs_id, uint32_t number,
                            LEA_TBS_CALL_STATE_S *states_s)
{
    lea_ccpc_msg_t *msg;

    if (number < 1) {
        BT_LOGW("%s ,the number of call state is zero!", __func__);
        return;
    }

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_CALL_STATE, addr, tbs_id,
                               sizeof(LEA_TBS_CALL_STATE_S) * number);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint32 = number;
    memcpy(&msg->event_data.dataarry, states_s, sizeof(LEA_TBS_CALL_STATE_S) * number);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_bearer_list_current_calls(bt_address_t *addr, uint32_t tbs_id, uint32_t number, size_t size,
                                           LEA_TBS_CALLS_LIST_ITEM_S *calls)
{
    lea_ccpc_msg_t *msg;

    if (number < 1) {
        BT_LOGW("%s ,the number of bearer list current call is zero!", __func__);
        return;
    }

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_BEARER_LIST_CURRENT_CALL, addr, tbs_id,
                               (sizeof(LEA_TBS_CALLS_LIST_ITEM_S) + size) * number);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint32 = number;
    memcpy(&msg->event_data.dataarry, calls, (sizeof(LEA_TBS_CALLS_LIST_ITEM_S) + size) * number);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_call_friendly_name(bt_address_t *addr, uint32_t tbs_id, uint8_t call_index, size_t size,
                                    const char *name)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new_ext(STACK_EVENT_READ_CALL_FRIENDLY_NAME, addr, tbs_id, size);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    strcpy((char *)msg->event_data.dataarry, name);

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_termination_reason(bt_address_t *addr, uint32_t tbs_id, uint8_t call_index,
                                    uint8_t reason)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_TERMINATION_REASON, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = call_index;
    msg->event_data.valueint8_1 = reason;

    lea_ccpc_send_msg(msg);
}

void lea_ccpc_on_call_control_result(bt_address_t *addr, uint32_t tbs_id, uint8_t opcode,
                                     uint8_t call_index, uint8_t result)
{
    lea_ccpc_msg_t *msg;

    msg = lea_ccpc_msg_new(STACK_EVENT_CALL_CONTROL_RESULT, addr, tbs_id);
    if (!msg) {
        BT_LOGE("%s, Failed to create msg", __func__);
        return;
    }

    msg->event_data.valueint8_0 = opcode;
    msg->event_data.valueint8_1 = call_index;
    msg->event_data.valueint8_2 = result;

    lea_ccpc_send_msg(msg);
}

/****************************************************************************
 * Private Data
 ****************************************************************************/
static bt_status_t bts_lea_ccpc_read_bearer_provider_name(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_bearer_provider_name(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_bearer_uci(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_bearer_uci(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_bearer_technology(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_bearer_technology(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_bearer_uri_schemes_supported_list(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_bearer_uri_schemes_supported_list(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_bearer_signal_strength(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_bearer_signal_strength(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_bearer_signal_strength_report_interval(void *handle,
                                                                            bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_bearer_signal_strength_report_interval(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_content_control_id(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_content_control_id(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_status_flags(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_status_flags(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_call_control_optional_opcodes(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_call_control_optional_opcodes(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_incoming_call(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_incoming_call(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_incoming_call_target_bearer_uri(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_incoming_call_target_bearer_uri(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_call_state(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_call_state(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_bearer_list_current_calls(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_bearer_list_current_calls(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_read_call_friendly_name(void *handle, bt_address_t *addr)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_read_call_friendly_name(addr, service->tbs_info.sid);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_call_control_by_index(void *handle, bt_address_t *addr, uint8_t opcode)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;
    uint8_t call_index;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }

    if (lea_ccpc_find_call_index(opcode, &call_index)) {
        ret = bt_sal_lea_tbc_call_control_by_index(addr, service->tbs_info.sid, opcode, call_index);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGE("%s fail, err:%d ", __func__, ret);
            return BT_STATUS_FAIL;
            pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
        }
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_originate_call(void *handle, bt_address_t *addr, uint8_t *uri)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_originate_call(addr, service->tbs_info.sid, uri);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static bt_status_t bts_lea_ccpc_join_calls(void *handle, bt_address_t *addr, uint8_t number,
                                           uint8_t *call_indexes)
{
    CHECK_ENABLED();
    bt_status_t ret;
    lea_ccpc_service_t *service = &g_ccpc_service;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    if (!service->tbs_info.num) {
        BT_LOGE("%s, tbs num is unexpected", __func__);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    ret = bt_sal_lea_tbc_join_calls(addr, service->tbs_info.sid, number, call_indexes);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s fail, err:%d ", __func__, ret);
        return BT_STATUS_FAIL;
        pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    }
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static void *bts_ccpc_register_callbacks(void *handle, lea_ccpc_callbacks_t *callbacks)
{
    if (!g_ccpc_service.started)
        return NULL;

    return bt_remote_callbacks_register(g_ccpc_service.callbacks, handle, (void *)callbacks);
}

static bool bts_ccpc_unregister_callbacks(void **handle, void *cookie)
{
    if (!g_ccpc_service.started)
        return false;

    return bt_remote_callbacks_unregister(g_ccpc_service.callbacks, handle, cookie);
}

static const lea_ccpc_interface_t leaCcpInterface = {
    .size = sizeof(leaCcpInterface),
    .read_bearer_provider_name = bts_lea_ccpc_read_bearer_provider_name,
    .read_bearer_uci = bts_lea_ccpc_read_bearer_uci,
    .read_bearer_technology = bts_lea_ccpc_read_bearer_technology,
    .read_bearer_uri_schemes_supported_list = bts_lea_ccpc_read_bearer_uri_schemes_supported_list,
    .read_bearer_signal_strength = bts_lea_ccpc_read_bearer_signal_strength,
    .read_bearer_signal_strength_report_interval = bts_lea_ccpc_read_bearer_signal_strength_report_interval,
    .read_content_control_id = bts_lea_ccpc_read_content_control_id,
    .read_status_flags = bts_lea_ccpc_read_status_flags,
    .read_call_control_optional_opcodes = bts_lea_ccpc_read_call_control_optional_opcodes,
    .read_incoming_call = bts_lea_ccpc_read_incoming_call,
    .read_incoming_call_target_bearer_uri = bts_lea_ccpc_read_incoming_call_target_bearer_uri,
    .read_call_state = bts_lea_ccpc_read_call_state,
    .read_bearer_list_current_calls = bts_lea_ccpc_read_bearer_list_current_calls,
    .read_call_friendly_name = bts_lea_ccpc_read_call_friendly_name,
    .call_control_by_index = bts_lea_ccpc_call_control_by_index,
    .originate_call = bts_lea_ccpc_originate_call,
    .join_calls = bts_lea_ccpc_join_calls,
    .register_callbacks = bts_ccpc_register_callbacks,
    .unregister_callbacks = bts_ccpc_unregister_callbacks,
};

/****************************************************************************
 * Public function
 ****************************************************************************/
static const void *get_lea_ccpc_profile_interface(void)
{
    return &leaCcpInterface;
}

static bt_status_t lea_ccpc_init(void)
{
    BT_LOGD("%s", __func__);
    lea_ccpc_service_t *service = &g_ccpc_service;
    service->tbs_info.num = 0;
    return BT_STATUS_SUCCESS;
}

static bt_status_t lea_ccpc_startup(profile_on_startup_t cb)
{
    bt_status_t status;
    pthread_mutexattr_t attr;
    lea_ccpc_service_t *service = &g_ccpc_service;

    BT_LOGD("%s", __func__);
    if (service->started)
        return BT_STATUS_SUCCESS;

    service->lea_calls = bt_list_new((bt_list_free_cb_t)lea_ccpc_call_delete);
    service->info = (bearer_tele_info_t *)malloc(sizeof(bearer_tele_info_t));
    service->callbacks = bt_callbacks_list_new(2);
    if (!service->callbacks) {
        status = BT_STATUS_NOMEM;
        goto fail;
    }

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&service->ccpc_lock, &attr);

    service->started = true;
    return BT_STATUS_SUCCESS;

fail:
    bt_list_free(service->lea_calls);
    free((void *)service->info);
    bt_callbacks_list_free(service->callbacks);
    pthread_mutex_destroy(&service->ccpc_lock);
    return status;
}

static bt_status_t lea_ccpc_shutdown(profile_on_shutdown_t cb)
{
    BT_LOGD("%s", __func__);
    if (!g_ccpc_service.started)
        return BT_STATUS_SUCCESS;

    pthread_mutex_lock(&g_ccpc_service.ccpc_lock);
    g_ccpc_service.started = false;

    bt_list_free(g_ccpc_service.lea_calls);
    g_ccpc_service.lea_calls = NULL;
    free((void *)g_ccpc_service.info);
    g_ccpc_service.info = NULL;
    bt_callbacks_list_free(g_ccpc_service.callbacks);
    g_ccpc_service.callbacks = NULL;
    pthread_mutex_unlock(&g_ccpc_service.ccpc_lock);
    pthread_mutex_destroy(&g_ccpc_service.ccpc_lock);

    return BT_STATUS_SUCCESS;
}

static void lea_ccpc_cleanup(void)
{
    BT_LOGD("%s", __func__);
}

static int lea_ccpc_dump(void)
{
    printf("impl leaudio ccpc dump");
    return 0;
}

static const profile_service_t lea_ccpc_service = {
    .auto_start = true,
    .name = "lea_ccpc",
    .id = PROFILE_LEA_CCPC,
    .transport = BT_TRANSPORT_BLE,
    .uuid = {BT_UUID128_TYPE, { 0 }},
    .init = lea_ccpc_init,
    .startup = lea_ccpc_startup,
    .shutdown = lea_ccpc_shutdown,
    .process_msg = NULL,
    .get_state = NULL,
    .get_profile_interface = get_lea_ccpc_profile_interface,
    .cleanup = lea_ccpc_cleanup,
    .dump = lea_ccpc_dump,
};

void register_lea_ccpc_service(void)
{
    register_service(&lea_ccpc_service);
}

void adpt_tbs_sid_changed(uint32_t sid)
{
    lea_ccpc_service_t *service = &g_ccpc_service;
    BT_LOGD("%s, sid:%d", __func__, sid);
    service->tbs_info.num = CONFIG_BLUETOOTH_LEAUDIO_SERVER_CALL_CONTROL_NUMBER;
    service->tbs_info.sid = sid;
}

#endif