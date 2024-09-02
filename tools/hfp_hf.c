/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_hfp_hf.h"
#include "bt_tools.h"

static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int set_policy_cmd(void* handle, int argc, char* argv[]);
static int get_hfp_connection_state_cmd(void* handle, int argc, char* argv[]);
static int connect_audio_cmd(void* handle, int argc, char* argv[]);
static int disconnect_audio_cmd(void* handle, int argc, char* argv[]);
static int start_voice_recognition_cmd(void* handle, int argc, char* argv[]);
static int stop_voice_recognition_cmd(void* handle, int argc, char* argv[]);
static int dial_cmd(void* handle, int argc, char* argv[]);
static int dial_memory_cmd(void* handle, int argc, char* argv[]);
static int redial_cmd(void* handle, int argc, char* argv[]);
static int accept_call_cmd(void* handle, int argc, char* argv[]);
static int reject_call_cmd(void* handle, int argc, char* argv[]);
static int hold_call_cmd(void* handle, int argc, char* argv[]);
static int terminate_call_cmd(void* handle, int argc, char* argv[]);
static int control_call_cmd(void* handle, int argc, char* argv[]);
static int query_current_calls_cmd(void* handle, int argc, char* argv[]);
static int send_at_cmd_cmd(void* handle, int argc, char* argv[]);
static int update_battery_level_cmd(void* handle, int argc, char* argv[]);
static int send_dtmf_cmd(void* handle, int argc, char* argv[]);

#define CHLD_0_DESC "0: Releases all held calls or sets User Determined User Busy (UDUB) for a waiting call"
#define CHLD_1_DESC "1: Releases all active calls (if any exist) and accepts the other (held or waiting) call"
#define CHLD_2_DESC "2: Places all active calls (if any exist) on hold and accepts the other (held or waiting) call"
#define CHLD_3_DESC "3: Adds a held call to the conversation"
#define CHLD_4_DESC "4: Connects the two calls and disconnects the subscriber from both calls (Explicit Call Transfer)." \
                    "Support for this value and its associated functionality is optional for the HF"

#define ACCEPT_CALL_USAGE "Accept voice call                    params: <address> <flag>\n"      \
                          "\t\t\t0: Accept an incoming call, invalid when is no incoming call\n" \
                          "\t\t\t" CHLD_1_DESC "\n"                                              \
                          "\t\t\t" CHLD_2_DESC "\n"

#define REJECT_CALL_USAGE "Reject  voice call                   params: <address>\n" \
                          "\t\t\treject an incoming call if any exist, otherwise then releases all held calls or a waiting call"

#define HANGUP_CALL_USAGE "Terminate a call                     params: <address>\n" \
                          "\t\t\thangup an active/dialing/alerting voice call if any exist, otherwise then releases all held calls."

#define HOLD_CALL_USAGE "Control multi call                   params: <address> <control>\n" \
                        "\t\t\t" CHLD_0_DESC "\n"                                            \
                        "\t\t\t" CHLD_1_DESC "\n"                                            \
                        "\t\t\t" CHLD_2_DESC "\n"                                            \
                        "\t\t\t" CHLD_3_DESC "\n"

#define SEND_DTMF_USAGE "Send DTMF code                       params: <address> <dtmf>\n" \
                        "\t\t\t<dtmf>: one of \"0, 1, 2, 3, 4, 5, 6, 7, 8, 9, *, #, A, B, C, D\"\n"

#define SET_POLICY_USAGE "Set HF connection policy            params: <address> <policy>\n" \
                         "\t\t\t0: CONNECTION_POLICY_ALLOWED \n"                            \
                         "\t\t\t1: CONNECTION_POLICY_FORBIDDEN \n"                          \
                         "\t\t\t2: CONNECTION_POLICY_UNKNOWN \n"

static bt_command_t g_hfp_tables[] = {
    { "connect", connect_cmd, 0, "Establish hfp SLC connection         params: <address>" },
    { "disconnect", disconnect_cmd, 0, "Disconnect hfp SLC connection        params: <address>" },
    { "policy", set_policy_cmd, 0, SET_POLICY_USAGE },
    { "connectaudio", connect_audio_cmd, 0, "Establish hfp SCO connection         params: <address>" },
    { "disconnectaudio", disconnect_audio_cmd, 0, "Disconnect hfp SCO connection        params: <address>" },
    { "startvr", start_voice_recognition_cmd, 0, "Start voice recognition              params: <address>" },
    { "stopvr", stop_voice_recognition_cmd, 0, "Stop voice recognition               params: <address>" },
    { "dial", dial_cmd, 0, "Dial phone number                    params: <address> <number>" },
    { "dialm", dial_memory_cmd, 0, "Place a call using memory dialing    params: <address> <memory>" },
    { "redial", redial_cmd, 0, "Redial the last number               params: <address>" },
    { "accept", accept_call_cmd, 0, ACCEPT_CALL_USAGE },
    { "reject", reject_call_cmd, 0, REJECT_CALL_USAGE },
    { "hold", hold_call_cmd, 0, "Hold an Three-way calling            params: <address>" },
    { "term", terminate_call_cmd, 0, HANGUP_CALL_USAGE },
    { "control", control_call_cmd, 0, HOLD_CALL_USAGE },
    { "query", query_current_calls_cmd, 0, "Query current calls                  params: <address>" },
    { "sendat", send_at_cmd_cmd, 0, "Send customize AT command to peer    params: <address> <atcmd>" },
    { "battery", update_battery_level_cmd, 0, "Update battery level within [0, 100] params: <address> <level>\"" },
    { "dtmf", send_dtmf_cmd, 0, SEND_DTMF_USAGE },
    { "state", get_hfp_connection_state_cmd, 0, "get hfp profile state" },
};

static void* hf_callbacks = NULL;
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
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_connect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_disconnect(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int set_policy_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    connection_policy_t policy;
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    switch (atoi(argv[1])) {
    case CONNECTION_POLICY_ALLOWED:
        policy = CONNECTION_POLICY_ALLOWED;
        break;
    case CONNECTION_POLICY_FORBIDDEN:
        policy = CONNECTION_POLICY_FORBIDDEN;
        break;
    default:
        policy = CONNECTION_POLICY_UNKNOWN;
        break;
    };

    if (bt_hfp_hf_set_connection_policy(handle, &addr, policy) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int get_hfp_connection_state_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int state = bt_hfp_hf_get_connection_state(handle, &addr);
    return state;
}

static int connect_audio_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_connect_audio(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int disconnect_audio_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_disconnect_audio(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int start_voice_recognition_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_start_voice_recognition(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int stop_voice_recognition_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_stop_voice_recognition(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int dial_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_dial(handle, &addr, argv[1]) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int dial_memory_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_dial_memory(handle, &addr, atoi(argv[1])) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int redial_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_redial(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int accept_call_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    int flag = atoi(argv[1]);
    if (flag < 0 || flag > 2)
        return CMD_INVALID_PARAM;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_accept_call(handle, &addr, flag) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int reject_call_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_reject_call(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int hold_call_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_hold_call(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int terminate_call_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_terminate_call(handle, &addr) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int control_call_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int chld = atoi(argv[1]);
    if (chld < 0 || chld > 3)
        return CMD_INVALID_PARAM;

    if (bt_hfp_hf_control_call(handle, &addr, chld, 0) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int query_current_calls_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    hfp_current_call_t* calls = NULL;
    int num = 0;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_hfp_hf_query_current_calls(handle, &addr, &calls, &num, bttool_allocator) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Calls:[%d]", num);
    if (num) {
        hfp_current_call_t* call = calls;
        for (int i = 0; i < num; i++) {
            PRINT("\tidx[%d], dir:%d, state:%d, number:%s, name:%s",
                (int)call->index, call->dir, call->state, call->number, call->name);
            call++;
        }
    }
    free(calls);

    return CMD_OK;
}

static int send_at_cmd_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    int len = 0;
    char at_buf[64];

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    len = strlen(argv[1]);
    if (len + 3 > 64)
        return CMD_INVALID_PARAM;

    memcpy(at_buf, argv[1], len);
    at_buf[len] = '\r';
    at_buf[len + 1] = '\n';
    at_buf[len + 2] = '\0';
    if (bt_hfp_hf_send_at_cmd(handle, &addr, at_buf) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int update_battery_level_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int level = atoi(argv[1]);
    if (level < 0 || level > 100)
        return CMD_INVALID_PARAM;

    if (bt_hfp_hf_update_battery_level(handle, &addr, level) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int send_dtmf_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (strlen(argv[1]) != 1)
        return CMD_INVALID_PARAM;

    char dtmf = argv[1][0];
    if (((dtmf < '0') || (dtmf > '9')) && ((dtmf < 'A') || (dtmf > 'D')) && (dtmf != '*') && (dtmf != '#'))
        return CMD_INVALID_PARAM;

    if (bt_hfp_hf_send_dtmf(handle, &addr, dtmf) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static void hf_connection_state_callback(void* context, bt_address_t* addr, profile_connection_state_t state)
{
    PRINT_ADDR("hf_connection_state_callback, addr:%s, state:%d", addr, state);
}

static void hf_audio_state_callback(void* context, bt_address_t* addr, hfp_audio_state_t state)
{
    PRINT_ADDR("hf_audio_state_callback, addr:%s, state:%d", addr, state);
}

static void hf_vr_cmd_callback(void* context, bt_address_t* addr, bool started)
{
    PRINT_ADDR("hf_vr_cmd_callback, addr:%s, started:%d", addr, started);
}

static void hf_call_state_change_callback(void* context, bt_address_t* addr, hfp_current_call_t* call)
{
    PRINT_ADDR("hf_call_state_change_callback, addr:%s, idx[%d], dir:%d, state:%d, number:%s, name:%s",
        addr, (int)call->index, call->dir, call->state, call->number, call->name);
}

static void hf_cmd_complete_callback(void* context, bt_address_t* addr, const char* resp)
{
    PRINT_ADDR("hf_cmd_complete_callback, addr:%s, AT cmd resp:%s", addr, resp);
}

static void hf_ring_indication_callback(void* context, bt_address_t* addr, bool inband_ring_tone)
{
    PRINT_ADDR("hf_ring_indication_callback, addr:%s, inband-ring:%d", addr, inband_ring_tone);
}

static void hf_vol_changed_callback(void* context, bt_address_t* addr, hfp_volume_type_t type, uint8_t volume)
{
    PRINT_ADDR("hf_vol_changed_callback, addr:%s, type:%s, vol:%d", addr, type ? "Microphone" : "Speaker", volume);
}

static const hfp_hf_callbacks_t hfp_hf_cbs = {
    sizeof(hfp_hf_cbs),
    hf_connection_state_callback,
    hf_audio_state_callback,
    hf_vr_cmd_callback,
    hf_call_state_change_callback,
    hf_cmd_complete_callback,
    hf_ring_indication_callback,
    hf_vol_changed_callback,
    NULL,
    NULL,
    NULL,
};

int hfp_hf_commond_init(void* handle)
{
    hf_callbacks = bt_hfp_hf_register_callbacks(handle, &hfp_hf_cbs);

    return 0;
}

int hfp_hf_commond_uninit(void* handle)
{
    bt_hfp_hf_unregister_callbacks(handle, hf_callbacks);

    return 0;
}

int hfp_hf_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_hfp_tables, ARRAY_SIZE(g_hfp_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}
