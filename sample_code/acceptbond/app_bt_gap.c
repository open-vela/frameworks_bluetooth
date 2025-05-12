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
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include "acceptbond.h"

void demo_acceptbond_handle_gap_message(bt_instance_t* g_bt_ins, node_t* node)
{
    app_demo_message_t* msg = &node->data;
    switch (msg->msg_type) {
    case APP_BT_GAP_GET_NAME:
        bt_adapter_get_name(g_bt_ins, msg->gap_req._bt_adapter_get_name.name, BT_NAME_LENGTH);
        LOGI("Adapter Name: %s\n", msg->gap_req._bt_adapter_get_name.name);
        break;
    case APP_BT_GAP_SET_SCANMODE:
        bt_adapter_set_scan_mode(g_bt_ins, msg->gap_req._bt_adapter_set_scan_mode.mode,
            msg->gap_req._bt_adapter_set_scan_mode.bondable);
        break;
    case APP_BT_GAP_SET_IO_CAPABILITY:
        bt_adapter_set_io_capability(g_bt_ins, msg->gap_req._bt_adapter_set_io_capability.cap);
        break;
    case APP_BT_GAP_CONNECT_REQUEST_REPLY:
        bt_device_connect_request_reply(g_bt_ins, &msg->gap_req._bt_device_connect_request_reply.addr,
            msg->gap_req._bt_device_connect_request_reply.accept);
        break;
    case APP_BT_GAP_ON_GAP_STATE_CHANGED:
        break;
    case APP_BT_GAP_ON_CONNECTION_STATE_CHANGED:
        break;
    case APP_BT_GAP_ON_BOND_STATE_CHANGED:
        LOGI("Bond state changed: %d", msg->gap_cb._on_bond_state_changed.state);
        if (msg->gap_cb._on_bond_state_changed.state == BOND_STATE_BONDED) {
            // The sample code is used to test passive connection/pairing/binding,
            // so after device is bonded, reset the running flag.
            bt_adapter_disable(g_bt_ins);
        }
        break;
    default:
        break;
    }
}
