/****************************************************************************
 * tests/unittest/service_profiles/src/test_avrcp_msg.c
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
#include "avrcp_msg.h"

#undef LOG_TAG
#include "../../../../service/profiles/avrcp/avrcp_msg.c"

#include "cm_avrcp_msg.h"

int test_avrcp_msg_setup(FAR void** state)
{
    return 0;
}

int test_avrcp_msg_teardown(FAR void** state)
{
    return 0;
}

void test_avrcp_msg_new_normal(FAR void** state)
{
    bt_address_t addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_STARTUP, &addr);
    assert_non_null(msg);
    assert_int_equal(msg->id, AVRC_STARTUP);
    assert_memory_equal(&msg->addr, &addr, sizeof(bt_address_t));
    free(msg);
}

void test_avrcp_msg_new_null_addr(FAR void** state)
{
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_SHUTDOWN, NULL);
    assert_non_null(msg);
    assert_int_equal(msg->id, AVRC_SHUTDOWN);
    free(msg);
}

void test_avrcp_msg_destroy_with_attrs(FAR void** state)
{
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_GET_ELEMENT_ATTRIBUTES_RSP, NULL);
    assert_non_null(msg);
    msg->data.attrs.count = 2;
    msg->data.attrs.attrs[0] = strdup("title");
    msg->data.attrs.attrs[1] = strdup("artist");
    /* Should free attrs then the msg */
    avrcp_msg_destory(msg);
}

void test_avrcp_msg_destroy_no_attrs(FAR void** state)
{
    avrcp_msg_t* msg = avrcp_msg_new(AVRC_STARTUP, NULL);
    assert_non_null(msg);
    avrcp_msg_destory(msg);
}
