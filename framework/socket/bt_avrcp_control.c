/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "avrcp_control_api"

#include <stdint.h>

#include "bt_avrcp_control.h"
#include "bt_socket.h"

void* bt_avrcp_control_register_callbacks(bt_instance_t* ins, const avrcp_control_callbacks_t* callbacks)
{
    bt_message_packet_t packet;
    bt_status_t status;
    void* handle;

    BT_SOCKET_INS_VALID(ins, NULL);

    if (ins->avrcp_control_callbacks != NULL) {
        handle = bt_remote_callbacks_register(ins->avrcp_control_callbacks, NULL, (void*)callbacks);
        return handle;
    }

    ins->avrcp_control_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);

#ifdef CONFIG_BLUETOOTH_FEATURE
    handle = bt_remote_callbacks_register(ins->avrcp_control_callbacks, ins, (void*)callbacks);
#else
    handle = bt_remote_callbacks_register(ins->avrcp_control_callbacks, NULL, (void*)callbacks);
#endif

    if (handle == NULL) {
        bt_callbacks_list_free(ins->avrcp_control_callbacks);
        ins->avrcp_control_callbacks = NULL;
        return handle;
    }

    status = bt_socket_client_sendrecv(ins, &packet, BT_AVRCP_CONTROL_REGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.avrcp_control_r.status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(ins->avrcp_control_callbacks);
        ins->avrcp_control_callbacks = NULL;
        return NULL;
    }

    return handle;
}

bool bt_avrcp_control_unregister_callbacks(bt_instance_t* ins, void* cookie)
{
    bt_message_packet_t packet;
    bt_status_t status;
    callbacks_list_t* cbsl;

    BT_SOCKET_INS_VALID(ins, false);

    if (!ins->avrcp_control_callbacks)
        return false;

    bt_remote_callbacks_unregister(ins->avrcp_control_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(ins->avrcp_control_callbacks) > 0) {
        return true;
    }

    cbsl = ins->avrcp_control_callbacks;
    ins->avrcp_control_callbacks = NULL;
    bt_socket_client_free_callbacks(ins, cbsl);

    status = bt_socket_client_sendrecv(ins, &packet, BT_AVRCP_CONTROL_UNREGISTER_CALLBACKS);
    if (status != BT_STATUS_SUCCESS || packet.avrcp_control_r.status != BT_STATUS_SUCCESS) {
        return false;
    }

    return true;
}

bt_status_t bt_avrcp_control_get_element_attributes(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.avrcp_control_pl._bt_avrcp_control_get_element_attribute.addr, addr, sizeof(packet.avrcp_control_pl._bt_avrcp_control_get_element_attribute.addr));
    status = bt_socket_client_sendrecv(ins, &packet, BT_AVRCP_CONTROL_GET_ELEMENT_ATTRIBUTES);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.avrcp_control_r.status;
}
