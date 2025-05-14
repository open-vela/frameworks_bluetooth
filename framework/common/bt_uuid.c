/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt_utils.h"
#include "bt_uuid.h"
#include "utils/log.h"

static const bt_uuid_t bt_uuid128_base = {
    .type = BT_UUID128_TYPE,
    .val.u128 = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
        0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

#define BASE_UUID16_OFFSET 12
#define BASE_UUID32_OFFSET 12

static void bt_uuid16_to_uuid128(const bt_uuid_t* uuid16, bt_uuid_t* uuid128)
{
    uint8_t uuid[2];
    uint8_t* p = uuid;

    *uuid128 = bt_uuid128_base;
    UINT16_TO_STREAM(p, uuid16->val.u16);

    memcpy(&uuid128->val.u128[BASE_UUID16_OFFSET], uuid, sizeof(uuid));
}

static void bt_uuid128_to_uuid16(const bt_uuid_t* uuid128, bt_uuid_t* uuid16)
{
    uuid16->type = BT_UUID16_TYPE;
    uuid16->val.u16 = (uint16_t)(uuid128->val.u128[BASE_UUID16_OFFSET + 1] << 8 | uuid128->val.u128[BASE_UUID16_OFFSET]);
}

static void bt_uuid32_to_uuid128(const bt_uuid_t* uuid32, bt_uuid_t* uuid128)
{
    uint8_t uuid[4];
    uint8_t* p = uuid;

    *uuid128 = bt_uuid128_base;
    UINT32_TO_STREAM(p, uuid32->val.u32);
    memcpy(&uuid128->val.u128[BASE_UUID32_OFFSET], uuid, sizeof(uuid));
}

void bt_uuid_to_uuid128(const bt_uuid_t* src, bt_uuid_t* uuid128)
{
    switch (src->type) {
    case BT_UUID128_TYPE:
        *uuid128 = *src;
        break;
    case BT_UUID32_TYPE:
        bt_uuid32_to_uuid128(src, uuid128);
        break;
    case BT_UUID16_TYPE:
        bt_uuid16_to_uuid128(src, uuid128);
        break;
    default:
        break;
    }
}

void bt_uuid_to_uuid16(const bt_uuid_t* src, bt_uuid_t* uuid16)
{
    switch (src->type) {
    case BT_UUID128_TYPE:
        bt_uuid128_to_uuid16(src, uuid16);
        break;
    case BT_UUID32_TYPE:
        BT_LOGE("uuid32 to uuid16 not supported!");
        break;
    case BT_UUID16_TYPE:
        *uuid16 = *src;
        break;
    default:
        break;
    }
}

static int bt_uuid128_cmp(const bt_uuid_t* u1, const bt_uuid_t* u2)
{
    return memcmp(&u1->val.u128, &u2->val.u128, 16);
}

int bt_uuid16_create(bt_uuid_t* uuid16, uint16_t value)
{
    memset(uuid16, 0, sizeof(bt_uuid_t));
    uuid16->type = BT_UUID16_TYPE;
    uuid16->val.u16 = value;

    return 0;
}

int bt_uuid32_create(bt_uuid_t* uuid32, uint32_t value)
{
    memset(uuid32, 0, sizeof(bt_uuid_t));
    uuid32->type = BT_UUID32_TYPE;
    uuid32->val.u32 = value;

    return 0;
}

int bt_uuid128_create(bt_uuid_t* uuid128, const uint8_t* value)
{
    memset(uuid128, 0, sizeof(bt_uuid_t));
    uuid128->type = BT_UUID128_TYPE;
    memcpy(uuid128->val.u128, value, 16);

    return 0;
}

bool bt_uuid_create_common(bt_uuid_t* uuid, const uint8_t* data, uint8_t type)
{
    switch (type) {
    case BT_UUID128_TYPE:
        bt_uuid128_create(uuid, data);
        break;
    case BT_UUID32_TYPE: {
        uint32_t val32;
        STREAM_TO_UINT32(val32, data);
        bt_uuid32_create(uuid, val32);
        break;
    }
    case BT_UUID16_TYPE: {
        uint16_t val16;
        STREAM_TO_UINT32(val16, data);
        bt_uuid16_create(uuid, val16);
        break;
    }
    default:
        return false;
    }

    return true;
}

int bt_uuid_compare(const bt_uuid_t* uuid1, const bt_uuid_t* uuid2)
{
    bt_uuid_t u1 = { 0 };
    bt_uuid_t u2 = { 0 };

    bt_uuid_to_uuid128(uuid1, &u1);
    bt_uuid_to_uuid128(uuid2, &u2);

    return bt_uuid128_cmp(&u1, &u2);
}

int bt_uuid_to_string(const bt_uuid_t* uuid, char* str, uint32_t len)
{
    bt_uuid_t uuid128 = { 0 };
    uint32_t tmp1, tmp5;
    uint16_t tmp0, tmp2, tmp3, tmp4;
    const uint8_t* p;

    bt_uuid_to_uuid128(uuid, &uuid128);
    p = (uint8_t*)&uuid128.val.u128;

    STREAM_TO_UINT16(tmp0, p);
    STREAM_TO_UINT32(tmp1, p);
    STREAM_TO_UINT16(tmp2, p);
    STREAM_TO_UINT16(tmp3, p);
    STREAM_TO_UINT16(tmp4, p);
    STREAM_TO_UINT32(tmp5, p);

    snprintf(str, len, "%08" PRIx32 "-%04x-%04x-%04x-%08" PRIx32 "%04x", tmp5, tmp4, tmp3, tmp2, tmp1, tmp0);

    return 0;
}
