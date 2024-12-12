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

#ifndef __BT_UTILS_H__
#define __BT_UTILS_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

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

#define BE_STREAM_TO_UINT8(u8, p) \
    {                             \
        (u8) = (uint8_t)(*(p));   \
        (p) += 1;                 \
    }
#define BE_STREAM_TO_UINT16(u16, p)                                           \
    {                                                                         \
        (u16) = (uint16_t)(((uint16_t)(*(p)) << 8) + (uint16_t)(*((p) + 1))); \
        (p) += 2;                                                             \
    }
#define BE_STREAM_TO_UINT24(u32, p)                                                                    \
    {                                                                                                  \
        (u32) = (((uint32_t)(*((p) + 2))) + ((uint32_t)(*((p) + 1)) << 8) + ((uint32_t)(*(p)) << 16)); \
        (p) += 3;                                                                                      \
    }
#define BE_STREAM_TO_UINT32(u32, p)                                                                                                   \
    {                                                                                                                                 \
        (u32) = ((uint32_t)(*((p) + 3)) + ((uint32_t)(*((p) + 2)) << 8) + ((uint32_t)(*((p) + 1)) << 16) + ((uint32_t)(*(p)) << 24)); \
        (p) += 4;                                                                                                                     \
    }

#ifdef __cplusplus
}
#endif

#endif