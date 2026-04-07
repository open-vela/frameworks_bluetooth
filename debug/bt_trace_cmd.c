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
#include <inttypes.h>
#include <time.h>
#include <string.h>

#ifndef CONFIG_BLUETOOTH_TOOLS
#include "trace/bt_trace_tool.h"
#else
#include "../tools/bt_tools.h"
#endif

#include "bt_event_trace.h"
#include "bt_event_trace_internal.h"

#ifdef CONFIG_BT_TRACE_SPP
int trace_spp_command_exec(void* handle, int argc, char* argv[]);
#endif

static int trace_start_cmd(void* handle, int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    bt_trace_start();
    PRINT("trace started");
    return CMD_OK;
}

static int trace_stop_cmd(void* handle, int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    bt_trace_stop();
    PRINT("trace stopped");
    return CMD_OK;
}

static int trace_status_cmd(void* handle, int argc, char* argv[])
{
    bool enabled = bt_trace_enabled();

    (void)argc;
    (void)argv;

    PRINT("=== Trace Status ===");
    PRINT("State:   %s", enabled ? "CAPTURING" : "STOPPED");
    PRINT("Backend: system trace (sched_note)");
    PRINT("Use system 'trace status' for buffer details");

    return CMD_OK;
}

static int trace_clock_test_cmd(void* handle, int argc, char* argv[])
{
    uint32_t delay_ms = 5;  /* default 5 ms */

    if (argc > 0) {
        delay_ms = atoi(argv[0]);
        if (delay_ms == 0 || delay_ms > 10000) {
            PRINT("Invalid delay (must be 1-10000 ms)");
            return CMD_INVALID_PARAM;
        }
    }

#if CONFIG_DRIVERS_NOTE_CLOCKID < 0
    (void)delay_ms;
    PRINT("Error: CONFIG_DRIVERS_NOTE_CLOCKID not configured (-1)");
    PRINT("Cannot test clock accuracy without a valid clock source");
    return CMD_ERROR;
#else
    {
        struct timespec req, t1, t2;
        uint64_t actual_us;
        float ratio;

        PRINT("Testing clock accuracy with %" PRIu32 " ms delay...", delay_ms);
        PRINT("Clock source: clock_gettime(%d) (CONFIG_DRIVERS_NOTE_CLOCKID)",
              CONFIG_DRIVERS_NOTE_CLOCKID);

        req.tv_sec  = delay_ms / 1000;
        req.tv_nsec = (delay_ms % 1000) * 1000000L;

        clock_gettime(CONFIG_DRIVERS_NOTE_CLOCKID, &t1);
        nanosleep(&req, NULL);
        clock_gettime(CONFIG_DRIVERS_NOTE_CLOCKID, &t2);

        actual_us = (uint64_t)(t2.tv_sec - t1.tv_sec) * 1000000UL
                  + (t2.tv_nsec - t1.tv_nsec) / 1000;
        ratio = (float)actual_us / (delay_ms * 1000.0f);

        PRINT("Expected: %" PRIu32 " us (%.2f ms)", delay_ms * 1000, delay_ms / 1.0);
        PRINT("Actual:   %" PRIu64 " us (%.2f ms)", actual_us, actual_us / 1000.0);
        PRINT("Ratio:    %.2f", ratio);

        if (ratio >= 0.9 && ratio <= 1.1) {
            PRINT("Result:   PASS (clock is accurate)");
        } else if (ratio >= 0.5 && ratio <= 2.0) {
            PRINT("Result:   WARNING (clock deviation %.0f%%)", (ratio - 1.0) * 100);
        } else {
            PRINT("Result:   FAIL (clock is inaccurate, ratio %.2f)", ratio);
            if (ratio > 2.0) {
                PRINT("Hint:     Clock is too slow (possible QEMU time dilation)");
            } else {
                PRINT("Hint:     Clock is too fast");
            }
        }

        return CMD_OK;
    }
#endif
}

static bt_command_t g_trace_tables[] = {
    { "start", trace_start_cmd, 0, "start trace capture" },
    { "stop", trace_stop_cmd, 0, "stop trace capture" },
    { "status", trace_status_cmd, 0, "show trace status" },
    { "clock-test", trace_clock_test_cmd, 0,
      "test clock accuracy, params: [delay_ms]" },
#ifdef CONFIG_BT_TRACE_SPP
    { "spp", trace_spp_command_exec, 0,
      "spp trace filter, input 'trace spp' show usage" },
#endif
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\ttrace <command> [parameters]\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_trace_tables); i++) {
        printf("\t%-12s\t%s\n", g_trace_tables[i].cmd,
               g_trace_tables[i].help);
    }
    printf("\nNote: use system 'trace dump [path]' to export trace data\n");
}

int trace_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_trace_tables,
            ARRAY_SIZE(g_trace_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
