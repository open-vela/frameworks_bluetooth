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
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_KVDB
#include <kvdb.h>
#endif
#ifdef CONFIG_UORB
#include <connectivity/bt.h>
#include <uORB/uORB.h>
#endif

#include "adapter_internel.h"
#include "bt_list.h"
#ifdef CONFIG_BLUETOOTH_BLE_ADV
#include "advertising.h"
#endif
#ifdef CONFIG_BLUETOOTH_L2CAP
#include "l2cap_service.h"
#endif
#include "advertising.h"
#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_adapter.h"
#include "bt_addr.h"
#include "bt_device.h"
#include "bt_list.h"
#include "bt_profile.h"
#include "bt_uuid.h"
#include "btservice.h"
#include "callbacks_list.h"
#include "connection_manager.h"
#include "device.h"
#include "hci_error.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "service_manager.h"
#include "state_machine.h"
#include "storage.h"
#define LOG_TAG "adapter-svc"

#include "bt_utils.h"
#include "utils/log.h"

#define CALLBACK_FOREACH(_list, _struct, _cback, ...) BT_CALLBACK_FOREACH(_list, _struct, _cback, ##__VA_ARGS__)

#define CHECK_ADAPTER_READY()                                     \
    if (g_adapter_service.adapter_state != BT_ADAPTER_STATE_ON) { \
        status = BT_STATUS_NOT_ENABLED;                           \
        goto error;                                               \
    }

#define CBLIST (g_adapter_service.adapter_callbacks)

typedef struct adapter_properties {
    char name[BT_LOC_NAME_MAX_LEN + 1];
    bt_address_t addr;
    uint32_t class_of_device;
    uint32_t io_capability;
    uint8_t scan_mode;
    bool bondable;
    bt_uuid_t uuids[10];
} adapter_properties_t;

typedef struct le_adapter_properties {
    char name[BT_LOC_NAME_MAX_LEN + 1];
    bt_address_t addr;
    uint8_t addr_type;
    uint32_t le_io_capability;
    uint32_t le_appearance;
} le_adapter_properties_t;

typedef struct adapter_service {
    adapter_state_machine_t* stm;
    adapter_properties_t properties;
    bt_list_t* devices;
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    le_adapter_properties_t le_properties;
    bt_list_t* le_devices;
#endif
    pthread_mutex_t adapter_lock;
    bt_adapter_state_t adapter_state;
    bool is_discovering;
    uint8_t max_acl_connections;
    callbacks_list_t* adapter_callbacks;
    int adapter_state_adv;
} adapter_service_t;

static adapter_service_t g_adapter_service;

adapter_service_t* get_adapter_service(void)
{
    return &g_adapter_service;
}

static void adapter_lock(void)
{
    pthread_mutex_lock(&g_adapter_service.adapter_lock);
}

static void adapter_unlock(void)
{
    pthread_mutex_unlock(&g_adapter_service.adapter_lock);
}

static bt_device_t* adapter_find_device(const bt_address_t* addr, bt_transport_t transport)
{
    bt_list_node_t* node;
    bt_list_t* list;

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (transport == BT_TRANSPORT_BREDR)
        list = g_adapter_service.devices;
    else
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        if (transport == BT_TRANSPORT_BLE)
        list = g_adapter_service.le_devices;
    else
#endif
        return NULL;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        if (!memcmp(device_get_address(device), addr, sizeof(bt_address_t)) && device_get_transport(device) == transport)
            return device;
    }

    return NULL;
}

static bt_device_t* adapter_find_create_classic_device(bt_address_t* addr)
{
    bt_device_t* device;

    if ((device = adapter_find_device(addr, BT_TRANSPORT_BREDR)))
        return device;

    device = br_device_create(addr);
    assert(device);
    bt_list_add_tail(g_adapter_service.devices, device);

    return device;
}

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
static bt_device_t* adapter_find_create_le_device(bt_address_t* addr, ble_addr_type_t addr_type)
{
    bt_device_t* device;

    if ((device = adapter_find_device(addr, BT_TRANSPORT_BLE)))
        return device;

    device = le_device_create(addr, addr_type);
    assert(device);
    bt_list_add_tail(g_adapter_service.le_devices, device);

    return device;
}
#endif

static void adapter_delete_device(void* data)
{
    bt_device_t* device = (bt_device_t*)data;
    bt_address_t* addr;

    if (device_get_connection_state(device) != CONNECTION_STATE_DISCONNECTED) {
        addr = device_get_address(device);
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_connection_state_changed, addr,
            device_get_transport(device), CONNECTION_STATE_DISCONNECTED);
    }

    device_delete(device);
}

static adapter_remote_event_t* create_remote_event(bt_address_t* addr, uint8_t evt_id)
{
    adapter_remote_event_t* evt = malloc(sizeof(adapter_remote_event_t));
    if (!evt) {
        BT_LOGE("adapter event alloc fail");
        return NULL;
    }

    memcpy(&evt->addr, addr, sizeof(bt_address_t));
    evt->evt_id = evt_id;

    return evt;
}

static void adapter_properties_copy(adapter_properties_t* prop, adapter_storage_t* storage)
{
    strlcpy(prop->name, storage->name, sizeof(prop->name));
    prop->class_of_device = storage->class_of_device;
    prop->io_capability = storage->io_capability;
    prop->scan_mode = storage->scan_mode;
    prop->bondable = storage->bondable;
}

static int get_devices_cnt(int flag, uint8_t transport)
{
    bt_list_t* list;
    bt_list_node_t* node;
    int cnt = 0;

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    if (transport == BT_TRANSPORT_BLE) {
        list = g_adapter_service.le_devices;
    } else
#endif
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
        if (transport == BT_TRANSPORT_BREDR) {
        list = g_adapter_service.devices;
    } else
#endif
    {
        BT_LOGE("%s, transport invalid!", __func__);
        return cnt;
    }

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        if ((flag == DFLAG_BONDED && device_is_bonded(device)) || (flag == DFLAG_CONNECTED && device_is_connected(device)) || (device_check_flag(device, flag)))
            cnt++;
    }

    return cnt;
}

static void load_remote_uuids(remote_device_properties_t* remote, bt_device_t* device)
{
    bt_uuid_t* uuids;
    bt_uuid_t* tmp;
    uint16_t count_uuid16 = 0;
    uint16_t count_uuid128 = 0;
    uint16_t count_uuids = 0;
    uint8_t* remote_uuids = remote->uuids;

    if (*remote_uuids == 0) {
        BT_LOGD("%s, No uuids found", __func__);
        return;
    }

    switch (*remote_uuids >> 5) {
    case BT_HEAD_UUID16_TYPE:
        count_uuid16 = *remote_uuids & 0x1F;
        break;
    case BT_HEAD_UUID128_TYPE:
        count_uuid128 = *remote_uuids & 0x1F;
        break;
    default:
        break;
    }

    remote_uuids++;
    count_uuids = count_uuid16;

    if (count_uuid16 != 0) {
        if (count_uuid16 * 2 + 1 < CONFIG_BLUETOOTH_MAX_SAVED_REMOTE_UUIDS_LEN) {
            count_uuid128 = *(remote_uuids + count_uuid16 * 2) & 0x1F;
            count_uuids += count_uuid128;
        }
    }

    uuids = (bt_uuid_t*)malloc(sizeof(bt_uuid_t) * count_uuids);
    tmp = uuids;
    for (int i = 0; i < count_uuid16; i++) {
        bt_uuid_t uuid;

        uuid.type = BT_UUID16_TYPE;
        BE_STREAM_TO_UINT16(uuid.val.u16, remote_uuids)
        bt_uuid_to_uuid128(&uuid, tmp);
        tmp++;
    }

    if (count_uuid128 != 0) {
        remote_uuids++;
    }

    for (int i = 0; i < count_uuid128; i++) {
        tmp->type = BT_UUID128_TYPE;
        memcpy(tmp->val.u128, remote_uuids, sizeof(tmp->val.u128));
        remote_uuids += 16;
        tmp++;
    }

    device_set_uuids(device, uuids, count_uuids);
    free(uuids);
}

static void bonded_device_loaded(void* data, uint16_t length, uint16_t items)
{
    if (data && items) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        remote_device_properties_t* remote = (remote_device_properties_t*)data;

        BT_LOGD("load classic bonded device successfully:");
        for (int i = 0; i < items; i++) {
            bt_device_t* device = br_device_create(&remote->addr);
            device_set_name(device, remote->name);
            device_set_alias(device, remote->alias);
            device_set_device_class(device, remote->class_of_device);
            device_set_device_type(device, remote->device_type);
            device_set_link_key(device, remote->link_key);
            device_set_link_key_type(device, remote->link_key_type);
            device_set_bond_state(device, BOND_STATE_BONDED);
            load_remote_uuids(remote, device);
            bt_list_add_tail(g_adapter_service.devices, device);
            bt_addr_ba2str(&remote->addr, addr_str);
            uint8_t* lk = remote->link_key;
            BT_LOGD("BONDED DEVICE[%d], Name:[%s] Addr:[%s] LinkKey: [%02X] | [%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]",
                i, remote->name, addr_str, remote->link_key_type, lk[0], lk[1], lk[2], lk[3], lk[4], lk[5], lk[6],
                lk[7], lk[8], lk[9], lk[10], lk[11], lk[12], lk[13], lk[14], lk[15]);
            bt_sal_set_bonded_devices(PRIMARY_ADAPTER, remote, 1);
            remote++;
        }
    }
    BT_LOGD("classic bonded device cnt: %" PRIu16, items);

    send_to_state_machine((state_machine_t*)g_adapter_service.stm, BREDR_ENABLED, NULL);
}

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
static void whitelist_device_loaded(void* data, uint16_t length, uint16_t items)
{
    if (data && items) {
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        remote_device_le_properties_t* remote = (remote_device_le_properties_t*)data;
        BT_LOGD("load whitelist device successfully:");
        for (int i = 0; i < items; i++) {
            bt_device_t* device = adapter_find_create_le_device(&remote->addr, remote->addr_type);
            device_set_flags(device, DFLAG_WHITELIST_ADDED);
            bt_addr_ba2str(&remote->addr, addr_str);
            BT_LOGD("LE WHITELIST[%d] [%s]", i, addr_str);
            bt_sal_le_add_white_list(PRIMARY_ADAPTER, &remote->addr, remote->addr_type);
            remote++;
        }
    }
    BT_LOGD("ble whitelist device cnt: %" PRIu16, items);
}

static void le_bonded_device_loaded(void* data, uint16_t length, uint16_t items)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    if (data && items) {
        remote_device_le_properties_t* remote = (remote_device_le_properties_t*)data;
        BT_LOGD("load ble bonded device successfully:");
        for (int i = 0; i < items; i++) {
            bt_device_t* device = adapter_find_create_le_device(&remote->addr, remote->addr_type);
            device_set_bond_state(device, BOND_STATE_BONDED);
            device_set_smp_key(device, remote->smp_key);
            device_set_identity_address(device, (bt_address_t*)remote->smp_key);
            bt_addr_ba2str(&remote->addr, addr_str);
            uint8_t* ltk = &remote->smp_key[12];
            BT_LOGD("LE BOND DEVICE[%d], Addr:[%s] Atype:[%d] LTK: [%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]",
                i, addr_str, remote->addr_type, ltk[0], ltk[1], ltk[2], ltk[3], ltk[4], ltk[5], ltk[6], ltk[7],
                ltk[8], ltk[9], ltk[10], ltk[11], ltk[12], ltk[13], ltk[14], ltk[15]);
            remote++;
        }

        bt_sal_le_set_bonded_devices(PRIMARY_ADAPTER, data, items);
    }

    BT_LOGD("ble bonded device cnt: %" PRIu16, items);
}
#endif

static void adapter_update_bonded_device(void)
{
    bt_list_t* list = g_adapter_service.devices;
    bt_list_node_t* node;

    int size = get_devices_cnt(DFLAG_BONDED, BT_TRANSPORT_BREDR);
    if (!size) {
        bt_storage_save_bonded_device(NULL, 0);
        return;
    }

    remote_device_properties_t remotes[size];
    size = 0;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        if (device_is_bonded(device)) {
            device_get_property(device, &remotes[size]);
            size++;
        }
    }

    bt_storage_save_bonded_device(remotes, size);
}

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
static void adapter_update_whitelist(void)
{
    BT_LOGD("%s", __func__);

    bt_list_t* list = g_adapter_service.le_devices;
    bt_list_node_t* node;

    int size = get_devices_cnt(DFLAG_WHITELIST_ADDED, BT_TRANSPORT_BLE);
    if (!size) {
        bt_storage_save_whitelist(NULL, 0);
        return;
    }

    remote_device_le_properties_t remotes[size];
    size = 0;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        if (device_check_flag(device, DFLAG_WHITELIST_ADDED)) {
            device_get_le_property(device, &remotes[size]);
            size++;
        }
    }

    bt_storage_save_whitelist(remotes, size);
}
#endif

static void adapter_save_properties(void)
{
    adapter_properties_t* prop = &g_adapter_service.properties;
    adapter_storage_t storage;

    strlcpy(storage.name, prop->name, BT_LOC_NAME_MAX_LEN);
    storage.class_of_device = prop->class_of_device;
    storage.io_capability = prop->io_capability;
    storage.scan_mode = prop->scan_mode;
    storage.bondable = prop->bondable;
    bt_storage_save_adapter_info(&storage);
}

static void send_pair_display_notification(bt_address_t* addr, uint8_t transport,
    bt_pair_type_t type, uint32_t passkey)
{
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_pair_display, addr, transport, type, passkey);
}

static void process_pair_request_evt(bt_address_t* addr, bool local_initiate, bool is_bondable)
{
    bt_device_t* device;

    if (!is_bondable) {
        BT_ADDR_LOG("Pair not allowed for:%s", addr);
        bt_sal_pair_reply(PRIMARY_ADAPTER, addr, HCI_ERR_PAIRING_NOT_ALLOWED);
        return;
    }

    adapter_lock();
    device = adapter_find_create_classic_device(addr);
    device_set_bond_initiate_local(device, local_initiate);
    adapter_unlock();
    /* maybe remote device initiate pairing request (get io capability) or
     * user and app initiated pairing process, notify user to ensure,
     * We need to obtain user authorization.
     */
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_pair_request, addr);
}

static void process_pin_request_evt(bt_address_t* addr, uint32_t cod,
    bool min_16_digit, const char* name)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_create_classic_device(addr);
    if (device_get_bond_state(device) == BOND_STATE_CANCELING) {
        BT_LOGE("%s, canceling reject", __func__);
        bt_sal_pin_reply(PRIMARY_ADAPTER, addr, false, NULL, 0);
        adapter_unlock();
        return;
    }

    if (!device_check_flag(device, DFLAG_NAME_SET | DFLAG_GET_RMT_NAME)) {
        BT_LOGD("pin requesting, request remote name...");
        bt_sal_get_remote_name(PRIMARY_ADAPTER, addr);
        device_set_flags(device, DFLAG_GET_RMT_NAME);
    }

    if (device_get_bond_state(device) != BOND_STATE_BONDING)
        device_set_bond_state(device, BOND_STATE_BONDING);
    adapter_unlock();
    /* send pin code request notification*/
    send_pair_display_notification(addr, BT_TRANSPORT_BREDR, PAIR_TYPE_PIN_CODE, 0x0);
}

static void process_ssp_request_evt(bt_address_t* addr, uint8_t transport,
    uint32_t cod, bt_pair_type_t ssp_type,
    uint32_t pass_key, const char* name)
{
    bt_device_t* device;
    adapter_lock();

    device = adapter_find_device(addr, transport);

    if (device_get_bond_state(device) == BOND_STATE_CANCELING) {
        BT_LOGE("%s, canceling reject", __func__);
        if (transport == BT_TRANSPORT_BREDR) {
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
            bt_sal_ssp_reply(PRIMARY_ADAPTER, addr, false, ssp_type, 0x0);
#endif
        } else {
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
            bt_sal_le_smp_reply(PRIMARY_ADAPTER, addr, false, ssp_type, 0);
#endif
        }

        adapter_unlock();
        return;
    }

    if (!device_check_flag(device, DFLAG_NAME_SET | DFLAG_GET_RMT_NAME)) {
        BT_LOGD("ssp, request remote name...");
        bt_sal_get_remote_name(PRIMARY_ADAPTER, addr);
        device_set_flags(device, DFLAG_GET_RMT_NAME);
    }

    if (device_get_bond_state(device) != BOND_STATE_BONDING)
        device_set_bond_state(device, BOND_STATE_BONDING);
    adapter_unlock();
    /* send ssp request notification*/
    send_pair_display_notification(addr, transport, ssp_type, pass_key);
}

static void process_bond_state_change_evt(bt_address_t* addr, bond_state_t state,
    uint8_t transport, bool is_ctkd)
{
    remote_device_properties_t remote;
    bt_device_t* device;

    adapter_lock();
    if (transport == BT_TRANSPORT_BREDR) {
        device = adapter_find_create_classic_device(addr);
        if (state == BOND_STATE_BONDED) {
            device_set_bond_state(device, BOND_STATE_BONDED);
            bt_sal_get_remote_device_info(PRIMARY_ADAPTER, addr, &remote);
            device_set_device_type(device, remote.device_type);
            /* update bonded device info */
            adapter_update_bonded_device();
            // device_set_connection_state(device, CONNECTION_STATE_ENCRYPTED_BREDR);
            if (device_is_connected(device))
                bt_sal_start_service_discovery(PRIMARY_ADAPTER, addr, NULL);
        }
    } else {
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        device = adapter_find_create_le_device(addr, BT_LE_ADDR_TYPE_PUBLIC);
        if (state == BOND_STATE_BONDED) {
            device_set_device_type(device, BT_DEVICE_TYPE_BLE);
            // device_set_connection_state(device, CONNECTION_STATE_ENCRYPTED_LE);
        } else if (state == BOND_STATE_NONE) {
            device_delete_smp_key(device);
            device_set_identity_address(device, NULL);
        }
#else
        adapter_unlock();
        return;
#endif
    }

    device_set_bond_state(device, state);
    adapter_unlock();
    /* send bond state change notification */
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_bond_state_changed, addr, transport, state, is_ctkd);
}

static void process_service_search_done_evt(bt_address_t* addr, bt_uuid_t* uuids, uint16_t size)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_create_classic_device(addr);
    device_set_uuids(device, uuids, size);
    adapter_unlock();
    adapter_update_bonded_device();
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_remote_uuids_changed, addr, uuids, size);
    free(uuids);
}

static void process_enc_state_change_evt(bt_address_t* addr, bool encrypted,
    uint8_t transport)
{
    bt_device_t* device;

    adapter_lock();
    if (transport == BT_TRANSPORT_BREDR)
        device = adapter_find_create_classic_device(addr);
    else if (transport == BT_TRANSPORT_BLE)
        device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    else
        return;

    if (encrypted) {
        if (transport == BT_TRANSPORT_BREDR)
            device_set_connection_state(device, CONNECTION_STATE_ENCRYPTED_BREDR);
        else
            device_set_connection_state(device, CONNECTION_STATE_ENCRYPTED_LE);
    } else
        device_set_connection_state(device, CONNECTION_STATE_CONNECTED);
    adapter_unlock();
}

static void process_link_key_update_evt(bt_address_t* addr, bt_128key_t link_key,
    bt_link_key_type_t type)
{
    bt_device_t* device;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    adapter_lock();
    device = adapter_find_create_classic_device(addr);
    device_set_link_key(device, link_key);
    device_set_link_key_type(device, type);
    adapter_update_bonded_device();
    bt_addr_ba2str(addr, addr_str);
    uint8_t* lk = link_key;
    BT_LOGI("DEVICE[%s] LinkKey: %02X | [%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]",
        addr_str, type, lk[0], lk[1], lk[2], lk[3], lk[4], lk[5], lk[6],
        lk[7], lk[8], lk[9], lk[10], lk[11], lk[12], lk[13], lk[14], lk[15]);
    adapter_unlock();
}

static void process_link_key_removed_evt(bt_address_t* addr, bt_status_t status)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_create_classic_device(addr);
    device_delete_link_key(device);
    if (device_get_bond_state(device) == BOND_STATE_BONDED)
        device_set_bond_state(device, BOND_STATE_NONE);
    /* remove bond device */
    adapter_update_bonded_device();
    adapter_unlock();
}

static void handle_security_event(void* data)
{
    adapter_remote_event_t* evt = (adapter_remote_event_t*)data;

    switch (evt->evt_id) {
    case PAIR_REQUEST_EVT:
        process_pair_request_evt(&evt->addr, evt->pair_req.local_initiate,
            evt->pair_req.is_bondable);
        break;
    case PIN_REQUEST_EVT:
        process_pin_request_evt(&evt->addr, evt->pin_req.cod, evt->pin_req.min_16_digit,
            evt->pin_req.name);
        break;
    case SSP_REQUEST_EVT:
        process_ssp_request_evt(&evt->addr, evt->ssp_req.transport, evt->ssp_req.cod,
            evt->ssp_req.ssp_type, evt->ssp_req.pass_key, evt->ssp_req.name);
        break;
    case BOND_STATE_CHANGE_EVT:
        process_bond_state_change_evt(&evt->addr, evt->bond_state.state,
            evt->bond_state.transport,
            evt->bond_state.is_ctkd);
        break;
    case SDP_SEARCH_DONE_EVT:
        process_service_search_done_evt(&evt->addr, evt->sdp.uuids, evt->sdp.uuid_size);
        break;
    case ENC_STATE_CHANGE_EVT:
        process_enc_state_change_evt(&evt->addr, evt->enc_state.encrypted,
            evt->enc_state.transport);
        break;
    case LINK_KEY_UPDATE_EVT:
        process_link_key_update_evt(&evt->addr, evt->link_key.key, evt->link_key.type);
        break;
    case LINK_KEY_REMOVED_EVT:
        process_link_key_removed_evt(&evt->addr, evt->link_key.status);
        break;
    default:
        break;
    }

    free(data);
}

static void process_connect_request_evt(bt_address_t* addr, uint32_t cod)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_device_t* device;
    bool reject = false;

    BT_ADDR_LOG("ACL Connect Request from :%s", addr);

    adapter_lock();
    device = adapter_find_create_classic_device(addr);
    device_set_device_class(device, cod);
    if (get_devices_cnt(DFLAG_CONNECTED, BT_TRANSPORT_BREDR) >= adapter->max_acl_connections) {
        reject = true;
        BT_LOGW("Reject connect request without available connection");
        /*  if a2dp source support, accept link with master role ? */
        bt_sal_acl_connection_reply(PRIMARY_ADAPTER, addr, false);
    }
    adapter_unlock();
    if (!reject) {
        /* send connect request notification */
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_connect_request, addr);
    }
}

static const char* acl_connection_str(connection_state_t state)
{
    switch (state) {
        CASE_RETURN_STR(CONNECTION_STATE_DISCONNECTED);
        CASE_RETURN_STR(CONNECTION_STATE_CONNECTING);
        CASE_RETURN_STR(CONNECTION_STATE_DISCONNECTING);
        CASE_RETURN_STR(CONNECTION_STATE_CONNECTED);
    default:
        return "Unknow";
    }
}

static void process_connection_state_changed_evt(bt_address_t* addr, acl_state_param_t* acl_params)
{
    bt_device_t* device;

    BT_ADDR_LOG("ACL connection state changed, addr:%s, link:%d, state:%s, status:%d, reason:%" PRIu32 "", addr,
        acl_params->transport, acl_connection_str(acl_params->connection_state),
        acl_params->status, acl_params->hci_reason_code);

    adapter_lock();
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (acl_params->transport == BT_TRANSPORT_BREDR) {
        device = adapter_find_create_classic_device(addr);
        if (device_get_bond_state(device) == BOND_STATE_BONDING && !device_check_flag(device, DFLAG_NAME_SET | DFLAG_GET_RMT_NAME)) {
            BT_LOGD("bonding, request remote name...");
            bt_sal_get_remote_name(PRIMARY_ADAPTER, addr);
            device_set_flags(device, DFLAG_GET_RMT_NAME);
        }
    } else
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        if (acl_params->transport == BT_TRANSPORT_BLE)
        device = adapter_find_create_le_device(addr, acl_params->addr_type);
    else
#endif
    {
        adapter_unlock();
        BT_LOGW("%s, unexpected device", __func__);
        return;
    }

    device_set_connection_state(device, acl_params->connection_state);
    if (acl_params->connection_state == CONNECTION_STATE_CONNECTED) {
        device_set_acl_handle(device, bt_sal_get_acl_connection_handle(PRIMARY_ADAPTER, addr, acl_params->transport));
        // if (acl_params->transport == BT_TRANSPORT_BLE)
        //     adapter_le_add_whitelist(addr);
    }
    adapter_unlock();

    if (acl_params->transport == BT_TRANSPORT_BREDR) {
        switch (acl_params->connection_state) {
        case CONNECTION_STATE_CONNECTED:
            bt_pm_remote_device_connected(addr);
            break;
        case CONNECTION_STATE_DISCONNECTED:
            bt_pm_remote_device_disconnected(addr);
            break;
        default:
            break;
        }
    }

    if (acl_params->connection_state == CONNECTION_STATE_DISCONNECTED)
        bt_cm_process_disconnect_event(addr, acl_params->transport);

    /* send connection changed notification */
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_connection_state_changed, addr,
        acl_params->transport, acl_params->connection_state);
}

static void handle_connection_event(void* data)
{
    adapter_remote_event_t* conn_evt = (adapter_remote_event_t*)data;

    switch (conn_evt->evt_id) {
    case CONNECT_REQUEST_EVT:
        process_connect_request_evt(&conn_evt->addr, conn_evt->cod);
        break;
    case CONNECTION_STATE_CHANGE_EVT:
        process_connection_state_changed_evt(&conn_evt->addr, &conn_evt->acl_params);
        break;
    default:
        break;
    }

    free(data);
}

static void process_discovery_state_changed_evt(bt_discovery_state_t state)
{
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    /* discovery state real changed */
    adapter->is_discovering = ((state == BT_DISCOVERY_STATE_STARTED) ? true : false);
    adapter_unlock();

    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_discovery_state_changed, state);
}

static void process_device_found_evt(bt_discovery_result_t* remote)
{
    bt_device_t* device;

    adapter_lock();
    if (!g_adapter_service.is_discovering) {
        adapter_unlock();
        return;
    }

    device = adapter_find_create_classic_device(&remote->addr);
    device_set_name(device, remote->name);
    device_set_device_class(device, remote->cod);
    device_set_rssi(device, remote->rssi);
    device_set_device_type(device, BT_DEVICE_TYPE_BREDR);
    /* uuids ? stack don't parse uuid EIR */
    adapter_unlock();
    /* send device found notification */
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_discovery_result, remote);
}

static void process_remote_name_recieved_evt(bt_address_t* addr, const char* name)
{
    adapter_lock();
    bt_device_t* device = adapter_find_create_classic_device(addr);
    bool notify = false;

    BT_ADDR_LOG("remote device:%s name:%s", addr, name);
    notify = device_set_name(device, name);
    device_clear_flag(device, DFLAG_GET_RMT_NAME);
    adapter_unlock();
    if (notify) {
        /* send name changed notification to all observer */
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_remote_name_changed, addr, name);
    }
}

static void handle_discovery_event(void* data)
{
    adapter_discovery_evt_t* evt = (adapter_discovery_evt_t*)data;

    switch (evt->evt_id) {
    case DISCOVER_STATE_CHANGE_EVT:
        process_discovery_state_changed_evt(evt->state);
        break;
    case DEVICE_FOUND_EVT:
        process_device_found_evt(&evt->result);
        break;
    case REMOTE_NAME_RECIEVED_EVT:
        process_remote_name_recieved_evt(&evt->remote_name.addr, (const char*)evt->remote_name.name);
        break;
    }

    free(data);
}

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
static void process_le_address_update_evt(bt_address_t* addr, ble_addr_type_t type)
{
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    memcpy(&adapter->le_properties.addr, addr, sizeof(*addr));
    adapter->le_properties.addr_type = type;
    adapter_unlock();
}

static void process_le_phy_update_evt(bt_address_t* addr, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy, bt_status_t status)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (device == NULL) {
        adapter_unlock();
        return;
    }

    if (status != BT_STATUS_SUCCESS) {
        adapter_unlock();
        char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
        bt_addr_ba2str(addr, addr_str);
        BT_LOGE("Device:%s update phy failed:0x%02x", addr_str, status);
        return;
    }

    device_set_le_phy(device, tx_phy, rx_phy);
    adapter_unlock();
}

static void process_le_whitelist_update_evt(bt_address_t* addr, bool is_add, bt_status_t status)
{
    bool is_added;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s, %s isadded:%d, status:%d", __func__, addr_str, is_add, status);

    if (status != BT_STATUS_SUCCESS) {
        return;
    }

    adapter_lock();

    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (device == NULL) {
        bt_sal_le_remove_white_list(PRIMARY_ADAPTER, addr, device_get_address_type(device));
        goto out;
    }

    is_added = device_check_flag(device, DFLAG_WHITELIST_ADDED);

    if (is_add == is_added)
        goto out;

    if (is_add) {
        device_set_flags(device, DFLAG_WHITELIST_ADDED);
    } else {
        device_clear_flag(device, DFLAG_WHITELIST_ADDED);
    }
    adapter_update_whitelist();

out:
    adapter_unlock();
}

static void process_le_bonded_device_update_evt(remote_device_le_properties_t* props, uint16_t bonded_devices_cnt)
{
    bt_device_t* device;
    remote_device_le_properties_t* prop = props;
    char addr_str[BT_ADDR_STR_LENGTH];

    adapter_lock();
    for (int i = 0; i < bonded_devices_cnt; i++) {
        device = adapter_find_device(&prop->addr, BT_TRANSPORT_BLE);
        if (!device)
            continue;

        /* device had bonded and smpkey had stored */
        if (device_check_flag(device, DFLAG_LE_KEY_SET)) {
            device_delete_smp_key(device);
        }

        device_set_address_type(device, prop->addr_type);
        /* store smp key to mapped device struct */
        device_set_smp_key(device, prop->smp_key);
        device_set_identity_address(device, (bt_address_t*)prop->smp_key);

        bt_addr_ba2str(&prop->addr, addr_str);
        uint8_t* ltk = &prop->smp_key[12];
        BT_LOGD("LE BOND DEVICE[%d]: Addr:[%s] Atype:[%d] LTK: [%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]",
            i, addr_str, prop->addr_type, ltk[0], ltk[1], ltk[2], ltk[3], ltk[4], ltk[5], ltk[6], ltk[7],
            ltk[8], ltk[9], ltk[10], ltk[11], ltk[12], ltk[13], ltk[14], ltk[15]);
        prop++;
    }

    /* update all bonded le device to storage */
    bt_storage_save_le_bonded_device(props, bonded_devices_cnt);
    free(props);
    adapter_unlock();
}

static void process_le_sc_local_oob_data_got_evt(bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    adapter_lock();

    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (device == NULL) {
        adapter_unlock();
        return;
    }

    adapter_unlock();

    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_le_sc_local_oob_data_got, addr, c_val, r_val);
}

static void handle_ble_event(void* data)
{
    adapter_ble_evt_t* evt = (adapter_ble_evt_t*)data;

    switch (evt->evt_id) {
    case LE_ADDR_UPDATE_EVT:
        process_le_address_update_evt(&evt->addr_update.local_addr, evt->addr_update.type);
        break;
    case LE_PHY_UPDATE_EVT:
        process_le_phy_update_evt(&evt->phy_update.addr, evt->phy_update.tx_phy,
            evt->phy_update.rx_phy, evt->phy_update.status);
        break;
    case LE_WHITELIST_UPDATE_EVT:
        process_le_whitelist_update_evt(&evt->whitelist.addr,
            evt->whitelist.is_add,
            evt->whitelist.status);
        break;
    case LE_BONDED_DEVICE_UPDATE_EVT:
        process_le_bonded_device_update_evt(evt->bonded_devices.props,
            evt->bonded_devices.bonded_devices_cnt);
        break;
    case LE_SC_LOCAL_OOB_DATA_GOT_EVT:
        process_le_sc_local_oob_data_got_evt(&evt->oob_data.addr,
            evt->oob_data.c_val,
            evt->oob_data.r_val);
        break;
    default:
        break;
    }

    free(data);
}
#endif

#ifdef CONFIG_UORB
static void adapter_broadcast_state(int state)
{
    adapter_service_t* adapter = &g_adapter_service;
    struct bt_stack_state uORB_state;
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    memset(&uORB_state, 0, sizeof(uORB_state));
    uORB_state.timestamp = ts.tv_sec * 1000 + ts.tv_nsec / 1000000UL;
    uORB_state.state = state;

    if (adapter->adapter_state_adv > 0) {
        int ret = orb_publish(ORB_ID(bt_stack_state), adapter->adapter_state_adv, &uORB_state);
        if (ret != 0)
            BT_LOGE("Failed to publish stack state, ret: %d", ret);
    } else
        BT_LOGE("%s error advertise orb fd: %d", __func__, adapter->adapter_state_adv);
}
#endif

void adapter_notify_state_change(bt_adapter_state_t prev, bt_adapter_state_t current)
{
    adapter_service_t* adapter = &g_adapter_service;

    BT_LOGD("%s, prev:%d--->current:%d", __func__, prev, current);

#ifdef CONFIG_UORB
    if (current == BT_ADAPTER_STATE_ON)
        adapter_broadcast_state(BT_STACK_STATE_ON);
    else if (current == BT_ADAPTER_STATE_OFF)
        adapter_broadcast_state(BT_STACK_STATE_OFF);
#endif

    adapter_lock();
    adapter->adapter_state = current;
    adapter_unlock();
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_adapter_state_changed, current);
}

void adapter_on_adapter_state_changed(uint8_t stack_state)
{
    uint16_t event;
    adapter_service_t* adapter = &g_adapter_service;

    switch (stack_state) {
    case BT_BREDR_STACK_STATE_ON: {
        adapter_storage_t storage;
        int ret;

        bt_storage_load_adapter_info(&storage);
        adapter_properties_copy(&adapter->properties, &storage);

        /* load bonded devices to stack (name/address/cod/alias/linkkey) */
        ret = bt_storage_load_bonded_device(bonded_device_loaded);
        if (ret < 0) {
            BT_LOGE("%s, load_bonded_device err:%d", __func__, ret);
            bonded_device_loaded(NULL, 0, 0);
        }

        /* waiting for device load finished */
        return;
    }
    case BT_BREDR_STACK_STATE_OFF:
        event = BREDR_DISABLED;
        break;
    case BLE_STACK_STATE_ON:
        event = BLE_ENABLED;
        break;
    case BLE_STACK_STATE_OFF:
        event = BLE_DISABLED;
        break;
    default:
        return;
    }
    send_to_state_machine((state_machine_t*)adapter->stm, event, NULL);
}

void adapter_on_le_enabled(bool enablebt)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;
    int ret;

    BT_LOGD("%s, enablebt:%d", __func__, enablebt);
    /* get le address async */
    bt_sal_le_get_address(PRIMARY_ADAPTER);
    /* set le io capability ? */
    /* set appearance ? */
    /* load bonded device to stack ? SMP keys */
    ret = bt_storage_load_le_bonded_device(le_bonded_device_loaded);
    if (ret < 0) {
        le_bonded_device_loaded(NULL, 0, 0);
    }
    /* set white list ? */
    ret = bt_storage_load_whitelist_device(whitelist_device_loaded);
    if (ret < 0) {
        whitelist_device_loaded(NULL, 0, 0);
    }

    /* set resolvinglist list ? */
    /* enable cdtk */
    // bt_sal_le_enable_key_derivation(true, true);

    /* enable advertiser manager */
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    adv_manager_init();
#endif

    /* enable scan manager */
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    scan_manager_init();
#endif
    /* enable L2CAP service */
#ifdef CONFIG_BLUETOOTH_L2CAP
    if (!enablebt)
        l2cap_service_init();
#endif
    /* startup gatt service */
    if (enablebt)
        send_to_state_machine((state_machine_t*)adapter->stm, SYS_TURN_ON, NULL);
#endif
}

void adapter_on_le_disabled(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    BT_LOGD("%s", __func__);
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    adv_manager_cleanup();
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    adapter_lock();
    bt_list_clear(g_adapter_service.le_devices);
    adapter_unlock();
    scan_manager_cleanup();
#endif
#ifdef CONFIG_BLUETOOTH_L2CAP
    l2cap_service_cleanup();
#endif
    /* wait save info done*/
#endif
}

void adapter_on_br_enabled(void)
{
    adapter_properties_t* props = &g_adapter_service.properties;
    char addrstr[BT_ADDR_STR_LENGTH];

    /* set local name */
    bt_sal_set_name(PRIMARY_ADAPTER, props->name);
    /* get local address */
    bt_sal_get_address(PRIMARY_ADAPTER, &props->addr);
    /* set io capability, first load stored adapter info, or use Kconfig default */
    bt_sal_set_io_capability(PRIMARY_ADAPTER, props->io_capability);
    /* set scan mode, no discoverable no connectable */
    bt_sal_set_scan_mode(PRIMARY_ADAPTER, props->scan_mode, props->bondable);
    /* set local class of device  */
    bt_sal_set_device_class(PRIMARY_ADAPTER, props->class_of_device);
    /* set default inquiry scan parameter */
    /*  */
    /* enable L2CAP service */
#ifdef CONFIG_BLUETOOTH_L2CAP
    l2cap_service_init();
#endif

    bt_addr_ba2str(&props->addr, addrstr);
    BT_LOGI("Adapter Info:\n"
            "\tName:%s\n"
            "\tAddress:%s\n"
            "\tIoCap:%" PRIu32 "\n"
            "\tScanmode:%d\n"
            "\tBondable:%d\n"
            "\tDeviceClass:0x%08" PRIx32 "\n",
        props->name, addrstr,
        props->io_capability, props->scan_mode, props->bondable,
        props->class_of_device);
}

void adapter_on_br_disabled(void)
{
    BT_LOGD("%s", __func__);

    adapter_lock();
    bt_list_clear(g_adapter_service.devices);
    adapter_unlock();
}

static void handle_scan_mode_changed(void* data)
{
    bt_scan_mode_t scan_mode = *((bt_scan_mode_t*)data);

    free(data);
    adapter_lock();
    adapter_save_properties();
    adapter_unlock();
    /* notify properties changed */
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_scan_mode_changed, scan_mode);
}

static void process_link_role_changed_evt(bt_address_t* addr, bt_link_role_t role)
{
    bt_device_t* device;
    uint32_t cod;
    bt_link_policy_t policy;
    bool disable_policy = false;

    /* callback on HCI Role Change event received,
       only BT_LINK_ROLE_MASTER or BT_LINK_ROLE_SLAVE are possible */
    BT_ADDR_LOG("Link role switched at %s, new local role: %s", addr,
        role == BT_LINK_ROLE_MASTER ? "Master" : "Slave");

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device) {
        device_set_local_role(device, role);
        cod = device_get_device_class(device);
        policy = device_get_link_policy(device);
        if (IS_HEADSET(cod) && role == BT_LINK_ROLE_MASTER)
            disable_policy = true;
    }

    adapter_unlock();
    if (disable_policy) {
        BT_ADDR_LOG("Disable role switch at %s", addr);
        policy &= ~BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH;
        bt_sal_set_link_policy(PRIMARY_ADAPTER, addr, policy);
    }
}

static void process_link_policy_changed_evt(bt_address_t* addr, bt_link_policy_t policy)
{
    bt_device_t* device;

    BT_ADDR_LOG("Link policy changed at %s, role switch: %s, sniff: %s", addr,
        policy & BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH ? "enabled" : "disabled",
        policy & BT_BR_LINK_POLICY_ENABLE_SNIFF ? "enabled" : "disabled");

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device)
        device_set_link_policy(device, policy);

    adapter_unlock();
}

static void handle_link_event(void* data)
{
    adapter_remote_event_t* evt = (adapter_remote_event_t*)data;
    switch (evt->evt_id) {
    case LINK_MODE_CHANGED_EVT:
        bt_pm_remote_link_mode_changed(&evt->addr, evt->link_mode.mode, evt->link_mode.sniff_interval);
        break;
    case LINK_ROLE_CHANGED_EVT:
        process_link_role_changed_evt(&evt->addr, evt->link_role.role);
        break;
    case LINK_POLICY_CHANGED_EVT:
        process_link_policy_changed_evt(&evt->addr, evt->link_policy.policy);
        break;
    }

    free(data);
}

void adapter_on_scan_mode_changed(bt_scan_mode_t mode)
{
    bt_scan_mode_t* scan_mode = malloc(sizeof(bt_scan_mode_t));

    *scan_mode = mode;
    do_in_service_loop(handle_scan_mode_changed, scan_mode);
}

void adapter_on_discovery_state_changed(bt_discovery_state_t state)
{
    adapter_discovery_evt_t* evt = malloc(sizeof(adapter_discovery_evt_t));
    if (!evt)
        return;

    evt->evt_id = DISCOVER_STATE_CHANGE_EVT;
    evt->state = state;
    do_in_service_loop(handle_discovery_event, evt);
}

void adapter_on_device_found(bt_discovery_result_t* result)
{
    adapter_discovery_evt_t* evt = malloc(sizeof(adapter_discovery_evt_t));
    if (!evt)
        return;

    evt->evt_id = DEVICE_FOUND_EVT;
    memcpy(&evt->result, result, sizeof(bt_discovery_result_t));
    do_in_service_loop(handle_discovery_event, evt);
}

void adapter_on_remote_name_recieved(bt_address_t* addr, const char* name)
{
    adapter_discovery_evt_t* evt = malloc(sizeof(adapter_discovery_evt_t));
    if (!evt)
        return;

    evt->evt_id = REMOTE_NAME_RECIEVED_EVT;
    memcpy(&evt->remote_name.addr, addr, sizeof(bt_address_t));
    if (name) {
        strncpy((char*)evt->remote_name.name, name, BT_REM_NAME_MAX_LEN);
    } else {
        evt->remote_name.name[0] = '\0';
    }

    do_in_service_loop(handle_discovery_event, evt);
}

void adapter_on_connect_request(bt_address_t* addr, uint32_t cod)
{
    adapter_remote_event_t* evt = create_remote_event(addr, CONNECT_REQUEST_EVT);
    if (!evt)
        return;

    evt->cod = cod;
    do_in_service_loop(handle_connection_event, evt);
}

void adapter_on_connection_state_changed(acl_state_param_t* param)
{
    adapter_remote_event_t* evt = create_remote_event(&param->addr, CONNECTION_STATE_CHANGE_EVT);
    if (!evt)
        return;

    memcpy(&evt->acl_params, param, sizeof(acl_state_param_t));
    do_in_service_loop(handle_connection_event, evt);
}

void adapter_on_pairing_request(bt_address_t* addr, bool local_initiate, bool is_bondable)
{
    adapter_remote_event_t* evt = create_remote_event(addr, PAIR_REQUEST_EVT);
    if (!evt)
        return;

    evt->pair_req.local_initiate = local_initiate;
    evt->pair_req.is_bondable = is_bondable;
    do_in_service_loop(handle_security_event, evt);
}

void adapter_on_pin_request(bt_address_t* addr, uint32_t cod,
    bool min_16_digit, const char* name)
{
    adapter_remote_event_t* evt = create_remote_event(addr, PIN_REQUEST_EVT);
    if (!evt)
        return;

    evt->pin_req.cod = cod;
    evt->pin_req.min_16_digit = min_16_digit;
    if (name)
        strncpy(evt->pin_req.name, name, BT_REM_NAME_MAX_LEN);

    do_in_service_loop(handle_security_event, evt);
}

/* simple security pairing request or le smp pairing */
void adapter_on_ssp_request(bt_address_t* addr, uint8_t transport,
    uint32_t cod, bt_pair_type_t ssp_type,
    uint32_t pass_key, const char* name)
{
    adapter_remote_event_t* evt = create_remote_event(addr, SSP_REQUEST_EVT);
    if (!evt)
        return;

    evt->ssp_req.cod = cod;
    evt->ssp_req.ssp_type = ssp_type;
    evt->ssp_req.pass_key = pass_key;
    evt->ssp_req.transport = transport;
    if (name)
        strncpy(evt->ssp_req.name, name, BT_REM_NAME_MAX_LEN);

    do_in_service_loop(handle_security_event, evt);
}

void adapter_on_bond_state_changed(bt_address_t* addr, bond_state_t state, uint8_t transport, bt_status_t status, bool is_ctkd)
{
    adapter_remote_event_t* evt = create_remote_event(addr, BOND_STATE_CHANGE_EVT);
    if (!evt)
        return;

    evt->bond_state.state = state;
    evt->bond_state.transport = transport;
    evt->bond_state.is_ctkd = is_ctkd;
    evt->bond_state.status = status;
    do_in_service_loop(handle_security_event, evt);
}

void adapter_on_service_search_done(bt_address_t* addr, bt_uuid_t* uuids, uint16_t size)
{
    adapter_remote_event_t* evt = create_remote_event(addr, SDP_SEARCH_DONE_EVT);
    if (!evt)
        return;

    evt->sdp.uuid_size = size;
    evt->sdp.uuids = malloc(sizeof(bt_uuid_t) * size);
    if (!evt->sdp.uuids) {
        free(evt);
        return;
    }
    memcpy(evt->sdp.uuids, uuids, sizeof(bt_uuid_t) * size);
    do_in_service_loop(handle_security_event, evt);
}

void adapter_on_encryption_state_changed(bt_address_t* addr, bool encrypted, uint8_t transport)
{
    adapter_remote_event_t* evt = create_remote_event(addr, ENC_STATE_CHANGE_EVT);
    if (!evt)
        return;

    evt->enc_state.encrypted = encrypted;
    evt->enc_state.transport = transport;
    do_in_service_loop(handle_security_event, evt);
}

void adapter_on_link_key_update(bt_address_t* addr, bt_128key_t link_key, bt_link_key_type_t type)
{
    adapter_remote_event_t* evt = create_remote_event(addr, LINK_KEY_UPDATE_EVT);
    if (!evt)
        return;

    memcpy(evt->link_key.key, link_key, sizeof(bt_128key_t));
    evt->link_key.type = type;
    do_in_service_loop(handle_security_event, evt);
}

void adapter_on_link_key_removed(bt_address_t* addr, bt_status_t status)
{
    adapter_remote_event_t* evt = create_remote_event(addr, LINK_KEY_REMOVED_EVT);
    if (!evt)
        return;

    evt->link_key.status = status;
    do_in_service_loop(handle_security_event, evt);
}

void adapter_on_link_role_changed(bt_address_t* addr, bt_link_role_t role)
{
    BT_LOGD("%s", __func__);
    adapter_remote_event_t* evt = create_remote_event(addr, LINK_ROLE_CHANGED_EVT);
    if (!evt)
        return;

    evt->link_role.role = role;
    do_in_service_loop(handle_link_event, evt);
}

/* PM need implement */
void adapter_on_link_mode_changed(bt_address_t* addr, bt_link_mode_t mode, uint16_t sniff_interval)
{
    adapter_remote_event_t* evt = create_remote_event(addr, LINK_MODE_CHANGED_EVT);
    if (!evt)
        return;

    evt->link_mode.mode = mode;
    evt->link_mode.sniff_interval = sniff_interval;
    do_in_service_loop(handle_link_event, evt);
}

void adapter_on_link_policy_changed(bt_address_t* addr, bt_link_policy_t policy)
{
    BT_LOGD("%s", __func__);
    adapter_remote_event_t* evt = create_remote_event(addr, LINK_POLICY_CHANGED_EVT);
    if (!evt)
        return;

    evt->link_policy.policy = policy;
    do_in_service_loop(handle_link_event, evt);
}

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
void adapter_on_le_addr_update(bt_address_t* addr, ble_addr_type_t type)
{
    BT_LOGD("%s", __func__);

    adapter_ble_evt_t* evt = malloc(sizeof(adapter_ble_evt_t));

    evt->evt_id = LE_ADDR_UPDATE_EVT;
    memcpy(&evt->addr_update.local_addr, addr, sizeof(*addr));
    evt->addr_update.type = type;

    do_in_service_loop(handle_ble_event, evt);
}

void adapter_on_le_phy_update(bt_address_t* addr, ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy, bt_status_t status)
{
    BT_LOGD("%s", __func__);

    adapter_ble_evt_t* evt = malloc(sizeof(adapter_ble_evt_t));

    evt->evt_id = LE_PHY_UPDATE_EVT;
    memcpy(&evt->phy_update.addr, addr, sizeof(*addr));
    evt->phy_update.tx_phy = tx_phy;
    evt->phy_update.rx_phy = rx_phy;
    evt->phy_update.status = status;

    do_in_service_loop(handle_ble_event, evt);
}

void adapter_on_whitelist_update(bt_address_t* addr, bool is_add, bt_status_t status)
{
    BT_LOGD("%s", __func__);

    adapter_ble_evt_t* evt = malloc(sizeof(adapter_ble_evt_t));

    evt->evt_id = LE_WHITELIST_UPDATE_EVT;
    memcpy(&evt->whitelist.addr, addr, sizeof(*addr));
    evt->whitelist.is_add = is_add;
    evt->whitelist.status = status;

    do_in_service_loop(handle_ble_event, evt);
}

void adapter_on_le_bonded_device_update(remote_device_le_properties_t* props, uint16_t bonded_devices_cnt)
{
    BT_LOGD("%s", __func__);

    adapter_ble_evt_t* evt = malloc(sizeof(adapter_ble_evt_t));

    evt->evt_id = LE_BONDED_DEVICE_UPDATE_EVT;
    size_t prop_size = sizeof(remote_device_le_properties_t) * bonded_devices_cnt;
    evt->bonded_devices.props = malloc(prop_size);
    evt->bonded_devices.bonded_devices_cnt = bonded_devices_cnt;
    memcpy(evt->bonded_devices.props, props, prop_size);

    do_in_service_loop(handle_ble_event, evt);
}

void adapter_on_le_local_oob_data_got(bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    adapter_ble_evt_t* evt = malloc(sizeof(adapter_ble_evt_t));

    evt->evt_id = LE_SC_LOCAL_OOB_DATA_GOT_EVT;
    memcpy(&evt->oob_data.addr, addr, sizeof(evt->oob_data.addr));
    memcpy(evt->oob_data.c_val, c_val, sizeof(evt->oob_data.c_val));
    memcpy(evt->oob_data.r_val, r_val, sizeof(evt->oob_data.r_val));

    do_in_service_loop(handle_ble_event, evt);
}
#endif

void adapter_init(void)
{
    adapter_service_t* adapter = &g_adapter_service;
    pthread_mutexattr_t attr;

    memset(adapter, 0, sizeof(g_adapter_service));
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&adapter->adapter_lock, &attr);

    adapter->is_discovering = false;
    adapter->max_acl_connections = 10;
    adapter->devices = bt_list_new(adapter_delete_device);
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter->le_devices = bt_list_new(adapter_delete_device);
#endif
    adapter->adapter_callbacks = bt_callbacks_list_new(CONFIG_BLUETOOTH_MAX_REGISTER_NUM);
    adapter->stm = adapter_state_machine_new(NULL);
    adapter->adapter_state_adv = -1;
#ifdef CONFIG_UORB
    adapter->adapter_state_adv = orb_advertise_multi_queue_persist(ORB_ID(bt_stack_state),
        NULL, NULL, 1);
    if (adapter->adapter_state_adv < 0)
        BT_LOGE("adapter service state advertise failed :%d", adapter->adapter_state_adv);
#endif
}

void* adapter_register_callback(void* remote, const adapter_callbacks_t* adapter_cbs)
{
    return (void*)bt_remote_callbacks_register(g_adapter_service.adapter_callbacks, remote, (void*)adapter_cbs);
}

bool adapter_unregister_callback(void** remote, void* cookie)
{
    return bt_remote_callbacks_unregister(g_adapter_service.adapter_callbacks, remote, (remote_callback_t*)cookie);
}

#if 0
void *adapter_register_remote_callback(void *remote, const remote_device_callbacks_t *remote_cbs)
{
    return true;
}

bool adapter_unregister_remote_callback(void **remote, void *cookie)
{
    return true;
}
#endif

bt_status_t adapter_send_event(uint16_t event_id, void* data)
{
    adapter_service_t* adapter = &g_adapter_service;

    send_to_state_machine((state_machine_t*)adapter->stm, event_id, data);

    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_on_profile_services_startup(uint8_t transport, bool ret)
{
    adapter_service_t* adapter = &g_adapter_service;

    BT_LOGD("%s transport all profiles is startup", transport == BT_TRANSPORT_BREDR ? "BREDR" : "BLE");

    uint16_t event = transport == BT_TRANSPORT_BREDR ? BREDR_PROFILE_ENABLED : BLE_PROFILE_ENABLED;
    send_to_state_machine((state_machine_t*)adapter->stm, event, NULL);

    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_on_profile_services_shutdown(uint8_t transport, bool ret)
{
    adapter_service_t* adapter = &g_adapter_service;

    BT_LOGD("%s transport all profiles is shutdown", transport == BT_TRANSPORT_BREDR ? "BREDR" : "BLE");

    uint16_t event = transport == BT_TRANSPORT_BREDR ? BREDR_PROFILE_DISABLED : BLE_PROFILE_DISABLED;
    send_to_state_machine((state_machine_t*)adapter->stm, event, NULL);

    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_enable(uint8_t opt)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_adapter_state_t state = adapter_get_state();

    if (state == BT_ADAPTER_STATE_ON) {
        return BT_STATUS_DONE;
    }

    if (opt == SYS_SET_BT_ALL)
        send_to_state_machine((state_machine_t*)adapter->stm, SYS_TURN_ON, NULL);
    else
        send_to_state_machine((state_machine_t*)adapter->stm, TURN_ON_BLE, NULL);

    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_disable(uint8_t opt)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_adapter_state_t state = adapter_get_state();

    if (state == BT_ADAPTER_STATE_OFF) {
        return BT_STATUS_DONE;
    }

    if (opt == SYS_SET_BT_ALL)
        send_to_state_machine((state_machine_t*)adapter->stm, SYS_TURN_OFF, NULL);
    else
        send_to_state_machine((state_machine_t*)adapter->stm, TURN_OFF_BLE, NULL);

    return BT_STATUS_SUCCESS;
}

void adapter_cleanup(void)
{
    adapter_service_t* adapter = &g_adapter_service;

    /*TODO: disable adapter services brefore cleanup */
    //
#ifdef CONFIG_UORB
    if (adapter->adapter_state_adv > 0)
        orb_unadvertise(adapter->adapter_state_adv);
#endif
    if (adapter->stm) {
        adapter_lock();
        bt_list_free(adapter->devices);
        adapter->devices = NULL;
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        bt_list_free(adapter->le_devices);
        adapter->le_devices = NULL;
#endif
        bt_callbacks_list_free(adapter->adapter_callbacks);
        adapter->adapter_callbacks = NULL;
        adapter_state_machine_destory(adapter->stm);
        adapter_unlock();
        pthread_mutex_destroy(&adapter->adapter_lock);
    }
}

bt_adapter_state_t adapter_get_state(void)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_adapter_state_t state;

    adapter_lock();
    state = adapter->adapter_state;
    adapter_unlock();

    return state;
}

bool adapter_is_le_enabled(void)
{
    bt_adapter_state_t state;

    if (!adapter_is_support_le())
        return false;

    state = adapter_get_state();
    if (state == BT_ADAPTER_STATE_BLE_ON || state == BT_ADAPTER_STATE_TURNING_ON || state == BT_ADAPTER_STATE_TURNING_OFF || state == BT_ADAPTER_STATE_ON)
        return true;

    return false;
}

bt_device_type_t adapter_get_type(void)
{
#if defined(CONFIG_KVDB) && defined(__NuttX__)
    return property_get_int32("persist.bluetooth.adapter.type", 2);
#else
    return BT_DEVICE_TYPE_DUAL;
#endif
}

bt_status_t adapter_set_discovery_filter(void)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t adapter_start_discovery(uint32_t timeout)
{
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    if (adapter->adapter_state != BT_ADAPTER_STATE_ON) {
        adapter_unlock();
        return BT_STATUS_NOT_ENABLED;
    }

    if (adapter->is_discovering) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    bt_status_t status = bt_sal_start_discovery(PRIMARY_ADAPTER, timeout);
    if (status != BT_STATUS_SUCCESS) {
        adapter_unlock();
        return status;
    }

    adapter->is_discovering = true;
    adapter_unlock();
    return status;
}

bt_status_t adapter_cancel_discovery(void)
{
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    if (adapter->adapter_state != BT_ADAPTER_STATE_ON) {
        adapter_unlock();
        return BT_STATUS_NOT_ENABLED;
    }

    if (!adapter->is_discovering) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    bt_status_t status = bt_sal_stop_discovery(PRIMARY_ADAPTER);
    adapter->is_discovering = false;
    adapter_unlock();

    return status;
}

bool adapter_is_discovering(void)
{
    adapter_service_t* adapter = &g_adapter_service;
    bool is_discovering;

    adapter_lock();
    is_discovering = adapter->is_discovering;
    adapter_unlock();

    return is_discovering;
}

void adapter_get_address(bt_address_t* addr)
{
    adapter_lock();
    memcpy(addr, &g_adapter_service.properties.addr, sizeof(bt_address_t));
    adapter_unlock();
}

bt_status_t adapter_set_name(const char* name)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_status_t status = BT_STATUS_SUCCESS;

    if (strlen(name) > BT_LOC_NAME_MAX_LEN)
        return BT_STATUS_PARM_INVALID;

    adapter_lock();
    CHECK_ADAPTER_READY();
    if (strncmp(adapter->properties.name, name, BT_LOC_NAME_MAX_LEN) == 0)
        goto error;

    status = bt_sal_set_name(PRIMARY_ADAPTER, (char*)name);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    strncpy(adapter->properties.name, name, BT_LOC_NAME_MAX_LEN);
    adapter_save_properties();
    adapter_unlock();
    /* TODO notify properties changed */
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_device_name_changed, name);
    return status;
error:
    adapter_unlock();
    return status;
}

void adapter_get_name(char* name, int size)
{
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    strlcpy(name, adapter->properties.name, size);
    adapter_unlock();
}

bt_status_t adapter_get_uuids(bt_uuid_t* uuids, uint16_t* size)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t adapter_set_scan_mode(bt_scan_mode_t mode, bool bondable)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_status_t status = BT_STATUS_SUCCESS;

    adapter_lock();
    CHECK_ADAPTER_READY();
    if (adapter->properties.scan_mode == mode && adapter->properties.bondable == bondable)
        goto error;

    status = bt_sal_set_scan_mode(PRIMARY_ADAPTER, mode, bondable);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    adapter->properties.scan_mode = mode;
    adapter->properties.bondable = bondable;

error:
    adapter_unlock();
    return status;
}

bt_scan_mode_t adapter_get_scan_mode(void)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_scan_mode_t mode;

    adapter_lock();
    mode = adapter->properties.scan_mode;
    adapter_unlock();

    return mode;
}

bt_status_t adapter_set_device_class(uint32_t cod)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_status_t status = BT_STATUS_SUCCESS;

    adapter_lock();
    CHECK_ADAPTER_READY();
    if (adapter->properties.class_of_device == cod)
        goto error;

    status = bt_sal_set_device_class(PRIMARY_ADAPTER, cod);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    adapter->properties.class_of_device = cod;
    adapter_save_properties();
error:
    adapter_unlock();
    return status;
}

uint32_t adapter_get_device_class(void)
{
    adapter_service_t* adapter = &g_adapter_service;
    uint32_t cod;

    adapter_lock();
    cod = adapter->properties.class_of_device;
    adapter_unlock();

    return cod;
}

bt_status_t adapter_set_io_capability(bt_io_capability_t cap)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_status_t status = BT_STATUS_SUCCESS;

    adapter_lock();
    CHECK_ADAPTER_READY();
    if (adapter->properties.io_capability == cap)
        goto error;

    status = bt_sal_set_io_capability(PRIMARY_ADAPTER, cap);
    if (status != BT_STATUS_SUCCESS)
        goto error;

    adapter->properties.io_capability = cap;
    adapter_save_properties();

error:
    adapter_unlock();
    return status;
}

bt_io_capability_t adapter_get_io_capability(void)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_io_capability_t cap;

    adapter_lock();
    cap = adapter->properties.io_capability;
    adapter_unlock();

    return cap;
}

bt_status_t adapter_set_inquiry_scan_parameters(bt_scan_type_t type,
    uint16_t interval,
    uint16_t window)
{
    return bt_sal_set_inquiry_scan_parameters(PRIMARY_ADAPTER, type, interval, window);
}

bt_status_t adapter_set_page_scan_parameters(bt_scan_type_t type,
    uint16_t interval,
    uint16_t window)
{
    return bt_sal_set_page_scan_parameters(PRIMARY_ADAPTER, type, interval, window);
}

bt_status_t adapter_get_le_address(bt_address_t* addr, ble_addr_type_t* type)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    memcpy(addr, &adapter->le_properties.addr, sizeof(*addr));
    *type = adapter->le_properties.addr_type;
    /* TODO notify properties changed */
    adapter_unlock();

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_set_le_address(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    return bt_sal_le_set_address(PRIMARY_ADAPTER, addr);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_set_le_identity_address(bt_address_t* addr, bool public)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    // adapter_service_t *adapter = &g_adapter_service;
    if (public)
        bt_sal_le_set_public_identity(PRIMARY_ADAPTER, addr);
    else
        bt_sal_le_set_static_identity(PRIMARY_ADAPTER, addr);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_set_le_io_capability(uint32_t le_io_cap)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    adapter->le_properties.le_io_capability = le_io_cap;
    /* TODO update storage */
    /* TODO notify properties changed */
    adapter_unlock();
    bt_sal_le_set_io_capability(PRIMARY_ADAPTER, le_io_cap);

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

uint32_t adapter_get_le_io_capability(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;
    uint32_t cap;

    adapter_lock();
    cap = adapter->le_properties.le_io_capability;
    adapter_unlock();

    return cap;
#else
    return 0;
#endif
}

bt_status_t adapter_set_le_appearance(uint16_t appearance)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;

    adapter_lock();
    bt_status_t status = bt_sal_le_set_appearance(PRIMARY_ADAPTER, appearance);
    if (status != BT_STATUS_SUCCESS) {
        adapter_unlock();
        return status;
    }

    adapter->le_properties.le_appearance = appearance;
    /* TODO update storage */
    /* TODO notify properties changed */
    adapter_unlock();

    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

uint16_t adapter_get_le_appearance(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;
    uint16_t appearance;

    adapter_lock();
    appearance = adapter->le_properties.le_appearance;
    adapter_unlock();

    return appearance;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

static bt_status_t adapter_get_devices(int flag, bt_address_t** addr, int* size, bt_allocator_t allocator, uint8_t transport)
{
    bt_list_t* list;
    bt_list_node_t* node;

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    if (transport == BT_TRANSPORT_BLE) {
        list = g_adapter_service.le_devices;
    } else
#endif
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
        if (transport == BT_TRANSPORT_BREDR) {
        list = g_adapter_service.devices;
    } else
#endif
    {
        return BT_STATUS_PARM_INVALID;
    }

    *size = 0;
    adapter_lock();
    int cnt = get_devices_cnt(flag, transport);
    if (!cnt) {
        adapter_unlock();
        return BT_STATUS_SUCCESS;
    }

    if (!allocator((void**)addr, sizeof(bt_address_t) * cnt)) {
        adapter_unlock();
        return BT_STATUS_NOMEM;
    }

    *size = cnt;
    cnt = 0;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        if ((flag == DFLAG_BONDED && device_is_bonded(device)) || (flag == DFLAG_CONNECTED && device_is_connected(device))) {
            memcpy(*addr + cnt, device_get_address(device), sizeof(bt_address_t));
            cnt++;
        }
    }
    adapter_unlock();

    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_get_bonded_devices(bt_transport_t transport, bt_address_t** addr, int* size, bt_allocator_t allocator)
{
    return adapter_get_devices(DFLAG_BONDED, addr, size, allocator, transport);
}

bt_status_t adapter_get_connected_devices(bt_transport_t transport, bt_address_t** addr, int* size, bt_allocator_t allocator)
{
    return adapter_get_devices(DFLAG_CONNECTED, addr, size, allocator, transport);
}

/*

bt_device_t *adapter_get_remote_device(bt_address_t *addr)
{

}
*/

bool adapter_is_support_bredr(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    return true;
#endif
    return false;
}

bool adapter_is_support_le(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    return true;
#endif
    return false;
}

bool adapter_is_support_leaudio(void)
{
#ifdef CONFIG_BLUETOOTH_LE_AUDIO_SUPPORT
    return true;
#endif
    return false;
}

bt_status_t adapter_get_remote_identity_address(bt_address_t* bd_addr, bt_address_t* id_addr)
{
    bt_device_t* device;
    bt_address_t* identity_addr;

    adapter_lock();
    device = adapter_find_device(bd_addr, BT_TRANSPORT_BLE);
    if (device == NULL) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    identity_addr = device_get_identity_address(device);
    if (bt_addr_is_empty(identity_addr)) {
        adapter_unlock();
        return BT_STATUS_NOT_FOUND;
    }

    memcpy(id_addr, identity_addr, sizeof(bt_address_t));
    adapter_unlock();
    return BT_STATUS_SUCCESS;
}

bt_device_type_t adapter_get_remote_device_type(bt_address_t* addr)
{
    bt_device_t* device;
    bt_device_type_t device_type = 0;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device != NULL) {
        device_type |= device_get_device_type(device);
        device_type |= BT_DEVICE_TYPE_BREDR;
    }

    device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (device != NULL) {
        device_type |= device_get_device_type(device);
        device_type |= BT_DEVICE_TYPE_BLE;
    }
    adapter_unlock();

    return device_type;
}

bool adapter_get_remote_name(bt_address_t* addr, char* name)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device == NULL) {
        adapter_unlock();
        return false;
    }

    memcpy(name, device_get_name(device), BT_REM_NAME_MAX_LEN);
    adapter_unlock();
    return true;
}

uint32_t adapter_get_remote_device_class(bt_address_t* addr)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device == NULL) {
        adapter_unlock();
        return 0;
    }

    uint32_t cod = device_get_device_class(device);
    adapter_unlock();

    return cod;
}

bt_status_t adapter_get_remote_uuids(bt_address_t* addr, bt_uuid_t** uuids, uint16_t* size, bt_allocator_t allocator)
{
    bt_device_t* device;

    *size = 0;
    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device == NULL) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    *size = device_get_uuids_size(device);
    if (*size == 0) {
        adapter_unlock();
        return BT_STATUS_SUCCESS;
    }

    if (!allocator((void**)uuids, sizeof(bt_uuid_t) * (*size))) {
        adapter_unlock();
        return BT_STATUS_NOMEM;
    }

    *size = device_get_uuids(device, *uuids, *size);
    adapter_unlock();

    return BT_STATUS_SUCCESS;
}

uint16_t adapter_get_remote_appearance(bt_address_t* addr)
{
    return 0;
}

int8_t adapter_get_remote_rssi(bt_address_t* addr)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device == NULL) {
        adapter_unlock();
        return 0;
    }

    int8_t rssi = device_get_rssi(device);
    adapter_unlock();

    return rssi;
}

bool adapter_get_remote_alias(bt_address_t* addr, char* alias)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device == NULL) {
        adapter_unlock();
        return false;
    }

    strncpy(alias, device_get_alias(device), BT_REM_NAME_MAX_LEN);
    adapter_unlock();
    return true;
}

bt_status_t adapter_set_remote_alias(bt_address_t* addr, const char* alias)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (device == NULL) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    device_set_alias(device, alias);
    adapter_unlock();
    CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_remote_alias_changed, addr, alias);

    return BT_STATUS_SUCCESS;
}

bool adapter_is_remote_connected(bt_address_t* addr, bt_transport_t transport)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, transport);
    if (device == NULL) {
        adapter_unlock();
        return false;
    }

    bool connected = device_is_connected(device);
    adapter_unlock();

    return connected;
}

bool adapter_is_remote_encrypted(bt_address_t* addr, bt_transport_t transport)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, transport);
    if (device == NULL) {
        adapter_unlock();
        return false;
    }

    bool enc = device_is_encrypted(device);
    adapter_unlock();

    return enc;
}

bool adapter_is_bond_initiate_local(bt_address_t* addr, bt_transport_t transport)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, transport);
    if (device == NULL) {
        adapter_unlock();
        return false;
    }

    bool isbondlocal = device_is_bond_initiate_local(device);
    adapter_unlock();

    return isbondlocal;
}

bond_state_t adapter_get_remote_bond_state(bt_address_t* addr, bt_transport_t transport)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, transport);
    if (device == NULL) {
        adapter_unlock();
        return BOND_STATE_NONE;
    }

    bond_state_t state = device_get_bond_state(device);
    adapter_unlock();

    return state;
}

bool adapter_is_remote_bonded(bt_address_t* addr, bt_transport_t transport)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, transport);
    if (device == NULL) {
        adapter_unlock();
        return false;
    }

    bool bonded = device_is_bonded(device);
    adapter_unlock();

    return bonded;
}

bt_status_t adapter_connect(bt_address_t* addr)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_create_classic_device(addr);
    if (bt_sal_connect(PRIMARY_ADAPTER, addr) != BT_STATUS_SUCCESS) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    device_set_connection_state(device, CONNECTION_STATE_CONNECTING);
    adapter_unlock();
    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_disconnect(bt_address_t* addr)
{
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    if (device_get_connection_state(device) == CONNECTION_STATE_DISCONNECTED || device_get_connection_state(device) == CONNECTION_STATE_DISCONNECTING) {
        adapter_unlock();
        return BT_STATUS_BUSY;
    }

    if (bt_sal_disconnect(PRIMARY_ADAPTER, addr,
            HCI_ERR_CONNECTION_TERMINATED_BY_LOCAL_HOST)
        != BT_STATUS_SUCCESS) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    device_set_connection_state(device, CONNECTION_STATE_DISCONNECTING);
    adapter_unlock();
    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_le_connect(bt_address_t* addr,
    ble_addr_type_t type,
    ble_connect_params_t* param)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_create_le_device(addr, type);
    if (bt_sal_le_connect(PRIMARY_ADAPTER, addr, type, param) != BT_STATUS_SUCCESS) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    device_set_connection_state(device, CONNECTION_STATE_CONNECTING);
    adapter_unlock();
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_le_disconnect(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    bt_device_t* device;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    if (device_get_connection_state(device) == CONNECTION_STATE_DISCONNECTED || device_get_connection_state(device) == CONNECTION_STATE_DISCONNECTING) {
        adapter_unlock();
        return BT_STATUS_BUSY;
    }

    if (bt_sal_le_disconnect(PRIMARY_ADAPTER, addr) != BT_STATUS_SUCCESS) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    device_set_connection_state(device, CONNECTION_STATE_DISCONNECTING);
    adapter_unlock();
    return BT_STATUS_SUCCESS;
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_connect_request_reply(bt_address_t* addr, bool accept)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }
    adapter_unlock();
    bt_status_t status;
    status = bt_sal_acl_connection_reply(PRIMARY_ADAPTER, addr, accept);
    if (status == BT_STATUS_SUCCESS && accept) {
        device_set_connection_state(device, CONNECTION_STATE_CONNECTING);
    }

    return status;
}

bt_status_t adapter_le_set_phy(bt_address_t* addr,
    ble_phy_type_t tx_phy,
    ble_phy_type_t rx_phy)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    adapter_unlock();

    return bt_sal_le_set_phy(PRIMARY_ADAPTER, addr, tx_phy, rx_phy);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_le_enable_key_derivation(bool brkey_to_lekey,
    bool lekey_to_brkey)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    return bt_sal_le_enable_key_derivation(PRIMARY_ADAPTER, brkey_to_lekey, lekey_to_brkey);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_le_add_whitelist(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;
    bt_device_t* device;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    adapter_lock();
    if (adapter->adapter_state != BT_ADAPTER_STATE_ON) {
        adapter_unlock();
        return BT_STATUS_NOT_ENABLED;
    }

    device = adapter_find_create_le_device(addr, BT_LE_ADDR_TYPE_PUBLIC);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_NOMEM;
    }

    if (device_check_flag(device, DFLAG_WHITELIST_ADDED)) {
        adapter_unlock();
        return BT_STATUS_SUCCESS;
    }

    adapter_unlock();
    return bt_sal_le_add_white_list(PRIMARY_ADAPTER, addr, device_get_address_type(device));

    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s, %s", __func__, addr_str);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_le_remove_whitelist(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_service_t* adapter = &g_adapter_service;
    bt_device_t* device;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    adapter_lock();
    if (adapter->adapter_state != BT_ADAPTER_STATE_ON) {
        adapter_unlock();
        return BT_STATUS_NOT_ENABLED;
    }

    device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    if (!device_check_flag(device, DFLAG_WHITELIST_ADDED)) {
        adapter_unlock();
        return BT_STATUS_SUCCESS;
    }

    adapter_unlock();

    bt_addr_ba2str(addr, addr_str);
    BT_LOGD("%s, %s", __func__, addr_str);

    return bt_sal_le_remove_white_list(PRIMARY_ADAPTER, addr, device_get_address_type(device));
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_create_bond(bt_address_t* addr, bt_transport_t transport)
{
    adapter_service_t* adapter = &g_adapter_service;
    bt_device_t* device;

    adapter_lock();
    if (adapter->adapter_state != BT_ADAPTER_STATE_ON) {
        adapter_unlock();
        return BT_STATUS_NOT_ENABLED;
    }

    if (adapter->is_discovering)
        bt_sal_stop_discovery(PRIMARY_ADAPTER);

#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (transport == BT_TRANSPORT_BREDR)
        device = adapter_find_create_classic_device(addr);
    else
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        if (transport == BT_TRANSPORT_BLE) {
        device = adapter_find_device(addr, BT_TRANSPORT_BLE);
        if (!device) {
            adapter_unlock();
            return BT_STATUS_DEVICE_NOT_FOUND;
        }
    } else
#endif
    {
        adapter_unlock();
        return BT_STATUS_PARM_INVALID;
    }

    if (device_get_bond_state(device) != BOND_STATE_NONE) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    adapter_unlock();
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (transport == BT_TRANSPORT_BREDR)
        return bt_sal_create_bond(PRIMARY_ADAPTER, addr, transport, device_get_address_type(device));
    else
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        if (transport == BT_TRANSPORT_BLE)
        return bt_sal_le_create_bond(PRIMARY_ADAPTER, addr, device_get_address_type(device));
    else
#endif
        return BT_STATUS_PARM_INVALID;
}

bt_status_t adapter_remove_bond(bt_address_t* addr, uint8_t transport)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, transport);
    if (!device || device_get_bond_state(device) != BOND_STATE_BONDED) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    device_set_bond_state(device, BOND_STATE_NONE);
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (transport == BT_TRANSPORT_BREDR) {
        device_delete_link_key(device);
        bt_sal_remove_bond(PRIMARY_ADAPTER, addr, transport);
        /* remove bond device form storage */
        adapter_update_bonded_device();
    } else
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        if (transport == BT_TRANSPORT_BLE) {
        bt_sal_le_remove_bond(PRIMARY_ADAPTER, addr);
    }
#endif

    adapter_unlock();
    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_cancel_bond(bt_address_t* addr)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (!device || device_get_bond_state(device) != BOND_STATE_BONDING) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    bt_status_t status = bt_sal_cancel_bond(PRIMARY_ADAPTER, addr, BT_TRANSPORT_BREDR);
    if (status == BT_STATUS_SUCCESS)
        device_set_bond_state(device, BOND_STATE_CANCELING);
    adapter_unlock();

    return status;
}

bt_status_t adapter_pair_request_reply(bt_address_t* addr, bool accept)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }
    adapter_unlock();
    bt_status_t status;
    status = bt_sal_pair_reply(PRIMARY_ADAPTER, addr, accept ? 0 : HCI_ERR_PAIRING_NOT_ALLOWED);
    if (status == BT_STATUS_SUCCESS && accept) {
        /* callback bonding */
        CALLBACK_FOREACH(CBLIST, adapter_callbacks_t, on_bond_state_changed,
            addr, BT_TRANSPORT_BREDR, BOND_STATE_BONDING, false);
    }

    return status;
}

bt_status_t adapter_set_pin_code(bt_address_t* addr, bool accept,
    char* pincode, int len)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (!device || device_get_bond_state(device) != BOND_STATE_BONDING) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    adapter_unlock();
    return bt_sal_pin_reply(PRIMARY_ADAPTER, addr, accept, pincode, len);
}

bt_status_t adapter_set_pairing_confirmation(bt_address_t* addr, uint8_t transport, bool accept)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, transport);
    if (!device || device_get_bond_state(device) != BOND_STATE_BONDING) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    adapter_unlock();
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (transport == BT_TRANSPORT_BREDR)
        return bt_sal_ssp_reply(PRIMARY_ADAPTER, addr, accept, PAIR_TYPE_PASSKEY_CONFIRMATION, 0);
    else
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        if (transport == BT_TRANSPORT_BLE)
        return bt_sal_le_smp_reply(PRIMARY_ADAPTER, addr, accept, PAIR_TYPE_PASSKEY_CONFIRMATION, 0);
    else
#endif
        return BT_STATUS_PARM_INVALID;
}

bt_status_t adapter_set_pass_key(bt_address_t* addr, uint8_t transport, bool accept, uint32_t passkey)
{
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, transport);
    if (!device || device_get_bond_state(device) != BOND_STATE_BONDING) {
        adapter_unlock();
        return BT_STATUS_FAIL;
    }

    adapter_unlock();
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    if (transport == BT_TRANSPORT_BREDR)
        return bt_sal_ssp_reply(PRIMARY_ADAPTER, addr, accept, PAIR_TYPE_PASSKEY_ENTRY, passkey);
    else
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
        if (transport == BT_TRANSPORT_BLE)
        return bt_sal_le_smp_reply(PRIMARY_ADAPTER, addr, accept, PAIR_TYPE_PASSKEY_ENTRY, passkey);
    else
#endif
        return BT_STATUS_PARM_INVALID;
}

bt_status_t adapter_le_set_legacy_tk(bt_address_t* addr, bt_128key_t tk_val)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    adapter_unlock();
    return bt_sal_le_set_legacy_tk(PRIMARY_ADAPTER, addr, tk_val);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_le_set_remote_oob_data(bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    adapter_unlock();
    return bt_sal_le_set_remote_oob_data(PRIMARY_ADAPTER, addr, c_val, r_val);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_le_get_local_oob_data(bt_address_t* addr)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    adapter_lock();
    bt_device_t* device = adapter_find_device(addr, BT_TRANSPORT_BLE);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    adapter_unlock();
    return bt_sal_le_get_local_oob_data(PRIMARY_ADAPTER, addr);
#else
    return BT_STATUS_NOT_SUPPORTED;
#endif
}

bt_status_t adapter_switch_role(bt_address_t* addr, bt_link_role_t role)
{
    bt_device_t* device;
    bt_link_role_t prev_role = BT_LINK_ROLE_UNKNOWN;

    if (role != BT_LINK_ROLE_MASTER && role != BT_LINK_ROLE_SLAVE)
        return BT_STATUS_PARM_INVALID;

    adapter_lock();
    device = adapter_find_device(addr, BT_TRANSPORT_BREDR);
    if (!device) {
        adapter_unlock();
        return BT_STATUS_DEVICE_NOT_FOUND;
    }

    prev_role = device_get_local_role(device);
    adapter_unlock();

    if (prev_role != role)
        return bt_sal_set_link_role(PRIMARY_ADAPTER, addr, role);

    return BT_STATUS_SUCCESS;
}

bt_status_t adapter_set_afh_channel_classification(uint16_t central_frequency,
    uint16_t band_width,
    uint16_t number)
{
    return bt_sal_set_afh_channel_classification(PRIMARY_ADAPTER, central_frequency, band_width, number);
}

void adapter_get_support_profiles(void) { }

void adapter_dump(void)
{
}

void adapter_dump_device(bt_address_t* addr)
{
}

void adapter_dump_profile(enum profile_id id)
{
}

void adapter_dump_all_device(void)
{
    bt_list_node_t* node;
    bt_list_t* list = g_adapter_service.devices;
    BT_LOGD("%s", __func__);
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        device_dump(device);
    }

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    list = g_adapter_service.le_devices;
    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        device_dump(device);
    }
#endif
}
