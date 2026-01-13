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
#ifndef __BT_UUID_H__
#define __BT_UUID_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/**
 * @cond
 */

#define BT_UUID_A2DP_SRC 0x110A
#define BT_UUID_A2DP_SNK 0x110B
#define BT_UUID_HFP 0x111E
#define BT_UUID_HFP_AG 0x111F

/* GATT Descriptor UUIDs (16-bit) */
#define BT_UUID_GATT_CEPD 0x2900
#define BT_UUID_GATT_CUDD 0x2901
#define BT_UUID_GATT_CCCD 0x2902
#define BT_UUID_GATT_CPFD 0x2904

#define BT_UUID_RANGING_VAL 0x185B
#define BT_UUID_RANGE_FEAT_VAL 0x2C14
#define BT_UUID_RANGE_RTT_DT_VAL 0x2C15
#define BT_UUID_RANGE_ON_DEM_DT_VAL 0x2C16
#define BT_UUID_RANGE_RAS_CTR_POINT_VAL 0x2C17
#define BT_UUID_RANGE_DT_RD_VAL 0x2C18
#define BT_UUID_RANGE_DT_OV_WR_VAL 0x2C19

typedef enum {
    BT_UUID16_TYPE = 2,
    BT_UUID32_TYPE = 4,
    BT_UUID128_TYPE = 16,
} uuid_type_t;

typedef enum {
    BT_HEAD_UUID16_TYPE = 1,
    BT_HEAD_UUID32_TYPE = 2,
    BT_HEAD_UUID128_TYPE = 3,
} head_uuid_type_t;

typedef struct {
    uint8_t type; /* uuid_type_t */
    uint8_t pad[3];

    union {
        uint16_t u16;
        uint32_t u32;
        uint8_t u128[16];
    } val;
} bt_uuid_t;

#ifndef BT_UUID_DECLARE_16
#define BT_UUID_DECLARE_16(value) \
    ((bt_uuid_t) { .type = BT_UUID16_TYPE, .val.u16 = (value) })
#endif

#ifndef BT_UUID_DECLARE_32
#define BT_UUID_DECLARE_32(value) \
    ((bt_uuid_t) { .type = BT_UUID32_TYPE, .val.u32 = (value) })
#endif

#ifndef BT_UUID_DECLARE_128
#define BT_UUID_DECLARE_128(value...) \
    ((bt_uuid_t) { .type = BT_UUID128_TYPE, .val.u128 = { value } })
#endif

/**
 * @endcond
 */

/**
 * @brief Converting UUID to a 128-bits UUID.
 *
 * @param src - src UUID(16-bits, 32-bits, 128-bits).
 * @param uuid128 - Converted 128-bits UUID.
 * @return void.
 *
 * **Example:**
 * @code
int bt_uuid_compare(const bt_uuid_t* uuid1, const bt_uuid_t* uuid2)
{
    bt_uuid_t u1 = { 0 };
    bt_uuid_t u2 = { 0 };

    bt_uuid_to_uuid128(uuid1, &u1);
    bt_uuid_to_uuid128(uuid2, &u2);

    return bt_uuid128_cmp(&u1, &u2);
}
 * @endcode
 */
void bt_uuid_to_uuid128(const bt_uuid_t* src, bt_uuid_t* uuid128);

void bt_uuid_to_uuid16(const bt_uuid_t* src, bt_uuid_t* uuid16);

/**
 * @brief compare two UUIDs.
 *
 * @param uuid1 - UUID1.
 * @param uuid2 - UUID2.
 * @return int. If uuid1 is equal to uuid2, return 0.
 *
 * **Example:**
 * @code
if (!bt_uuid_compare(&element->uuid, attr_uuid)) {
    BT_LOGE("uuid match");
    // Handle continue
} else {
    BT_LOGE("uuid not match");
    // Handle error
}
 * @endcode
 */
int bt_uuid_compare(const bt_uuid_t* uuid1, const bt_uuid_t* uuid2);

/**
 * @brief create a 16-bits UUID.
 *
 * @param uuid16 - pointer of the generated UUID16.
 * @param value - UUID value.
 * @return int.
 *
 * **Example:**
 * @code
bool bt_uuid_create_common(bt_uuid_t* uuid, const uint8_t* data, uint8_t type)
{
    switch (type) {
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
 * @endcode
 */
int bt_uuid16_create(bt_uuid_t* uuid16, uint16_t value);

/**
 * @brief create a 32-bits UUID.
 *
 * @param uuid16 - pointer of the generated UUID32.
 * @param value - UUID value.
 * @return int.
 *
 * **Example:**
 * @code
bool bt_uuid_create_common(bt_uuid_t* uuid, const uint8_t* data, uint8_t type)
{
    switch (type) {
    case BT_UUID32_TYPE: {
        uint32_t val32;
        STREAM_TO_UINT32(val32, data);
        bt_uuid32_create(uuid, val32);
        break;
    }
    default:
        return false;
    }

    return true;
}
 * @endcode
 */
int bt_uuid32_create(bt_uuid_t* uuid32, uint32_t value);

/**
 * @brief create a 128-bits UUID.
 *
 * @param uuid16 - pointer of the generated UUID128.
 * @param value - UUID value.
 * @return int.
 *
 * **Example:**
 * @code
bool bt_uuid_create_common(bt_uuid_t* uuid, const uint8_t* data, uint8_t type)
{
    switch (type) {
    case BT_UUID128_TYPE:
        bt_uuid128_create(uuid, data);
        break;
    default:
        return false;
    }

    return true;
}
 * @endcode
 */
int bt_uuid128_create(bt_uuid_t* uuid128, const uint8_t* value);

/**
 * @brief Wrapper of UUID create.
 *
 * @param uuid - pointer of the generated UUID(16-bits, 32-bits, 128-bits).
 * @param data - UUID value.
 * @param type - UUID type(16-bits, 32-bits, 128-bits).
 * @return bool. If type belongs to （UUID16, UUID32, UUID128）return true, else return false.
 *
 * **Example:**
 * @code
bt_uuid_create_common(&uuid, data, BT_UUID128_TYPE)
 * @endcode
 */
bool bt_uuid_create_common(bt_uuid_t* uuid, const uint8_t* data, uint8_t type);

/**
 * @brief Converting UUID to string.
 *
 * @param uuid - pointer of the generated UUID.
 * @param str - UUID string.
 * @param len - string length.
 * @return int.
 *
 * **Example:**
 * @code
bt_device_get_uuids(handle, addr, &uuids, &uuid_cnt, bttool_allocator);
if (uuid_cnt) {
    PRINT("\tUUIDs:[%d]", uuid_cnt);
    for (int i = 0; i < uuid_cnt; i++) {
        bt_uuid_to_string(uuids + i, uuid_str, 40);
        PRINT("\t\tuuid[%-2d]: %s", i, uuid_str);
    }
}
free(uuids);
 * @endcode
 */
int bt_uuid_to_string(const bt_uuid_t* uuid, char* str, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BT_UUID_H__ */
