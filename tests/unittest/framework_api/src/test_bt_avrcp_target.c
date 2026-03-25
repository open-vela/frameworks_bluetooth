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

#include "avrcp_target_service.h"
#include "bt_avrcp_target.h"
#include "bt_status.h"

#include "cm_bt_avrcp_target.h"
#include "mock_service_manager.h"

static void* g_mock_cookie = (void*)0xA701;

static void* mock_register_callbacks(void* r, const avrcp_target_callbacks_t* c) { return mock_ptr_type(void*); }
static bool mock_unregister_callbacks(void** r, void* c) { return mock_type(bool); }

static bt_status_t mock_get_play_status_rsp(bt_address_t* a, avrcp_play_status_t ps, uint32_t sl, uint32_t sp)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}
static bt_status_t mock_play_status_notify(bt_address_t* a, avrcp_play_status_t ps)
{
    check_expected_ptr(a);
    return mock_type(bt_status_t);
}

static avrcp_target_interface_t g_mock_interface = {
    .register_callbacks = mock_register_callbacks,
    .unregister_callbacks = mock_unregister_callbacks,
    .get_play_status_rsp = mock_get_play_status_rsp,
    .play_status_notify = mock_play_status_notify,
};

int test_bt_avrcp_target_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_AVRCP_TG, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_avrcp_target_teardown(FAR void** state)
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

void test_bt_avrcp_target_register_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    avrcp_target_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_callbacks, g_mock_cookie);
    assert_ptr_equal(bt_avrcp_target_register_callbacks(ins, &cbs), g_mock_cookie);
}
void test_bt_avrcp_target_register_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_register_callbacks, NULL);
    assert_null(bt_avrcp_target_register_callbacks(ins, NULL));
}
void test_bt_avrcp_target_unregister_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_callbacks, true);
    assert_true(bt_avrcp_target_unregister_callbacks(ins, g_mock_cookie));
}
void test_bt_avrcp_target_unregister_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_callbacks, false);
    assert_false(bt_avrcp_target_unregister_callbacks(ins, NULL));
}

void test_bt_avrcp_target_get_play_status_response_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_get_play_status_rsp, a);
    will_return(mock_get_play_status_rsp, BT_STATUS_SUCCESS);
    assert_int_equal(bt_avrcp_target_get_play_status_response(ins, &addr, 0, 1000, 500), BT_STATUS_SUCCESS);
}
void test_bt_avrcp_target_get_play_status_response_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_get_play_status_rsp, a);
    will_return(mock_get_play_status_rsp, BT_STATUS_FAIL);
    assert_int_equal(bt_avrcp_target_get_play_status_response(ins, &addr, 0, 0, 0), BT_STATUS_FAIL);
}
void test_bt_avrcp_target_play_status_notify_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x11);
    expect_any(mock_play_status_notify, a);
    will_return(mock_play_status_notify, BT_STATUS_SUCCESS);
    assert_int_equal(bt_avrcp_target_play_status_notify(ins, &addr, 1), BT_STATUS_SUCCESS);
}
void test_bt_avrcp_target_play_status_notify_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x22);
    expect_any(mock_play_status_notify, a);
    will_return(mock_play_status_notify, BT_STATUS_FAIL);
    assert_int_equal(bt_avrcp_target_play_status_notify(ins, &addr, 0), BT_STATUS_FAIL);
}
void test_bt_avrcp_target_register_callbacks_null_cbs(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_register_callbacks, NULL);
    assert_null(bt_avrcp_target_register_callbacks(ins, NULL));
}
void test_bt_avrcp_target_unregister_callbacks_null_cookie(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_callbacks, false);
    assert_false(bt_avrcp_target_unregister_callbacks(ins, NULL));
}
void test_bt_avrcp_target_get_play_status_response_playing(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x33);
    expect_any(mock_get_play_status_rsp, a);
    will_return(mock_get_play_status_rsp, BT_STATUS_SUCCESS);
    assert_int_equal(bt_avrcp_target_get_play_status_response(ins, &addr, 1, 5000, 2500), BT_STATUS_SUCCESS);
}
void test_bt_avrcp_target_play_status_notify_paused(FAR void** state)
{
    bt_instance_t* ins = *state;
    ADDR_INIT(addr, 0x33);
    expect_any(mock_play_status_notify, a);
    will_return(mock_play_status_notify, BT_STATUS_SUCCESS);
    assert_int_equal(bt_avrcp_target_play_status_notify(ins, &addr, 2), BT_STATUS_SUCCESS);
}
