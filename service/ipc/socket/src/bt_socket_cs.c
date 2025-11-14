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

#include <sys/socket.h>
#include <sys/un.h>

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
static void on_distance_measure_result_cb(void* cookie, bt_address_t* addr, uint8_t centimeter, uint8_t errorCentimeter,
    uint8_t azimuthAngle, uint8_t errorAzimuthAngle, uint8_t altitudeAngle, uint8_t errorAltitudeAngle,
    init16_t elapsedRealtimeNanos, uint8_t confidenceLevel, uint32_t delaySpreadMeters,
    uint8_t detectedAttackLevel, uint32_t velocityMetersPerSecond, uint8_t method)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.cs_cb._on_distance_measure_result.addr, addr, sizeof(bt_address_t));
    packet.cs_cb._on_distance_measure_result.centimeter = centimeter;
    packet.cs_cb._on_distance_measure_result.errorCentimeter = errorCentimeter;
    packet.cs_cb._on_distance_measure_result.azimuthAngle = azimuthAngle;
    packet.cs_cb._on_distance_measure_result.errorAzimuthAngle = errorAzimuthAngle;
    packet.cs_cb._on_distance_measure_result.altitudeAngle = altitudeAngle;
    packet.cs_cb._on_distance_measure_result.errorAltitudeAngle = errorAltitudeAngle;
    packet.cs_cb._on_distance_measure_result.elapsedRealtimeNanos = elapsedRealtimeNanos;
    packet.cs_cb._on_distance_measure_result.confidenceLevel = confidenceLevel;
    packet.cs_cb._on_distance_measure_result.delaySpreadMeters = delaySpreadMeters;
    packet.cs_cb._on_distance_measure_result.detectedAttackLevel = detectedAttackLevel;
    packet.cs_cb._on_distance_measure_result.velocityMetersPerSecond = velocityMetersPerSecond;
    packet.cs_cb._on_distance_measure_result.method = method;
    bt_socket_server_send(ins, &packet, CS_SUBCODE_ON_DISTANCE_MEASURE_RESULT);
}

const static cs_callbacks_t g_cs_cbs = {
    .size = sizeof(cs_callbacks_t),
    .cs_distance_measure_started_cb = on_distance_measure_started_cb,
    .cs_distance_measure_stopped_cb = on_distance_measure_stopped_cb,
    .cs_distance_measure_result_cb = on_distance_measure_result_cb,
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
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_distance_measure_started_cb,
            &packet->cs_cb._on_distance_measure_started.addr,
            packet->cs_cb._on_distance_measure_started.method);
        break;
    case BT_CS_SUBCODE_ON_DISTANCE_MEASURE_STOPPED:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_distance_measure_stopped_cb,
            &packet->cs_cb._on_distance_measure_stopped.addr,
            packet->cs_cb._on_distance_measure_stopped.reason,
            packet->cs_cb._on_distance_measure_stopped.method);
        break;
    case BT_CS_SUBCODE_ON_DISTANCE_MEASURE_RESULT:
        CALLBACK_FOREACH(CBLIST, cs_callbacks_t, cs_distance_measure_result_cb,
            &packet->cs_cb._on_distance_measure_result.addr,
            packet->cs_cb._on_distance_measure_result.centimeter,
            packet->cs_cb._on_distance_measure_result.errorCentimeter,
            packet->cs_cb._on_distance_measure_result.azimuthAngle,
            packet->cs_cb._on_distance_measure_result.errorAzimuthAngle,
            packet->cs_cb._on_distance_measure_result.altitudeAngle,
            packet->cs_cb._on_distance_measure_result.errorAltitudeAngle,
            packet->cs_cb._on_distance_measure_result.elapsedRealtimeNanos,
            packet->cs_cb._on_distance_measure_result.confidenceLevel,
            packet->cs_cb._on_distance_measure_result.delaySpreadMeters,
            packet->cs_cb._on_distance_measure_result.detectedAttackLevel,
            packet->cs_cb._on_distance_measure_result.velocityMetersPerSecond,
            packet->cs_cb._on_distance_measure_result.method);
        break;
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
