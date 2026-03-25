/****************************************************************************
 * tests/unittest/framework_api/src/test_bt_device.c
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

#include "bt_adapter.h"
#include "bt_device.h"
#include "bt_status.h"

#include "cm_bt_device.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/


/****************************************************************************
 * Private Functions - Mock adapter functions
 ****************************************************************************/

bt_status_t adapter_get_remote_identity_address(bt_address_t *bd_addr,
    bt_address_t *id_addr)
{
    check_expected_ptr(bd_addr);
    return mock_type(bt_status_t);
}

ble_addr_type_t adapter_get_le_remote_address_type(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(ble_addr_type_t);
}

bt_device_type_t adapter_get_remote_device_type(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_device_type_t);
}

bool adapter_get_remote_name(bt_address_t *addr, char *name)
{
    check_expected_ptr(addr);
    bool ret = mock_type(bool);
    if (ret) {
        strcpy(name, "TestDevice");
    }
    return ret;
}

uint32_t adapter_get_remote_device_class(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(uint32_t);
}

bt_status_t adapter_get_remote_uuids(bt_address_t *addr,
    bt_uuid_t **uuids, uint16_t *size, bt_allocator_t allocator)
{
    check_expected_ptr(addr);
    *size = 0;
    return mock_type(bt_status_t);
}

uint16_t adapter_get_remote_appearance(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(uint16_t);
}

int8_t adapter_get_remote_rssi(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(int8_t);
}

bool adapter_get_remote_alias(bt_address_t *addr, char *alias)
{
    check_expected_ptr(addr);
    bool ret = mock_type(bool);
    if (ret) {
        strcpy(alias, "TestAlias");
    }
    return ret;
}

bt_status_t adapter_set_remote_alias(bt_address_t *addr, const char *alias)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bool adapter_is_remote_connected(bt_address_t *addr,
    bt_transport_t transport)
{
    check_expected_ptr(addr);
    return mock_type(bool);
}

bool adapter_is_remote_encrypted(bt_address_t *addr,
    bt_transport_t transport)
{
    check_expected_ptr(addr);
    return mock_type(bool);
}

bool adapter_is_bond_initiate_local(bt_address_t *addr,
    bt_transport_t transport)
{
    check_expected_ptr(addr);
    return mock_type(bool);
}

bond_state_t adapter_get_remote_bond_state(bt_address_t *addr,
    bt_transport_t transport)
{
    check_expected_ptr(addr);
    return mock_type(bond_state_t);
}

bool adapter_is_remote_bonded(bt_address_t *addr,
    bt_transport_t transport)
{
    check_expected_ptr(addr);
    return mock_type(bool);
}

bt_status_t adapter_connect(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_disconnect(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_connect(bt_address_t *addr,
    ble_addr_type_t type, ble_connect_params_t *param)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_disconnect(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_connect_request_reply(bt_address_t *addr, bool accept)
{
    check_expected_ptr(addr);
    check_expected(accept);
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_set_phy(bt_address_t *addr,
    ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_create_bond(bt_address_t *addr,
    bt_transport_t transport)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_security_level(uint8_t level,
    bt_transport_t transport)
{
    check_expected(level);
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_set_bondable(bool bondable)
{
    check_expected(bondable);
    return mock_type(bt_status_t);
}

bt_status_t adapter_remove_bond(bt_address_t *addr, uint8_t transport)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_cancel_bond(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_pair_request_reply(bt_address_t *addr, bool accept)
{
    check_expected_ptr(addr);
    check_expected(accept);
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_pairing_confirmation(bt_address_t *addr,
    uint8_t transport, bool accept)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_pin_code(bt_address_t *addr, bool accept,
    char *pincode, int len)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_pass_key(bt_address_t *addr, uint8_t transport,
    bool accept, uint32_t passkey)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_set_legacy_tk(bt_address_t *addr,
    bt_128key_t tk_val)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_set_remote_oob_data(bt_address_t *addr,
    bt_128key_t c_val, bt_128key_t r_val)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_get_local_oob_data(bt_address_t *addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_device_setup(FAR void **state)
{
    bt_instance_t *ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    *state = ins;
    return 0;
}

int test_bt_device_teardown(FAR void **state)
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

/* bt_device_get_identity_address */

void test_bt_device_get_identity_address_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t bd_addr;
    bt_address_t id_addr;

    memset(&bd_addr, 0x11, sizeof(bd_addr));
    expect_any(adapter_get_remote_identity_address, bd_addr);
    will_return(adapter_get_remote_identity_address, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_get_identity_address(ins, &bd_addr, &id_addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_get_identity_address_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t bd_addr;
    bt_address_t id_addr;

    memset(&bd_addr, 0x22, sizeof(bd_addr));
    expect_any(adapter_get_remote_identity_address, bd_addr);
    will_return(adapter_get_remote_identity_address, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_get_identity_address(ins, &bd_addr, &id_addr);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_get_address_type */

void test_bt_device_get_address_type_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_le_remote_address_type, addr);
    will_return(adapter_get_le_remote_address_type, BT_LE_ADDR_TYPE_PUBLIC);

    ble_addr_type_t type = bt_device_get_address_type(ins, &addr);
    assert_int_equal(type, BT_LE_ADDR_TYPE_PUBLIC);
}

void test_bt_device_get_address_type_unknown(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_le_remote_address_type, addr);
    will_return(adapter_get_le_remote_address_type, BT_LE_ADDR_TYPE_UNKNOWN);

    ble_addr_type_t type = bt_device_get_address_type(ins, &addr);
    assert_int_equal(type, BT_LE_ADDR_TYPE_UNKNOWN);
}

/* bt_device_get_device_type */

void test_bt_device_get_device_type_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_device_type, addr);
    will_return(adapter_get_remote_device_type, BT_DEVICE_DEVTYPE_DUAL);

    bt_device_type_t type = bt_device_get_device_type(ins, &addr);
    assert_int_equal(type, BT_DEVICE_DEVTYPE_DUAL);
}

void test_bt_device_get_device_type_not_found(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_device_type, addr);
    will_return(adapter_get_remote_device_type, 0);

    bt_device_type_t type = bt_device_get_device_type(ins, &addr);
    assert_int_equal(type, 0);
}

/* bt_device_get_name */

void test_bt_device_get_name_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    char name[64];

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_name, addr);
    will_return(adapter_get_remote_name, true);

    bool ret = bt_device_get_name(ins, &addr, name, sizeof(name));
    assert_true(ret);
    assert_string_equal(name, "TestDevice");
}

void test_bt_device_get_name_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    char name[64];

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_name, addr);
    will_return(adapter_get_remote_name, false);

    bool ret = bt_device_get_name(ins, &addr, name, sizeof(name));
    assert_false(ret);
}

/* bt_device_get_device_class */

void test_bt_device_get_device_class_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_device_class, addr);
    will_return(adapter_get_remote_device_class, 0x200408);

    uint32_t cod = bt_device_get_device_class(ins, &addr);
    assert_int_equal(cod, 0x200408);
}

void test_bt_device_get_device_class_not_found(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_device_class, addr);
    will_return(adapter_get_remote_device_class, 0);

    uint32_t cod = bt_device_get_device_class(ins, &addr);
    assert_int_equal(cod, 0);
}

/* bt_device_get_uuids */

void test_bt_device_get_uuids_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    bt_uuid_t *uuids = NULL;
    uint16_t size = 0;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_uuids, addr);
    will_return(adapter_get_remote_uuids, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_get_uuids(ins, &addr, &uuids, &size, NULL);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
    assert_int_equal(size, 0);
}

void test_bt_device_get_uuids_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    bt_uuid_t *uuids = NULL;
    uint16_t size = 0;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_uuids, addr);
    will_return(adapter_get_remote_uuids, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_get_uuids(ins, &addr, &uuids, &size, NULL);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_get_appearance */

void test_bt_device_get_appearance_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_appearance, addr);
    will_return(adapter_get_remote_appearance, 0x0040);

    uint16_t appearance = bt_device_get_appearance(ins, &addr);
    assert_int_equal(appearance, 0x0040);
}

void test_bt_device_get_appearance_zero(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_appearance, addr);
    will_return(adapter_get_remote_appearance, 0);

    uint16_t appearance = bt_device_get_appearance(ins, &addr);
    assert_int_equal(appearance, 0);
}

/* bt_device_get_rssi */

void test_bt_device_get_rssi_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_rssi, addr);
    will_return(adapter_get_remote_rssi, -50);

    int8_t rssi = bt_device_get_rssi(ins, &addr);
    assert_int_equal(rssi, -50);
}

void test_bt_device_get_rssi_zero(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_rssi, addr);
    will_return(adapter_get_remote_rssi, 0);

    int8_t rssi = bt_device_get_rssi(ins, &addr);
    assert_int_equal(rssi, 0);
}

/* bt_device_get_alias */

void test_bt_device_get_alias_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    char alias[64];

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_alias, addr);
    will_return(adapter_get_remote_alias, true);

    bool ret = bt_device_get_alias(ins, &addr, alias, sizeof(alias));
    assert_true(ret);
    assert_string_equal(alias, "TestAlias");
}

void test_bt_device_get_alias_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    char alias[64];

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_alias, addr);
    will_return(adapter_get_remote_alias, false);

    bool ret = bt_device_get_alias(ins, &addr, alias, sizeof(alias));
    assert_false(ret);
}

/* bt_device_set_alias */

void test_bt_device_set_alias_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_set_remote_alias, addr);
    will_return(adapter_set_remote_alias, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_alias(ins, &addr, "MyAlias");
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_alias_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_set_remote_alias, addr);
    will_return(adapter_set_remote_alias, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_alias(ins, &addr, "BadAlias");
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_is_connected */

void test_bt_device_is_connected_true(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_is_remote_connected, addr);
    will_return(adapter_is_remote_connected, true);

    assert_true(bt_device_is_connected(ins, &addr, BT_TRANSPORT_BREDR));
}

void test_bt_device_is_connected_false(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_is_remote_connected, addr);
    will_return(adapter_is_remote_connected, false);

    assert_false(bt_device_is_connected(ins, &addr, BT_TRANSPORT_BREDR));
}

/* bt_device_is_encrypted */

void test_bt_device_is_encrypted_true(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_is_remote_encrypted, addr);
    will_return(adapter_is_remote_encrypted, true);

    assert_true(bt_device_is_encrypted(ins, &addr, BT_TRANSPORT_BREDR));
}

void test_bt_device_is_encrypted_false(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_is_remote_encrypted, addr);
    will_return(adapter_is_remote_encrypted, false);

    assert_false(bt_device_is_encrypted(ins, &addr, BT_TRANSPORT_BREDR));
}

/* bt_device_is_bond_initiate_local */

void test_bt_device_is_bond_initiate_local_true(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_is_bond_initiate_local, addr);
    will_return(adapter_is_bond_initiate_local, true);

    assert_true(bt_device_is_bond_initiate_local(ins, &addr,
        BT_TRANSPORT_BREDR));
}

void test_bt_device_is_bond_initiate_local_false(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_is_bond_initiate_local, addr);
    will_return(adapter_is_bond_initiate_local, false);

    assert_false(bt_device_is_bond_initiate_local(ins, &addr,
        BT_TRANSPORT_BREDR));
}

/* bt_device_get_bond_state */

void test_bt_device_get_bond_state_bonded(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_get_remote_bond_state, addr);
    will_return(adapter_get_remote_bond_state, BOND_STATE_BONDED);

    bond_state_t s = bt_device_get_bond_state(ins, &addr,
        BT_TRANSPORT_BREDR);
    assert_int_equal(s, BOND_STATE_BONDED);
}

void test_bt_device_get_bond_state_none(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_get_remote_bond_state, addr);
    will_return(adapter_get_remote_bond_state, BOND_STATE_NONE);

    bond_state_t s = bt_device_get_bond_state(ins, &addr,
        BT_TRANSPORT_BREDR);
    assert_int_equal(s, BOND_STATE_NONE);
}

/* bt_device_is_bonded */

void test_bt_device_is_bonded_true(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_is_remote_bonded, addr);
    will_return(adapter_is_remote_bonded, true);

    assert_true(bt_device_is_bonded(ins, &addr, BT_TRANSPORT_BREDR));
}

void test_bt_device_is_bonded_false(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_is_remote_bonded, addr);
    will_return(adapter_is_remote_bonded, false);

    assert_false(bt_device_is_bonded(ins, &addr, BT_TRANSPORT_BREDR));
}

/* bt_device_connect */

void test_bt_device_connect_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_connect, addr);
    will_return(adapter_connect, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_connect(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_connect_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_connect, addr);
    will_return(adapter_connect, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_connect(ins, &addr);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_disconnect */

void test_bt_device_disconnect_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_disconnect, addr);
    will_return(adapter_disconnect, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_disconnect(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_disconnect_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_disconnect, addr);
    will_return(adapter_disconnect, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_disconnect(ins, &addr);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_connect_le */

void test_bt_device_connect_le_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    ble_connect_params_t param;

    memset(&addr, 0x11, sizeof(addr));
    memset(&param, 0, sizeof(param));
    expect_any(adapter_le_connect, addr);
    will_return(adapter_le_connect, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_connect_le(ins, &addr,
        BT_LE_ADDR_TYPE_PUBLIC, &param);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_connect_le_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    ble_connect_params_t param;

    memset(&addr, 0x22, sizeof(addr));
    memset(&param, 0, sizeof(param));
    expect_any(adapter_le_connect, addr);
    will_return(adapter_le_connect, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_connect_le(ins, &addr,
        BT_LE_ADDR_TYPE_PUBLIC, &param);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_disconnect_le */

void test_bt_device_disconnect_le_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_le_disconnect, addr);
    will_return(adapter_le_disconnect, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_disconnect_le(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_disconnect_le_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_le_disconnect, addr);
    will_return(adapter_le_disconnect, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_disconnect_le(ins, &addr);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_connect_request_reply */

void test_bt_device_connect_request_reply_accept(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_connect_request_reply, addr);
    expect_value(adapter_connect_request_reply, accept, true);
    will_return(adapter_connect_request_reply, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_connect_request_reply(ins, &addr, true);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_connect_request_reply_reject(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_connect_request_reply, addr);
    expect_value(adapter_connect_request_reply, accept, false);
    will_return(adapter_connect_request_reply, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_connect_request_reply(ins, &addr, false);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

/* bt_device_connect_all_profile / bt_device_disconnect_all_profile */

void test_bt_device_connect_all_profile_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    bt_device_connect_all_profile(ins, &addr);
}

void test_bt_device_disconnect_all_profile_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    bt_device_disconnect_all_profile(ins, &addr);
}

/* bt_device_set_le_phy */

void test_bt_device_set_le_phy_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_le_set_phy, addr);
    will_return(adapter_le_set_phy, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_le_phy(ins, &addr, 0x01, 0x01);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_le_phy_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_le_set_phy, addr);
    will_return(adapter_le_set_phy, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_le_phy(ins, &addr, 0x02, 0x02);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_create_bond */

void test_bt_device_create_bond_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_create_bond, addr);
    will_return(adapter_create_bond, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_create_bond(ins, &addr, BT_TRANSPORT_BREDR);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_create_bond_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_create_bond, addr);
    will_return(adapter_create_bond, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_create_bond(ins, &addr, BT_TRANSPORT_BREDR);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_set_security_level */

void test_bt_device_set_security_level_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_set_security_level, level, 4);
    will_return(adapter_set_security_level, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_security_level(ins, 4,
        BT_TRANSPORT_BREDR);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_security_level_fail(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_set_security_level, level, 0);
    will_return(adapter_set_security_level, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_security_level(ins, 0,
        BT_TRANSPORT_BREDR);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_set_bondable_le */

void test_bt_device_set_bondable_le_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_le_set_bondable, bondable, true);
    will_return(adapter_le_set_bondable, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_bondable_le(ins, true);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_bondable_le_fail(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_le_set_bondable, bondable, false);
    will_return(adapter_le_set_bondable, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_bondable_le(ins, false);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_remove_bond */

void test_bt_device_remove_bond_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_remove_bond, addr);
    will_return(adapter_remove_bond, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_remove_bond(ins, &addr, BT_TRANSPORT_BREDR);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_remove_bond_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_remove_bond, addr);
    will_return(adapter_remove_bond, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_remove_bond(ins, &addr, BT_TRANSPORT_BREDR);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_cancel_bond */

void test_bt_device_cancel_bond_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_cancel_bond, addr);
    will_return(adapter_cancel_bond, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_cancel_bond(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_cancel_bond_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_cancel_bond, addr);
    will_return(adapter_cancel_bond, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_cancel_bond(ins, &addr);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_pair_request_reply */

void test_bt_device_pair_request_reply_accept(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_pair_request_reply, addr);
    expect_value(adapter_pair_request_reply, accept, true);
    will_return(adapter_pair_request_reply, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_pair_request_reply(ins, &addr, true);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_pair_request_reply_reject(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_pair_request_reply, addr);
    expect_value(adapter_pair_request_reply, accept, false);
    will_return(adapter_pair_request_reply, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_pair_request_reply(ins, &addr, false);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

/* bt_device_set_pairing_confirmation */

void test_bt_device_set_pairing_confirmation_accept(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_set_pairing_confirmation, addr);
    will_return(adapter_set_pairing_confirmation, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_pairing_confirmation(ins, &addr,
        BT_TRANSPORT_BREDR, true);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_pairing_confirmation_reject(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_set_pairing_confirmation, addr);
    will_return(adapter_set_pairing_confirmation, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_pairing_confirmation(ins, &addr,
        BT_TRANSPORT_BREDR, false);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_set_pin_code */

void test_bt_device_set_pin_code_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    char pin[] = "1234";

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_set_pin_code, addr);
    will_return(adapter_set_pin_code, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_pin_code(ins, &addr, true,
        pin, strlen(pin));
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_pin_code_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    char pin[] = "0000";

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_set_pin_code, addr);
    will_return(adapter_set_pin_code, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_pin_code(ins, &addr, false,
        pin, strlen(pin));
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_set_pass_key */

void test_bt_device_set_pass_key_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_set_pass_key, addr);
    will_return(adapter_set_pass_key, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_pass_key(ins, &addr,
        BT_TRANSPORT_BREDR, true, 123456);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_pass_key_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_set_pass_key, addr);
    will_return(adapter_set_pass_key, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_pass_key(ins, &addr,
        BT_TRANSPORT_BREDR, false, 0);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_set_le_legacy_tk */

void test_bt_device_set_le_legacy_tk_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    bt_128key_t tk_val;

    memset(&addr, 0x11, sizeof(addr));
    memset(tk_val, 0xAA, sizeof(tk_val));
    expect_any(adapter_le_set_legacy_tk, addr);
    will_return(adapter_le_set_legacy_tk, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_le_legacy_tk(ins, &addr, tk_val);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_le_legacy_tk_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    bt_128key_t tk_val;

    memset(&addr, 0x22, sizeof(addr));
    memset(tk_val, 0, sizeof(tk_val));
    expect_any(adapter_le_set_legacy_tk, addr);
    will_return(adapter_le_set_legacy_tk, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_le_legacy_tk(ins, &addr, tk_val);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_set_le_sc_remote_oob_data */

void test_bt_device_set_le_sc_remote_oob_data_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    bt_128key_t c_val;
    bt_128key_t r_val;

    memset(&addr, 0x11, sizeof(addr));
    memset(c_val, 0xBB, sizeof(c_val));
    memset(r_val, 0xCC, sizeof(r_val));
    expect_any(adapter_le_set_remote_oob_data, addr);
    will_return(adapter_le_set_remote_oob_data, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_set_le_sc_remote_oob_data(ins, &addr,
        c_val, r_val);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_set_le_sc_remote_oob_data_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    bt_128key_t c_val;
    bt_128key_t r_val;

    memset(&addr, 0x22, sizeof(addr));
    memset(c_val, 0, sizeof(c_val));
    memset(r_val, 0, sizeof(r_val));
    expect_any(adapter_le_set_remote_oob_data, addr);
    will_return(adapter_le_set_remote_oob_data, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_set_le_sc_remote_oob_data(ins, &addr,
        c_val, r_val);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_get_le_sc_local_oob_data */

void test_bt_device_get_le_sc_local_oob_data_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(adapter_le_get_local_oob_data, addr);
    will_return(adapter_le_get_local_oob_data, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_device_get_le_sc_local_oob_data(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_device_get_le_sc_local_oob_data_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(adapter_le_get_local_oob_data, addr);
    will_return(adapter_le_get_local_oob_data, BT_STATUS_FAIL);

    bt_status_t ret = bt_device_get_le_sc_local_oob_data(ins, &addr);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

/* bt_device_enable_enhanced_mode / bt_device_disable_enhanced_mode */

void test_bt_device_enable_enhanced_mode_unsupported(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));

    bt_status_t ret = bt_device_enable_enhanced_mode(ins, &addr, 0);
    assert_int_equal(ret, BT_STATUS_UNSUPPORTED);
}

void test_bt_device_disable_enhanced_mode_unsupported(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));

    bt_status_t ret = bt_device_disable_enhanced_mode(ins, &addr, 0);
    assert_int_equal(ret, BT_STATUS_UNSUPPORTED);
}
