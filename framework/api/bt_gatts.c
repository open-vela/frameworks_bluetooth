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
#define LOG_TAG "gatts"

#include "bt_gatts.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "gatts_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static gatts_interface_t* get_profile_service(void)
{
    return (gatts_interface_t*)service_manager_get_profile(PROFILE_GATTS);
}

bt_status_t BTSYMBOLS(bt_gatts_register_service)(bt_instance_t* ins, gatts_handle_t* phandle, gatts_callbacks_t* callbacks)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->register_service(NULL, phandle, callbacks);
}

bt_status_t BTSYMBOLS(bt_gatts_unregister_service)(gatts_handle_t srv_handle)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->unregister_service(srv_handle);
}

bt_status_t BTSYMBOLS(bt_gatts_connect)(gatts_handle_t srv_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->connect(srv_handle, addr, addr_type);
}

bt_status_t BTSYMBOLS(bt_gatts_connect_bear)(gatts_handle_t srv_handle, bt_address_t* addr, ble_addr_type_t addr_type, uint8_t bear_type)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->connect_bear(srv_handle, addr, addr_type, bear_type);
}

bt_status_t BTSYMBOLS(bt_gatts_disconnect)(gatts_handle_t srv_handle, bt_address_t* addr)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->disconnect(srv_handle, addr);
}

bt_status_t BTSYMBOLS(bt_gatts_add_attr_table)(gatts_handle_t srv_handle, gatt_srv_db_t* srv_db)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->add_attr_table(srv_handle, srv_db);
}

bt_status_t BTSYMBOLS(bt_gatts_remove_attr_table)(gatts_handle_t srv_handle, uint16_t attr_handle)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->remove_attr_table(srv_handle, attr_handle);
}

bt_status_t BTSYMBOLS(bt_gatts_set_attr_value)(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->set_attr_value(srv_handle, attr_handle, value, length);
}

bt_status_t BTSYMBOLS(bt_gatts_get_attr_value)(gatts_handle_t srv_handle, uint16_t attr_handle, uint8_t* value, uint16_t* length)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->get_attr_value(srv_handle, attr_handle, value, length);
}

bt_status_t BTSYMBOLS(bt_gatts_response)(gatts_handle_t srv_handle, bt_address_t* addr, uint32_t req_handle, uint8_t* value, uint16_t length)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->response(srv_handle, addr, req_handle, value, length);
}

bt_status_t BTSYMBOLS(bt_gatts_notify)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->notify(srv_handle, addr, attr_handle, value, length);
}

bt_status_t BTSYMBOLS(bt_gatts_indicate)(gatts_handle_t srv_handle, bt_address_t* addr, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->indicate(srv_handle, addr, attr_handle, value, length);
}

bt_status_t BTSYMBOLS(bt_gatts_read_phy)(gatts_handle_t srv_handle, bt_address_t* addr)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->read_phy(srv_handle, addr);
}

bt_status_t BTSYMBOLS(bt_gatts_update_phy)(gatts_handle_t srv_handle, bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gatts_interface_t* profile = get_profile_service();

    return profile->update_phy(srv_handle, addr, tx_phy, rx_phy);
}
