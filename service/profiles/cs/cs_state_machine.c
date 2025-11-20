/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#define LOG_TAG "cs_stm"
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_cs.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "callbacks_list.h"
#include "cs_msg.h"
#include "cs_service.h"
#include "cs_state_machine.h"
#include "gatts_service.h"
#include "power_manager.h"
#include "sal_le_cs_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "state_machine.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LE_CS

static char* stack_event_to_string(cs_msg_id_t msg_id);

#define CS_TRANS_DBG(_sm, _addr, _action)                                                       \
    do {                                                                                        \
        char __addr_str[BT_ADDR_STR_LENGTH] = { 0 };                                            \
        bt_addr_ba2str(_addr, __addr_str);                                                      \
        BT_LOGD("%s State=%s, Peer=[%s]", _action, hsm_get_current_state_name(sm), __addr_str); \
    } while (0);

#define CS_DBG_ENTER(__sm, __addr) CS_TRANS_DBG(__sm, __addr, "Enter")
#define CS_DBG_EXIT(__sm, __addr) CS_TRANS_DBG(__sm, __addr, "Exit ")
#define CS_DBG_EVENT(__sm, __addr, __event)                                                      \
    do {                                                                                         \
        char __addr_str[BT_ADDR_STR_LENGTH] = { 0 };                                             \
        bt_addr_ba2str(__addr, __addr_str);                                                      \
        BT_LOGD("ProcessEvent, State=%s, Peer=[%s], Event=%s", hsm_get_current_state_name(__sm), \
            __addr_str, stack_event_to_string(__event));                                         \
    } while (0);

static char* stack_event_to_string(cs_msg_id_t msg_id)
{
    switch (msg_id) {
        CASE_RETURN_STR(CS_STARTUP)
        CASE_RETURN_STR(CS_SHUTDOWN)
        CASE_RETURN_STR(START_REQ)
        CASE_RETURN_STR(STOP_REQ)
        CASE_RETURN_STR(CAPBLITIES_RECEIVED_EVT)
        CASE_RETURN_STR(DISCONNECTED_EVT)
        CASE_RETURN_STR(CONNECTED_EVT)
        CASE_RETURN_STR(CONFIG_DONE_EVT)
        CASE_RETURN_STR(SECURITY_DONE_EVT)
    default:
        return "UNKNOWN_EVENT";
    }
}
typedef struct _cs_state_machine {
    state_machine_t sm;
    void* service;
    bt_address_t addr;
    bool is_capbilities_exchanged;
    bt_distance_measurement_params_t params;
    bool started;
    service_timer_t* start_timer;
    bt_le_srv_cs_set_default_settings_param_t* default_settings;
    bool is_start_req;
} cs_state_machine_t;

#define CS_START_TIMEOUT (5 * 1000)

static void disconnected_enter(state_machine_t* sm);
static void disconnected_exit(state_machine_t* sm);
// static void init_enter(state_machine_t* sm);
// static void init_exit(state_machine_t* sm);
static void connected_enter(state_machine_t* sm);
static void connected_exit(state_machine_t* sm);
static void wait_for_config_complete_enter(state_machine_t* sm);
static void wait_for_config_complete_exit(state_machine_t* sm);
static void wait_for_security_complete_enter(state_machine_t* sm);
static void wait_for_security_complete_exit(state_machine_t* sm);
static void wait_for_procedure_complete_enter(state_machine_t* sm);
static void wait_for_procedure_complete_exit(state_machine_t* sm);
static void started_enter(state_machine_t* sm);
static void started_exit(state_machine_t* sm);

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
// static bool init_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool wait_for_config_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool wait_for_security_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool wait_for_procedure_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool started_process_event(state_machine_t* sm, uint32_t event, void* p_data);

static const state_t disconnected_state = {
    .state_name = "Disconnected",
    .state_value = CS_STATE_DISCONNECTED,
    .enter = disconnected_enter,
    .exit = disconnected_exit,
    .process_event = disconnected_process_event,
};

static const state_t connected_state = {
    .state_name = "Connected",
    .state_value = CS_STATE_CONNECTED,
    .enter = connected_enter,
    .exit = connected_exit,
    .process_event = connected_process_event,
};

static const state_t wait_for_config_complete_state = {
    .state_name = "Wait_for_config_complete",
    .state_value = CS_STATE_WAIT_FOR_CONFIG_COMPLETE,
    .enter = wait_for_config_complete_enter,
    .exit = wait_for_config_complete_exit,
    .process_event = wait_for_config_complete_process_event,
};

static const state_t wait_for_security_complete_state = {
    .state_name = "Wait_for_security_complete",
    .state_value = CS_STATE_WAIT_FOR_SECURITY_COMPLETE,
    .enter = wait_for_security_complete_enter,
    .exit = wait_for_security_complete_exit,
    .process_event = wait_for_security_complete_process_event,
};

static const state_t wait_for_procedure_complete_state = {
    .state_name = "Wait_for_procedure_complete",
    .state_value = CS_STATE_WAIT_FOR_PROCEDURE_COMPLETE,
    .enter = wait_for_procedure_complete_enter,
    .exit = wait_for_procedure_complete_exit,
    .process_event = wait_for_procedure_complete_process_event,
};

static const state_t started_state = {
    .state_name = "Start",
    .state_value = CS_STATE_STARTED,
    .enter = started_enter,
    .exit = started_exit,
    .process_event = started_process_event,
};

static void disconnected_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    const state_t* prev_state = hsm_get_previous_state(sm);

    CS_DBG_ENTER(sm, &cs_sm->addr);
    if (prev_state != NULL) {
        if (cs_sm->started) {
            cs_service_notify_stopped_cb(&cs_sm->addr, 0, cs_sm->params.method);
            cs_sm->started = false;
        }

        if (cs_sm->start_timer != NULL) {
            service_loop_cancel_timer(cs_sm->start_timer);
            cs_sm->start_timer = NULL;
        }

        cs_sm->is_capbilities_exchanged = false;
        memset(&cs_sm->params, 0, sizeof(cs_sm->params));
        if (cs_sm->default_settings) {
            free(cs_sm->default_settings);
            cs_sm->default_settings = NULL;
        }

        cs_sm->is_start_req = false;
    }
}

static void disconnected_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool disconnected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // cs_msg_t* data = (cs_msg_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case START_REQ:
        BT_LOGE("cs has not connected");
        break;
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    default:
        break;
    }

    return true;
}

static void connected_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    const state_t* prev_state = hsm_get_previous_state(sm);

    CS_DBG_ENTER(sm, &cs_sm->addr);
    if (prev_state != NULL && prev_state != &disconnected_state) {
        cs_service_notify_stopped_cb(&cs_sm->addr, 0, cs_sm->params.method);
        cs_sm->started = false;
        cs_sm->is_start_req = false;
    }
}

static void connected_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static void cs_start_timeout(service_timer_t* timer, void* data)
{
    state_machine_t* cs_sm = (state_machine_t*)data;

    hsm_transition_to(cs_sm, &connected_state);
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    cs_msg_data_t* data = (cs_msg_data_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case START_REQ:
        bt_distance_measurement_params_t* params = data->data;

        cs_sm->is_start_req = true;
        if (!cs_sm->default_settings) {
            cs_sm->default_settings = (bt_le_srv_cs_set_default_settings_param_t*)malloc(sizeof(bt_le_srv_cs_set_default_settings_param_t));
        }

        cs_sm->default_settings->enable_initiator_role = params->role == CS_BT_SRV_CONN_LE_CS_ROLE_INITIATOR ? true : false;
        cs_sm->default_settings->enable_reflector_role = params->role == CS_BT_SRV_CONN_LE_CS_ROLE_REFLECTOR ? true : false;
        cs_sm->default_settings->cs_sync_antenna_selection = params->antenna_paths_mask;
        cs_sm->params = *params;

        bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &(data->bd_addr), cs_sm->default_settings);

        if (cs_sm->is_capbilities_exchanged) {
            hsm_transition_to(sm, &wait_for_config_complete_state);
        } else {
            bt_sal_cs_read_remote_supported_capabilities(PRIMARY_ADAPTER, &(data->bd_addr));
        }

        cs_sm->start_timer = service_loop_timer_no_repeating(CS_START_TIMEOUT, cs_start_timeout, cs_sm);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &disconnected_state);
        break;
    case CAPBLITIES_RECEIVED_EVT:
        cs_sm->is_capbilities_exchanged = true;
        if (cs_sm->default_settings) {
            bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &(data->bd_addr), cs_sm->default_settings);
        }

        if (cs_sm->is_start_req) {
            bt_le_srv_cs_create_config_params_t config_params;
            config_params.id = 0;
            config_params.main_mode_type = cs_sm->params.mainMode;
            config_params.sub_mode_type = cs_sm->params.submode;
            config_params.min_main_mode_steps = cs_sm->params.min_steps;
            config_params.max_main_mode_steps = cs_sm->params.max_steps;
            config_params.main_mode_repetition = cs_sm->params.repetition;
            config_params.mode_0_steps = cs_sm->params.mode0_steps;
            config_params.role = cs_sm->params.role;
            config_params.rtt_type = cs_sm->params.rtt_type;
            config_params.cs_sync_phy = cs_sm->params.sync_phy;
            config_params.channel_map_repetition = 0;
            config_params.channel_selection_type = cs_sm->params.channelSelectionType;
            config_params.ch3c_shape = cs_sm->params.ch3cShape;
            config_params.ch3c_jump = cs_sm->params.ch3cJump;

            bt_sal_cs_create_config(PRIMARY_ADAPTER, &(data->bd_addr), &config_params, BT_LE_SRV_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE);
        }

        hsm_transition_to(sm, &wait_for_config_complete_state);
        break;
    default:
        break;
    }

    return true;
}

static void wait_for_config_complete_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // const state_t* prev_state = hsm_get_previous_state(sm);

    if (cs_sm->is_start_req) {
    }

    CS_DBG_ENTER(sm, &cs_sm->addr);
}

static void wait_for_config_complete_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool wait_for_config_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // cs_msg_data_t* data = (cs_msg_data_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &disconnected_state);
        break;
    case CONFIG_DONE_EVT:
        if (cs_sm->is_start_req) {
            bt_sal_cs_security_enable(PRIMARY_ADAPTER, &(cs_sm->addr));
        }

        hsm_transition_to(sm, &wait_for_security_complete_state);
        break;
    default:
        break;
    }

    return true;
}

static void wait_for_security_complete_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // const state_t* prev_state = hsm_get_previous_state(sm);

    CS_DBG_ENTER(sm, &cs_sm->addr);
}

static void wait_for_security_complete_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool wait_for_security_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // cs_msg_t* data = (cs_msg_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &disconnected_state);
        break;
    case CONFIG_DONE_EVT:
        hsm_transition_to(sm, &wait_for_security_complete_state);
        break;
    case SECURITY_DONE_EVT:
        if (cs_sm->is_start_req) {
            // TODO: get procedure parameters from user, we can not set procedure parameters now in reflector role.
            bt_sal_cs_set_procedure_parameters(PRIMARY_ADAPTER, &(cs_sm->addr), NULL);

            bt_le_srv_cs_procedure_enable_param_t params;

            params.config_id = 0;
            params.enable = CS_BT_SRV_CONN_LE_CS_PROCEDURES_ENABLED;
            bt_sal_cs_procedure_enable(PRIMARY_ADAPTER, &(cs_sm->addr), &params);
        }

        hsm_transition_to(sm, &wait_for_procedure_complete_state);
        break;
    default:
        break;
    }

    return true;
}

static void wait_for_procedure_complete_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // const state_t* prev_state = hsm_get_previous_state(sm);

    CS_DBG_ENTER(sm, &cs_sm->addr);
}

static void wait_for_procedure_complete_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool wait_for_procedure_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // cs_msg_t* data = (cs_msg_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &disconnected_state);
        break;
    case CONFIG_DONE_EVT:
        hsm_transition_to(sm, &wait_for_security_complete_state);
        break;
    case SECURITY_DONE_EVT:
        hsm_transition_to(sm, &wait_for_procedure_complete_state);
        break;
    case PROCEDURE_DONE_EVT:
        hsm_transition_to(sm, &started_state);
        break;
    default:
        break;
    }

    return true;
}

static void started_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    const state_t* prev_state = hsm_get_previous_state(sm);

    CS_DBG_ENTER(sm, &cs_sm->addr);
    if (prev_state != NULL) {
        cs_service_notify_started_cb(&cs_sm->addr, cs_sm->params.method);
        cs_sm->started = true;
    }

    if (cs_sm->start_timer != NULL) {
        service_loop_cancel_timer(cs_sm->start_timer);
        cs_sm->start_timer = NULL;
    }

    cs_sm->is_start_req = false;
}

static void started_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool started_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    // cs_msg_t* data = (cs_msg_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case STOP_REQ:
        bt_le_srv_cs_procedure_enable_param_t params;

        params.config_id = 0;
        params.enable = CS_BT_SRV_CONN_LE_CS_PROCEDURES_DISABLED;
        bt_sal_cs_procedure_enable(PRIMARY_ADAPTER, &(cs_sm->addr), &params);
        hsm_transition_to(sm, &connected_state);
        break;
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &disconnected_state);
        break;
    case CONFIG_DONE_EVT:
        hsm_transition_to(sm, &wait_for_security_complete_state);
        break;
    case SECURITY_DONE_EVT:
        hsm_transition_to(sm, &wait_for_procedure_complete_state);
        break;
    default:
        break;
    }

    return true;
}

static void cs_state_machine_event_dispatch(cs_state_machine_t* sm, cs_msg_t* msg)
{
    if (!msg || !sm)
        return;

    hsm_dispatch_event(&sm->sm, msg->id, &msg->cs_data);
}

void cs_state_machine_handle_event(cs_state_machine_t* sm, cs_msg_t* msg)
{
    cs_state_machine_event_dispatch(sm, msg);
}

cs_state_machine_t* cs_state_machine_new(void* context, bt_address_t* bd_addr)
{
    cs_state_machine_t* cs_sm;

    cs_sm = (cs_state_machine_t*)malloc(sizeof(cs_state_machine_t));
    if (!cs_sm)
        return NULL;

    memset(cs_sm, 0, sizeof(cs_state_machine_t));
    cs_sm->service = context;
    hsm_ctor(&cs_sm->sm, (state_t*)&disconnected_state);
    memcpy(&cs_sm->addr, bd_addr, sizeof(bt_address_t));

    return cs_sm;
}

uint32_t cs_state_machine_get_state(cs_state_machine_t* cs_sm)
{
    return hsm_get_current_state_value(&cs_sm->sm);
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
