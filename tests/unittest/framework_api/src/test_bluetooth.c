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

#include "bluetooth.h"
#include "bt_internal.h"
#include "bt_status.h"

#include "cm_bluetooth.h"

/****************************************************************************
 * Private Functions - Mock functions
 ****************************************************************************/

/* BLUETOOTH_SYSTEM: instance type passed to manager_create_instance,
 * matches the value used in bluetooth.c for local system instances. */

#define BLUETOOTH_SYSTEM 0

static uint32_t g_mock_app_id = 42; /* arbitrary non-zero test value */

void* zalloc(size_t size)
{
    return mock_ptr_type(void*);
}

pid_t getpid(void)
{
    return (pid_t)1234;
}

bt_status_t manager_create_instance(uint64_t handle, int type,
    const char* name, pid_t pid, int flags, uint32_t* app_id)
{
    check_expected(type);
    bt_status_t ret = mock_type(bt_status_t);
    if (ret == BT_STATUS_SUCCESS) {
        *app_id = g_mock_app_id;
    }
    return ret;
}

bt_status_t manager_get_instance(const char* name, pid_t pid, uint64_t* handle)
{
    bt_status_t ret = mock_type(bt_status_t);
    *handle = mock_type(uint64_t);
    return ret;
}

void manager_delete_instance(uint32_t app_id)
{
    check_expected(app_id);
    function_called();
}

bt_status_t manager_start_service(uint32_t app_id, enum profile_id id)
{
    check_expected(app_id);
    return mock_type(bt_status_t);
}

bt_status_t manager_stop_service(uint32_t app_id, enum profile_id id)
{
    check_expected(app_id);
    return mock_type(bt_status_t);
}

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bluetooth_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    *state = ins;
    return 0;
}

int test_bluetooth_teardown(FAR void** state)
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

void test_bluetooth_create_instance_normal(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));

    will_return(zalloc, &mock_ins);
    expect_value(manager_create_instance, type, BLUETOOTH_SYSTEM);
    will_return(manager_create_instance, BT_STATUS_SUCCESS);

    bt_instance_t* ins = bluetooth_create_instance();
    assert_non_null(ins);
    assert_int_equal(ins->app_id, g_mock_app_id);
}

void test_bluetooth_create_instance_alloc_fail(FAR void** state)
{
    will_return(zalloc, NULL);

    bt_instance_t* ins = bluetooth_create_instance();
    assert_null(ins);
}

void test_bluetooth_create_instance_manager_fail(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));

    will_return(zalloc, &mock_ins);
    expect_value(manager_create_instance, type, BLUETOOTH_SYSTEM);
    will_return(manager_create_instance, BT_STATUS_FAIL);

    bt_instance_t* ins = bluetooth_create_instance();
    assert_null(ins);
}

void test_bluetooth_get_instance_existing(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));
    mock_ins.app_id = 99;

    will_return(manager_get_instance, BT_STATUS_SUCCESS);
    will_return(manager_get_instance, (uint64_t)(uintptr_t)&mock_ins);

    bt_instance_t* ins = bluetooth_get_instance();
    assert_non_null(ins);
    assert_ptr_equal(ins, &mock_ins);
}

void test_bluetooth_get_instance_new(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));

    /* manager_get_instance fails, so create_instance is called */
    will_return(manager_get_instance, BT_STATUS_FAIL);
    will_return(manager_get_instance, (uint64_t)0);

    /* create_instance path */
    will_return(zalloc, &mock_ins);
    expect_value(manager_create_instance, type, BLUETOOTH_SYSTEM);
    will_return(manager_create_instance, BT_STATUS_SUCCESS);

    bt_instance_t* ins = bluetooth_get_instance();
    assert_non_null(ins);
}

void test_bluetooth_get_proxy_hfp_hf(FAR void** state)
{
    bt_instance_t* ins = *state;

    /* bluetooth_get_proxy returns NULL for all profiles when binder IPC
     * is not configured (CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC unset). */

    void* proxy = bluetooth_get_proxy(ins, PROFILE_HFP_HF);
    assert_null(proxy);
}

void test_bluetooth_get_proxy_default(FAR void** state)
{
    bt_instance_t* ins = *state;

    void* proxy = bluetooth_get_proxy(ins, 0xFF);
    assert_null(proxy);
}

void test_bluetooth_delete_instance_normal(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = g_mock_app_id;

    expect_value(manager_delete_instance, app_id, g_mock_app_id);
    expect_function_call(manager_delete_instance);
    bluetooth_delete_instance(ins);
}

void test_bluetooth_start_service_normal(FAR void** state)
{
    bt_instance_t* ins = *state;

    expect_value(manager_start_service, app_id, ins->app_id);
    will_return(manager_start_service, BT_STATUS_SUCCESS);

    assert_int_equal(bluetooth_start_service(ins, 0), BT_STATUS_SUCCESS);
}

void test_bluetooth_start_service_fail(FAR void** state)
{
    bt_instance_t* ins = *state;

    expect_value(manager_start_service, app_id, ins->app_id);
    will_return(manager_start_service, BT_STATUS_FAIL);

    assert_int_equal(bluetooth_start_service(ins, 0), BT_STATUS_FAIL);
}

void test_bluetooth_stop_service_normal(FAR void** state)
{
    bt_instance_t* ins = *state;

    expect_value(manager_stop_service, app_id, ins->app_id);
    will_return(manager_stop_service, BT_STATUS_SUCCESS);

    assert_int_equal(bluetooth_stop_service(ins, 0), BT_STATUS_SUCCESS);
}

void test_bluetooth_stop_service_fail(FAR void** state)
{
    bt_instance_t* ins = *state;

    expect_value(manager_stop_service, app_id, ins->app_id);
    will_return(manager_stop_service, BT_STATUS_FAIL);

    assert_int_equal(bluetooth_stop_service(ins, 0), BT_STATUS_FAIL);
}

void test_bluetooth_create_instance_sets_app_id(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));

    will_return(zalloc, &mock_ins);
    expect_value(manager_create_instance, type, BLUETOOTH_SYSTEM);
    will_return(manager_create_instance, BT_STATUS_SUCCESS);

    bt_instance_t* ins = bluetooth_create_instance();
    assert_non_null(ins);
    assert_int_equal(ins->app_id, g_mock_app_id);
}

void test_bluetooth_get_instance_returns_existing(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));
    mock_ins.app_id = 77;

    will_return(manager_get_instance, BT_STATUS_SUCCESS);
    will_return(manager_get_instance, (uint64_t)(uintptr_t)&mock_ins);

    bt_instance_t* ins = bluetooth_get_instance();
    assert_int_equal(ins->app_id, 77);
}

void test_bluetooth_get_proxy_null_proxy(FAR void** state)
{
    bt_instance_t* ins = *state;

    void* proxy = bluetooth_get_proxy(ins, PROFILE_HFP_HF);
    assert_null(proxy);
}

void test_bluetooth_start_service_invalid_profile(FAR void** state)
{
    bt_instance_t* ins = *state;

    expect_value(manager_start_service, app_id, ins->app_id);
    will_return(manager_start_service, BT_STATUS_PARM_INVALID);

    assert_int_equal(bluetooth_start_service(ins, 0xFF), BT_STATUS_PARM_INVALID);
}

void test_bluetooth_stop_service_invalid_profile(FAR void** state)
{
    bt_instance_t* ins = *state;

    expect_value(manager_stop_service, app_id, ins->app_id);
    will_return(manager_stop_service, BT_STATUS_PARM_INVALID);

    assert_int_equal(bluetooth_stop_service(ins, 0xFF), BT_STATUS_PARM_INVALID);
}

void test_bluetooth_delete_instance_frees_memory(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 55;

    expect_value(manager_delete_instance, app_id, 55);
    expect_function_call(manager_delete_instance);
    bluetooth_delete_instance(ins);
}

void test_bluetooth_create_instance_getpid_called(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));

    will_return(zalloc, &mock_ins);
    expect_value(manager_create_instance, type, BLUETOOTH_SYSTEM);
    will_return(manager_create_instance, BT_STATUS_SUCCESS);

    bt_instance_t* ins = bluetooth_create_instance();
    assert_non_null(ins);
}

void test_bluetooth_get_instance_getpid_called(FAR void** state)
{
    bt_instance_t mock_ins;
    memset(&mock_ins, 0, sizeof(mock_ins));
    mock_ins.app_id = 88;

    will_return(manager_get_instance, BT_STATUS_SUCCESS);
    will_return(manager_get_instance, (uint64_t)(uintptr_t)&mock_ins);

    bt_instance_t* ins = bluetooth_get_instance();
    assert_non_null(ins);
}
