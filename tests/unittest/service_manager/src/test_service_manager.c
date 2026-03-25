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

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Mock: adapter_on_profile_services_startup/shutdown
 ****************************************************************************/

static int g_mock_startup_called;
static uint8_t g_mock_startup_transport;
static bool g_mock_startup_ret;

static void mock_adapter_on_profile_services_startup(uint8_t transport,
    bool ret)
{
    g_mock_startup_called++;
    g_mock_startup_transport = transport;
    g_mock_startup_ret = ret;
}

static int g_mock_shutdown_called;
static uint8_t g_mock_shutdown_transport;
static bool g_mock_shutdown_ret;

static void mock_adapter_on_profile_services_shutdown(uint8_t transport,
    bool ret)
{
    g_mock_shutdown_called++;
    g_mock_shutdown_transport = transport;
    g_mock_shutdown_ret = ret;
}

/* Redirect calls from service_manager.c to our mocks */

#define adapter_on_profile_services_startup \
    mock_adapter_on_profile_services_startup
#define adapter_on_profile_services_shutdown \
    mock_adapter_on_profile_services_shutdown

/****************************************************************************
 * Prevent service_manager.c from including problematic headers
 ****************************************************************************/

#define _BT_ADAPTER_INTERNAL_H__

/* Include the headers that service_manager.c needs */

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_profile.h"
#include "bt_uuid.h"

/* Undefine LOG_TAG before including service_manager.c to avoid
 * redefinition warning (service_manager.c defines its own LOG_TAG) */

#undef LOG_TAG

/* Now include service_manager.c directly */

#include "../../../../service/profiles/service_manager.c"

/* Remove redirect macros */

#undef adapter_on_profile_services_startup
#undef adapter_on_profile_services_shutdown

#include "cm_service_manager.h"

/****************************************************************************
 * Mock profile service data
 ****************************************************************************/

static int g_mock_init_called;
static int g_mock_startup_cb_called;
static int g_mock_shutdown_cb_called;
static int g_mock_cleanup_called;
static int g_mock_process_msg_called;
static int g_mock_get_state_val;

static bt_status_t mock_profile_init(void)
{
    g_mock_init_called++;
    return BT_STATUS_SUCCESS;
}

static bt_status_t mock_profile_startup(profile_on_startup_t cb)
{
    g_mock_startup_cb_called++;
    if (cb) {
        cb(PROFILE_A2DP, true);
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t mock_profile_shutdown(profile_on_shutdown_t cb)
{
    g_mock_shutdown_cb_called++;
    if (cb) {
        cb(PROFILE_A2DP, true);
    }

    return BT_STATUS_SUCCESS;
}

static void mock_profile_cleanup(void)
{
    g_mock_cleanup_called++;
}

static void mock_profile_process_msg(profile_msg_t* msg)
{
    g_mock_process_msg_called++;
}

static int mock_profile_get_state(void)
{
    return g_mock_get_state_val;
}

static int g_mock_iface_val = 42;

static const void* mock_get_profile_interface(void)
{
    return &g_mock_iface_val;
}

static profile_service_t g_test_profile;

static void reset_mock_counters(void)
{
    g_mock_init_called = 0;
    g_mock_startup_cb_called = 0;
    g_mock_shutdown_cb_called = 0;
    g_mock_cleanup_called = 0;
    g_mock_process_msg_called = 0;
    g_mock_startup_called = 0;
    g_mock_shutdown_called = 0;
    g_mock_get_state_val = 1;
}

static void init_test_profile(profile_service_t* p, enum profile_id id,
    uint8_t transport, bool auto_start)
{
    memset(p, 0, sizeof(*p));
    *(enum profile_id*)&p->id = id;
    p->name = "test_profile";
    p->transport = transport;
    p->auto_start = auto_start;
    p->init = mock_profile_init;
    p->startup = mock_profile_startup;
    p->shutdown = mock_profile_shutdown;
    p->cleanup = mock_profile_cleanup;
    p->process_msg = mock_profile_process_msg;
    p->get_state = mock_profile_get_state;
    p->get_profile_interface = mock_get_profile_interface;
}

/****************************************************************************
 * Setup / Teardown
 ****************************************************************************/

int test_service_manager_setup(FAR void** state)
{
    memset(service_slots, 0, sizeof(service_slots));
    reset_mock_counters();
    init_test_profile(&g_test_profile, PROFILE_A2DP, 0, true);
    return 0;
}

int test_service_manager_teardown(FAR void** state)
{
    memset(service_slots, 0, sizeof(service_slots));
    return 0;
}

/****************************************************************************
 * Test Cases: register_service
 ****************************************************************************/

void test_register_service_normal(FAR void** state)
{
    assert_null(service_slots[PROFILE_A2DP].service);
    register_service(&g_test_profile);
    assert_ptr_equal(service_slots[PROFILE_A2DP].service, &g_test_profile);
    assert_int_equal(service_slots[PROFILE_A2DP].state, TURN_OFF);
}

void test_register_service_duplicate(FAR void** state)
{
    register_service(&g_test_profile);
    assert_ptr_equal(service_slots[PROFILE_A2DP].service, &g_test_profile);

    /* Register again - should not overwrite */

    profile_service_t other;
    init_test_profile(&other, PROFILE_A2DP, 0, true);
    other.name = "other_profile";
    register_service(&other);

    /* Original should still be there */

    assert_ptr_equal(service_slots[PROFILE_A2DP].service, &g_test_profile);
}

/****************************************************************************
 * Test Cases: service_manager_init
 ****************************************************************************/

void test_service_manager_init_normal(FAR void** state)
{
    register_service(&g_test_profile);
    int ret = service_manager_init();
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_init_called, 1);
}

void test_service_manager_init_no_profiles(FAR void** state)
{
    /* No profiles registered */

    int ret = service_manager_init();
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_init_called, 0);
}

/****************************************************************************
 * Test Cases: service_manager_startup
 ****************************************************************************/

void test_service_manager_startup_normal(FAR void** state)
{
    register_service(&g_test_profile);
    int ret = service_manager_startup(0);
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_startup_cb_called, 1);

    /* The mock callback calls service_on_startup which sets state to
     * TURN_ON, then check_is_all_startup returns true and calls
     * adapter_on_profile_services_startup */

    assert_int_equal(g_mock_startup_called, 1);
}

void test_service_manager_startup_all_already_started(FAR void** state)
{
    /* No profiles registered for transport 0 -> all started trivially */

    int ret = service_manager_startup(0);
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_startup_called, 1);
    assert_int_equal(g_mock_startup_cb_called, 0);
}

/****************************************************************************
 * Test Cases: service_manager_shutdown
 ****************************************************************************/

void test_service_manager_shutdown_normal(FAR void** state)
{
    register_service(&g_test_profile);

    /* First startup so state is TURN_ON */

    service_manager_startup(0);
    reset_mock_counters();

    int ret = service_manager_shutdown(0);
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_shutdown_cb_called, 1);
    assert_int_equal(g_mock_shutdown_called, 1);
}

void test_service_manager_shutdown_all_already_off(FAR void** state)
{
    /* No profiles registered -> all off trivially */

    int ret = service_manager_shutdown(0);
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_shutdown_called, 1);
    assert_int_equal(g_mock_shutdown_cb_called, 0);
}

/****************************************************************************
 * Test Cases: service_manager_get_uuid
 ****************************************************************************/

void test_service_manager_get_uuid_normal(FAR void** state)
{
    /* Set a non-empty UUID on the test profile */

    g_test_profile.uuid.type = BT_UUID128_TYPE;
    g_test_profile.uuid.val.u128[0] = 0x01;
    register_service(&g_test_profile);

    bt_uuid_t uuids[BT_UUID_MAX_NUM];
    uint16_t size = 0;
    memset(uuids, 0, sizeof(uuids));

    int ret = service_manager_get_uuid(uuids, &size);
    assert_int_equal(ret, 0);
    assert_int_equal(size, 1);
    assert_int_equal(uuids[0].val.u128[0], 0x01);
}

void test_service_manager_get_uuid_empty(FAR void** state)
{
    /* No profiles registered */

    bt_uuid_t uuids[BT_UUID_MAX_NUM];
    uint16_t size = 99;
    memset(uuids, 0, sizeof(uuids));

    int ret = service_manager_get_uuid(uuids, &size);
    assert_int_equal(ret, 0);
    assert_int_equal(size, 0);
}

/****************************************************************************
 * Test Cases: service_manager_processmsg
 ****************************************************************************/

void test_service_manager_processmsg_normal(FAR void** state)
{
    register_service(&g_test_profile);

    profile_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.event = PROFILE_EVT_REMOTE_DETACH;

    int ret = service_manager_processmsg(&msg);
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_process_msg_called, 1);
}

/****************************************************************************
 * Test Cases: service_manager_control
 ****************************************************************************/

void test_service_manager_control_start(FAR void** state)
{
    register_service(&g_test_profile);
    bt_status_t ret = service_manager_control(PROFILE_A2DP,
        CONTROL_CMD_START);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
    assert_int_equal(g_mock_startup_cb_called, 1);
}

void test_service_manager_control_stop(FAR void** state)
{
    register_service(&g_test_profile);
    bt_status_t ret = service_manager_control(PROFILE_A2DP,
        CONTROL_CMD_STOP);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
    assert_int_equal(g_mock_shutdown_cb_called, 1);
}

void test_service_manager_control_not_supported(FAR void** state)
{
    /* No profile registered at slot -> startup is NULL */

    profile_service_t p;
    init_test_profile(&p, PROFILE_A2DP_SINK, 0, false);
    p.startup = NULL;
    p.shutdown = NULL;
    register_service(&p);

    bt_status_t ret = service_manager_control(PROFILE_A2DP_SINK,
        CONTROL_CMD_START);
    assert_int_equal(ret, BT_STATUS_NOT_SUPPORTED);

    ret = service_manager_control(PROFILE_A2DP_SINK, CONTROL_CMD_STOP);
    assert_int_equal(ret, BT_STATUS_NOT_SUPPORTED);
}

void test_service_manager_control_dump(FAR void** state)
{
    register_service(&g_test_profile);
    bt_status_t ret = service_manager_control(PROFILE_A2DP,
        CONTROL_CMD_DUMP);
    assert_int_equal(ret, BT_STATUS_SUCCESS);
}

/****************************************************************************
 * Test Cases: service_manager_cleanup
 ****************************************************************************/

void test_service_manager_cleanup_normal(FAR void** state)
{
    register_service(&g_test_profile);
    int ret = service_manager_cleanup();
    assert_int_equal(ret, 0);
    assert_int_equal(g_mock_cleanup_called, 1);
    assert_null(service_slots[PROFILE_A2DP].service);
}

void test_service_manager_cleanup_null_cleanup(FAR void** state)
{
    profile_service_t p;
    init_test_profile(&p, PROFILE_A2DP, 0, false);
    p.cleanup = NULL;
    p.name = "no_cleanup_profile";
    register_service(&p);

    /* Should not crash, just log error */

    int ret = service_manager_cleanup();
    assert_int_equal(ret, 0);
    assert_null(service_slots[PROFILE_A2DP].service);
}
