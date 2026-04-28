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
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_cs.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "callbacks_list.h"
#include "cs_msg.h"
#include "cs_rap_gattc.h"
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
        CASE_RETURN_STR(ENCRYPTED_EVT)
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
    bt_srv_conn_le_cs_capabilities_t remote_capabilities;
} cs_state_machine_t;

static bt_le_srv_cs_set_default_settings_param_t g_default_settings = {};

static void stopped_enter(state_machine_t* sm);
static void stopped_exit(state_machine_t* sm);
static void connected_enter(state_machine_t* sm);
static void connected_exit(state_machine_t* sm);
static void wait_for_encryption_enter(state_machine_t* sm);
static void wait_for_encryption_exit(state_machine_t* sm);
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
static bool wait_for_encryption_process_event(state_machine_t* sm, uint32_t event, void* p_data);
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

static const state_t wait_for_encryption_state = {
    .state_name = "Wait_for_encryption",
    .state_value = CS_STATE_WAIT_FOR_ENCRYPTION,
    .enter = wait_for_encryption_enter,
    .exit = wait_for_encryption_exit,
    .process_event = wait_for_encryption_process_event,
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
    g_default_settings.is_ras = params->is_ras;
}

bool cs_get_is_ras(void)
{
    return g_default_settings.is_ras;
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
        BT_LOGD("stopped_process_event: CONNECTED_EVT received, transitioning to Connected");
        hsm_transition_to(sm, &connected_state);
        break;
    default:
        BT_LOGW("stopped_process_event: unhandled event %" PRIu32 " (%s) in Stopped state",
            event, stack_event_to_string(event));
        break;
    }

    return true;
}

static void connected_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_ENTER(sm, &cs_sm->addr);

    /* Both RAP and RAS Initiator need RAP GATTC connection to discover
     * the Reflector's RAS GATT Server and write CCC for notifications.
     * Without this, Reflector reports "No mode have been set" because
     * char_notify_state is never configured.
     */
    if (g_default_settings.enable_initiator_role) {
        BT_LOGW("Initiator: initiating RAP GATTC connect for discovery, is_ras=%d", cs_get_is_ras());
        bt_status_t ret = cs_rap_gattc_connect(&cs_sm->addr);
        BT_LOGW("Initiator: cs_rap_gattc_connect returned %d", ret);
        if (ret != BT_STATUS_SUCCESS) {
            BT_LOGW("RAP GATTC connect returned %d", ret);
        }
    } else {
        BT_LOGW("Reflector role: skipping RAP GATTC connect");
    }
}

static void connected_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bt_le_srv_cs_create_config_params_t cs_get_default_create_config_params(
    const bt_srv_conn_le_cs_capabilities_t* caps)
{
    bt_le_srv_cs_create_config_params_t config = { 0 };

    config.id = 0;

    /* Use Mode 2 (PBR) with Sub Mode 1 (RTT) to get both
     * phase-based and RTT distance measurements.
     * This matches the Zephyr CS demo configuration.
     */
    config.main_mode_type = CS_BT_SRV_CONN_LE_CS_MAIN_MODE_2;
    config.sub_mode_type = CS_BT_SRV_CONN_LE_CS_SUB_MODE_1;

    BT_LOGW("CS config: main_mode=%d, sub_mode=%d (mode3=%d, nadm_sounding=%d, nadm_random=%d)",
        config.main_mode_type, config.sub_mode_type,
        caps->mode_3_supported,
        caps->amplitude_based_nadm_sounding_supported,
        caps->amplitude_based_nadm_random_supported);

    config.min_main_mode_steps = 2;
    config.max_main_mode_steps = 10;
    config.main_mode_repetition = 0;
    config.mode_0_steps = 3;
    config.role = g_default_settings.enable_initiator_role
        ? CS_BT_SRV_CONN_LE_CS_ROLE_INITIATOR
        : CS_BT_SRV_CONN_LE_CS_ROLE_REFLECTOR;
    config.rtt_type = CS_BT_SRV_CONN_LE_CS_RTT_TYPE_AA_ONLY;
    config.cs_sync_phy = CS_BT_SRV_CONN_LE_CS_SYNC_1M_PHY;
    config.channel_map_repetition = 1;
    config.channel_selection_type = caps->chsel_alg_3c_supported
        ? CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3C
        : CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3B;
    config.ch3c_shape = CS_BT_SRV_CONN_LE_CS_CH3C_SHAPE_HAT;
    config.ch3c_jump = 2;

    /* Enable channels 26-61 (32 consecutive channels) */
    memset(config.channel_map, 0, 10);
    for (uint8_t i = 26; i < 62; i++) {
        config.channel_map[i / 8] |= (1 << (i % 8));
    }

    return config;
}

static bt_le_srv_cs_set_procedure_parameters_param_t cs_get_default_procedure_params(void)
{
    bt_le_srv_cs_set_procedure_parameters_param_t params = { 0 };

    params.config_id = 0;
    params.max_procedure_len = 0xFFFF;
    params.min_procedure_interval = 100;
    params.max_procedure_interval = 100;
    params.max_procedure_count = 0;
    params.min_subevent_len = 15000;
    params.max_subevent_len = 15000;
    params.tone_antenna_config_selection = CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ZERO;
    params.phy = BT_LE_SRV_CS_PROCEDURE_PHY_1M;
    params.tx_power_delta = 0x80;
    params.preferred_peer_antenna = 1;
    params.snr_control_initiator = BT_LE_SRV_CS_SNR_CONTROL_NOT_USED;
    params.snr_control_reflector = BT_LE_SRV_CS_SNR_CONTROL_NOT_USED;

    return params;
}

static bool connected_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    cs_msg_data_t* data = (cs_msg_data_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case START_REQ:
        /* Save params for address tracking */
        cs_sm->params = *(bt_distance_measurement_params_t*)data->data;

        if ((!g_default_settings.enable_reflector_role) && (!g_default_settings.enable_initiator_role)) {
            g_default_settings.enable_reflector_role = true;
        }

        BT_LOGW("Connected: START_REQ, initiator=%d, reflector=%d, triggering link encryption",
            g_default_settings.enable_initiator_role, g_default_settings.enable_reflector_role);

        /* Step 1: trigger link encryption, CS HCI commands require encrypted link */
        bt_status_t sec_ret = bt_sal_cs_set_link_security(PRIMARY_ADAPTER, &(data->bd_addr));
        BT_LOGW("Connected: bt_sal_cs_set_link_security returned %d", sec_ret);
        hsm_transition_to(sm, &wait_for_encryption_state);
        break;
    case ENCRYPTED_EVT:
        /* Reflector side: encryption triggered by remote initiator.
         * Read remote capabilities first, then set_default_settings after receiving them.
         */
        BT_LOGD("Connected: ENCRYPTED_EVT, reading remote capabilities before set_default_settings");
        if (cs_sm->is_capabilities_exchanged) {
            /* Already have capabilities, go straight to set_default_settings */
            bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &cs_sm->addr, &g_default_settings);
        } else {
            bt_sal_cs_read_remote_supported_capabilities(PRIMARY_ADAPTER, &cs_sm->addr);
        }
        break;
    case CAPABILITIES_RECEIVED_EVT: {
        /* Save remote capabilities, then set_default_settings so controller is ready */
        bt_srv_conn_le_cs_capabilities_t* caps = (bt_srv_conn_le_cs_capabilities_t*)data->data;
        if (caps) {
            cs_sm->remote_capabilities = *caps;
        }
        cs_sm->is_capabilities_exchanged = true;
        BT_LOGD("Connected: capabilities received, setting default settings for reflector side");
        bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &cs_sm->addr, &g_default_settings);
        break;
    }
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &stopped_state);
        break;
    default:
        break;
    }

    return true;
}

/* Wait_for_encryption state: waiting for BLE link encryption to complete */

static void wait_for_encryption_enter(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_ENTER(sm, &cs_sm->addr);
}

static void wait_for_encryption_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
}

static bool wait_for_encryption_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;
    cs_msg_data_t* data = (cs_msg_data_t*)p_data;

    CS_DBG_EVENT(sm, &cs_sm->addr, event);
    switch (event) {
    case ENCRYPTED_EVT:
        /* SMP encryption done. Now do RAS discovery + features read + CCC write,
         * then proceed with CS capabilities exchange. */
        BT_LOGW("Wait_for_encryption: ENCRYPTED_EVT, starting RAS discovery");
        cs_rap_gattc_discover(&cs_sm->addr);

        /* Also trigger RAS features read (will be processed after discover completes) */
        BT_LOGW("Wait_for_encryption: ENCRYPTED_EVT, reading remote CS capabilities");
        if (cs_sm->is_capabilities_exchanged) {
            BT_LOGW("Wait_for_encryption: capabilities already exchanged, setting defaults + create_config");
            bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &cs_sm->addr, &g_default_settings);
            bt_le_srv_cs_create_config_params_t config = cs_get_default_create_config_params(
                &cs_sm->remote_capabilities);
            bt_sal_cs_create_config(PRIMARY_ADAPTER, &cs_sm->addr, &config,
                BT_LE_SRV_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE);
            hsm_transition_to(sm, &wait_for_config_complete_state);
        } else {
            bt_sal_cs_read_remote_supported_capabilities(PRIMARY_ADAPTER, &cs_sm->addr);
        }
        break;
    case CAPABILITIES_RECEIVED_EVT: {
        /* Save remote capabilities, then set_default_settings + create_config */
        bt_srv_conn_le_cs_capabilities_t* caps = (bt_srv_conn_le_cs_capabilities_t*)data->data;
        if (caps) {
            cs_sm->remote_capabilities = *caps;
            BT_LOGW("Wait_for_encryption: CAPABILITIES_RECEIVED, mode3=%d, nadm_sounding=%d, nadm_random=%d",
                caps->mode_3_supported, caps->amplitude_based_nadm_sounding_supported,
                caps->amplitude_based_nadm_random_supported);
        }
        cs_sm->is_capabilities_exchanged = true;

        BT_LOGW("Wait_for_encryption: setting default settings + create_config");
        bt_sal_cs_set_default_settings(PRIMARY_ADAPTER, &cs_sm->addr, &g_default_settings);

        bt_le_srv_cs_create_config_params_t config = cs_get_default_create_config_params(
            &cs_sm->remote_capabilities);
        bt_sal_cs_create_config(PRIMARY_ADAPTER, &(data->bd_addr), &config,
            BT_LE_SRV_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE);
        BT_LOGW("Wait_for_encryption: transitioning to Wait_for_config_complete");
        hsm_transition_to(sm, &wait_for_config_complete_state);
        break;
    }
    case DISCONNECTED_EVT:
        hsm_transition_to(sm, &stopped_state);
        break;
    case CONNECTED_EVT:
        /* ACL connection is now established (from cs_rap_gattc_connection_cb).
         * The initial set_link_security in START_REQ failed because the ACL
         * link wasn't up yet. Retry now that the connection is live. */
        BT_LOGW("Wait_for_encryption: CONNECTED_EVT, ACL link up, retrying set_link_security");
        bt_sal_cs_set_link_security(PRIMARY_ADAPTER, &(cs_sm->addr));
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
        /* Config created, now enable CS security */
        BT_LOGW("Wait_for_config_complete: CONFIG_DONE_EVT, enabling CS security");
        bt_sal_cs_security_enable(PRIMARY_ADAPTER, &cs_sm->addr);
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
    case SECURITY_DONE_EVT: {
        /* CS security enabled, now set procedure parameters and enable procedure */
        BT_LOGW("Wait_for_security_complete: SECURITY_DONE_EVT, setting procedure params + enable");
        bt_le_srv_cs_set_procedure_parameters_param_t proc_params = cs_get_default_procedure_params();
        bt_sal_cs_set_procedure_parameters(PRIMARY_ADAPTER, &cs_sm->addr, &proc_params);

        bt_le_srv_cs_procedure_enable_param_t enable = {
            .config_id = 0,
            .enable = 1,
        };
        bt_sal_cs_procedure_enable(PRIMARY_ADAPTER, &cs_sm->addr, &enable);
        BT_LOGW("Wait_for_security_complete: procedure_enable called, transitioning to Wait_for_procedure_complete");
        hsm_transition_to(sm, &wait_for_procedure_complete_state);
        break;
    }
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
    cs_notify_distance_measure_started(&cs_sm->addr);
}

static void started_exit(state_machine_t* sm)
{
    cs_state_machine_t* cs_sm = (cs_state_machine_t*)sm;

    CS_DBG_EXIT(sm, &cs_sm->addr);
    cs_notify_distance_measure_stopped(&cs_sm->addr, 0);
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
