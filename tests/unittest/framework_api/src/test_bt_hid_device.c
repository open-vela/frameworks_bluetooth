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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdlib.h>
#include <string.h>

#include "bt_hid_device.h"
#include "bt_status.h"
#include "hid_device_service.h"

#include "cm_bt_hid_device.h"
#include "mock_service_manager.h"

static void* g_mock_cookie = (void*)0x0D01;

static void* mock_register_callbacks(void* r, const hid_device_callbacks_t* c) { return mock_ptr_type(void*); }
static bool mock_unregister_callbacks(void** r, void* c) { return mock_type(bool); }

static bt_status_t mock_register_app(hid_device_sdp_settings_t* sdp, bool le) { return mock_type(bt_status_t); }
static bt_status_t mock_unregister_app(void) { return mock_type(bt_status_t); }
static bt_status_t mock_connect(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_disconnect(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_send_report(bt_address_t* a, uint8_t id, uint8_t* d, int s)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_response_report(bt_address_t* a, uint8_t t, uint8_t* d, int s)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_report_error(bt_address_t* a, hid_status_error_t e)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_virtual_unplug(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}

static hid_device_interface_t g_mock_interface = {
    .register_callbacks = mock_register_callbacks,
    .unregister_callbacks = mock_unregister_callbacks,
    .register_app = mock_register_app,
    .unregister_app = mock_unregister_app,
    .connect = mock_connect,
    .disconnect = mock_disconnect,
    .send_report = mock_send_report,
    .response_report = mock_response_report,
    .report_error = mock_report_error,
    .virtual_unplug = mock_virtual_unplug,
};

int test_bt_hid_device_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_HID_DEV, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_hid_device_teardown(FAR void** state)
{
    if (*state) {
        test_free(*state);
        *state = NULL;
    }
    return 0;
}

#define ADDR_INIT(a, v) \
    bt_address_t a;     \
    memset(&a, v, sizeof(a))

void test_bt_hid_device_register_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    hid_device_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_callbacks, g_mock_cookie);
    assert_ptr_equal(bt_hid_device_register_callbacks(ins, &cbs), g_mock_cookie);
}
void test_bt_hid_device_register_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_register_callbacks, NULL);
    assert_null(bt_hid_device_register_callbacks(ins, NULL));
}
void test_bt_hid_device_unregister_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_callbacks, true);
    assert_true(bt_hid_device_unregister_callbacks(ins, g_mock_cookie));
}
void test_bt_hid_device_unregister_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_callbacks, false);
    assert_false(bt_hid_device_unregister_callbacks(ins, NULL));
}

void test_bt_hid_device_register_app_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    hid_device_sdp_settings_t sdp;
    memset(&sdp, 0, sizeof(sdp));
    will_return(mock_register_app, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_register_app(ins, &sdp, false), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_register_app_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_register_app, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_register_app(ins, NULL, false), BT_STATUS_FAIL);
}
void test_bt_hid_device_unregister_app_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_app, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_unregister_app(ins), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_unregister_app_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_app, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_unregister_app(ins), BT_STATUS_FAIL);
}
void test_bt_hid_device_connect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_connect, a);
    will_return(mock_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_connect(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_connect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_connect, a);
    will_return(mock_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_connect(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hid_device_disconnect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_disconnect, a);
    will_return(mock_disconnect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_disconnect(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_disconnect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_disconnect, a);
    will_return(mock_disconnect, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_disconnect(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hid_device_send_report_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    uint8_t data[] = { 0x01, 0x02 };
    expect_any(mock_send_report, a);
    will_return(mock_send_report, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_send_report(ins, &addr, 1, data, 2), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_send_report_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_send_report, a);
    will_return(mock_send_report, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_send_report(ins, &addr, 1, NULL, 0), BT_STATUS_FAIL);
}
void test_bt_hid_device_response_report_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    uint8_t data[] = { 0x01 };
    expect_any(mock_response_report, a);
    will_return(mock_response_report, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_response_report(ins, &addr, 1, data, 1), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_response_report_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_response_report, a);
    will_return(mock_response_report, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_response_report(ins, &addr, 1, NULL, 0), BT_STATUS_FAIL);
}
void test_bt_hid_device_report_error_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_report_error, a);
    will_return(mock_report_error, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_report_error(ins, &addr, 0), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_report_error_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_report_error, a);
    will_return(mock_report_error, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_report_error(ins, &addr, 1), BT_STATUS_FAIL);
}
void test_bt_hid_device_virtual_unplug_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_virtual_unplug, a);
    will_return(mock_virtual_unplug, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hid_device_virtual_unplug(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hid_device_virtual_unplug_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_virtual_unplug, a);
    will_return(mock_virtual_unplug, BT_STATUS_FAIL);
    assert_int_equal(bt_hid_device_virtual_unplug(ins, &addr), BT_STATUS_FAIL);
}
