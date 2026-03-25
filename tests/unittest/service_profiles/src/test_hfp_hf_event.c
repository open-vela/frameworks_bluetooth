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

#include "bluetooth.h"
#include "hfp_hf_event.h"


#include "cm_hfp_hf_event.h"

int test_hfp_hf_event_setup(FAR void** state)
{
    return 0;
}

int test_hfp_hf_event_teardown(FAR void** state)
{
    return 0;
}

void test_hfp_hf_msg_new_normal(FAR void** state)
{
    bt_address_t addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 } };
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_CONNECT, &addr);
    assert_non_null(msg);
    assert_int_equal(msg->event, HF_CONNECT);
    assert_memory_equal(&msg->data.addr, &addr, sizeof(bt_address_t));
    hfp_hf_msg_destroy(msg);
}

void test_hfp_hf_msg_new_null_addr(FAR void** state)
{
    hfp_hf_msg_t* msg = hfp_hf_msg_new(HF_STARTUP, NULL);
    assert_non_null(msg);
    assert_int_equal(msg->event, HF_STARTUP);
    hfp_hf_msg_destroy(msg);
}

void test_hfp_hf_msg_new_ext_with_data(FAR void** state)
{
    bt_address_t addr = { { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF } };
    uint8_t payload[] = { 0x01, 0x02, 0x03 };
    hfp_hf_msg_t* msg = hfp_hf_msg_new_ext(HF_DISCONNECT, &addr,
        payload, sizeof(payload));
    assert_non_null(msg);
    assert_int_equal(msg->event, HF_DISCONNECT);
    assert_int_equal(msg->data.size, sizeof(payload));
    assert_non_null(msg->data.data);
    assert_memory_equal(msg->data.data, payload, sizeof(payload));
    hfp_hf_msg_destroy(msg);
}

void test_hfp_hf_msg_destroy_null(FAR void** state)
{
    hfp_hf_msg_destroy(NULL);
}
