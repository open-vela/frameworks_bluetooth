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

#include "discovery.h"

void demo_discovery_handle_gap_message(bt_instance_t* g_bt_ins, node_t* node)
{
    app_demo_message_t* msg = &node->data;
    switch (msg->msg_type) {
    case APP_BT_GAP_START_DISCOVERY:
        bt_adapter_start_discovery(g_bt_ins, msg->gap_req._bt_adapter_start_discovery.timeout);
        break;
    case APP_BT_GAP_ON_GAP_STATE_CHANGED:
        LOGI("Adapter state changed: %d", msg->gap_cb._on_adapter_state_changed.state);
        break;
    case APP_BT_GAP_ON_DISCOVERY_STATE_CHANGED:
        if (msg->gap_cb._on_discovery_state_changed.state == BT_DISCOVERY_STATE_STARTED) {
            LOGI("Discovery started");
            break;
        }
        LOGI("Discovery stopped");
        // This sample code is used to test discovering nearby Bluetooth devices,
        // so after the discovery process ends, Bluetooth is turned off.
        bt_adapter_disable(g_bt_ins);

        break;
    default:
        break;
    }
}