/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sal_a2dp_sink_interface.h"
#include "sal_a2dp_source_interface.h"
#include "sal_adapter_interface.h"
#include "sal_avrcp_control_interface.h"
#include "sal_avrcp_target_interface.h"

#include "a2dp_audio.h"
#include "a2dp_event.h"
#include "a2dp_sink.h"
#include "a2dp_source.h"
#include "a2dp_state_machine.h"
#include "bt_utils.h"
#include "state_machine.h"

#include "bt_avrcp.h"

#include "service_loop.h"

#define LOG_TAG "a2dp_stm"
#include "utils/log.h"

#ifndef CONFIG_BLUETOOTH_A2DP_CONNECT_TIMEOUT
#define A2DP_CONNECT_TIMEOUT 6000
#else
#define A2DP_CONNECT_TIMEOUT (CONFIG_BLUETOOTH_A2DP_CONNECT_TIMEOUT * 1000)
#endif
#define A2DP_START_TIMEOUT   5000
#define A2DP_SUSPEND_TIMEOUT 5000
#define A2DP_DELAY_START     100
#define A2DP_DELAY_SUSPEND   200

typedef enum pending_state {
    PENDING_NONE = 0x0,
    PENDING_START = 0X02,
    PENDING_STOP = 0x04,
} pending_state_t;

typedef struct _a2dp_state_machine {
    state_machine_t sm;
    void *service;
    bt_address_t addr;
    pending_state_t pending;
    bool audio_ready;
    uint8_t peer_sep;
    service_timer_t *connect_timer;
    service_timer_t *start_timer;
    service_timer_t *delay_start_timer;
    service_timer_t *delay_suspend_timer;
} a2dp_state_machine_t;

typedef struct {
    a2dp_state_machine_t *a2dp_sm;
    a2dp_event_t *a2dp_event;
} a2dp_inter_event_t;

static void idle_enter(state_machine_t *sm);
static void idle_exit(state_machine_t *sm);
static void opening_enter(state_machine_t *sm);
static void opening_exit(state_machine_t *sm);
static void opened_enter(state_machine_t *sm);
static void opened_exit(state_machine_t *sm);
static void started_enter(state_machine_t *sm);
static void started_exit(state_machine_t *sm);
static void closing_enter(state_machine_t *sm);
static void closing_exit(state_machine_t *sm);

static bool idle_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool opening_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool opened_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool started_process_event(state_machine_t *sm, uint32_t event, void *p_data);
static bool closing_process_event(state_machine_t *sm, uint32_t event, void *p_data);

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

#define A2DP_STM_DEBUG 1
#if A2DP_STM_DEBUG
static char *stack_event_to_string(a2dp_event_type_t event);

#define A2DP_TRANS_DBG(_sm, _addr, _action)                                                     \
    do {                                                                                        \
        char __addr_str[BT_ADDR_STR_LENGTH] = { 0 };                                            \
        bt_addr_ba2str(_addr, __addr_str);                                                      \
        BT_LOGD("%s State=%s, Peer=[%s]", _action, hsm_get_current_state_name(sm), __addr_str); \
    } while (0);

#define A2DP_DBG_ENTER(__sm, __addr) A2DP_TRANS_DBG(__sm, __addr, "Enter")
#define A2DP_DBG_EXIT(__sm, __addr)  A2DP_TRANS_DBG(__sm, __addr, "Exit ")
#define A2DP_DBG_EVENT(__sm, __addr, __event)                                                      \
    do {                                                                                           \
        char __addr_str[BT_ADDR_STR_LENGTH] = { 0 };                                               \
        bt_addr_ba2str(__addr, __addr_str);                                                        \
        if (__event != DATA_IND_EVT)                                                               \
            BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(sm), \
                    __addr_str, stack_event_to_string(event));                                     \
    } while (0);
#else
#define A2DP_DBG_ENTER(__sm, __addr)
#define A2DP_DBG_EXIT(__sm, __addr)
#define A2DP_DBG_EVENT(__sm, __addr, __event)
#endif

#if A2DP_STM_DEBUG
static char *stack_event_to_string(a2dp_event_type_t event)
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
#endif

static void a2dp_report_connection_state(a2dp_state_machine_t *stm, bt_address_t *addr, a2dp_connection_state_t state)
{
    BT_LOGD("%s, addr:%s, state: %d", __func__, bt_addr_str(addr), state);
    if (state == A2DP_CONNECTION_STATE_CONNECTED)
        BT_LOGD("PERFORMANCE-A2DP-BTM-CONNECTED");

    if (stm->peer_sep == SEP_SRC) {
        a2dp_sink_service_notify_connection_state_changed(addr, state);
    } else {
        a2dp_source_service_notify_connection_state_changed(addr, state);
    }
}

static void a2dp_report_audio_state(a2dp_state_machine_t *stm, bt_address_t *addr, a2dp_audio_state_t state)
{
    BT_LOGD("%s, addr:%s, state: %d", __func__, bt_addr_str(addr), state);

    if (stm->peer_sep == SEP_SRC) {
        a2dp_sink_service_notify_audio_state_changed(addr, state);
    } else {
        a2dp_source_service_notify_audio_state_changed(addr, state);
    }
}

static void a2dp_report_audio_config_state(a2dp_state_machine_t *stm, bt_address_t *addr)
{
    BT_LOGD("%s, addr:%s", __func__, bt_addr_str(addr));

    if (stm->peer_sep == SEP_SRC) {
        a2dp_sink_service_notify_audio_sink_config_changed(addr);
    } else {
        a2dp_source_service_notify_audio_source_config_changed(addr);
    }
}

static void a2dp_connect_timeout_callback(service_timer_t *timer, void *data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)data;
    a2dp_event_t *a2dp_event;

    a2dp_event = a2dp_event_new(CONNECT_TIMEOUT, &a2dp_sm->addr);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void a2dp_start_timeout_callback(service_timer_t *timer, void *data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)data;
    a2dp_event_t *a2dp_event;

    a2dp_event = a2dp_event_new(START_TIMEOUT, &a2dp_sm->addr);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void a2dp_delay_start_timeout_callback(service_timer_t *timer, void *data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)data;
    a2dp_event_t *a2dp_event;

    a2dp_event = a2dp_event_new(DELAY_STREAM_START_REQ, &a2dp_sm->addr);
    a2dp_state_machine_handle_event(a2dp_sm, a2dp_event);
    a2dp_event_destory(a2dp_event);
}

static void a2dp_delay_suspend_timeout_callback(service_timer_t *timer, void *data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)data;
    a2dp_event_t *a2dp_event;

    a2dp_event = a2dp_event_new(STREAM_SUSPEND_REQ, &a2dp_sm->addr);
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

static void idle_enter(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;
    const state_t *prev_state = hsm_get_previous_state(sm);

    A2DP_DBG_ENTER(sm, &a2dp_sm->addr);

    a2dp_sm->audio_ready = false;
    if (prev_state != NULL) {
        a2dp_report_connection_state(a2dp_sm, &a2dp_sm->addr,
                                     A2DP_CONNECTION_STATE_DISCONNECTED);
    }
}

static void idle_exit(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EXIT(sm, &a2dp_sm->addr);
}

static bool idle_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;
    a2dp_event_data_t *data = (a2dp_event_data_t *)p_data;

    A2DP_DBG_EVENT(sm, &a2dp_sm->addr, event);
    switch (event) {
    case CONNECT_REQ: {
        bt_status_t status;
        BT_LOGD("PERFORMANCE-A2DP-SRC-BLUELET-CONNECT-START");
        if (a2dp_sm->peer_sep == SEP_SNK)
            status = bt_sal_a2dp_source_connect(&data->bd_addr);
        else
            status = bt_sal_a2dp_sink_connect(&data->bd_addr);
        if (status != BT_STATUS_SUCCESS) {
            a2dp_report_connection_state(a2dp_sm, &a2dp_sm->addr,
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
            bt_status_t status;
            status = bt_sal_a2dp_source_connect(&data->bd_addr);
            if (status != BT_STATUS_SUCCESS) {
                a2dp_report_connection_state(a2dp_sm, &a2dp_sm->addr,
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

static void opening_enter(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_ENTER(sm, &a2dp_sm->addr);
    a2dp_sm->connect_timer = service_loop_timer(A2DP_CONNECT_TIMEOUT, 0, a2dp_connect_timeout_callback, a2dp_sm);
    a2dp_report_connection_state(a2dp_sm, &a2dp_sm->addr, A2DP_CONNECTION_STATE_CONNECTING);
}

static void opening_exit(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EXIT(sm, &a2dp_sm->addr);
}

static bool opening_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;
    a2dp_event_data_t *data = (a2dp_event_data_t *)p_data;
    bt_status_t status;

    A2DP_DBG_EVENT(sm, &a2dp_sm->addr, event);
    switch (event) {
    case DISCONNECT_REQ: {
        if (a2dp_sm->peer_sep == SEP_SNK)
            status = bt_sal_a2dp_source_disconnect(&data->bd_addr);
        else
            status = bt_sal_a2dp_sink_disconnect(&data->bd_addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect failed");
        }
        hsm_transition_to(sm, &idle_state);
        break;
    }

    case CONNECTED_EVT:
        if (a2dp_sm->connect_timer) {
            service_loop_cancel_timer(a2dp_sm->connect_timer);
            a2dp_sm->connect_timer = NULL;
        }
        hsm_transition_to(sm, &opened_state);
        break;

    case DISCONNECTED_EVT:
    case CONNECT_TIMEOUT:
        service_loop_cancel_timer(a2dp_sm->connect_timer);
        a2dp_sm->connect_timer = NULL;
        hsm_transition_to(sm, &idle_state);
        break;
    default:
        break;
    }

    return true;
}

static void opened_enter(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;
    const state_t *prev_state = hsm_get_previous_state(sm);

    A2DP_DBG_ENTER(sm, &a2dp_sm->addr);
    if (prev_state == &idle_state || prev_state == &opening_state) {
        /* if we are accept link as a2dp src, change the av link role to master */
        if (a2dp_sm->peer_sep == SEP_SNK)
            bt_sal_set_link_role(&a2dp_sm->addr, BT_LINK_ROLE_MASTER);
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
        if (a2dp_sm->peer_sep == SEP_SRC)
            bt_sal_avrcp_control_connect(&a2dp_sm->addr);
#endif
        a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, true);
        a2dp_report_connection_state(a2dp_sm, &a2dp_sm->addr,
                                     A2DP_CONNECTION_STATE_CONNECTED);
    }
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    else if (prev_state == &started_state) {
        bt_sal_avrcp_target_play_status_notify(&a2dp_sm->addr, PLAY_STATUS_PAUSED);
    }
#endif
}

static void opened_exit(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EXIT(sm, &a2dp_sm->addr);
}

static bool opened_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EVENT(sm, &a2dp_sm->addr, event);
    switch (event) {
    case DISCONNECT_REQ: {
        bt_status_t status;

        if (a2dp_sm->peer_sep == SEP_SNK)
            status = bt_sal_a2dp_source_disconnect(&a2dp_sm->addr);
        else
            status = bt_sal_a2dp_sink_disconnect(&a2dp_sm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("A2dp disconnect failed");
        }
        status = bt_sal_avrcp_control_disconnect(&a2dp_sm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Avrc disconnect failed");
        }
        a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, false);
        hsm_transition_to(sm, &closing_state);
        break;
    }
    case STREAM_START_REQ: {
        bt_status_t status;

        /* if we are in suspending substate, ignore this request */
        if (flag_isset(a2dp_sm, PENDING_STOP) || flag_isset(a2dp_sm, PENDING_START)) {
            BT_LOGD("in suspending or starting substate, ignore this request");
            break;
        }
        if (!a2dp_sm->audio_ready) {
            BT_LOGE("A2DP Audio is not ready, Ignore start cmd");
            break;
        }
        status = bt_sal_a2dp_source_start_stream(&a2dp_sm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Stream start failed");
            break;
        }
        flag_set(a2dp_sm, PENDING_START);
        a2dp_sm->start_timer = service_loop_timer(A2DP_START_TIMEOUT, 0, a2dp_start_timeout_callback, a2dp_sm);
        break;
    }
    case DELAY_STREAM_START_REQ: {
        bt_status_t status;

        if (a2dp_sm->delay_start_timer)
            service_loop_cancel_timer(a2dp_sm->delay_start_timer);
        a2dp_sm->delay_start_timer = NULL;
        if (flag_isset(a2dp_sm, PENDING_START))
            break;
        status = bt_sal_a2dp_source_start_stream(&a2dp_sm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Stream delay start failed");
            break;
        }
        flag_set(a2dp_sm, PENDING_START);
        a2dp_sm->start_timer = service_loop_timer(A2DP_START_TIMEOUT, 0, a2dp_start_timeout_callback, a2dp_sm);
        break;
    }
    case DISCONNECTED_EVT:
        if (flag_isset(a2dp_sm, PENDING_START)) {
            flag_clear(a2dp_sm, PENDING_START);
            service_loop_cancel_timer(a2dp_sm->start_timer);
            a2dp_sm->start_timer = NULL;
            /* When pending on start request, then received stream close event
               call a2dp_audio_on_started(), shoule ack start failure; */
            a2dp_audio_on_started(a2dp_sm->peer_sep, false);
        }
        a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, false);
        hsm_transition_to(sm, &idle_state);
        break;

    case STREAM_STARTED_EVT:
        if (a2dp_sm->peer_sep == SEP_SNK) {
            /* If remote tries to start A2DP when DUT is A2DP Source, then Suspend.
             If A2DP is Sink and call is active, then disconnect the AVDTP channel. */
            flag_clear(a2dp_sm, PENDING_START);
            service_loop_cancel_timer(a2dp_sm->start_timer);
            a2dp_sm->start_timer = NULL;
            if (a2dp_sm->delay_start_timer)
                service_loop_cancel_timer(a2dp_sm->delay_start_timer);
            a2dp_sm->delay_start_timer = NULL;
        }

        if (!a2dp_sm->audio_ready) {
            BT_LOGW("A2dp device is not ready: %s", stack_event_to_string(event));
            break;
        }

        a2dp_audio_on_started(a2dp_sm->peer_sep, true);
        hsm_transition_to(sm, &started_state);
        break;

    case STREAM_SUSPENDED_EVT:
    case STREAM_CLOSED_EVT:
        if (flag_isset(a2dp_sm, PENDING_STOP) && a2dp_sm->delay_start_timer) {
            service_loop_cancel_timer(a2dp_sm->delay_start_timer);
            a2dp_sm->delay_start_timer = service_loop_timer(A2DP_DELAY_START, 0, a2dp_delay_start_timeout_callback, a2dp_sm);
        }
        flag_clear(a2dp_sm, PENDING_STOP);
        a2dp_report_audio_state(a2dp_sm, &a2dp_sm->addr,
                                A2DP_AUDIO_STATE_STOPPED);
        a2dp_audio_on_stopped(a2dp_sm->peer_sep);
        break;

    case DEVICE_CODEC_STATE_CHANGE_EVT:
        a2dp_sm->audio_ready = true;
        a2dp_report_audio_config_state(a2dp_sm, &a2dp_sm->addr);
        a2dp_audio_setup_codec(a2dp_sm->peer_sep, &a2dp_sm->addr);
        break;

    case START_TIMEOUT: {
        flag_clear(a2dp_sm, PENDING_START);
        service_loop_cancel_timer(a2dp_sm->start_timer);
        a2dp_sm->start_timer = NULL;
        a2dp_audio_on_started(a2dp_sm->peer_sep, false);
        break;
    }

    default:
        break;
    }

    return true;
}

static void started_enter(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_ENTER(sm, &a2dp_sm->addr);
    a2dp_report_audio_state(a2dp_sm, &a2dp_sm->addr,
                            A2DP_AUDIO_STATE_STARTED);
}

static void started_exit(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EXIT(sm, &a2dp_sm->addr);
}

static bool started_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EVENT(sm, &a2dp_sm->addr, event);
    switch (event) {
    case DISCONNECT_REQ: {
        bt_status_t status;
        if (a2dp_sm->peer_sep == SEP_SNK)
            status = bt_sal_a2dp_source_disconnect(&a2dp_sm->addr);
        else
            status = bt_sal_a2dp_sink_disconnect(&a2dp_sm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Disconnect failed");
        }
        status = bt_sal_avrcp_control_disconnect(&a2dp_sm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Avrc disconnect failed");
        }
        hsm_transition_to(sm, &closing_state);
        break;
    }

    case STREAM_SUSPEND_DELAY:
        if (!a2dp_sm->delay_suspend_timer)
            a2dp_sm->delay_suspend_timer = service_loop_timer(A2DP_DELAY_SUSPEND, 0, a2dp_delay_suspend_timeout_callback, a2dp_sm);
        break;

    case STREAM_START_REQ:
        if (a2dp_sm->delay_suspend_timer) {
            BT_LOGD("need stop suspend timer");
            service_loop_cancel_timer(a2dp_sm->delay_suspend_timer);
            a2dp_sm->delay_suspend_timer = NULL;
            a2dp_audio_on_started(a2dp_sm->peer_sep, true);
            break;
        }
        /* received start request when we are in pending a2dp stream
           suspend sub-state, we need restart stream and transmit state to
           opened state, and wait for started event */
        if (flag_isset(a2dp_sm, PENDING_STOP)) {
            a2dp_sm->delay_start_timer = service_loop_timer(A2DP_SUSPEND_TIMEOUT, 0, a2dp_delay_start_timeout_callback, a2dp_sm);
            hsm_transition_to(sm, &opened_state);
            break;
        }
        // We were started remotely, just ACK back the local request
        if (a2dp_sm->peer_sep == SEP_SNK)
            a2dp_audio_on_started(a2dp_sm->peer_sep, true);
        break;

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    case DATA_IND_EVT: {
        a2dp_event_data_t *data = (a2dp_event_data_t *)p_data;
        a2dp_sink_packet_recieve(data->packet);
        break;
    }
#endif
    case STREAM_SUSPEND_REQ: {
        bt_status_t status;

        if (a2dp_sm->delay_suspend_timer) {
            BT_LOGD("need stop suspend timer");
            service_loop_cancel_timer(a2dp_sm->delay_suspend_timer);
            a2dp_sm->delay_suspend_timer = NULL;
        }

        /* if device had already send suspend request, ignore it */
        if (flag_isset(a2dp_sm, PENDING_STOP)) {
            BT_LOGD("had already send suspend request, ignore it");
            break;
        }
        flag_set(a2dp_sm, PENDING_STOP);
        status = bt_sal_a2dp_source_suspend_stream(&a2dp_sm->addr);
        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("Stream suspend failed");
        }
        a2dp_audio_on_stopped(a2dp_sm->peer_sep);
        break;
    }

    case DISCONNECTED_EVT:
        // check active, if active should nofify ffmpeg to stop
        a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, false);
        hsm_transition_to(sm, &idle_state);
        break;

    case STREAM_SUSPENDED_EVT:
        // If remote suspend, notify ffmpeg to
        //  suspend/stop stream.
        a2dp_sm->pending = PENDING_NONE;
        a2dp_audio_on_suspended(a2dp_sm->peer_sep);
        a2dp_report_audio_state(a2dp_sm, &a2dp_sm->addr,
                                A2DP_AUDIO_STATE_STOPPED);
        hsm_transition_to(sm, &opened_state);
        break;

    case STREAM_CLOSED_EVT:
        a2dp_sm->pending = PENDING_NONE;
        a2dp_audio_on_stopped(a2dp_sm->peer_sep);
        a2dp_report_audio_state(a2dp_sm, &a2dp_sm->addr,
                                A2DP_AUDIO_STATE_STOPPED);
        hsm_transition_to(sm, &opened_state);
        break;

    case DEVICE_CODEC_STATE_CHANGE_EVT:
        a2dp_report_audio_config_state(a2dp_sm, &a2dp_sm->addr);
        break;

    default:
        break;
    }

    return true;
}

static void closing_enter(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_ENTER(sm, &a2dp_sm->addr);
    a2dp_audio_on_connection_changed(a2dp_sm->peer_sep, false);
    a2dp_report_connection_state(a2dp_sm, &a2dp_sm->addr,
                                 A2DP_CONNECTION_STATE_DISCONNECTING);
}

static void closing_exit(state_machine_t *sm)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EXIT(sm, &a2dp_sm->addr);
}

static bool closing_process_event(state_machine_t *sm, uint32_t event, void *p_data)
{
    a2dp_state_machine_t *a2dp_sm = (a2dp_state_machine_t *)sm;

    A2DP_DBG_EVENT(sm, &a2dp_sm->addr, event);
    switch (event) {
    case STREAM_SUSPEND_REQ:
    case STREAM_CLOSED_EVT:
    case STREAM_SUSPENDED_EVT:
        a2dp_audio_on_stopped(a2dp_sm->peer_sep);
        break;

    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &idle_state);
        break;

    default:
        break;
    }

    return true;
}

static void a2dp_state_machine_event_dispatch(a2dp_state_machine_t *a2dp_sm, a2dp_event_t *a2dp_event)
{
    if (!a2dp_event || !a2dp_sm)
        return;

    hsm_dispatch_event(&a2dp_sm->sm, a2dp_event->event, &a2dp_event->event_data);
}

a2dp_state_machine_t *a2dp_state_machine_new(void *context, uint8_t peer_sep, bt_address_t *bd_addr)
{
    a2dp_state_machine_t *a2dp_sm;

    a2dp_sm = (a2dp_state_machine_t *)malloc(sizeof(a2dp_state_machine_t));
    if (!a2dp_sm)
        return NULL;

    memset(a2dp_sm, 0, sizeof(a2dp_state_machine_t));
    a2dp_sm->service = context;
    a2dp_sm->peer_sep = peer_sep;
    hsm_ctor(&a2dp_sm->sm, (state_t *)&idle_state);
    memcpy(&a2dp_sm->addr, bd_addr, sizeof(bt_address_t));

    return a2dp_sm;
}

void a2dp_state_machine_destory(a2dp_state_machine_t *a2dp_sm)
{
    if (!a2dp_sm)
        return;

    hsm_dtor(&a2dp_sm->sm);
    free((void *)a2dp_sm);
}

void a2dp_state_machine_handle_event(a2dp_state_machine_t *sm,
                                     a2dp_event_t *a2dp_event)
{
    a2dp_state_machine_event_dispatch(sm, a2dp_event);
}

a2dp_state_t a2dp_state_machine_get_state(a2dp_state_machine_t *sm)
{
    const state_t *cur_state = hsm_get_current_state(&sm->sm);
    a2dp_state_t state;
    if (cur_state == (state_t *)&idle_state) {
        state = A2DP_STATE_IDLE;
    } else if (cur_state == (state_t *)&opening_state) {
        state = A2DP_STATE_OPENING;
    } else if (cur_state == (state_t *)&opened_state) {
        state = A2DP_STATE_OPENED;
    } else if (cur_state == (state_t *)&started_state) {
        state = A2DP_STATE_STARTED;
    } else if (cur_state == (state_t *)&closing_state) {
        state = A2DP_STATE_CLOSING;
    } else {
        state = A2DP_STATE_IDLE;
    }

    return state;
}

const char *a2dp_state_machine_current_state(a2dp_state_machine_t *sm)
{
    return hsm_get_current_state_name(&sm->sm);
}

bool a2dp_state_machine_is_pending_stop(a2dp_state_machine_t *sm)
{
    if (flag_isset(sm, PENDING_STOP) || sm->delay_suspend_timer)
        return true;

    return false;
}
