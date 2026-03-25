/****************************************************************************
 * tests/unittest/framework_common/src/test_advertiser_data.c
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

/* Include source directly to test internal structures */

#include "../../../../framework/common/advertiser_data.c"

#include "cm_advertiser_data.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_advertiser_data_setup(FAR void** state)
{
    return 0;
}

int test_advertiser_data_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

void test_advertiser_data_new_normal(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();

    assert_non_null(ad);
    assert_non_null(ad->data);
    assert_null(ad->buffer);

    advertiser_data_free(ad);
}

void test_advertiser_data_set_flags_normal(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;

    assert_non_null(ad);
    advertiser_data_set_flags(ad, BT_AD_FLAG_GENERAL_DISCOVERABLE | BT_AD_FLAG_BREDR_NOT_SUPPORT);

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* Expected: len=2, type=0x01, data=0x06 */

    assert_int_equal(len, 3);
    assert_int_equal(buf[0], 2);
    assert_int_equal(buf[1], BT_AD_FLAGS);
    assert_int_equal(buf[2], 0x06);

    advertiser_data_free(ad);
}

void test_advertiser_data_set_name_short(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;

    /* Name longer than BT_LE_AD_NAME_LEN (29) should be truncated */

    const char* long_name = "ThisIsAVeryLongDeviceNameThatExceedsLimit";

    assert_non_null(ad);
    advertiser_data_set_name(ad, long_name);

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* type should be BT_AD_NAME_SHORT (0x08) */

    assert_int_equal(buf[1], BT_AD_NAME_SHORT);

    /* data length = BT_LE_AD_NAME_LEN + 1 (type byte) */

    assert_int_equal(buf[0], BT_LE_AD_NAME_LEN + 1);

    advertiser_data_free(ad);
}

void test_advertiser_data_set_name_complete(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;
    const char* name = "ShortName";

    assert_non_null(ad);
    advertiser_data_set_name(ad, name);

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* type should be BT_AD_NAME_COMPLETE (0x09) */

    assert_int_equal(buf[1], BT_AD_NAME_COMPLETE);
    assert_int_equal(buf[0], strlen(name) + 1);
    assert_memory_equal(&buf[2], name, strlen(name));

    advertiser_data_free(ad);
}

void test_advertiser_data_set_appearance_normal(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;

    assert_non_null(ad);
    advertiser_data_set_appearance(ad, 0x03C0);

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* len=3, type=0x19, data=LE16(0x03C0) */

    assert_int_equal(len, 4);
    assert_int_equal(buf[0], 3);
    assert_int_equal(buf[1], BT_AD_GAP_APPEARANCE);
    assert_int_equal(buf[2], 0xC0);
    assert_int_equal(buf[3], 0x03);

    advertiser_data_free(ad);
}

void test_advertiser_data_add_data_normal(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;
    uint8_t payload[] = { 0xAA, 0xBB, 0xCC };

    assert_non_null(ad);
    advertiser_data_add_data(ad, 0xFE, payload, sizeof(payload));

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    advertiser_data_free(ad);
}

void test_advertiser_data_add_manufacture_data_normal(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;
    uint8_t mdata[] = { 0x12, 0x34 };

    assert_non_null(ad);
    advertiser_data_add_manufacture_data(ad, 0x038F, mdata, sizeof(mdata));

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* len = 2(mfg_id) + 2(data) + 1(type) = 5 */

    assert_int_equal(buf[0], 5);
    assert_int_equal(buf[1], BT_AD_MANUFACTURER_DATA);

    /* manufacture_id in LE: 0x8F, 0x03 */

    assert_int_equal(buf[2], 0x8F);
    assert_int_equal(buf[3], 0x03);
    assert_int_equal(buf[4], 0x12);
    assert_int_equal(buf[5], 0x34);

    advertiser_data_free(ad);
}

void test_advertiser_data_add_service_uuid16(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;
    bt_uuid_t uuid;

    assert_non_null(ad);
    uuid.type = BT_UUID16_TYPE;
    uuid.val.u16 = 0x180D;

    assert_true(advertiser_data_add_service_uuid(ad, &uuid));

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* len=3, type=BT_AD_UUID16_ALL, data=LE16(0x180D) */

    assert_int_equal(buf[0], 3);
    assert_int_equal(buf[1], BT_AD_UUID16_ALL);
    assert_int_equal(buf[2], 0x0D);
    assert_int_equal(buf[3], 0x18);

    advertiser_data_free(ad);
}

void test_advertiser_data_add_service_uuid128(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;
    bt_uuid_t uuid;
    uint8_t u128[16] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10 };

    assert_non_null(ad);
    uuid.type = BT_UUID128_TYPE;
    memcpy(uuid.val.u128, u128, 16);

    assert_true(advertiser_data_add_service_uuid(ad, &uuid));

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* len=17, type=BT_AD_UUID128_ALL */

    assert_int_equal(buf[0], 17);
    assert_int_equal(buf[1], BT_AD_UUID128_ALL);
    assert_memory_equal(&buf[2], u128, 16);

    advertiser_data_free(ad);
}

void test_advertiser_data_add_service_uuid_invalid(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    bt_uuid_t uuid;

    assert_non_null(ad);
    uuid.type = 0xFF; /* invalid type */

    assert_false(advertiser_data_add_service_uuid(ad, &uuid));

    advertiser_data_free(ad);
}

void test_advertiser_data_build_empty(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;

    assert_non_null(ad);

    buf = advertiser_data_build(ad, &len);

    /* Empty list returns NULL */

    assert_null(buf);

    advertiser_data_free(ad);
}

void test_advertiser_data_build_with_flags(FAR void** state)
{
    advertiser_data_t* ad = advertiser_data_new();
    uint16_t len = 0;
    uint8_t* buf;

    assert_non_null(ad);
    advertiser_data_set_flags(ad, 0x06);
    advertiser_data_set_appearance(ad, 0x0040);

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);

    /* flags: 3 bytes + appearance: 4 bytes = 7 total */

    assert_int_equal(len, 7);

    /* Rebuild should free old buffer and create new one */

    buf = advertiser_data_build(ad, &len);
    assert_non_null(buf);
    assert_int_equal(len, 7);

    advertiser_data_free(ad);
}
