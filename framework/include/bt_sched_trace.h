/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#include <stdint.h>

#ifndef _BT_SCHED_TRACE_H_
#define _BT_SCHED_TRACE_H_

#define MAX_TAG_LEN 16

typedef struct {
    uint64_t start_ns;
    char tag[MAX_TAG_LEN];
} bt_timepoint_t;

#ifdef CONFIG_BLUETOOTH_DEBUG_TRACE
void bt_note_start(void);
void bt_note_stop(void);
void bt_note_begin(const char* tag, bt_timepoint_t* point);
void bt_note_end(const char* tag, bt_timepoint_t* point);

#define bt_trace_start() bt_note_start()
#define bt_trace_stop() bt_note_stop()
#define bt_trace_begin(tag, point) bt_note_begin(tag, point)
#define bt_trace_end(tag, point) bt_note_end(tag, point)
#else
#define bt_trace_start()
#define bt_trace_stop()
#define bt_trace_begin(tag, point)
#define bt_trace_end(tag, point)
#endif

#endif // _BT_SCHED_TRACE_H_