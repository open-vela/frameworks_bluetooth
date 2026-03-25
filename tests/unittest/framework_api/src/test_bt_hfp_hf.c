/****************************************************************************
 * tests/unittest/framework_api/src/test_bt_hfp_hf.c
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

#include "hfp_hf_service.h"
#include "bt_hfp_hf.h"
#include "bt_status.h"

#include "mock_service_manager.h"
#include "cm_bt_hfp_hf.h"


static void *g_mock_cookie = (void *)0x0F01;

static void *mock_register_callbacks(void *r, const hfp_hf_callbacks_t *c) { return mock_ptr_type(void *); }
static bool mock_unregister_callbacks(void **r, void *c) { return mock_type(bool); }
static bool mock_is_connected(bt_address_t *a) { check_expected_ptr(a); return mock_type(bool); }
static bool mock_is_audio_connected(bt_address_t *a) { check_expected_ptr(a); return mock_type(bool); }
static profile_connection_state_t mock_get_connection_state(bt_address_t *a) { check_expected_ptr(a); return mock_type(profile_connection_state_t); }
static bt_status_t mock_connect(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_disconnect(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_set_connection_policy(bt_address_t *a, connection_policy_t p) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_connect_audio(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_disconnect_audio(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_start_voice_recognition(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_stop_voice_recognition(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_dial(bt_address_t *a, const char *n) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_dial_memory(bt_address_t *a, uint32_t m) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_redial(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_accept_call(bt_address_t *a, hfp_call_accept_t f) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_reject_call(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_hold_call(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_terminate_call(bt_address_t *a) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_send_dtmf(bt_address_t *a, char d) { check_expected_ptr(a); return mock_type(bt_status_t); }
static bt_status_t mock_volume_control(bt_address_t *a, hfp_volume_type_t t, uint8_t v) { check_expected_ptr(a); return mock_type(bt_status_t); }

static hfp_hf_interface_t g_mock_interface = {
    .register_callbacks = mock_register_callbacks,
    .unregister_callbacks = mock_unregister_callbacks,
    .is_connected = mock_is_connected,
    .is_audio_connected = mock_is_audio_connected,
    .get_connection_state = mock_get_connection_state,
    .connect = mock_connect,
    .disconnect = mock_disconnect,
    .set_connection_policy = mock_set_connection_policy,
    .connect_audio = mock_connect_audio,
    .disconnect_audio = mock_disconnect_audio,
    .start_voice_recognition = mock_start_voice_recognition,
    .stop_voice_recognition = mock_stop_voice_recognition,
    .dial = mock_dial,
    .dial_memory = mock_dial_memory,
    .redial = mock_redial,
    .accept_call = mock_accept_call,
    .reject_call = mock_reject_call,
    .hold_call = mock_hold_call,
    .terminate_call = mock_terminate_call,
    .send_dtmf = mock_send_dtmf,
    .volume_control = mock_volume_control,
};


int test_bt_hfp_hf_setup(FAR void **state)
{
    bt_instance_t *ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_HFP_HF, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_hfp_hf_teardown(FAR void **state)
{
    if (*state) { test_free(*state); *state = NULL; }
    return 0;
}

#define ADDR_INIT(a, v) bt_address_t a; memset(&a, v, sizeof(a))

void test_bt_hfp_hf_register_callbacks_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    hfp_hf_callbacks_t cbs; memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_callbacks, g_mock_cookie);
    assert_ptr_equal(bt_hfp_hf_register_callbacks(ins, (const hfp_hf_callbacks_t *)&cbs), g_mock_cookie);
}
void test_bt_hfp_hf_register_callbacks_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    will_return(mock_register_callbacks, NULL);
    assert_null(bt_hfp_hf_register_callbacks(ins, NULL));
}
void test_bt_hfp_hf_unregister_callbacks_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    will_return(mock_unregister_callbacks, true);
    assert_true(bt_hfp_hf_unregister_callbacks(ins, g_mock_cookie));
}
void test_bt_hfp_hf_unregister_callbacks_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    will_return(mock_unregister_callbacks, false);
    assert_false(bt_hfp_hf_unregister_callbacks(ins, NULL));
}
void test_bt_hfp_hf_is_connected_true(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_is_connected, a); will_return(mock_is_connected, true);
    assert_true(bt_hfp_hf_is_connected(ins, &addr));
}
void test_bt_hfp_hf_is_connected_false(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_is_connected, a); will_return(mock_is_connected, false);
    assert_false(bt_hfp_hf_is_connected(ins, &addr));
}
void test_bt_hfp_hf_is_audio_connected_true(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_is_audio_connected, a); will_return(mock_is_audio_connected, true);
    assert_true(bt_hfp_hf_is_audio_connected(ins, &addr));
}
void test_bt_hfp_hf_is_audio_connected_false(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_is_audio_connected, a); will_return(mock_is_audio_connected, false);
    assert_false(bt_hfp_hf_is_audio_connected(ins, &addr));
}
void test_bt_hfp_hf_get_connection_state_connected(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_get_connection_state, a); will_return(mock_get_connection_state, PROFILE_STATE_CONNECTED);
    assert_int_equal(bt_hfp_hf_get_connection_state(ins, &addr), PROFILE_STATE_CONNECTED);
}
void test_bt_hfp_hf_get_connection_state_disconnected(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_get_connection_state, a); will_return(mock_get_connection_state, PROFILE_STATE_DISCONNECTED);
    assert_int_equal(bt_hfp_hf_get_connection_state(ins, &addr), PROFILE_STATE_DISCONNECTED);
}
void test_bt_hfp_hf_connect_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_connect, a); will_return(mock_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_connect(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_connect_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_connect, a); will_return(mock_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_connect(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_disconnect_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_disconnect, a); will_return(mock_disconnect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_disconnect(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_disconnect_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_disconnect, a); will_return(mock_disconnect, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_disconnect(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_set_connection_policy_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_set_connection_policy, a); will_return(mock_set_connection_policy, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_set_connection_policy(ins, &addr, 0), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_set_connection_policy_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_set_connection_policy, a); will_return(mock_set_connection_policy, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_set_connection_policy(ins, &addr, 0), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_connect_audio_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_connect_audio, a); will_return(mock_connect_audio, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_connect_audio(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_connect_audio_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_connect_audio, a); will_return(mock_connect_audio, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_connect_audio(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_disconnect_audio_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_disconnect_audio, a); will_return(mock_disconnect_audio, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_disconnect_audio(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_disconnect_audio_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_disconnect_audio, a); will_return(mock_disconnect_audio, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_disconnect_audio(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_start_voice_recognition_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_start_voice_recognition, a); will_return(mock_start_voice_recognition, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_start_voice_recognition(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_start_voice_recognition_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_start_voice_recognition, a); will_return(mock_start_voice_recognition, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_start_voice_recognition(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_stop_voice_recognition_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_stop_voice_recognition, a); will_return(mock_stop_voice_recognition, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_stop_voice_recognition(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_stop_voice_recognition_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_stop_voice_recognition, a); will_return(mock_stop_voice_recognition, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_stop_voice_recognition(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_dial_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_dial, a); will_return(mock_dial, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_dial(ins, &addr, "1234567890"), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_dial_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_dial, a); will_return(mock_dial, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_dial(ins, &addr, NULL), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_dial_memory_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_dial_memory, a); will_return(mock_dial_memory, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_dial_memory(ins, &addr, 1), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_dial_memory_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_dial_memory, a); will_return(mock_dial_memory, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_dial_memory(ins, &addr, 99), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_redial_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_redial, a); will_return(mock_redial, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_redial(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_redial_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_redial, a); will_return(mock_redial, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_redial(ins, &addr), BT_STATUS_FAIL);
}
void test_bt_hfp_hf_accept_call_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_accept_call, a); will_return(mock_accept_call, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_accept_call(ins, &addr, 0), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_reject_call_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_reject_call, a); will_return(mock_reject_call, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_reject_call(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_hold_call_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_hold_call, a); will_return(mock_hold_call, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_hold_call(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_terminate_call_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_terminate_call, a); will_return(mock_terminate_call, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_terminate_call(ins, &addr), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_send_dtmf_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_send_dtmf, a); will_return(mock_send_dtmf, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_send_dtmf(ins, &addr, '1'), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_volume_control_normal(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x11);
    expect_any(mock_volume_control, a); will_return(mock_volume_control, BT_STATUS_SUCCESS);
    assert_int_equal(bt_hfp_hf_volume_control(ins, &addr, 0, 10), BT_STATUS_SUCCESS);
}
void test_bt_hfp_hf_volume_control_fail(FAR void **state)
{
    bt_instance_t *ins = *state; ADDR_INIT(addr, 0x22);
    expect_any(mock_volume_control, a); will_return(mock_volume_control, BT_STATUS_FAIL);
    assert_int_equal(bt_hfp_hf_volume_control(ins, &addr, 0, 15), BT_STATUS_FAIL);
}
