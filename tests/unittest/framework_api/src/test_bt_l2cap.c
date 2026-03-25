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

#include "bt_l2cap.h"
#include "bt_status.h"

#include "cm_bt_l2cap.h"

static void* g_mock_handle = (void*)0x1201;

void* l2cap_register_callbacks(void* remote, const l2cap_callbacks_t* cbs)
{
    return mock_ptr_type(void*);
}

bool l2cap_unregister_callbacks(void** remote, void* cookie)
{
    return mock_type(bool);
}

bt_status_t l2cap_listen_channel(void* handle, l2cap_config_option_t* option)
{
    return mock_type(bt_status_t);
}

bt_status_t l2cap_connect_channel(void* handle, bt_address_t* addr, l2cap_config_option_t* option)
{
    return mock_type(bt_status_t);
}

bt_status_t l2cap_disconnect_channel(void* handle, uint16_t id)
{
    return mock_type(bt_status_t);
}

bt_status_t l2cap_stop_listen_channel(void* handle, bt_transport_t transport, uint16_t psm)
{
    return mock_type(bt_status_t);
}

int test_bt_l2cap_setup(FAR void** state)
{
    bt_instance_t* ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    *state = ins;
    return 0;
}
int test_bt_l2cap_teardown(FAR void** state)
{
    if (*state) {
        test_free(*state);
        *state = NULL;
    }
    return 0;
}

void test_bt_l2cap_register_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    l2cap_callbacks_t cbs;
    memset(&cbs, 0, sizeof(cbs));
    will_return(l2cap_register_callbacks, g_mock_handle);
    assert_ptr_equal(bt_l2cap_register_callbacks(ins, &cbs), g_mock_handle);
}
void test_bt_l2cap_register_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_register_callbacks, NULL);
    assert_null(bt_l2cap_register_callbacks(ins, NULL));
}
void test_bt_l2cap_unregister_callbacks_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_unregister_callbacks, true);
    assert_true(bt_l2cap_unregister_callbacks(ins, g_mock_handle));
}
void test_bt_l2cap_unregister_callbacks_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_unregister_callbacks, false);
    assert_false(bt_l2cap_unregister_callbacks(ins, NULL));
}

void test_bt_l2cap_listen_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    l2cap_config_option_t opt;
    memset(&opt, 0, sizeof(opt));
    will_return(l2cap_listen_channel, BT_STATUS_SUCCESS);
    assert_int_equal(bt_l2cap_listen(ins, g_mock_handle, &opt), BT_STATUS_SUCCESS);
}
void test_bt_l2cap_listen_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_listen_channel, BT_STATUS_FAIL);
    assert_int_equal(bt_l2cap_listen(ins, g_mock_handle, NULL), BT_STATUS_FAIL);
}
void test_bt_l2cap_connect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x11, sizeof(addr));
    l2cap_config_option_t opt;
    memset(&opt, 0, sizeof(opt));
    will_return(l2cap_connect_channel, BT_STATUS_SUCCESS);
    assert_int_equal(bt_l2cap_connect(ins, g_mock_handle, &addr, &opt), BT_STATUS_SUCCESS);
}
void test_bt_l2cap_connect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    bt_address_t addr;
    memset(&addr, 0x22, sizeof(addr));
    will_return(l2cap_connect_channel, BT_STATUS_FAIL);
    assert_int_equal(bt_l2cap_connect(ins, g_mock_handle, &addr, NULL), BT_STATUS_FAIL);
}
void test_bt_l2cap_disconnect_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_disconnect_channel, BT_STATUS_SUCCESS);
    assert_int_equal(bt_l2cap_disconnect(ins, g_mock_handle, 1), BT_STATUS_SUCCESS);
}
void test_bt_l2cap_disconnect_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_disconnect_channel, BT_STATUS_FAIL);
    assert_int_equal(bt_l2cap_disconnect(ins, g_mock_handle, 1), BT_STATUS_FAIL);
}
void test_bt_l2cap_stop_listen_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_stop_listen_channel, BT_STATUS_SUCCESS);
    assert_int_equal(bt_l2cap_stop_listen(ins, g_mock_handle, 0x0025), BT_STATUS_SUCCESS);
}
void test_bt_l2cap_stop_listen_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_stop_listen_channel, BT_STATUS_FAIL);
    assert_int_equal(bt_l2cap_stop_listen(ins, g_mock_handle, 0x0025), BT_STATUS_FAIL);
}
void test_bt_l2cap_stop_listen_with_transport_normal(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_stop_listen_channel, BT_STATUS_SUCCESS);
    assert_int_equal(bt_l2cap_stop_listen_with_transport(ins, g_mock_handle, BT_TRANSPORT_BLE, 0x0025), BT_STATUS_SUCCESS);
}
void test_bt_l2cap_stop_listen_with_transport_fail(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_stop_listen_channel, BT_STATUS_FAIL);
    assert_int_equal(bt_l2cap_stop_listen_with_transport(ins, g_mock_handle, BT_TRANSPORT_BREDR, 0x0025), BT_STATUS_FAIL);
}
void test_bt_l2cap_register_callbacks_null_cbs(FAR void** state)
{
    bt_instance_t* ins = *state;
    will_return(l2cap_register_callbacks, NULL);
    assert_null(bt_l2cap_register_callbacks(ins, NULL));
}
