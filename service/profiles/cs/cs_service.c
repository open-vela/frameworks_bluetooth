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

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_cs.h"
#include "bt_list.h"
#include "callbacks_list.h"
#include "cs_msg.h"
#include "cs_ras_server.h"
#include "cs_ras_test.h"
#include "cs_service.h"
#include "cs_state_machine.h"
#include "gatts_service.h"
#include "power_manager.h"
#include "service_loop.h"
#include "service_manager.h"
#include "utils/log.h"

#ifdef CONFIG_BLUETOOTH_LE_CS

#define CS_CALLBACK_FOREACH(_list, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, cs_callbacks_t, _cback, ##__VA_ARGS__)

static gatts_handle_t g_cs_handle = NULL;
typedef struct {
    struct list_node list;
    callbacks_list_t* callbacks;
} cs_servie_t;

static cs_servie_t g_cs_service = { 0 };

static void service_startup(profile_on_startup_t cb);
static void service_shutdown(profile_on_shutdown_t cb);

static cs_device_t* cs_device_new(void* ctx, bt_address_t* bd_addr)
{
    cs_device_t* device;
    cs_state_machine_t* cs_sm;

    device = (cs_device_t*)malloc(sizeof(cs_device_t));
    if (!device)
        return NULL;

    memcpy(&device->bd_addr, bd_addr, sizeof(bt_address_t));
    cs_sm = cs_state_machine_new(ctx, bd_addr);
    if (!cs_sm) {
        BT_LOGE("Create state machine failed");
        free(device);
        return NULL;
    }

    device->cs_sm = cs_sm;

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
    default: {
        cs_state_machine_t* cs_sm;
        cs_sm = get_state_machine(&msg->cs_data.bd_addr);
        if (!cs_sm) {
            break;
        }

        cs_state_machine_handle_event(cs_sm, msg);
        break;
    } break;
    }

    cs_msg_destory(msg);
}

static void do_in_cs_service(cs_msg_t* msg)
{
    if (msg == NULL)
        return;

    do_in_service_loop(cs_service_handle_event, msg);
}

// static void connect_callback(void* srv_handle, bt_address_t* addr)
// {
//     cs_msg_t* msg = cs_msg_new(CONNECTED_EVT, NULL);
//     msg->data.data = addr;
//     do_in_cs_service(msg);

//     return BT_STATUS_SUCCESS;
//     BT_LOGD("gatts_connect_callback, addr:%s", addr);
// }

// static void disconnect_callback(void* srv_handle, bt_address_t* addr)
// {
//     BT_LOGD("gatts_disconnect_callback, addr:%s", addr);
// }

// static void attr_table_added_callback(void* srv_handle, gatt_status_t status, uint16_t attr_handle)
// {
//     BT_LOGD("gatts add attribute table complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
// }

// static void attr_table_removed_callback(void* srv_handle, gatt_status_t status, uint16_t attr_handle)
// {
//     BT_LOGD("gatts remove attribute table complete, handle 0x%" PRIx16 ", status:%d", attr_handle, status);
// }

// static void notify_complete_callback(void* srv_handle, bt_address_t* addr, gatt_status_t status, uint16_t attr_handle)
// {
//     if (status != GATT_STATUS_SUCCESS) {
//         BT_LOGD("gatts service notify failed, addr:%s, handle 0x%" PRIx16 ", status:%d", addr, attr_handle, status);
//         return;
//     }
// }

// static void mtu_changed_callback(void* srv_handle, bt_address_t* addr, uint32_t mtu)
// {
//     BT_LOGD("gatts_mtu_changed_callback, addr:%s, mtu:%" PRIu32, addr, mtu);
// }

// static void phy_read_callback(void* srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
// {
//     BT_LOGD("gatts read phy complete, addr:%s, tx:%d, rx:%d", addr, tx_phy, rx_phy);
// }

// static void phy_updated_callback(void* srv_handle, bt_address_t* addr, gatt_status_t status, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
// {
//     BT_LOGD("gatts phy updated, addr:%s, status:%d, tx:%d, rx:%d", addr, status, tx_phy, rx_phy);
// }

// static void conn_param_changed_callback(void* srv_handle, bt_address_t* addr, uint16_t connection_interval,
//     uint16_t peripheral_latency, uint16_t supervision_timeout)
// {
//     BT_LOGD("gatts_conn_param_changed_callback, addr:%s, interval:%" PRIu16 ", latency:%" PRIu16 ", timeout:%" PRIu16,
//         addr, connection_interval, peripheral_latency, supervision_timeout);
// }

// static gatts_callbacks_t gatts_cbs = {
//     sizeof(gatts_cbs),
//     connect_callback,
//     disconnect_callback,
//     attr_table_added_callback,
//     attr_table_removed_callback,
//     notify_complete_callback,
//     mtu_changed_callback,
//     phy_read_callback,
//     phy_updated_callback,
//     conn_param_changed_callback,
// };

static bt_status_t cs_init(void)
{
    g_cs_service.callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

    if (g_cs_handle) {
        BT_LOGD("has registed, please unregister then try again");
        return BT_STATUS_FAIL;
    }

    list_initialize(&g_cs_service.list);

    return BT_STATUS_SUCCESS;
}

static void cs_cleanup(void)
{
    bt_callbacks_list_free(g_cs_service.callbacks);
    g_cs_service.callbacks = NULL;
}

static void service_startup(profile_on_startup_t cb)
{
    le_cs_enable();
    cb(PROFILE_CS, true);
}

static void service_shutdown(profile_on_shutdown_t cb)
{
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
        msg->cs_data.data = params;
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

static bt_status_t cs_test(void* data, uint16_t len)
{
    int err = cs_ras_subevent_recv_test(data, len);
    return (err == 0) ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;
}

static cs_state_t cs_get_measure_state(bt_address_t* bd_addr)
{
    cs_device_t* device = find_cs_device_by_addr(&g_cs_service.list, bd_addr);
    cs_stm_state_t state;

    if (!device)
        return CS_STOPPED;

    state = cs_state_machine_get_state(device->cs_sm);
    if (state == CS_STATE_STARTED)
        return CS_STARTED;
    else
        return CS_STOPPED;
}

static const bt_cs_interface_t cs_interface = {
    .size = sizeof(cs_interface),
    .register_callbacks = cs_register_callbacks,
    .unregister_callbacks = cs_unregister_callbacks,
    .start_distance_measurement = cs_start_distance_measurement,
    .stop_distance_measurement = cs_stop_distance_measurement,
    .cs_test = cs_test,
    .get_state = cs_get_measure_state,
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

void bt_register_cs_service(void)
{
    register_service(&cs_service);
}

void cs_service_notify_started_cb(bt_address_t* addr, uint8_t method)
{
    BT_LOGD("%s", __FUNCTION__);
    CS_CALLBACK_FOREACH(g_cs_service.callbacks, cs_distance_measure_started_cb, addr, method);
}

void cs_service_notify_stopped_cb(bt_address_t* addr, uint8_t reason, uint8_t method)
{
    BT_LOGD("%s", __FUNCTION__);
    CS_CALLBACK_FOREACH(g_cs_service.callbacks, cs_distance_measure_stopped_cb, addr, reason, method);
}

void cs_service_notify_result_cb(bt_address_t* addr, uint8_t centimeter, uint8_t errorCentimeter,
    uint8_t azimuthAngle, uint8_t errorAzimuthAngle, uint8_t altitudeAngle, uint8_t errorAltitudeAngle,
    uint16_t elapsedRealtimeNanos, uint8_t confidenceLevel, uint32_t delaySpreadMeters,
    uint8_t detectedAttackLevel, uint32_t velocityMetersPerSecond, uint8_t method)
{
    BT_LOGD("%s", __FUNCTION__);
    CS_CALLBACK_FOREACH(g_cs_service.callbacks, cs_distance_measure_result_cb, addr, centimeter, errorCentimeter,
        azimuthAngle, errorAzimuthAngle, altitudeAngle, errorAltitudeAngle, elapsedRealtimeNanos, confidenceLevel,
        delaySpreadMeters, detectedAttackLevel, velocityMetersPerSecond, method);
}
#endif /* CONFIG_BLUETOOTH_LE_CS */
