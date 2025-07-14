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

#include "a2dpsnk.h"
#include "bt_a2dp_sink.h"

void app_bt_handle_message(bt_instance_t* bt_ins, node_t* node)
{
    bt_status_t ret;
    app_demo_message_t* msg = &node->data;
    switch (msg->msg_type) {
    case APP_BT_GAP_SET_IO_CAPABILITY:
        bt_adapter_set_io_capability(bt_ins, msg->message._bt_adapter_set_io_capability.cap);
        break;
    case APP_BT_GAP_CREATE_CONN:
        ret = bt_device_connect(bt_ins, &msg->message._bt_device_create_conn.addr);
        if (ret != BT_STATUS_SUCCESS) {
            LOGE("create conn failed, ret = %d\n", ret);
        }
        break;
    case APP_BT_A2DP_CREATE_CONN:
        LOGI("Connection state changed: %d", msg->message._on_connection_state_changed.state);
        if (msg->message._on_connection_state_changed.state == CONNECTION_STATE_CONNECTED) {
            LOGI("Connect A2DP now\r\n");
            bt_a2dp_sink_connect(bt_ins, &msg->message._on_connection_state_changed.addr);
        }
        if (msg->message._on_connection_state_changed.state == CONNECTION_STATE_DISCONNECTED) {
            LOGI("ACL is disconnect\r\n");
            app_demo_a2dpsnk_set_acl_active(false);
        }
        break;       
    default:
        break;
    }
}