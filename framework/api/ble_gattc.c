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

#include "ble_gattc.h"
#include "bt_profile.h"
#include "gattc_service.h"
#include "service_manager.h"
#include "utils/log.h"
#include <stdint.h>

static gattc_interface_t *get_profile_service(void)
{
    return (gattc_interface_t *)service_manager_get_profile(PROFILE_GATTC);
}

bt_status_t ble_gattc_create_connect(bt_instance_t *ins, gattc_handle_t *phandle, gattc_callbacks_t *callbacks)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->create_connect(phandle, callbacks);
}

bt_status_t ble_gattc_delete_connect(gattc_handle_t conn_handle)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->delete_connect(conn_handle);
}

bt_status_t ble_gattc_connect(gattc_handle_t conn_handle, bt_address_t *addr, ble_addr_type_t addr_type)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->connect(conn_handle, addr, addr_type);
}

bt_status_t ble_gattc_disconnect(gattc_handle_t conn_handle)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->disconnect(conn_handle);
}

bt_status_t ble_gattc_discover_service(gattc_handle_t conn_handle, bt_uuid_t *filter_uuid)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->discover_service(conn_handle, filter_uuid);
}

bt_status_t ble_gattc_get_attribute_by_handle(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t *attr_desc)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->get_attribute_by_handle(conn_handle, attr_handle, attr_desc);
}

bt_status_t ble_gattc_get_attribute_by_uuid(gattc_handle_t conn_handle, bt_uuid_t *attr_uuid, gatt_attr_desc_t *attr_desc)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->get_attribute_by_uuid(conn_handle, attr_uuid, attr_desc);
}

bt_status_t ble_gattc_read(gattc_handle_t conn_handle, uint16_t attr_handle, gattc_read_cb_t read_cb)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->read(conn_handle, attr_handle, read_cb);
}

bt_status_t ble_gattc_write(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t *value, uint16_t length, uint16_t offset, gattc_write_cb_t write_cb)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->write(conn_handle, attr_handle, value, length, offset, write_cb);
}

bt_status_t ble_gattc_write_without_response(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t *value, uint16_t length, gattc_write_cb_t write_cb)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->write_without_response(conn_handle, attr_handle, value, length, write_cb);
}

bt_status_t ble_gattc_subscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle, gattc_write_cb_t write_cb, gattc_notify_cb_t notify_cb)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->subscribe(conn_handle, value_handle, cccd_handle, write_cb, notify_cb);
}

bt_status_t ble_gattc_unsubscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle, gattc_write_cb_t write_cb)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->unsubscribe(conn_handle, value_handle, cccd_handle, write_cb);
}

bt_status_t ble_gattc_exchange_mtu(gattc_handle_t conn_handle, uint32_t mtu)
{
    gattc_interface_t *profile = get_profile_service();

    return profile->exchange_mtu(conn_handle, mtu);
}
