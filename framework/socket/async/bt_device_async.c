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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_async.h"
#include "bt_device.h"
#include "bt_message.h"
#include "bt_socket.h"

static void device_s8_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_s8_cb_t ret_cb = (bt_s8_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, (int8_t)packet->devs_r.v8, userdata);
}

static void device_status_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_status_cb_t ret_cb = (bt_status_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, userdata);
}

static void device_bool_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_bool_cb_t ret_cb = (bt_bool_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_r.bbool, userdata);
}

static void device_u16_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_u16_cb_t ret_cb = (bt_u16_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_r.v16, userdata);
}

static void device_u32_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_u32_cb_t ret_cb = (bt_u32_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_r.v32, userdata);
}

static void device_get_device_type_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_device_type_cb_t ret_cb = (bt_device_type_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_r.dtype, userdata);
}

static void device_get_name_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_string_cb_t ret_cb = (bt_string_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_pl._bt_device_get_name.name, userdata);
}

static void device_get_uuids_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_uuids_cb_t ret_cb = (bt_uuids_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_pl._bt_device_get_uuids.uuids, packet->devs_pl._bt_device_get_uuids.size, userdata);
}

static void device_get_alias_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_string_cb_t ret_cb = (bt_string_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_pl._bt_device_get_alias.alias, userdata);
}

static void device_get_bond_state_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_device_get_bond_state_cb_t ret_cb = (bt_device_get_bond_state_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_r.bstate, userdata);
}

static void device_get_identity_address_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_address_cb_t ret_cb = (bt_address_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, &packet->devs_pl._bt_device_addr.addr, userdata);
}

static void device_get_address_type_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_device_get_address_type_cb_t ret_cb = (bt_device_get_address_type_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->devs_r.status, packet->devs_r.atype, userdata);
}

static int bt_device_send_async(bt_instance_t* ins, bt_address_t* addr,
    bt_message_packet_t* packet, bt_message_type_t code, bt_socket_reply_cb_t reply, void* cb, void* userdata)
{
    memcpy(&packet->devs_pl._bt_device_addr.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, packet, code, reply, cb, userdata);
}

bt_status_t bt_device_get_identity_address_async(bt_instance_t* ins, bt_address_t* bd_addr, bt_address_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, bd_addr, &packet, BT_DEVICE_GET_IDENTITY_ADDRESS, device_get_identity_address_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_address_type_async(bt_instance_t* ins, bt_address_t* addr, bt_device_get_address_type_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_GET_ADDRESS_TYPE, device_get_address_type_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_device_type_async(bt_instance_t* ins, bt_address_t* addr, bt_device_type_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_GET_DEVICE_TYPE, device_get_device_type_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_name_async(bt_instance_t* ins, bt_address_t* addr, bt_string_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_get_name.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_GET_NAME, device_get_name_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_device_class_async(bt_instance_t* ins, bt_address_t* addr, bt_u32_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_GET_DEVICE_CLASS, device_u32_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_uuids_async(bt_instance_t* ins, bt_address_t* addr, bt_uuids_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_get_uuids.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_GET_UUIDS, device_get_uuids_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_appearance_async(bt_instance_t* ins, bt_address_t* addr, bt_u16_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_GET_APPEARANCE, device_u16_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_rssi_async(bt_instance_t* ins, bt_address_t* addr, bt_s8_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_GET_RSSI, device_s8_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_alias_async(bt_instance_t* ins, bt_address_t* addr, bt_string_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_get_alias.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_GET_ALIAS, device_get_alias_reply, cb, userdata);
}

bt_status_t bt_device_set_alias_async(bt_instance_t* ins, bt_address_t* addr,
    const char* alias, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet = { 0 };

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_set_alias.addr, addr, sizeof(*addr));
    strncpy(packet.devs_pl._bt_device_set_alias.alias, alias,
        sizeof(packet.devs_pl._bt_device_set_alias.alias) - 1);

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_SET_ALIAS, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_is_connected_async(bt_instance_t* ins, bt_address_t* addr,
    bt_transport_t transport, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.devs_pl._bt_device_is_connected.transport = transport;

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_IS_CONNECTED, device_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_device_is_encrypted_async(bt_instance_t* ins, bt_address_t* addr,
    bt_transport_t transport, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.devs_pl._bt_device_is_encrypted.transport = transport;

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_IS_ENCRYPTED, device_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_device_is_bond_initiate_local_async(bt_instance_t* ins, bt_address_t* addr,
    bt_transport_t transport, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.devs_pl._bt_device_is_bond_initiate_local.transport = transport;

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_IS_BOND_INITIATE_LOCAL, device_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_bond_state_async(bt_instance_t* ins, bt_address_t* addr,
    bt_transport_t transport, bt_device_get_bond_state_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.devs_pl._bt_device_get_bond_state.transport = transport;

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_GET_BOND_STATE, device_get_bond_state_reply, cb, userdata);
}

bt_status_t bt_device_is_bonded_async(bt_instance_t* ins, bt_address_t* addr,
    bt_transport_t transport, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.devs_pl._bt_device_is_bonded.transport = transport;

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_IS_BONDED, device_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_device_connect_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_CONNECT, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_disconnect_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_DISCONNECT, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_connect_le_async(bt_instance_t* ins,
    bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_connect_le.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_connect_le.type = type;
    memcpy(&packet.devs_pl._bt_device_connect_le.param, param, sizeof(*param));

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_CONNECT_LE, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_disconnect_le_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_DISCONNECT_LE, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_connect_request_reply_async(bt_instance_t* ins, bt_address_t* addr,
    bool accept, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_connect_request_reply.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_connect_request_reply.accept = accept;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_CONNECT_REQUEST_REPLY, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_connect_all_profile_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_CONNECT_ALL_PROFILE, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_disconnect_all_profile_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_device_send_async(ins, addr, &packet, BT_DEVICE_DISCONNECT_ALL_PROFILE, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_set_le_phy_async(bt_instance_t* ins,
    bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_set_le_phy.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_set_le_phy.tx_phy = tx_phy;
    packet.devs_pl._bt_device_set_le_phy.rx_phy = rx_phy;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_SET_LE_PHY, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_create_bond_async(bt_instance_t* ins, bt_address_t* addr,
    bt_transport_t transport, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_create_bond.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_create_bond.transport = transport;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_CREATE_BOND, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_remove_bond_async(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_remove_bond.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_remove_bond.transport = transport;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_REMOVE_BOND, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_cancel_bond_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_cancel_bond.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_CANCEL_BOND, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_pair_request_reply_async(bt_instance_t* ins, bt_address_t* addr, bool accept, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_pair_request_reply.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_pair_request_reply.accept = accept;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_PAIR_REQUEST_REPLY, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_set_pairing_confirmation_async(bt_instance_t* ins, bt_address_t* addr,
    uint8_t transport, bool accept, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_set_pairing_confirmation.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_set_pairing_confirmation.transport = transport;
    packet.devs_pl._bt_device_set_pairing_confirmation.accept = accept;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_SET_PAIRING_CONFIRMATION, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_set_pin_code_async(bt_instance_t* ins, bt_address_t* addr, bool accept,
    char* pincode, int len, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    if (len > sizeof(packet.devs_pl._bt_device_set_pin_code.pincode))
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.devs_pl._bt_device_set_pin_code.addr, addr, sizeof(*addr));
    memcpy(&packet.devs_pl._bt_device_set_pin_code.pincode, pincode, len);
    packet.devs_pl._bt_device_set_pin_code.len = len;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_SET_PIN_CODE, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_set_pass_key_async(bt_instance_t* ins, bt_address_t* addr,
    uint8_t transport, bool accept, uint32_t passkey, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_set_pass_key.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_set_pass_key.transport = transport;
    packet.devs_pl._bt_device_set_pass_key.accept = accept;
    packet.devs_pl._bt_device_set_pass_key.passkey = passkey;

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_SET_PASS_KEY, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_set_le_legacy_tk_async(bt_instance_t* ins, bt_address_t* addr,
    bt_128key_t tk_val, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_set_le_legacy_tk.addr, addr, sizeof(packet.devs_pl._bt_device_set_le_legacy_tk.addr));
    memcpy(packet.devs_pl._bt_device_set_le_legacy_tk.tk_val, tk_val, sizeof(packet.devs_pl._bt_device_set_le_legacy_tk.tk_val));

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_SET_LE_LEGACY_TK, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_set_le_sc_remote_oob_data_async(bt_instance_t* ins, bt_address_t* addr,
    bt_128key_t c_val, bt_128key_t r_val, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_set_le_sc_remote_oob_data.addr, addr, sizeof(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.addr));
    memcpy(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.c_val, c_val, sizeof(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.c_val));
    memcpy(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.r_val, r_val, sizeof(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.r_val));

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_SET_LE_SC_REMOTE_OOB_DATA, device_status_reply, (void*)cb, userdata);
}

bt_status_t bt_device_get_le_sc_local_oob_data_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_get_le_sc_local_oob_data.addr, addr, sizeof(packet.devs_pl._bt_device_get_le_sc_local_oob_data.addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_DEVICE_GET_LE_SC_LOCAL_OOB_DATA, device_status_reply, (void*)cb, userdata);
}