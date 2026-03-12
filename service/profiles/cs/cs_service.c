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

#define CS_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, cs_callbacks_t, _cback, ##__VA_ARGS__)

typedef void (*subevent_result_cb_t)(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result);

typedef struct {
    struct list_node list;
    callbacks_list_t* callbacks;
} cs_service_t;

static cs_service_t g_cs_service = { 0 };
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

static bt_status_t cs_subevent_result_callbacks(bt_address_t* addr, void* data)
{
    if (result_cb == NULL) {
        BT_LOGW("The subevent result callbacks haven't been registered.");
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
        cs_sm = get_state_machine(&msg->cs_data.bd_addr);
        if (!cs_sm) {
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
    bt_cs_ras_enable();
    cb(PROFILE_CS, true);
}

static void service_shutdown(profile_on_shutdown_t cb)
{
    bt_cs_ras_disable();
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
    BT_LOGD("cs_start_distance_measurement");
    switch (params->method) {
    case METHOD_AUTO:
    case METHOD_RSSI:
        BT_LOGD("not supported method");
        break;
    case METHOD_CS: {
        cs_msg_t* msg = cs_msg_new(START_REQ, &params->addr);
        bt_distance_measurement_params_t* cs_params = (bt_distance_measurement_params_t*)zalloc(sizeof(bt_distance_measurement_params_t));

        if (!cs_params) {
            BT_LOGE("malloc failed");
            return BT_STATUS_FAIL;
        }

        memcpy(cs_params, params, sizeof(bt_distance_measurement_params_t));
        msg->cs_data.data = cs_params;
        do_in_cs_service(msg);
        break;
    }

    default:
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
    BT_LOGD("cs_set_config: addr=%s, ras_feature=0x%08" PRIx32 ", role=0x%02x, antenna=0x%02x, max_tx_power=%d",
        bt_addr_str(addr), params->ras_feature, params->role,
        params->cs_sync_antenna_selection, params->max_tx_power);

    bt_status_t ret = bt_cs_ras_set_feature(params->ras_feature);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    ret = bt_cs_ras_set_role(params->role);
    if (ret != BT_STATUS_SUCCESS) {
        return ret;
    }

    cs_update_default_settings(params);
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
