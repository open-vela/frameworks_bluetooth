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
#define LOG_TAG "device"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adapter_internel.h"
#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_addr.h"
#include "bt_config.h"
#include "bt_device.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "bt_uuid.h"
#include "device.h"
#include "utils/log.h"

#define BASE_UUID16_OFFSET 12

typedef struct remote_device {
    char name[BT_REM_NAME_MAX_LEN + 1];
    char alias[BT_REM_NAME_MAX_LEN + 1];
    bt_address_t addr;
    ble_addr_type_t addr_type;
    bt_link_role_t local_role;
    uint32_t device_class;
    bt_transport_t transport;
    bt_device_type_t device_type;
    uint32_t acl_handle;
    uint32_t sco_handle;
    int8_t rssi;
    struct {
        bt_uuid_t* uuids;
        uint16_t uuid_cnt;
    } uuids;
    uint8_t battery_level;
    uint8_t hfp_volume;
    uint8_t media_volume;
    connection_state_t connection_state;
    bond_state_t bond_state;
    bool local_initiate_bond;
    bt_128key_t link_key;
    bt_link_key_type_t link_key_type;
    bt_link_policy_t link_policy;
    bt_address_t identity_addr;
    uint16_t appearance;
    uint8_t smp_data[80];
    ble_phy_type_t tx_phy;
    ble_phy_type_t rx_phy;
    // uint8_t scan_repetition_mode;
    // uint16_t clock_offset;
} remote_device_t;

typedef struct bt_device {
    remote_device_t remote;
    bool is_temporary;
    uint32_t flags;
    uint8_t create_type;
} bt_device_t;

static bt_device_t* device_create(bt_address_t* addr, bt_transport_t transport, ble_addr_type_t addr_type)
{
    bt_device_t* device = zalloc(sizeof(bt_device_t));

    if (!device)
        return NULL;

    memset(device, 0, sizeof(bt_device_t));

    strcpy((char*)device->remote.name, "");
    strcpy((char*)device->remote.alias, "");
    memcpy(&device->remote.addr, addr, sizeof(bt_address_t));
    bt_addr_set_empty(&device->remote.identity_addr);
    device->remote.transport = transport;
    device->remote.addr_type = addr_type;
    device->remote.connection_state = CONNECTION_STATE_DISCONNECTED;
    device->remote.local_role = BT_LINK_ROLE_UNKNOWN;
    device->remote.bond_state = BOND_STATE_NONE;
    device->remote.uuids.uuids = NULL;
    device->remote.uuids.uuid_cnt = 0;
    device->remote.link_policy = BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH_AND_SNIFF;
    device->is_temporary = true;

    return device;
}

bt_device_t* br_device_create(bt_address_t* addr)
{
    return device_create(addr, BT_TRANSPORT_BREDR, BT_LE_ADDR_TYPE_UNKNOWN);
}

bt_device_t* le_device_create(bt_address_t* addr, ble_addr_type_t addr_type)
{
    return device_create(addr, BT_TRANSPORT_BLE, addr_type);
}

void device_delete(bt_device_t* device)
{
    if (device->remote.uuids.uuids)
        free(device->remote.uuids.uuids);
    free(device);
}

bt_transport_t device_get_transport(bt_device_t* device)
{
    return device->remote.transport;
}

bt_address_t* device_get_address(bt_device_t* device)
{
    return &device->remote.addr;
}

bt_address_t* device_get_identity_address(bt_device_t* device)
{
    return &device->remote.identity_addr;
}

void device_set_identity_address(bt_device_t* device, bt_address_t* addr)
{
    if (addr) {
        memcpy(&device->remote.identity_addr, addr, sizeof(bt_address_t));
    } else {
        bt_addr_set_empty(&device->remote.identity_addr);
    }
}

ble_addr_type_t device_get_address_type(bt_device_t* device)
{
    return device->remote.addr_type;
}

void device_set_address_type(bt_device_t* device, ble_addr_type_t type)
{
    device->remote.addr_type = type;
}

void device_set_device_type(bt_device_t* device, bt_device_type_t type)
{
    device->remote.device_type = type;
}

bt_device_type_t device_get_device_type(bt_device_t* device)
{
    return device->remote.device_type;
}

const char* device_get_name(bt_device_t* device)
{
    return (const char*)device->remote.name;
}

bool device_set_name(bt_device_t* device, const char* name)
{
    if (!strncmp(device->remote.name, name, BT_REM_NAME_MAX_LEN)) {
        return false;
    }

    strlcpy((char*)device->remote.name, name, sizeof(device->remote.name));
    if (!strncmp(device->remote.alias, "", BT_REM_NAME_MAX_LEN))
        strlcpy((char*)device->remote.alias, name, sizeof(device->remote.alias));

    device_set_flags(device, DFLAG_NAME_SET);

    return true;
}

uint32_t device_get_device_class(bt_device_t* device)
{
    return device->remote.device_class;
}

bool device_set_device_class(bt_device_t* device, uint32_t cod)
{
    if (device->remote.device_class == cod) {
        return false;
    }

    device->remote.device_class = cod;
    return true;
}

uint16_t device_get_uuids_size(bt_device_t* device)
{
    return device->remote.uuids.uuid_cnt;
}

uint16_t device_get_uuids(bt_device_t* device, bt_uuid_t* uuids, uint16_t size)
{
    if (!device->remote.uuids.uuid_cnt)
        return 0;

    uint16_t min = size > device->remote.uuids.uuid_cnt ? device->remote.uuids.uuid_cnt : size;
    memcpy(uuids, device->remote.uuids.uuids, sizeof(bt_uuid_t) * min);

    return min;
}

bool device_set_uuids(bt_device_t* device, bt_uuid_t* uuids, uint16_t size)
{
    bool update = true;

    if (!device->remote.uuids.uuid_cnt)
        goto copy;

    /* check uuid list is equal */
    if (device->remote.uuids.uuid_cnt == size) {
        bt_uuid_t* uuid1 = uuids;
        for (int i = 0; i < size; i++) {
            update = true;
            bt_uuid_t* uuid2 = device->remote.uuids.uuids;
            for (int j = 0; j < device->remote.uuids.uuid_cnt; j++) {
                if (!bt_uuid_compare(uuid1, uuid2)) {
                    update = false;
                    break;
                }
                uuid2++;
            }
            uuid1++;
        }
    }

    if (!update)
        return false;

    free(device->remote.uuids.uuids);
copy:
    device->remote.uuids.uuids = malloc(sizeof(bt_uuid_t) * size);
    memcpy(device->remote.uuids.uuids, uuids, sizeof(bt_uuid_t) * size);
    device->remote.uuids.uuid_cnt = size;

    return true;
}

uint16_t device_get_appearance(bt_device_t* device)
{
    return device->remote.appearance;
}

void device_set_appearance(bt_device_t* device, uint16_t appearance)
{
    device->remote.appearance = appearance;
}

int8_t device_get_rssi(bt_device_t* device)
{
    return device->remote.rssi;
}

void device_set_rssi(bt_device_t* device, int8_t rssi)
{
    device->remote.rssi = rssi;
}

const char* device_get_alias(bt_device_t* device)
{
    return (const char*)device->remote.alias;
}

bool device_set_alias(bt_device_t* device, const char* alias)
{
    if (!strncmp(device->remote.alias, alias, BT_REM_NAME_MAX_LEN))
        return false;

    strlcpy((char*)device->remote.alias, alias, sizeof(device->remote.alias));
    return true;
}

connection_state_t device_get_connection_state(bt_device_t* device)
{
    return device->remote.connection_state;
}

void device_set_connection_state(bt_device_t* device, connection_state_t state)
{
    device->remote.connection_state = state;
}

bool device_is_connected(bt_device_t* device)
{
    return device->remote.connection_state >= CONNECTION_STATE_CONNECTED;
}

bool device_is_encrypted(bt_device_t* device)
{
    return device->remote.connection_state > CONNECTION_STATE_CONNECTED;
}

uint16_t device_get_acl_handle(bt_device_t* device)
{
    return device->remote.acl_handle;
}

void device_set_acl_handle(bt_device_t* device, uint16_t handle)
{
    device->remote.acl_handle = handle;
}

bt_link_role_t device_get_local_role(bt_device_t* device)
{
    return device->remote.local_role;
}

void device_set_local_role(bt_device_t* device, bt_link_role_t role)
{
    device->remote.local_role = role;
}

void device_set_bond_initiate_local(bt_device_t* device, bool initiate_local)
{
    device->remote.local_initiate_bond = initiate_local;
}

bool device_is_bond_initiate_local(bt_device_t* device)
{
    return device->remote.local_initiate_bond;
}

bond_state_t device_get_bond_state(bt_device_t* device)
{
    return device->remote.bond_state;
}

void device_set_bond_state(bt_device_t* device, bond_state_t state)
{
    device->remote.bond_state = state;
}

bool device_is_bonded(bt_device_t* device)
{
    return device->remote.bond_state == BOND_STATE_BONDED;
}

uint8_t* device_get_link_key(bt_device_t* device)
{
    return device->remote.link_key;
}

void device_set_link_key(bt_device_t* device, bt_128key_t link_key)
{
    memcpy(device->remote.link_key, link_key, sizeof(bt_128key_t));
}

void device_delete_link_key(bt_device_t* device)
{
    memset(device->remote.link_key, 0, sizeof(bt_128key_t));
}

bt_link_key_type_t device_get_link_key_type(bt_device_t* device)
{
    return device->remote.link_key_type;
}

void device_set_link_key_type(bt_device_t* device, bt_link_key_type_t type)
{
    device->remote.link_key_type = type;
}

bt_link_policy_t device_get_link_policy(bt_device_t* device)
{
    return device->remote.link_policy;
}

void device_set_link_policy(bt_device_t* device, bt_link_policy_t policy)
{
    device->remote.link_policy = policy;
}

void device_set_le_phy(bt_device_t* device, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    device->remote.tx_phy = tx_phy;
    device->remote.rx_phy = rx_phy;
}

void device_get_le_phy(bt_device_t* device, ble_phy_type_t* tx_phy, ble_phy_type_t* rx_phy)
{
    *tx_phy = device->remote.tx_phy;
    *rx_phy = device->remote.rx_phy;
}

static void device_get_remote_uuids(bt_device_t* device, remote_device_properties_t* prop)
{
    bt_uuid_t* uuids;
    uint8_t count_uuid16 = 0;
    uint8_t count_uuid128 = 0;
    uint8_t* uuids_prop = prop->uuids;
    uint8_t* p;
    uint8_t* q;
    bt_uuid_t bt_uuid128_base = {
        .type = BT_UUID128_TYPE,
        .val.u128 = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
            0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
    };

    if (device->remote.uuids.uuid_cnt == 0) {
        BT_LOGD("%s, No uuids found", __func__);
        return;
    }

    uuids = device->remote.uuids.uuids;

    for (int i = 0; i < device->remote.uuids.uuid_cnt; i++) {
        switch ((uuids + i)->type) {
        case BT_UUID16_TYPE:
            count_uuid16++;
            break;
        case BT_UUID32_TYPE:
            break; // TODO: Save 32bit uuid
        case BT_UUID128_TYPE: {
            bt_uuid_t tmp = { 0 };
            int result = -1;

            memcpy(&tmp, uuids + i, sizeof(bt_uuid_t));
            tmp.val.u128[12] = 0x00;
            tmp.val.u128[13] = 0x00;
            result = bt_uuid_compare(&tmp, &bt_uuid128_base);
            if (result != 0) {
                count_uuid128++;
                break;
            } else if ((uuids + i)->val.u128[14] == 0x00 && (uuids + i)->val.u128[15] == 0x00) {
                count_uuid16++;
            }

            break;
        }

        default:
            break;
        }
    }

    if (count_uuid16 == 0 && count_uuid128 == 0) {
        BT_LOGD("%s, No uuids found", __func__);
        return;
    }

    if (count_uuid16 > 0) {
        count_uuid16 = count_uuid16 * 2 + 1 > CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN ? (CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN - 1) / 2 : count_uuid16;
        *uuids_prop = (BT_HEAD_UUID16_TYPE << 5 | count_uuid16) & 0x7F;
    }

    if (count_uuid128 > 0) {
        if (count_uuid16 > 0) {
            count_uuid128 = count_uuid128 * 16 + 1 > CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN - count_uuid16 * 2 + 1 ? (CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN - count_uuid16 * 2 - 2) / 16 : count_uuid128;
            *(uuids_prop + count_uuid16 * 2 + 1) = (BT_HEAD_UUID128_TYPE << 5 | count_uuid128) & 0x7F;
        } else {
            count_uuid128 = count_uuid128 * 16 + 1 > CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN ? (CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN - 1) / 16 : count_uuid128;
            *(uuids_prop) = (BT_HEAD_UUID128_TYPE << 5 | count_uuid128) & 0x7F;
        }
    }

    if (count_uuid16 > 0) {
        p = uuids_prop + 1;
    }

    if (count_uuid128 > 0) {
        q = uuids_prop + count_uuid16 * 2 + 2;
    }

    for (int i = 0; i < device->remote.uuids.uuid_cnt && ((count_uuid16 > 0) | (count_uuid128 > 0)); i++) {
        switch ((uuids + i)->type) {
        case BT_UUID16_TYPE:
            if (count_uuid16 > 0 && p - uuids_prop < CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN - 1) {
                UINT16_TO_STREAM(p, (uuids + i)->val.u16);
                count_uuid16--;
            }
            break;
        case BT_UUID32_TYPE:
            break; // TODO: Save 32bit uuid
        case BT_UUID128_TYPE: {
            bt_uuid_t tmp = { 0 };
            int result = -1;

            memcpy(&tmp, uuids + i, sizeof(bt_uuid_t));
            tmp.val.u128[12] = 0x00;
            tmp.val.u128[13] = 0x00;
            result = bt_uuid_compare(&tmp, &bt_uuid128_base);
            if (result != 0) {
                if (count_uuid128 > 0 && q - uuids_prop < CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN - 15) {
                    memcpy(q, (uuids + i)->val.u128, 16);
                    q += 16;
                    count_uuid128--;
                }

                break;
            } else if ((uuids + i)->val.u128[14] == 0x00 && (uuids + i)->val.u128[15] == 0x00) {
                if (count_uuid16 > 0 && p - uuids_prop < CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN - 1) {
                    *(p++) = (uuids + i)->val.u128[BASE_UUID16_OFFSET + 1];
                    *(p++) = (uuids + i)->val.u128[BASE_UUID16_OFFSET];
                    count_uuid16--;
                }
            }

            break;
        }

        default:
            break;
        }
    }
}

void device_get_property(bt_device_t* device, remote_device_properties_t* prop)
{
    memcpy(&prop->addr, &device->remote.addr, sizeof(bt_address_t));
    prop->addr_type = device->remote.addr_type;
    strlcpy(prop->name, device->remote.name, BT_REM_NAME_MAX_LEN);
    strlcpy(prop->alias, device->remote.alias, BT_REM_NAME_MAX_LEN);
    prop->class_of_device = device->remote.device_class;
    memcpy(prop->link_key, device->remote.link_key, 16);
    prop->link_key_type = device->remote.link_key_type;
    prop->device_type = device->remote.device_type;
    device_get_remote_uuids(device, prop);
}

void device_get_le_property(bt_device_t* device, remote_device_le_properties_t* prop)
{
    memcpy(&prop->addr, &device->remote.addr, sizeof(bt_address_t));
    prop->addr_type = device->remote.addr_type;
    memcpy(prop->smp_key, device->remote.smp_data, 80);
    prop->device_type = device->remote.device_type;
}

void device_set_flags(bt_device_t* device, uint32_t flags)
{
    device->flags |= flags;
}

void device_clear_flag(bt_device_t* device, uint32_t flag)
{
    device->flags &= ~flag;
}

bool device_check_flag(bt_device_t* device, uint32_t flag)
{
    return device->flags & flag;
}

uint8_t* device_get_smp_key(bt_device_t* device)
{
    return device->remote.smp_data;
}

void device_set_smp_key(bt_device_t* device, uint8_t* smp_key)
{
    device_set_flags(device, DFLAG_LE_KEY_SET);
    memcpy(device->remote.smp_data, smp_key, sizeof(device->remote.smp_data));
}

void device_delete_smp_key(bt_device_t* device)
{
    device_clear_flag(device, DFLAG_LE_KEY_SET);
    memset(device->remote.smp_data, 0, sizeof(device->remote.smp_data));
}

static int linkkey_dump(bt_device_t* device, char* str)
{
    uint8_t* lk = device->remote.link_key;
    uint8_t type = device->remote.link_key_type;

    return sprintf(str, "%02x | %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
        type, lk[0], lk[1], lk[2], lk[3], lk[4], lk[5], lk[6], lk[7], lk[8], lk[9], lk[10],
        lk[11], lk[12], lk[13], lk[14], lk[15]);
}

void device_dump(bt_device_t* device)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    char link_key_str[40] = { 0 };
    char uuid_str[40] = { 0 };

    bt_addr_ba2str(&device->remote.addr, addr_str);
    printf("device: %s\n", addr_str);
    printf("\tName: %s\n", device->remote.name);
    printf("\tAlias: %s\n", device->remote.alias);
    printf("\tClass: 0x%08" PRIx32 "\n", device->remote.device_class);
    printf("\tType: %d\n", device->remote.device_type);
    printf("\tTransport: %d\n", device->remote.transport);
    printf("\tRssi: %d\n", device->remote.rssi);
    printf("\tBondState: %d\n", device->remote.bond_state);
    printf("\tConnState: %d\n", device->remote.connection_state);
    printf("\tisEnc: %d\n", device_is_encrypted(device));
    linkkey_dump(device, link_key_str);
    printf("\tLinkkey: %s\n", link_key_str);
    if (device->remote.uuids.uuid_cnt) {
        printf("\tUUIDs:\n");
        bt_uuid_t* uuid = device->remote.uuids.uuids;
        for (int i = 0; i < device->remote.uuids.uuid_cnt; i++) {
            bt_uuid_to_string(uuid, uuid_str, 40);
            printf("\t\tuuid[%-2d]: %s\n", i, uuid_str);
            uuid++;
        }
    }
}
