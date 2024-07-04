/****************************************************************************
 * frameworks/media/media_daemon.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

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

#include "bt_internal.h"

#include "advertising.h"
#include "bluetooth.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__) && defined(CONFIG_BLUETOOTH_BLE_ADV)

static void on_advertising_start_cb(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status)
{
    bt_advertiser_remote_t* adver = adv;
    bt_message_packet_t packet = { 0 };

    packet.adv_cb._on_advertising_start.adver = adver->remote;
    packet.adv_cb._on_advertising_start.adv_id = adv_id;
    packet.adv_cb._on_advertising_start.status = status;

    bt_socket_server_send(adver->ins, &packet, BT_LE_ON_ADVERTISER_START);
    if (status != 0)
        free(adver);
}

static void on_advertising_stopped_cb(bt_advertiser_t* adv, uint8_t adv_id)
{
    bt_advertiser_remote_t* adver = adv;
    bt_message_packet_t packet = { 0 };

    packet.adv_cb._on_advertising_stopped.adver = adver->remote;
    packet.adv_cb._on_advertising_stopped.adv_id = adv_id;

    bt_socket_server_send(adver->ins, &packet, BT_LE_ON_ADVERTISER_STOPPED);
    free(adv);
}

static advertiser_callback_t g_advertiser_socket_cb = {
    sizeof(g_advertiser_socket_cb),
    on_advertising_start_cb,
    on_advertising_stopped_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_advertiser_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_LE_START_ADVERTISING: {
        bt_advertiser_remote_t* adver = malloc(sizeof(*adver));
        adver->ins = ins;
        adver->remote = packet->adv_pl._bt_le_start_advertising.adver;
        packet->adv_r.remote = PTR2INT(uint64_t) start_advertising((void*)adver,
            &packet->adv_pl._bt_le_start_advertising.params,
            packet->adv_pl._bt_le_start_advertising.adv_data,
            packet->adv_pl._bt_le_start_advertising.adv_len,
            packet->adv_pl._bt_le_start_advertising.scan_rsp_data,
            packet->adv_pl._bt_le_start_advertising.scan_rsp_len,
            &g_advertiser_socket_cb);

        if (!packet->adv_r.remote)
            free(adver);

        break;
    }
    case BT_LE_STOP_ADVERTISING: {
        stop_advertising(INT2PTR(bt_advertiser_t*) packet->adv_pl._bt_le_stop_advertising.adver);
        break;
    }
    case BT_LE_STOP_ADVERTISING_ID: {
        stop_advertising_id(packet->adv_pl._bt_le_stop_advertising_id.id);
        break;
    }
    case BT_LE_ADVERTISING_IS_SUPPORT: {
        packet->adv_r.vbool = advertising_is_supported();
        break;
    }
    default:
        break;
    }
}
#endif

int bt_socket_client_advertiser_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_LE_ON_ADVERTISER_START: {
        bt_advertiser_remote_t* adver = INT2PTR(bt_advertiser_remote_t*) packet->adv_cb._on_advertising_start.adver;

        adver->callback->on_advertising_start(adver,
            packet->adv_cb._on_advertising_start.adv_id,
            packet->adv_cb._on_advertising_start.status);
        break;
    }
    case BT_LE_ON_ADVERTISER_STOPPED: {
        bt_advertiser_remote_t* adver = INT2PTR(bt_advertiser_remote_t*) packet->adv_cb._on_advertising_stopped.adver;

        adver->callback->on_advertising_stopped(adver,
            packet->adv_cb._on_advertising_stopped.adv_id);
        free(adver);
        break;
    }
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
