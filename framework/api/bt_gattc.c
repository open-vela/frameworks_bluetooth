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
#define LOG_TAG "gattc"

#include "bt_gattc.h"
#include "bt_internal.h"
#include "bt_profile.h"
#include "gattc_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static gattc_interface_t* get_profile_service(void)
{
    return (gattc_interface_t*)service_manager_get_profile(PROFILE_GATTC);
}

bt_status_t BTSYMBOLS(bt_gattc_create_connect)(bt_instance_t* ins, gattc_handle_t* phandle, gattc_callbacks_t* callbacks)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->create_connect(NULL, phandle, callbacks);
}

bt_status_t BTSYMBOLS(bt_gattc_delete_connect)(gattc_handle_t conn_handle)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->delete_connect(conn_handle);
}

bt_status_t BTSYMBOLS(bt_gattc_connect)(gattc_handle_t conn_handle, bt_address_t* addr, ble_addr_type_t addr_type)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->connect(conn_handle, addr, addr_type);
}

bt_status_t BTSYMBOLS(bt_gattc_disconnect)(gattc_handle_t conn_handle)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->disconnect(conn_handle);
}

bt_status_t BTSYMBOLS(bt_gattc_discover_service)(gattc_handle_t conn_handle, bt_uuid_t* filter_uuid)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->discover_service(conn_handle, filter_uuid);
}

bt_status_t BTSYMBOLS(bt_gattc_get_attribute_by_handle)(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t* attr_desc)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->get_attribute_by_handle(conn_handle, attr_handle, attr_desc);
}

bt_status_t BTSYMBOLS(bt_gattc_get_attribute_by_uuid)(gattc_handle_t conn_handle, uint16_t start_handle, uint16_t end_handle, bt_uuid_t* attr_uuid, gatt_attr_desc_t* attr_desc)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->get_attribute_by_uuid(conn_handle, start_handle, end_handle, attr_uuid, attr_desc);
}

bt_status_t BTSYMBOLS(bt_gattc_read)(gattc_handle_t conn_handle, uint16_t attr_handle)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->read(conn_handle, attr_handle);
}

bt_status_t BTSYMBOLS(bt_gattc_write)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->write(conn_handle, attr_handle, value, length);
}

bt_status_t BTSYMBOLS(bt_gattc_write_without_response)(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t* value, uint16_t length)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->write_without_response(conn_handle, attr_handle, value, length);
}

bt_status_t BTSYMBOLS(bt_gattc_subscribe)(gattc_handle_t conn_handle, uint16_t attr_handle, uint16_t ccc_value)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->subscribe(conn_handle, attr_handle, ccc_value);
}

bt_status_t BTSYMBOLS(bt_gattc_unsubscribe)(gattc_handle_t conn_handle, uint16_t attr_handle)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->unsubscribe(conn_handle, attr_handle);
}

bt_status_t BTSYMBOLS(bt_gattc_exchange_mtu)(gattc_handle_t conn_handle, uint32_t mtu)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->exchange_mtu(conn_handle, mtu);
}

bt_status_t BTSYMBOLS(bt_gattc_update_connection_parameter)(gattc_handle_t conn_handle, uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->update_connection_parameter(conn_handle, min_interval, max_interval, latency,
        timeout, min_connection_event_length, max_connection_event_length);
}

bt_status_t BTSYMBOLS(bt_gattc_read_phy)(gattc_handle_t conn_handle)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->read_phy(conn_handle);
}

bt_status_t BTSYMBOLS(bt_gattc_update_phy)(gattc_handle_t conn_handle, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->update_phy(conn_handle, tx_phy, rx_phy);
}

bt_status_t BTSYMBOLS(bt_gattc_read_rssi)(gattc_handle_t conn_handle)
{
    gattc_interface_t* profile = get_profile_service();

    return profile->read_rssi(conn_handle);
}
