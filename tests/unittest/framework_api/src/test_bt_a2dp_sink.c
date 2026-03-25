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

#include "a2dp_sink_service.h"
#include "bt_a2dp_sink.h"
#include "bt_status.h"
#include "mock_service_manager.h"

#include "cm_bt_a2dp_sink.h"

/****************************************************************************
 * Private Functions - Mock profile interface
 ****************************************************************************/

static void* g_mock_cookie = (void*)0xA2D0; /* arbitrary sentinel (A2D0 ~ A2DP) */

static void* mock_register_callbacks(void* remote,
    const a2dp_sink_callbacks_t* cbs)
{
    return mock_ptr_type(void*);
}

static bool mock_unregister_callbacks(void** remote, void* cookie)
{
    return mock_type(bool);
}

static bool mock_is_connected(bt_address_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(bool);
}

static bool mock_is_playing(bt_address_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(bool);
}

static profile_connection_state_t mock_get_connection_state(bt_address_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(profile_connection_state_t);
}

static bt_status_t mock_connect(bt_address_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

static bt_status_t mock_disconnect(bt_address_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

static bt_status_t mock_set_active_device(bt_address_t* addr)
{
    check_expected_ptr(addr);
    return mock_type(bt_status_t);
}

static a2dp_sink_interface_t g_mock_interface = {
    .register_callbacks = mock_register_callbacks,
    .unregister_callbacks = mock_unregister_callbacks,
    .is_connected = mock_is_connected,
    .is_playing = mock_is_playing,
    .get_connection_state = mock_get_connection_state,
    .connect = mock_connect,
    .disconnect = mock_disconnect,
    .set_active_device = mock_set_active_device,
};

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_a2dp_sink_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_A2DP_SINK, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_a2dp_sink_teardown(FAR void** state)
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

void test_bt_a2dp_sink_register_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    a2dp_sink_callbacks_t cbs;

    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_callbacks, g_mock_cookie);

    void* cookie = bt_a2dp_sink_register_callbacks(ins, &cbs);
    assert_ptr_equal(cookie, g_mock_cookie);
}

void test_bt_a2dp_sink_register_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;

    will_return(mock_register_callbacks, NULL);

    void* cookie = bt_a2dp_sink_register_callbacks(ins, NULL);
    assert_null(cookie);
}

void test_bt_a2dp_sink_unregister_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;

    will_return(mock_unregister_callbacks, true);

    bool ret = bt_a2dp_sink_unregister_callbacks(ins, g_mock_cookie);
    assert_true(ret);
}

void test_bt_a2dp_sink_unregister_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;

    will_return(mock_unregister_callbacks, false);

    bool ret = bt_a2dp_sink_unregister_callbacks(ins, NULL);
    assert_false(ret);
}

void test_bt_a2dp_sink_is_connected_true(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(mock_is_connected, addr);
    will_return(mock_is_connected, true);

    assert_true(bt_a2dp_sink_is_connected(ins, &addr));
}

void test_bt_a2dp_sink_is_connected_false(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(mock_is_connected, addr);
    will_return(mock_is_connected, false);

    assert_false(bt_a2dp_sink_is_connected(ins, &addr));
}

void test_bt_a2dp_sink_is_playing_true(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(mock_is_playing, addr);
    will_return(mock_is_playing, true);

    assert_true(bt_a2dp_sink_is_playing(ins, &addr));
}

void test_bt_a2dp_sink_is_playing_false(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(mock_is_playing, addr);
    will_return(mock_is_playing, false);

    assert_false(bt_a2dp_sink_is_playing(ins, &addr));
}

void test_bt_a2dp_sink_get_connection_state_connected(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(mock_get_connection_state, addr);
    will_return(mock_get_connection_state, PROFILE_STATE_CONNECTED);

    profile_connection_state_t s
        = bt_a2dp_sink_get_connection_state(ins, &addr);
    assert_int_equal(s, PROFILE_STATE_CONNECTED);
}

void test_bt_a2dp_sink_get_connection_state_disconnected(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(mock_get_connection_state, addr);
    will_return(mock_get_connection_state, PROFILE_STATE_DISCONNECTED);

    profile_connection_state_t s
        = bt_a2dp_sink_get_connection_state(ins, &addr);
    assert_int_equal(s, PROFILE_STATE_DISCONNECTED);
}

void test_bt_a2dp_sink_connect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(mock_connect, addr);
    will_return(mock_connect, BT_STATUS_SUCCESS);

    assert_int_equal(bt_a2dp_sink_connect(ins, &addr), BT_STATUS_SUCCESS);
}

void test_bt_a2dp_sink_connect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(mock_connect, addr);
    will_return(mock_connect, BT_STATUS_FAIL);

    assert_int_equal(bt_a2dp_sink_connect(ins, &addr), BT_STATUS_FAIL);
}

void test_bt_a2dp_sink_disconnect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(mock_disconnect, addr);
    will_return(mock_disconnect, BT_STATUS_SUCCESS);

    assert_int_equal(bt_a2dp_sink_disconnect(ins, &addr), BT_STATUS_SUCCESS);
}

void test_bt_a2dp_sink_disconnect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(mock_disconnect, addr);
    will_return(mock_disconnect, BT_STATUS_FAIL);

    assert_int_equal(bt_a2dp_sink_disconnect(ins, &addr), BT_STATUS_FAIL);
}

void test_bt_a2dp_sink_set_active_device_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x11, sizeof(addr));
    expect_any(mock_set_active_device, addr);
    will_return(mock_set_active_device, BT_STATUS_SUCCESS);

    assert_int_equal(
        bt_a2dp_sink_set_active_device(ins, &addr), BT_STATUS_SUCCESS);
}

void test_bt_a2dp_sink_set_active_device_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;

    memset(&addr, 0x22, sizeof(addr));
    expect_any(mock_set_active_device, addr);
    will_return(mock_set_active_device, BT_STATUS_FAIL);

    assert_int_equal(
        bt_a2dp_sink_set_active_device(ins, &addr), BT_STATUS_FAIL);
}
