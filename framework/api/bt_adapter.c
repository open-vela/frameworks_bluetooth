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
#include "bt_adapter.h"
<<<<<<< HEAD
#include "bt_internal.h"

void *BTSYMBOLS(bt_adapter_register_callback)(bt_instance_t *ins, const adapter_callbacks_t *adapter_cbs)
=======

void *bt_adapter_register_callback(bt_instance_t *ins, const adapter_callbacks_t *adapter_cbs)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_register_callback(NULL, adapter_cbs);
}

<<<<<<< HEAD
bool BTSYMBOLS(bt_adapter_unregister_callback)(bt_instance_t *ins, void *cookie)
=======
bool bt_adapter_unregister_callback(bt_instance_t *ins, void *cookie)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_unregister_callback(NULL, cookie);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_enable)(bt_instance_t *ins)
=======
bt_status_t bt_adapter_enable(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_enable(SYS_SET_BT_ALL);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_disable)(bt_instance_t *ins)
=======
bt_status_t bt_adapter_disable(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_disable(SYS_SET_BT_ALL);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_enable_le)(bt_instance_t *ins)
=======
bt_status_t bt_adapter_enable_le(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_enable(APP_SET_LE_ONLY);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_disable_le)(bt_instance_t *ins)
=======
bt_status_t bt_adapter_disable_le(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_disable(APP_SET_LE_ONLY);
}

<<<<<<< HEAD
bt_adapter_state_t BTSYMBOLS(bt_adapter_get_state)(bt_instance_t *ins)
=======
bt_adapter_state_t bt_adapter_get_state(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_state();
}

<<<<<<< HEAD
bool BTSYMBOLS(bt_adapter_is_le_enabled)(bt_instance_t *ins)
=======
bool bt_adapter_is_le_enabled(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_is_le_enabled();
}

<<<<<<< HEAD
bt_device_type_t BTSYMBOLS(bt_adapter_get_type)(bt_instance_t *ins)
=======
bt_device_type_t bt_adapter_get_type(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_type();
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_discovery_filter)(bt_instance_t *ins)
=======
bt_status_t bt_adapter_set_discovery_filter(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return 0;
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_start_discovery)(bt_instance_t *ins, uint32_t timeout)
=======
bt_status_t bt_adapter_start_discovery(bt_instance_t *ins, uint32_t timeout)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_start_discovery(timeout);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_cancel_discovery)(bt_instance_t *ins)
=======
bt_status_t bt_adapter_cancel_discovery(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_cancel_discovery();
}

<<<<<<< HEAD
bool BTSYMBOLS(bt_adapter_is_discovering)(bt_instance_t *ins)
=======
bool bt_adapter_is_discovering(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_is_discovering();
}

<<<<<<< HEAD
void BTSYMBOLS(bt_adapter_get_address)(bt_instance_t *ins, bt_address_t *addr)
=======
void bt_adapter_get_address(bt_instance_t *ins, bt_address_t *addr)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_address(addr);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_name)(bt_instance_t *ins, const char *name)
=======
bt_status_t bt_adapter_set_name(bt_instance_t *ins, const char *name)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_name(name);
}

<<<<<<< HEAD
void BTSYMBOLS(bt_adapter_get_name)(bt_instance_t *ins, char *name, int length)
=======
void bt_adapter_get_name(bt_instance_t *ins, char *name, int length)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_name(name);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_get_uuids)(bt_instance_t *ins, bt_uuid_t *uuids, uint16_t *size)
=======
bt_status_t bt_adapter_get_uuids(bt_instance_t *ins, bt_uuid_t *uuids, uint16_t *size)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_uuids(uuids, size);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_scan_mode)(bt_instance_t *ins, bt_scan_mode_t mode, bool bondable)
=======
bt_status_t bt_adapter_set_scan_mode(bt_instance_t *ins, bt_scan_mode_t mode, bool bondable)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_scan_mode(mode, bondable);
}

<<<<<<< HEAD
bt_scan_mode_t BTSYMBOLS(bt_adapter_get_scan_mode)(bt_instance_t *ins)
=======
bt_scan_mode_t bt_adapter_get_scan_mode(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_scan_mode();
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_device_class)(bt_instance_t *ins, uint32_t cod)
=======
bt_status_t bt_adapter_set_device_class(bt_instance_t *ins, uint32_t cod)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_device_class(cod);
}

<<<<<<< HEAD
uint32_t BTSYMBOLS(bt_adapter_get_device_class)(bt_instance_t *ins)
=======
uint32_t bt_adapter_get_device_class(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_device_class();
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_io_capability)(bt_instance_t *ins, bt_io_capability_t cap)
=======
bt_status_t bt_adapter_set_io_capability(bt_instance_t *ins, bt_io_capability_t cap)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_io_capability(cap);
}

<<<<<<< HEAD
bt_io_capability_t BTSYMBOLS(bt_adapter_get_io_capability)(bt_instance_t *ins)
=======
bt_io_capability_t bt_adapter_get_io_capability(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_io_capability();
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_inquiry_scan_parameters)(bt_instance_t *ins, bt_scan_type_t type,
                                                              uint16_t interval, uint16_t window)
{
    return adapter_set_inquiry_scan_parameters(type, interval, window);
}

bt_status_t BTSYMBOLS(bt_adapter_set_page_scan_parameters)(bt_instance_t *ins, bt_scan_type_t type,
                                                           uint16_t interval, uint16_t window)
{
    return adapter_set_page_scan_parameters(type, interval, window);
}

bt_status_t BTSYMBOLS(bt_adapter_set_le_io_capability)(bt_instance_t *ins, uint32_t le_io_cap)
=======
bt_status_t bt_adapter_set_le_io_capability(bt_instance_t *ins, uint32_t le_io_cap)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_le_io_capability(le_io_cap);
}

<<<<<<< HEAD
uint32_t BTSYMBOLS(bt_adapter_get_le_io_capability)(bt_instance_t *ins)
=======
uint32_t bt_adapter_get_le_io_capability(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_le_io_capability();
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_get_le_address)(bt_instance_t *ins, bt_address_t *addr, ble_addr_type_t *type)
=======
bt_status_t bt_adapter_get_le_address(bt_instance_t *ins, bt_address_t *addr, ble_addr_type_t *type)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_le_address(addr, type);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_le_address)(bt_instance_t *ins, bt_address_t *addr)
=======
bt_status_t bt_adapter_set_le_address(bt_instance_t *ins, bt_address_t *addr)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_le_address(addr);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_le_identity_address)(bt_instance_t *ins, bt_address_t *addr, bool public)
=======
bt_status_t bt_adapter_set_le_identity_address(bt_instance_t *ins, bt_address_t *addr, bool public)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_le_identity_address(addr, public);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_set_le_appearance)(bt_instance_t *ins, uint16_t appearance)
=======
bt_status_t bt_adapter_set_le_appearance(bt_instance_t *ins, uint16_t appearance)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_set_le_appearance(appearance);
}

<<<<<<< HEAD
uint16_t BTSYMBOLS(bt_adapter_get_le_appearance)(bt_instance_t *ins)
=======
uint16_t bt_adapter_get_le_appearance(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_get_le_appearance();
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_le_enable_key_derivation)(bt_instance_t *ins,
                                                           bool brkey_to_lekey,
                                                           bool lekey_to_brkey)
=======
bt_status_t bt_adapter_le_enable_key_derivation(bt_instance_t *ins,
                                                bool brkey_to_lekey,
                                                bool lekey_to_brkey)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_le_enable_key_derivation(brkey_to_lekey, lekey_to_brkey);
}

<<<<<<< HEAD
bt_status_t BTSYMBOLS(bt_adapter_le_add_whitelist)(bt_instance_t *ins, bt_address_t *addr)
{
    return adapter_le_add_whitelist(addr);
}

bt_status_t BTSYMBOLS(bt_adapter_le_remove_whitelist)(bt_instance_t *ins, bt_address_t *addr)
{
    return adapter_le_remove_whitelist(addr);
}

bt_status_t BTSYMBOLS(bt_adapter_get_bonded_devices)(bt_instance_t *ins, bt_transport_t transport, bt_address_t **addr, int *num, bt_allocator_t allocator)
{
    return adapter_get_bonded_devices(transport, addr, num, allocator);
}

bt_status_t BTSYMBOLS(bt_adapter_get_connected_devices)(bt_instance_t *ins, bt_transport_t transport, bt_address_t **addr, int *num, bt_allocator_t allocator)
{
    return adapter_get_connected_devices(transport, addr, num, allocator);
}

bt_status_t BTSYMBOLS(bt_adapter_set_afh_channel_classification)(bt_instance_t *ins, uint16_t central_frequency,
                                                                 uint16_t band_width, uint16_t number)
{
    return adapter_set_afh_channel_classification(central_frequency, band_width, number);
}

bt_status_t BTSYMBOLS(bt_adapter_set_auto_sniff)(bt_instance_t *ins, bt_auto_sniff_params_t *params)
{
    return adapter_set_auto_sniff(params);
}

void BTSYMBOLS(bt_adapter_disconnect_all_devices)(bt_instance_t *ins)
{
}

bool BTSYMBOLS(bt_adapter_is_support_bredr)(bt_instance_t *ins)
=======
bt_status_t bt_adapter_get_bonded_devices(bt_instance_t *ins, bt_address_t **addr, int *num, bt_allocator_t allocator)
{
    return adapter_get_bonded_devices(addr, num, allocator);
}

bt_status_t bt_adapter_get_connected_devices(bt_instance_t *ins, bt_address_t **addr, int *num, bt_allocator_t allocator)
{
    return adapter_get_connected_devices(addr, num, allocator);
}

void bt_adapter_disconnect_all_devices(bt_instance_t *ins)
{
}

bool bt_adapter_is_support_bredr(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_is_support_bredr();
}

<<<<<<< HEAD
bool BTSYMBOLS(bt_adapter_is_support_le)(bt_instance_t *ins)
=======
bool bt_adapter_is_support_le(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_is_support_le();
}

<<<<<<< HEAD
bool BTSYMBOLS(bt_adapter_is_support_leaudio)(bt_instance_t *ins)
=======
bool bt_adapter_is_support_leaudio(bt_instance_t *ins)
>>>>>>> bluetooth framework re-implement base
{
    return adapter_is_support_leaudio();
}
