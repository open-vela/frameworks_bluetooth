/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __BT_UTILS_H__
#define __BT_UTILS_H__

#include "btm_manager.h"

#define CASE_RETURN_STR(const) \
    case const:                \
        return #const;

#define DEFAULT_BREAK() \
    default:            \
        break;

#define UINT64_TO_BE_STREAM(p, u64)      \
    {                                    \
        *(p)++ = (uint8_t)((u64) >> 56); \
        *(p)++ = (uint8_t)((u64) >> 48); \
        *(p)++ = (uint8_t)((u64) >> 40); \
        *(p)++ = (uint8_t)((u64) >> 32); \
        *(p)++ = (uint8_t)((u64) >> 24); \
        *(p)++ = (uint8_t)((u64) >> 16); \
        *(p)++ = (uint8_t)((u64) >> 8);  \
        *(p)++ = (uint8_t)(u64);         \
    }
#define UINT32_TO_STREAM(p, u32)         \
    {                                    \
        *(p)++ = (uint8_t)(u32);         \
        *(p)++ = (uint8_t)((u32) >> 8);  \
        *(p)++ = (uint8_t)((u32) >> 16); \
        *(p)++ = (uint8_t)((u32) >> 24); \
    }
#define UINT24_TO_STREAM(p, u24)         \
    {                                    \
        *(p)++ = (uint8_t)(u24);         \
        *(p)++ = (uint8_t)((u24) >> 8);  \
        *(p)++ = (uint8_t)((u24) >> 16); \
    }
#define UINT16_TO_STREAM(p, u16)        \
    {                                   \
        *(p)++ = (uint8_t)(u16);        \
        *(p)++ = (uint8_t)((u16) >> 8); \
    }
#define UINT8_TO_STREAM(p, u8)  \
    {                           \
        *(p)++ = (uint8_t)(u8); \
    }
#define INT8_TO_STREAM(p, u8)  \
    {                          \
        *(p)++ = (int8_t)(u8); \
    }
#define ARRAY16_TO_STREAM(p, a)              \
    {                                        \
        int ijk;                             \
        for (ijk = 0; ijk < 16; ijk++)       \
            *(p)++ = (uint8_t)(a)[15 - ijk]; \
    }
#define ARRAY8_TO_STREAM(p, a)              \
    {                                       \
        int ijk;                            \
        for (ijk = 0; ijk < 8; ijk++)       \
            *(p)++ = (uint8_t)(a)[7 - ijk]; \
    }
#define LAP_TO_STREAM(p, a)                           \
    {                                                 \
        int ijk;                                      \
        for (ijk = 0; ijk < LAP_LEN; ijk++)           \
            *(p)++ = (uint8_t)(a)[LAP_LEN - 1 - ijk]; \
    }
#define ARRAY_TO_STREAM(p, a, len)        \
    {                                     \
        int ijk;                          \
        for (ijk = 0; ijk < (len); ijk++) \
            *(p)++ = (uint8_t)(a)[ijk];   \
    }
#define STREAM_TO_INT8(u8, p)     \
    {                             \
        (u8) = (*((int8_t*)(p))); \
        (p) += 1;                 \
    }
#define STREAM_TO_UINT8(u8, p)  \
    {                           \
        (u8) = (uint8_t)(*(p)); \
        (p) += 1;               \
    }
#define STREAM_TO_UINT16(u16, p)                                      \
    {                                                                 \
        (u16) = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); \
        (p) += 2;                                                     \
    }
#define STREAM_TO_UINT24(u32, p)                                                                               \
    {                                                                                                          \
        (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16)); \
        (p) += 3;                                                                                              \
    }
#define STREAM_TO_UINT32(u32, p)                                                                                                                    \
    {                                                                                                                                               \
        (u32) = (((uint32_t)(*(p))) + ((((uint32_t)(*((p) + 1)))) << 8) + ((((uint32_t)(*((p) + 2)))) << 16) + ((((uint32_t)(*((p) + 3)))) << 24)); \
        (p) += 4;                                                                                                                                   \
    }
#define STREAM_TO_UINT64(u64, p)                                                                                                                                                                                                                                                                        \
    {                                                                                                                                                                                                                                                                                                   \
        (u64) = (((uint64_t)(*(p))) + ((((uint64_t)(*((p) + 1)))) << 8) + ((((uint64_t)(*((p) + 2)))) << 16) + ((((uint64_t)(*((p) + 3)))) << 24) + ((((uint64_t)(*((p) + 4)))) << 32) + ((((uint64_t)(*((p) + 5)))) << 40) + ((((uint64_t)(*((p) + 6)))) << 48) + ((((uint64_t)(*((p) + 7)))) << 56)); \
        (p) += 8;                                                                                                                                                                                                                                                                                       \
    }
#define STREAM_TO_ARRAY16(a, p)            \
    {                                      \
        int ijk;                           \
        uint8_t* _pa = (uint8_t*)(a) + 15; \
        for (ijk = 0; ijk < 16; ijk++)     \
            *_pa-- = *(p)++;               \
    }
#define STREAM_TO_ARRAY8(a, p)            \
    {                                     \
        int ijk;                          \
        uint8_t* _pa = (uint8_t*)(a) + 7; \
        for (ijk = 0; ijk < 8; ijk++)     \
            *_pa-- = *(p)++;              \
    }
#define STREAM_TO_LAP(a, p)                          \
    {                                                \
        int ijk;                                     \
        uint8_t* plap = (uint8_t*)(a) + LAP_LEN - 1; \
        for (ijk = 0; ijk < LAP_LEN; ijk++)          \
            *plap-- = *(p)++;                        \
    }
#define STREAM_TO_ARRAY(a, p, len)         \
    {                                      \
        int ijk;                           \
        for (ijk = 0; ijk < (len); ijk++)  \
            ((uint8_t*)(a))[ijk] = *(p)++; \
    }
#define STREAM_SKIP_UINT8(p) \
    do {                     \
        (p) += 1;            \
    } while (0)
#define STREAM_SKIP_UINT16(p) \
    do {                      \
        (p) += 2;             \
    } while (0)
#define STREAM_SKIP_UINT32(p) \
    do {                      \
        (p) += 4;             \
    } while (0)

int ba2str(bt_address addr, char* str);
int str2ba(const char* str, bt_address addr);
int str2hex(const char* str, char* hex, int len);
char* addr_str(bt_address addr);
char * uuid_str(bt_uuid_t uuid);
bool uuid_is_empty(bt_uuid_t uuid);
#endif