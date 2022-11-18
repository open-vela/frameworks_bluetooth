/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
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
#include "utils/log.h"
#include "bt_tools.h"
#include <debug.h>
#include <stdlib.h>
#include <string.h>

static struct option log_options[] = {
    {"level", 1, NULL, 'l'},
    { "help", 0, NULL, 'h'},
    { 0,      0, 0,    0  }
};

static int enable_cmd(void *handle, int argc, char *argv[]);
static int disable_cmd(void *handle, int argc, char *argv[]);
static int mask_cmd(void *handle, int argc, char *argv[]);
static int unmask_cmd(void *handle, int argc, char *argv[]);
static int level_cmd(void *handle, int argc, char *argv[]);

static bt_command_t g_log_tables[] = {
    {"enable",   enable_cmd,  "\"Enable Log <LogID> (SNOOP: 0, STACK: 1, FRAMEWORK: 2)\""     },
    { "disable", disable_cmd, "\"Disable Log <LogID>\""                                       },
    { "mask",    mask_cmd,    "\"Enable Stack Profile & Protocol Log <bit>\"\n"
                        "\t\t\tExample enable HCI and L2CAP: \"bttool> log mask 1 2\" \n"
                        "\t\t\tProfile && Protocol Enum:\n"
                        "\t\t\t  HCI:   1\n"
                        "\t\t\t  L2CAP: 2\n"
                        "\t\t\t  SDP:   3\n"
                        "\t\t\t  RFCOMM:4\n"
                        "\t\t\t  ATT:   5\n"
                        "\t\t\t  OBEX:  7\n"
                        "\t\t\t  AVCTP: 13\n"
                        "\t\t\t  AVDTP: 14\n"
                        "\t\t\t  AVRCP: 16\n"
                        "\t\t\t  SMP:   18\n"
                        "\t\t\t  HFP:   25\n"
                        "\t\t\t  RAW PDU:29\n"                      },
    { "unmask",  unmask_cmd,  "\"Disable Stack Profile & Protocol Log <bit>\""                },
    { "level",   level_cmd,   "\"Set framework log level, (OFF:0,ERR:3,WARN:4,INFO:6,DBG:7)\""},
};

static void usage(void)
{
    printf("Usage:\n");
    printf("Options:\n"
           "\t--level\\-l  \t\"Set framework log level, (OFF:0,ERR:3,WARN:4,INFO:6,DBG:7)\"\n"
           "\t--help       \t\"Display help\"\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_log_tables); i++) {
        printf("\t%-8s\t%s\n", g_log_tables[i].cmd, g_log_tables[i].help);
    }
}

static int enable_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    int id = atoi(argv[0]);

    return utils_log_enable((uint8_t)id);
}

static int disable_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    int id = atoi(argv[0]);

    return utils_log_disable((uint8_t)id);
}

static int mask_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL)
            utils_set_log_mask_level(LOG_ID_STACK, atoi(argv[i]), true);
    }

    return 0;
}

static int unmask_cmd(void *handle, int argc, char *argv[])
{
    if (argc < 1)
        return -1;

    for (int i = 0; i < argc; i++) {
        if (argv[i] != NULL)
            utils_set_log_mask_level(LOG_ID_STACK, atoi(argv[i]), false);
    }

    return 0;
}

static int level_cmd(void *handle, int argc, char *argv[])
{
    uint8_t level;

    if (argc < 1)
        level = utils_get_log_level();
    else {
        level = utils_set_log_level((uint8_t)atoi(argv[0]));
    }
    printf("Current Log level :%d\n", level);

    return 0;
}

int log_command(void *handle, int argc, char *argv[])
{
    int opt, ret = -1;

    while ((opt = getopt_long(argc, argv, "l:h", log_options, NULL)) != -1) {
        switch (opt) {
        case 'l':
            ret = 0;
            if (optarg != NULL) {
                int level = atoi(optarg);
                utils_set_log_level((uint8_t)level);
            } else
                printf("Log level :%d\n", utils_get_log_level());
            break;
        case 'h':
            ret = 0;
            usage();
            break;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_log_tables); i++) {
            if (strcmp(g_log_tables[i].cmd, argv[1]) == 0) {
                if (g_log_tables[i].func) {
                    ret = g_log_tables[i].func(handle, argc - 2, &argv[2]);
                }
            }
        }
    }
    if (ret < 0) {
        printf("UnKnow command %s\n", argv[1]);
        usage();
    }

    return 0;
}