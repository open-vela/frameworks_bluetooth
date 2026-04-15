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
#include <string.h>
#include <time.h>

#include <nuttx/clock.h>

#ifndef CONFIG_BLUETOOTH_TOOLS
#include "bt_trace_tool.h"
#else
#include "bt_tools.h"
#endif

#ifdef CONFIG_BT_TRACE_SPP
int trace_spp_command_exec(void* handle, int argc, char* argv[]);
#endif

static int trace_clock_test_cmd(void* handle, int argc, char* argv[])
{
    uint32_t delay_ms = 5; /* default 5 ms */
    struct timespec req, ts_elapsed;
    clock_t t1, t2, elapsed;
    uint64_t actual_us;
    unsigned long freq;
    float ratio;

    if (argc > 0) {
        delay_ms = atoi(argv[0]);
        if (delay_ms == 0 || delay_ms > 10000) {
            PRINT("Invalid delay (must be 1-10000 ms)");
            return CMD_INVALID_PARAM;
        }
    }

    freq = perf_getfreq();
    PRINT("Testing trace clock accuracy with %" PRIu32 " ms delay...", delay_ms);
    PRINT("Clock source: perf_gettime() (freq=%lu Hz)", freq);
#if CONFIG_DRIVERS_NOTE_CLOCKID >= 0
    PRINT("CLOCKID calibration: clock_gettime(%d)", CONFIG_DRIVERS_NOTE_CLOCKID);
#else
    PRINT("CLOCKID calibration: disabled (raw perf counter)");
#endif

    req.tv_sec = delay_ms / 1000;
    req.tv_nsec = (delay_ms % 1000) * 1000000L;

    t1 = perf_gettime();
    nanosleep(&req, NULL);
    t2 = perf_gettime();

    elapsed = t2 - t1;
    perf_convert(elapsed, &ts_elapsed);
    actual_us = (uint64_t)ts_elapsed.tv_sec * 1000000UL
        + ts_elapsed.tv_nsec / 1000;
    ratio = (float)actual_us / (delay_ms * 1000.0f);

    PRINT("Expected: %" PRIu32 " us (%.2f ms)", delay_ms * 1000, delay_ms / 1.0);
    PRINT("Actual:   %" PRIu64 " us (%.2f ms)", actual_us, actual_us / 1000.0);
    PRINT("Ratio:    %.2f", ratio);

    if (ratio >= 0.9 && ratio <= 1.1) {
        PRINT("Result:   PASS (trace clock is accurate)");
    } else if (ratio >= 0.5 && ratio <= 2.0) {
        PRINT("Result:   WARNING (trace clock deviation %.0f%%)",
            (ratio - 1.0) * 100);
    } else {
        PRINT("Result:   FAIL (trace clock is inaccurate, ratio %.2f)", ratio);
        if (ratio > 2.0) {
            PRINT("Hint:     Clock is too slow (possible QEMU time dilation)");
        } else {
            PRINT("Hint:     Clock is too fast");
        }
    }

    return CMD_OK;
}

static bt_command_t g_trace_tables[] = {
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
    printf("\nNote: use system 'trace start/stop' to control capture\n");
    printf("      use system 'trace dump [path]' to export trace data\n");
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
