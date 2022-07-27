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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_UORB
#include <connectivity/bt.h>
#include <uORB/uORB.h>
#endif
#include "stack_adapter_gap.h"
#include "stack_adapter_a2dp_sink.h"
#include "stack_adapter_avrcp.h"
#include "stack_adapter_a2dp_source.h"
#include "stack_adapter_common.h"
#include "stack_adapter_service_base.h"

#include "state_machine.h"
#include "bts_a2dp_event.h"
#include "bts_a2dp_source.h"
#include "bts_a2dp_sink.h"
#include "bts_a2dp_audio.h"
#include "bts_a2dp_state_machine.h"
#include "bts_avrc.h"
#include "utils/utils.h"
#ifdef CONFIG_BLUETOOTH_VENDOR_DEPENDENCY_BES
#include "bt_drv_reg_op.h"
#endif
#define LOG_TAG "a2dp_stm"
#include "log.h"

#ifndef CONFIG_BLUETOOTH_A2DP_CONNECT_TIMEOUT
#define A2DP_CONNECT_TIMEOUT 6000
#else
#define A2DP_CONNECT_TIMEOUT (CONFIG_BLUETOOTH_A2DP_CONNECT_TIMEOUT * 1000)
#endif
#define A2DP_START_TIMEOUT 5000
#define A2DP_SUSPEND_TIMEOUT 5000
#define A2DP_DELAY_START 100
#define A2DP_DELAY_SUSPEND 200
#ifdef CONFIG_BLUETOOTH_A2DP_AAC_CODEC
#define A2DP_PREFERRED_CODEC    SERVICE_AVDTP_CODEC_TYPE_MPEG2_4_AAC
#else
#define A2DP_PREFERRED_CODEC    SERVICE_AVDTP_CODEC_TYPE_SBC
#endif

typedef enum pending_state {
    PENDING_NONE = 0x0,
    PENDING_START = 0X02,
    PENDING_STOP = 0x04,
} pending_state_t;

typedef struct _a2dp_state_machine {
    state_machine_t sm;
    void* service;
    bt_address addr;
    pending_state_t pending;
    bool audio_ready;
    uint8_t peer_sep;
    uv_timer_t* connect_timer;
    uv_timer_t* start_timer;
    uv_timer_t* delay_start_timer;
    uv_timer_t* delay_suspend_timer;
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
        CASE_RETURN_STR(DELAY_STREAM_START_REQ)
        CASE_RETURN_STR(STREAM_SUSPEND_REQ)
        CASE_RETURN_STR(CONNECTED_EVT)
        CASE_RETURN_STR(DISCONNECTED_EVT)
        CASE_RETURN_STR(STREAM_STARTED_EVT)
        CASE_RETURN_STR(STREAM_SUSPENDED_EVT)
        CASE_RETURN_STR(STREAM_CLOSED_EVT)
#ifdef CONFIG_BLUETOOTH_A2DP_PEER_PARTIAL_RECONN
        CASE_RETURN_STR(PEER_PARTIAL_RECONN_EVT)
#endif
        CASE_RETURN_STR(CODEC_CONFIG_EVT)
        CASE_RETURN_STR(DEVICE_CODEC_STATE_CHANGE_EVT)
        CASE_RETURN_STR(DATA_IND_EVT)
        CASE_RETURN_STR(CONNECT_TIMEOUT)
        CASE_RETURN_STR(START_TIMEOUT)
        CASE_RETURN_STR(STREAM_SUSPEND_DELAY)
    default:
        return "UNKNOWN_EVENT";
    }
}

#ifdef CONFIG_UORB
static void broadcast_a2dp_state(int orb_fd, bt_address addr, int conn_state, int audio_state)
{
    struct a2dp_state uORB_state;
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uORB_state.timestamp = ts.tv_sec * 1000 + ts.tv_nsec / 1000000UL;
    uORB_state.conn_state = conn_state;
    uORB_state.audio_state = audio_state;
    memcpy(uORB_state.addr, addr, 6);

    if (orb_fd > 0) {
        int ret = orb_publish(ORB_ID(a2dp_state), orb_fd, &uORB_state);
        if (ret != 0)
            BT_LOGE("Failed to publish connection state");
    }
}
#endif

static void bts_a2dp_report_connection_state(a2dp_state_machine_t* stm, bt_address addr, a2dp_connection_state_t state)
{
    int orb_fd;
    BT_LOGD("%s, addr:%s, state: %d", __func__, addr_str(addr), state);
    if(state == A2DP_CONNECTION_STATE_CONNECTED)
        BT_LOGD("PERFORMANCE-A2DP-BTM-CONNECTED");

    if (stm->peer_sep == SEP_SRC) {
        a2dp_sink_t* snk_service = (a2dp_sink_t*)stm->service;

        orb_fd = snk_service->orb_fd;
        if (snk_service->callbacks)
            snk_service->callbacks->connection_state_cb(addr, state);
    } else {
        a2dp_source_t* src_service = (a2dp_source_t*)stm->service;

        orb_fd = src_service->orb_fd;
        if (src_service->callbacks)
            src_service->callbacks->connection_state_cb(addr, state);
    }
#ifdef CONFIG_UORB
    broadcast_a2dp_state(orb_fd, addr, state, A2DP_AUDIO_NOT_READY);
#else
    (void)orb_fd;
#endif
}

static void bts_a2dp_report_audio_state(a2dp_state_machine_t* stm, bt_address addr, a2dp_audio_state_t state)
{
    int orb_fd;
    BT_LOGD("%s, addr:%s, state: %d", __func__, addr_str(addr), state);

    if (stm->peer_sep == SEP_SRC) {
        a2dp_sink_t* snk_service = (a2dp_sink_t*)stm->service;

        orb_fd = snk_service->orb_fd;
        if (snk_service->callbacks)
            snk_service->callbacks->audio_state_cb(addr, state);
    } else {
        a2dp_source_t* src_service = (a2dp_source_t*)stm->service;

        orb_fd = src_service->orb_fd;
        if (src_service->callbacks)
            src_service->callbacks->audio_state_cb(addr, state);
    }
#ifdef CONFIG_UORB
    broadcast_a2dp_state(orb_fd, addr, PROFILE_CONN_CONNECTED, state);
#else
    (void)orb_fd;
#endif
}

static void bts_a2dp_report_audio_config_state(a2dp_state_machine_t* stm, bt_address addr)
{
    int orb_fd;
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));

    if (stm->peer_sep == SEP_SRC) {
        a2dp_sink_t* snk_service = (a2dp_sink_t*)stm->service;

        orb_fd = snk_service->orb_fd;
        if (snk_service->callbacks)
            snk_service->callbacks->audio_sink_config_cb(addr);
    } else {
        a2dp_source_t* src_service = (a2dp_source_t*)stm->service;

        orb_fd = src_service->orb_fd;
        if (src_service->callbacks)
            src_service->callbacks->audio_source_config_cb(addr);
    }
#ifdef CONFIG_UORB
    broadcast_a2dp_state(orb_fd, addr, PROFILE_CONN_CONNECTED, A2DP_AUDIO_STOPPED);
#else
    (void)orb_fd;
#endif
}

static void a2dp_connect_timeout_callback(char* data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)data;
    a2dp_event_t* a2dp_event;

    a2dp_event = a2dp_event_new(CONNECT_TIMEOUT, a2dp_sm->addr);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void a2dp_start_timeout_callback(char* data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)data;
    a2dp_event_t* a2dp_event;

    a2dp_event = a2dp_event_new(START_TIMEOUT, a2dp_sm->addr);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void a2dp_delay_start_timeout_callback(char* data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)data;
    a2dp_event_t* a2dp_event;

    a2dp_event = a2dp_event_new(DELAY_STREAM_START_REQ, a2dp_sm->addr);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void a2dp_delay_suspend_timeout_callback(char* data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)data;
    a2dp_event_t* a2dp_event;

    a2dp_event = a2dp_event_new(STREAM_SUSPEND_REQ, a2dp_sm->addr);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static bool flag_isset(a2dp_state_machine_t *a2dp_sm, pending_state_t flag)
{
    return (bool)(a2dp_sm->pending & flag);
}

static void flag_set(a2dp_state_machine_t *a2dp_sm, pending_state_t flag)
{
    a2dp_sm->pending |= flag;
}

static void flag_clear(a2dp_state_machine_t *a2dp_sm, pending_state_t flag)
{
    a2dp_sm->pending &= ~flag;
}

static void idle_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    state_t* prev_state = hsm_get_previous_state(sm);

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    a2dp_sm->audio_ready = false;
    if (prev_state != NULL) {
        bts_a2dp_report_connection_state(a2dp_sm, a2dp_sm->addr,
            A2DP_CONNECTION_STATE_DISCONNECTED);
    }
}

static void idle_exit(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Exit, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
}

static bool idle_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;
    a2dp_event_data_t* data = (a2dp_event_data_t*)p_data;

    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case CONNECT_REQ: {
        SERVICE_BT_STATUS status;
        BT_LOGD("PERFORMANCE-A2DP-SRC-BLUELET-CONNECT-START");
        if (a2dp_sm->peer_sep == SEP_SNK)
            status = service_adapter_a2dp_source_connect(data->bd_addr,
                                                         A2DP_PREFERRED_CODEC);
        else
            status = service_adapter_a2dp_sink_connect(data->bd_addr,
                                                       A2DP_PREFERRED_CODEC);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            bts_a2dp_report_connection_state(a2dp_sm, a2dp_sm->addr,
                A2DP_CONNECTION_STATE_DISCONNECTED);
            break;
        }
        hsm_transition_to(sm, &opening_state);
        break;
    }

    case CONNECTED_EVT:
        hsm_transition_to(sm, &opened_state);
        break;

#ifdef CONFIG_BLUETOOTH_A2DP_PEER_PARTIAL_RECONN
    case PEER_PARTIAL_RECONN_EVT:
        if (a2dp_sm->peer_sep == SEP_SNK) {
            SERVICE_BT_STATUS status;
            status = service_adapter_a2dp_source_connect(data->bd_addr,
                                                         A2DP_PREFERRED_CODEC);
            if (status != SERVICE_BT_STATUS_SUCCESS) {
                bts_a2dp_report_connection_state(a2dp_sm, a2dp_sm->addr,
                    A2DP_CONNECTION_STATE_DISCONNECTED);
            }
        }
        break;
#endif
    default:
        break;
    }

    return true;
}

static void opening_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    a2dp_sm->connect_timer = start_timer(A2DP_CONNECT_TIMEOUT, 0, a2dp_connect_timeout_callback, a2dp_sm);
    bts_a2dp_report_connection_state(a2dp_sm, a2dp_sm->addr, A2DP_CONNECTION_STATE_CONNECTING);
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
    a2dp_event_data_t* data = (a2dp_event_data_t*)p_data;
    SERVICE_BT_STATUS status;
    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case DISCONNECT_REQ: {
        if (a2dp_sm->peer_sep == SEP_SNK)
            status = service_adapter_a2dp_source_disconnect(data->bd_addr);
        else
            status = service_adapter_a2dp_sink_disconnect(data->bd_addr);
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
    state_t* prev_state = hsm_get_previous_state(sm);

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    if (prev_state == &idle_state || prev_state == &opening_state) {
        /* if we are accept link as a2dp src, change the av link role to master */
        if (a2dp_sm->peer_sep == SEP_SNK)
            service_adapter_gap_set_link_role(a2dp_sm->addr, BT_ROLE_MASTER);
#ifdef CONFIG_BLUETOOTH_AVRCP_CT
        if (a2dp_sm->peer_sep == SEP_SRC)
            service_adapter_avrcp_connect(a2dp_sm->addr);
#endif
        bts_a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, true);
        bts_a2dp_report_connection_state(a2dp_sm, a2dp_sm->addr,
                                         A2DP_CONNECTION_STATE_CONNECTED);
    }
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    else if (prev_state == &started_state) {
        bts_avrcp_notify_play_state_changed(a2dp_sm->addr, PLAY_STATUS_PAUSED);
    }
#endif
#ifdef CONFIG_BLUETOOTH_VENDOR_DEPENDENCY_BES
    uint16_t acl_handle = service_adapter_gap_get_acl_handle(a2dp_sm->addr);
    bt_drv_reg_op_set_music_link(acl_handle-0x80);
    BT_LOGD("%s acl handle:%08x,linkid:%08x", __func__, acl_handle, acl_handle-0x80);
#endif
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

    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case DISCONNECT_REQ: {
        SERVICE_BT_STATUS status;

        if (a2dp_sm->peer_sep == SEP_SNK)
            status = service_adapter_a2dp_source_disconnect(a2dp_sm->addr);
        else
            status = service_adapter_a2dp_sink_disconnect(a2dp_sm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("A2dp disconnect failed");
        }
        status = service_adapter_avrcp_disconnect(a2dp_sm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Avrc disconnect failed");
        }
        hsm_transition_to(sm, &closing_state);
        break;
    }
    case STREAM_START_REQ: {
        SERVICE_BT_STATUS status;

        /* if we are in suspending substate, ignore this request */
        if (flag_isset(a2dp_sm, PENDING_STOP) || flag_isset(a2dp_sm, PENDING_START)) {
            BT_LOGD("in suspending or starting substate, ignore this request");
            break;
        }
        if (!a2dp_sm->audio_ready) {
            BT_LOGE("A2DP Audio is not ready, Ignore start cmd");
            break;
        }
        status = service_adapter_a2dp_source_start_stream(a2dp_sm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Stream start failed");
            break;
        }
        flag_set(a2dp_sm, PENDING_START);
        a2dp_sm->start_timer = start_timer(A2DP_START_TIMEOUT, 0, a2dp_start_timeout_callback, a2dp_sm);
        break;
    }
    case DELAY_STREAM_START_REQ: {
        SERVICE_BT_STATUS status;

        if (a2dp_sm->delay_start_timer)
            stop_timer(a2dp_sm->delay_start_timer);
        a2dp_sm->delay_start_timer = NULL;
        if (flag_isset(a2dp_sm, PENDING_START))
            break;
        status = service_adapter_a2dp_source_start_stream(a2dp_sm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Stream delay start failed");
            break;
        }
        flag_set(a2dp_sm, PENDING_START);
        a2dp_sm->start_timer = start_timer(A2DP_START_TIMEOUT, 0, a2dp_start_timeout_callback, a2dp_sm);
        break;
    }
    case DISCONNECTED_EVT:
        if (flag_isset(a2dp_sm, PENDING_START)) {
            flag_clear(a2dp_sm, PENDING_START);
            stop_timer(a2dp_sm->start_timer);
            a2dp_sm->start_timer = NULL;
            /* When pending on start request, then received stream close event
               call bts_a2dp_audio_on_started(), shoule ack start failure; */
            bts_a2dp_audio_on_started(a2dp_sm->peer_sep, false);
        }
        bts_a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, false);
        hsm_transition_to(sm, &idle_state);
        break;

    case STREAM_STARTED_EVT:
        if (a2dp_sm->peer_sep == SEP_SNK) {
            /* If remote tries to start A2DP when DUT is A2DP Source, then Suspend.
             If A2DP is Sink and call is active, then disconnect the AVDTP channel. */
            flag_clear(a2dp_sm, PENDING_START);
            stop_timer(a2dp_sm->start_timer);
            a2dp_sm->start_timer = NULL;
            if (a2dp_sm->delay_start_timer)
                stop_timer(a2dp_sm->delay_start_timer);
            a2dp_sm->delay_start_timer = NULL;
        }

        if (!a2dp_sm->audio_ready) {
            BT_LOGW("A2dp device is not ready: %s", stack_event_to_string(event));
            break;
        }

        bts_a2dp_audio_on_started(a2dp_sm->peer_sep, true);
        hsm_transition_to(sm, &started_state);
        break;

    case STREAM_SUSPENDED_EVT:
    case STREAM_CLOSED_EVT:
        if (flag_isset(a2dp_sm, PENDING_STOP) && a2dp_sm->delay_start_timer) {
            stop_timer(a2dp_sm->delay_start_timer);
            a2dp_sm->delay_start_timer = start_timer(A2DP_DELAY_START, 0, a2dp_delay_start_timeout_callback, a2dp_sm);
        }
        flag_clear(a2dp_sm, PENDING_STOP);
        bts_a2dp_report_audio_state(a2dp_sm, a2dp_sm->addr,
                                    A2DP_AUDIO_STATE_STOPPED);
        bts_a2dp_audio_on_stopped(a2dp_sm->peer_sep);
        break;

    case DEVICE_CODEC_STATE_CHANGE_EVT:
        a2dp_sm->audio_ready = true;
        bts_a2dp_report_audio_config_state(a2dp_sm, a2dp_sm->addr);
        bts_a2dp_audio_setup_codec(a2dp_sm->peer_sep, a2dp_sm->addr);
        break;

    case START_TIMEOUT: {
        flag_clear(a2dp_sm, PENDING_START);
        stop_timer(a2dp_sm->start_timer);
        a2dp_sm->start_timer = NULL;
        bts_a2dp_audio_on_started(a2dp_sm->peer_sep, false);
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

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));

    bts_a2dp_report_audio_state(a2dp_sm, a2dp_sm->addr,
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
    if (event != DATA_IND_EVT)
        BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
            stack_event_to_string(event),
            addr_str(a2dp_sm->addr));
    switch (event) {
    case DISCONNECT_REQ: {
        SERVICE_BT_STATUS status;
        if (a2dp_sm->peer_sep == SEP_SNK)
            status = service_adapter_a2dp_source_disconnect(a2dp_sm->addr);
        else
            status = service_adapter_a2dp_sink_disconnect(a2dp_sm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect failed");
        }
        status = service_adapter_avrcp_disconnect(a2dp_sm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Avrc disconnect failed");
        }
        bts_a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, false);
        hsm_transition_to(sm, &closing_state);
        break;
    }

    case STREAM_SUSPEND_DELAY:
        if (!a2dp_sm->delay_suspend_timer)
            a2dp_sm->delay_suspend_timer = start_timer(A2DP_DELAY_SUSPEND, 0, a2dp_delay_suspend_timeout_callback, a2dp_sm);
        break;

    case STREAM_START_REQ:
        if (a2dp_sm->delay_suspend_timer) {
            BT_LOGD("need stop suspend timer");
            stop_timer(a2dp_sm->delay_suspend_timer);
            a2dp_sm->delay_suspend_timer = NULL;
            bts_a2dp_audio_on_started(a2dp_sm->peer_sep, true);
            break;
        }
        /* received start request when we are in pending a2dp stream
           suspend sub-state, we need restart stream and transmit state to
           opened state, and wait for started event */
        if (flag_isset(a2dp_sm, PENDING_STOP)) {
            a2dp_sm->delay_start_timer = start_timer(A2DP_SUSPEND_TIMEOUT, 0, a2dp_delay_start_timeout_callback, a2dp_sm);
            hsm_transition_to(sm, &opened_state);
            break;
        }
        // We were started remotely, just ACK back the local request
        if (a2dp_sm->peer_sep == SEP_SNK)
            bts_a2dp_audio_on_started(a2dp_sm->peer_sep, true);
        break;

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case DATA_IND_EVT: {
        a2dp_event_data_t* data = (a2dp_event_data_t*)p_data;
        bts_a2dp_sink_packet_recieve(data->packet);
        break;
    }
#endif
    case STREAM_SUSPEND_REQ: {
        SERVICE_BT_STATUS status;

        if (a2dp_sm->delay_suspend_timer) {
            BT_LOGD("need stop suspend timer");
            stop_timer(a2dp_sm->delay_suspend_timer);
            a2dp_sm->delay_suspend_timer = NULL;
        }

        /* if device had already send suspend request, ignore it */
        if (flag_isset(a2dp_sm, PENDING_STOP)) {
            BT_LOGD("had already send suspend request, ignore it");
            break;
        }
        flag_set(a2dp_sm, PENDING_STOP);
        status = service_adapter_a2dp_source_suspend_stream(a2dp_sm->addr);
        if (status != SERVICE_BT_STATUS_SUCCESS) {
            BT_LOGE("Stream suspend failed");
        }
        bts_a2dp_audio_on_stopped(a2dp_sm->peer_sep);
        break;
    }

    case DISCONNECTED_EVT:
        //check active, if active should nofify ffmpeg to stop
        bts_a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, false);
        hsm_transition_to(sm, &idle_state);
        break;

    case STREAM_SUSPENDED_EVT:
        //If remote suspend, notify ffmpeg to
        // suspend/stop stream.
        a2dp_sm->pending = PENDING_NONE;
        bts_a2dp_audio_on_suspended(a2dp_sm->peer_sep);
        bts_a2dp_report_audio_state(a2dp_sm, a2dp_sm->addr,
            A2DP_AUDIO_STATE_STOPPED);
        hsm_transition_to(sm, &opened_state);
        break;

    case STREAM_CLOSED_EVT:
        a2dp_sm->pending = PENDING_NONE;
        bts_a2dp_audio_on_stopped(a2dp_sm->peer_sep);
        bts_a2dp_report_audio_state(a2dp_sm, a2dp_sm->addr,
            A2DP_AUDIO_STATE_STOPPED);
        hsm_transition_to(sm, &opened_state);
        break;

    case DEVICE_CODEC_STATE_CHANGE_EVT:
        bts_a2dp_report_audio_config_state(a2dp_sm, a2dp_sm->addr);
        break;

    default:
        break;
    }

    return true;
}

static void closing_enter(state_machine_t* sm)
{
    a2dp_state_machine_t* a2dp_sm = (a2dp_state_machine_t*)sm;

    BT_LOGD("state=%s Enter, peer=%s", hsm_get_current_state_name(sm),
        addr_str(a2dp_sm->addr));
    bts_a2dp_report_connection_state(a2dp_sm, a2dp_sm->addr,
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
    BT_LOGD("state=%s, event=%s peer=%s", hsm_get_current_state_name(sm),
        stack_event_to_string(event),
        addr_str(a2dp_sm->addr));
    switch (event) {
    case STREAM_SUSPEND_REQ:
    case STREAM_CLOSED_EVT:
    case STREAM_SUSPENDED_EVT:
        bts_a2dp_audio_on_stopped(a2dp_sm->peer_sep);
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

a2dp_state_machine_t* a2dp_state_machine_new(void* context, uint8_t peer_sep, bt_address bd_addr)
{
    a2dp_state_machine_t* a2dp_sm;

    a2dp_sm = (a2dp_state_machine_t*)malloc(sizeof(a2dp_state_machine_t));
    if (!a2dp_sm)
        return NULL;

    memset(a2dp_sm, 0, sizeof(a2dp_state_machine_t));
    a2dp_sm->service = context;
    a2dp_sm->peer_sep = peer_sep;
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

const char* a2dp_state_machine_current_state(a2dp_state_machine_t* sm)
{
    return hsm_get_current_state_name(&sm->sm);
}

bool a2dp_state_machine_is_pending_stop(a2dp_state_machine_t* sm)
{
    if (flag_isset(sm, PENDING_STOP) || sm->delay_suspend_timer)
        return true;

    return false;
}
