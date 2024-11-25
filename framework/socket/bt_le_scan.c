/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#define LOG_TAG "adv"

#include <stdlib.h>

#include "bluetooth.h"
#include "bt_debug.h"
#include "bt_le_scan.h"
#include "bt_list.h"
#include "bt_socket.h"
#include "scan_manager.h"
#include "utils/log.h"

bt_scanner_t* bt_le_start_scan(bt_instance_t* ins, const scanner_callbacks_t* cbs)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, NULL);

    bt_scan_remote_t* scan = malloc(sizeof(*scan));
    if (scan == NULL)
        return NULL;

    scan->callback = (scanner_callbacks_t*)cbs;
    packet.scan_pl._bt_le_start_scan.remote = PTR2INT(uint64_t) scan;

    status = bt_socket_client_sendrecv(ins, &packet, BT_LE_SCAN_START);
    if (status != BT_STATUS_SUCCESS || !packet.scan_r.remote) {
        free(scan);
        return NULL;
    }

    scan->remote = packet.scan_r.remote;
    return scan;
}

bt_scanner_t* bt_le_start_scan_settings(bt_instance_t* ins,
    ble_scan_settings_t* settings,
    const scanner_callbacks_t* cbs)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, NULL);

    bt_scan_remote_t* scan = malloc(sizeof(*scan));
    if (scan == NULL)
        return NULL;

    scan->callback = (scanner_callbacks_t*)cbs;
    packet.scan_pl._bt_le_start_scan_settings.remote = PTR2INT(uint64_t) scan;
    if (settings)
        memcpy(&packet.scan_pl._bt_le_start_scan_settings.settings, settings, sizeof(*settings));

    status = bt_socket_client_sendrecv(ins, &packet, BT_LE_SCAN_START_SETTINGS);
    if (status != BT_STATUS_SUCCESS || !packet.scan_r.remote) {
        free(scan);
        return NULL;
    }

    scan->remote = packet.scan_r.remote;
    return scan;
}

bt_scanner_t* bt_le_start_scan_with_filters(bt_instance_t* ins,
    ble_scan_settings_t* settings,
    ble_scan_filter_t* filter,
    const scanner_callbacks_t* cbs)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, NULL);

    bt_scan_remote_t* scan = zalloc(sizeof(*scan));
    if (scan == NULL)
        return NULL;

    scan->callback = (scanner_callbacks_t*)cbs;
    packet.scan_pl._bt_le_start_scan_with_filters.remote = PTR2INT(uint64_t) scan;
    if (settings)
        memcpy(&packet.scan_pl._bt_le_start_scan_with_filters.settings, settings, sizeof(*settings));

    if (filter) {
        memcpy(&packet.scan_pl._bt_le_start_scan_with_filters.filter, filter, sizeof(*filter));
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_LE_SCAN_START_WITH_FILTERS);
    if (status != BT_STATUS_SUCCESS || !packet.scan_r.remote) {
        free(scan);
        return NULL;
    }

    scan->remote = packet.scan_r.remote;
    return scan;
}

void bt_le_stop_scan(bt_instance_t* ins, bt_scanner_t* scanner)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );

    if (!scanner)
        return;

    packet.scan_pl._bt_le_stop_scan.remote = ((bt_scan_remote_t*)scanner)->remote;
    (void)bt_socket_client_sendrecv(ins, &packet, BT_LE_SCAN_STOP);
}

bool bt_le_scan_is_supported(bt_instance_t* ins)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, NULL);

    status = bt_socket_client_sendrecv(ins, &packet, BT_LE_SCAN_IS_SUPPORT);
    if (status != BT_STATUS_SUCCESS || !packet.scan_r.vbool) {
        return false;
    }

    return true;
}
