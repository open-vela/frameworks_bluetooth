/****************************************************************************
 *
 *   Copyright (C) 2023 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_KVDB
#include "kvdb.h"
#endif

#include "bt_debug.h"
#include "bt_tools.h"
#include "bt_trace.h"
#include "utils/btsnoop_log.h"

static int enable_cmd(void* handle, int argc, char* argv[]);
static int disable_cmd(void* handle, int argc, char* argv[]);
static int mask_cmd(void* handle, int argc, char* argv[]);
static int filter_cmd(void* handle, int argc, char* argv[]);
static int unfilter_cmd(void* handle, int argc, char* argv[]);
static int unmask_cmd(void* handle, int argc, char* argv[]);
static int level_cmd(void* handle, int argc, char* argv[]);

static bt_command_t g_log_tables[] = {
    { "enable", enable_cmd, 0, "\"Enable param: (\"snoop\" or \"stack\")\"" },
    { "disable", disable_cmd, 0, "\"Disable param: (\"snoop\" or \"stack\")\"" },
    { "mask", mask_cmd, 0, "\"Enable Stack Profile & Protocol Log <bit>\"\n"
                           "\t\t\tExample enable HCI and L2CAP: \"bttool> log mask 1 4\" \n"
                           "\t\t\tProfile && Protocol Enum:\n"
                           "\t\t\t  HCI:   1\n"
                           "\t\t\t  HCI RAW PDU:2\n"
                           "\t\t\t  L2CAP: 4\n"
                           "\t\t\t  SDP:   5\n"
                           "\t\t\t  ATT:   6\n"
                           "\t\t\t  SMP:   7\n"
                           "\t\t\t  RFCOMM:8\n"
                           "\t\t\t  OBEX:  9\n"
                           "\t\t\t  AVCTP: 10\n"
                           "\t\t\t  AVDTP: 11\n"
                           "\t\t\t  AVRCP: 12\n"
                           "\t\t\t  HFP:   14\n" },
    { "unmask", unmask_cmd, 0, "\"Filter hci data  <bit>\"" },
    { "filter", filter_cmd, 0, "\"Filter hci data written to btsnoop <bit>\""
                               "\t\t\tData type Enum:\n"
                               "\t\t\tAudio data:               0\n"
                               "\t\t\tAVCTP browsing data:      1\n"
                               "\t\t\tATT data:                 2\n"
                               "\t\t\tSPP data:                 3\n" },
    { "unfilter", unfilter_cmd, 0, "\"Disable Stack Profile & Protocol Log <bit>\"" },
    { "level", level_cmd, 0, "\"Set framework log level, (OFF:0,ERR:3,WARN:4,INFO:6,DBG:7)\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_log_tables); i++) {
        printf("\t%-8s\t%s\n", g_log_tables[i].cmd, g_log_tables[i].help);
    }
}

static void property_change_commit(int bit)
{
#ifdef CONFIG_KVDB
    property_set_int32("persist.bluetooth.log.changed", (1 << bit) & 0xFFFFFFFF);
    property_commit();
#endif
}

static int log_control(void* handle, char* id, int enable)
{
#ifdef CONFIG_KVDB
    if (strncmp(id, "stack", strlen("stack")) == 0) {
        property_set_int32("persist.bluetooth.log.stack_enable", enable);
        property_change_commit(1);
    } else if (strncmp(id, "snoop", strlen("snoop")) == 0) {
        if (enable)
            bluetooth_enable_btsnoop_log(handle);
        else
            bluetooth_disable_btsnoop_log(handle);
    } else
        return CMD_INVALID_PARAM;

    return CMD_OK;
#else
    return CMD_ERROR;
#endif
}

static int enable_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    return log_control(handle, argv[0], 1);
}

static int disable_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    return log_control(handle, argv[0], 0);
}

static int mask_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;
#ifdef CONFIG_KVDB
    int mask = property_get_int32("persist.bluetooth.log.stack_mask", 0x0);
    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            int bit = atoi(argv[i]);
            if (bit < 0 || bit > 31)
                return CMD_INVALID_PARAM;

            mask |= 1 << bit;
        }
    }

    property_set_int32("persist.bluetooth.log.stack_mask", mask);
    property_change_commit(2);

    return CMD_OK;
#else
    return CMD_ERROR;
#endif
}

static int filter_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            int bit = atoi(argv[i]);
            if (bit < 0 || bit >= BTSNOOP_FILTER_MAX)
                return CMD_INVALID_PARAM;

            bluetooth_set_btsnoop_filter(handle, bit);
        }
    }

    return CMD_OK;
}

static int unfilter_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            int bit = atoi(argv[i]);
            if (bit < 0 || bit >= BTSNOOP_FILTER_MAX)
                return CMD_INVALID_PARAM;

            bluetooth_remove_btsnoop_filter(handle, bit);
        }
    }

    return CMD_OK;
}

static int unmask_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

#ifdef CONFIG_KVDB
    int mask = property_get_int32("persist.bluetooth.log.stack_mask", 0x0);
    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            int bit = atoi(argv[i]);
            if (bit < 0 || bit > 31)
                return CMD_INVALID_PARAM;

            mask &= ~(1 << bit);
        }
    }

    property_set_int32("persist.bluetooth.log.stack_mask", mask);
    property_change_commit(2);

    return CMD_OK;
#else
    return CMD_ERROR;
#endif
}

static int level_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int level = atoi(argv[0]);
    if (level != 0 && level != LOG_ERR && level != LOG_WARNING && level != LOG_INFO && level != LOG_DEBUG)
        return CMD_INVALID_PARAM;

#ifdef CONFIG_KVDB
    property_set_int32("persist.bluetooth.log.level", level);
    property_change_commit(0);

    return CMD_OK;
#else
    return CMD_ERROR;
#endif
}

int log_command(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_log_tables, ARRAY_SIZE(g_log_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
