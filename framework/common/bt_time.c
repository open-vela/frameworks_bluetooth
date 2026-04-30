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
#include <stdio.h>
#include <time.h>

#include "bt_time.h"

uint64_t bt_get_os_timestamp_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);

    return (uint64_t)(((uint64_t)ts.tv_sec * 1000000L) + ((uint64_t)ts.tv_nsec / 1000));
}

uint32_t bt_get_os_timestamp_ms(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);

    return (uint32_t)((ts.tv_sec * 1000) + (ts.tv_nsec / 1000000UL));
}

const char* bt_slots_to_time_str(char* buf, uint16_t size, uint16_t slots)
{
    if (!buf || size == 0)
        return "";

    /* 1 Bluetooth slot = 625 microseconds */
    uint32_t total_us = (uint32_t)slots * 625U;
    uint32_t total_ms = total_us / 1000U;
    uint32_t sec = total_us / 1000000U;
    int needed;

    if (sec) {
        /* print seconds with 6-digit fractional part (microseconds) */
        uint32_t frac = total_us % 1000000U;
        needed = snprintf(buf, size, "%" PRIu32 ".%06" PRIu32 " s", sec, frac);
    } else {
        /* print milliseconds with 3-digit fractional part */
        uint32_t frac = total_us % 1000U;
        needed = snprintf(buf, size, "%" PRIu32 ".%03" PRIu32 " ms", total_ms, frac);
    }

    if (needed < 0 || needed >= (int)size)
        return "<oversize>";

    return buf;
}