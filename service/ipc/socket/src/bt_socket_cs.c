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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/un.h>

#include "bt_internal.h"

#include "bluetooth.h"
#include "bt_cs.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "cs_service.h"
#include "service_manager.h"

#ifdef CONFIG_BLUETOOTH_LE_CS
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (__async ? __async->cs_callbacks : ins->cs_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)

static void on_distance_measure_started_cb(void* cookie, bt_address_t* addr, uint8_t method)
{
    bt_instance_t* ins = (bt_instance_t*)cookie;
    bt_message_packet_t packet = { 0 };
    memcpy(&packet.cs_cb._on_distance_measure_started.addr, addr, sizeof(bt_address_t));
    packet.cs_cb._on_distance_measure_started.method = method;
    bt_socket_server_send(ins, &packet, BT_CS_ON_DISTANCE_MEASURE_STARTED);
}

static void on_distance_measure_stopped_cb(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method)
{
    bt_instance_t* ins = (bt_instance_t*)cookie;
    bt_message_packet_t packet = { 0 };
    memcpy(&packet.cs_cb._on_distance_measure_stopped.addr, addr, sizeof(bt_address_t));
    packet.cs_cb._on_distance_measure_stopped.reason = reason;
    packet.cs_cb._on_distance_measure_stopped.method = method;
    bt_socket_server_send(ins, &packet, BT_CS_ON_DISTANCE_MEASURE_STOPPED);
}

static void on_distance_measure_result_cb(void* cookie, bt_address_t* addr, bt_distance_measurement_result_t* result)
{
    bt_instance_t* ins = (bt_instance_t*)cookie;
    bt_message_packet_t packet = { 0 };
    memcpy(&packet.cs_cb._on_distance_measure_result.addr, addr, sizeof(bt_address_t));
    memcpy(&packet.cs_cb._on_distance_measure_result.result, result, sizeof(bt_distance_measurement_result_t));
    bt_socket_server_send(ins, &packet, BT_CS_ON_DISTANCE_MEASURE_RESULT);
}

static void on_rap_distance_result_cb(void* cookie, bt_address_t* addr, cs_rap_distance_result_t* result)
{
    bt_instance_t* ins = (bt_instance_t*)cookie;
    bt_message_packet_t packet = { 0 };
    memcpy(&packet.cs_cb._on_rap_distance_result.addr, addr, sizeof(bt_address_t));
    memcpy(&packet.cs_cb._on_rap_distance_result.result, result, sizeof(cs_rap_distance_result_t));
    bt_socket_server_send(ins, &packet, BT_CS_ON_RAP_DISTANCE_RESULT);
}

const static cs_callbacks_t g_cs_cbs = {
    .size = sizeof(cs_callbacks_t),
    .cs_distance_measure_started_cb = on_distance_measure_started_cb,
    .cs_distance_measure_stopped_cb = on_distance_measure_stopped_cb,
    .cs_distance_measure_result_cb = on_distance_measure_result_cb,
    .rap_distance_result_cb = on_rap_distance_result_cb,
};
/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_cs_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    bt_cs_interface_t* profile;

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case CS_SUBCODE_REGISTER_CALLBACKS:
        if (ins->cs_cookie == NULL) {
            profile = (bt_cs_interface_t*)service_manager_get_profile(PROFILE_CS);
            if (profile) {
                ins->cs_cookie = profile->register_callbacks(ins, &g_cs_cbs);
                if (ins->cs_cookie) {
                    packet->cs_r.status = BT_STATUS_SUCCESS;
                } else {
                    packet->cs_r.status = BT_STATUS_NO_RESOURCES;
                }
            } else {
                packet->cs_r.status = BT_STATUS_SERVICE_NOT_FOUND;
            }
        } else {
            packet->cs_r.status = BT_STATUS_BUSY;
        }
        break;
    case CS_SUBCODE_UNREGISTER_CALLBACKS:
        if (ins->cs_cookie) {
            profile = (bt_cs_interface_t*)service_manager_get_profile(PROFILE_CS);
            if (profile)
                profile->unregister_callbacks((void**)&ins, ins->cs_cookie);
            ins->cs_cookie = NULL;
            packet->cs_r.status = BT_STATUS_SUCCESS;
        } else {
            packet->cs_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    case CS_SUBCODE_START_DISTANCE_MEASUREMENT:
        packet->cs_r.status = BTSYMBOLS(bt_cs_start_distance_measurement)(ins,
            &packet->cs_pl._bt_cs_start_distance_measurement.params);
        break;
    case CS_SUBCODE_STOP_DISTANCE_MEASUREMENT:
        packet->cs_r.status = BTSYMBOLS(bt_cs_stop_distance_measurement)(ins,
            &packet->cs_pl._bt_cs_stop_distance_measurement.addr,
            packet->cs_pl._bt_cs_stop_distance_measurement.method,
            packet->cs_pl._bt_cs_stop_distance_measurement.timeout_bool);
        break;
    case CS_SUBCODE_SET_CONFIG:
        packet->cs_r.status = BTSYMBOLS(bt_cs_set_config)(ins,
            &packet->cs_pl._bt_cs_set_config.addr,
            &packet->cs_pl._bt_cs_set_config.params);
        break;
#ifdef CONFIG_BT_CS_RAS_TEST
    case CS_SUBCODE_TEST:
        packet->cs_r.status = BTSYMBOLS(bt_cs_test)(ins,
            &packet->cs_pl._bt_cs_test.data,
            packet->cs_pl._bt_cs_test.len);
        break;
#endif /* CONFIG_BT_CS_RAS_TEST */
    default:
        break;
    }
}

#endif

int bt_socket_client_cs_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    bt_socket_async_client_t* __async = NULL;

    if (is_async) {
        __async = ins->priv;
    }

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case CS_CB_SUBCODE_DISTANCE_MEASURE_STARTED:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_distance_measure_started_cb,
            &packet->cs_cb._on_distance_measure_started.addr,
            packet->cs_cb._on_distance_measure_started.method);
        break;
    case CS_CB_SUBCODE_DISTANCE_MEASURE_STOPPED:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_distance_measure_stopped_cb,
            &packet->cs_cb._on_distance_measure_stopped.addr,
            packet->cs_cb._on_distance_measure_stopped.reason,
            packet->cs_cb._on_distance_measure_stopped.method);
        break;
    case CS_CB_SUBCODE_DISTANCE_MEASURE_RESULT:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_distance_measure_result_cb,
            &packet->cs_cb._on_distance_measure_result.addr,
            &packet->cs_cb._on_distance_measure_result.result);
        break;
    case CS_CB_SUBCODE_RAP_DISTANCE_RESULT:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, rap_distance_result_cb,
            &packet->cs_cb._on_rap_distance_result.addr,
            &packet->cs_cb._on_rap_distance_result.result);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
