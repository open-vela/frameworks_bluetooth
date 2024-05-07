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

#ifdef CONFIG_BLUETOOTH_GATT

typedef struct {
    uint16_t start_handle;
    uint16_t end_handle;
    int elements_size;
    SERVICE_GATT_ELEMENT_S elements[0];
} sal_gatt_database_t;

static pthread_mutex_t db_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static bt_list_t* g_gatt_db_list = NULL;
static bt_list_t* g_peer_addr_list = NULL;

static bool gatt_db_group_cmp(void* gatt_db, void* handle)
{
    uint16_t f_handle = *(uint16_t*)handle;
    sal_gatt_database_t* f_gatt_db = (sal_gatt_database_t*)gatt_db;
    return (f_handle >= f_gatt_db->start_handle && f_handle <= f_gatt_db->end_handle);
}

static SERVICE_GATT_ELEMENT_S* gatt_db_find_element_by_handle(bt_list_t* db_list, uint16_t handle)
{
    sal_gatt_database_t* db_group = bt_list_find(db_list, gatt_db_group_cmp, &handle);
    if (db_group == NULL) {
        return NULL;
    }

    SERVICE_GATT_ELEMENT_S* sal_element = db_group->elements;
    for (int i = 0; i < db_group->elements_size; i++, sal_element++) {
        if (sal_element->id == handle)
            return sal_element;
    }
    return NULL;
}

static bool peer_addr_cmp(void* peer, void* addr)
{
    return memcmp(peer, addr, BT_ADDR_LENGTH) == 0;
}

static bt_address_t* gatts_add_peer_addr(bt_list_t* addr_list, BD_ADDR remote_addr)
{
    bt_address_t* addr = bt_list_find(addr_list, peer_addr_cmp, remote_addr);
    if (addr)
        return addr;

    addr = malloc(sizeof(bt_address_t));
    if (!addr)
        return NULL;

    memcpy(addr->addr, remote_addr, BT_ADDR_LENGTH);
    bt_list_add_tail(addr_list, addr);
    return addr;
}

static void gatts_remove_peer_addr(bt_list_t* addr_list, BD_ADDR remote_addr)
{
    bt_address_t* addr = bt_list_find(addr_list, peer_addr_cmp, remote_addr);
    if (addr)
        bt_list_remove(addr_list, addr);
}

static void gatts_connection_state_changed_callback(BD_ADDR remote_addr, SERVICE_PROFILE_CONNECTION_STATE state)
{
    bt_address_t addr;
    if (state == SERVICE_PROFILE_CONNECTED) {
        gatts_add_peer_addr(g_peer_addr_list, remote_addr);
    } else if (state == SERVICE_PROFILE_DISCONNECTED) {
        gatts_remove_peer_addr(g_peer_addr_list, remote_addr);
    }
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_connection_state_changed(&addr, bluelet_profile_connection_state(state));
}

static void gatts_elements_added_callback(SERVICE_GATT_STATUS status, SERVICE_GATT_ELEMENT_S* elements, uint16_t size)
{
    sal_gatt_database_t* db_group;

    if (!g_gatt_db_list)
        return;

    pthread_mutex_lock(&db_lock);
    db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &elements->id);
    pthread_mutex_unlock(&db_lock);
    if (db_group != NULL) {
        if_gatts_on_elements_added(bluelet_gatt_status(status), (uint16_t)elements->id, size);
    }
}

static void gatts_elements_removed_callback(SERVICE_GATT_STATUS status, SERVICE_GATT_ELEMENT_S* elements, uint16_t size)
{
    sal_gatt_database_t* db_group;
    uint16_t element_id;

    if (!g_gatt_db_list)
        return;

    element_id = elements->id;
    pthread_mutex_lock(&db_lock);
    db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &element_id);
    if (db_group != NULL) {
        bt_list_remove(g_gatt_db_list, db_group);
    }
    pthread_mutex_unlock(&db_lock);

    if (db_group != NULL) {
        if_gatts_on_elements_removed(bluelet_gatt_status(status), element_id, size);
    }
}

static void gatts_phy_read_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy, SERVICE_BLE_PHY_TYPE rx_phy)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_phy_read(&addr, tx_phy, rx_phy);
}

static void gatts_phy_update_callback(BD_ADDR remote_addr, SERVICE_BLE_PHY_TYPE tx_phy, SERVICE_BLE_PHY_TYPE rx_phy, SERVICE_GATT_STATUS status)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_phy_updated(&addr, tx_phy, rx_phy, bluelet_gatt_status(status));
}

static void gatts_received_element_read_request_callback(BD_ADDR remote_addr, uint32_t request_id, SERVICE_GATT_ELEMENT_S* element)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_received_element_read_request(&addr, request_id, element->id);
}

static void gatts_received_element_write_request_callback(BD_ADDR remote_addr, uint32_t request_id, SERVICE_GATT_ELEMENT_S* element,
    uint8_t* value, uint16_t offset, uint16_t length)
{
    bt_address_t addr;
    memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
    if_gatts_on_received_element_write_request(&addr, request_id, element->id, value, offset, length);
}

static void gatts_mtu_changed_callback(BD_ADDR remote_addr, uint32_t mtu)
{
    if (bt_list_find(g_peer_addr_list, peer_addr_cmp, remote_addr)) {
        bt_address_t addr;
        memcpy(addr.addr, remote_addr, BT_ADDR_LENGTH);
        if_gatts_on_mtu_changed(&addr, mtu);
    }
}

static void gatts_notification_sent_callback(BD_ADDR remote_addr, SERVICE_GATT_ELEMENT_S* element, SERVICE_GATT_STATUS status)
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
    pthread_mutex_lock(&db_lock);
    g_gatt_db_list = bt_list_new(free);
    g_peer_addr_list = bt_list_new(free);
    pthread_mutex_unlock(&db_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_disable(void)
{
    SAL_CHECK_RET(service_adapter_gatt_server_close(), GATT_SUCCESS);
    pthread_mutex_lock(&db_lock);
    bt_list_free(g_gatt_db_list);
    g_gatt_db_list = NULL;
    bt_list_free(g_peer_addr_list);
    g_peer_addr_list = NULL;
    pthread_mutex_unlock(&db_lock);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_add_elements(gatt_element_t* elements, uint16_t size)
{
    sal_gatt_database_t* db_group;

    SAL_CHECK_PARAM(elements);
    SAL_CHECK_PARAM(size);

    pthread_mutex_lock(&db_lock);
    db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &elements->handle);
    if (db_group != NULL) {
        pthread_mutex_unlock(&db_lock);
        return BT_STATUS_FAIL;
    }

    db_group = (sal_gatt_database_t*)malloc(sizeof(sal_gatt_database_t) + sizeof(SERVICE_GATT_ELEMENT_S) * size);
    if (db_group == NULL) {
        pthread_mutex_unlock(&db_lock);
        return BT_STATUS_NOMEM;
    }

    db_group->start_handle = elements[0].handle;
    db_group->end_handle = elements[size - 1].handle;
    db_group->elements_size = size;
    for (int i = 0; i < size; i++) {
        db_group->elements[i].id = elements[i].handle;
        db_group->elements[i].type = elements[i].type;
        db_group->elements[i].properties = elements[i].properties;
        db_group->elements[i].permissions = elements[i].permissions;
        memcpy(db_group->elements[i].uuid, &elements[i].uuid.val, sizeof(BT_UUID_T));
    }
    bt_list_add_tail(g_gatt_db_list, db_group);
    pthread_mutex_unlock(&db_lock);

    SAL_CHECK_RET(service_adapter_gatt_server_add_elements(db_group->elements, size), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_remove_elements(gatt_element_t* elements, uint16_t size)
{
    sal_gatt_database_t* db_group;

    SAL_CHECK_PARAM(elements);
    SAL_CHECK_PARAM(size);

    pthread_mutex_lock(&db_lock);
    db_group = bt_list_find(g_gatt_db_list, gatt_db_group_cmp, &elements->handle);
    pthread_mutex_unlock(&db_lock);
    if (db_group == NULL)
        return BT_STATUS_NO_RESOURCES;

    uint32_t* remove_ids = (uint32_t*)malloc(sizeof(uint32_t) * size);
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

bt_status_t bt_sal_gatt_server_connect(bt_address_t* addr, ble_addr_type_t addr_type)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_connect_v1(addr->addr, addr_type), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_cancel_connection(bt_address_t* addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_cancel_connection(addr->addr), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_send_response(bt_address_t* addr, uint32_t request_id, uint8_t* value, uint16_t length)
{
    SAL_CHECK_PARAM(addr);

    SERVICE_GATT_RESPONSE_S* sal_response = (SERVICE_GATT_RESPONSE_S*)malloc(sizeof(SERVICE_GATT_RESPONSE_S) + length);
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

bt_status_t bt_sal_gatt_server_send_notification(bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length)
{
    SERVICE_GATT_ELEMENT_S* sal_element;

    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(value);

    pthread_mutex_lock(&db_lock);
    sal_element = gatt_db_find_element_by_handle(g_gatt_db_list, element_id);
    pthread_mutex_unlock(&db_lock);
    if (sal_element == NULL)
        return BT_STATUS_FAIL;

    SAL_CHECK_RET(service_adapter_gatt_server_send_notification(addr->addr, sal_element, value, length), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_send_indication(bt_address_t* addr, uint16_t element_id, uint8_t* value, uint16_t length)
{
    SERVICE_GATT_ELEMENT_S* sal_element;

    SAL_CHECK_PARAM(addr);
    SAL_CHECK_PARAM(value);

    pthread_mutex_lock(&db_lock);
    sal_element = gatt_db_find_element_by_handle(g_gatt_db_list, element_id);
    pthread_mutex_unlock(&db_lock);
    if (sal_element == NULL)
        return BT_STATUS_FAIL;

    SAL_CHECK_RET(service_adapter_gatt_server_send_indication(addr->addr, sal_element, value, length), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_read_phy(bt_address_t* addr)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_read_phy(addr->addr), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_gatt_server_set_phy(bt_address_t* addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    SAL_CHECK_PARAM(addr);
    SAL_CHECK_RET(service_adapter_gatt_server_set_phy(addr->addr, tx_phy, rx_phy), GATT_SUCCESS);

    return BT_STATUS_SUCCESS;
}

void bt_sal_gatt_server_connection_changed_callback(bt_address_t* addr, uint16_t connection_interval, uint16_t peripheral_latency,
    uint16_t supervision_timeout)
{
    if (bt_list_find(g_peer_addr_list, peer_addr_cmp, addr->addr)) {
        if_gatts_on_connection_parameter_changed(addr, connection_interval, peripheral_latency, supervision_timeout);
    }
}

#endif
