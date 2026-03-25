/****************************************************************************
 * tests/unittest/framework_api/src/test_bt_adapter.c
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

#include "cm_bt_adapter.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/


/****************************************************************************
 * Private Functions - Mock adapter functions
 ****************************************************************************/

static void *g_mock_register_cookie = (void *)0xDEAD;

void *adapter_register_callback(void *remote, const void *cbs)
{
    check_expected_ptr(cbs);
    return mock_ptr_type(void *);
}

bool adapter_unregister_callback(void *remote, void *cookie)
{
    check_expected_ptr(cookie);
    return mock_type(bool);
}

bt_status_t adapter_enable(int mode)
{
    check_expected(mode);
    return mock_type(bt_status_t);
}

bt_status_t adapter_disable(int mode)
{
    check_expected(mode);
    return mock_type(bt_status_t);
}

bt_status_t adapter_disable_safe(int mode)
{
    check_expected(mode);
    return mock_type(bt_status_t);
}

bt_adapter_state_t adapter_get_state(void)
{
    return mock_type(bt_adapter_state_t);
}

bool adapter_is_le_enabled(void)
{
    return mock_type(bool);
}

bt_device_type_t adapter_get_type(void)
{
    return mock_type(bt_device_type_t);
}

bt_status_t adapter_start_discovery(uint32_t timeout, bool limited)
{
    check_expected(timeout);
    check_expected(limited);
    return mock_type(bt_status_t);
}

bt_status_t adapter_cancel_discovery(void)
{
    return mock_type(bt_status_t);
}

bool adapter_is_discovering(void)
{
    return mock_type(bool);
}

void adapter_get_address(bt_address_t *addr)
{
    uint8_t mock_addr[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    memcpy(addr->addr, mock_addr, 6);
}

bt_status_t adapter_set_name(const char *name)
{
    check_expected_ptr(name);
    return mock_type(bt_status_t);
}

void adapter_get_name(char *name, int length)
{
    strncpy(name, "TestAdapter", length);
}

bt_status_t adapter_get_uuids(bt_uuid_t *uuids, uint16_t *size)
{
    *size = 0;
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_scan_mode(bt_scan_mode_t mode, bool bondable)
{
    check_expected(mode);
    return mock_type(bt_status_t);
}

bt_scan_mode_t adapter_get_scan_mode(void)
{
    return mock_type(bt_scan_mode_t);
}

bt_status_t adapter_set_device_class(uint32_t cod)
{
    check_expected(cod);
    return mock_type(bt_status_t);
}

uint32_t adapter_get_device_class(void)
{
    return mock_type(uint32_t);
}

bt_status_t adapter_set_io_capability(bt_io_capability_t cap)
{
    return mock_type(bt_status_t);
}

bt_io_capability_t adapter_get_io_capability(void)
{
    return mock_type(bt_io_capability_t);
}

bt_status_t adapter_set_inquiry_scan_parameters(bt_scan_type_t type,
    uint16_t interval, uint16_t window)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_page_scan_parameters(bt_scan_type_t type,
    uint16_t interval, uint16_t window)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_le_io_capability(uint32_t le_io_cap)
{
    return mock_type(bt_status_t);
}

uint32_t adapter_get_le_io_capability(void)
{
    return mock_type(uint32_t);
}

bt_status_t adapter_get_le_address(bt_address_t *addr, ble_addr_type_t *type)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_le_address(bt_address_t *addr)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_le_identity_address(bt_address_t *addr, bool is_public)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_le_appearance(uint16_t appearance)
{
    return mock_type(bt_status_t);
}

uint16_t adapter_get_le_appearance(void)
{
    return mock_type(uint16_t);
}

bt_status_t adapter_le_enable_key_derivation(bool brkey_to_lekey,
    bool lekey_to_brkey)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_add_whitelist_with_type(bt_address_t *addr,
    ble_addr_type_t type)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_le_remove_whitelist(bt_address_t *addr)
{
    return mock_type(bt_status_t);
}

bt_status_t adapter_get_bonded_devices(bt_transport_t transport,
    bt_address_t **addr, int *num, bt_allocator_t allocator)
{
    *num = 0;
    return mock_type(bt_status_t);
}

bt_status_t adapter_get_connected_devices(bt_transport_t transport,
    bt_address_t **addr, int *num, bt_allocator_t allocator)
{
    *num = 0;
    return mock_type(bt_status_t);
}

bt_status_t adapter_set_afh_channel_classification(uint16_t central_frequency,
    uint16_t band_width, uint16_t number)
{
    return mock_type(bt_status_t);
}

bool adapter_is_support_bredr(void)
{
    return mock_type(bool);
}

bool adapter_is_support_le(void)
{
    return mock_type(bool);
}

bool adapter_is_support_leaudio(void)
{
    return mock_type(bool);
}

bt_status_t adapter_set_debug_mode(bt_debug_mode_t mode, uint8_t operation)
{
    return mock_type(bt_status_t);
}

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_adapter_setup(FAR void **state)
{
    bt_instance_t *ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    *state = ins;
    return 0;
}

int test_bt_adapter_teardown(FAR void **state)
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

void test_bt_adapter_register_callback_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    adapter_callbacks_t cbs;

    memset(&cbs, 0, sizeof(cbs));
    expect_any(adapter_register_callback, cbs);
    will_return(adapter_register_callback, g_mock_register_cookie);

    void *cookie = bt_adapter_register_callback(ins, &cbs);
    assert_ptr_equal(cookie, g_mock_register_cookie);
}

void test_bt_adapter_register_callback_null_cbs(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_register_callback, cbs);
    will_return(adapter_register_callback, NULL);

    void *cookie = bt_adapter_register_callback(ins, NULL);
    assert_null(cookie);
}

void test_bt_adapter_unregister_callback_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_unregister_callback, cookie);
    will_return(adapter_unregister_callback, true);

    bool ret = bt_adapter_unregister_callback(ins, g_mock_register_cookie);
    assert_true(ret);
}

void test_bt_adapter_unregister_callback_null_cookie(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_unregister_callback, cookie);
    will_return(adapter_unregister_callback, false);

    bool ret = bt_adapter_unregister_callback(ins, NULL);
    assert_false(ret);
}

void test_bt_adapter_enable_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_enable, mode, 0x03);
    will_return(adapter_enable, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_enable(ins);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_enable_fail(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_enable, mode, 0x03);
    will_return(adapter_enable, BT_STATUS_FAIL);

    bt_status_t ret = bt_adapter_enable(ins);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

void test_bt_adapter_disable_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_disable, mode, 0x03);
    will_return(adapter_disable, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_disable(ins);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_disable_fail(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_disable, mode, 0x03);
    will_return(adapter_disable, BT_STATUS_FAIL);

    bt_status_t ret = bt_adapter_disable(ins);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

void test_bt_adapter_disable_safe_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_disable_safe, mode, 0x03);
    will_return(adapter_disable_safe, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_disable_safe(ins);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_enable_le_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_enable, mode, 0x04);
    will_return(adapter_enable, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_enable_le(ins);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_disable_le_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_disable, mode, 0x04);
    will_return(adapter_disable, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_disable_le(ins);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_get_state_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_state, BT_ADAPTER_STATE_ON);

    bt_adapter_state_t s = bt_adapter_get_state(ins);
    assert_int_equal(s, BT_ADAPTER_STATE_ON);
}

void test_bt_adapter_get_state_disabled(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_state, BT_ADAPTER_STATE_OFF);

    bt_adapter_state_t s = bt_adapter_get_state(ins);
    assert_int_equal(s, BT_ADAPTER_STATE_OFF);
}

void test_bt_adapter_is_le_enabled_true(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_le_enabled, true);
    assert_true(bt_adapter_is_le_enabled(ins));
}

void test_bt_adapter_is_le_enabled_false(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_le_enabled, false);
    assert_false(bt_adapter_is_le_enabled(ins));
}

void test_bt_adapter_get_type_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_type, BT_DEVICE_DEVTYPE_DUAL);

    bt_device_type_t t = bt_adapter_get_type(ins);
    assert_int_equal(t, BT_DEVICE_DEVTYPE_DUAL);
}

void test_bt_adapter_set_discovery_filter_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    bt_status_t ret = bt_adapter_set_discovery_filter(ins);
    assert_int_equal(ret, 0);
}

void test_bt_adapter_start_discovery_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_start_discovery, timeout, 10000);
    expect_value(adapter_start_discovery, limited, false);
    will_return(adapter_start_discovery, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_start_discovery(ins, 10000);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_start_discovery_fail(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_start_discovery, timeout, 5000);
    expect_value(adapter_start_discovery, limited, false);
    will_return(adapter_start_discovery, BT_STATUS_BUSY);

    bt_status_t ret = bt_adapter_start_discovery(ins, 5000);
    assert_int_equal(ret, BT_STATUS_BUSY);
}

void test_bt_adapter_start_discovery_zero_timeout(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_start_discovery, timeout, 0);
    expect_value(adapter_start_discovery, limited, false);
    will_return(adapter_start_discovery, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_start_discovery(ins, 0);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_start_limited_discovery_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_start_discovery, timeout, 5000);
    expect_value(adapter_start_discovery, limited, true);
    will_return(adapter_start_discovery, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_start_limited_discovery(ins, 5000);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_cancel_discovery_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_cancel_discovery, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_cancel_discovery(ins);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_cancel_discovery_fail(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_cancel_discovery, BT_STATUS_FAIL);

    bt_status_t ret = bt_adapter_cancel_discovery(ins);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

void test_bt_adapter_is_discovering_true(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_discovering, true);
    assert_true(bt_adapter_is_discovering(ins));
}

void test_bt_adapter_is_discovering_false(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_discovering, false);
    assert_false(bt_adapter_is_discovering(ins));
}

void test_bt_adapter_get_address_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    bt_adapter_get_address(ins, &addr);
    assert_int_equal(addr.addr[0], 0x11);
    assert_int_equal(addr.addr[5], 0x66);
}

void test_bt_adapter_set_name_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_set_name, name);
    will_return(adapter_set_name, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_name(ins, "MyDevice");
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_set_name_null(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_set_name, name);
    will_return(adapter_set_name, BT_STATUS_PARM_INVALID);

    bt_status_t ret = bt_adapter_set_name(ins, NULL);
    assert_int_equal(ret, BT_STATUS_PARM_INVALID);
}

void test_bt_adapter_set_name_empty(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_set_name, name);
    will_return(adapter_set_name, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_name(ins, "");
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_get_name_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    char name[64];

    bt_adapter_get_name(ins, name, sizeof(name));
    assert_string_equal(name, "TestAdapter");
}

void test_bt_adapter_get_uuids_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_uuid_t uuids[10];
    uint16_t size = 10;

    will_return(adapter_get_uuids, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_get_uuids(ins, uuids, &size);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
    assert_int_equal(size, 0);
}

void test_bt_adapter_set_scan_mode_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_set_scan_mode, mode);
    will_return(adapter_set_scan_mode, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_scan_mode(ins,
        BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_set_scan_mode_fail(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(adapter_set_scan_mode, mode);
    will_return(adapter_set_scan_mode, BT_STATUS_FAIL);

    bt_status_t ret = bt_adapter_set_scan_mode(ins,
        BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
    assert_int_equal(ret, BT_STATUS_FAIL);
}

void test_bt_adapter_get_scan_mode_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_scan_mode, BT_SCAN_MODE_CONNECTABLE);

    bt_scan_mode_t mode = bt_adapter_get_scan_mode(ins);
    assert_int_equal(mode, BT_SCAN_MODE_CONNECTABLE);
}

void test_bt_adapter_set_device_class_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(adapter_set_device_class, cod, 0x200408);
    will_return(adapter_set_device_class, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_device_class(ins, 0x200408);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_get_device_class_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_device_class, 0x200408);

    uint32_t cod = bt_adapter_get_device_class(ins);
    assert_int_equal(cod, 0x200408);
}

void test_bt_adapter_set_io_capability_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_set_io_capability, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_io_capability(ins, 0);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_get_io_capability_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_io_capability, 0x03);

    bt_io_capability_t cap = bt_adapter_get_io_capability(ins);
    assert_int_equal(cap, 0x03);
}

void test_bt_adapter_set_inquiry_scan_params_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_set_inquiry_scan_parameters, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_inquiry_scan_parameters(ins, 0, 0x100, 0x12);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_set_page_scan_params_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_set_page_scan_parameters, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_page_scan_parameters(ins, 0, 0x100, 0x12);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_set_le_io_capability_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_set_le_io_capability, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_le_io_capability(ins, 0x03);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_get_le_io_capability_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_le_io_capability, 0x03);

    uint32_t cap = bt_adapter_get_le_io_capability(ins);
    assert_int_equal(cap, 0x03);
}

void test_bt_adapter_get_le_address_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;
    ble_addr_type_t type;

    will_return(adapter_get_le_address, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_get_le_address(ins, &addr, &type);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_set_le_address_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    will_return(adapter_set_le_address, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_le_address(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_set_le_identity_address_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    will_return(adapter_set_le_identity_address, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_le_identity_address(ins, &addr, true);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_set_le_appearance_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_set_le_appearance, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_le_appearance(ins, 0x0040);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_get_le_appearance_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_get_le_appearance, 0x0040);

    uint16_t appearance = bt_adapter_get_le_appearance(ins);
    assert_int_equal(appearance, 0x0040);
}

void test_bt_adapter_le_enable_key_derivation_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_le_enable_key_derivation, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_le_enable_key_derivation(ins, true, false);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_le_add_whitelist_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x33, sizeof(addr));
    will_return(adapter_le_add_whitelist_with_type, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_le_add_whitelist(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_le_add_whitelist_with_type_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x44, sizeof(addr));
    will_return(adapter_le_add_whitelist_with_type, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_le_add_whitelist_with_type(ins, &addr, 0x01);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_le_remove_whitelist_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t addr;

    memset(&addr, 0x55, sizeof(addr));
    will_return(adapter_le_remove_whitelist, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_le_remove_whitelist(ins, &addr);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_get_bonded_devices_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t *addrs = NULL;
    int num = 0;

    will_return(adapter_get_bonded_devices, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_get_bonded_devices(ins,
        BT_TRANSPORT_BREDR, &addrs, &num, NULL);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
    assert_int_equal(num, 0);
}

void test_bt_adapter_get_connected_devices_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    bt_address_t *addrs = NULL;
    int num = 0;

    will_return(adapter_get_connected_devices, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_get_connected_devices(ins,
        BT_TRANSPORT_BREDR, &addrs, &num, NULL);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
    assert_int_equal(num, 0);
}

void test_bt_adapter_set_afh_channel_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_set_afh_channel_classification, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_afh_channel_classification(ins,
        2402, 80, 1);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

void test_bt_adapter_disconnect_all_devices_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    bt_adapter_disconnect_all_devices(ins);
}

void test_bt_adapter_is_support_bredr_true(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_support_bredr, true);
    assert_true(bt_adapter_is_support_bredr(ins));
}

void test_bt_adapter_is_support_bredr_false(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_support_bredr, false);
    assert_false(bt_adapter_is_support_bredr(ins));
}

void test_bt_adapter_is_support_le_true(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_support_le, true);
    assert_true(bt_adapter_is_support_le(ins));
}

void test_bt_adapter_is_support_le_false(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_support_le, false);
    assert_false(bt_adapter_is_support_le(ins));
}

void test_bt_adapter_is_support_leaudio_true(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_support_leaudio, true);
    assert_true(bt_adapter_is_support_leaudio(ins));
}

void test_bt_adapter_is_support_leaudio_false(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_is_support_leaudio, false);
    assert_false(bt_adapter_is_support_leaudio(ins));
}

void test_bt_adapter_set_debug_mode_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(adapter_set_debug_mode, BT_STATUS_SUCCESS);

    bt_status_t ret = bt_adapter_set_debug_mode(ins, 0, 1);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}
