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
#define LOG_TAG "scan"

#include <stdlib.h>

#include "bluetooth.h"
#include "bt_async.h"
#include "bt_debug.h"
#include "bt_le_scan.h"
#include "bt_list.h"
#include "bt_socket.h"
#include "scan_manager.h"
#include "utils/log.h"

typedef struct {
    void* userdata;
    void* scan;
} bt_le_start_scan_data_t;

static void le_scan_bool_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_bool_cb_t ret_cb = (bt_bool_cb_t)cb;

    if (!ret_cb)
        return;

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, 0, userdata);
        return;
    }

    ret_cb(ins, packet->scan_r.status, packet->scan_r.vbool, userdata);
}

static void le_start_scan_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_scan_remote_t* scan;
    bt_le_start_scan_data_t* data = userdata;
    bt_le_start_scan_cb_t ret_cb = (bt_le_start_scan_cb_t)cb;
    bt_status_t status;

    if (!packet) {
        status = BT_STATUS_UNHANDLED;
        goto error;
    }

    scan = (bt_scan_remote_t*)data->scan;
    if (!packet->scan_r.remote) {
        status = BT_STATUS_FAIL;
        goto error;
    }

    scan->remote = packet->scan_r.remote;
    ret_cb(ins, packet->scan_r.status, data->scan, data->userdata);

    free(data);
    return;

error:
    ret_cb(ins, status, NULL, data->userdata);
    free(data->scan);
    free(data);
}

static void le_stop_scan_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_le_stop_scan_cb_t ret_cb = (bt_le_stop_scan_cb_t)cb;

    if (!ret_cb)
        return;

    ret_cb(ins, userdata);
}

bt_status_t bt_le_start_scan_async(bt_instance_t* ins, const scanner_callbacks_t* scan_cbs,
    bt_le_start_scan_cb_t cb, void* userdata)
{
    bt_le_start_scan_data_t* data;
    bt_message_packet_t packet = { 0 };
    bt_status_t status;
    bt_scan_remote_t* scan;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(cb, BT_STATUS_PARM_INVALID);

    scan = malloc(sizeof(*scan));
    if (scan == NULL)
        return BT_STATUS_FAIL;

    scan->ins = ins;
    scan->callbacks = (scanner_callbacks_t*)scan_cbs;
    packet.scan_pl._bt_le_start_scan.remote = PTR2INT(uint64_t) scan;

    data = calloc(1, sizeof(bt_le_start_scan_data_t));
    data->userdata = userdata;
    data->scan = (void*)scan;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_LE_SCAN_START, le_start_scan_reply, cb, data);
    if (status != BT_STATUS_SUCCESS) {
        free(scan);
        free(data);
        return BT_STATUS_FAIL;
    }

    return status;
}

bt_status_t bt_le_start_scan_settings_async(bt_instance_t* ins, ble_scan_settings_t* settings,
    const scanner_callbacks_t* scan_cbs, bt_le_start_scan_cb_t cb, void* userdata)
{
    bt_le_start_scan_data_t* data;
    bt_message_packet_t packet = { 0 };
    bt_status_t status;
    bt_scan_remote_t* scan;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(cb, BT_STATUS_PARM_INVALID);

    scan = malloc(sizeof(*scan));
    if (scan == NULL)
        return BT_STATUS_FAIL;

    scan->ins = ins;
    scan->callbacks = (scanner_callbacks_t*)scan_cbs;
    packet.scan_pl._bt_le_start_scan_settings.remote = PTR2INT(uint64_t) scan;
    if (settings)
        memcpy(&packet.scan_pl._bt_le_start_scan_settings.settings, settings, sizeof(*settings));

    data = calloc(1, sizeof(bt_le_start_scan_data_t));
    data->userdata = userdata;
    data->scan = (void*)scan;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_LE_SCAN_START_SETTINGS, le_start_scan_reply, cb, data);
    if (status != BT_STATUS_SUCCESS) {
        free(scan);
        free(data);
        return BT_STATUS_FAIL;
    }

    return status;
}

bt_status_t bt_le_start_scan_with_filters_async(bt_instance_t* ins, ble_scan_settings_t* settings,
    ble_scan_filter_t* filter, const scanner_callbacks_t* scan_cbs, bt_le_start_scan_cb_t cb, void* userdata)
{
    bt_le_start_scan_data_t* data;
    bt_message_packet_t packet = { 0 };
    bt_status_t status;
    bt_scan_remote_t* scan;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(cb, BT_STATUS_PARM_INVALID);

    scan = zalloc(sizeof(*scan));
    if (scan == NULL)
        return BT_STATUS_FAIL;

    scan->ins = ins;
    scan->callbacks = (scanner_callbacks_t*)scan_cbs;
    packet.scan_pl._bt_le_start_scan_with_filters.remote = PTR2INT(uint64_t) scan;
    if (settings)
        memcpy(&packet.scan_pl._bt_le_start_scan_with_filters.settings, settings, sizeof(*settings));

    if (filter) {
        memcpy(&packet.scan_pl._bt_le_start_scan_with_filters.filter, filter, sizeof(*filter));
    }

    data = calloc(1, sizeof(bt_le_start_scan_data_t));
    data->userdata = userdata;
    data->scan = (void*)scan;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_LE_SCAN_START_WITH_FILTERS, le_start_scan_reply, cb, data);
    if (status != BT_STATUS_SUCCESS) {
        free(scan);
        free(data);
        return BT_STATUS_FAIL;
    }

    return status;
}

bt_status_t bt_le_stop_scan_async(bt_instance_t* ins, bt_scanner_t* scanner, bt_le_stop_scan_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (!scanner)
        return BT_STATUS_FAIL;

    packet.scan_pl._bt_le_stop_scan.remote = ((bt_scan_remote_t*)scanner)->remote;

    return bt_socket_client_send_with_reply(ins, &packet, BT_LE_SCAN_STOP, le_stop_scan_reply, (void*)cb, userdata);
}

bt_status_t bt_le_scan_is_supported_async(bt_instance_t* ins, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_LE_SCAN_IS_SUPPORT, le_scan_bool_reply, (void*)cb, userdata);
}
