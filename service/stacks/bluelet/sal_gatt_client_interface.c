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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "stack_adapter_common.h"
#include "stack_adapter_gatt.h"

#include "bluetooth.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_gatt_client_interface.h"

static void gattc_connection_state_changed_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gattc_on_connection_state_changed(&addr, bluelet_profile_connection_state(state));
}

static void gattc_service_discovered_callback(BD_ADDR remote_addr, SERVICE_GATT_ELEMENT_S *elements, uint16_t size)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);

    if (elements == NULL || size == 0) {
        if_gattc_on_discover_completed(&addr, GATT_STATUS_SUCCESS);
        return;
    }

    gatt_element_t *disc_elements = (gatt_element_t *)calloc(size, sizeof(gatt_element_t));
    if (disc_elements == NULL) {
        if_gattc_on_discover_completed(&addr, GATT_STATUS_FAILURE);
        return;
    }

    gatt_element_t *sal_element = disc_elements;
    for (int i = 0; i < size; i++, sal_element++) {
        sal_element->handle = elements[i].id;
        sal_element->type = elements[i].type;
        sal_element->properties = elements[i].properties;
        sal_element->permissions = elements[i].permissions;
        sal_element->uuid.type = BT_UUID128_TYPE;
        memcpy(&sal_element->uuid.val, elements[i].uuid, sizeof(sal_element->uuid.val));
    }

    if_gattc_on_service_discovered(&addr, disc_elements, size);
}

static void gattc_element_read_callback(BD_ADDR remote_addr, SERVICE_GATT_ELEMENT_S *element,
                                        uint8_t *value, uint16_t length, SERVICE_GATT_STATUS status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gattc_on_element_read(&addr, element->id, value, length, bluelet_gatt_status(status));
}

static void gattc_element_written_callback(BD_ADDR remote_addr, SERVICE_GATT_ELEMENT_S *element, SERVICE_GATT_STATUS status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gattc_on_element_written(&addr, element->id, bluelet_gatt_status(status));
}

static void gattc_element_changed_callback(BD_ADDR remote_addr, SERVICE_GATT_ELEMENT_S *element, uint8_t *value, uint16_t length)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gattc_on_element_changed(&addr, element->id, value, length);
}

static void gattc_remote_rssi_read_callback(BD_ADDR remote_addr, int32_t rssi, SERVICE_GATT_STATUS status)
{
    // TODO
}

static void gattc_phy_read_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy, SERVICE_BLE_PHY_TYPE rx_phy)
{
    // TODO
}

static void gattc_phy_update_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy, SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_GATT_STATUS status)
{
    // TODO
}

static void gattc_mtu_changed_callback(BD_ADDR remote_addr, uint32_t mtu, SERVICE_GATT_STATUS status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gattc_on_mtu_changed(&addr, mtu, bluelet_gatt_status(status));
}

static GATT_CLIENT_CALLBACKS_S gattc_callbacks = {
    .size = sizeof(gattc_callbacks),
    .gatt_client_connection_state_changed_cb = gattc_connection_state_changed_callback,
    .gatt_client_service_discovered_cb = gattc_service_discovered_callback,
    .gatt_client_element_read_cb = gattc_element_read_callback,
    .gatt_client_element_written_cb = gattc_element_written_callback,
    .gatt_client_element_changed_cb = gattc_element_changed_callback,
    .gatt_client_remote_rssi_read_cb = gattc_remote_rssi_read_callback,
    .gatt_client_phy_read_cb = gattc_phy_read_callback,
    .gatt_client_phy_update_cb = gattc_phy_update_callback,
    .gatt_client_mtu_changed_cb = gattc_mtu_changed_callback,
};

bt_status_t bt_sal_gatt_client_connect(bt_address_t *addr, ble_addr_type_t addr_type)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_client_connect_v1(addr->addr, addr_type, &gattc_callbacks), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_disconnect(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_client_disconnect(addr->addr), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_discover_all_services(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_client_discover_services(addr->addr), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_discover_service_by_uuid(bt_address_t *addr, bt_uuid_t *uuid)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(uuid);
    SAL_CHECK_RET(service_adapter_gatt_client_discover_service(addr->addr, uuid->val.u128), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_read_element(bt_address_t *addr, uint16_t element_id)
{
    SAL_CHECK_PARAM(addr);

    SERVICE_GATT_ELEMENT_S char_element = {
        .id = element_id,
    };

    SAL_CHECK_RET(service_adapter_gatt_client_read_element(addr->addr, &char_element), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_write_element(bt_address_t *addr, uint16_t element_id, uint8_t *value, uint16_t length, gatt_write_type_t write_type)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(value);

    SERVICE_GATT_ELEMENT_S char_element = {
        .id = element_id,
    };
    if (write_type == GATT_WRITE_TYPE_NO_RSP)
        char_element.properties = GATT_PROPERTY_WRITE_NO_RESPONSE;
    else
        char_element.properties = GATT_PROPERTY_WRITE;

    SAL_CHECK_RET(service_adapter_gatt_client_write_element(addr->addr, &char_element, value, length), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_register_notifications(bt_address_t *addr, uint16_t element_id, bool enable, gatt_change_type_t change_type)
{
    SAL_CHECK_PARAM(addr);

    SERVICE_GATT_ELEMENT_S char_element = {
        .id = element_id,
        .type = CHARACTERISTIC,
    };
    if (change_type == GATT_CHANGE_TYPE_NOTIFY)
        char_element.properties = GATT_PROPERTY_NOTIFY;
    else
        char_element.properties = GATT_PROPERTY_INDICATE;

    SAL_CHECK_RET(service_adapter_gatt_client_register_notifications(addr->addr, &char_element, enable), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_client_send_mtu_req(bt_address_t *addr, uint32_t mtu)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_client_set_mtu(addr->addr, mtu), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}
