/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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

/* clang-format off */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
/* clang-format on */

#include <stdlib.h>
#include <string.h>

#include "bt_gatts.h"
#include "bt_status.h"
#include "gatts_service.h"

#include "cm_bt_gatts.h"
#include "mock_service_manager.h"

static gatts_handle_t g_mock_handle = (void*)0x5678;

static bt_status_t mock_register_service(void* remote, gatts_handle_t* phandle, gatts_callbacks_t* cbs)
{
    *phandle = mock_ptr_type(gatts_handle_t);
    return mock_type(bt_status_t);
}
static bt_status_t mock_unregister_service(gatts_handle_t h) { return mock_type(bt_status_t); }
static bt_status_t mock_connect(gatts_handle_t h, bt_address_t* a, ble_addr_type_t t) { return mock_type(bt_status_t); }
static bt_status_t mock_connect_bear(gatts_handle_t h, bt_address_t* a, ble_addr_type_t t, uint8_t b) { return mock_type(bt_status_t); }
static bt_status_t mock_disconnect(gatts_handle_t h, bt_address_t* a) { return mock_type(bt_status_t); }
static bt_status_t mock_add_attr_table(gatts_handle_t h, gatt_srv_db_t* db) { return mock_type(bt_status_t); }
static bt_status_t mock_remove_attr_table(gatts_handle_t h, uint16_t ah) { return mock_type(bt_status_t); }
static bt_status_t mock_set_attr_value(gatts_handle_t h, uint16_t ah, uint8_t* v, uint16_t l) { return mock_type(bt_status_t); }
static bt_status_t mock_get_attr_value(gatts_handle_t h, uint16_t ah, uint8_t* v, uint16_t* l) { return mock_type(bt_status_t); }
static bt_status_t mock_response(gatts_handle_t h, bt_address_t* a, uint32_t rh, uint8_t* v, uint16_t l) { return mock_type(bt_status_t); }
static bt_status_t mock_notify(gatts_handle_t h, bt_address_t* a, uint16_t ah, uint8_t* v, uint16_t l) { return mock_type(bt_status_t); }
static bt_status_t mock_indicate(gatts_handle_t h, bt_address_t* a, uint16_t ah, uint8_t* v, uint16_t l) { return mock_type(bt_status_t); }
static bt_status_t mock_read_phy(gatts_handle_t h, bt_address_t* a) { return mock_type(bt_status_t); }
static bt_status_t mock_update_phy(gatts_handle_t h, bt_address_t* a, ble_phy_type_t tx, ble_phy_type_t rx) { return mock_type(bt_status_t); }

static gatts_interface_t g_mock_interface = {
    .register_service = mock_register_service,
    .unregister_service = mock_unregister_service,
    .connect = mock_connect,
    .connect_bear = mock_connect_bear,
    .disconnect = mock_disconnect,
    .add_attr_table = mock_add_attr_table,
    .remove_attr_table = mock_remove_attr_table,
    .set_attr_value = mock_set_attr_value,
    .get_attr_value = mock_get_attr_value,
    .response = mock_response,
    .notify = mock_notify,
    .indicate = mock_indicate,
    .read_phy = mock_read_phy,
    .update_phy = mock_update_phy,
};

int test_bt_gatts_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_GATTS, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_gatts_teardown(FAR void** state)
{
    if (*state) {
        test_free(*state);
        *state = NULL;
    }
    return 0;
}

void test_bt_gatts_register_service_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    gatts_handle_t handle;
    gatts_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_service, g_mock_handle);
    will_return(mock_register_service, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_register_service(ins, &handle, &cbs), BT_STATUS_SUCCESS);
}

void test_bt_gatts_register_service_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    gatts_handle_t handle;
    gatts_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_service, NULL);
    will_return(mock_register_service, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_register_service(ins, &handle, &cbs), BT_STATUS_FAIL);
}

void test_bt_gatts_unregister_service_normal(FAR void** state)
{
    will_return(mock_unregister_service, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_unregister_service(g_mock_handle), BT_STATUS_SUCCESS);
}

void test_bt_gatts_unregister_service_fail(FAR void** state)
{
    will_return(mock_unregister_service, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_unregister_service(g_mock_handle), BT_STATUS_FAIL);
}

void test_bt_gatts_connect_normal(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_connect(g_mock_handle, &addr, 0), BT_STATUS_SUCCESS);
}

void test_bt_gatts_connect_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_connect(g_mock_handle, &addr, 0), BT_STATUS_FAIL);
}

void test_bt_gatts_connect_bear_normal(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_connect_bear, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_connect_bear(g_mock_handle, &addr, 0, 1), BT_STATUS_SUCCESS);
}

void test_bt_gatts_connect_bear_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_connect_bear, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_connect_bear(g_mock_handle, &addr, 0, 1), BT_STATUS_FAIL);
}

void test_bt_gatts_disconnect_normal(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_disconnect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_disconnect(g_mock_handle, &addr), BT_STATUS_SUCCESS);
}

void test_bt_gatts_disconnect_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_disconnect, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_disconnect(g_mock_handle, &addr), BT_STATUS_FAIL);
}

void test_bt_gatts_add_attr_table_normal(FAR void** state)
{
    gatt_srv_db_t db;
    memset(&db, 0, sizeof(db));
    will_return(mock_add_attr_table, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_add_attr_table(g_mock_handle, &db), BT_STATUS_SUCCESS);
}

void test_bt_gatts_add_attr_table_fail(FAR void** state)
{
    will_return(mock_add_attr_table, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_add_attr_table(g_mock_handle, NULL), BT_STATUS_FAIL);
}

void test_bt_gatts_remove_attr_table_normal(FAR void** state)
{
    will_return(mock_remove_attr_table, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_remove_attr_table(g_mock_handle, 0x0001), BT_STATUS_SUCCESS);
}

void test_bt_gatts_remove_attr_table_fail(FAR void** state)
{
    will_return(mock_remove_attr_table, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_remove_attr_table(g_mock_handle, 0x0001), BT_STATUS_FAIL);
}

void test_bt_gatts_set_attr_value_normal(FAR void** state)
{
    uint8_t val[] = { 0x01, 0x02 };
    will_return(mock_set_attr_value, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_set_attr_value(g_mock_handle, 0x0003, val, 2), BT_STATUS_SUCCESS);
}

void test_bt_gatts_set_attr_value_fail(FAR void** state)
{
    uint8_t val[] = { 0x01 };
    will_return(mock_set_attr_value, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_set_attr_value(g_mock_handle, 0x0003, val, 1), BT_STATUS_FAIL);
}

void test_bt_gatts_get_attr_value_normal(FAR void** state)
{
    uint8_t val[64];
    uint16_t len = 64;
    will_return(mock_get_attr_value, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_get_attr_value(g_mock_handle, 0x0003, val, &len), BT_STATUS_SUCCESS);
}

void test_bt_gatts_get_attr_value_fail(FAR void** state)
{
    uint8_t val[64];
    uint16_t len = 64;
    will_return(mock_get_attr_value, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_get_attr_value(g_mock_handle, 0x0003, val, &len), BT_STATUS_FAIL);
}

void test_bt_gatts_response_normal(FAR void** state)
{
    bt_address_t addr;
    uint8_t val[] = { 0x01 };
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_response, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_response(g_mock_handle, &addr, 1, val, 1), BT_STATUS_SUCCESS);
}

void test_bt_gatts_response_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_response, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_response(g_mock_handle, &addr, 1, NULL, 0), BT_STATUS_FAIL);
}

void test_bt_gatts_notify_normal(FAR void** state)
{
    bt_address_t addr;
    uint8_t val[] = { 0x01 };
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_notify, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_notify(g_mock_handle, &addr, 0x0003, val, 1), BT_STATUS_SUCCESS);
}

void test_bt_gatts_notify_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_notify, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_notify(g_mock_handle, &addr, 0x0003, NULL, 0), BT_STATUS_FAIL);
}

void test_bt_gatts_indicate_normal(FAR void** state)
{
    bt_address_t addr;
    uint8_t val[] = { 0x01 };
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_indicate, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_indicate(g_mock_handle, &addr, 0x0003, val, 1), BT_STATUS_SUCCESS);
}

void test_bt_gatts_indicate_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_indicate, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_indicate(g_mock_handle, &addr, 0x0003, NULL, 0), BT_STATUS_FAIL);
}

void test_bt_gatts_read_phy_normal(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_read_phy, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_read_phy(g_mock_handle, &addr), BT_STATUS_SUCCESS);
}

void test_bt_gatts_read_phy_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_read_phy, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_read_phy(g_mock_handle, &addr), BT_STATUS_FAIL);
}

void test_bt_gatts_update_phy_normal(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_update_phy, BT_STATUS_SUCCESS);
    assert_int_equal(bt_gatts_update_phy(g_mock_handle, &addr, 0x01, 0x01), BT_STATUS_SUCCESS);
}

void test_bt_gatts_update_phy_fail(FAR void** state)
{
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_update_phy, BT_STATUS_FAIL);
    assert_int_equal(bt_gatts_update_phy(g_mock_handle, &addr, 0x02, 0x02), BT_STATUS_FAIL);
}
