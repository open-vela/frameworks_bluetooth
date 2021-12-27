/* uuid.c - Bluetooth UUID handling */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "uuid.h"

#define UUID_16_BASE_OFFSET 12

/* TODO: Decide whether to continue using BLE format or switch to RFC 4122 */

/* Base UUID : 0000[0000]-0000-1000-8000-00805F9B34FB
 * 0x2800    : 0000[2800]-0000-1000-8000-00805F9B34FB
 *  little endian 0x2800 : [00 28] -> no swapping required
 *  big endian 0x2800    : [28 00] -> swapping required
 */
static const struct bt_uuid_128 uuid128_base = {
    .uuid = { BT_UUID_TYPE_128 },
    .val = { BT_UUID_128_ENCODE(0x00000000, 0x0000, 0x1000, 0x8000, 0x00805F9B34FB) }
};

/**
 *  @brief Put a 16-bit integer as little-endian to arbitrary location.
 *
 *  Put a 16-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in little-endian format.
 *
 *  @param val 16-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_le16(uint16_t val, uint8_t dst[2])
{
    dst[0] = val;
    dst[1] = val >> 8;
}

/**
 *  @brief Put a 32-bit integer as little-endian to arbitrary location.
 *
 *  Put a 32-bit integer, originally in host endianness, to a
 *  potentially unaligned memory location in little-endian format.
 *
 *  @param val 32-bit integer in host endianness.
 *  @param dst Destination memory address to store the result.
 */
static inline void sys_put_le32(uint32_t val, uint8_t dst[4])
{
    sys_put_le16(val, dst);
    sys_put_le16(val >> 16, &dst[2]);
}

/**
 *  @brief Get a 16-bit integer stored in little-endian format.
 *
 *  Get a 16-bit integer, stored in little-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the little-endian 16-bit integer to get.
 *
 *  @return 16-bit integer in host endianness.
 */
static inline uint16_t sys_get_le16(const uint8_t src[2])
{
    return ((uint16_t)src[1] << 8) | src[0];
}

/**
 *  @brief Get a 32-bit integer stored in little-endian format.
 *
 *  Get a 32-bit integer, stored in little-endian format in a potentially
 *  unaligned memory location, and convert it to the host endianness.
 *
 *  @param src Location of the little-endian 32-bit integer to get.
 *
 *  @return 32-bit integer in host endianness.
 */
static inline uint32_t sys_get_le32(const uint8_t src[4])
{
    return ((uint32_t)sys_get_le16(&src[2]) << 16) | sys_get_le16(&src[0]);
}

void uuid_to_uuid128(const struct bt_uuid* src, struct bt_uuid_128* dst)
{
    switch (src->type) {
    case BT_UUID_TYPE_16:
        *dst = uuid128_base;
        sys_put_le16(BT_UUID_16(src)->val,
            &dst->val[UUID_16_BASE_OFFSET]);
        return;
    case BT_UUID_TYPE_32:
        *dst = uuid128_base;
        sys_put_le32(BT_UUID_32(src)->val,
            &dst->val[UUID_16_BASE_OFFSET]);
        return;
    case BT_UUID_TYPE_128:
        memcpy(dst, src, sizeof(*dst));
        return;
    }
}

static int uuid128_cmp(const struct bt_uuid* u1, const struct bt_uuid* u2)
{
    struct bt_uuid_128 uuid1, uuid2;

    uuid_to_uuid128(u1, &uuid1);
    uuid_to_uuid128(u2, &uuid2);

    return memcmp(uuid1.val, uuid2.val, 16);
}

int bt_utils_uuid_cmp(const struct bt_uuid* u1, const struct bt_uuid* u2)
{
    /* Convert to 128 bit if types don't match */
    if (u1->type != u2->type) {
        return uuid128_cmp(u1, u2);
    }

    switch (u1->type) {
    case BT_UUID_TYPE_16:
        return (int)BT_UUID_16(u1)->val - (int)BT_UUID_16(u2)->val;
    case BT_UUID_TYPE_32:
        return (int)BT_UUID_32(u1)->val - (int)BT_UUID_32(u2)->val;
    case BT_UUID_TYPE_128:
        return memcmp(BT_UUID_128(u1)->val, BT_UUID_128(u2)->val, 16);
    }

    return -EINVAL;
}

bool bt_utils_uuid_create(struct bt_uuid* uuid, const uint8_t* data, uint8_t data_len)
{
    /* Copy UUID from packet data/internal variable to internal bt_uuid */
    switch (data_len) {
    case 2:
        uuid->type = BT_UUID_TYPE_16;
        BT_UUID_16(uuid)->val = sys_get_le16(data);
        break;
    case 4:
        uuid->type = BT_UUID_TYPE_32;
        BT_UUID_32(uuid)->val = sys_get_le32(data);
        break;
    case 16:
        uuid->type = BT_UUID_TYPE_128;
        memcpy(&BT_UUID_128(uuid)->val, data, 16);
        break;
    default:
        return false;
    }
    return true;
}

void bt_utils_uuid_to_str(const struct bt_uuid* uuid, char* str, size_t len)
{
    uint32_t tmp1, tmp5;
    uint16_t tmp0, tmp2, tmp3, tmp4;

    switch (uuid->type) {
    case BT_UUID_TYPE_16:
        snprintk(str, len, "%04x", BT_UUID_16(uuid)->val);
        break;
    case BT_UUID_TYPE_32:
        snprintk(str, len, "%08lx", BT_UUID_32(uuid)->val);
        break;
    case BT_UUID_TYPE_128:
        memcpy(&tmp0, &BT_UUID_128(uuid)->val[0], sizeof(tmp0));
        memcpy(&tmp1, &BT_UUID_128(uuid)->val[2], sizeof(tmp1));
        memcpy(&tmp2, &BT_UUID_128(uuid)->val[6], sizeof(tmp2));
        memcpy(&tmp3, &BT_UUID_128(uuid)->val[8], sizeof(tmp3));
        memcpy(&tmp4, &BT_UUID_128(uuid)->val[10], sizeof(tmp4));
        memcpy(&tmp5, &BT_UUID_128(uuid)->val[12], sizeof(tmp5));

        snprintk(str, len, "%08lx-%04x-%04x-%04x-%08lx%04x",
            tmp5, tmp4, tmp3, tmp2, tmp1, tmp0);
        break;
    default:
        (void)memset(str, 0, len);
        return;
    }
}
