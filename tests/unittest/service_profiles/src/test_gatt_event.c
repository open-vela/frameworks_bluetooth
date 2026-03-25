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
#include "gattc_event.h"
#include "gatts_event.h"

#include "../../../../service/profiles/gatt/gattc_event.c"
#include "../../../../service/profiles/gatt/gatts_event.c"

#include "cm_gatt_event.h"

int test_gatt_event_setup(FAR void** state)
{
    return 0;
}

int test_gatt_event_teardown(FAR void** state)
{
    return 0;
}

/* gatts_msg */

void test_gatts_msg_new_normal(FAR void** state)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_CONNECT_CHANGE, 0);
    assert_non_null(msg);
    assert_int_equal(msg->event, GATTS_EVENT_CONNECT_CHANGE);
    gatts_msg_destory(msg);
}

void test_gatts_msg_new_with_payload(FAR void** state)
{
    gatts_msg_t* msg = gatts_msg_new(GATTS_EVENT_WRITE_REQUEST, 64);
    assert_non_null(msg);
    assert_int_equal(msg->event, GATTS_EVENT_WRITE_REQUEST);
    gatts_msg_destory(msg);
}

void test_gatts_op_new_normal(FAR void** state)
{
    gatts_op_t* op = gatts_op_new(GATTS_REQ_CONNECT);
    assert_non_null(op);
    assert_int_equal(op->request, GATTS_REQ_CONNECT);
    gatts_op_destory(op);
}

/* gattc_msg */

void test_gattc_msg_new_normal(FAR void** state)
{
    bt_address_t addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_CONNECT_CHANGE, &addr, 0);
    assert_non_null(msg);
    assert_int_equal(msg->event, GATTC_EVENT_CONNECT_CHANGE);
    assert_memory_equal(&msg->addr, &addr, sizeof(bt_address_t));
    gattc_msg_destory(msg);
}

void test_gattc_msg_new_with_payload(FAR void** state)
{
    bt_address_t addr = { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF } };
    gattc_msg_t* msg = gattc_msg_new(GATTC_EVENT_READ, &addr, 128);
    assert_non_null(msg);
    assert_int_equal(msg->event, GATTC_EVENT_READ);
    gattc_msg_destory(msg);
}

void test_gattc_op_new_normal(FAR void** state)
{
    gattc_op_t* op = gattc_op_new(GATTC_REQ_DISCOVER);
    assert_non_null(op);
    assert_int_equal(op->request, GATTC_REQ_DISCOVER);
    gattc_op_destory(op);
}
