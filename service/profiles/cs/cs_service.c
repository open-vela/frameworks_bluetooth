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
#define LOG_TAG "cs_service"

#include <stdint.h>
#include <stdlib.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_cs.h"
#include "bt_list.h"
#include "callbacks_list.h"
#include "cs_msg.h"
#include "cs_rap.h"
#include "cs_ras.h"
#include "cs_ras_gatts.h"
#include "cs_ras_test.h"
#include "cs_service.h"
#include "cs_state_machine.h"
#include "sal_le_cs_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LE_CS

#define CS_CALLBACK_FOREACH(_list, _cback, ...) BT_CALLBACK_FOREACH(_list, cs_callbacks_t, _cback, ##__VA_ARGS__)

typedef struct {
    callbacks_list_t* callbacks;
    struct list_node list;
} cs_service_t;

static cs_service_t g_cs_service;

static subevent_result_cb_t result_cb = NULL;

static void service_startup(profile_on_startup_t cb);
static void service_shutdown(profile_on_shutdown_t cb);
static const void* get_cs_profile_interface(void);

static cs_device_t* cs_device_new(void* ctx, bt_address_t* bd_addr)
{
    cs_device_t* device;

    device = (cs_device_t*)malloc(sizeof(cs_device_t));
    if (!device)
        return NULL;

    memcpy(&device->bd_addr, bd_addr, sizeof(bt_address_t));
    device->cs_sm = cs_state_machine_new(ctx, bd_addr);
    if (!device->cs_sm) {
        BT_LOGE("Create state machine failed");
        free(device);
        return NULL;
    }

    return device;
}

cs_device_t* find_cs_device_by_addr(struct list_node* list, bt_address_t* bd_addr)
{
    cs_device_t* device;
    struct list_node* node;

    list_for_every(list, node)
    {
        device = (cs_device_t*)node;
        if (memcmp(&device->bd_addr, bd_addr, sizeof(bt_address_t)) == 0)
            return device;
    }

    return NULL;
}

static cs_device_t* find_or_create_device(bt_address_t* bd_addr)
{
    cs_device_t* device = find_cs_device_by_addr(&g_cs_service.list, bd_addr);
    if (device)
        return device;

    device = cs_device_new(&g_cs_service, bd_addr);
    if (!device) {
        BT_LOGE("CS new device alloc failed");
        return NULL;
    }
    list_add_tail(&g_cs_service.list, &device->node);

    return device;
}

static cs_state_machine_t* get_state_machine(bt_address_t* bd_addr)
{
    cs_device_t* device = find_or_create_device(bd_addr);

    if (!device)
        return NULL;

    return device->cs_sm;
}

static void cs_rap_distance_result_handler(bt_address_t* addr, cs_rap_internal_distance_result_t* result)
{
    if (!addr || !result) {
        return;
    }

    bt_distance_measurement_result_t app_result = { 0 };

    /* Convert internal result to application-facing result */
    if (result->rtt_valid) {
        app_result.centimeter = (uint32_t)(result->rtt_distance * 100.0f);
        app_result.method = METHOD_CS;
    } else if (result->phase_valid) {
        app_result.centimeter = (uint32_t)(result->phase_distance * 100.0f);
        app_result.method = METHOD_CS;
    }

    app_result.confidence_level = (result->rtt_valid || result->phase_valid) ? 100 : 0;

    CS_CALLBACK_FOREACH(g_cs_service.callbacks, cs_distance_measure_result_cb, addr, &app_result);

    /* Also fire RAP-specific distance callback if registered */
    cs_rap_distance_result_t rap_result;
    rap_result.ranging_counter = result->ranging_counter;
    rap_result.rtt_distance = result->rtt_distance;
    rap_result.phase_distance = result->phase_distance;
    rap_result.mode1_samples = result->mode1_samples;
    rap_result.mode2_samples = result->mode2_samples;
    rap_result.rtt_valid = result->rtt_valid;
    rap_result.phase_valid = result->phase_valid;

    CS_CALLBACK_FOREACH(g_cs_service.callbacks, rap_distance_result_cb, addr, &rap_result);

    BT_LOGD("cs_rap_distance_result_handler: rtt=%.2f phase=%.2f",
        rap_result.rtt_distance, rap_result.phase_distance);
}

void cs_notify_distance_measure_started(bt_address_t* addr)
{
    CS_CALLBACK_FOREACH(g_cs_service.callbacks, cs_distance_measure_started_cb, addr, METHOD_CS);
}

void cs_notify_distance_measure_stopped(bt_address_t* addr, uint8_t reason)
{
    CS_CALLBACK_FOREACH(g_cs_service.callbacks, cs_distance_measure_stopped_cb, addr, reason, METHOD_CS);
}

static bt_status_t cs_subevent_result_callbacks(bt_address_t* addr, void* data)
{
    if (result_cb == NULL) {
        BT_LOGD("The subevent result callbacks haven't been registered.");
        return BT_STATUS_PARM_INVALID;
    }

    result_cb(addr, (bt_srv_conn_le_cs_subevent_result_t*)data);
    return BT_STATUS_SUCCESS;
}

static void cs_service_handle_event(void* data)
{
    cs_msg_t* msg = (cs_msg_t*)data;

    switch (msg->id) {
    case CS_STARTUP:
        service_startup((profile_on_startup_t)msg->cs_data.cb);
        break;
    case CS_SHUTDOWN:
        service_shutdown((profile_on_shutdown_t)msg->cs_data.cb);
        break;
    case SUBEVENT_RESULT_EVT:
        cs_subevent_result_callbacks(&msg->cs_data.bd_addr, msg->cs_data.data);
        if (msg->cs_data.data) {
            free(((bt_srv_conn_le_cs_subevent_result_t*)(msg->cs_data.data))->step_data_buf);
        }

        break;
    case LOCAL_SUPPORTED_CAPABILITIES_EVT:
        // TODO:

        break;
    default: {
        cs_state_machine_t* cs_sm;
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(&msg->cs_data.bd_addr, addr_str);
        BT_LOGD("cs_service_handle_event: dispatching event %d to state machine, addr=%s", msg->id, addr_str);
        cs_sm = get_state_machine(&msg->cs_data.bd_addr);
        if (!cs_sm) {
            BT_LOGE("cs_service_handle_event: get_state_machine returned NULL for addr=%s", addr_str);
            break;
        }

        cs_state_machine_handle_event(cs_sm, msg);
        break;
    }
    }

    cs_msg_destroy(msg);
}

static void do_in_cs_service(cs_msg_t* msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(cs_service_handle_event, msg);
}

static bt_status_t cs_init(void)
{
    g_cs_service.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    list_initialize(&g_cs_service.list);

    return BT_STATUS_SUCCESS;
}

static void cs_cleanup(void)
{
    bt_callbacks_list_free(g_cs_service.callbacks);
    g_cs_service.callbacks = NULL;

    struct list_node* node;
    struct list_node* tmp;

    list_for_every_safe(&g_cs_service.list, node, tmp)
    {
        list_delete(node);
        free(node);
    }
}

static void service_startup(profile_on_startup_t cb)
{
    BT_LOGD("CS service startup, is_ras=%d", cs_get_is_ras());

    if (cs_get_is_ras()) {
        bt_cs_ras_enable();
    }
    /* RAP init is deferred to cs_set_config when is_ras=false */

    cb(PROFILE_CS, true);
}

static void service_shutdown(profile_on_shutdown_t cb)
{
    if (cs_get_is_ras()) {
        bt_cs_ras_disable();
    }
    /* Always try to deinit RAP in case it was initialized */
    cs_rap_deinit();

    cb(PROFILE_CS, true);
}

static bt_status_t cs_service_startup(profile_on_startup_t cb)
{
    cs_msg_t* msg = cs_msg_new(CS_STARTUP, NULL);
    msg->cs_data.cb = cb;
    do_in_cs_service(msg);

    return BT_STATUS_SUCCESS;
}

static bt_status_t cs_service_shutdown(profile_on_shutdown_t cb)
{
    cs_msg_t* msg = cs_msg_new(CS_SHUTDOWN, NULL);
    msg->cs_data.cb = cb;
    do_in_cs_service(msg);

    return BT_STATUS_SUCCESS;
}

static void* cs_register_callbacks(void* remote, const cs_callbacks_t* callbacks)
{
    return bt_remote_callbacks_register(g_cs_service.callbacks, remote, (void*)callbacks);
}

static bool cs_unregister_callbacks(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_cs_service.callbacks, remote, cookie);
}

static bt_status_t cs_start_distance_measurement(bt_distance_measurement_params_t* params)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    bt_addr_ba2str(&params->addr, addr_str);
    BT_LOGD("cs_start_distance_measurement: addr=%s, method=%d", addr_str, params->method);

    switch (params->method) {
    case METHOD_AUTO:
    case METHOD_RSSI:
        BT_LOGD("cs_start_distance_measurement: method %d not supported", params->method);
        break;
    case METHOD_CS: {
        cs_msg_t* conn_msg = cs_msg_new(CONNECTED_EVT, &params->addr);
        do_in_cs_service(conn_msg);

        cs_msg_t* msg = cs_msg_new(START_REQ, &params->addr);
        bt_distance_measurement_params_t* cs_params = (bt_distance_measurement_params_t*)zalloc(sizeof(bt_distance_measurement_params_t));

        if (!cs_params) {
            BT_LOGE("cs_start_distance_measurement: malloc failed");
            return BT_STATUS_FAIL;
        }

        memcpy(cs_params, params, sizeof(bt_distance_measurement_params_t));
        msg->cs_data.data = cs_params;
        do_in_cs_service(msg);
        break;
    }

    default:
        BT_LOGD("cs_start_distance_measurement: unknown method %d", params->method);
        break;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t cs_stop_distance_measurement(bt_address_t* addr, int method, bool timeout)
{
    BT_LOGD("cs_stop_distance_measurement");
    switch (method) {
    case METHOD_AUTO:
    case METHOD_RSSI:
        BT_LOGD("not supported method");
        break;
    case METHOD_CS: {
        cs_msg_t* msg = cs_msg_new(STOP_REQ, addr);
        do_in_cs_service(msg);
        break;
    }

    default:
        break;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t cs_set_config(bt_address_t* addr, const bt_cs_set_params_t* params)
{
    BT_LOGD("cs_set_config: addr=%s, is_ras=%d, ras_feature=0x%08" PRIx32 ", role=0x%02x, antenna=0x%02x, max_tx_power=%d",
        bt_addr_str(addr), params->is_ras, params->ras_feature, params->role,
        params->cs_sync_antenna_selection, params->max_tx_power);

    cs_update_default_settings(params);

    if (params->is_ras) {
        /* RAS server mode — ensure RAS is enabled first */
        bt_cs_ras_enable();

        bt_status_t ret = bt_cs_ras_set_feature(params->ras_feature);
        if (ret != BT_STATUS_SUCCESS) {
            return ret;
        }

        ret = bt_cs_ras_set_role(params->role);
        if (ret != BT_STATUS_SUCCESS) {
            return ret;
        }

        /* RAS Initiator also needs RAP GATTC to discover Reflector's RAS
         * GATT Server and write CCC to enable notifications. Without this,
         * the Reflector reports "No mode have been set" because CCC is
         * never written and char_notify_state is not configured.
         */
        if (params->role == CS_ROLE_REFLECTOR) {
            cs_rap_init(cs_rap_distance_result_handler);
        }
    } else {
        /* RAP client mode — initialize RAP if not already done */
        cs_rap_init(cs_rap_distance_result_handler);
    }

    return BT_STATUS_SUCCESS;
}

#ifdef CONFIG_BT_CS_RAS_TEST
static bt_status_t cs_test(void* data, uint16_t len)
{
    int err = cs_ras_subevent_recv_test(data, len);
    return (err == 0) ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}
#endif /* CONFIG_BT_CS_RAS_TEST */

static const bt_cs_interface_t cs_interface = {
    .size = sizeof(cs_interface),
    .register_callbacks = cs_register_callbacks,
    .unregister_callbacks = cs_unregister_callbacks,
    .start_distance_measurement = cs_start_distance_measurement,
    .stop_distance_measurement = cs_stop_distance_measurement,
    .set_config = cs_set_config,
#ifdef CONFIG_BT_CS_RAS_TEST
    .cs_test = cs_test,
#endif /* CONFIG_BT_CS_RAS_TEST */
};

static const void* get_cs_profile_interface(void)
{
    return (void*)&cs_interface;
}

static int cs_dump(void)
{
    return 0;
}

static void cs_process_msg(profile_msg_t* msg)
{
    switch (msg->event) {
    default:
        break;
    }
}

static int cs_get_state(void)
{
    return 1;
}

static const profile_service_t cs_service = {
    .auto_start = true,
    .name = PROFILE_CS_NAME,
    .id = PROFILE_CS,
    .transport = BT_TRANSPORT_BLE,
    .uuid = { BT_UUID128_TYPE, { 0 } },
    .init = cs_init,
    .startup = cs_service_startup,
    .shutdown = cs_service_shutdown,
    .process_msg = cs_process_msg,
    .get_state = cs_get_state,
    .get_profile_interface = get_cs_profile_interface,
    .cleanup = cs_cleanup,
    .dump = cs_dump,
};

void bt_sal_cs_event_callback(cs_msg_t* msg)
{
    do_in_cs_service(msg);
}

void register_cs_service(void)
{
    register_service(&cs_service);
}

void bt_cs_register_subevent_cb(subevent_result_cb_t cb)
{
    result_cb = cb;
    return;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
