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

typedef enum {
    BT_UUID16_TYPE = 2,
    BT_UUID32_TYPE = 4,
    BT_UUID128_TYPE = 16,
} uuid_type_t;

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

void bt_uuid_to_uuid128(const bt_uuid_t* src, bt_uuid_t* uuid128);
int bt_uuid_compare(const bt_uuid_t* uuid1, const bt_uuid_t* uuid2);
int bt_uuid16_create(bt_uuid_t* uuid16, uint16_t value);
int bt_uuid32_create(bt_uuid_t* uuid32, uint32_t value);
int bt_uuid128_create(bt_uuid_t* uuid128, const uint8_t* value);
bool bt_uuid_create_common(bt_uuid_t* uuid, const uint8_t* data, uint8_t type);
int bt_uuid_to_string(const bt_uuid_t* uuid, char* str, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __BT_UUID_H__ */
