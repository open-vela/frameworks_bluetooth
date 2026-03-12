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
#include "sal_le_cs_interface.h"
#include "service_loop.h"
#include "state_machine.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LE_CS

static char* stack_event_to_string(cs_msg_id_t msg_id);

#define CS_TRANS_DBG(_sm, _addr, _action)                                                        \
    do {                                                                                         \
        char __addr_str[BT_ADDR_STR_LENGTH] = { 0 };                                             \
        bt_addr_ba2str(_addr, __addr_str);                                                       \
        BT_LOGD("%s State=%s, Peer=[%s]", _action, hsm_get_current_state_name(_sm), __addr_str); \
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
        CASE_RETURN_STR(CAPABILITIES_RECEIVED_EVT)
        CASE_RETURN_STR(DISCONNECTED_EVT)
        CASE_RETURN_STR(CONNECTED_EVT)
        CASE_RETURN_STR(CONFIG_DONE_EVT)
        CASE_RETURN_STR(SECURITY_DONE_EVT)
        CASE_RETURN_STR(PROCEDURE_DONE_EVT)
        CASE_RETURN_STR(SUBEVENT_RESULT_EVT)
    default:
        return "UNKNOWN_EVENT";
    }
}

typedef struct _cs_state_machine {
    state_machine_t sm;
    void* service;
    bt_address_t addr;
    bool is_capabilities_exchanged;
    bt_distance_measurement_params_t params;
} cs_state_machine_t;

static bt_le_srv_cs_set_default_settings_param_t g_default_settings = {};

static void stopped_enter(state_machine_t* sm);
static void stopped_exit(state_machine_t* sm);
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

static bool stopped_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool wait_for_config_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool wait_for_security_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool wait_for_procedure_complete_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool started_process_event(state_machine_t* sm, uint32_t event, void* p_data);

static const state_t stopped_state = {
    .state_name = "Stopped",
    .state_value = CS_STATE_STOPPED,
    .enter = stopped_enter,
    .exit = stopped_exit,
    .process_event = stopped_process_event,
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
    .state_value = CS_STATE_START,
    .enter = started_enter,
    .exit = started_exit,
    .process_event = started_process_event,
};

void cs_update_default_settings(const bt_cs_set_params_t* params)
{
    g_default_settings.enable_initiator_role = params->role & 0x01;
    g_default_settings.enable_reflector_role = params->role & 0x02;
    g_default_settings.cs_sync_antenna_selection = params->cs_sync_antenna_selection;
    g_default_settings.max_tx_power = params->max_tx_power;
}

static void stopped_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_ENTER(sm, &cs_sm->addr);
}

static void stopped_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool stopped_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
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

    CS_DBG_ENTER(sm, &cs_sm->addr);
}

static void connected_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    cs_msg_data_t* data = (cs_msg_data_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case START_REQ:
        bt_distance_measurement_params_t* params = data->data;

        g_default_settings.enable_initiator_role = params->role == CS_BT_SRV_CONN_LE_CS_ROLE_INITIATOR ? true : false;
        g_default_settings.enable_reflector_role = params->role == CS_BT_SRV_CONN_LE_CS_ROLE_REFLECTOR ? true : false;
        g_default_settings.cs_sync_antenna_selection = params->antenna_paths_mask;
        cs_sm->params = *params;

        if (cs_sm->is_capabilities_exchanged) {
            bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &(data->bd_addr), &g_default_settings);
            hsm_transition_to(sm, &wait_for_config_complete_state);
        } else {
            bt_sal_cs_read_remote_supported_capabilities(PRIMARY_ADAPTER, &(data->bd_addr));
        }
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &stopped_state);
        break;
    case CAPABILITIES_RECEIVED_EVT:
        cs_sm->is_capabilities_exchanged = true;

        if ((!g_default_settings.enable_reflector_role) && (!g_default_settings.enable_initiator_role)) {
            /** the channel sounding procedures are initiated by the remote device */
            g_default_settings.enable_reflector_role = true;
        }

        bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &(data->bd_addr), &g_default_settings);
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

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &stopped_state);
        break;
    case CONFIG_DONE_EVT:
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

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &stopped_state);
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

static void wait_for_procedure_complete_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

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

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &stopped_state);
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

    CS_DBG_ENTER(sm, &cs_sm->addr);
}

static void started_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool started_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    cs_msg_data_t* data = (cs_msg_data_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case CONNECTED_EVT:
        hsm_transition_to(sm, &connected_state);
        break;
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &stopped_state);
        break;
    case CONFIG_DONE_EVT:
        hsm_transition_to(sm, &wait_for_security_complete_state);
        break;
    case SECURITY_DONE_EVT:
        hsm_transition_to(sm, &wait_for_procedure_complete_state);
        break;
    case PROCEDURE_DONE_EVT: {
        bt_srv_conn_le_cs_procedure_enable_complete_t* enable = (bt_srv_conn_le_cs_procedure_enable_complete_t*)data->data;

        if (enable) {
            if (enable->state == CS_BT_SRV_CONN_LE_CS_PROCEDURES_DISABLED) {
                hsm_transition_to(sm, &connected_state);
            }
        }

        break;
    }
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

    cs_sm = (cs_state_machine_t*)zalloc(sizeof(cs_state_machine_t));
    if (!cs_sm)
        return NULL;

    cs_sm->service = context;
    hsm_ctor(&cs_sm->sm, (state_t*)&stopped_state);
    memcpy(&cs_sm->addr, bd_addr, sizeof(bt_address_t));

    return cs_sm;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
