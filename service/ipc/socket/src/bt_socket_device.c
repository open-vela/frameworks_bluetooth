/****************************************************************************
 * service/ipc/socket/src/bt_socket_device.c
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
#include "bt_device.h"
#include "bt_message.h"
#include "bt_socket.h"
#include "callbacks_list.h"
#include "service_loop.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static bool socket_allocator(void** data, uint32_t size)
{
    *data = malloc(size);
    if (!(*data))
        return false;

    return true;
}
/****************************************************************************
 * Public Functions
 ****************************************************************************/

void bt_socket_server_device_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet)
{
    switch (packet->code) {
    case BT_DEVICE_GET_IDENTITY_ADDRESS: {
        bt_address_t id_addr;
        packet->devs_r.status = BTSYMBOLS(bt_device_get_identity_address)(ins,
            &packet->devs_pl._bt_device_addr.addr,
            &id_addr);
        memcpy(&packet->devs_pl._bt_device_addr.addr, &id_addr, sizeof(id_addr));
        break;
    }
    case BT_DEVICE_GET_ADDRESS_TYPE: {
        packet->devs_r.atype = BT_LE_ADDR_TYPE_PUBLIC;
        break;
    }
    case BT_DEVICE_GET_DEVICE_TYPE: {
        packet->devs_r.atype = BTSYMBOLS(bt_device_get_device_type)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_GET_NAME: {
        memset(packet->devs_pl._bt_device_get_name.name, 0, sizeof(packet->devs_pl._bt_device_get_name.name));
        packet->devs_r.bbool = BTSYMBOLS(bt_device_get_name)(ins,
            &packet->devs_pl._bt_device_get_name.addr,
            packet->devs_pl._bt_device_get_name.name,
            sizeof(packet->devs_pl._bt_device_get_name.name));
        break;
    }
    case BT_DEVICE_GET_DEVICE_CLASS: {
        packet->devs_r.v32 = BTSYMBOLS(bt_device_get_device_class)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_GET_UUIDS: {
        bt_uuid_t* uuid;
        packet->devs_r.status = BTSYMBOLS(bt_device_get_uuids)(ins,
            &packet->devs_pl._bt_device_get_uuids.addr,
            &uuid,
            &packet->devs_pl._bt_device_get_uuids.size, socket_allocator);

        if (packet->devs_pl._bt_device_get_uuids.size > 0) {
            if (packet->devs_pl._bt_device_get_uuids.size > nitems(packet->devs_pl._bt_device_get_uuids.uuids)) {
                packet->devs_pl._bt_device_get_uuids.size = nitems(packet->devs_pl._bt_device_get_uuids.uuids);
            }

            memcpy(packet->devs_pl._bt_device_get_uuids.uuids, uuid,
                sizeof(bt_uuid_t) * packet->devs_pl._bt_device_get_uuids.size);
            free(uuid);
        }
        break;
    }
    case BT_DEVICE_GET_APPEARANCE: {
        packet->devs_r.v16 = BTSYMBOLS(bt_device_get_appearance)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_GET_RSSI: {
        packet->devs_r.v8 = BTSYMBOLS(bt_device_get_rssi)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_GET_ALIAS: {
        memset(packet->devs_pl._bt_device_get_alias.alias, 0, sizeof(packet->devs_pl._bt_device_get_alias.alias));
        packet->devs_r.bbool = BTSYMBOLS(bt_device_get_alias)(ins,
            &packet->devs_pl._bt_device_get_alias.addr,
            packet->devs_pl._bt_device_get_alias.alias,
            sizeof(packet->devs_pl._bt_device_get_alias.alias));
        break;
    }
    case BT_DEVICE_SET_ALIAS: {
        packet->devs_r.status = BTSYMBOLS(bt_device_set_alias)(ins,
            &packet->devs_pl._bt_device_set_alias.addr,
            packet->devs_pl._bt_device_set_alias.alias);
        break;
    }
    case BT_DEVICE_IS_CONNECTED: {
        packet->devs_r.bbool = BTSYMBOLS(bt_device_is_connected)(ins,
            &packet->devs_pl._bt_device_addr.addr,
            packet->devs_pl._bt_device_is_connected.transport);
        break;
    }
    case BT_DEVICE_IS_ENCRYPTED: {
        packet->devs_r.bbool = BTSYMBOLS(bt_device_is_encrypted)(ins,
            &packet->devs_pl._bt_device_addr.addr,
            packet->devs_pl._bt_device_is_encrypted.transport);
        break;
    }
    case BT_DEVICE_IS_BOND_INITIATE_LOCAL: {
        packet->devs_r.bbool = BTSYMBOLS(bt_device_is_bond_initiate_local)(ins,
            &packet->devs_pl._bt_device_addr.addr,
            packet->devs_pl._bt_device_is_bond_initiate_local.transport);
        break;
    }
    case BT_DEVICE_GET_BOND_STATE: {
        packet->devs_r.bstate = BTSYMBOLS(bt_device_get_bond_state)(ins,
            &packet->devs_pl._bt_device_addr.addr,
            packet->devs_pl._bt_device_get_bond_state.transport);
        break;
    }
    case BT_DEVICE_IS_BONDED: {
        packet->devs_r.bbool = BTSYMBOLS(bt_device_is_bonded)(ins,
            &packet->devs_pl._bt_device_addr.addr,
            packet->devs_pl._bt_device_is_bonded.transport);
        break;
    }
    case BT_DEVICE_CREATE_BOND: {
        packet->devs_r.status = BTSYMBOLS(bt_device_create_bond)(ins,
            &packet->devs_pl._bt_device_create_bond.addr,
            packet->devs_pl._bt_device_create_bond.transport);
        break;
    }
    case BT_DEVICE_REMOVE_BOND: {
        packet->devs_r.status = BTSYMBOLS(bt_device_remove_bond)(ins,
            &packet->devs_pl._bt_device_remove_bond.addr,
            packet->devs_pl._bt_device_remove_bond.transport);
        break;
    }
    case BT_DEVICE_CANCEL_BOND: {
        packet->devs_r.status = BTSYMBOLS(bt_device_cancel_bond)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_PAIR_REQUEST_REPLY: {
        packet->devs_r.status = BTSYMBOLS(bt_device_pair_request_reply)(ins,
            &packet->devs_pl._bt_device_pair_request_reply.addr,
            packet->devs_pl._bt_device_pair_request_reply.accept);
        break;
    }
    case BT_DEVICE_SET_PAIRING_CONFIRMATION: {
        packet->devs_r.status = BTSYMBOLS(bt_device_set_pairing_confirmation)(ins,
            &packet->devs_pl._bt_device_set_pairing_confirmation.addr,
            packet->devs_pl._bt_device_set_pairing_confirmation.transport,
            packet->devs_pl._bt_device_set_pairing_confirmation.accept);
        break;
    }
    case BT_DEVICE_SET_PIN_CODE: {
        packet->devs_r.status = BTSYMBOLS(bt_device_set_pin_code)(ins,
            &packet->devs_pl._bt_device_set_pin_code.addr,
            packet->devs_pl._bt_device_set_pin_code.accept,
            (char*)packet->devs_pl._bt_device_set_pin_code.pincode,
            packet->devs_pl._bt_device_set_pin_code.len);
        break;
    }
    case BT_DEVICE_SET_PASS_KEY: {
        packet->devs_r.status = BTSYMBOLS(bt_device_set_pass_key)(ins,
            &packet->devs_pl._bt_device_set_pass_key.addr,
            packet->devs_pl._bt_device_set_pass_key.transport,
            packet->devs_pl._bt_device_set_pass_key.accept,
            packet->devs_pl._bt_device_set_pass_key.passkey);
        break;
    }
    case BT_DEVICE_SET_LE_LEGACY_TK: {
        packet->devs_r.status = BTSYMBOLS(bt_device_set_le_legacy_tk)(ins,
            &packet->devs_pl._bt_device_set_le_legacy_tk.addr,
            packet->devs_pl._bt_device_set_le_legacy_tk.tk_val);
        break;
    }
    case BT_DEVICE_SET_LE_SC_REMOTE_OOB_DATA: {
        packet->devs_r.status = BTSYMBOLS(bt_device_set_le_sc_remote_oob_data)(ins,
            &packet->devs_pl._bt_device_set_le_sc_remote_oob_data.addr,
            packet->devs_pl._bt_device_set_le_sc_remote_oob_data.c_val,
            packet->devs_pl._bt_device_set_le_sc_remote_oob_data.r_val);
        break;
    }
    case BT_DEVICE_GET_LE_SC_LOCAL_OOB_DATA: {
        packet->devs_r.status = BTSYMBOLS(bt_device_get_le_sc_local_oob_data)(ins,
            &packet->devs_pl._bt_device_get_le_sc_local_oob_data.addr);
        break;
    }
    case BT_DEVICE_CONNECT: {
        packet->devs_r.status = BTSYMBOLS(bt_device_connect)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_DISCONNECT: {
        packet->devs_r.status = BTSYMBOLS(bt_device_disconnect)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_BACKGROUND_CONNECT: {
        packet->devs_r.status = BTSYMBOLS(bt_device_background_connect)(ins,
            &packet->devs_pl._bt_device_background_connect.addr,
            packet->devs_pl._bt_device_background_connect.transport);
        break;
    }
    case BT_DEVICE_BACKGROUND_DISCONNECT: {
        packet->devs_r.status = BTSYMBOLS(bt_device_background_disconnect)(ins,
            &packet->devs_pl._bt_device_background_disconnect.addr,
            packet->devs_pl._bt_device_background_disconnect.transport);
        break;
    }
    case BT_DEVICE_CONNECT_LE: {
        packet->devs_r.status = BTSYMBOLS(bt_device_connect_le)(ins,
            &packet->devs_pl._bt_device_connect_le.addr,
            packet->devs_pl._bt_device_connect_le.type,
            &packet->devs_pl._bt_device_connect_le.param);
        break;
    }
    case BT_DEVICE_DISCONNECT_LE: {
        packet->devs_r.status = BTSYMBOLS(bt_device_disconnect_le)(ins,
            &packet->devs_pl._bt_device_addr.addr);
        break;
    }
    case BT_DEVICE_CONNECT_REQUEST_REPLY: {
        packet->devs_r.status = BTSYMBOLS(bt_device_connect_request_reply)(ins,
            &packet->devs_pl._bt_device_connect_request_reply.addr,
            packet->devs_pl._bt_device_connect_request_reply.accept);
        break;
    }
    case BT_DEVICE_SET_LE_PHY: {
        packet->devs_r.status = BTSYMBOLS(bt_device_set_le_phy)(ins,
            &packet->devs_pl._bt_device_set_le_phy.addr,
            packet->devs_pl._bt_device_set_le_phy.tx_phy,
            packet->devs_pl._bt_device_set_le_phy.rx_phy);
        break;
    }
    case BT_DEVICE_ENABLE_ENHANCED_MODE: {
        packet->devs_r.status = BTSYMBOLS(bt_device_enable_enhanced_mode)(ins,
            &packet->devs_pl._bt_device_enable_enhanced_mode.addr,
            packet->devs_pl._bt_device_enable_enhanced_mode.mode);
        break;
    }
    case BT_DEVICE_DISABLE_ENHANCED_MODE: {
        packet->devs_r.status = BTSYMBOLS(bt_device_disable_enhanced_mode)(ins,
            &packet->devs_pl._bt_device_disable_enhanced_mode.addr,
            packet->devs_pl._bt_device_enable_enhanced_mode.mode);
        break;
    }
    case BT_DEVICE_CONNECT_ALL_PROFILE:
    case BT_DEVICE_DISCONNECT_ALL_PROFILE:
    default:
        packet->devs_r.status = BT_STATUS_NOT_SUPPORTED;
        break;
    }
}
