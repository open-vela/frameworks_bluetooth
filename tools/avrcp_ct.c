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
#define LOG_TAG "ct_tool"

#include <stdlib.h>
#include <string.h>
#include <debug.h>

#include "bt_tools.h"
#include "btm_manager.h"
#include "bts_service.h"
#include "btm_avrcp_ctrl.h"
#include "bts_avrcp_target.h"
#include "utils/log.h"

static int pass_through_cmd(void* handle, int argc, char* argv[]);
static int get_playback_status_cmd(void* handle, int argc, char* argv[]);
static int volume_change_cmd(void* handle, int argc, char* argv[]);

static const avrc_ctrl_interface_t* avrcp_ctrl_interface = NULL;
static uint8_t g_abs_volume = 0x3F;
static bt_command_t g_avrcp_ct_tables[] = {
    { "pass", pass_through_cmd, "\"CT send passthrough command      param: <address> <key>(play/pause/stop/next/prev)\"" },
    { "playstatus", get_playback_status_cmd, "\"CT get playback status param: <address> \"" },
    { "volume", volume_change_cmd, "\"CT notify volume changed param: <address> <volume> (range 1~127) \"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_avrcp_ct_tables); i++) {
        printf("\t%-8s\t%s\n", g_avrcp_ct_tables[i].cmd, g_avrcp_ct_tables[i].help);
    }
}

static void avrcp_connection_state_cb(bt_address addr, avrcp_connection_state_t state)
{
    BT_LOGD("%s addr: %s, state:%d", __func__, addr_str(addr), state);
}

static void avrcp_passthrough_rsp_cb(bt_address addr, avrcp_passthr_cmd_t key_code,
                                    avrcp_key_state_t key_state, uint8_t response)
{
    BT_LOGD("%s addr: %s, key_code:%d, key_state:%d, response:%d", __func__, addr_str(addr), key_code, key_state, response);
}

static void avrcp_play_position_changed_cb(bt_address addr, uint32_t song_len, uint32_t song_pos)
{
    BT_LOGD("%s addr: %s, song_len:%"PRIu32", song_pos:%"PRIu32, __func__, addr_str(addr), song_len, song_pos);
}

static void avrcp_play_status_changed_cb(bt_address addr, avrcp_play_status_t play_status)
{
    BT_LOGD("%s addr: %s, play_status:%d", __func__, addr_str(addr), play_status);
}

static void avrcp_register_notification_absvol_cb(bt_address addr)
{
    BT_LOGD("%s addr: %s", __func__, addr_str(addr));
    avrcp_ctrl_interface->volume_changed_notify(addr, g_abs_volume);
}

static void avrcp_set_volume_cb(bt_address addr, uint8_t volume)
{
    BT_LOGD("%s addr: %s, volume: %d", __func__, addr_str(addr), volume);
    g_abs_volume = volume;
    avrcp_ctrl_interface->volume_changed_notify(addr, volume);
}

static const avrc_ctrl_callbacks_t g_tools_avrcp_ctrl_cbs = {
    sizeof(avrc_ctrl_callbacks_t),
    avrcp_connection_state_cb,
    avrcp_passthrough_rsp_cb,
    avrcp_play_position_changed_cb,
    avrcp_play_status_changed_cb,
    avrcp_register_notification_absvol_cb,
    avrcp_set_volume_cb,
};

static int pass_through_cmd(void* handle, int argc, char* argv[])
{
    char* key;
    uint32_t keycode;
    bt_address addr;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    key = argv[1];
    if (strcmp(key, "play") == 0)
        keycode = PASSTHROUGH_CMD_ID_PLAY;
    else if (strcmp(key, "pause") == 0)
        keycode = PASSTHROUGH_CMD_ID_PAUSE;
    else if (strcmp(key, "stop") == 0)
        keycode = PASSTHROUGH_CMD_ID_STOP;
    else if (strcmp(key, "next") == 0)
        keycode = PASSTHROUGH_CMD_ID_FORWARD;
    else if (strcmp(key, "prev") == 0)
        keycode = PASSTHROUGH_CMD_ID_BACKWARD;
    else
        return -1;

    avrcp_ctrl_interface->send_pass_through_cmd(addr, keycode, AVRCP_KEY_PRESSED);
    avrcp_ctrl_interface->send_pass_through_cmd(addr, keycode, AVRCP_KEY_RELEASED);
    return 0;
}

static int get_playback_status_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 1)
        return -1;

    str2ba(argv[0], addr);

    avrcp_ctrl_interface->get_playback_state(addr);
    return 0;
}

static int volume_change_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 2)
        return -1;

    str2ba(argv[0], addr);
    int volume = atoi(argv[1]);
    if (volume < 0 || volume > 127)
        return -1;

    avrcp_ctrl_interface->volume_changed_notify(addr, volume);
    g_abs_volume = volume;

    return 0;
}

int avrcct_command_init(void)
{
    if (!avrcp_ctrl_interface) {
        avrcp_ctrl_interface = get_avrcp_ctrl_interface();
        avrcp_ctrl_interface->set_callbacks(&g_tools_avrcp_ctrl_cbs);
    }    

    return 0;
}

void avrcct_command_uninit(void)
{
    if (avrcp_ctrl_interface) {
        avrcp_ctrl_interface->reset_callbacks();
        avrcp_ctrl_interface = NULL;
    }
}

int avrcp_ct_command(void* handle, int argc, char* argv[])
{
    int ret = -1;

    if (avrcp_ctrl_interface == NULL) {
        avrcp_ctrl_interface = get_avrcp_ctrl_interface();
        avrcp_ctrl_interface->set_callbacks(&g_tools_avrcp_ctrl_cbs);
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_avrcp_ct_tables); i++) {
            if (strcmp(g_avrcp_ct_tables[i].cmd, argv[1]) == 0) {
                if (g_avrcp_ct_tables[i].func) {
                    ret = g_avrcp_ct_tables[i].func(handle, argc - 2, &argv[2]);
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