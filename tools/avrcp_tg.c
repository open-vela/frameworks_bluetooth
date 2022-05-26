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
#define LOG_TAG "tg"

#include <stdlib.h>
#include <string.h>
#include <debug.h>

#include "bt_tools.h"
#include "btm_manager.h"
#include "bts_service.h"
#include "btm_avrcp.h"
#include "bts_avrcp_target.h"
#include "utils/log.h"

static int playback_cmd(void* handle, int argc, char* argv[]);
static int volume_cmd(void* handle, int argc, char* argv[]);

static const avrcp_tg_interface_t* avrcp_tg_interface = NULL;
static bt_command_t g_avrcp_tg_tables[] = {
    { "playback", playback_cmd, "\"TG notify playback status      param: <address> <status>\"" },
    { "volume", volume_cmd, "\"TG notify volume changed param: <address> <volume>\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_avrcp_tg_tables); i++) {
        printf("\t%-8s\t%s\n", g_avrcp_tg_tables[i].cmd, g_avrcp_tg_tables[i].help);
    }
}

static int playback_cmd(void* handle, int argc, char* argv[])
{
    uint32_t status;
    bt_address addr;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    status = atoi(argv[1]);

    avrcp_tg_interface->play_status_notify(addr, status);
    return 0;
}

static int volume_cmd(void* handle, int argc, char* argv[])
{
    uint32_t volume;
    bt_address addr;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    volume = atoi(argv[1]);

    avrcp_tg_interface->volume_changed_notify(addr, volume);
    return 0;
}

int avrcp_tg_command(void* handle, int argc, char* argv[])
{
    int ret = -1;

    if (avrcp_tg_interface == NULL) {
        avrcp_tg_interface = get_avrcp_tg_interface();
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_avrcp_tg_tables); i++) {
            if (strcmp(g_avrcp_tg_tables[i].cmd, argv[1]) == 0) {
                if (g_avrcp_tg_tables[i].func) {
                    ret = g_avrcp_tg_tables[i].func(handle, argc - 2, &argv[2]);
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