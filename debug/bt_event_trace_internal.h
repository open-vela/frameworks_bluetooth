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

/* Internal header for trace spec implementations ONLY.
 * Do NOT include this in probe headers or protocol layer code.
 */
#ifndef __BT_EVENT_TRACE_INTERNAL_H__
#define __BT_EVENT_TRACE_INTERNAL_H__

#include "bt_event_trace.h"

#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <syslog.h>
#include <nuttx/sched_note.h>

#ifndef CONFIG_BT_TRACE_SYSTRACE_TAG
#define CONFIG_BT_TRACE_SYSTRACE_TAG 25
#endif
#define BT_NOTE_TAG ((enum note_tag_e)CONFIG_BT_TRACE_SYSTRACE_TAG)

/* ---- Common: enabled flag ---- */

extern atomic_bool g_bt_trace_enabled;

static inline bool bt_trace_enabled(void)
{
    return atomic_load_explicit(&g_bt_trace_enabled, memory_order_relaxed);
}

/*
 * Write a BT trace record into the system trace buffer via sched_note_event().
 *
 * The system trace provides its own timestamp and buffer management.
 * We use NOTE_DUMP_BINARY as the event type (opaque binary data) and pack
 * our 16-bit event_id as the first 2 bytes of the buffer, followed by the
 * original payload.  The offline converter extracts event_id + payload from
 * the binary blob.
 *
 * Buffer layout: [event_id_lo(1)] [event_id_hi(1)] [payload(N)]
 */
static inline void bt_trace_write_var(uint16_t event_id,
                                      const void *payload, uint8_t payload_len)
{
    uint8_t buf[2 + 248]; /* event_id(2) + max payload(248) = 250 bytes */

    buf[0] = (uint8_t)(event_id & 0xFF);
    buf[1] = (uint8_t)(event_id >> 8);
    if (payload_len > 0)
        memcpy(&buf[2], payload, payload_len);

    sched_note_event(BT_NOTE_TAG, LOG_INFO, NOTE_DUMP_BINARY,
                     buf, 2 + payload_len);
}
#endif /* __BT_EVENT_TRACE_INTERNAL_H__ */
