/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "stack_adapter_a2dp_sink.h"
#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"
#include "stack_adapter_service_base.h"

#include "state_machine.h"

#include "btm_a2dp_source.h"
#include "bts_a2dp_source.h"
#include "bts_a2dp_codec.h"
#include "bts_a2dp_control.h"
#include "bts_a2dp_event.h"
#include "bts_a2dp_source_audio.h"
#include "bts_a2dp_state_machine.h"
#include "utils/utils.h"
#define LOG_TAG "a2dp_stm"
#include "log.h"

#define A2DP_CONNECT_TIMEOUT 4 * 1000
#define A2DP_START_TIMEOUT 2 * 1000

typedef enum pending_state {
    PENDING_NONE = 0x0,
    PENDING_START = 0X02,
    PENDING_STOP = 0x04
} pending_state_t;
typedef struct _a2dp_state_machine {
    state_machine_t sm;
    a2dp_source_t* service;
    bt_address addr;
    pending_state_t pending;
    uv_timer_t *connect_timer;
    uv_timer_t *start_timer;
} a2dp_state_machine_t;

typedef struct {
    a2dp_state_machine_t* a2dp_sm;
    a2dp_event_t* a2dp_event;
} a2dp_inter_event_t;

static void idle_enter(state_machine_t* sm);
static void idle_exit(state_machine_t* sm);
static void opening_enter(state_machine_t* sm);
static void opening_exit(state_machine_t* sm);
static void opened_enter(state_machine_t* sm);
static void opened_exit(state_machine_t* sm);
static void started_enter(state_machine_t* sm);
static void started_exit(state_machine_t* sm);
static void closing_enter(state_machine_t* sm);
static void closing_exit(state_machine_t* sm);

static bool idle_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool opening_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool opened_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool started_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool closing_process_event(state_machine_t* sm, uint32_t event, void* p_data);

static const state_t idle_state = {
    .state_name = "Idle",
    .enter = idle_enter,
    .exit = idle_exit,
    .process_event = idle_process_event,
};

static const state_t opening_state = {
    .state_name = "Opening",
    .enter = opening_enter,
    .exit = opening_exit,
    .process_event = opening_process_event,
};

static const state_t opened_state = {
    .state_name = "Opened",
    .enter = opened_enter,
    .exit = opened_exit,
    .process_event = opened_process_event,
};

static const state_t started_state = {
    .state_name = "Started",
    .enter = started_enter,
    .exit = started_exit,
    .process_event = started_process_event,
};

static const state_t closing_state = {
    .state_name = "Closing",
    .enter = closing_enter,
    .exit = closing_exit,
    .process_event = closing_process_event,
};

static char* stack_event_to_string(a2dp_event_type_t event)
{
    switch (event) {
        CASE_RETURN_STR(CONNECT_REQ)
        CASE_RETURN_STR(DISCONNECT_REQ)
        CASE_RETURN_STR(STREAM_START_REQ)
        CASE_RETURN_STR(STREAM_SUSPEND_REQ)
        CASE_RETURN_STR(CONNECTED_EVT)
        CASE_RETURN_STR(DISCONNECTED_EVT)
        CASE_RETURN_STR(STREAM_STARTED_EVT)
        CASE_RETURN_STR(STREAM_SUSPENDED_EVT)
        CASE_RETURN_STR(STREAM_CLOSED_EVT)
        CASE_RETURN_STR(STREAM_MTU_CONFIG_EVT)
        CASE_RETURN_STR(CODEC_CONFIG_EVT)
        CASE_RETURN_STR(DEVICE_CODEC_STATE_CHANGE_EVT)
        CASE_RETURN_STR(CONNECT_TIMEOUT)
        CASE_RETURN_STR(START_TIMEOUT)
    default:
        return "UNKNOWN_EVENT";
    }
}

static void bts_a2dp_report_connection_state(a2dp_source_t* service, bt_address addr, a2dp_connection_state_t state)
{
    BT_LOGD("%s, addr:%s, state: %d", __func__, addr_str(addr), state);
    if (service->callbacks)
        service->callbacks->connection_state_cb(addr, state);
}

static void bts_a2dp_report_audio_state(a2dp_source_t* service, bt_address addr, a2dp_audio_state_t state)
{
    BT_LOGD("%s, addr:%s, state: %d", __func__, addr_str(addr), state);
    if (service->callbacks)
        service->callbacks->audio_state_cb(addr, state);
}

static void bts_a2dp_report_audio_config_state(a2dp_source_t* service, bt_address addr)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));
    if (service->callbacks)
        service->callbacks->audio_source_config_cb(addr);
}

static void a2dp_connect_timeout_callback(char* data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)data;
    a2dp_event_t* a2dp_event;

    a2dp_event = a2dp_event_new(CONNECT_TIMEOUT, NULL);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void a2dp_start_timeout_callback(char* data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)data;
    a2dp_event_t* a2dp_event;

    a2dp_event = a2dp_event_new(START_TIMEOUT, NULL);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void idle_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
}

static void idle_exit(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;
    state_t *prev_state = hsm_get_previous_state(sm);

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    if (prev_state != NULL) {
        bts_a2dp_report_connection_state(service, a2dp_sm->addr,
            A2DP_CONNECTION_STATE_DISCONNECTED);
    }
}

static bool idle_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;
    a2dp_event_data_t* event_data = (a2dp_event_data_t*)p_data;

    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case CONNECT_REQ: {
        SERVICE_BT_STATUS status;
        status = service_adapter_a2dp_source_connect(event_data->bd_addr,
            SERVICE_AVDTP_CODEC_TYPE_SBC);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            bts_a2dp_report_connection_state(service, a2dp_sm->addr,
                A2DP_CONNECTION_STATE_DISCONNECTED);
            break;
        }
        hsm_transition_to(sm, &opening_state);
        break;
    }

    case CONNECTED_EVT:
        hsm_transition_to(sm, &opened_state);
        break;

    default:
        break;
    }

    return true;
}

static void opening_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    a2dp_sm->connect_timer = start_timer(A2DP_CONNECT_TIMEOUT, 0, a2dp_connect_timeout_callback, a2dp_sm);
    bts_a2dp_report_connection_state(service, a2dp_sm->addr, A2DP_CONNECTION_STATE_CONNECTING);
}

static void opening_exit(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
}

static bool opening_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;
    a2dp_event_data_t* event_data = (a2dp_event_data_t*)p_data;
    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case DISCONNECT_REQ: {
        SERVICE_BT_STATUS status;

        status = service_adapter_a2dp_source_disconnect(event_data->bd_addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect failed");
        }
        hsm_transition_to(sm, &idle_state);
        break;
    }

    case CONNECTED_EVT:
        if (a2dp_sm->connect_timer) {
            stop_timer(a2dp_sm->connect_timer);
            a2dp_sm->connect_timer = NULL;
        }
        hsm_transition_to(sm, &opened_state);
        break;

    case DISCONNECTED_EVT:
    case CONNECT_TIMEOUT:
        stop_timer(a2dp_sm->connect_timer);
        a2dp_sm->connect_timer = NULL;
        hsm_transition_to(sm, &idle_state);
        break;

    default:
        break;
    }

    return true;
}

static void opened_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;
    state_t *prev_state = hsm_get_previous_state(sm);

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    if (prev_state == &idle_state || prev_state == &opening_state) {
        bts_a2dp_source_on_connection_changed(true);
        bts_a2dp_report_connection_state(service, a2dp_sm->addr,
            A2DP_CONNECTION_STATE_CONNECTED);
    }
}

static void opened_exit(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
}

static bool opened_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;
    a2dp_event_data_t* event_data = (a2dp_event_data_t*)p_data;
    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case DISCONNECT_REQ: {
        SERVICE_BT_STATUS status;

        status = service_adapter_a2dp_source_disconnect(event_data->bd_addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect failed");
        }
        hsm_transition_to(sm, &closing_state);
        break;
    }
    case STREAM_START_REQ: {
        SERVICE_BT_STATUS status;
        status = service_adapter_a2dp_source_start_stream(event_data->bd_addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Stream start failed");
            break;
        }
        a2dp_sm->pending |= PENDING_START;
        a2dp_sm->start_timer = start_timer(A2DP_START_TIMEOUT, 0, a2dp_start_timeout_callback, a2dp_sm);
        break;
    }

    case DISCONNECTED_EVT:
        if (a2dp_sm->pending & PENDING_START) {
            a2dp_sm->pending &= ~PENDING_START;
            stop_timer(a2dp_sm->start_timer);
            a2dp_sm->start_timer = NULL;
            // When pending on start request, then received stream close event
            //call bts_a2dp_source_on_started(), shoule ack start failure;
            bts_a2dp_source_on_started(false);
            return true;
        }
        bts_a2dp_source_on_connection_changed(false);
        hsm_transition_to(sm, &idle_state);
        break;

    case STREAM_STARTED_EVT:
        // If remote tries to start A2DP when DUT is A2DP Source, then Suspend.
        // If A2DP is Sink and call is active, then disconnect the AVDTP channel.
        a2dp_sm->pending &= ~PENDING_START;
        stop_timer(a2dp_sm->start_timer);
        a2dp_sm->start_timer = NULL;
        bts_a2dp_source_on_started(true);
        hsm_transition_to(sm, &started_state);
        break;

    case STREAM_SUSPENDED_EVT:
    case STREAM_CLOSED_EVT:
        a2dp_sm->pending = PENDING_NONE;
        bts_a2dp_source_on_stopped();
        break;

    case STREAM_MTU_CONFIG_EVT:
        bts_a2dp_source_set_mtu(event_data->mtu);
        BT_LOGD("STREAM_MTU_CONFIG_EVT :stream_chnl_mtu:%d", event_data->mtu);
        break;

    case DEVICE_CODEC_STATE_CHANGE_EVT:
        bts_a2dp_report_audio_config_state(service, a2dp_sm->addr);
        break;

    case START_TIMEOUT: {
        a2dp_sm->pending &= ~PENDING_START;
        stop_timer(a2dp_sm->start_timer);
        a2dp_sm->start_timer = NULL;
        bts_a2dp_source_on_started(false);
        break;
    }

    default:
        break;
    }

    return true;
}

static void started_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    bts_a2dp_report_audio_state(service, a2dp_sm->addr,
        A2DP_AUDIO_STATE_STARTED);
}

static void started_exit(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
}

static bool started_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;
    a2dp_event_data_t* event_data = (a2dp_event_data_t*)p_data;
    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case DISCONNECT_REQ: {
        SERVICE_BT_STATUS status;

        status = service_adapter_a2dp_source_disconnect(event_data->bd_addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect failed");
        }
        bts_a2dp_source_on_connection_changed(false);
        hsm_transition_to(sm, &closing_state);
        break;
    }

    case STREAM_START_REQ:
        // We were started remotely, just ACK back the local request
        bts_a2dp_source_on_started(true);
        break;

    case STREAM_SUSPEND_REQ: {
        SERVICE_BT_STATUS status;
        status = service_adapter_a2dp_source_suspend_stream(event_data->bd_addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Stream suspend failed");
        }
        bts_a2dp_source_on_stopped();
        break;
    }

    case DISCONNECTED_EVT:
        bts_a2dp_source_on_connection_changed(false);
        hsm_transition_to(sm, &idle_state);
        break;

    case STREAM_SUSPENDED_EVT:
        //If remote suspend, notify ffmpeg to
        // suspend/stop stream.
        bts_a2dp_source_on_suspended();
        bts_a2dp_report_audio_state(service, a2dp_sm->addr,
            A2DP_AUDIO_STATE_STOPPED);
        hsm_transition_to(sm, &opened_state);
        break;

    case STREAM_CLOSED_EVT:
        bts_a2dp_source_on_stopped();
        bts_a2dp_report_audio_state(service, a2dp_sm->addr,
            A2DP_AUDIO_STATE_STOPPED);
        hsm_transition_to(sm, &opened_state);
        break;

    case DEVICE_CODEC_STATE_CHANGE_EVT:
        bts_a2dp_report_audio_config_state(service, a2dp_sm->addr);
        break;

    default:
        break;
    }

    return true;
}

static void closing_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    bts_a2dp_report_connection_state(service, a2dp_sm->addr,
            A2DP_CONNECTION_STATE_DISCONNECTING);
}

static void closing_exit(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
}

static bool closing_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_source_t* service = a2dp_sm->service;
    //a2dp_event_data_t *event_data = (a2dp_event_data_t *)p_data;
    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case STREAM_SUSPEND_REQ:
    case STREAM_CLOSED_EVT:
    case STREAM_SUSPENDED_EVT:
        bts_a2dp_source_on_stopped();
        break;

    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &idle_state);
        break;

    default:
        break;
    }

    return true;
}

static void a2dp_state_machine_event_dispatch(a2dp_state_machine_t* a2dp_sm, a2dp_event_t* a2dp_event)
{
    if (!a2dp_event || !a2dp_sm)
        return;

    hsm_dispatch_event(&a2dp_sm->sm, a2dp_event->event, &a2dp_event->event_data);
}

a2dp_state_machine_t* a2dp_state_machine_new(void* context, bt_address bd_addr)
{
    a2dp_state_machine_t* a2dp_sm;

    a2dp_sm = (a2dp_state_machine_t*)malloc(sizeof(a2dp_state_machine_t));
    if (!a2dp_sm)
        return NULL;

    a2dp_sm->service = (a2dp_source_t*)context;
    hsm_ctor(&a2dp_sm->sm, (state_t*)&idle_state);
    memcpy(a2dp_sm->addr, bd_addr, sizeof(bt_address));

    return a2dp_sm;
}

void a2dp_state_machine_destory(a2dp_state_machine_t* a2dp_sm)
{
    if (!a2dp_sm)
        return;

    hsm_dtor(&a2dp_sm->sm);
    free((void*)a2dp_sm);
}

void a2dp_state_machine_handle_event(a2dp_state_machine_t* sm,
    a2dp_event_t* a2dp_event)
{
    a2dp_state_machine_event_dispatch(sm, a2dp_event);
}

a2dp_state_t a2dp_state_machine_get_state(a2dp_state_machine_t* sm)
{
    state_t* cur_state = hsm_get_current_state(&sm->sm);
    a2dp_state_t state;
    if (cur_state == (state_t*)&idle_state) {
        state = A2DP_STATE_IDLE;
    } else if (cur_state == (state_t*)&opening_state) {
        state = A2DP_STATE_OPENING;
    } else if (cur_state == (state_t*)&opened_state) {
        state = A2DP_STATE_OPENED;
    } else if (cur_state == (state_t*)&started_state) {
        state = A2DP_STATE_STARTED;
    } else if (cur_state == (state_t*)&closing_state) {
        state = A2DP_STATE_CLOSING;
    } else {
        state = A2DP_STATE_IDLE;
    }

    return state;
}

const char * a2dp_state_machine_current_state(a2dp_state_machine_t* sm)
{
    return hsm_get_current_state_name(&sm->sm);
}