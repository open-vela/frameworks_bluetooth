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
#define LOG_TAG "adv"

#include <stdlib.h>

#include "advertising.h"
#include "bluetooth.h"
#include "bt_async.h"
#include "bt_le_advertiser.h"
#include "bt_list.h"
#include "bt_socket.h"
#include "utils/log.h"

typedef struct {
    void* userdata;
    void* adv;
} bt_le_start_advertising_data_t;

static void le_advertiser_status_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* context)
{
    bt_status_cb_t ret_cb = (bt_status_cb_t)cb;

    if (!ret_cb)
        return;

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, context);
        return;
    }

    ret_cb(ins, packet->adv_r.status, context);
}

static void le_advertiser_bool_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_bool_cb_t ret_cb = (bt_bool_cb_t)cb;

    if (!ret_cb)
        return;

    if (!packet) {
        ret_cb(ins, BT_STATUS_UNHANDLED, 0, userdata);
        return;
    }

    ret_cb(ins, packet->adv_r.status, packet->adv_r.vbool, userdata);
}

static void le_start_advertising_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_advertiser_remote_t* adv;
    bt_status_t status;
    bt_le_start_advertising_data_t* data = userdata;
    bt_le_start_adv_callback_cb_t ret_cb = (bt_le_start_adv_callback_cb_t)cb;

    adv = (bt_advertiser_remote_t*)data->adv;

    if (!packet) {
        status = BT_STATUS_UNHANDLED;
        goto error;
    }

    if (!packet->adv_r.remote) {
        status = packet->adv_r.status;
        goto error;
    } else {
        adv->remote = packet->adv_r.remote;
    }

    ret_cb(ins, packet->adv_r.status, data->adv, data->userdata);
    free(data);
    return;

error:
    data->adv = NULL;
    free(adv);
    ret_cb(ins, status, data->adv, data->userdata);
    free(data);
}

bt_status_t bt_le_start_advertising_async(bt_instance_t* ins, ble_adv_params_t* params, uint8_t* adv_data,
    uint16_t adv_len, uint8_t* scan_rsp_data, uint16_t scan_rsp_len,
    advertiser_callback_t* adv_cbs, bt_le_start_adv_callback_cb_t cb, void* userdata)
{
    bt_le_start_advertising_data_t* context;
    bt_message_packet_t packet = { 0 };
    bt_status_t status;
    bt_advertiser_remote_t* adv;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(cb, BT_STATUS_PARM_INVALID);

    adv = malloc(sizeof(*adv));
    if (adv == NULL)
        return BT_STATUS_NOMEM;

    adv->ins = ins;
    adv->callbacks = adv_cbs;
    packet.adv_pl._bt_le_start_advertising.adver = PTR2INT(uint64_t) adv;
    memcpy(&packet.adv_pl._bt_le_start_advertising.params, params, sizeof(*params));
    if ((adv_len && (adv_len > sizeof(packet.adv_pl._bt_le_start_advertising.adv_data)))
        || (scan_rsp_len && (scan_rsp_len > sizeof(packet.adv_pl._bt_le_start_advertising.scan_rsp_data)))) {
        free(adv);
        return BT_STATUS_FAIL;
    }

    if (adv_len)
        memcpy(packet.adv_pl._bt_le_start_advertising.adv_data, adv_data, adv_len);
    packet.adv_pl._bt_le_start_advertising.adv_len = adv_len;

    if (scan_rsp_len)
        memcpy(packet.adv_pl._bt_le_start_advertising.scan_rsp_data, scan_rsp_data, scan_rsp_len);
    packet.adv_pl._bt_le_start_advertising.scan_rsp_len = scan_rsp_len;

    context = calloc(1, sizeof(bt_le_start_advertising_data_t));
    context->userdata = userdata;
    context->adv = (void*)adv;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_LE_START_ADVERTISING, le_start_advertising_reply, cb, context);

    if (status != BT_STATUS_SUCCESS) {
        free(adv);
        free(context);
        return BT_STATUS_FAIL;
    }

    return status;
}

bt_status_t bt_le_stop_advertising_async(bt_instance_t* ins, bt_advertiser_t* adver, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    BT_SOCKET_PTR_VALID(adver, BT_STATUS_FAIL);

    packet.adv_pl._bt_le_stop_advertising.adver = (uint32_t)((bt_advertiser_remote_t*)adver)->remote;

    return bt_socket_client_send_with_reply(ins, &packet, BT_LE_STOP_ADVERTISING, le_advertiser_status_reply, (void*)cb, userdata);
}

bt_status_t bt_le_stop_advertising_id_async(bt_instance_t* ins, uint8_t adv_id, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adv_pl._bt_le_stop_advertising_id.id = adv_id;

    return bt_socket_client_send_with_reply(ins, &packet, BT_LE_STOP_ADVERTISING_ID, le_advertiser_status_reply, (void*)cb, userdata);
}

bt_status_t bt_le_advertising_is_supported_async(bt_instance_t* ins, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_LE_ADVERTISING_IS_SUPPORT, le_advertiser_bool_reply, (void*)cb, userdata);
}
