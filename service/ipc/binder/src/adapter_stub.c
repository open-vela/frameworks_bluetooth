/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#include <uchar.h>

#include "adapter_internel.h"
#include "advertising.h"
#include "bluetooth.h"
#include "scan_manager.h"

#include "adapter_callbacks_proxy.h"
#include "adapter_callbacks_stub.h"
#include "adapter_proxy.h"
#include "adapter_stub.h"
#include "advertiser_callbacks_proxy.h"
#include "advertiser_callbacks_stub.h"
#include "binder_utils.h"
#include "parcel.h"
#include "scanner_callbacks_proxy.h"
#include "scanner_callbacks_stub.h"

#include "utils/log.h"

#define BT_ADAPTER_DESC "BluetoothAdapter"

static void* IBtAdapter_Class_onCreate(void* arg)
{
    return arg;
}

static void IBtAdapter_Class_onDestroy(void* userData)
{
}

static binder_status_t IBtAdapter_Class_onTransact(AIBinder* binder, transaction_code_t code, const AParcel* in, AParcel* out)
{
    binder_status_t stat = STATUS_FAILED_TRANSACTION;
    bt_status_t status;
    bt_address_t addr;

    switch (code) {
    case IBTADAPTER_REGISTER_CALLBACK: {
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtAdapterCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        void* cookie = adapter_register_callback(remote, BpBtAdapterCallbacks_getStatic());
        stat = AParcel_writeUint32(out, (uint32_t)cookie);
        break;
    }
    case IBTADAPTER_UNREGISTER_CALLBACK: {
        AIBinder* remote = NULL;
        uint32_t cookie;

        stat = AParcel_readUint32(in, &cookie);
        if (stat != STATUS_OK)
            return stat;

        bool ret = adapter_unregister_callback((void**)&remote, (void*)cookie);
        if (ret && remote)
            AIBinder_decStrong(remote);

        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IBTADAPTER_ENABLE: {
        status = adapter_enable(SYS_SET_BT_ALL);
        stat = AParcel_writeInt32(out, status);
        break;
    }
    case IBTADAPTER_DISABLE: {
        status = adapter_disable(SYS_SET_BT_ALL);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_ENABLE_LE: {
        status = adapter_enable(APP_SET_LE_ONLY);
        stat = AParcel_writeInt32(out, status);
        break;
    }
    case IBTADAPTER_DISABLE_LE: {
        status = adapter_disable(APP_SET_LE_ONLY);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_STATE: {
        bt_adapter_state_t state = adapter_get_state();
        stat = AParcel_writeUint32(out, state);
        break;
    }
    case IBTADAPTER_IS_LE_ENABLED: {
        bool ret = adapter_is_le_enabled();
        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IBTADAPTER_GET_TYPE: {
        bt_device_type_t type = adapter_get_type();
        stat = AParcel_writeUint32(out, type);
        break;
    }
    case IBTADAPTER_SET_DISCOVERY_FILTER: {
        status = adapter_set_discovery_filter();
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_START_DISCOVERY: {
        uint32_t timeout;

        stat = AParcel_readUint32(in, &timeout);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_start_discovery(timeout);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_CANCEL_DISCOVERY: {
        status = adapter_cancel_discovery();
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_IS_DISCOVERING: {
        bool ret = adapter_is_discovering();
        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IBTADAPTER_GET_ADDR: {
        adapter_get_address(&addr);
        stat = AParcel_writeAddress(out, &addr);
        break;
    }
    case IBTADAPTER_SET_NAME: {
        char* name;

        stat = AParcel_readString(in, &name, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_name(name);
        free(name);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_NAME: {
        char name[BT_LOC_NAME_MAX_LEN + 1];

        adapter_get_name(name, BT_LOC_NAME_MAX_LEN + 1);
        stat = AParcel_writeString(out, name, strlen(name));
        break;
    }
    case IBTADAPTER_GET_UUIDS: {
        break;
    }
    case IBTADAPTER_SET_SCAN_MODE: {
        bt_scan_mode_t mode;
        bool bondable;

        stat = AParcel_readUint32(in, &mode);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &bondable);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_scan_mode(mode, bondable);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_SCAN_MODE: {
        bt_scan_mode_t mode = adapter_get_scan_mode();
        stat = AParcel_writeUint32(out, mode);
        break;
    }
    case IBTADAPTER_SET_DEVICE_CLASS: {
        uint32_t cod;

        stat = AParcel_readUint32(in, &cod);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_device_class(cod);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_DEVICE_CLASS: {
        uint32_t cod = adapter_get_device_class();
        stat = AParcel_writeUint32(out, cod);
        break;
    }
    case IBTADAPTER_SET_IO_CAP: {
        bt_io_capability_t io;

        stat = AParcel_readUint32(in, &io);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_io_capability(io);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_IO_CAP: {
        bt_io_capability_t io = adapter_get_io_capability();
        stat = AParcel_writeUint32(out, io);
        break;
    }
    case IBTADAPTER_GET_LE_IO_CAP: {
        bt_io_capability_t io = adapter_get_le_io_capability();
        stat = AParcel_writeUint32(out, io);
        break;
    }
    case IBTADAPTER_SET_LE_IO_CAP: {
        bt_io_capability_t io;

        stat = AParcel_readUint32(in, &io);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_le_io_capability(io);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_LE_ADDR: {
        ble_addr_type_t type;

        status = adapter_get_le_address(&addr, &type);

        stat = AParcel_writeAddress(out, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(out, type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_SET_LE_ADDR: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_le_address(&addr);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_SET_LE_ID: {
        bool isPublic;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &isPublic);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_le_identity_address(&addr, isPublic);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_SET_LE_APPEARANCE: {
        uint32_t appearance;

        stat = AParcel_readUint32(in, &appearance);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_le_appearance((uint16_t)appearance);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_LE_APPEARANCE: {
        uint16_t appearance = adapter_get_le_appearance();
        stat = AParcel_writeUint32(out, appearance);
    }
    case IBTADAPTER_GET_BONDED_DEVICES: {
        bt_address_t* addrs = NULL;
        int size = 0;

        status = adapter_get_bonded_devices(&addrs, &size, AParcelUtils_btCommonAllocator, BT_TRANSPORT_BREDR);
        if (addrs && size) {
            stat = AParcel_writeAddressArray(out, addrs, size);
            free(addrs);
            if (stat != STATUS_OK)
                return stat;
        }
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_GET_CONNECTED_DEVICES: {
        bt_address_t* addrs = NULL;
        int size = 0;

        status = adapter_get_connected_devices(&addrs, &size, AParcelUtils_btCommonAllocator, BT_TRANSPORT_BREDR);
        if (addrs && size) {
            stat = AParcel_writeAddressArray(out, addrs, size);
            free(addrs);
            if (stat != STATUS_OK)
                return stat;
        }
        stat = AParcel_writeUint32(out, status);
        break;
        break;
    }
    case IBTADAPTER_ENABLE_KEY_DERIVATION: {
        bool brkey_to_lekey, lekey_to_brkey;

        stat = AParcel_readBool(in, &brkey_to_lekey);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &lekey_to_brkey);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_le_enable_key_derivation(brkey_to_lekey, lekey_to_brkey);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IBTADAPTER_START_ADVERTISING: {
#ifdef CONFIG_BLUETOOTH_BLE_ADV
        AIBinder* remote;
        ble_adv_params_t param;
        uint8_t *adv, *scan_rsp;
        uint32_t adv_len, scan_rsp_len;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtAdvertiserCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        stat = AParcel_readBleAdvParam(in, &param);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&adv, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &adv_len);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readByteArray(in, (void*)&scan_rsp, AParcelUtils_byteArrayAllocator);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &scan_rsp_len);
        if (stat != STATUS_OK)
            return stat;

        void* cookie = start_advertising(remote, &param, adv, adv_len, scan_rsp, scan_rsp_len, BpBtAdvertiserCallbacks_getStatic());
        free(adv);
        free(scan_rsp);
        stat = AParcel_writeUint32(out, (uint32_t)cookie);
#endif
        break;
    }
    case IBTADAPTER_STOP_ADVERTISING: {
#ifdef CONFIG_BLUETOOTH_BLE_ADV
        uint32_t adver;

        stat = AParcel_readUint32(in, &adver);
        if (stat != STATUS_OK)
            return stat;

        stop_advertising((bt_advertiser_t*)adver);
#endif
        break;
    }
    case IBTADAPTER_STOP_ADVERTISING_ID: {
#ifdef CONFIG_BLUETOOTH_BLE_ADV
        uint32_t adv_id;

        stat = AParcel_readUint32(in, &adv_id);
        if (stat != STATUS_OK)
            return stat;

        stop_advertising_id((uint8_t)adv_id);
#endif
        break;
    }
    case IBTADAPTER_START_SCAN: {
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
        AIBinder* remote;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtScannerCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        void* cookie = scanner_start_scan(remote, BpBtScannerCallbacks_getStatic());
        stat = AParcel_writeUint32(out, (uint32_t)cookie);
#endif
        break;
    }
    case IBTADAPTER_START_SCAN_SETTINGS: {
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
        AIBinder* remote;
        ble_scan_settings_t settings;

        stat = AParcel_readStrongBinder(in, &remote);
        if (stat != STATUS_OK)
            return stat;

        if (!BtScannerCallbacks_associateClass(remote)) {
            AIBinder_decStrong(remote);
            return STATUS_FAILED_TRANSACTION;
        }

        stat = AParcel_readInt32(in, &settings.scan_mode);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &settings.legacy);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &settings.scan_phy);
        if (stat != STATUS_OK)
            return stat;

        void* cookie = scanner_start_scan_settings(remote, &settings, BpBtScannerCallbacks_getStatic());
        stat = AParcel_writeUint32(out, (uint32_t)cookie);
#endif
        break;
    }
    case IBTADAPTER_STOP_SCAN: {
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
        uint32_t scanner;

        stat = AParcel_readUint32(in, &scanner);
        if (stat != STATUS_OK)
            return stat;

        scanner_stop_scan((bt_scanner_t*)scanner);
#endif
        break;
    }
    case IREMOTE_GET_DEVICE_TYPE: {
        bt_device_type_t device_type;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;
        device_type = adapter_get_remote_device_type(&addr);
        stat = AParcel_writeUint32(out, device_type);

        break;
    }
    case IREMOTE_GET_NAME: {
        char name[BT_REM_NAME_MAX_LEN + 1];

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        bool ret = adapter_get_remote_name(&addr, name);
        if (!ret)
            return STATUS_FAILED_TRANSACTION;
        stat = AParcel_writeString(out, name, strlen(name));
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IREMOTE_GET_DEVICE_CLASS: {
        uint32_t cod;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        cod = adapter_get_remote_device_class(&addr);
        stat = AParcel_writeUint32(out, cod);
        break;
    }
    case IREMOTE_GET_UUIDS: {
        bt_uuid_t* uuids = NULL;
        uint16_t uuidSize = 0;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_get_remote_uuids(&addr, &uuids, &uuidSize, AParcelUtils_btCommonAllocator);
        stat = AParcel_writeUuidArray(out, uuids, (int32_t)uuidSize);
        if (stat != STATUS_OK)
            return stat;

        if (uuids && uuidSize)
            free(uuids);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_GET_APPEARANCE: {
        uint32_t appearance;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        appearance = adapter_get_remote_appearance(&addr);
        stat = AParcel_writeUint32(out, appearance);
        break;
    }
    case IREMOTE_GET_RSSI: {
        int8_t rssi;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        rssi = adapter_get_remote_rssi(&addr);
        stat = AParcel_writeByte(out, rssi);
        break;
    }
    case IREMOTE_GET_ALIAS: {
        char name[BT_REM_NAME_MAX_LEN + 1];

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        bool ret = adapter_get_remote_alias(&addr, name);
        if (!ret)
            return STATUS_FAILED_TRANSACTION;
        stat = AParcel_writeString(out, name, strlen(name));
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IREMOTE_SET_ALIAS: {
        char* alias = NULL;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &alias, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_remote_alias(&addr, alias);
        free(alias);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_IS_CONNECTED: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        bool ret = adapter_is_remote_connected(&addr);
        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IREMOTE_IS_ENCRYPTED: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        bool ret = adapter_is_remote_encrypted(&addr);
        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IREMOTE_IS_BOND_INIT_LOCAL: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        bool ret = adapter_is_bond_initiate_local(&addr);
        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IREMOTE_GET_BOND_STATE: {
        bond_state_t bondState;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        bondState = adapter_get_remote_bond_state(&addr);
        stat = AParcel_writeUint32(out, bondState);
        break;
    }
    case IREMOTE_IS_BONDED: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        bool ret = adapter_is_remote_bonded(&addr);
        stat = AParcel_writeBool(out, ret);
        break;
    }
    case IREMOTE_CREATE_BOND: {
        bt_transport_t transport;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &transport);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_create_bond(&addr, transport);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_REMOVE_BOND: {
        bt_transport_t transport;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &transport);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_remove_bond(&addr, transport);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_CANCEL_BOND: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_cancel_bond(&addr);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_PAIR_REQUEST_REPLY: {
        bool accept;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &accept);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_pair_request_reply(&addr, accept);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_SET_PAIRING_CONFIRM: {
        bt_transport_t transport;
        bool accept;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &transport);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &accept);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_pairing_confirmation(&addr, transport, accept);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_SET_PIN_CODE: {
        bool accept;
        char* pincode = NULL;
        uint32_t len;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &accept);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readString(in, &pincode, AParcelUtils_stringAllocator);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &len);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_pin_code(&addr, accept, pincode, len);
        free(pincode);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_SET_PASSKEY: {
        bt_transport_t transport;
        bool accept;
        uint32_t passKey;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &transport);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBool(in, &accept);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &passKey);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_set_pass_key(&addr, transport, accept, passKey);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_CONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_connect(&addr);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_DISCONNECT: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_disconnect(&addr);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_CONNECT_LE: {
        ble_addr_type_t type;
        ble_connect_params_t param;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &type);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readBleConnectParam(in, &param);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_le_connect(&addr, type, &param);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_DISCONNECT_LE: {
        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_le_disconnect(&addr);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    case IREMOTE_SET_LE_PHY: {
        ble_phy_type_t tx_phy;
        ble_phy_type_t rx_phy;

        stat = AParcel_readAddress(in, &addr);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &tx_phy);
        if (stat != STATUS_OK)
            return stat;

        stat = AParcel_readUint32(in, &rx_phy);
        if (stat != STATUS_OK)
            return stat;

        status = adapter_le_set_phy(&addr, tx_phy, rx_phy);
        stat = AParcel_writeUint32(out, status);
        break;
    }
    default:
        break;
    }

    return stat;
}

static AIBinder* BtAdapter_getBinder(IBtAdapter* adapter)
{
    AIBinder* binder = NULL;

    if (adapter->WeakBinder != NULL) {
        binder = AIBinder_Weak_promote(adapter->WeakBinder);
    }

    if (binder == NULL) {
        binder = AIBinder_new(adapter->clazz, (void*)adapter);
        if (adapter->WeakBinder != NULL) {
            AIBinder_Weak_delete(adapter->WeakBinder);
        }

        adapter->WeakBinder = AIBinder_Weak_new(binder);
    }

    return binder;
}

AIBinder_Class* BtAdapter_Class_define(void)
{
    return AIBinder_Class_define(BT_ADAPTER_DESC, IBtAdapter_Class_onCreate,
        IBtAdapter_Class_onDestroy, IBtAdapter_Class_onTransact);
}

binder_status_t BtAdapter_addService(IBtAdapter* adapter, const char* instance)
{
    adapter->clazz = BtAdapter_Class_define();
    AIBinder* binder = BtAdapter_getBinder(adapter);
    adapter->usr_data = NULL;

    binder_status_t status = AServiceManager_addService(binder, instance);
    AIBinder_decStrong(binder);

    return status;
}

BpBtAdapter* BpBtAdapter_new(const char* instance)
{
    AIBinder* binder = NULL;
    AIBinder_Class* clazz;
    BpBtAdapter* bpBinder = NULL;

    clazz = BtAdapter_Class_define();
    binder = AServiceManager_getService(instance);
    if (!binder)
        return NULL;

    if (!AIBinder_associateClass(binder, clazz))
        goto bail;

    if (!AIBinder_isRemote(binder))
        goto bail;

    /* linktoDeath ? */

    bpBinder = malloc(sizeof(*bpBinder));
    if (!bpBinder)
        goto bail;

    bpBinder->binder = binder;
    bpBinder->clazz = clazz;

    return bpBinder;

bail:
    AIBinder_decStrong(binder);
    return NULL;
}

void BpBtAdapter_delete(BpBtAdapter* bpAdapter)
{
    AIBinder_decStrong(bpAdapter->binder);
    free(bpAdapter);
}

AIBinder* BtAdapter_getService(BpBtAdapter** bpAdapter, const char* instance)
{
    BpBtAdapter* bpBinder = *bpAdapter;

    if (bpBinder && bpBinder->binder)
        return bpBinder->binder;

    bpBinder = BpBtAdapter_new(instance);
    if (!bpBinder)
        return NULL;

    *bpAdapter = bpBinder;

    return bpBinder->binder;
}
