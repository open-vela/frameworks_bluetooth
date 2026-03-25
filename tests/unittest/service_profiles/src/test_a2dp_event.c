/****************************************************************************
 * tests/unittest/service_profiles/src/test_a2dp_event.c
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
#include "a2dp_event.h"

#undef LOG_TAG
#include "../../../../service/profiles/a2dp/a2dp_event.c"

#include "cm_a2dp_event.h"

int test_a2dp_event_setup(FAR void** state)
{
    return 0;
}

int test_a2dp_event_teardown(FAR void** state)
{
    return 0;
}

void test_a2dp_event_new_normal(FAR void** state)
{
    bt_address_t addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    a2dp_event_t* evt = a2dp_event_new(CONNECT_REQ, &addr);
    assert_non_null(evt);
    assert_int_equal(evt->event, CONNECT_REQ);
    assert_memory_equal(&evt->event_data.bd_addr, &addr,
        sizeof(bt_address_t));
    a2dp_event_destory(evt);
}

void test_a2dp_event_new_null_addr(FAR void** state)
{
    a2dp_event_t* evt = a2dp_event_new(A2DP_STARTUP, NULL);
    assert_non_null(evt);
    assert_int_equal(evt->event, A2DP_STARTUP);
    a2dp_event_destory(evt);
}

void test_a2dp_event_new_ext_with_data(FAR void** state)
{
    bt_address_t addr = { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF } };
    uint8_t payload[] = { 0x01, 0x02, 0x03, 0x04 };
    a2dp_event_t* evt = a2dp_event_new_ext(DISCONNECT_REQ, &addr,
        payload, sizeof(payload));
    assert_non_null(evt);
    assert_int_equal(evt->event, DISCONNECT_REQ);
    assert_int_equal(evt->event_data.size, sizeof(payload));
    assert_non_null(evt->event_data.data);
    assert_memory_equal(evt->event_data.data, payload, sizeof(payload));
    a2dp_event_destory(evt);
}

void test_a2dp_event_new_ext_zero_size(FAR void** state)
{
    a2dp_event_t* evt = a2dp_event_new_ext(A2DP_SHUTDOWN, NULL, NULL, 0);
    assert_non_null(evt);
    assert_int_equal(evt->event, A2DP_SHUTDOWN);
    assert_null(evt->event_data.data);
    a2dp_event_destory(evt);
}
