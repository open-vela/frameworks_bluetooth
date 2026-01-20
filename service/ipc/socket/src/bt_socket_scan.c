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

#include "bluetooth.h"
#include "bt_le_scan.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "manager_service.h"
#include "scan_manager.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

typedef struct {
    service_timer_t* flush_timer;
    bt_scanner_t* scanner;
} bt_scan_flush_ctrl_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__) && defined(CONFIG_BLUETOOTH_BLE_SCAN)
#include "utils/log.h"

static void flush_scan_result_cache(bt_scanner_t* scanner)
{
    bt_scan_remote_t* scan = (bt_scan_remote_t*)scanner;
    bt_scan_flush_ctrl_t* ctrl = (bt_scan_flush_ctrl_t*)scan->flush_ctrl;
    bt_message_batch_scan_result_callbacks_t* cache = &scan->scan_result_cache;

    if (ctrl && ctrl->flush_timer) {
        service_loop_cancel_timer(ctrl->flush_timer);
        ctrl->flush_timer = NULL;
    }

    if (cache->count > 0) {
        bt_message_packet_t packet = { 0 };
        memcpy(&packet.scan_batch_cb, cache, sizeof(bt_message_batch_scan_result_callbacks_t));
        bt_socket_server_send(scan->ins, &packet, BT_LE_ON_BATCH_SCAN_RESULT);
        cache->count = 0;
    }
}

static void flush_scan_result_timer_cb(service_timer_t* timer, void* arg)
{
    bt_scanner_t* scanner = (bt_scanner_t*)arg;
    flush_scan_result_cache(scanner);
}

static bt_scan_flush_ctrl_t* flush_ctrl_create(bt_scanner_t* scanner)
{
    bt_scan_remote_t* scan = (bt_scan_remote_t*)scanner;

    if (scan->flush_ctrl)
        return (bt_scan_flush_ctrl_t*)scan->flush_ctrl;

    bt_scan_flush_ctrl_t* ctrl = zalloc(sizeof(bt_scan_flush_ctrl_t));
    if (!ctrl) {
        BT_LOGE("Failed to allocate flush_ctrl");
        return NULL;
    }

    ctrl->scanner = scanner;

    scan->flush_ctrl = ctrl;
    return ctrl;
}

static void flush_ctrl_destroy(bt_scanner_t* scanner)
{
    bt_scan_remote_t* scan = (bt_scan_remote_t*)scanner;
    bt_scan_flush_ctrl_t* ctrl = (bt_scan_flush_ctrl_t*)scan->flush_ctrl;

    if (!ctrl)
        return;

    if (ctrl->flush_timer) {
        service_loop_cancel_timer(ctrl->flush_timer);
        ctrl->flush_timer = NULL;
    }

    free(ctrl);
    scan->flush_ctrl = NULL;
}

static void restart_flush_timer(bt_scanner_t* scanner)
{
    bt_scan_remote_t* scan = (bt_scan_remote_t*)scanner;
    bt_scan_flush_ctrl_t* ctrl = (bt_scan_flush_ctrl_t*)scan->flush_ctrl;

    if (!ctrl)
        return;

    if (ctrl->flush_timer) {
        service_loop_cancel_timer(ctrl->flush_timer);
        ctrl->flush_timer = NULL;
    }

    ctrl->flush_timer = service_loop_timer(SCAN_FLUSH_INTERVAL_MS, 0, flush_scan_result_timer_cb, scanner);
}

static void on_scan_result_cb(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    bt_scan_remote_t* scan = scanner;
    bt_message_batch_scan_result_callbacks_t* cache = &scan->scan_result_cache;
    cache->scanner = scan->remote;

    if (result->length <= MAX_LEGACY_SCAN_RESULTS_LENGTH) {
        if (!scan->flush_ctrl) {
            scan->flush_ctrl = flush_ctrl_create(scanner);
        }

        memcpy(&cache->results[cache->count].result, result, sizeof(*result));
        memcpy(cache->results[cache->count].adv_data, result->adv_data, result->length);
        cache->count++;

    } else if (result->length <= MAX_EXT_SCAN_RESULTS_LENGTH) {
        bt_message_packet_t packet = { 0 };
        packet.scan_cb._on_scan_result_cb.scanner = scan->remote;
        memcpy(&packet.scan_cb._on_scan_result_cb.result, result, sizeof(*result));
        memcpy(packet.scan_cb._on_scan_result_cb.adv_data, result->adv_data, result->length);
        bt_socket_server_send(scan->ins, &packet, BT_LE_ON_SCAN_RESULT);

    } else {
        BT_LOGW("exceeds scan result maximum length :%d", result->length);
    }

    if (cache->count >= MAX_SCAN_RESULTS_PER_PACKET) {
        flush_scan_result_cache(scanner);
    } else {
        restart_flush_timer(scanner);
    }
}

static void on_scan_status_cb(bt_scanner_t* scanner, uint8_t status)
{
    bt_scan_remote_t* scan = scanner;
    bt_message_packet_t packet = { 0 };

    packet.scan_cb._on_scan_status_cb.scanner = scan->remote;
    packet.scan_cb._on_scan_status_cb.status = status;

    bt_socket_server_send(scan->ins, &packet, BT_LE_ON_SCAN_START_STATUS);

    if (status != 0)
        free(scan);
}

static void on_scan_stopped_cb(bt_scanner_t* scanner)
{
    bt_scan_remote_t* scan = scanner;

    flush_scan_result_cache(scanner);
    flush_ctrl_destroy(scanner);

    bt_message_packet_t packet = { 0 };

    packet.scan_cb._on_scan_stopped_cb.scanner = scan->remote;
    bt_socket_server_send(scan->ins, &packet, BT_LE_ON_SCAN_STOPPED);
    free(scanner);
}

static scanner_callbacks_t g_scanner_socket_cb = {
    sizeof(g_scanner_socket_cb),
    on_scan_result_cb,
    on_scan_status_cb,
    on_scan_stopped_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_scan_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_LE_SCAN_START: {
        bt_scan_remote_t* scan = zalloc(sizeof(*scan));

        scan->ins = ins;
        scan->remote = packet->scan_pl._bt_le_start_scan.remote;
        packet->scan_r.remote = PTR2INT(uint64_t) scanner_start_scan(scan, &g_scanner_socket_cb);
        if (!packet->scan_r.remote)
            free(scan);
        break;
    }
    case BT_LE_SCAN_START_SETTINGS: {
        bt_scan_remote_t* scan = zalloc(sizeof(*scan));

        scan->ins = ins;
        scan->remote = packet->scan_pl._bt_le_start_scan_settings.remote;
        packet->scan_r.remote = PTR2INT(uint64_t) scanner_start_scan_settings(scan,
            &packet->scan_pl._bt_le_start_scan_settings.settings, &g_scanner_socket_cb);
        if (!packet->scan_r.remote) {
            free(scan);
        }
        break;
    }
    case BT_LE_SCAN_START_WITH_FILTERS: {
        bt_scan_remote_t* scan = zalloc(sizeof(*scan));

        scan->ins = ins;
        scan->remote = packet->scan_pl._bt_le_start_scan_with_filters.remote;
        packet->scan_r.remote = PTR2INT(uint64_t) scanner_start_scan_with_filters(scan,
            &packet->scan_pl._bt_le_start_scan_with_filters.settings,
            &packet->scan_pl._bt_le_start_scan_with_filters.filter,
            &g_scanner_socket_cb);
        if (!packet->scan_r.remote) {
            free(scan);
        }
        break;
    }
    case BT_LE_SCAN_STOP: {
        scanner_stop_scan(INT2PTR(bt_scanner_t*) packet->scan_pl._bt_le_stop_scan.remote);
        break;
    }
    case BT_LE_SCAN_IS_SUPPORT: {
        packet->scan_r.vbool = scan_is_supported();
        break;
    }
    default:
        break;
    }
}

#endif

int bt_socket_client_scan_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    switch (packet->code) {
    case BT_LE_ON_SCAN_RESULT: {
        bt_scan_remote_t* scan = INT2PTR(bt_scan_remote_t*) packet->scan_cb._on_scan_result_cb.scanner;
        ble_scan_result_t* result = &packet->scan_cb._on_scan_result_cb.result;
        ble_scan_result_t* tmp = malloc(sizeof(ble_scan_result_t) + result->length);
        memcpy(tmp, result, sizeof(ble_scan_result_t));
        memcpy(tmp->adv_data, packet->scan_cb._on_scan_result_cb.adv_data, result->length);
        CALLBACK_REMOTE(scan, scanner_callbacks_t,
            on_scan_result,
            tmp);
        free(tmp);
        break;
    }
    case BT_LE_ON_SCAN_START_STATUS: {
        bt_scan_remote_t* scan = INT2PTR(bt_scan_remote_t*) packet->scan_cb._on_scan_status_cb.scanner;

        CALLBACK_REMOTE(scan, scanner_callbacks_t,
            on_scan_start_status,
            packet->scan_cb._on_scan_status_cb.status);
        if (packet->scan_cb._on_scan_status_cb.status != 0)
            free(scan);
        break;
    }
    case BT_LE_ON_SCAN_STOPPED: {
        bt_scan_remote_t* scan = INT2PTR(bt_scan_remote_t*) packet->scan_cb._on_scan_stopped_cb.scanner;

        CALLBACK_REMOTE(scan, scanner_callbacks_t,
            on_scan_stopped);
        free(scan);
        break;
    }
    default: {
        uint32_t subcode = BT_IPC_GET_SUBCODE(packet->code);
        switch (subcode) {
        case BLE_SCAN_SUBCODE_BATCH_SCAN_CALLBACK: {
            bt_scan_remote_t* scan = INT2PTR(bt_scan_remote_t*) packet->scan_batch_cb.scanner;

            uint8_t tmp_buf[sizeof(ble_scan_result_t) + MAX_LEGACY_SCAN_RESULTS_LENGTH];

            for (int i = 0; i < packet->scan_batch_cb.count; ++i) {
                ble_scan_result_t* result = &packet->scan_batch_cb.results[i].result;
                uint8_t len = result->length;

                if (len > MAX_LEGACY_SCAN_RESULTS_LENGTH)
                    continue;

                ble_scan_result_t* tmp = (ble_scan_result_t*)tmp_buf;

                memcpy(tmp, result, sizeof(ble_scan_result_t));
                memcpy(tmp->adv_data, packet->scan_batch_cb.results[i].adv_data, len);

                CALLBACK_REMOTE(scan, scanner_callbacks_t,
                    on_scan_result,
                    tmp);
            }
            break;
        }
        default:
            return BT_STATUS_PARM_INVALID;
        }
    }
    }

    return BT_STATUS_SUCCESS;
}
