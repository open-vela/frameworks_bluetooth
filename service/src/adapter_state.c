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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif

#include "adapter_internel.h"
#include "bt_adapter.h"
#include "btservice.h"
#include "media_system.h"
#include "sal_interface.h"
#include "service_manager.h"
#include "state_machine.h"

#define LOG_TAG "adapter-stm"
#include "bt_utils.h"
#include "utils/log.h"

static void off_enter(state_machine_t* sm);
static void off_exit(state_machine_t* sm);
static void ble_turning_on_enter(state_machine_t* sm);
static void ble_turning_on_exit(state_machine_t* sm);
static void ble_on_enter(state_machine_t* sm);
static void ble_on_exit(state_machine_t* sm);
static void turning_on_enter(state_machine_t* sm);
static void turning_on_exit(state_machine_t* sm);
static void on_state_enter(state_machine_t* sm);
static void on_state_exit(state_machine_t* sm);
static void turning_off_enter(state_machine_t* sm);
static void turning_off_exit(state_machine_t* sm);
static void ble_turning_off_enter(state_machine_t* sm);
static void ble_turning_off_exit(state_machine_t* sm);

static bool off_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool ble_turning_on_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool ble_on_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool turning_on_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool on_state_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool turning_off_process_event(state_machine_t* sm, uint32_t event, void* p_data);
static bool ble_turning_off_process_event(state_machine_t* sm, uint32_t event, void* p_data);

static const state_t off_state = {
    .state_name = "Off",
    .state_value = BT_ADAPTER_STATE_OFF,
    .enter = off_enter,
    .exit = off_exit,
    .process_event = off_process_event,
};

static const state_t ble_turning_on_state = {
    .state_name = "BleTurningOn",
    .state_value = BT_ADAPTER_STATE_BLE_TURNING_ON,
    .enter = ble_turning_on_enter,
    .exit = ble_turning_on_exit,
    .process_event = ble_turning_on_process_event,
};

static const state_t ble_on_state = {
    .state_name = "BleOn",
    .state_value = BT_ADAPTER_STATE_BLE_ON,
    .enter = ble_on_enter,
    .exit = ble_on_exit,
    .process_event = ble_on_process_event,
};

static const state_t turning_on_state = {
    .state_name = "TurningOn",
    .state_value = BT_ADAPTER_STATE_TURNING_ON,
    .enter = turning_on_enter,
    .exit = turning_on_exit,
    .process_event = turning_on_process_event,
};

static const state_t on_state = {
    .state_name = "On",
    .state_value = BT_ADAPTER_STATE_ON,
    .enter = on_state_enter,
    .exit = on_state_exit,
    .process_event = on_state_process_event,
};

static const state_t turning_off_state = {
    .state_name = "TurningOff",
    .state_value = BT_ADAPTER_STATE_TURNING_OFF,
    .enter = turning_off_enter,
    .exit = turning_off_exit,
    .process_event = turning_off_process_event,
};

static const state_t ble_turning_off_state = {
    .state_name = "BleTurningOff",
    .state_value = BT_ADAPTER_STATE_BLE_TURNING_OFF,
    .enter = ble_turning_off_enter,
    .exit = ble_turning_off_exit,
    .process_event = ble_turning_off_process_event,
};

typedef struct adapter_state_machine {
    state_machine_t sm;
    bool ble_enabled;
    bool pending_turn_on;
    bool a2dp_offloading;
    bool hfp_offloading;
    bool lea_offloading;
} adapter_state_machine_t;

#define ADPATER_STM_DEBUG 1
#if ADPATER_STM_DEBUG

static const char* event_to_string(uint16_t event)
{
    switch (event) {
        CASE_RETURN_STR(SYS_TURN_ON)
        CASE_RETURN_STR(SYS_TURN_OFF)
        CASE_RETURN_STR(TURN_ON_BLE)
        CASE_RETURN_STR(TURN_OFF_BLE)
        CASE_RETURN_STR(BREDR_ENABLED)
        CASE_RETURN_STR(BREDR_DISABLED)
        CASE_RETURN_STR(BREDR_PROFILE_ENABLED)
        CASE_RETURN_STR(BREDR_PROFILE_DISABLED)
        CASE_RETURN_STR(BREDR_ENABLE_TIMEOUT)
        CASE_RETURN_STR(BREDR_DISABLE_TIMEOUT)
        CASE_RETURN_STR(BREDR_ENABLE_PROFILE_TIMEOUT)
        CASE_RETURN_STR(BREDR_DISABLE_PROFILE_TIMEOUT)
        CASE_RETURN_STR(BLE_ENABLED)
        CASE_RETURN_STR(BLE_DISABLED)
        CASE_RETURN_STR(BLE_PROFILE_ENABLED)
        CASE_RETURN_STR(BLE_PROFILE_DISABLED)
        CASE_RETURN_STR(BLE_ENABLE_TIMEOUT)
        CASE_RETURN_STR(BLE_DISABLE_TIMEOUT)
        CASE_RETURN_STR(BLE_ENABLE_PROFILE_TIMEOUT)
        CASE_RETURN_STR(BLE_DISABLE_PROFILE_TIMEOUT)
    default:
        return "unknown";
    }
}

#define ADAPTER_DBG_ENTER(__sm)                           \
    BT_LOGD("Enter, PrevState=%s ---> NewState=%s",       \
        hsm_get_state_name(hsm_get_previous_state(__sm)), \
        hsm_get_current_state_name(__sm))

#define ADAPTER_DBG_EXIT(__sm) \
    BT_LOGD("Exit, State=%s", hsm_get_current_state_name(__sm))

#define ADAPTER_DBG_EVENT(__sm, __event) \
    BT_LOGD("Process, State=%s, Event=%s", hsm_get_current_state_name(__sm), event_to_string(__event))
#else
#define ADAPTER_DBG_ENTER(__sm)
#define ADAPTER_DBG_EXIT(__sm)
#define ADAPTER_DBG_EVENT(__sm, __event)
#endif

static bool a2dp_is_offloading(void)
{
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    return property_get_bool("persist.bluetooth.a2dp.offloading", false);
#else
    return false;
#endif
}

static bool hfp_is_offloading(void)
{
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    return property_get_bool("persist.bluetooth.hfp.offloading", false);
#else
    return false;
#endif
}

static bool lea_is_offloading(void)
{
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    return property_get_bool("persist.bluetooth.lea.offloading", false);
#else
    return false;
#endif
}

static void off_enter(state_machine_t* sm)
{
    adapter_state_machine_t* stm = (adapter_state_machine_t*)sm;
    ADAPTER_DBG_ENTER(sm);

    stm->ble_enabled = false;
    stm->pending_turn_on = false;
    const state_t* prev = hsm_get_previous_state(sm);
    if (prev) {
        adapter_notify_state_change(hsm_get_state_value(prev), BT_ADAPTER_STATE_OFF);
    } else {
        stm->a2dp_offloading = a2dp_is_offloading();
        stm->hfp_offloading = hfp_is_offloading();
        stm->lea_offloading = lea_is_offloading();
    }
}

static void off_exit(state_machine_t* sm)
{
    ADAPTER_DBG_EXIT(sm);
}

static void adapter_notify_media_offloading(adapter_state_machine_t* stm)
{
    profile_msg_t msg;

    msg.event = PROFILE_EVT_A2DP_OFFLOADING;
    msg.data.valuebool = stm->a2dp_offloading;
    service_manager_processmsg(&msg);

    msg.event = PROFILE_EVT_HFP_OFFLOADING;
    msg.data.valuebool = stm->hfp_offloading;
    service_manager_processmsg(&msg);

    msg.event = PROFILE_EVT_LEA_OFFLOADING;
    msg.data.valuebool = stm->lea_offloading;
    service_manager_processmsg(&msg);
}

static bool off_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    adapter_state_machine_t* stm = (adapter_state_machine_t*)sm;
    ADAPTER_DBG_EVENT(sm, event);

    switch (event) {
    case SYS_TURN_ON:
        adapter_notify_media_offloading(stm);
        if (!adapter_is_support_le()) {
            hsm_transition_to(sm, &turning_on_state);
            break;
        }
        if (adapter_is_support_bredr())
            stm->pending_turn_on = true;
    case TURN_ON_BLE:
        hsm_transition_to(sm, &ble_turning_on_state);
        break;
    default:
        return false;
    }

    return true;
}

static void ble_turning_on_enter(state_machine_t* sm)
{
    ADAPTER_DBG_ENTER(sm);
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    bt_status_t status = bt_sal_le_enable(PRIMARY_ADAPTER);
    if (status == BT_STATUS_SUCCESS)
        adapter_notify_state_change(BT_ADAPTER_STATE_OFF, BT_ADAPTER_STATE_BLE_TURNING_ON);
#else
    BT_LOGE("Not supported");
#endif
}

static void ble_turning_on_exit(state_machine_t* sm)
{
    ADAPTER_DBG_EXIT(sm);
}

static bool ble_turning_on_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ADAPTER_DBG_EVENT(sm, event);

    switch (event) {
    case BLE_ENABLED:
        /* LE profile service startup */
        service_manager_startup(BT_TRANSPORT_BLE);
        break;
    case BLE_PROFILE_ENABLED:
        hsm_transition_to(sm, &ble_on_state);
        break;
    case BLE_ENABLE_TIMEOUT:
    case BLE_ENABLE_PROFILE_TIMEOUT:
        break;
    default:
        return false;
    }

    return true;
}

static void ble_on_enter(state_machine_t* sm)
{
    adapter_state_machine_t* stm = (adapter_state_machine_t*)sm;
    ADAPTER_DBG_ENTER(sm);

    const state_t* prev = hsm_get_previous_state(sm);
    adapter_notify_state_change(hsm_get_state_value(prev), BT_ADAPTER_STATE_BLE_ON);
    stm->ble_enabled = true;
    adapter_on_le_enabled(stm->pending_turn_on);
    stm->pending_turn_on = false;
}

static void ble_on_exit(state_machine_t* sm)
{
    ADAPTER_DBG_EXIT(sm);
}

static bool ble_on_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ADAPTER_DBG_EVENT(sm, event);

    switch (event) {
    case SYS_TURN_ON:
        hsm_transition_to(sm, &turning_on_state);
        break;
    case SYS_TURN_OFF:
    case TURN_OFF_BLE:
        hsm_transition_to(sm, &ble_turning_off_state);
        break;
    default:
        return false;
    }

    return true;
}

static void turning_on_enter(state_machine_t* sm)
{
    ADAPTER_DBG_ENTER(sm);
    bt_status_t status = bt_sal_enable(PRIMARY_ADAPTER);
    if (status == BT_STATUS_SUCCESS) {
        const state_t* prev = hsm_get_previous_state(sm);
        adapter_notify_state_change(hsm_get_state_value(prev), BT_ADAPTER_STATE_TURNING_ON);
    }
}

static void turning_on_exit(state_machine_t* sm)
{
    ADAPTER_DBG_EXIT(sm);
}

static bool turning_on_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ADAPTER_DBG_EVENT(sm, event);

    switch (event) {
    case BREDR_ENABLED:
        /* BREDR profile service startup */
        service_manager_startup(BT_TRANSPORT_BREDR);
        break;
    case BREDR_PROFILE_ENABLED:
        hsm_transition_to(sm, &on_state);
        break;
    case BREDR_ENABLE_TIMEOUT:
    case BREDR_ENABLE_PROFILE_TIMEOUT:
        break;
    default:
        return false;
    }

    return true;
}

static void on_state_enter(state_machine_t* sm)
{
    ADAPTER_DBG_ENTER(sm);
    const state_t* prev = hsm_get_previous_state(sm);
    adapter_on_br_enabled();
    adapter_notify_state_change(hsm_get_state_value(prev), BT_ADAPTER_STATE_ON);

#if defined(CONFIG_BLUETOOTH_A2DP) || defined(CONFIG_BLUETOOTH_LE_AUDIO_SUPPORT) || defined(CONFIG_BLUETOOTH_HFP_HF) || defined(CONFIG_BLUETOOTH_HFP_AG)
    adapter_state_machine_t* stm = (adapter_state_machine_t*)sm;
    bt_media_set_a2dp_offloading(stm->a2dp_offloading);
    bt_media_set_hfp_offloading(stm->hfp_offloading);
    bt_media_set_lea_offloading(stm->lea_offloading);
#endif
}

static void on_state_exit(state_machine_t* sm)
{
    ADAPTER_DBG_EXIT(sm);
}

static bool on_state_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ADAPTER_DBG_EVENT(sm, event);

    switch (event) {
    case SYS_TURN_OFF:
        hsm_transition_to(sm, &turning_off_state);
        break;
    default:
        return false;
    }

    return true;
}

static void turning_off_enter(state_machine_t* sm)
{
    ADAPTER_DBG_ENTER(sm);
    /* profile service shotdown */
    service_manager_shutdown(BT_TRANSPORT_BREDR);
    adapter_notify_state_change(BT_ADAPTER_STATE_ON, BT_ADAPTER_STATE_TURNING_OFF);
}

static void turning_off_exit(state_machine_t* sm)
{
    ADAPTER_DBG_EXIT(sm);
    adapter_on_br_disabled();
}

static bool turning_off_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ADAPTER_DBG_EVENT(sm, event);

    switch (event) {
    case BREDR_PROFILE_DISABLED:
        bt_sal_disable(PRIMARY_ADAPTER);
        break;
    case BREDR_DISABLED:
        if (adapter_is_support_le()) {
            hsm_transition_to(sm, &ble_turning_off_state);
            break;
        }
        hsm_transition_to(sm, &off_state);
        break;
    case BREDR_DISABLE_TIMEOUT:
    case BREDR_DISABLE_PROFILE_TIMEOUT:
        break;
    default:
        return false;
    }

    return true;
}

static void ble_turning_off_enter(state_machine_t* sm)
{
    ADAPTER_DBG_ENTER(sm);
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    /* LE profile service shotdown */
    service_manager_shutdown(BT_TRANSPORT_BLE);
    const state_t* prev = hsm_get_previous_state(sm);
    adapter_notify_state_change(hsm_get_state_value(prev), BT_ADAPTER_STATE_BLE_TURNING_OFF);
#else

#endif
}

static void ble_turning_off_exit(state_machine_t* sm)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_state_machine_t* stm = (adapter_state_machine_t*)sm;
    ADAPTER_DBG_EXIT(sm);
    stm->ble_enabled = false;
    adapter_on_le_disabled();
#endif
}

static bool ble_turning_off_process_event(state_machine_t* sm, uint32_t event, void* p_data)
{
    ADAPTER_DBG_EVENT(sm, event);

    switch (event) {
    case BLE_PROFILE_DISABLED:
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        bt_sal_le_disable(PRIMARY_ADAPTER);
#endif
        break;
    case BLE_DISABLED:
        hsm_transition_to(sm, &off_state);
        break;
    case BLE_DISABLE_TIMEOUT:
    case BLE_DISABLE_PROFILE_TIMEOUT:
        break;
    default:
        return false;
    }

    return true;
}

adapter_state_machine_t* adapter_state_machine_new(void* context)
{
    adapter_state_machine_t* stm = malloc(sizeof(adapter_state_machine_t));
    if (!stm)
        return NULL;

    memset(stm, 0, sizeof(adapter_state_machine_t));
    hsm_ctor(&stm->sm, &off_state);

    return stm;
}

void adapter_state_machine_destory(adapter_state_machine_t* stm)
{
    if (!stm)
        return;

    hsm_dtor(&stm->sm);
    free((void*)stm);
}
