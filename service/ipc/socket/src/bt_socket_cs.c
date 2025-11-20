/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "bluetooth.h"
#include "bt_internal.h"
#include "bt_message.h"
#include "bt_socket.h"

#include "bt_cs.h"
#include "cs_service.h"
#include "service_manager.h"
#include <sys/socket.h>
#include <sys/un.h>

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
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.cs_cb._on_distance_measure_started.addr, addr, sizeof(bt_address_t));
    packet.cs_cb._on_distance_measure_started.method = method;
    bt_socket_server_send(ins, &packet, BT_CS_SUBCODE_ON_DISTANCE_MEASURE_STARTED);
}

static void on_distance_measure_stopped_cb(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.cs_cb._on_distance_measure_stopped.addr, addr, sizeof(bt_address_t));
    packet.cs_cb._on_distance_measure_stopped.reason = reason;
    packet.cs_cb._on_distance_measure_stopped.method = method;
    bt_socket_server_send(ins, &packet, CS_SUBCODE_ON_DISTANCE_MEASURE_STOPPED);
}

static void on_distance_measure_result_cb(void* cookie, bt_address_t* addr, uint8_t centimeter, uint8_t error_centimeter,
    uint8_t azimuth_angle, uint8_t errorazimuth_angle, uint8_t altitude_angle, uint8_t erroraltitude_angle,
    uint16_t elapsed_realtime_nanos, uint8_t confidence_level, uint32_t delay_spread_meters,
    uint8_t detected_attack_level, uint32_t velocity_meters_per_second, uint8_t method)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.cs_cb._on_distance_measure_result.addr, addr, sizeof(bt_address_t));
    packet.cs_cb._on_distance_measure_result.centimeter = centimeter;
    packet.cs_cb._on_distance_measure_result.error_centimeter = error_centimeter;
    packet.cs_cb._on_distance_measure_result.azimuth_angle = azimuth_angle;
    packet.cs_cb._on_distance_measure_result.errorazimuth_angle = errorazimuth_angle;
    packet.cs_cb._on_distance_measure_result.altitude_angle = altitude_angle;
    packet.cs_cb._on_distance_measure_result.erroraltitude_angle = erroraltitude_angle;
    packet.cs_cb._on_distance_measure_result.elapsed_realtime_nanos = elapsed_realtime_nanos;
    packet.cs_cb._on_distance_measure_result.confidence_level = confidence_level;
    packet.cs_cb._on_distance_measure_result.delay_spread_meters = delay_spread_meters;
    packet.cs_cb._on_distance_measure_result.detected_attack_level = detected_attack_level;
    packet.cs_cb._on_distance_measure_result.velocity_meters_per_second = velocity_meters_per_second;
    packet.cs_cb._on_distance_measure_result.method = method;
    bt_socket_server_send(ins, &packet, CS_SUBCODE_ON_DISTANCE_MEASURE_RESULT);
}

const static cs_callbacks_t g_cs_cbs = {
    .size = sizeof(cs_callbacks_t),
    .cs_started_cb = on_distance_measure_started_cb,
    .cs_stopped_cb = on_distance_measure_stopped_cb,
    .cs_result_cb = on_distance_measure_result_cb,
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
    case CS_SUBCODE_TEST:
        packet->cs_r.status = BTSYMBOLS(bt_cs_test)(ins,
            &packet->cs_pl._bt_cs_test.data,
            packet->cs_pl._bt_cs_test.len);
        break;
    case CS_SUBCODE_GET_STATE:
        packet->cs_r.measure_state = BTSYMBOLS(bt_cs_get_state)(ins,
            &packet->cs_pl._bt_cs_get_state.addr);
        break;
    default:
        break;
    }
}

#endif

int bt_socket_client_cs_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    bt_socket_async_client_t* __async = NULL;

    if (is_async)
        __async = ins->priv;

    switch (packet->code) {
    case BT_CS_SUBCODE_ON_DISTANCE_MEASURE_STARTED:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_started_cb,
            &packet->cs_cb._on_distance_measure_started.addr,
            packet->cs_cb._on_distance_measure_started.method);
        break;
    case BT_CS_SUBCODE_ON_DISTANCE_MEASURE_STOPPED:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_stopped_cb,
            &packet->cs_cb._on_distance_measure_stopped.addr,
            packet->cs_cb._on_distance_measure_stopped.reason,
            packet->cs_cb._on_distance_measure_stopped.method);
        break;
    case BT_CS_SUBCODE_ON_DISTANCE_MEASURE_RESULT:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_result_cb,
            &packet->cs_cb._on_distance_measure_result.addr,
            packet->cs_cb._on_distance_measure_result.centimeter,
            packet->cs_cb._on_distance_measure_result.error_centimeter,
            packet->cs_cb._on_distance_measure_result.azimuth_angle,
            packet->cs_cb._on_distance_measure_result.errorazimuth_angle,
            packet->cs_cb._on_distance_measure_result.altitude_angle,
            packet->cs_cb._on_distance_measure_result.erroraltitude_angle,
            packet->cs_cb._on_distance_measure_result.elapsed_realtime_nanos,
            packet->cs_cb._on_distance_measure_result.confidence_level,
            packet->cs_cb._on_distance_measure_result.delay_spread_meters,
            packet->cs_cb._on_distance_measure_result.detected_attack_level,
            packet->cs_cb._on_distance_measure_result.velocity_meters_per_second,
            packet->cs_cb._on_distance_measure_result.method);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
