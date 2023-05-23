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
#define LOG_TAG "hfp_tool"
#include <debug.h>
#include <nuttx/list.h>
#include <stdlib.h>
#include <string.h>

#include "bt_tools.h"
#include "btm_hfp_hf.h"
#include "btm_manager.h"
#include "euv_pty.h"
#include "utils/log.h"

#define HFP_CHECK_DTMF(idx) (((idx) <= '9' && (idx) >= '0') || ((idx) <= 'D' && (idx) >= 'A') || (idx) == '*' || (idx) == '#')

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int connect_audio_cmd(void* handle, int argc, char* argv[]);
static int disconnect_audio_cmd(void* handle, int argc, char* argv[]);
static int start_voice_recognition_cmd(void* handle, int argc, char* argv[]);
static int stop_voice_recognition_cmd(void* handle, int argc, char* argv[]);
static int volume_control_cmd(void* handle, int argc, char* argv[]);
static int dial_cmd(void* handle, int argc, char* argv[]);
static int dial_memory_cmd(void* handle, int argc, char* argv[]);
static int redial_cmd(void* handle, int argc, char* argv[]);
static int accept_call_cmd(void* handle, int argc, char* argv[]);
static int reject_call_cmd(void* handle, int argc, char* argv[]);
static int hold_call_cmd(void* handle, int argc, char* argv[]);
static int terminate_call_cmd(void* handle, int argc, char* argv[]);
static int control_call_cmd(void* handle, int argc, char* argv[]);
static int query_current_calls_cmd(void* handle, int argc, char* argv[]);
static int updata_battery_level_cmd(void* handle, int argc, char* argv[]);
static int send_dtmf_cmd(void* handle, int argc, char* argv[]);
static int send_at_cmd_cmd(void* handle, int argc, char* argv[]);

static const hf_client_interface_t* hf_interface = NULL;
static bt_command_t g_hfp_tables[] = {
    { "connect", connect_cmd, "\"establish hfp SLC connection   :<address>\"" },
    { "disconnect", disconnect_cmd, "\"disconnect hfp SLC connection  :<address>\"" },
    { "connectaudio", connect_audio_cmd, "\"establish hfp SCO connection   :<address>\"" },
    { "disconnectaudio", disconnect_audio_cmd, "\"disconnect hfp SCO connection  :<address>\"" },
    { "startvr", start_voice_recognition_cmd, "\"start voice recognition        :<address>\"" },
    { "stopvr", stop_voice_recognition_cmd, "\"stop voice recognition         :<address>\"" },
    { "volc", volume_control_cmd, "\"volume control, type(0:spk, 1:mic), vol(1~15)\t:<address> <type> <volume>\"" },
    { "dial", dial_cmd, "\"dial phone number              :<address> <number>\"" },
    { "dialm", dial_memory_cmd, "\"Place a call using memory dialing  :<address> <memory>\"" },
    { "redial", redial_cmd, "\"redial the last number         :<address>\"" },
    { "accept", accept_call_cmd, "\"accept an incoming voice call  :<address>\"" },
    { "reject", reject_call_cmd, "\"reject an incoming voice call  :<address>\"" },
    { "hold", hold_call_cmd, "\"hold an Three-way calling      :<address>\"" },
    { "term", terminate_call_cmd, "\"terminate a call               :<address>\"" },
    { "control_call", control_call_cmd, "\"control a call               :<address> <chld> <index>\"" },
    { "query", query_current_calls_cmd, "\"query current calls            :<address>\"" },
    { "bat", updata_battery_level_cmd, "\"update battery level range in <0~100>           :<address> <battery>\"" },
    { "dtmf", send_dtmf_cmd, "\"dtmf range in <0123456789*#ABCD>           :<address> <dtmf>\"" },
    { "at", send_at_cmd_cmd, "\"send customize AT command to peer  :<address> <at>\"" },
};

static struct option hfp_options[] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_hfp_tables); i++) {
        printf("\t%-8s\t%s\n", g_hfp_tables[i].cmd, g_hfp_tables[i].help);
    }
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->connect(NULL, addr);

    return 0;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->disconnect(NULL, addr);

    return 0;
}

static int connect_audio_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->connect_audio(NULL, addr);

    return 0;
}

static int disconnect_audio_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->disconnect_audio(NULL, addr);

    return 0;
}

static int start_voice_recognition_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->start_voice_recognition(NULL, addr);

    return 0;
}

static int stop_voice_recognition_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->stop_voice_recognition(NULL, addr);

    return 0;
}

static int volume_control_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    int type, volume;
    if (argc < 3 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    type = atoi(argv[1]);
    volume = atoi(argv[2]);
    hf_interface->volume_control(NULL, addr, type, volume);

    return 0;
}

static int dial_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    char* number;
    if (argc < 2 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    number = argv[1];
    hf_interface->dial(NULL, addr, number);

    return 0;
}

static int dial_memory_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    int mem;
    if (argc < 2 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    mem = atoi(argv[1]);
    hf_interface->dial_memory(NULL, addr, mem);

    return 0;
}

static int redial_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->redial(NULL, addr);

    return 0;
}

static int accept_call_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->accept_call(NULL, addr);

    return 0;
}

static int reject_call_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->reject_call(NULL, addr);

    return 0;
}

static int hold_call_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->hold_call(NULL, addr);

    return 0;
}

static int terminate_call_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->terminate_call(NULL, addr);

    return 0;
}

static int control_call_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    hf_client_call_control_t chld;
    uint8_t index;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    chld = (uint8_t)atoi(argv[1]);
    index = (uint8_t)atoi(argv[2]);
    hf_interface->control_call(NULL, addr, chld, index);

    return 0;
}

static int query_current_calls_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    hf_interface->query_current_calls(NULL, addr);

    return 0;
}

static int updata_battery_level_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    uint8_t battery;

    if (argc < 2 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    battery = (uint8_t)atoi(argv[1]);
    if (battery > 100)
        return -EINVAL;

    hf_interface->update_battery_level(NULL, addr, battery);

    return 0;
}

static int send_dtmf_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    uint8_t dtmf;

    if (argc < 2 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    dtmf = (argv[1])[0];
    if (!HFP_CHECK_DTMF(dtmf)) {
        BT_LOGE("dtmf range in <0123456789*#ABCD>");
        return 0;
    }

    hf_interface->send_dtmf(NULL, addr, dtmf);

    return 0;
}

static int send_at_cmd_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    int len = 0;
    char at_buf[64];
    if (argc < 1 || hf_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    len = strlen(argv[1]);
    if (len + 3 > 64)
        return -1;

    memcpy(at_buf, argv[1], len);
    at_buf[len] = '\r';
    at_buf[len + 1] = '\n';
    at_buf[len + 2] = '\0';
    hf_interface->send_at_cmd(NULL, addr, at_buf);

    return 0;
}

static void hf_client_connection_state_cb(
    bt_address addr, hf_client_connection_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
}

static void hf_client_audio_state_cb(
    bt_address addr, hf_client_audio_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
}

static void hf_client_vr_cmd_cb(bt_address addr,
    hf_client_vr_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
}

static void hf_client_call_cb(bt_address addr,
    hf_client_call_t call)
{
    BT_LOGD("%s, addr:%s, call:%d", __func__, addr_str(addr), call);
}

static void hf_client_callsetup_cb(
    bt_address addr, hf_client_callsetup_t callsetup)
{
    BT_LOGD("%s, addr:%s, callsetup:%d", __func__, addr_str(addr), callsetup);
}

static void hf_client_callheld_cb(bt_address addr,
    hf_client_callheld_t callheld)
{
    BT_LOGD("%s, addr:%s, callheld:%d", __func__, addr_str(addr), callheld);
}

static void hf_client_clip_cb(bt_address addr,
    const char* number, const char* name)
{
    BT_LOGD("%s, addr:%s, number:%s, name:%s", __func__, addr_str(addr), number, name);
}

static void hf_client_current_calls_cb(bt_address addr, int index,
    hf_client_call_direction_t dir,
    hf_client_call_state_t state,
    hf_client_call_mpty_type_t mpty,
    const char* number)
{
    if (index != 0)
        BT_LOGD("%s, addr:%s, call[%d], dir:%d, state:%d, mpty:%d, number:%s", __func__, addr_str(addr), index, dir, state, mpty, number);
}

static void hf_client_volume_change_cb(
    bt_address addr, hf_client_volume_type_t type, int volume)
{
    BT_LOGD("%s, addr:%s, volume:%d", __func__, addr_str(addr), volume);
}

static void hf_client_cmd_complete_cb(
    bt_address addr, const char* resp)
{
    BT_LOGD("%s, addr:%s, resp:%s", __func__, addr_str(addr), resp);
}

static void hf_client_ring_indication_cb(bt_address addr,
    hf_client_in_band_ring_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
}

hf_client_callbacks_t hf_client_cbs = {
    sizeof(hf_client_callbacks_t),
    hf_client_connection_state_cb,
    hf_client_audio_state_cb,
    hf_client_vr_cmd_cb,
    hf_client_call_cb,
    hf_client_callsetup_cb,
    hf_client_callheld_cb,
    hf_client_clip_cb,
    hf_client_current_calls_cb,
    hf_client_volume_change_cb,
    hf_client_cmd_complete_cb,
    hf_client_ring_indication_cb,
};

int hfp_client_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (hf_interface == NULL) {
        hf_interface = get_hf_client_interface();
        hf_interface->set_callbacks(NULL, &hf_client_cbs);
    }

    while ((opt = getopt_long(argc, argv, "h", hfp_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_hfp_tables); i++) {
            if (strcmp(g_hfp_tables[i].cmd, argv[1]) == 0) {
                if (g_hfp_tables[i].func) {
                    ret = g_hfp_tables[i].func(handle, argc - 2, &argv[2]);
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