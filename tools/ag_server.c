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
#include "btm_hfp_ag.h"
#include "btm_manager.h"
#include "euv_pty.h"
#include "utils/log.h"

static int is_connected_cmd(void* handle, int argc, char* argv[]);
static int is_audio_connected_cmd(void* handle, int argc, char* argv[]);
static int get_connection_state_cmd(void* handle, int argc, char* argv[]);
static int connect_cmd(void* handle, int argc, char* argv[]);
static int disconnect_cmd(void* handle, int argc, char* argv[]);
static int connect_audio_cmd(void* handle, int argc, char* argv[]);
static int disconnect_audio_cmd(void* handle, int argc, char* argv[]);
static int start_voice_recognition_cmd(void* handle, int argc, char* argv[]);
static int stop_voice_recognition_cmd(void* handle, int argc, char* argv[]);
static int phone_state_change_cmd(void* handle, int argc, char* argv[]);
static int device_status_changed_cmd(void* handle, int argc, char* argv[]);
static int set_inband_ring_enable_cmd(void* handle, int argc, char* argv[]);
static int send_at_command_cmd(void* handle, int argc, char* argv[]);
static int dial_result_cmd(void* handle, int argc, char* argv[]);
static int cind_response_cmd(void* handle, int argc, char* argv[]);
static int clcc_response_cmd(void* handle, int argc, char* argv[]);
static int cops_response_cmd(void* handle, int argc, char* argv[]);

static const ag_server_interface_t* ag_interface = NULL;
static bt_command_t g_hfp_ag_tables[] = {
    { "is_connected_cmd", is_connected_cmd, "\"get connected state :<address>\"" },
    { "is_audio_connected_cmd", is_audio_connected_cmd, "\"get audio connected state :<address>\"" },
    { "get_connection_state_cmd", get_connection_state_cmd, "\"get connection state :<address>\"" },

    { "connect", connect_cmd, "\"establish hfp SLC connection :<address>\"" },
    { "disconnect", disconnect_cmd, "\"disconnect hfp SLC connection :<address>\"" },
    { "connect_audio", connect_audio_cmd, "\"establish hfp SCO connection :<address>\"" },
    { "disconnect_audio", disconnect_audio_cmd, "\"disconnect hfp sco connection :<address>\"" },
    { "start_voice_recognition", start_voice_recognition_cmd, "\"start voice recognition :<address>\"" },
    { "stop_voice_recognition", stop_voice_recognition_cmd, "\"stop voice recognition :<address>\"" },
    { "phone_state_change", phone_state_change_cmd, "\"phone state change :<address> <num_active> <num_held> <call_state> <type> <number name>\"" },
    { "device_status_changed", device_status_changed_cmd, "\"device status changed :<address> <network> <roam> <signal> <battery>\"" },
    { "set_inband_ring_enable_cmd", set_inband_ring_enable_cmd, "\"set inband ring enable :<address>\"" },
    { "send_at_command_cmd", send_at_command_cmd, "\"send at command cmd :<address> <AT> \"" },
    { "dial_result", dial_result_cmd, "\"dial result :<address> <dial>\"" },
    { "cind_response", cind_response_cmd, "\"cind response :<address> <service> <signal> <roam> <battery> <call> <call_setup> <call_held>\"" },
    { "clcc_response", clcc_response_cmd, "\"clcc response :<address> <index> <dir> <status> <mode> <mpty> <number>\"" },
    { "cops_response", cops_response_cmd, "\"cops response :<address> <operator_name> <length>\"" }
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
    for (int i = 0; i < ARRAY_SIZE(g_hfp_ag_tables); i++) {
        printf("\t%-8s\t%s\n", g_hfp_ag_tables[i].cmd, g_hfp_ag_tables[i].help);
    }
}

static int is_connected_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    bool ret;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ret = ag_interface->is_connected(NULL, addr);
    BT_LOGD("%s connected stat %d", __func__, ret);

    return 0;
}

static int is_audio_connected_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    bool ret;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ret = ag_interface->is_audio_connected(NULL, addr);
    BT_LOGD("%s audio connected stat %d", __func__, ret);

    return 0;
}

static int get_connection_state_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    ag_server_state_t ret;
    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ret = ag_interface->get_connection_state(NULL, addr);
    BT_LOGD("%s connection stat %d", __func__, ret);

    return 0;
}

static int connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    BT_LOGD("%s connect", __func__);

    ag_interface->connect(NULL, addr);

    return 0;
}

static int disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ag_interface->disconnect(NULL, addr);

    return 0;
}

static int connect_audio_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ag_interface->connect_audio(NULL, addr);

    return 0;
}

static int disconnect_audio_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ag_interface->disconnect_audio(NULL, addr);

    return 0;
}

static int start_voice_recognition_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ag_interface->start_voice_recognition(NULL, addr);

    return 0;
}

static int stop_voice_recognition_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    ag_interface->stop_voice_recognition(NULL, addr);

    return 0;
}

static int phone_state_change_cmd(void* handle, int argc, char* argv[])
{

    ag_server_call_state_t call_state;
    ag_server_call_addrtype_t type;
    bt_address addr;
    char* number;
    char* name;
    uint8_t num_active;
    uint8_t num_held;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    num_active = atoi(argv[1]);
    num_held = atoi(argv[2]);
    call_state = atoi(argv[3]);
    type = atoi(argv[4]);
    number = argv[5];
    name = argv[6];

    ag_interface->phone_state_change(addr,
        num_active,
        num_held,
        call_state,
        type,
        number,
        name);
    return 0;
}

static int device_status_changed_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    hfp_network_state_t network;
    hfp_roaming_state_t roam;
    uint8_t signal;
    uint8_t battery;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    network = atoi(argv[1]);
    roam = atoi(argv[2]);
    signal = atoi(argv[3]);
    battery = atoi(argv[4]);

    ag_interface->device_status_changed(addr,
        network,
        roam,
        signal,
        battery);
    return 0;
}

static int set_inband_ring_enable_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);

    ag_interface->set_inband_ring_enable(addr);
    return 0;
}

static int send_at_command_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    char* at_command;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    at_command = argv[1];

    ag_interface->send_at_command(addr, at_command);
    return 0;
}

static int dial_result_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    uint8_t result;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    result = atoi(argv[1]);

    ag_interface->dial_result(addr, result);

    return 0;
}

static int cind_response_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    hfp_network_state_t service = 0;
    uint8_t signal;
    hfp_roaming_state_t roam = 0;
    uint8_t battery;
    hfp_call_t call = 0;
    hfp_callsetup_t call_setup = 0;
    hfp_callheld_t call_held = 0;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    service = atoi(argv[1]);
    signal = atoi(argv[2]);
    roam = atoi(argv[3]);
    battery = atoi(argv[4]);
    call = atoi(argv[5]);
    call_setup = atoi(argv[6]);
    call_held = atoi(argv[7]);

    ag_interface->cind_response(addr,
        service,
        signal,
        roam,
        battery,
        call,
        call_setup,
        call_held);

    return 0;
}

static int clcc_response_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    uint32_t index;
    uint8_t dir;
    ag_server_call_state_t status = AG_SERVER_CALL_STATE_IDLE;
    uint8_t mode;
    uint8_t mpty;
    const char* number = NULL;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    index = atoi(argv[1]);
    dir = atoi(argv[2]);
    status = atoi(argv[3]);
    mode = atoi(argv[4]);
    mpty = atoi(argv[5]);
    number = argv[6];

    ag_interface->clcc_response(addr,
        index,
        dir,
        status,
        mode,
        mpty,
        number);

    return 0;
}

static int cops_response_cmd(void* handle, int argc, char* argv[])
{
    bt_address addr;
    char* operator_name = NULL;
    uint16_t length;

    if (argc < 1 || ag_interface == NULL)
        return -1;

    str2ba(argv[0], addr);
    operator_name = argv[1];
    length = atoi(argv[2]);

    ag_interface->cops_response(addr, operator_name, length);

    return 0;
}

static void ag_server_connection_state_cb(
    bt_address addr, profile_connection_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
}

static void ag_server_audio_state_cb(
    bt_address addr, hfp_audio_state_t state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
}

static void ag_server_vr_cmd_cb(bt_address addr,
    bool state)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), state);
}

static void ag_server_ag_battery_update_cb(bt_address addr,
    uint8_t value)
{
    BT_LOGD("%s, addr:%s, state:%d", __func__, addr_str(addr), value);
}

static void ag_server_answer_call_cb(bt_address addr)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));
}

static void ag_server_reject_call_cb(bt_address addr)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));
}

static void ag_server_hangup_call_cb(bt_address addr)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));
}

static void ag_server_dial_number_cb(
    bt_address addr, char* number)
{
    BT_LOGD("%s, addr:%s, number:%s", __func__, addr_str(addr), number);
}

static void ag_server_call_control_cb(
    bt_address addr, uint8_t chld)
{
    BT_LOGD("%s, addr:%s, chld:%d", __func__, addr_str(addr), chld);
}

static void ag_server_cind_cb(bt_address addr)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));
}

static void ag_server_clcc_cb(bt_address addr)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));
}

static void ag_server_cops_cb(bt_address addr)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(addr));
}

static void ag_server_at_command_cb(bt_address addr, char* at_command)
{
    BT_LOGD("%s, addr:%s at command: %s", __func__, addr_str(addr), at_command);
}

ag_server_callbacks_t ag_server_cbs = {
    sizeof(ag_server_callbacks_t),
    ag_server_connection_state_cb,
    ag_server_audio_state_cb,
    ag_server_vr_cmd_cb,
    ag_server_ag_battery_update_cb,
    ag_server_answer_call_cb,
    ag_server_reject_call_cb,
    ag_server_hangup_call_cb,
    ag_server_dial_number_cb,
    ag_server_call_control_cb,
    ag_server_at_command_cb,
    ag_server_cind_cb,
    ag_server_clcc_cb,
    ag_server_cops_cb
};

int hfp_server_command(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;
    BT_LOGD("%s", __func__);

    if (ag_interface == NULL) {
        BT_LOGD("%s ag_interface", __func__);
        ag_interface = get_ag_server_interface();
        ag_interface->set_callbacks(NULL, &ag_server_cbs);
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
        for (int i = 0; i < ARRAY_SIZE(g_hfp_ag_tables); i++) {
            if (strcmp(g_hfp_ag_tables[i].cmd, argv[1]) == 0) {
                BT_LOGD("%s ag_interface %s", __func__, g_hfp_ag_tables[i].cmd);
                if (g_hfp_ag_tables[i].func) {
                    ret = g_hfp_ag_tables[i].func(handle, argc - 2, &argv[2]);
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
