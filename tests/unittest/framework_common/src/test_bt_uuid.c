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

#include <string.h>

#include "bt_uuid.h"

#include "cm_bt_uuid.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_uuid_setup(FAR void** state)
{
    return 0;
}

int test_bt_uuid_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid16_create
 ****************************************************************************/

void test_bt_uuid16_create_normal(FAR void** state)
{
    bt_uuid_t uuid;
    int ret;

    ret = bt_uuid16_create(&uuid, 0x110A);
    assert_int_equal(ret, 0);
    assert_int_equal(uuid.type, BT_UUID16_TYPE);
    assert_int_equal(uuid.val.u16, 0x110A);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid32_create
 ****************************************************************************/

void test_bt_uuid32_create_normal(FAR void** state)
{
    bt_uuid_t uuid;
    int ret;

    ret = bt_uuid32_create(&uuid, 0x12345678);
    assert_int_equal(ret, 0);
    assert_int_equal(uuid.type, BT_UUID32_TYPE);
    assert_int_equal(uuid.val.u32, 0x12345678);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid128_create
 ****************************************************************************/

void test_bt_uuid128_create_normal(FAR void** state)
{
    bt_uuid_t uuid;
    uint8_t val[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
    int ret;

    ret = bt_uuid128_create(&uuid, val);
    assert_int_equal(ret, 0);
    assert_int_equal(uuid.type, BT_UUID128_TYPE);
    assert_memory_equal(uuid.val.u128, val, 16);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid_create_common
 ****************************************************************************/

void test_bt_uuid_create_common_uuid16(FAR void** state)
{
    bt_uuid_t uuid;
    uint8_t data[2] = { 0x0A, 0x11 }; /* 0x110A in LE */
    bool ret;

    ret = bt_uuid_create_common(&uuid, data, BT_UUID16_TYPE);
    assert_true(ret);
    assert_int_equal(uuid.type, BT_UUID16_TYPE);
}

void test_bt_uuid_create_common_uuid32(FAR void** state)
{
    bt_uuid_t uuid;
    uint8_t data[4] = { 0x78, 0x56, 0x34, 0x12 }; /* 0x12345678 in LE */
    bool ret;

    ret = bt_uuid_create_common(&uuid, data, BT_UUID32_TYPE);
    assert_true(ret);
    assert_int_equal(uuid.type, BT_UUID32_TYPE);
}

void test_bt_uuid_create_common_uuid128(FAR void** state)
{
    bt_uuid_t uuid;
    uint8_t data[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };
    bool ret;

    ret = bt_uuid_create_common(&uuid, data, BT_UUID128_TYPE);
    assert_true(ret);
    assert_int_equal(uuid.type, BT_UUID128_TYPE);
    assert_memory_equal(uuid.val.u128, data, 16);
}

void test_bt_uuid_create_common_invalid_type(FAR void** state)
{
    bt_uuid_t uuid;
    uint8_t data[2] = { 0x00, 0x00 };
    bool ret;

    ret = bt_uuid_create_common(&uuid, data, 0xFF);
    assert_false(ret);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid_to_uuid128
 ****************************************************************************/

void test_bt_uuid_to_uuid128_from_uuid16(FAR void** state)
{
    bt_uuid_t src;
    bt_uuid_t dst;

    bt_uuid16_create(&src, 0x110A);
    bt_uuid_to_uuid128(&src, &dst);
    assert_int_equal(dst.type, BT_UUID128_TYPE);

    /* Verify the UUID16 value is at offset 12-13 in the base UUID */

    assert_int_equal(dst.val.u128[12], 0x0A);
    assert_int_equal(dst.val.u128[13], 0x11);
}

void test_bt_uuid_to_uuid128_from_uuid32(FAR void** state)
{
    bt_uuid_t src;
    bt_uuid_t dst;

    bt_uuid32_create(&src, 0x12345678);
    bt_uuid_to_uuid128(&src, &dst);
    assert_int_equal(dst.type, BT_UUID128_TYPE);
}

void test_bt_uuid_to_uuid128_from_uuid128(FAR void** state)
{
    bt_uuid_t src;
    bt_uuid_t dst;
    uint8_t val[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };

    bt_uuid128_create(&src, val);
    bt_uuid_to_uuid128(&src, &dst);
    assert_int_equal(dst.type, BT_UUID128_TYPE);
    assert_memory_equal(dst.val.u128, val, 16);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid_to_uuid16
 ****************************************************************************/

void test_bt_uuid_to_uuid16_from_uuid128(FAR void** state)
{
    bt_uuid_t src;
    bt_uuid_t uuid16;
    bt_uuid_t intermediate;

    /* Create a UUID16, convert to 128, then back to 16 */

    bt_uuid16_create(&src, 0x110A);
    bt_uuid_to_uuid128(&src, &intermediate);
    bt_uuid_to_uuid16(&intermediate, &uuid16);
    assert_int_equal(uuid16.type, BT_UUID16_TYPE);
    assert_int_equal(uuid16.val.u16, 0x110A);
}

void test_bt_uuid_to_uuid16_from_uuid16(FAR void** state)
{
    bt_uuid_t src;
    bt_uuid_t dst;

    bt_uuid16_create(&src, 0x2902);
    bt_uuid_to_uuid16(&src, &dst);
    assert_int_equal(dst.type, BT_UUID16_TYPE);
    assert_int_equal(dst.val.u16, 0x2902);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid_compare
 ****************************************************************************/

void test_bt_uuid_compare_equal(FAR void** state)
{
    bt_uuid_t a;
    bt_uuid_t b;

    bt_uuid16_create(&a, 0x110A);
    bt_uuid16_create(&b, 0x110A);
    assert_int_equal(bt_uuid_compare(&a, &b), 0);
}

void test_bt_uuid_compare_not_equal(FAR void** state)
{
    bt_uuid_t a;
    bt_uuid_t b;

    bt_uuid16_create(&a, 0x110A);
    bt_uuid16_create(&b, 0x110B);
    assert_int_not_equal(bt_uuid_compare(&a, &b), 0);
}

void test_bt_uuid_compare_cross_type(FAR void** state)
{
    bt_uuid_t u16;
    bt_uuid_t u128;

    /* A UUID16 and its UUID128 equivalent should be equal */

    bt_uuid16_create(&u16, 0x110A);
    bt_uuid_to_uuid128(&u16, &u128);
    assert_int_equal(bt_uuid_compare(&u16, &u128), 0);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_uuid_to_string
 ****************************************************************************/

void test_bt_uuid_to_string_uuid16(FAR void** state)
{
    bt_uuid_t uuid;
    char str[BT_UUID_STR_LENGTH];
    int ret;

    bt_uuid16_create(&uuid, 0x110A);
    ret = bt_uuid_to_string(&uuid, str, sizeof(str));
    assert_int_equal(ret, 0);

    /* String should be a valid UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */

    assert_int_equal(strlen(str), 36);
    assert_int_equal(str[8], '-');
    assert_int_equal(str[13], '-');
}

void test_bt_uuid_to_string_uuid128(FAR void** state)
{
    bt_uuid_t uuid;
    uint8_t val[16] = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
        0x00, 0x10, 0x00, 0x00, 0x0A, 0x11, 0x00, 0x00 };
    char str[BT_UUID_STR_LENGTH];
    int ret;

    bt_uuid128_create(&uuid, val);
    ret = bt_uuid_to_string(&uuid, str, sizeof(str));
    assert_int_equal(ret, 0);
    assert_int_equal(strlen(str), 36);
}
