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

#include "enable.h"

void demo_enable_handle_gap_message(bt_instance_t* g_bt_ins, node_t* node)
{
    app_demo_message_t* msg = &node->data;
    switch (msg->msg_type) {
    case APP_BT_GAP_ON_GAP_STATE_CHANGED:
        LOGI("Adapter state changed: %d", msg->gap_cb._on_adapter_state_changed.state);
        if (msg->gap_cb._on_adapter_state_changed.state == BT_ADAPTER_STATE_ON) {
            // This sample code is used to test opening Bluetooth,
            // so after Bluetooth is turned on, turn off Bluetooth.
            bt_adapter_disable(g_bt_ins);
        } else if (msg->gap_cb._on_adapter_state_changed.state == BT_ADAPTER_STATE_OFF) {
        }
        break;
    default:
        break;
    }
}