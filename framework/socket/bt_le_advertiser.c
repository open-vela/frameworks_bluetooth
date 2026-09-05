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

#include "advertising.h"
#include "bluetooth.h"
#include "bt_le_advertiser.h"
#include "bt_list.h"
#include "bt_socket.h"
#include "utils/log.h"

bt_advertiser_t* bt_le_start_advertising(bt_instance_t* ins,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    advertiser_callback_t* cbs)
{
    bt_message_packet_t packet;
    bt_status_t status;
    bt_advertiser_remote_t* adv;

    BT_SOCKET_INS_VALID(ins, NULL);

    adv = malloc(sizeof(*adv));
    if (adv == NULL)
        return NULL;

    adv->callback = cbs;
    packet.adv_pl._bt_le_start_advertising.adver = PTR2INT(uint64_t) adv;
    memcpy(&packet.adv_pl._bt_le_start_advertising.params, params, sizeof(*params));
    if ((adv_len && (adv_len > sizeof(packet.adv_pl._bt_le_start_advertising.adv_data)))
        || (scan_rsp_len && (scan_rsp_len > sizeof(packet.adv_pl._bt_le_start_advertising.scan_rsp_data)))) {
        free(adv);
        return NULL;
    }
    if (adv_len)
        memcpy(packet.adv_pl._bt_le_start_advertising.adv_data, adv_data, adv_len);
    packet.adv_pl._bt_le_start_advertising.adv_len = adv_len;

    if (scan_rsp_len)
        memcpy(packet.adv_pl._bt_le_start_advertising.scan_rsp_data, scan_rsp_data, scan_rsp_len);
    packet.adv_pl._bt_le_start_advertising.scan_rsp_len = scan_rsp_len;

    status = bt_socket_client_sendrecv(ins, &packet, BT_LE_START_ADVERTISING);
    if (status != BT_STATUS_SUCCESS || !packet.adv_r.remote) {
        free(adv);
        return NULL;
    }

    adv->remote = packet.adv_r.remote;
    return adv;
}

void bt_le_stop_advertising(bt_instance_t* ins, bt_advertiser_t* adver)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );

    if (!adver)
        return;

    packet.adv_pl._bt_le_stop_advertising.adver = (uint32_t)((bt_advertiser_remote_t*)adver)->remote;
    (void)bt_socket_client_sendrecv(ins, &packet, BT_LE_STOP_ADVERTISING);
}

void bt_le_stop_advertising_id(bt_instance_t* ins, uint8_t adv_id)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );

    packet.adv_pl._bt_le_stop_advertising_id.id = adv_id;
    (void)bt_socket_client_sendrecv(ins, &packet, BT_LE_STOP_ADVERTISING_ID);
}

bool bt_le_advertising_is_supported(bt_instance_t* ins)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);

    status = bt_socket_client_sendrecv(ins, &packet, BT_LE_ADVERTISING_IS_SUPPORT);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    return packet.adv_r.vbool;
}
