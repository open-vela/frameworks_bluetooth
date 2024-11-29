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

#include "bt_adapter.h"
#include "bt_async.h"
#include "bt_socket.h"

typedef struct {
    void* userdata;
    void* cookie;
} bt_register_callback_data_t;

static void adapter_status_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_status_cb_t ret_cb = (bt_status_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, userdata);
}

static void adapter_bool_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_bool_cb_t ret_cb = (bt_bool_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_r.bbool, userdata);
}

static void adapter_uint16_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_u16_cb_t ret_cb = (bt_u16_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_r.v16, userdata);
}

static void adapter_uint32_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_u32_cb_t ret_cb = (bt_u32_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_r.v32, userdata);
}

static void adapter_get_state_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_adapter_get_state_cb_t ret_cb = (bt_adapter_get_state_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_r.state, userdata);
}

static void adapter_get_device_type_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_device_type_cb_t ret_cb = (bt_device_type_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_r.dtype, userdata);
}

static void adapter_get_address_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_address_cb_t ret_cb = (bt_address_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, &packet->adpt_pl._bt_adapter_get_address.addr, userdata);
}

static void adapter_get_name_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_string_cb_t ret_cb = (bt_string_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_pl._bt_adapter_get_name.name, userdata);
}

static void adapter_get_uuids_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_uuids_cb_t ret_cb = (bt_uuids_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_pl._bt_adapter_get_uuids.uuids, packet->adpt_pl._bt_adapter_get_uuids.size, userdata);
}

static void adapter_get_scan_mode_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_adapter_get_scan_mode_cb_t ret_cb = (bt_adapter_get_scan_mode_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_r.mode, userdata);
}

static void adapter_get_io_capability_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_adapter_get_io_capability_cb_t ret_cb = (bt_adapter_get_io_capability_cb_t)cb;

    if (ret_cb)
        ret_cb(ins, packet->adpt_r.status, packet->adpt_r.ioc, userdata);
}

static void adapter_get_devices_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_adapter_get_devices_cb_t ret_cb = (bt_adapter_get_devices_cb_t)cb;

    if (ret_cb) {
        ret_cb(ins, packet->adpt_r.status, packet->adpt_pl._bt_adapter_get_bonded_devices.addr,
            packet->adpt_pl._bt_adapter_get_bonded_devices.num, userdata);
    }
}

static void adapter_get_le_address_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_adapter_get_le_address_cb_t ret_cb = (bt_adapter_get_le_address_cb_t)cb;

    if (ret_cb) {
        ret_cb(ins, packet->adpt_r.status, &packet->adpt_pl._bt_adapter_get_le_address.addr,
            packet->adpt_pl._bt_adapter_get_le_address.type, userdata);
    }
}

static void adapter_register_callback_reply(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata)
{
    bt_register_callback_data_t* data = userdata;
    bt_socket_async_client_t* priv = ins->priv;
    bt_register_callback_cb_t ret_cb = (bt_register_callback_cb_t)cb;

    if (packet->adpt_r.status != BT_STATUS_SUCCESS || !ret_cb) {
        bt_callbacks_list_free(priv->adapter_callbacks);
        priv->adapter_callbacks = NULL;
    }

    if (ret_cb) {
        ret_cb(ins, packet->adpt_r.status, data->cookie, data->userdata);
    }

    free(data);
}

bt_status_t bt_adapter_register_callback_async(bt_instance_t* ins,
    const adapter_callbacks_t* adapter_cbs, bt_register_callback_cb_t cb, void* userdata)
{
    bt_register_callback_data_t* data;
    bt_socket_async_client_t* priv;
    bt_message_packet_t packet;
    bt_status_t status;
    void* handle;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    priv = ins->priv;
    if (!priv)
        return BT_STATUS_IPC_ERROR;

    if (priv->adapter_callbacks) {
        handle = bt_remote_callbacks_register(priv->adapter_callbacks, NULL, (void*)adapter_cbs);
        cb(ins, BT_STATUS_SUCCESS, handle, userdata);
        return BT_STATUS_SUCCESS;
    }

    priv->adapter_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    if (priv->adapter_callbacks == NULL)
        return BT_STATUS_NOMEM;

#ifdef CONFIG_BLUETOOTH_FEATURE
    handle = bt_remote_callbacks_register(priv->adapter_callbacks, ins, (void*)adapter_cbs);
#else
    handle = bt_remote_callbacks_register(priv->adapter_callbacks, NULL, (void*)adapter_cbs);
#endif

    if (handle == NULL) {
        bt_callbacks_list_free(priv->adapter_callbacks);
        priv->adapter_callbacks = NULL;
        return BT_STATUS_NO_RESOURCES;
    }

    data = calloc(1, sizeof(bt_register_callback_data_t));
    data->userdata = userdata;
    data->cookie = handle;

    status = bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_REGISTER_CALLBACK, adapter_register_callback_reply, cb, data);
    if (status != BT_STATUS_SUCCESS) {
        bt_callbacks_list_free(priv->adapter_callbacks);
        priv->adapter_callbacks = NULL;
        free(data);
        return BT_STATUS_FAIL;
    }

    return status;
}

bt_status_t bt_adapter_unregister_callback_async(bt_instance_t* ins, void* cookie, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;
    bt_socket_async_client_t* priv;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    priv = ins->priv;
    if (!priv || !priv->adapter_callbacks)
        return BT_STATUS_IPC_ERROR;

    bt_remote_callbacks_unregister(priv->adapter_callbacks, NULL, cookie);
    if (bt_callbacks_list_count(priv->adapter_callbacks) > 0) {
        return BT_STATUS_SUCCESS;
    }

    bt_callbacks_list_free(priv->adapter_callbacks);
    priv->adapter_callbacks = NULL;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_UNREGISTER_CALLBACK, adapter_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_enable_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_ENABLE, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_disable_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_DISABLE, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_enable_le_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_ENABLE_LE, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_disable_le_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_DISABLE_LE, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_state_async(bt_instance_t* ins, bt_adapter_get_state_cb_t get_state_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_STATE, adapter_get_state_reply, (void*)get_state_cb, userdata);
}

bt_status_t bt_adapter_is_le_enabled_async(bt_instance_t* ins, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_IS_LE_ENABLED, adapter_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_type_async(bt_instance_t* ins, bt_device_type_cb_t get_dtype_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_TYPE, adapter_get_device_type_reply, (void*)get_dtype_cb, userdata);
}

bt_status_t bt_adapter_set_discovery_filter_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_adapter_start_discovery_async(bt_instance_t* ins, uint32_t timeout, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_start_discovery.v32 = timeout;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_START_DISCOVERY, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_cancel_discovery_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_CANCEL_DISCOVERY, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_is_discovering_async(bt_instance_t* ins, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_IS_DISCOVERING, adapter_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_address_async(bt_instance_t* ins, bt_address_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_ADDRESS, adapter_get_address_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_set_name_async(bt_instance_t* ins, const char* name, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    if (strlen(name) > sizeof(packet.adpt_pl._bt_adapter_set_name.name))
        return BT_STATUS_PARM_INVALID;

    memset(packet.adpt_pl._bt_adapter_set_name.name, 0, sizeof(packet.adpt_pl._bt_adapter_set_name.name));
    strncpy(packet.adpt_pl._bt_adapter_set_name.name, name, sizeof(packet.adpt_pl._bt_adapter_set_name.name) - 1);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_NAME, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_name_async(bt_instance_t* ins, bt_string_cb_t get_name_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_NAME, adapter_get_name_reply, (void*)get_name_cb, userdata);
}

bt_status_t bt_adapter_get_uuids_async(bt_instance_t* ins, bt_uuids_cb_t get_uuids_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_UUIDS, adapter_get_uuids_reply, (void*)get_uuids_cb, userdata);
}

bt_status_t bt_adapter_set_scan_mode_async(bt_instance_t* ins, bt_scan_mode_t mode, bool bondable, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_scan_mode.mode = mode;
    packet.adpt_pl._bt_adapter_set_scan_mode.bondable = bondable;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_SCAN_MODE, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_scan_mode_async(bt_instance_t* ins, bt_adapter_get_scan_mode_cb_t get_scan_mode_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_SCAN_MODE, adapter_get_scan_mode_reply, (void*)get_scan_mode_cb, userdata);
}

bt_status_t bt_adapter_set_device_class_async(bt_instance_t* ins, uint32_t cod, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_device_class.v32 = cod;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_DEVICE_CLASS, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_device_class_async(bt_instance_t* ins, bt_u32_cb_t get_cod_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_DEVICE_CLASS, adapter_uint32_reply, (void*)get_cod_cb, userdata);
}

bt_status_t bt_adapter_set_io_capability_async(bt_instance_t* ins, bt_io_capability_t cap, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_io_capability.cap = cap;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_IO_CAPABILITY, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_io_capability_async(bt_instance_t* ins, bt_adapter_get_io_capability_cb_t get_ioc_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_IO_CAPABILITY, adapter_get_io_capability_reply, (void*)get_ioc_cb, userdata);
}

bt_status_t bt_adapter_set_inquiry_scan_parameters_async(bt_instance_t* ins, bt_scan_type_t type,
    uint16_t interval, uint16_t window, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_inquiry_scan_parameters.type = type;
    packet.adpt_pl._bt_adapter_set_inquiry_scan_parameters.interval = interval;
    packet.adpt_pl._bt_adapter_set_inquiry_scan_parameters.window = window;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_INQUIRY_SCAN_PARAMETERS, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_set_page_scan_parameters_async(bt_instance_t* ins, bt_scan_type_t type,
    uint16_t interval, uint16_t window, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_page_scan_parameters.type = type;
    packet.adpt_pl._bt_adapter_set_page_scan_parameters.interval = interval;
    packet.adpt_pl._bt_adapter_set_page_scan_parameters.window = window;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_PAGE_SCAN_PARAMETERS, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_set_le_io_capability_async(bt_instance_t* ins, uint32_t le_io_cap, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_le_io_capability.v32 = le_io_cap;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_LE_IO_CAPABILITY, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_le_io_capability_async(bt_instance_t* ins, bt_u32_cb_t get_le_ioc_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_LE_IO_CAPABILITY, adapter_uint32_reply, (void*)get_le_ioc_cb, userdata);
}

bt_status_t bt_adapter_get_le_address_async(bt_instance_t* ins, bt_adapter_get_le_address_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_LE_ADDRESS, adapter_get_le_address_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_set_le_address_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.adpt_pl._bt_adapter_set_le_address.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_LE_ADDRESS, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_set_le_identity_address_async(bt_instance_t* ins, bt_address_t* addr, bool public, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.adpt_pl._bt_adapter_set_le_identity_address.addr, addr, sizeof(*addr));
    packet.adpt_pl._bt_adapter_set_le_identity_address.pub = public;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_LE_IDENTITY_ADDRESS, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_set_le_appearance_async(bt_instance_t* ins, uint16_t appearance, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_le_appearance.v16 = appearance;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_LE_APPEARANCE, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_le_appearance_async(bt_instance_t* ins, bt_u16_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_LE_APPEARANCE, adapter_uint16_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_le_enable_key_derivation_async(bt_instance_t* ins,
    bool brkey_to_lekey, bool lekey_to_brkey, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_le_enable_key_derivation.brkey_to_lekey = brkey_to_lekey;
    packet.adpt_pl._bt_adapter_le_enable_key_derivation.lekey_to_brkey = lekey_to_brkey;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_LE_ENABLE_KEY_DERIVATION, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_le_add_whitelist_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.adpt_pl._bt_adapter_le_add_whitelist.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_LE_ADD_WHITELIST, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_le_remove_whitelist_async(bt_instance_t* ins, bt_address_t* addr, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    memcpy(&packet.adpt_pl._bt_adapter_le_remove_whitelist.addr, addr, sizeof(*addr));

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_LE_REMOVE_WHITELIST, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_get_bonded_devices_async(bt_instance_t* ins, bt_transport_t transport, bt_adapter_get_devices_cb_t get_bonded_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_get_bonded_devices.transport = transport;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_BONDED_DEVICES, adapter_get_devices_reply, (void*)get_bonded_cb, userdata);
}

bt_status_t bt_adapter_get_connected_devices_async(bt_instance_t* ins, bt_transport_t transport, bt_adapter_get_devices_cb_t get_connected_cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_get_connected_devices.transport = transport;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_GET_CONNECTED_DEVICES, adapter_get_devices_reply, (void*)get_connected_cb, userdata);
}

bt_status_t bt_adapter_set_afh_channel_classification_async(bt_instance_t* ins, uint16_t central_frequency,
    uint16_t band_width, uint16_t number, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    packet.adpt_pl._bt_adapter_set_afh_channel_classification.central_frequency = central_frequency;
    packet.adpt_pl._bt_adapter_set_afh_channel_classification.band_width = band_width;
    packet.adpt_pl._bt_adapter_set_afh_channel_classification.number = number;

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_SET_AFH_CHANNEL_CLASSFICATION, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_set_auto_sniff_async(bt_instance_t* ins, bt_auto_sniff_params_t* params, bt_status_cb_t cb, void* userdata)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_adapter_disconnect_all_devices_async(bt_instance_t* ins, bt_status_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_DISCONNECT_ALL_DEVICES, adapter_status_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_is_support_bredr_async(bt_instance_t* ins, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_IS_SUPPORT_BREDR, adapter_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_is_support_le_async(bt_instance_t* ins, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_IS_SUPPORT_LE, adapter_bool_reply, (void*)cb, userdata);
}

bt_status_t bt_adapter_is_support_leaudio_async(bt_instance_t* ins, bt_bool_cb_t cb, void* userdata)
{
    bt_message_packet_t packet;

    BT_SOCKET_INS_VALID(ins, BT_STATUS_PARM_INVALID);

    return bt_socket_client_send_with_reply(ins, &packet, BT_ADAPTER_IS_SUPPORT_LEAUDIO, adapter_bool_reply, (void*)cb, userdata);
}
