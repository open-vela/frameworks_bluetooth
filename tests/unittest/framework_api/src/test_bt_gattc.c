/****************************************************************************
 * tests/unittest/framework_api/src/test_bt_gattc.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/* clang-format off */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
/* clang-format on */

#include <stdlib.h>
#include <string.h>

#include "gattc_service.h"
#include "bt_gattc.h"
#include "bt_status.h"

#include "mock_service_manager.h"
#include "cm_bt_gattc.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/


/****************************************************************************
 * Private Functions - Mock profile interface
 ****************************************************************************/

static gattc_handle_t g_mock_handle = (void *)0x1234;

static bt_status_t mock_create_connect(void *remote, gattc_handle_t *phandle, gattc_callbacks_t *cbs)
{
    *phandle = mock_ptr_type(gattc_handle_t);
    return mock_type(bt_status_t);
}

static bt_status_t mock_delete_connect(gattc_handle_t handle)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_connect(gattc_handle_t handle, bt_address_t *addr, ble_addr_type_t type)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_disconnect(gattc_handle_t handle)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_discover_service(gattc_handle_t handle, bt_uuid_t *uuid)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_get_attribute_by_handle(gattc_handle_t handle, uint16_t attr_handle, gatt_attr_desc_t *desc)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_get_attribute_by_uuid(gattc_handle_t handle, uint16_t start, uint16_t end, bt_uuid_t *uuid, gatt_attr_desc_t *desc)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_read(gattc_handle_t handle, uint16_t attr_handle)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_write(gattc_handle_t handle, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_write_without_response(gattc_handle_t handle, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_write_signed(gattc_handle_t handle, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_subscribe(gattc_handle_t handle, uint16_t attr_handle, uint16_t ccc_value)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_unsubscribe(gattc_handle_t handle, uint16_t attr_handle)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_exchange_mtu(gattc_handle_t handle, uint32_t mtu)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_update_connection_parameter(gattc_handle_t handle,
    uint32_t min_interval, uint32_t max_interval, uint32_t latency,
    uint32_t timeout, uint32_t min_ce, uint32_t max_ce)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_read_phy(gattc_handle_t handle)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_update_phy(gattc_handle_t handle, ble_phy_type_t tx, ble_phy_type_t rx)
{
    return mock_type(bt_status_t);
}

static bt_status_t mock_read_rssi(gattc_handle_t handle)
{
    return mock_type(bt_status_t);
}

static gattc_interface_t g_mock_interface = {
    .create_connect = mock_create_connect,
    .delete_connect = mock_delete_connect,
    .connect = mock_connect,
    .disconnect = mock_disconnect,
    .discover_service = mock_discover_service,
    .get_attribute_by_handle = mock_get_attribute_by_handle,
    .get_attribute_by_uuid = mock_get_attribute_by_uuid,
    .read = mock_read,
    .write = mock_write,
    .write_without_response = mock_write_without_response,
    .write_signed = mock_write_signed,
    .subscribe = mock_subscribe,
    .unsubscribe = mock_unsubscribe,
    .exchange_mtu = mock_exchange_mtu,
    .update_connection_parameter = mock_update_connection_parameter,
    .read_phy = mock_read_phy,
    .update_phy = mock_update_phy,
    .read_rssi = mock_read_rssi,
};


int test_bt_gattc_setup(FAR void **state)
{
    bt_instance_t *ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_GATTC, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_gattc_teardown(FAR void **state)
{
    if (*state) {
        test_free(*state);
        *state = NULL;
    }
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

void test_bt_gattc_create_connect_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    gattc_handle_t handle;
    gattc_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_create_connect, g_mock_handle);
    will_return(mock_create_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_create_connect(ins, &handle, &cbs), BT_STATUS_SUCCESS);
    assert_ptr_equal(handle, g_mock_handle);
}

void test_bt_gattc_create_connect_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    gattc_handle_t handle;
    gattc_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_create_connect, NULL);
    will_return(mock_create_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_create_connect(ins, &handle, &cbs), BT_STATUS_FAIL);
}

void test_bt_gattc_delete_connect_normal(FAR void **state)
{
    will_return(mock_delete_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_delete_connect(g_mock_handle), BT_STATUS_SUCCESS);
}

void test_bt_gattc_delete_connect_fail(FAR void **state)
{
    will_return(mock_delete_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_delete_connect(g_mock_handle), BT_STATUS_FAIL);
}

void test_bt_gattc_connect_normal(FAR void **state)
{
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_connect(g_mock_handle, &addr, 0), BT_STATUS_SUCCESS);
}

void test_bt_gattc_connect_fail(FAR void **state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_connect(g_mock_handle, &addr, 0), BT_STATUS_FAIL);
}

void test_bt_gattc_disconnect_normal(FAR void **state)
{
    will_return(mock_disconnect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_disconnect(g_mock_handle), BT_STATUS_SUCCESS);
}

void test_bt_gattc_disconnect_fail(FAR void **state)
{
    will_return(mock_disconnect, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_disconnect(g_mock_handle), BT_STATUS_FAIL);
}

void test_bt_gattc_discover_service_normal(FAR void **state)
{
    bt_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    will_return(mock_discover_service, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_discover_service(g_mock_handle, &uuid), BT_STATUS_SUCCESS);
}

void test_bt_gattc_discover_service_fail(FAR void **state)
{
    will_return(mock_discover_service, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_discover_service(g_mock_handle, NULL), BT_STATUS_FAIL);
}

void test_bt_gattc_get_attribute_by_handle_normal(FAR void **state)
{
    gatt_attr_desc_t desc;
    will_return(mock_get_attribute_by_handle, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_get_attribute_by_handle(g_mock_handle, 0x0001, &desc), BT_STATUS_SUCCESS);
}

void test_bt_gattc_get_attribute_by_handle_fail(FAR void **state)
{
    gatt_attr_desc_t desc;
    will_return(mock_get_attribute_by_handle, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_get_attribute_by_handle(g_mock_handle, 0xFFFF, &desc), BT_STATUS_FAIL);
}

void test_bt_gattc_get_attribute_by_uuid_normal(FAR void **state)
{
    bt_uuid_t uuid;
    gatt_attr_desc_t desc;
    memset(&uuid, 0, sizeof(uuid));
    will_return(mock_get_attribute_by_uuid, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_get_attribute_by_uuid(g_mock_handle, 0x0001, 0xFFFF, &uuid, &desc), BT_STATUS_SUCCESS);
}

void test_bt_gattc_get_attribute_by_uuid_fail(FAR void **state)
{
    bt_uuid_t uuid;
    gatt_attr_desc_t desc;
    memset(&uuid, 0, sizeof(uuid));
    will_return(mock_get_attribute_by_uuid, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_get_attribute_by_uuid(g_mock_handle, 0x0001, 0xFFFF, &uuid, &desc), BT_STATUS_FAIL);
}

void test_bt_gattc_read_normal(FAR void **state)
{
    will_return(mock_read, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_read(g_mock_handle, 0x0003), BT_STATUS_SUCCESS);
}

void test_bt_gattc_read_fail(FAR void **state)
{
    will_return(mock_read, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_read(g_mock_handle, 0x0003), BT_STATUS_FAIL);
}

void test_bt_gattc_write_normal(FAR void **state)
{
    uint8_t value[] = {0x01, 0x02};
    will_return(mock_write, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_write(g_mock_handle, 0x0003, value, 2), BT_STATUS_SUCCESS);
}

void test_bt_gattc_write_fail(FAR void **state)
{
    uint8_t value[] = {0x01};
    will_return(mock_write, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_write(g_mock_handle, 0x0003, value, 1), BT_STATUS_FAIL);
}

void test_bt_gattc_write_without_response_normal(FAR void **state)
{
    uint8_t value[] = {0x01, 0x02};
    will_return(mock_write_without_response, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_write_without_response(g_mock_handle, 0x0003, value, 2), BT_STATUS_SUCCESS);
}

void test_bt_gattc_write_without_response_fail(FAR void **state)
{
    uint8_t value[] = {0x01};
    will_return(mock_write_without_response, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_write_without_response(g_mock_handle, 0x0003, value, 1), BT_STATUS_FAIL);
}

void test_bt_gattc_write_with_signed_normal(FAR void **state)
{
    uint8_t value[] = {0x01, 0x02};
    will_return(mock_write_signed, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_write_with_signed(g_mock_handle, 0x0003, value, 2), BT_STATUS_SUCCESS);
}

void test_bt_gattc_write_with_signed_fail(FAR void **state)
{
    uint8_t value[] = {0x01};
    will_return(mock_write_signed, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_write_with_signed(g_mock_handle, 0x0003, value, 1), BT_STATUS_FAIL);
}

void test_bt_gattc_subscribe_normal(FAR void **state)
{
    will_return(mock_subscribe, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_subscribe(g_mock_handle, 0x0004, 0x0001), BT_STATUS_SUCCESS);
}

void test_bt_gattc_subscribe_fail(FAR void **state)
{
    will_return(mock_subscribe, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_subscribe(g_mock_handle, 0x0004, 0x0001), BT_STATUS_FAIL);
}

void test_bt_gattc_unsubscribe_normal(FAR void **state)
{
    will_return(mock_unsubscribe, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_unsubscribe(g_mock_handle, 0x0004), BT_STATUS_SUCCESS);
}

void test_bt_gattc_unsubscribe_fail(FAR void **state)
{
    will_return(mock_unsubscribe, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_unsubscribe(g_mock_handle, 0x0004), BT_STATUS_FAIL);
}

void test_bt_gattc_exchange_mtu_normal(FAR void **state)
{
    will_return(mock_exchange_mtu, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_exchange_mtu(g_mock_handle, 512), BT_STATUS_SUCCESS);
}

void test_bt_gattc_exchange_mtu_fail(FAR void **state)
{
    will_return(mock_exchange_mtu, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_exchange_mtu(g_mock_handle, 512), BT_STATUS_FAIL);
}

void test_bt_gattc_update_connection_parameter_normal(FAR void **state)
{
    will_return(mock_update_connection_parameter, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_update_connection_parameter(g_mock_handle, 6, 12, 0, 500, 0, 0), BT_STATUS_SUCCESS);
}

void test_bt_gattc_update_connection_parameter_fail(FAR void **state)
{
    will_return(mock_update_connection_parameter, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_update_connection_parameter(g_mock_handle, 6, 12, 0, 500, 0, 0), BT_STATUS_FAIL);
}

void test_bt_gattc_read_phy_normal(FAR void **state)
{
    will_return(mock_read_phy, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_read_phy(g_mock_handle), BT_STATUS_SUCCESS);
}

void test_bt_gattc_read_phy_fail(FAR void **state)
{
    will_return(mock_read_phy, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_read_phy(g_mock_handle), BT_STATUS_FAIL);
}

void test_bt_gattc_update_phy_normal(FAR void **state)
{
    will_return(mock_update_phy, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_update_phy(g_mock_handle, 0x01, 0x01), BT_STATUS_SUCCESS);
}

void test_bt_gattc_update_phy_fail(FAR void **state)
{
    will_return(mock_update_phy, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_update_phy(g_mock_handle, 0x02, 0x02), BT_STATUS_FAIL);
}

void test_bt_gattc_read_rssi_normal(FAR void **state)
{
    will_return(mock_read_rssi, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gattc_read_rssi(g_mock_handle), BT_STATUS_SUCCESS);
}

void test_bt_gattc_read_rssi_fail(FAR void **state)
{
    will_return(mock_read_rssi, BT_STATUS_FAIL);
    assert_int_equal(bt_gattc_read_rssi(g_mock_handle), BT_STATUS_FAIL);
}
