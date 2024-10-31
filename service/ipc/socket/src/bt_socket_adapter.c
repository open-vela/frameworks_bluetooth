/****************************************************************************
 * service/ipc/socket/src/bt_socket_adapter.c
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

#include "adapter_internel.h"
#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) \
    BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)
#define CBLIST (__async ? __async->adapter_callbacks : ins->adapter_callbacks)

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#if defined(CONFIG_BLUETOOTH_SERVER) && defined(__NuttX__)
static bool socket_allocator(void** data, uint32_t size)
{
    *data = malloc(size);
    if (!(*data))
        return false;

    return true;
}

static void on_adapter_state_changed_cb(void* cookie, bt_adapter_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    packet.adpt_cb._on_adapter_state_changed.state = state;
    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_ADAPTER_STATE_CHANGED);
}

static void on_discovery_state_changed_cb(void* cookie, bt_discovery_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    packet.adpt_cb._on_discovery_state_changed.state = state;
    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_DISCOVERY_STATE_CHANGED);
}

static void on_discovery_result_cb(void* cookie, bt_discovery_result_t* result)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_discovery_result.result, result, sizeof(*result));
    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_DISCOVERY_RESULT);
}

static void on_scan_mode_changed_cb(void* cookie, bt_scan_mode_t mode)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    packet.adpt_cb._on_scan_mode_changed.mode = mode;
    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_SCAN_MODE_CHANGED);
}

static void on_device_name_changed_cb(void* cookie, const char* device_name)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    strncpy(packet.adpt_cb._on_device_name_changed.device_name, device_name,
        sizeof(packet.adpt_cb._on_device_name_changed.device_name) - 1);
    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_DEVICE_NAME_CHANGED);
}

static void on_pair_request_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_pair_request.addr, addr, sizeof(bt_address_t));
    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_PAIR_REQUEST);
}

static void on_pair_display_cb(void* cookie, bt_address_t* addr,
    bt_transport_t transport, bt_pair_type_t type, uint32_t passkey)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_pair_display.addr, addr, sizeof(bt_address_t));
    packet.adpt_cb._on_pair_display.transport = transport;
    packet.adpt_cb._on_pair_display.type = type;
    packet.adpt_cb._on_pair_display.passkey = passkey;

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_PAIR_DISPLAY);
}

static void on_connect_request_cb(void* cookie, bt_address_t* addr)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_connect_request.addr, addr, sizeof(bt_address_t));

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_CONNECT_REQUEST);
}

static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr,
    bt_transport_t transport, connection_state_t state)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_connection_state_changed.addr, addr, sizeof(bt_address_t));
    packet.adpt_cb._on_connection_state_changed.transport = transport;
    packet.adpt_cb._on_connection_state_changed.state = state;

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_CONNECTION_STATE_CHANGED);
}

static void on_bond_state_changed_cb(void* cookie, bt_address_t* addr,
    bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_bond_state_changed.addr, addr, sizeof(bt_address_t));
    packet.adpt_cb._on_bond_state_changed.transport = transport;
    packet.adpt_cb._on_bond_state_changed.state = state;
    packet.adpt_cb._on_bond_state_changed.is_ctkd = is_ctkd;

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_BOND_STATE_CHANGED);
}

static void on_le_sc_local_oob_data_got_cb(void* cookie, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_le_sc_local_oob_data_got.addr, addr, sizeof(bt_address_t));
    memcpy(packet.adpt_cb._on_le_sc_local_oob_data_got.c_val, c_val, sizeof(bt_128key_t));
    memcpy(packet.adpt_cb._on_le_sc_local_oob_data_got.r_val, r_val, sizeof(bt_128key_t));

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_LE_SC_LOCAL_OOB_DATA_GOT);
}

static void on_remote_name_changed_cb(void* cookie, bt_address_t* addr, const char* name)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_remote_name_changed.addr, addr, sizeof(bt_address_t));
    strncpy(packet.adpt_cb._on_remote_name_changed.name, name,
        sizeof(packet.adpt_cb._on_remote_name_changed.name) - 1);

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_REMOTE_NAME_CHANGED);
}

static void on_remote_alias_changed_cb(void* cookie, bt_address_t* addr, const char* alias)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_remote_alias_changed.addr, addr, sizeof(bt_address_t));
    strncpy(packet.adpt_cb._on_remote_alias_changed.alias, alias,
        sizeof(packet.adpt_cb._on_remote_alias_changed.alias) - 1);

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_REMOTE_ALIAS_CHANGED);
}

static void on_remote_cod_changed_cb(void* cookie, bt_address_t* addr, uint32_t cod)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;

    memcpy(&packet.adpt_cb._on_remote_cod_changed.addr, addr, sizeof(bt_address_t));
    packet.adpt_cb._on_remote_cod_changed.cod = cod;

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_REMOTE_COD_CHANGED);
}

static void on_remote_uuids_changed_cb(void* cookie, bt_address_t* addr, bt_uuid_t* uuids, uint16_t size)
{
    bt_message_packet_t packet = { 0 };
    bt_instance_t* ins = cookie;
    uint16_t max_uuid_num;
    uint16_t uuid_num;

    max_uuid_num = sizeof(packet.adpt_cb._on_remote_uuids_changed.uuids) / sizeof(bt_uuid_t);
    uuid_num = size < max_uuid_num ? size : max_uuid_num;

    memcpy(&packet.adpt_cb._on_remote_uuids_changed.addr, addr, sizeof(bt_address_t));
    memcpy(packet.adpt_cb._on_remote_uuids_changed.uuids, uuids, sizeof(bt_uuid_t) * uuid_num);
    packet.adpt_cb._on_remote_uuids_changed.size = uuid_num;

    bt_socket_server_send(ins, &packet, BT_ADAPTER_ON_REMOTE_UUIDS_CHANGED);
}

const static adapter_callbacks_t g_adapter_socket_cbs = {
    .on_adapter_state_changed = on_adapter_state_changed_cb,
    .on_discovery_state_changed = on_discovery_state_changed_cb,
    .on_discovery_result = on_discovery_result_cb,
    .on_scan_mode_changed = on_scan_mode_changed_cb,
    .on_device_name_changed = on_device_name_changed_cb,
    .on_pair_request = on_pair_request_cb,
    .on_pair_display = on_pair_display_cb,
    .on_connect_request = on_connect_request_cb,
    .on_connection_state_changed = on_connection_state_changed_cb,
    .on_bond_state_changed = on_bond_state_changed_cb,
    .on_le_sc_local_oob_data_got = on_le_sc_local_oob_data_got_cb,
    .on_remote_name_changed = on_remote_name_changed_cb,
    .on_remote_alias_changed = on_remote_alias_changed_cb,
    .on_remote_cod_changed = on_remote_cod_changed_cb,
    .on_remote_uuids_changed = on_remote_uuids_changed_cb,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_adapter_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_ADAPTER_ENABLE: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_enable)(ins);
        break;
    }
    case BT_ADAPTER_DISABLE: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_disable)(ins);
        break;
    }
    case BT_ADAPTER_ENABLE_LE: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_enable_le)(ins);
        break;
    }
    case BT_ADAPTER_DISABLE_LE: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_disable_le)(ins);
        break;
    }
    case BT_ADAPTER_GET_STATE: {
        packet->adpt_r.state = BTSYMBOLS(bt_adapter_get_state)(ins);
        break;
    }
    case BT_ADAPTER_GET_TYPE: {
        packet->adpt_r.dtype = BTSYMBOLS(bt_adapter_get_type)(ins);
        break;
    }
    case BT_ADAPTER_IS_LE_ENABLED: {
        packet->adpt_r.dtype = BTSYMBOLS(bt_adapter_is_le_enabled)(ins);
        break;
    }
    case BT_ADAPTER_SET_DISCOVERY_FILTER: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_discovery_filter)(ins);
        break;
    }
    case BT_ADAPTER_START_DISCOVERY: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_start_discovery)(ins,
            packet->adpt_pl._bt_adapter_start_discovery.v32);
        break;
    }
    case BT_ADAPTER_CANCEL_DISCOVERY: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_cancel_discovery)(ins);
        break;
    }
    case BT_ADAPTER_IS_DISCOVERING: {
        packet->adpt_r.bbool = BTSYMBOLS(bt_adapter_is_discovering)(ins);
        break;
    }
    case BT_ADAPTER_GET_ADDRESS: {
        BTSYMBOLS(bt_adapter_get_address)
        (ins,
            &packet->adpt_pl._bt_adapter_get_address.addr);
        break;
    }
    case BT_ADAPTER_SET_NAME: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_name)(ins,
            packet->adpt_pl._bt_adapter_set_name.name);
        break;
    }
    case BT_ADAPTER_GET_NAME: {
        BTSYMBOLS(bt_adapter_get_name)
        (ins,
            packet->adpt_pl._bt_adapter_get_name.name,
            sizeof(packet->adpt_pl._bt_adapter_get_name.name));
        break;
    }
    case BT_ADAPTER_GET_UUIDS: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_get_uuids)(ins,
            packet->adpt_pl._bt_adapter_get_uuids.uuids,
            &packet->adpt_pl._bt_adapter_get_uuids.size);
        break;
    }
    case BT_ADAPTER_SET_SCAN_MODE: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_scan_mode)(ins,
            packet->adpt_pl._bt_adapter_set_scan_mode.mode,
            packet->adpt_pl._bt_adapter_set_scan_mode.bondable);
        break;
    }
    case BT_ADAPTER_GET_SCAN_MODE: {
        packet->adpt_r.mode = BTSYMBOLS(bt_adapter_get_scan_mode)(ins);
        break;
    }
    case BT_ADAPTER_SET_DEVICE_CLASS: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_device_class)(ins,
            packet->adpt_pl._bt_adapter_set_device_class.v32);
        break;
    }
    case BT_ADAPTER_GET_DEVICE_CLASS: {
        packet->adpt_r.v32 = BTSYMBOLS(bt_adapter_get_device_class)(ins);
        break;
    }
    case BT_ADAPTER_SET_IO_CAPABILITY: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_io_capability)(ins,
            packet->adpt_pl._bt_adapter_set_io_capability.cap);
        break;
    }
    case BT_ADAPTER_GET_IO_CAPABILITY: {
        packet->adpt_r.ioc = BTSYMBOLS(bt_adapter_get_io_capability)(ins);
        break;
    }
    case BT_ADAPTER_SET_INQUIRY_SCAN_PARAMETERS: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_inquiry_scan_parameters)(ins,
            packet->adpt_pl._bt_adapter_set_inquiry_scan_parameters.type,
            packet->adpt_pl._bt_adapter_set_inquiry_scan_parameters.interval,
            packet->adpt_pl._bt_adapter_set_inquiry_scan_parameters.window);
        break;
    }
    case BT_ADAPTER_SET_PAGE_SCAN_PARAMETERS: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_page_scan_parameters)(ins,
            packet->adpt_pl._bt_adapter_set_page_scan_parameters.type,
            packet->adpt_pl._bt_adapter_set_page_scan_parameters.interval,
            packet->adpt_pl._bt_adapter_set_page_scan_parameters.window);
        break;
    }
    case BT_ADAPTER_GET_BONDED_DEVICES: {
        bt_address_t* addr;
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_get_bonded_devices)(ins,
            packet->adpt_pl._bt_adapter_get_bonded_devices.transport,
            &addr,
            (int*)&packet->adpt_pl._bt_adapter_get_bonded_devices.num, socket_allocator);

        if (packet->adpt_pl._bt_adapter_get_bonded_devices.num > 0) {
            if (packet->adpt_pl._bt_adapter_get_bonded_devices.num > nitems(packet->adpt_pl._bt_adapter_get_bonded_devices.addr)) {
                packet->adpt_pl._bt_adapter_get_bonded_devices.num = nitems(packet->adpt_pl._bt_adapter_get_bonded_devices.addr);
            }

            memcpy(packet->adpt_pl._bt_adapter_get_bonded_devices.addr, addr,
                sizeof(bt_address_t) * packet->adpt_pl._bt_adapter_get_bonded_devices.num);
            free(addr);
        }
        break;
    }
    case BT_ADAPTER_GET_CONNECTED_DEVICES: {
        bt_address_t* addr;
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_get_connected_devices)(ins,
            packet->adpt_pl._bt_adapter_get_connected_devices.transport,
            &addr,
            (int*)&packet->adpt_pl._bt_adapter_get_connected_devices.num, socket_allocator);
        if (packet->adpt_pl._bt_adapter_get_connected_devices.num > 0) {
            if (packet->adpt_pl._bt_adapter_get_connected_devices.num > nitems(packet->adpt_pl._bt_adapter_get_connected_devices.addr)) {
                packet->adpt_pl._bt_adapter_get_connected_devices.num = nitems(packet->adpt_pl._bt_adapter_get_connected_devices.addr);
            }
            memcpy(packet->adpt_pl._bt_adapter_get_connected_devices.addr, addr,
                sizeof(bt_address_t) * packet->adpt_pl._bt_adapter_get_connected_devices.num);
            free(addr);
        }
        break;
    }
    case BT_ADAPTER_SET_AFH_CHANNEL_CLASSFICATION: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_afh_channel_classification)(ins,
            packet->adpt_pl._bt_adapter_set_afh_channel_classification.central_frequency,
            packet->adpt_pl._bt_adapter_set_afh_channel_classification.band_width,
            packet->adpt_pl._bt_adapter_set_afh_channel_classification.number);
        break;
    }
    case BT_ADAPTER_DISCONNECT_ALL_DEVICES: {
        BTSYMBOLS(bt_adapter_disconnect_all_devices)
        (ins);
        break;
    }
    case BT_ADAPTER_IS_SUPPORT_BREDR: {
        packet->adpt_r.bbool = BTSYMBOLS(bt_adapter_is_support_bredr)(ins);
        break;
    }
    case BT_ADAPTER_IS_SUPPORT_LE: {
        packet->adpt_r.bbool = BTSYMBOLS(bt_adapter_is_support_le)(ins);
        break;
    }
    case BT_ADAPTER_IS_SUPPORT_LEAUDIO: {
        packet->adpt_r.bbool = BTSYMBOLS(bt_adapter_is_support_leaudio)(ins);
        break;
    }
    case BT_ADAPTER_GET_LE_ADDRESS: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_get_le_address)(ins,
            &packet->adpt_pl._bt_adapter_get_le_address.addr,
            INT2PTR(ble_addr_type_t*) & packet->adpt_pl._bt_adapter_get_le_address.type);
        break;
    }
    case BT_ADAPTER_SET_LE_ADDRESS: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_le_address)(ins,
            &packet->adpt_pl._bt_adapter_set_le_address.addr);
        break;
    }
    case BT_ADAPTER_SET_LE_IDENTITY_ADDRESS: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_le_identity_address)(ins,
            &packet->adpt_pl._bt_adapter_set_le_address.addr,
            packet->adpt_pl._bt_adapter_set_le_identity_address.pub);
        break;
    }
    case BT_ADAPTER_SET_LE_IO_CAPABILITY: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_le_io_capability)(ins,
            packet->adpt_pl._bt_adapter_set_le_io_capability.v32);
        break;
    }
    case BT_ADAPTER_GET_LE_IO_CAPABILITY: {
        packet->adpt_r.v32 = BTSYMBOLS(bt_adapter_get_le_io_capability)(ins);
        break;
    }
    case BT_ADAPTER_SET_LE_APPEARANCE: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_set_le_appearance)(ins,
            packet->adpt_pl._bt_adapter_set_le_appearance.v16);
        break;
    }
    case BT_ADAPTER_GET_LE_APPEARANCE: {
        packet->adpt_r.v16 = BTSYMBOLS(bt_adapter_get_le_appearance)(ins);
        break;
    }
    case BT_ADAPTER_LE_ENABLE_KEY_DERIVATION: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_le_enable_key_derivation)(ins,
            packet->adpt_pl._bt_adapter_le_enable_key_derivation.brkey_to_lekey,
            packet->adpt_pl._bt_adapter_le_enable_key_derivation.lekey_to_brkey);
        break;
    }
    case BT_ADAPTER_LE_REMOVE_WHITELIST: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_le_remove_whitelist)(ins,
            &packet->adpt_pl._bt_adapter_le_add_whitelist.addr);
        break;
    }
    case BT_ADAPTER_LE_ADD_WHITELIST: {
        packet->adpt_r.status = BTSYMBOLS(bt_adapter_le_add_whitelist)(ins,
            &packet->adpt_pl._bt_adapter_le_remove_whitelist.addr);
        break;
    }
    case BT_ADAPTER_REGISTER_CALLBACK: {
        if (ins->adapter_cookie == NULL) {
            ins->adapter_cookie = adapter_register_callback(ins, (void*)&g_adapter_socket_cbs);
            if (ins->adapter_cookie) {
                packet->adpt_r.status = BT_STATUS_SUCCESS;
            } else {
                packet->adpt_r.status = BT_STATUS_FAIL;
            }
        } else {
            packet->adpt_r.status = BT_STATUS_SUCCESS;
        }
        break;
    }
    case BT_ADAPTER_UNREGISTER_CALLBACK: {
        if (ins->adapter_cookie) {
            if (adapter_unregister_callback((void**)&ins, ins->adapter_cookie)) {
                packet->adpt_r.status = BT_STATUS_SUCCESS;
            } else {
                packet->adpt_r.status = BT_STATUS_FAIL;
            }
            ins->adapter_cookie = NULL;
        } else {
            packet->adpt_r.status = BT_STATUS_NOT_FOUND;
        }
        break;
    }
    default:
        break;
    }
}
#endif

int bt_socket_client_adapter_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async)
{
    bt_socket_async_client_t* __async = NULL;

    if (is_async)
        __async = ins->priv;

    switch (packet->code) {
    case BT_ADAPTER_ON_ADAPTER_STATE_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_adapter_state_changed,
            packet->adpt_cb._on_adapter_state_changed.state);
        break;
    }
    case BT_ADAPTER_ON_DISCOVERY_STATE_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_discovery_state_changed,
            packet->adpt_cb._on_discovery_state_changed.state);
        break;
    }
    case BT_ADAPTER_ON_DISCOVERY_RESULT: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_discovery_result,
            &packet->adpt_cb._on_discovery_result.result);
        break;
    }
    case BT_ADAPTER_ON_SCAN_MODE_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_scan_mode_changed,
            packet->adpt_cb._on_scan_mode_changed.mode);
        break;
    }
    case BT_ADAPTER_ON_DEVICE_NAME_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_device_name_changed,
            packet->adpt_cb._on_device_name_changed.device_name);
        break;
    }
    case BT_ADAPTER_ON_PAIR_REQUEST: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_pair_request,
            &packet->adpt_cb._on_pair_request.addr);
        break;
    }
    case BT_ADAPTER_ON_PAIR_DISPLAY: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_pair_display,
            &packet->adpt_cb._on_pair_display.addr,
            packet->adpt_cb._on_pair_display.transport,
            packet->adpt_cb._on_pair_display.type,
            packet->adpt_cb._on_pair_display.passkey);

        break;
    }
    case BT_ADAPTER_ON_CONNECT_REQUEST: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_connect_request,
            &packet->adpt_cb._on_connect_request.addr);
        break;
    }
    case BT_ADAPTER_ON_CONNECTION_STATE_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_connection_state_changed,
            &packet->adpt_cb._on_connection_state_changed.addr,
            packet->adpt_cb._on_connection_state_changed.transport,
            packet->adpt_cb._on_connection_state_changed.state);
        break;
    }
    case BT_ADAPTER_ON_BOND_STATE_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_bond_state_changed,
            &packet->adpt_cb._on_bond_state_changed.addr,
            packet->adpt_cb._on_bond_state_changed.transport,
            packet->adpt_cb._on_bond_state_changed.state,
            packet->adpt_cb._on_bond_state_changed.is_ctkd);
        break;
    }
    case BT_ADAPTER_ON_LE_SC_LOCAL_OOB_DATA_GOT: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_le_sc_local_oob_data_got,
            &packet->adpt_cb._on_le_sc_local_oob_data_got.addr,
            packet->adpt_cb._on_le_sc_local_oob_data_got.c_val,
            packet->adpt_cb._on_le_sc_local_oob_data_got.r_val);
        break;
    }
    case BT_ADAPTER_ON_REMOTE_NAME_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_remote_name_changed,
            &packet->adpt_cb._on_remote_name_changed.addr,
            packet->adpt_cb._on_remote_name_changed.name);
        break;
    }
    case BT_ADAPTER_ON_REMOTE_ALIAS_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_remote_alias_changed,
            &packet->adpt_cb._on_remote_alias_changed.addr,
            packet->adpt_cb._on_remote_alias_changed.alias);
        break;
    }
    case BT_ADAPTER_ON_REMOTE_COD_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_remote_cod_changed,
            &packet->adpt_cb._on_remote_cod_changed.addr,
            packet->adpt_cb._on_remote_cod_changed.cod);
        break;
    }
    case BT_ADAPTER_ON_REMOTE_UUIDS_CHANGED: {
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t,
            on_remote_uuids_changed,
            &packet->adpt_cb._on_remote_uuids_changed.addr,
            packet->adpt_cb._on_remote_uuids_changed.uuids,
            packet->adpt_cb._on_remote_uuids_changed.size);
        break;
    }
    default:
        return BT_STATUS_PARM_INVALID;
    }

    return BT_STATUS_SUCCESS;
}
