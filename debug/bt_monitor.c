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

#include "bt_monitor.h"

#include <inttypes.h>
#include <nuttx/clock.h>

#include "utils/log.h"

/****************************************************************************
 * Private Functions
 ***************************************************************************/

static inline uint8_t mon_pending(bt_monitor_t* mon)
{
    unsigned int h = atomic_load(&mon->head);
    unsigned int t = atomic_load(&mon->tail);

    return (uint8_t)((h - t) % CONFIG_BLUETOOTH_MONITOR_DEPTH);
}

static inline uint32_t perf_elapsed_us(clock_t start, clock_t end)
{
    struct timespec ts;

    perf_convert(end - start, &ts);
    return (uint32_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

/****************************************************************************
 * Public Functions
 ***************************************************************************/

void bt_monitor_push(bt_monitor_t* mon)
{
    unsigned int h;

    if (mon->stopped) {
        return;
    }

    h = atomic_load(&mon->head);

    if (mon_pending(mon) >= CONFIG_BLUETOOTH_MONITOR_DEPTH - 1) {
        mon->stopped = true;
        return;
    }

    mon->timestamps[h] = perf_gettime();
    atomic_store(&mon->head, (h + 1) % CONFIG_BLUETOOTH_MONITOR_DEPTH);
}

void bt_monitor_pop(bt_monitor_t* mon)
{
    unsigned int t;
    clock_t now;
    clock_t start;
    uint32_t latency_us;

    if (mon->stopped) {
        BT_LOGW("[%s] monitor stopped, ring was full", mon->tag);
        return;
    }

    t = atomic_load(&mon->tail);

    if (t == atomic_load(&mon->head)) {
        return;
    }

    now = perf_gettime();
    start = mon->timestamps[t];
    atomic_store(&mon->tail, (t + 1) % CONFIG_BLUETOOTH_MONITOR_DEPTH);

    latency_us = perf_elapsed_us(start, now);

    if (latency_us >= mon->latency_warn_us) {
        BT_LOGW("[%s] latency %" PRIu32 " us (threshold %" PRIu32 " us)",
            mon->tag, latency_us, mon->latency_warn_us);
    }
}
