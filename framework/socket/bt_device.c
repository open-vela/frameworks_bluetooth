/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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

#include "adapter_internel.h"
#include "bluetooth.h"
#include "bt_addr.h"
#include "bt_device.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "device.h"

static int bt_device_send(bt_instance_t* ins, bt_address_t* addr,
    bt_message_packet_t* packet, bt_message_type_t code)
{
    memcpy(&packet->devs_pl._bt_device_addr.addr, addr, sizeof(*addr));

    return bt_socket_client_sendrecv(ins, packet, code);
}

bt_status_t bt_device_get_identity_address(bt_instance_t* ins, bt_address_t* bd_addr, bt_address_t* id_addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    status = bt_device_send(ins, bd_addr, &packet, BT_DEVICE_GET_IDENTITY_ADDRESS);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    if (packet.devs_r.status == BT_STATUS_SUCCESS) {
        memcpy(id_addr, &packet.devs_pl._bt_device_addr.addr, sizeof(*id_addr));
    }

    return packet.devs_r.status;
}

ble_addr_type_t bt_device_get_address_type(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_LE_ADDR_TYPE_UNKNOWN);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_GET_ADDRESS_TYPE);
    if (status != BT_STATUS_SUCCESS) {
        return BT_LE_ADDR_TYPE_UNKNOWN;
    }

    return packet.devs_r.atype;
}

bt_device_type_t bt_device_get_device_type(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_DEVICE_TYPE_UNKNOW);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_GET_DEVICE_TYPE);
    if (status != BT_STATUS_SUCCESS) {
        return BT_DEVICE_TYPE_UNKNOW;
    }

    return packet.devs_r.dtype;
}

bool bt_device_get_name(bt_instance_t* ins, bt_address_t* addr, char* name, uint32_t length)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    memcpy(&packet.devs_pl._bt_device_get_name.addr, addr, sizeof(*addr));
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_GET_NAME);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    char* nr = packet.devs_pl._bt_device_get_name.name;
    int len = (strlen(nr) > length) ? length : strlen(nr);
    memcpy(name, nr, len);

    return packet.devs_r.bbool;
}

uint32_t bt_device_get_device_class(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, 0);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_GET_DEVICE_CLASS);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.v32;
}

bt_status_t bt_device_get_uuids(bt_instance_t* ins, bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_get_uuids.addr, addr, sizeof(*addr));
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_GET_UUIDS);
    if (status != BT_STATUS_SUCCESS)
        return status;

    *size = packet.devs_pl._bt_device_get_uuids.size;

    if (*size > 0) {
        *uuids = calloc(*size, sizeof(bt_uuid_t));
        if (*uuids == NULL)
            return BT_STATUS_NOMEM;
        memcpy(*uuids, packet.devs_pl._bt_device_get_uuids.uuids, sizeof(bt_uuid_t) * *size);
    }

    return packet.devs_r.status;
}

uint16_t bt_device_get_appearance(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, 0);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_GET_APPEARANCE);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.v16;
}

int8_t bt_device_get_rssi(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, 0);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_GET_RSSI);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.v8;
}

bool bt_device_get_alias(bt_instance_t* ins, bt_address_t* addr, char* alias, uint32_t length)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    memcpy(&packet.devs_pl._bt_device_get_alias.addr, addr, sizeof(*addr));
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_GET_ALIAS);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    strlcpy(alias, packet.devs_pl._bt_device_get_alias.alias,
        MIN(length, sizeof(packet.devs_pl._bt_device_get_alias.alias)));

    return packet.devs_r.status;
}

bt_status_t bt_device_set_alias(bt_instance_t* ins, bt_address_t* addr, const char* alias)
{
    bt_message_packet_t packet = { 0 };
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_set_alias.addr, addr, sizeof(*addr));
    strncpy(packet.devs_pl._bt_device_set_alias.alias, alias,
        sizeof(packet.devs_pl._bt_device_set_alias.alias) - 1);
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_SET_ALIAS);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bool bt_device_is_connected(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    packet.devs_pl._bt_device_is_connected.transport = transport;
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_IS_CONNECTED);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    return packet.devs_r.bbool;
}

bool bt_device_is_encrypted(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    packet.devs_pl._bt_device_is_encrypted.transport = transport;
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_IS_ENCRYPTED);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    return packet.devs_r.bbool;
}

bool bt_device_is_bond_initiate_local(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    packet.devs_pl._bt_device_is_bond_initiate_local.transport = transport;
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_IS_BOND_INITIATE_LOCAL);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    return packet.devs_r.bbool;
}

bond_state_t bt_device_get_bond_state(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BOND_STATE_NONE);
    packet.devs_pl._bt_device_get_bond_state.transport = transport;
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_GET_BOND_STATE);
    if (status != BT_STATUS_SUCCESS) {
        return BOND_STATE_NONE;
    }

    return packet.devs_r.bstate;
}

bool bt_device_is_bonded(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, false);
    packet.devs_pl._bt_device_is_bonded.transport = transport;
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_IS_BONDED);
    if (status != BT_STATUS_SUCCESS) {
        return false;
    }

    return packet.devs_r.bbool;
}

bt_status_t bt_device_connect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_CONNECT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_background_connect(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    if (addr)
        memcpy(&packet.devs_pl._bt_device_background_connect, addr, sizeof(*addr));
    else
        bt_addr_set_empty(&packet.devs_pl._bt_device_background_connect.addr);
    packet.devs_pl._bt_device_background_connect.transport = transport;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_BACKGROUND_CONNECT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_background_disconnect(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_background_disconnect, addr, sizeof(*addr));
    packet.devs_pl._bt_device_background_disconnect.transport = transport;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_BACKGROUND_DISCONNECT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_disconnect(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_DISCONNECT);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_connect_le(bt_instance_t* ins,
    bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_connect_le.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_connect_le.type = type;
    memcpy(&packet.devs_pl._bt_device_connect_le.param, param, sizeof(*param));
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_CONNECT_LE);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_disconnect_le(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    status = bt_device_send(ins, addr, &packet, BT_DEVICE_DISCONNECT_LE);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_connect_request_reply(bt_instance_t* ins, bt_address_t* addr, bool accept)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_connect_request_reply.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_connect_request_reply.accept = accept;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_CONNECT_REQUEST_REPLY);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

void bt_device_connect_all_profile(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );
    (void)bt_device_send(ins, addr, &packet, BT_DEVICE_CONNECT_ALL_PROFILE);
}

void bt_device_disconnect_all_profile(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, );
    (void)bt_device_send(ins, addr, &packet, BT_DEVICE_DISCONNECT_ALL_PROFILE);
}

bt_status_t bt_device_set_le_phy(bt_instance_t* ins,
    bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_set_le_phy.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_set_le_phy.tx_phy = tx_phy;
    packet.devs_pl._bt_device_set_le_phy.rx_phy = rx_phy;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_SET_LE_PHY);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_create_bond(bt_instance_t* ins, bt_address_t* addr, bt_transport_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_create_bond.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_create_bond.transport = transport;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_CREATE_BOND);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_remove_bond(bt_instance_t* ins, bt_address_t* addr, uint8_t transport)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_remove_bond.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_remove_bond.transport = transport;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_REMOVE_BOND);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_cancel_bond(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_cancel_bond.addr, addr, sizeof(*addr));
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_CANCEL_BOND);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_pair_request_reply(bt_instance_t* ins, bt_address_t* addr, bool accept)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_pair_request_reply.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_pair_request_reply.accept = accept;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_PAIR_REQUEST_REPLY);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_set_pairing_confirmation(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_set_pairing_confirmation.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_set_pairing_confirmation.transport = transport;
    packet.devs_pl._bt_device_set_pairing_confirmation.accept = accept;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_SET_PAIRING_CONFIRMATION);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_set_pin_code(bt_instance_t* ins, bt_address_t* addr, bool accept,
    char* pincode, int len)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    if (len > sizeof(packet.devs_pl._bt_device_set_pin_code.pincode))
        return BT_STATUS_PARM_INVALID;

    memcpy(&packet.devs_pl._bt_device_set_pin_code.addr, addr, sizeof(*addr));
    memcpy(&packet.devs_pl._bt_device_set_pin_code.pincode, pincode, len);
    packet.devs_pl._bt_device_set_pin_code.len = len;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_SET_PIN_CODE);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_set_pass_key(bt_instance_t* ins, bt_address_t* addr, uint8_t transport, bool accept, uint32_t passkey)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_set_pass_key.addr, addr, sizeof(*addr));
    packet.devs_pl._bt_device_set_pass_key.transport = transport;
    packet.devs_pl._bt_device_set_pass_key.accept = accept;
    packet.devs_pl._bt_device_set_pass_key.passkey = passkey;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_SET_PASS_KEY);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t BTSYMBOLS(bt_device_set_le_legacy_tk)(bt_instance_t* ins, bt_address_t* addr, bt_128key_t tk_val)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_set_le_legacy_tk.addr, addr, sizeof(packet.devs_pl._bt_device_set_le_legacy_tk.addr));
    memcpy(packet.devs_pl._bt_device_set_le_legacy_tk.tk_val, tk_val, sizeof(packet.devs_pl._bt_device_set_le_legacy_tk.tk_val));
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_SET_LE_LEGACY_TK);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_set_le_sc_remote_oob_data(bt_instance_t* ins, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_set_le_sc_remote_oob_data.addr, addr, sizeof(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.addr));
    memcpy(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.c_val, c_val, sizeof(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.c_val));
    memcpy(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.r_val, r_val, sizeof(packet.devs_pl._bt_device_set_le_sc_remote_oob_data.r_val));
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_SET_LE_SC_REMOTE_OOB_DATA);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_get_le_sc_local_oob_data(bt_instance_t* ins, bt_address_t* addr)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);
    memcpy(&packet.devs_pl._bt_device_get_le_sc_local_oob_data.addr, addr, sizeof(packet.devs_pl._bt_device_get_le_sc_local_oob_data.addr));

    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_GET_LE_SC_LOCAL_OOB_DATA);
    if (status != BT_STATUS_SUCCESS) {
        return status;
    }

    return packet.devs_r.status;
}

bt_status_t bt_device_enable_enhanced_mode(bt_instance_t* ins, bt_address_t* addr, bt_enhanced_mode_t mode)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_enable_enhanced_mode.addr, addr, sizeof(packet.devs_pl._bt_device_enable_enhanced_mode.addr));
    packet.devs_pl._bt_device_enable_enhanced_mode.mode = mode;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_ENABLE_ENHANCED_MODE);

    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.devs_r.status;
}

bt_status_t bt_device_disable_enhanced_mode(bt_instance_t* ins, bt_address_t* addr, bt_enhanced_mode_t mode)
{
    bt_message_packet_t packet;
    bt_status_t status;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.devs_pl._bt_device_disable_enhanced_mode.addr, addr, sizeof(packet.devs_pl._bt_device_disable_enhanced_mode.addr));
    packet.devs_pl._bt_device_disable_enhanced_mode.mode = mode;
    status = bt_socket_client_sendrecv(ins, &packet, BT_DEVICE_DISABLE_ENHANCED_MODE);

    if (status != BT_STATUS_SUCCESS)
        return status;

    return packet.devs_r.status;
}