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
}

static void on_distance_measure_stopped_cb(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method)
{
}
static void on_distance_measure_result_cb(void* cookie, bt_address_t* addr, bt_distance_measurement_result_t* result)
{
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
    switch (packet->code) {
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */
