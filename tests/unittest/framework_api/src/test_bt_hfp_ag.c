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

#include "bt_hfp_ag.h"
#include "bt_status.h"
#include "hfp_ag_service.h"

#include "cm_bt_hfp_ag.h"
#include "mock_service_manager.h"

static void* g_mock_cookie = (void*)0xAE01;

static void* mock_register_callbacks(void* r, const hfp_ag_callbacks_t* c) { return mock_ptr_type(void*); }
static bool mock_unregister_callbacks(void** r, void* c) { return mock_type(bool); }
static bool mock_is_connected(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bool);
}
static bool mock_is_audio_connected(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bool);
}
static profile_connection_state_t mock_get_connection_state(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(profile_connection_state_t);
}
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
static bt_status_t mock_connect_audio(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_disconnect_audio(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_start_virtual_call(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_stop_virtual_call(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_start_voice_recognition(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_stop_voice_recognition(bt_address_t* a)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}

static bt_status_t mock_volume_control(bt_address_t* a, hfp_volume_type_t t, uint8_t v)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_send_at_command(bt_address_t* a, const char* cmd)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}

static hfp_ag_interface_t g_mock_interface = {
    .register_callbacks = mock_register_callbacks,
    .unregister_callbacks = mock_unregister_callbacks,
    .is_connected = mock_is_connected,
    .is_audio_connected = mock_is_audio_connected,
    .get_connection_state = mock_get_connection_state,
    .connect = mock_connect,
    .disconnect = mock_disconnect,
    .connect_audio = mock_connect_audio,
    .disconnect_audio = mock_disconnect_audio,
    .start_virtual_call = mock_start_virtual_call,
    .stop_virtual_call = mock_stop_virtual_call,
    .start_voice_recognition = mock_start_voice_recognition,
    .stop_voice_recognition = mock_stop_voice_recognition,
    .volume_control = mock_volume_control,
    .send_at_command = mock_send_at_command,
};

int test_bt_hfp_ag_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_HFP_AG, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_hfp_ag_teardown(FAR void** state)
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

void test_bt_hfp_ag_register_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    hfp_ag_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_callbacks, g_mock_cookie);
    assert_ptr_equal(bt_hfp_ag_register_callbacks(ins, &cbs), g_mock_cookie);
}
void test_bt_hfp_ag_register_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_register_callbacks, NULL);
    assert_null(bt_hfp_ag_register_callbacks(ins, NULL));
}
void test_bt_hfp_ag_unregister_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_callbacks, true);
    assert_true(bt_hfp_ag_unregister_callbacks(ins, g_mock_cookie));
}
void test_bt_hfp_ag_unregister_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_callbacks, false);
    assert_false(bt_hfp_ag_unregister_callbacks(ins, NULL));
}
void test_bt_hfp_ag_is_connected_true(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_is_connected, a);
    will_return(mock_is_connected, true);
    assert_true(bt_hfp_ag_is_connected(ins, &addr));
}
void test_bt_hfp_ag_is_connected_false(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_is_connected, a);
    will_return(mock_is_connected, false);
    assert_false(bt_hfp_ag_is_connected(ins, &addr));
}
void test_bt_hfp_ag_is_audio_connected_true(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_is_audio_connected, a);
    will_return(mock_is_audio_connected, true);
    assert_true(bt_hfp_ag_is_audio_connected(ins, &addr));
}
void test_bt_hfp_ag_is_audio_connected_false(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_is_audio_connected, a);
    will_return(mock_is_audio_connected, false);
    assert_false(bt_hfp_ag_is_audio_connected(ins, &addr));
}
void test_bt_hfp_ag_get_connection_state_connected(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_get_connection_state, a);
    will_return(mock_get_connection_state, PROFILE_STATE_CONNECTED);
    assert_int_equal(bt_hfp_ag_get_connection_state(ins, &addr), PROFILE_STATE_CONNECTED);
}
void test_bt_hfp_ag_get_connection_state_disconnected(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_get_connection_state, a);
    will_return(mock_get_connection_state, PROFILE_STATE_DISCONNECTED);
    assert_int_equal(bt_hfp_ag_get_connection_state(ins, &addr), PROFILE_STATE_DISCONNECTED);
}

void test_bt_hfp_ag_connect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_connect, a);
    will_return(mock_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_connect(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_connect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_connect, a);
    will_return(mock_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_connect(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_disconnect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_disconnect, a);
    will_return(mock_disconnect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_disconnect(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_disconnect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_disconnect, a);
    will_return(mock_disconnect, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_disconnect(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_connect_audio_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_connect_audio, a);
    will_return(mock_connect_audio, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_connect_audio(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_connect_audio_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_connect_audio, a);
    will_return(mock_connect_audio, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_connect_audio(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_disconnect_audio_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_disconnect_audio, a);
    will_return(mock_disconnect_audio, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_disconnect_audio(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_disconnect_audio_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_disconnect_audio, a);
    will_return(mock_disconnect_audio, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_disconnect_audio(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_start_virtual_call_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_start_virtual_call, a);
    will_return(mock_start_virtual_call, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_start_virtual_call(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_start_virtual_call_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_start_virtual_call, a);
    will_return(mock_start_virtual_call, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_start_virtual_call(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_stop_virtual_call_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_stop_virtual_call, a);
    will_return(mock_stop_virtual_call, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_stop_virtual_call(ins, &addr), BT_STATUS_SUCCESS);
}

void test_bt_hfp_ag_stop_virtual_call_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_stop_virtual_call, a);
    will_return(mock_stop_virtual_call, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_stop_virtual_call(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_start_voice_recognition_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_start_voice_recognition, a);
    will_return(mock_start_voice_recognition, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_start_voice_recognition(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_start_voice_recognition_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_start_voice_recognition, a);
    will_return(mock_start_voice_recognition, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_start_voice_recognition(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_stop_voice_recognition_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_stop_voice_recognition, a);
    will_return(mock_stop_voice_recognition, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_stop_voice_recognition(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_stop_voice_recognition_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_stop_voice_recognition, a);
    will_return(mock_stop_voice_recognition, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_stop_voice_recognition(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_volume_control_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_volume_control, a);
    will_return(mock_volume_control, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_volume_control(ins, &addr, 0, 10), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_volume_control_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_volume_control, a);
    will_return(mock_volume_control, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_volume_control(ins, &addr, 0, 15), BT_STATUS_FAIL);
}
void test_bt_hfp_ag_send_at_command_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_send_at_command, a);
    will_return(mock_send_at_command, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_ag_send_at_command(ins, &addr, "AT+TEST"), BT_STATUS_SUCCESS);
}
void test_bt_hfp_ag_send_at_command_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_send_at_command, a);
    will_return(mock_send_at_command, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_ag_send_at_command(ins, &addr, NULL), BT_STATUS_FAIL);
}
