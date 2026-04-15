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
 * Bluetooth Event Trace API.
 *
 * Provides bt_trace_write_var() for writing variable-length event records
 * to the NuttX system trace (sched_note_event / noteram) backend.
 *
 * Capture control (start/stop) is delegated to system trace commands.
 * All decoding is done offline.
 *
 * Callers (probe layer) are already guarded by their own Kconfig
 * (CONFIG_BT_PROBE_xxx), so this header does not add another guard.
 */
#ifndef __BT_EVENT_TRACE_H__
#define __BT_EVENT_TRACE_H__

#include <stdint.h>

/**
 * bt_trace_write_var - Write a variable-length event record to system trace.
 *
 * Constructs a binary blob [event_id_lo, event_id_hi, payload...] and
 * passes it to sched_note_event(). The noteram driver adds timestamps
 * and note headers. Safe to call from IRQ context.
 *
 * @event_id: 16-bit event identifier (see event_id encoding in trace spec).
 * @payload:  Event-specific payload bytes.
 * @payload_len: Length of payload in bytes.
 */
void bt_trace_write_var(uint16_t event_id,
    const uint8_t* payload, uint8_t payload_len);

#endif /* __BT_EVENT_TRACE_H__ */
