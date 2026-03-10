/****************************************************************************
 * service/ipc/socket/src/bt_socket_pa_sync.c
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

#include "bt_internal.h"

#include "bt_message.h"
#include "bt_socket.h"
#include "pa_sync_service.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/
typedef struct pa_sync_remote {
    bt_instance_t* ins;
    uint64_t cbs; /**< bt_pa_sync_callbacks_t* */
    uint64_t context; /**< void* */
} pa_sync_remote_t;

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void on_sync_established_cb(const bt_le_address_t* addr, uint8_t sid, void* context)
{
    pa_sync_remote_t* remote = (pa_sync_remote_t*)context;
    bt_message_packet_t packet = { 0 };

    packet.pa_sync_cb.cbs = remote->cbs;
    packet.pa_sync_cb.context = remote->context;
    packet.pa_sync_cb._on_sync_established.sid = sid;
    memcpy(&packet.pa_sync_cb._on_sync_established.addr, addr, sizeof(bt_le_address_t));

    bt_socket_server_send(remote->ins, &packet, BT_PA_SYNC_ON_SYNC_ESTABLISHED);
}

static void on_sync_terminated_cb(const bt_le_address_t* addr, uint8_t sid, void* context)
{
    pa_sync_remote_t* remote = (pa_sync_remote_t*)context;
    bt_message_packet_t packet = { 0 };

    packet.pa_sync_cb.cbs = remote->cbs;
    packet.pa_sync_cb.context = remote->context;
    packet.pa_sync_cb._on_sync_terminated.sid = sid;
    memcpy(&packet.pa_sync_cb._on_sync_terminated.addr, addr, sizeof(bt_le_address_t));

    bt_socket_server_send(remote->ins, &packet, BT_PA_SYNC_ON_SYNC_TERMINATED);

    free(remote);
}

static void on_sync_report_cb(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_report_t* report, void* context)
{
    pa_sync_remote_t* remote = (pa_sync_remote_t*)context;
    bt_message_packet_t packet = { 0 };

    if (report->adv_data_len > BT_PA_SYNC_DATA_LEN_MAX)
        return;

    packet.pa_sync_cb.cbs = remote->cbs;
    packet.pa_sync_cb.context = remote->context;
    packet.pa_sync_cb._on_sync_report.sid = sid;
    memcpy(&packet.pa_sync_cb._on_sync_report.addr, addr, sizeof(bt_le_address_t));

    packet.pa_sync_cb._on_sync_report.tx_power = report->tx_power;
    packet.pa_sync_cb._on_sync_report.rssi = report->rssi;
    packet.pa_sync_cb._on_sync_report.cnt = report->cnt;
    packet.pa_sync_cb._on_sync_report.adv_data_len = report->adv_data_len;
    packet.pa_sync_cb._on_sync_report.subevent = report->subevent;
    if (report->adv_data_len && report->data)
        memcpy(packet.pa_sync_cb._on_sync_report.data, report->data, report->adv_data_len);

    bt_socket_server_send(remote->ins, &packet, BT_PA_SYNC_ON_SYNC_REPORT);
}

static const bt_pa_sync_callbacks_t g_pa_sync_socket_cb = {
    .on_sync_established = on_sync_established_cb,
    .on_sync_terminated = on_sync_terminated_cb,
    .on_sync_report = on_sync_report_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_pa_sync_process(service_poll_t* poll, int fd, bt_instance_t* ins,
    bt_message_packet_t* packet)
{
    pa_sync_remote_t* remote;

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case PA_SYNC_SUBCODE_CREATE_SYNC:
        remote = zalloc(sizeof(pa_sync_remote_t));
        if (!remote) {
            packet->pa_sync_r.status = BT_STATUS_NOMEM;
            break;
        }

        remote->ins = ins;
        remote->cbs = packet->pa_sync_pl._bt_pa_sync_create.cbs;
        remote->context = packet->pa_sync_pl._bt_pa_sync_create.context;
        packet->pa_sync_r.status = pa_sync_create(&packet->pa_sync_pl._bt_pa_sync_create.addr,
            packet->pa_sync_pl._bt_pa_sync_create.sid,
            packet->pa_sync_pl._bt_pa_sync_create.have_params
                ? &packet->pa_sync_pl._bt_pa_sync_create.params
                : NULL,
            &g_pa_sync_socket_cb, remote);

        if (packet->pa_sync_r.status != BT_STATUS_SUCCESS)
            free(remote);

        break;
    case PA_SYNC_SUBCODE_TERMINATE_SYNC:
        break;
    default:
        break;
    }
}
#endif

int bt_socket_client_pa_sync_callback(service_poll_t* poll, int fd, bt_instance_t* ins,
    bt_message_packet_t* packet, bool is_async)
{
    bt_pa_sync_callbacks_t* cbs = INT2PTR(bt_pa_sync_callbacks_t*) packet->pa_sync_cb.cbs;
    void* context = INT2PTR(void*) packet->pa_sync_cb.context;

    if (!cbs)
        return BT_STATUS_PARM_INVALID;

    switch (BT_IPC_GET_SUBCODE(packet->code)) {
    case PA_SYNC_SUBCODE_SYNC_ESTABLISHED_CALLBACK:
        if (cbs->on_sync_established) {
            cbs->on_sync_established(&packet->pa_sync_cb._on_sync_established.addr,
                packet->pa_sync_cb._on_sync_established.sid, context);
        }
        break;
    case PA_SYNC_SUBCODE_SYNC_TERMINATED_CALLBACK:
        if (cbs->on_sync_terminated) {
            cbs->on_sync_terminated(&packet->pa_sync_cb._on_sync_terminated.addr,
                packet->pa_sync_cb._on_sync_terminated.sid, context);
        }
        break;
    case PA_SYNC_SUBCODE_SYNC_REPORT_CALLBACK:
        if (cbs->on_sync_report) {
            bt_pa_sync_report_t report = { 0 };
            report.tx_power = packet->pa_sync_cb._on_sync_report.tx_power;
            report.rssi = packet->pa_sync_cb._on_sync_report.rssi;
            report.cnt = packet->pa_sync_cb._on_sync_report.cnt;
            report.subevent = packet->pa_sync_cb._on_sync_report.subevent;
            report.adv_data_len = packet->pa_sync_cb._on_sync_report.adv_data_len;
            if (report.adv_data_len)
                report.data = packet->pa_sync_cb._on_sync_report.data;

            cbs->on_sync_report(&packet->pa_sync_cb._on_sync_report.addr,
                packet->pa_sync_cb._on_sync_report.sid, &report, context);
        }
        break;
    default:
        break;
    }

    return BT_STATUS_SUCCESS;
}