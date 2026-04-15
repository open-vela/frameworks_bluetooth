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

/**
 * Bluetooth Event Trace — core implementation.
 *
 * Provides the bt_trace_write_var() function that writes event records
 * to the NuttX system trace backend (sched_note_event via noteram driver).
 *
 * All events are written as binary blobs:
 *   [event_id_lo(1)] [event_id_hi(1)] [payload(N)]
 *
 * The noteram driver adds timestamps and note headers automatically.
 * IRQ-safe: noteram uses spinlock internally.
 *
 * Capture control (start/stop) is delegated to the system trace commands.
 */

#include "bt_event_trace.h"

#include <nuttx/sched_note.h>
#include <string.h>

void bt_trace_write_var(uint16_t event_id,
    const uint8_t* payload, uint8_t payload_len)
{
    uint8_t buf[2 + 32]; /* event_id(2) + max payload */
    uint8_t total;

    if (payload_len > 32)
        payload_len = 32;

    total = 2 + payload_len;

    buf[0] = (uint8_t)(event_id & 0xFF);
    buf[1] = (uint8_t)(event_id >> 8);

    if (payload_len > 0)
        memcpy(&buf[2], payload, payload_len);

    sched_note_event(CONFIG_BT_TRACE_SYSTRACE_TAG,
        LOG_INFO, NOTE_DUMP_BINARY, buf, total);
}
