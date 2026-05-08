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
#include "bt_sched_trace.h"

#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "utils/log.h"

#ifndef CONFIG_BLUETOOTH_TRACE_BUFFER_SIZE
#define CONFIG_BLUETOOTH_TRACE_BUFFER_SIZE 64
#endif
#define DUMP_THRESHOLD 256

typedef struct {
    char tag[MAX_TAG_LEN];
    uint64_t timestamp;
    uint32_t latency_us;
} bt_latency_record_t;

typedef struct {
    bt_latency_record_t buffer[CONFIG_BLUETOOTH_TRACE_BUFFER_SIZE];
    atomic_uint head;
    atomic_uint tail;
} bt_trace_manager_t;

static bt_trace_manager_t g_trace_manager = { 0 };

static inline uint64_t get_monotonic_ns()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000UL + ts.tv_nsec;
}

void bt_note_start(void)
{
    bt_trace_manager_t* trace_manager = &g_trace_manager;

    atomic_store(&trace_manager->head, 0);
    atomic_store(&trace_manager->tail, 0);
}

void bt_note_stop(void)
{
    bt_trace_manager_t* trace_manager = &g_trace_manager;
    char log_buf[DUMP_THRESHOLD];
    int written = 0;

    while (trace_manager->tail != trace_manager->head) {
        bt_latency_record_t* p = &trace_manager->buffer[trace_manager->tail % CONFIG_BLUETOOTH_TRACE_BUFFER_SIZE];

        written += snprintf(log_buf + written, sizeof(log_buf) - written,
            "[TAG=%s][TS=%" PRIu64 "][LAT=%" PRIu32 "us]\n",
            p->tag, p->timestamp, p->latency_us);
        trace_manager->tail = (trace_manager->tail + 1) % CONFIG_BLUETOOTH_TRACE_BUFFER_SIZE;
        written = written % DUMP_THRESHOLD;

        if (written > DUMP_THRESHOLD) {
            BT_LOGD("%s", log_buf);
            written = 0;
        }
    }

    if (written > 0) {
        BT_LOGD("%s", log_buf);
    }
}

void bt_note_begin(const char* tag, bt_timepoint_t* point)
{
    if (strlen(tag) > MAX_TAG_LEN) {
        BT_LOGD("tag is too long, max length is %d\n", MAX_TAG_LEN);
        return;
    }

    strlcpy(point->tag, tag, MAX_TAG_LEN);
    point->start_ns = get_monotonic_ns();
}

void bt_note_end(const char* tag, bt_timepoint_t* point)
{
    bt_trace_manager_t* trace_manager = &g_trace_manager;
    uint64_t end;
    uint32_t idx;

    if (strlen(tag) > MAX_TAG_LEN) {
        BT_LOGD("tag is too long, max length is %d\n", MAX_TAG_LEN);
        return;
    }

    end = get_monotonic_ns();
    idx = atomic_fetch_add(&trace_manager->head, 1) % CONFIG_BLUETOOTH_TRACE_BUFFER_SIZE;
    strlcpy(trace_manager->buffer[idx].tag, point->tag, MAX_TAG_LEN);

    trace_manager->buffer[idx].timestamp = end;
    trace_manager->buffer[idx].latency_us = (end - point->start_ns) / 1000;
}