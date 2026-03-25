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

// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on

#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "hfp_ag_event.h"

#include "../../../../service/profiles/hfp_ag/hfp_ag_event.c"

#include "cm_hfp_ag_event.h"

int test_hfp_ag_event_setup(FAR void** state)
{
    return 0;
}

int test_hfp_ag_event_teardown(FAR void** state)
{
    return 0;
}

void test_hfp_ag_msg_new_normal(FAR void** state)
{
    bt_address_t addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    hfp_ag_msg_t* msg = hfp_ag_msg_new(AG_CONNECT, &addr);
    assert_non_null(msg);
    assert_int_equal(msg->event, AG_CONNECT);
    assert_memory_equal(&msg->data.addr, &addr, sizeof(bt_address_t));
    hfp_ag_msg_destory(msg);
}

void test_hfp_ag_msg_new_null_addr(FAR void** state)
{
    hfp_ag_msg_t* msg = hfp_ag_msg_new(AG_STARTUP, NULL);
    assert_non_null(msg);
    assert_int_equal(msg->event, AG_STARTUP);
    hfp_ag_msg_destory(msg);
}

void test_hfp_ag_event_new_ext_with_data(FAR void** state)
{
    bt_address_t addr = { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF } };
    uint8_t payload[] = { 0x10, 0x20, 0x30 };
    hfp_ag_msg_t* msg = hfp_ag_event_new_ext(AG_DISCONNECT, &addr,
        payload, sizeof(payload));
    assert_non_null(msg);
    assert_int_equal(msg->event, AG_DISCONNECT);
    assert_int_equal(msg->data.size, sizeof(payload));
    assert_non_null(msg->data.data);
    assert_memory_equal(msg->data.data, payload, sizeof(payload));
    hfp_ag_msg_destory(msg);
}

void test_hfp_ag_event_new_ext_zero_size(FAR void** state)
{
    hfp_ag_msg_t* msg = hfp_ag_event_new_ext(AG_STARTUP, NULL, NULL, 0);
    assert_non_null(msg);
    assert_int_equal(msg->event, AG_STARTUP);
    assert_null(msg->data.data);
    hfp_ag_msg_destory(msg);
}
