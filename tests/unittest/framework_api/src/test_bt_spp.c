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

#include "bt_spp.h"
#include "bt_status.h"
#include "spp_service.h"

#include "cm_bt_spp.h"
#include "mock_service_manager.h"

static void* g_mock_handle = (void*)0x5B01;

static void* mock_register_app(void* r, const char* n, const spp_callbacks_t* c) { return mock_ptr_type(void*); }
static bt_status_t mock_unregister_app(void** r, void* h) { return mock_type(bt_status_t); }

static bt_status_t mock_server_start(void* h, uint16_t scn, bt_uuid_t* uuid, uint8_t max) { return mock_type(bt_status_t); }
static bt_status_t mock_server_stop(void* h, uint16_t scn) { return mock_type(bt_status_t); }
static bt_status_t mock_connect(void* h, bt_address_t* a, int16_t scn, bt_uuid_t* uuid, uint16_t* port, bool insecure) { return mock_type(bt_status_t); }
static bt_status_t mock_disconnect(void* h, bt_address_t* a, uint16_t port) { return mock_type(bt_status_t); }

static spp_interface_t g_mock_interface = {
    .register_app = mock_register_app,
    .unregister_app = mock_unregister_app,
    .server_start = mock_server_start,
    .server_stop = mock_server_stop,
    .connect = mock_connect,
    .disconnect = mock_disconnect,
};

int test_bt_spp_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    mock_service_manager_register(PROFILE_SPP, &g_mock_interface);
    *state = ins;
    return 0;
}

int test_bt_spp_teardown(FAR void** state)
{
    if (*state) {
        test_free(*state);
        *state = NULL;
    }
    return 0;
}

void test_bt_spp_register_app_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    spp_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_app, g_mock_handle);
    assert_ptr_equal(bt_spp_register_app(ins, &cbs), g_mock_handle);
}
void test_bt_spp_register_app_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_register_app, NULL);
    assert_null(bt_spp_register_app(ins, NULL));
}
void test_bt_spp_register_app_with_name_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    spp_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(mock_register_app, g_mock_handle);
    assert_ptr_equal(bt_spp_register_app_with_name(ins, "test_spp", &cbs), g_mock_handle);
}
void test_bt_spp_register_app_with_name_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_register_app, NULL);
    assert_null(bt_spp_register_app_with_name(ins, NULL, NULL));
}
void test_bt_spp_unregister_app_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_app, BT_STATUS_SUCCESS);
    assert_int_equal(bt_spp_unregister_app(ins, g_mock_handle), BT_STATUS_SUCCESS);
}
void test_bt_spp_unregister_app_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_unregister_app, BT_STATUS_FAIL);
    assert_int_equal(bt_spp_unregister_app(ins, NULL), BT_STATUS_FAIL);
}
void test_bt_spp_server_start_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    will_return(mock_server_start, BT_STATUS_SUCCESS);
    assert_int_equal(bt_spp_server_start(ins, g_mock_handle, 1, &uuid, 7), BT_STATUS_SUCCESS);
}
void test_bt_spp_server_start_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_server_start, BT_STATUS_FAIL);
    assert_int_equal(bt_spp_server_start(ins, g_mock_handle, 1, NULL, 0), BT_STATUS_FAIL);
}
void test_bt_spp_server_stop_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_server_stop, BT_STATUS_SUCCESS);
    assert_int_equal(bt_spp_server_stop(ins, g_mock_handle, 1), BT_STATUS_SUCCESS);
}
void test_bt_spp_server_stop_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(mock_server_stop, BT_STATUS_FAIL);
    assert_int_equal(bt_spp_server_stop(ins, g_mock_handle, 1), BT_STATUS_FAIL);
}

void test_bt_spp_connect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    bt_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    uint16_t port = 0;
    will_return(mock_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_spp_connect(ins, g_mock_handle, &addr, 1, &uuid, &port), BT_STATUS_SUCCESS);
}
void test_bt_spp_connect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    bt_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    uint16_t port = 0;
    will_return(mock_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_spp_connect(ins, g_mock_handle, &addr, 1, &uuid, &port), BT_STATUS_FAIL);
}
void test_bt_spp_insecure_connect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    bt_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    uint16_t port = 0;
    will_return(mock_connect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_spp_insecure_connect(ins, g_mock_handle, &addr, 1, &uuid, &port), BT_STATUS_SUCCESS);
}
void test_bt_spp_insecure_connect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    bt_uuid_t uuid;
    memset(&uuid, 0, sizeof(uuid));
    uint16_t port = 0;
    will_return(mock_connect, BT_STATUS_FAIL);
    assert_int_equal(bt_spp_insecure_connect(ins, g_mock_handle, &addr, 1, &uuid, &port), BT_STATUS_FAIL);
}
void test_bt_spp_disconnect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    will_return(mock_disconnect, BT_STATUS_SUCCESS);
    assert_int_equal(bt_spp_disconnect(ins, g_mock_handle, &addr, 1), BT_STATUS_SUCCESS);
}
void test_bt_spp_disconnect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(mock_disconnect, BT_STATUS_FAIL);
    assert_int_equal(bt_spp_disconnect(ins, g_mock_handle, &addr, 1), BT_STATUS_FAIL);
}
