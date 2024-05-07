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

#include "adapter_callbacks_stub.h"
#include "adapter_proxy.h"
#include "adapter_stub.h"
#include "bt_adapter.h"

void* bt_adapter_register_callback(bt_instance_t* ins, const adapter_callbacks_t* adapter_cbs)
{
    BpBtAdapter* bpAdapter = ins->adapter_proxy;

    if (!BtAdapter_getService(&bpAdapter, ADAPTER_BINDER_INSTANCE))
        return NULL;

    IBtAdapterCallbacks* cbks = BtAdapterCallbacks_new(adapter_cbs);
    AIBinder* binder = BtAdapterCallbacks_getBinder(cbks);
    if (!binder) {
        BtAdapterCallbacks_delete(cbks);
        return NULL;
    }

    void* rmt_cookie = BpBtAdapter_registerCallback(bpAdapter, binder);
    AIBinder_decStrong(binder);
    if (!rmt_cookie) {
        BtAdapterCallbacks_delete(cbks);
        return NULL;
    }

    cbks->cookie = rmt_cookie;
    return (void*)cbks;
}

bool bt_adapter_unregister_callback(bt_instance_t* ins, void* cookie)
{
    IBtAdapterCallbacks* cbks = cookie;

    bool ret = BpBtAdapter_unRegisterCallback((BpBtAdapter*)ins->adapter_proxy, cbks->cookie);
    if (ret)
        BtAdapterCallbacks_delete(cbks);

    return ret;
}

bt_status_t bt_adapter_enable(bt_instance_t* ins)
{
    return BpBtAdapter_enable((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_disable(bt_instance_t* ins)
{
    return BpBtAdapter_disable((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_enable_le(bt_instance_t* ins)
{
    return BpBtAdapter_enableLe((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_disable_le(bt_instance_t* ins)
{
    return BpBtAdapter_disableLe((BpBtAdapter*)ins->adapter_proxy);
}

bt_adapter_state_t bt_adapter_get_state(bt_instance_t* ins)
{
    return BpBtAdapter_getState((BpBtAdapter*)ins->adapter_proxy);
}

bool bt_adapter_is_le_enabled(bt_instance_t* ins)
{
    return BpBtAdapter_isLeEnabled((BpBtAdapter*)ins->adapter_proxy);
}

bt_device_type_t bt_adapter_get_type(bt_instance_t* ins)
{
    return BpBtAdapter_getType((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_set_discovery_filter(bt_instance_t* ins)
{
    return BpBtAdapter_setDiscoveryFilter((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_start_discovery(bt_instance_t* ins, uint32_t timeout)
{
    return BpBtAdapter_startDiscovery((BpBtAdapter*)ins->adapter_proxy, timeout);
}

bt_status_t bt_adapter_cancel_discovery(bt_instance_t* ins)
{
    return BpBtAdapter_cancelDiscovery((BpBtAdapter*)ins->adapter_proxy);
}

bool bt_adapter_is_discovering(bt_instance_t* ins)
{
    return BpBtAdapter_isDiscovering((BpBtAdapter*)ins->adapter_proxy);
}

void bt_adapter_get_address(bt_instance_t* ins, bt_address_t* addr)
{
    BpBtAdapter_getAddress((BpBtAdapter*)ins->adapter_proxy, addr);
}

bt_status_t bt_adapter_set_name(bt_instance_t* ins, const char* name)
{
    assert(strlen(name) <= 64);

    return BpBtAdapter_setName((BpBtAdapter*)ins->adapter_proxy, name);
}

void bt_adapter_get_name(bt_instance_t* ins, char* name, int length)
{
    BpBtAdapter_getName((BpBtAdapter*)ins->adapter_proxy, name, length);
}

bt_status_t bt_adapter_get_uuids(bt_instance_t* ins, bt_uuid_t* uuids, uint16_t* size)
{
    return BpBtAdapter_getUuids((BpBtAdapter*)ins->adapter_proxy, uuids, size);
}

bt_status_t bt_adapter_set_scan_mode(bt_instance_t* ins, bt_scan_mode_t mode, bool bondable)
{
    return BpBtAdapter_setScanMode((BpBtAdapter*)ins->adapter_proxy, mode, bondable);
}

bt_scan_mode_t bt_adapter_get_scan_mode(bt_instance_t* ins)
{
    return BpBtAdapter_getScanMode((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_set_device_class(bt_instance_t* ins, uint32_t cod)
{
    return BpBtAdapter_setDeviceClass((BpBtAdapter*)ins->adapter_proxy, cod);
}

uint32_t bt_adapter_get_device_class(bt_instance_t* ins)
{
    return BpBtAdapter_getDeviceClass((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_set_io_capability(bt_instance_t* ins, bt_io_capability_t cap)
{
    return BpBtAdapter_setIOCapability((BpBtAdapter*)ins->adapter_proxy, cap);
}

bt_io_capability_t bt_adapter_get_io_capability(bt_instance_t* ins)
{
    return BpBtAdapter_getIOCapability((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_set_le_io_capability(bt_instance_t* ins, uint32_t le_io_cap)
{
    return BpBtAdapter_SetLeIOCapability((BpBtAdapter*)ins->adapter_proxy, le_io_cap);
}

uint32_t bt_adapter_get_le_io_capability(bt_instance_t* ins)
{
    return BpBtAdapter_getLeIOCapability((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_get_le_address(bt_instance_t* ins, bt_address_t* addr, ble_addr_type_t* type)
{
    return BpBtAdapter_getLeAddress((BpBtAdapter*)ins->adapter_proxy, addr, type);
}

bt_status_t bt_adapter_set_le_address(bt_instance_t* ins, bt_address_t* addr)
{
    return BpBtAdapter_setLeAddress((BpBtAdapter*)ins->adapter_proxy, addr);
}

bt_status_t bt_adapter_set_le_identity_address(bt_instance_t* ins, bt_address_t* addr, bool public)
{
    return BpBtAdapter_setLeIdentityAddress((BpBtAdapter*)ins->adapter_proxy, addr, public);
}

bt_status_t bt_adapter_set_le_appearance(bt_instance_t* ins, uint16_t appearance)
{
    return BpBtAdapter_setLeAppearance((BpBtAdapter*)ins->adapter_proxy, appearance);
}

uint16_t bt_adapter_get_le_appearance(bt_instance_t* ins)
{
    return BpBtAdapter_getLeAppearance((BpBtAdapter*)ins->adapter_proxy);
}

bt_status_t bt_adapter_get_bonded_devices(bt_instance_t* ins, bt_transport_t transport, bt_address_t** addr, int* num, bt_allocator_t allocator)
{
    return BpBtAdapter_getBondedDevices((BpBtAdapter*)ins->adapter_proxy, addr, num, allocator);
}

bt_status_t bt_adapter_get_connected_devices(bt_instance_t* ins, bt_transport_t transport, bt_address_t** addr, int* num, bt_allocator_t allocator)
{
    return BpBtAdapter_getConnectedDevices((BpBtAdapter*)ins->adapter_proxy, addr, num, allocator);
}

void bt_adapter_disconnect_all_devices(bt_instance_t* ins)
{
}

bool bt_adapter_is_support_bredr(bt_instance_t* ins)
{
    return false;
}

bool bt_adapter_is_support_le(bt_instance_t* ins)
{
    return false;
}

bool bt_adapter_is_support_leaudio(bt_instance_t* ins)
{
    return false;
}
