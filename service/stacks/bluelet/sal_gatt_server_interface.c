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
#include "bt_list.h"
#include "sal.h"
#include "sal_bluelet.h"
#include "sal_gatt_server_interface.h"

typedef struct {
    uint16_t group_id;
    SERVICE_GATT_ELEMENT_S *elements;
    uint32_t elements_size;
} sal_gatt_database_t;

static bt_list_t *g_gatt_db_list = NULL;

static void gatt_db_free(void *data)
{
    sal_gatt_database_t *db_group = (sal_gatt_database_t *)data;
    if (db_group->elements)
        free(db_group->elements);
    free(db_group);
}

static bool gatt_db_group_cmp(void *gatt_db, void *handle)
{
    return (((sal_gatt_database_t *)gatt_db)->group_id == GATT_ELEMENT_GROUP_ID(*((uint16_t *)handle)));
}

static SERVICE_GATT_ELEMENT_S *gatt_db_find_element_by_handle(bt_list_t *db_list, uint16_t handle)
{
    sal_gatt_database_t *db_group = bt_list_find(db_list, gatt_db_group_cmp, &handle);
    if (db_group == NULL) {
        return NULL;
    }

    SERVICE_GATT_ELEMENT_S *sal_element = db_group->elements;
    for (int i = 0; i < db_group->elements_size; i++, sal_element++) {
        if (sal_element->id == handle)
            return sal_element;
    }
    return NULL;
}

static void gatts_connection_state_changed_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_connection_state_changed(&addr, bluelet_profile_connection_state(state));
}

static void gatts_elements_added_callback(SERVICE_GATT_STATUS status, SERVICE_GATT_ELEMENT_S *elements, uint16_t size)
{
    if (!g_gatt_db_list)
        return;

    sal_gatt_database_t *db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &elements->id);
    if (db_group != NULL) {
        if_gatts_on_elements_added(bluelet_gatt_status(status), (uint16_t)elements->id, size);
    }
}

static void gatts_elements_removed_callback(SERVICE_GATT_STATUS status, SERVICE_GATT_ELEMENT_S *elements, uint16_t size)
{
    if (!g_gatt_db_list)
        return;

    sal_gatt_database_t *db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &elements->id);
    if (db_group != NULL) {
        if_gatts_on_elements_removed(bluelet_gatt_status(status), (uint16_t)elements->id, size);
        bt_list_remove(g_gatt_db_list, db_group);
    }
}

static void gatts_phy_read_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy, SERVICE_BLE_PHY_TYPE rx_phy)
{
    // TODO
}

static void gatts_phy_update_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy, SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_GATT_STATUS status)
{
    // TODO
}

static void gatts_received_element_read_request_callback(BD_ADDR remote_addr, uint32_t request_id, SERVICE_GATT_ELEMENT_S *element)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_received_element_read_request(&addr, request_id, element->id);
}

static void gatts_received_element_write_request_callback(BD_ADDR remote_addr, uint32_t request_id, SERVICE_GATT_ELEMENT_S *element,
                                                          uint8_t *value, uint16_t offset, uint16_t length)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_received_element_write_request(&addr, request_id, element->id, value, offset, length, !(element->properties & GATT_PROPERTY_WRITE_NO_RESPONSE));
}

static void gatts_mtu_changed_callback(BD_ADDR remote_addr, uint32_t mtu)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_mtu_changed(&addr, mtu);
}

static void gatts_notification_sent_callback(BD_ADDR remote_addr, SERVICE_GATT_ELEMENT_S *element, SERVICE_GATT_STATUS status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_notification_sent(&addr, element->id, bluelet_gatt_status(status));
}

static GATT_SERVER_CALLBACKS_S gatts_callbacks = {
    .size = sizeof(gatts_callbacks),
    .gatt_server_connection_state_changed_cb = gatts_connection_state_changed_callback,
    .gatt_server_elements_added_cb = gatts_elements_added_callback,
    .gatt_server_elements_removed_cb = gatts_elements_removed_callback,
    .gatt_server_phy_read_cb = gatts_phy_read_callback,
    .gatt_server_phy_update_cb = gatts_phy_update_callback,
    .gatt_server_received_element_read_request_cb = gatts_received_element_read_request_callback,
    .gatt_server_received_element_write_request_cb = gatts_received_element_write_request_callback,
    .gatt_server_mtu_changed_cb = gatts_mtu_changed_callback,
    .gatt_server_notification_sent_cb = gatts_notification_sent_callback,
};

bt_status_t bt_sal_gatt_server_enable(void)
{
    SAL_CHECK_RET(service_adapter_gatt_server_open(&gatts_callbacks), GATT_SUCCESS);
    g_gatt_db_list = bt_list_new(gatt_db_free);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_disable(void)
{
    SAL_CHECK_RET(service_adapter_gatt_server_close(), GATT_SUCCESS);
    bt_list_free(g_gatt_db_list);
    g_gatt_db_list = NULL;

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_add_elements(gatt_element_t *elements, uint16_t size)
{
    SAL_CHECK_PARAM(elements);
    SAL_CHECK_PARAM(size);

    sal_gatt_database_t *db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &elements->handle);
    if (db_group == NULL) {
        db_group = (sal_gatt_database_t *)malloc(sizeof(sal_gatt_database_t));
        if (db_group == NULL)
            return BT_STATUS_NOMEM;

        db_group->group_id = GATT_ELEMENT_GROUP_ID(elements->handle);
        db_group->elements = NULL;
        db_group->elements_size = 0;
        bt_list_add_tail(g_gatt_db_list, db_group);
    }

    SERVICE_GATT_ELEMENT_S *sal_elements = (SERVICE_GATT_ELEMENT_S *)realloc(db_group->elements, sizeof(SERVICE_GATT_ELEMENT_S) * size);
    if (sal_elements == NULL)
        return BT_STATUS_NOMEM;

    db_group->elements = sal_elements;
    db_group->elements_size = size;
    for (int i = 0; i < size; i++, sal_elements++) {
        sal_elements->id = elements[i].handle;
        sal_elements->type = elements[i].type;
        sal_elements->properties = elements[i].properties;
        sal_elements->permissions = elements[i].permissions;
        memcpy(sal_elements->uuid, &elements[i].uuid.val, sizeof(sal_elements->uuid));
    }

    SAL_CHECK_RET(service_adapter_gatt_server_add_elements(db_group->elements, size), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_remove_elements(gatt_element_t *elements, uint16_t size)
{
    SAL_CHECK_PARAM(elements);
    SAL_CHECK_PARAM(size);

    sal_gatt_database_t *db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &elements->handle);
    if (db_group == NULL)
        return BT_STATUS_NO_RESOURCES;

    uint32_t *remove_ids = (uint32_t *)malloc(sizeof(uint32_t) * size);
    if (remove_ids == NULL)
        return BT_STATUS_NOMEM;

    for (int i = 0; i < size; i++) {
        remove_ids[i] = elements[i].handle;
    }

    SERVICE_GATT_STATUS ret = service_adapter_gatt_server_remove_elements(remove_ids, size);
    free(remove_ids);

    SAL_CHECK_RET(ret, GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_connect(bt_address_t *addr, ble_addr_type_t addr_type)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_connect_v1(addr->addr, addr_type), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_cancel_connection(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_cancel_connection(addr->addr), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_send_response(bt_address_t *addr, uint32_t request_id, uint8_t *value, uint16_t length)
{
    SAL_CHECK_PARAM(addr);

    SERVICE_GATT_RESPONSE_S *sal_response = (SERVICE_GATT_RESPONSE_S *)malloc(sizeof(SERVICE_GATT_RESPONSE_S) + length);
    if (sal_response == NULL)
        return BT_STATUS_NOMEM;

    sal_response->request_id = request_id;
    sal_response->status = GATT_SUCCESS;
    sal_response->length = length;
    if (value && length)
        memcpy(sal_response->value, value, length);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_server_send_response(addr->addr, sal_response);
    free(sal_response);

    SAL_CHECK_RET(ret, GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_send_notification(bt_address_t *addr, uint16_t element_id, uint8_t *value, uint16_t length)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(value);

    SERVICE_GATT_ELEMENT_S *sal_element = gatt_db_find_element_by_handle(g_gatt_db_list, element_id);
    if (sal_element == NULL)
        return BT_STATUS_FAIL;

    SAL_CHECK_RET(service_adapter_gatt_server_send_notification(addr->addr, sal_element, value, length), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_send_indication(bt_address_t *addr, uint16_t element_id, uint8_t *value, uint16_t length)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(value);

    SERVICE_GATT_ELEMENT_S *sal_element = gatt_db_find_element_by_handle(g_gatt_db_list, element_id);
    if (sal_element == NULL)
        return BT_STATUS_FAIL;

    SAL_CHECK_RET(service_adapter_gatt_server_send_indication(addr->addr, sal_element, value, length), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_read_phy(bt_address_t *addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_read_phy(addr->addr), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_set_phy(bt_address_t *addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_set_phy(addr->addr, tx_phy, rx_phy), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}
