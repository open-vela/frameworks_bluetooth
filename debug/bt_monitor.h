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

#ifndef _BT_MONITOR_H_
#define _BT_MONITOR_H_

#include <nuttx/config.h>

#ifdef CONFIG_BLUETOOTH_MONITOR

/****************************************************************************
 * Included Files
 ***************************************************************************/

#include <nuttx/clock.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

/****************************************************************************
 * Public Types
 ***************************************************************************/

typedef struct {
    const char* tag;
    clock_t timestamps[CONFIG_BLUETOOTH_MONITOR_DEPTH];
    atomic_uint head;
    atomic_uint tail;
    uint32_t latency_warn_us;
    bool stopped;
} bt_monitor_t;

/****************************************************************************
 * Macros
 ***************************************************************************/

/****************************************************************************
 * BT_MONITOR_DEFINE — define and initialize a monitor variable
 *
 * Declares a bt_monitor_t variable with static initializer. No runtime
 * init needed. Use for file-scope or function-scope monitors.
 *
 * NOTE: the variable is NOT declared 'static' — caller controls linkage:
 *   - File-local:  static BT_MONITOR_DEFINE(mon, "tag", 1000);
 *   - Global:      BT_MONITOR_DEFINE(g_mon, "tag", 1000);
 *                   // extern bt_monitor_t g_mon; in header
 ***************************************************************************/

#define BT_MONITOR_DEFINE(var, _tag, _latency_us) \
    bt_monitor_t var = {                          \
        .tag = (_tag),                            \
        .latency_warn_us = (_latency_us),         \
    }

/****************************************************************************
 * BT_MONITOR_FIELD — embed a monitor in a struct definition
 *
 * Expands to a bt_monitor_t member when enabled, nothing when disabled.
 * No #ifdef needed in the struct body.
 *
 * Example:
 *   typedef struct {
 *       int id;
 *       BT_MONITOR_FIELD(mon);
 *   } my_ctx_t;
 ***************************************************************************/

#define BT_MONITOR_FIELD(name) bt_monitor_t name

/****************************************************************************
 * BT_MONITOR_INIT — runtime init for embedded (BT_MONITOR_FIELD) instances
 *
 * Use after the containing struct is zeroed (calloc / memset).
 * Only sets tag and threshold; other fields are already zero.
 *
 * Example:
 *   my_ctx_t *c = calloc(1, sizeof(*c));
 *   BT_MONITOR_INIT(&c->mon, "my_tag", 1000);
 ***************************************************************************/

#define BT_MONITOR_INIT(mon, _tag, _latency_us) \
    do {                                        \
        (mon)->tag = (_tag);                    \
        (mon)->latency_warn_us = (_latency_us); \
    } while (0)

/****************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

/****************************************************************************
 * bt_monitor_push — record async operation start
 *
 * IRQ-safe. Only writes a perf_gettime() timestamp into the ring buffer.
 * If the ring is full, sets stopped flag and returns silently.
 ***************************************************************************/

void bt_monitor_push(bt_monitor_t* mon);

/****************************************************************************
 * bt_monitor_pop — record async operation completion
 *
 * Must be called from task context. Dequeues the oldest timestamp,
 * calculates elapsed time, and logs a warning if threshold is exceeded.
 ***************************************************************************/

void bt_monitor_pop(bt_monitor_t* mon);

#else /* !CONFIG_BLUETOOTH_MONITOR */

#define BT_MONITOR_DEFINE(var, _tag, _latency_us)
#define BT_MONITOR_FIELD(name)
#define BT_MONITOR_INIT(mon, _tag, _latency_us)
#define bt_monitor_push(mon)
#define bt_monitor_pop(mon)

#endif /* CONFIG_BLUETOOTH_MONITOR */

#endif /* _BT_MONITOR_H_ */
