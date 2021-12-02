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
#define LOG_TAG "bttools"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system/readline.h>

#include "btm_gap.h"
#include "btm_manager.h"
#include "stack_adapter_gap.h"

#include "bt_tools.h"
#include "utils/log.h"

static void usage(void);
static int usage_cmd(void* handle, int argc, char** argv);
static int enable_cmd(void* handle, int argc, char** argv);
static int disable_cmd(void* handle, int argc, char** argv);
static int get_state_cmd(void* handle, int argc, char** argv);
static int get_ble_state_cmd(void* handle, int argc, char** argv);

static int quit_cmd(void* handle, int argc, char** argv);
static int gap_cmd(void* handle, int argc, char** argv);

/*gap cmd*/
static int start_discovery(void* handle, int argc, char** argv);
static int stop_discovery(void* handle, int argc, char** argv);
static int set_scan_mode(void* handle, int argc, char** argv);
static int get_local_address(void* handle, int argc, char** argv);
static int set_local_io_capability(void* handle, int argc, char** argv);
static int get_local_name(void* handle, int argc, char** argv);
static int set_local_name(void* handle, int argc, char** argv);

static int get_remote_name(void* handle, int argc, char** argv);
static int reply_pair_request(void* handle, int argc, char** argv);
static int create_bond(void* handle, int argc, char** argv);
static int cancel_bond(void* handle, int argc, char** argv);
static int remove_bond(void* handle, int argc, char** argv);
static int get_bonded_devices(void* handle, int argc, char** argv);
static int get_connected_devices(void* handle, int argc, char** argv);
static int start_service_discovery(void* handle, int argc, char** argv);
static int stop_service_discovery(void* handle, int argc, char** argv);
static int get_remote_services(void* handle, int argc, char** argv);
static int set_local_device_class(void* handle, int argc, char** argv);
static int get_local_device_class(void* handle, int argc, char** argv);
static int ble_set_address(void* handle, int argc, char** argv);

static btm_gap_interface_t* gap_test_interface = NULL;
static btm_interface_t* manager;
static void* manager_handle = NULL;
static void* gap_hanlde = NULL;

static struct option main_options[] = {
    { "help", 0, 0, 'h' },
    { "version", 0, 0, 'v' },
    { 0, 0, 0, 0 }
};

static bt_command_t g_cmd_tables[] = {
    { "enable", enable_cmd, "enable stack" },
    { "disable", disable_cmd, "disable stack" },
    { "get_state", get_state_cmd, "get stack state" },
    { "get_ble_state", get_ble_state_cmd, "get stack ble state" },

#ifdef CONFIG_BLUETOOTH_SPP
    { "spp", spp_command, "<SPP> Serial Port Profile" },
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    { "hfp", hfp_client_command, "<HFP> HandFree Profile --Client" },
#endif
    { "a2dpsrc", NULL, "<A2DP> Advanced Audio Distribution Profile --Source" },
    { "gap", gap_cmd, "<GAP> General profile" },
    { "gatts", gatt_server_command, "<GATT> gatt server and le advertise" },
    { "gattc", gatt_client_command, "<GATT> gatt server and le scan" },
    { "help", usage_cmd, "Usage for bttools" },
    { "quit", quit_cmd, "Quit" },
};

static bt_command_t g_gap_tables[] = {
    { "scan_mode", set_scan_mode, "\"set scan mode       param: <mode>  <bondable> \"" },
    { "discovery", start_discovery, "\"start bluetooth discovery        param: <timer(n*1.28s)> \"" },
    { "stopdiscovery", stop_discovery, "\"stop bluetooth discovery      \"" },

    { "getaddr", get_local_address, "\"get local address      \"" },
    { "setIO", set_local_io_capability, "\"set local capaliblity        param: <iocapability> \"" },
    { "getname", get_local_name, "\"get local name        \"" },
    { "setname", set_local_name, "\"change local name        \"" },

    { "remotename", get_remote_name, "\"get remote name        param: <addr> \"" },
    { "replypair", reply_pair_request, "\"replay pair request        param: <addr> <accept>\"" },
    { "createbond", create_bond, "\"create bond device        param: <addr> \"" },
    { "cancelbond", cancel_bond, "\"cancel create bond        param: <addr> \"" },
    { "removebond", remove_bond, "\"remove bond device        param: <addr> \"" },
    { "getbonded", get_bonded_devices, "\"get bonded device list     \"" },
    { "getconnected", get_connected_devices, "\"get connected device list       \"" },
    { "servicediscovery", start_service_discovery, "\"start service discovery        param: <addr> <uuid>\"" },
    { "stopservicediscovery", stop_service_discovery, "\"stop service discovery        param: <addr> \"" },
    { "getremoteservice", get_remote_services, "\"get remote service        param: <addr> \"" },
    { "setclass", set_local_device_class, "\"set local class        param: <class> \"" },
    { "getclass", get_local_device_class, "\"get local class        \"" },
    { "setbleaddr", ble_set_address, "\"set ble address        param: <addr> \"" },

};

static struct option gap_options[] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
};

static int start_discovery(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    uint16_t timerout = atoi(argv[0]);
    gap_test_interface->bt_start_discovery(gap_hanlde, timerout);

    return 0;
}

static int stop_discovery(void* handle, int argc, char** argv)
{
    gap_test_interface->bt_stop_discovery(gap_hanlde);

    return 0;
}

static int set_scan_mode(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    uint16_t parameter0 = atoi(argv[0]);
    uint16_t parameter1 = atoi(argv[1]);
    bt_scan_mode scanMode = SCAN_MODE_NONE;
    bool bondable = false;
    if (parameter1 != 0)
        bondable = true;
    switch (parameter0) {
    case 0:
        scanMode = SCAN_MODE_NONE;
        break;
    case 1:
        scanMode = SCAN_MODE_CONNECTABLE;
        break;
    case 2:
        scanMode = SCAN_MODE_CONNECTABLE_DISCOVERABLE;
        break;
    default:
        break;
    }

    gap_test_interface->bt_set_scan_mode(gap_hanlde, scanMode, bondable);

    return 0;
}

static int get_local_address(void* handle, int argc, char** argv)
{
    bt_address addr;
    gap_test_interface->bt_get_local_address(gap_hanlde, addr);
    BT_LOGD("%s, bt_address :%s", __func__, addr_str(addr));
    return 0;
}

static int set_local_io_capability(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    uint16_t iocap = atoi(argv[0]);
    gap_test_interface->bt_set_local_io_capability(gap_hanlde, iocap);

    return 0;
}

static int get_local_name(void* handle, int argc, char** argv)
{

    char* name = gap_test_interface->bt_get_local_name(gap_hanlde);
    BT_LOGD("%s, name: %s", __func__, name);

    return 0;
}

static int set_local_name(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    char* name = malloc(strlen(argv[0]) + 1);
    memcpy(name, argv[0], strlen(argv[0]));
    name[strlen(argv[0]) + 1] = 0;
    gap_test_interface->bt_set_local_name(gap_hanlde, argv[0], strlen(argv[0]) + 1);

    return 0;
}

static int get_remote_name(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    str2ba(argv[0], device->addr);

    gap_test_interface->bt_get_remote_name(gap_hanlde, device);

    return 0;
}

static int reply_pair_request(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    uint16_t accept = atoi(argv[0]);
    gap_test_interface->bt_reply_pair_request(gap_hanlde, device, accept);

    return 0;
}

static int create_bond(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->bt_create_bond(gap_hanlde, device);

    return 0;
}

static int cancel_bond(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->bt_cancel_bond(gap_hanlde, device);

    return 0;
}

static int remove_bond(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->bt_remove_bond(gap_hanlde, device);

    return 0;
}

static int get_bonded_devices(void* handle, int argc, char** argv)
{

    bt_device_t device_list[MAX_PAIR_DEVICE];

    int ret = gap_test_interface->bt_get_bonded_devices(gap_hanlde, device_list);
    for (int i = 0; i < ret; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s, name : ", __func__, i, addr_str(device->addr), device->name);
    }

    return 0;
}

static int get_connected_devices(void* handle, int argc, char** argv)
{

    bt_device_t device_list[MAX_CONNECTED_DEVICE];

    int ret = gap_test_interface->bt_get_connected_devices(gap_hanlde, device_list);
    for (int i = 0; i < ret; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s, name: %s", __func__, i, addr_str(device->addr), device->name);
    }

    return 0;
}

static int start_service_discovery(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return -1;

    return 0;
}

static int stop_service_discovery(void* handle, int argc, char** argv)
{
    return 0;
}

static int get_remote_services(void* handle, int argc, char** argv)
{
    return 0;
}

static int set_local_device_class(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    uint16_t class = atoi(argv[0]);
    gap_test_interface->bt_set_local_device_class(gap_hanlde, class);

    return 0;
}

static int get_local_device_class(void* handle, int argc, char** argv)
{
    uint16_t class = gap_test_interface->bt_get_local_device_class(gap_hanlde);
    BT_LOGD("%s, class: %d", __func__, class);

    return 0;
}

static int ble_set_address(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    bt_device_t* device = malloc(sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->ble_set_address(gap_hanlde, device);
    free(device);
    return 0;
}

static void gap_usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_gap_tables); i++) {
        printf("\t%-8s\t%s\n", g_gap_tables[i].cmd, g_gap_tables[i].help);
    }
}

int gap_cmd(void* handle, int argc, char* argv[])
{
    int opt, ret = -1;

    if (gap_test_interface == NULL) {
        gap_test_interface = get_gap_instance();
        // /    gap_test_interface->gap_register_callbacks(manager_handle, &gap_hanlde, &gap_callbacks);
    }

    while ((opt = getopt_long(argc, argv, "h", gap_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            gap_usage();
            return 0;
        default:
            break;
        }
    }

    if (argc > 1) {
        for (int i = 0; i < ARRAY_SIZE(g_gap_tables); i++) {
            if (strcmp(g_gap_tables[i].cmd, argv[1]) == 0) {
                if (g_gap_tables[i].func) {
                    ret = g_gap_tables[i].func(handle, argc - 2, &argv[2]);
                }
            }
        }
    }

    if (ret < 0) {
        printf("UnKnow command %s\n", argv[1]);
        gap_usage();
    }

    return 0;
}

static void manager_state_changed_callback(bt_manager_bt_state state)
{
    BT_LOGD("%s", __func__);
#if 0 //name device_class and io had set in  stack_state_change
    char local_name[] = "BLUELET_NUTTX_Fzw";
    gap_test_interface->bt_set_local_name(gap_hanlde, local_name, sizeof(local_name));
    gap_test_interface->bt_set_local_device_class(gap_hanlde, BT_COD_SERVICE_RENDERING | BT_COD_SERVICE_AUDIO | BT_COD_SERVICE_TELEPHONY | BT_COD_AV_HEADSET);
    gap_test_interface->bt_set_local_io_capability(gap_hanlde, SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT);
#endif
    gap_test_interface->bt_set_scan_mode(gap_hanlde, SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
}

static void test_discovery_state_changed_callback(void* gap_handle, bt_discovery_state state)
{
    BT_LOGD("%s, state %d", __func__, state);
}

static void test_adapter_state_changed_callback(void* gap_handle, stack_state_t state)
{
    BT_LOGD("%s", __func__);
}

static void test_device_found_callback(void* gap_handle, bt_device_t* device)
{
    BT_LOGD("%s, device %02x:%02x:%02x:%02x:%02x:%02x", __func__, device->addr[0], device->addr[1], device->addr[2], device->addr[3], device->addr[4], device->addr[5]);
}

void test_connection_state_changed_callback(void* handle, bt_device_t* device, bt_connection_state state)
{
    //char* connection_state = NULL;
    BT_LOGD("%s, device %s, state:  %d", __func__, addr_str(device->addr), state);
}

void test_received_remote_name_callback(void* handle, bt_address bd_addr, char* bt_name, uint8_t length)
{
    BT_LOGD("%s, device %s, bt_name: %s", __func__, addr_str(bd_addr), bt_name);
}

void test_ssp_request_callback(void* handle, ssp_request_data_t* request_data)
{
    BT_LOGD("%s, : request_data->ssp_type: %d", __func__, request_data->ssp_type);

    if (request_data->ssp_type == GAP_SPP_TYPE_PASSKEY_CONFIRMATION) {
        SERVICE_SSP_REPLY_DATA_S reply;
        memcpy(reply.remote_addr, request_data->remote_addr, 6);
        reply.accept = true;
        reply.type = GAP_SPP_TYPE_PASSKEY_CONFIRMATION;
        service_adapter_gap_ssp_reply(&reply);
    }
    if (request_data->ssp_type == GAP_SPP_TYPE_PASSKEY_ENTRY) {
        SERVICE_SSP_REPLY_DATA_S reply;
        memcpy(reply.remote_addr, request_data->remote_addr, 6);
        reply.accept = true;
        reply.type = GAP_SPP_TYPE_PASSKEY_ENTRY;
        reply.passkey = request_data->pass_key;
        service_adapter_gap_ssp_reply(&reply);
    }
    if (request_data->ssp_type == GAP_SPP_TYPE_PASSKEY_NOTIFICATION) {
        SERVICE_SSP_REPLY_DATA_S reply;
        memcpy(reply.remote_addr, request_data->remote_addr, 6);
        reply.accept = true;
        reply.type = GAP_SPP_TYPE_PASSKEY_NOTIFICATION;
        // reply.passkey = request_data->pass_key;
        // service_adapter_gap_ssp_reply(&reply);
    }
}
void test_bond_state_changed_callback(void* handle, bt_device_t* device, bt_bond_state state)
{
    char* bond_state = NULL;
    switch (state) {
    case BT_BOND_STATE_NONE:
        bond_state = "BT_BOND_STATE_NONE";
        break;
    case BT_BOND_STATE_BONDING:
        bond_state = "BT_BOND_STATE_BONDING";
        break;
    case BT_BOND_STATE_BONDED:
        bond_state = "BT_BOND_STATE_BONDED";
        break;
    case BT_BOND_STATE_SDP_DONE:
        bond_state = "BT_BOND_STATE_SDP_DONE";
        break;
    case BT_BOND_STATE_BLE_NONE:
        bond_state = "BT_BOND_STATE_BLE_NONE";
        break;
    case BT_BOND_STATE_BLE_BONDING:
        bond_state = "BT_BOND_STATE_BLE_BONDING";
        break;
    case BT_BOND_STATE_BLE_BONDED:
        bond_state = "BT_BOND_STATE_BLE_BONDED";
        break;
    default:
        break;
    }
    BT_LOGD("%s, state:%s ", __func__, bond_state);
    BT_LOGD("%s, device : %s, state: %s", __func__, addr_str(device->addr), bond_state);
}
void test_local_name_callback(void* handle, char* bt_name, uint8_t length)
{
    BT_LOGD("%s,,  bt_name: %s", __func__, bt_name);
}
void test_local_address_callback(void* handle, bt_device_t* device)
{
    BT_LOGD("%s, device %s", __func__, addr_str(device->addr));
}
void test_local_device_class_callback(void* handle, uint32_t device_class)
{
    BT_LOGD("%s,  device_class is %d", __func__, device_class);
}
void test_smp_request_callback(void* gap_handle, ssp_request_data_t* request_data)
{
    BT_LOGD("%s,", __func__);
}
void test_pairing_request_callback(void* gap_handle, BD_ADDR remote_addr, bool local_initiate, bool is_bondable)
{
    BT_LOGD("%s,local_initiate: %d, is_bondable:%d,  device :%02x:%02x:%02x:%02x:%02x:%02x", __func__,
        local_initiate, is_bondable, remote_addr[0], remote_addr[1], remote_addr[2], remote_addr[3], remote_addr[4], remote_addr[5]);
}

btm_gap_callbacks_t gap_test_tool_callbacks = {
    .state_changed_cb = test_adapter_state_changed_callback,
    .discovery_state_changed_callback_cb = test_discovery_state_changed_callback,
    .device_found_callback_cb = test_device_found_callback,
    .connection_state_callback_cb = test_connection_state_changed_callback,
    .received_remote_name_callback_cb = test_received_remote_name_callback,
    .ssp_request_callback_cb = test_ssp_request_callback,
    .bond_state_changed_callback_cb = test_bond_state_changed_callback,
    .local_name_callback_cb = test_local_name_callback,
    .local_address_callback_cb = test_local_address_callback,
    .local_device_class_callback_cb = test_local_device_class_callback,
    .smp_requeset_cb = test_smp_request_callback,
    .pairing_request_cb = test_pairing_request_callback,
};

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
};

static int enable_cmd(void* handle, int argc, char** argv)
{
    manager->enable(handle);

    return 0;
}

static int disable_cmd(void* handle, int argc, char** argv)
{
    manager->disable(handle);

    return 0;
}

static int get_state_cmd(void* handle, int argc, char** argv)
{
    bt_manager_bt_state state = manager->bt_get_state(handle);
    BT_LOGD("%s, state: %d", __func__, state);
    return 0;
}

static int get_ble_state_cmd(void* handle, int argc, char** argv)
{
    bt_manager_ble_state state = manager->ble_get_state(handle);
    BT_LOGD("%s, state: %d", __func__, state);
    return 0;
}
static int usage_cmd(void* handle, int argc, char** argv)
{
    usage();

    return 0;
}

static int quit_cmd(void* handle, int argc, char** argv)
{
    //manager->cleanup(handle);

    return 0;
}

static void usage(void)
{
    printf("Usage:\n"
           "\tbttool [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_cmd_tables); i++) {
        printf("\t%-4s\t%s\n", g_cmd_tables[i].cmd, g_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tbttool <command> --help\n");
}

static void show_version(void)
{
    printf("Version :0.1.1\n");
}

static int execute_command(void* handle, int argc, char* argv[])
{
    for (int i = 0; i < ARRAY_SIZE(g_cmd_tables); i++) {
        if (strcmp(g_cmd_tables[i].cmd, argv[0]) == 0) {
            if (g_cmd_tables[i].func) {
                g_cmd_tables[i].func(handle, argc, &argv[0]);
                if (g_cmd_tables[i].func == quit_cmd)
                    return -2;
                return 0;
            }
        }
    }

    printf("UnKnow command %s\n", argv[0]);
    usage();

    return -1;
}

int main(int argc, char** argv)
{
    int opt;
    int _argc = 0;
    char* _argv[10];
    char* buffer;
    char* saveptr;
    int ret, len;

    while ((opt = getopt_long(argc, argv, "h-v", main_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            exit(0);
        case 'v':
            show_version();
            exit(0);
        default:
            break;
        }
    }

    //btm_manager init
    manager = get_bt_manager_interface();
    manager->init(&manager_handle, &mgt_cb);
    gap_test_interface = get_gap_instance();
    gap_test_interface->gap_register_callbacks(manager_handle, &gap_hanlde, &gap_test_tool_callbacks);

    buffer = malloc(CONFIG_NSH_LINELEN);
    if (!buffer)
        return -ENOMEM;

    while (1) {
        printf("bttool> ");
        fflush(stdout);

        len = readline(buffer, CONFIG_NSH_LINELEN, stdin, stdout);
        buffer[len] = '\0';
        if (len < 0)
            continue;

        if (buffer[0] == '!') {
#ifdef CONFIG_SYSTEM_SYSTEM
            system(buffer + 1);
#endif
            continue;
        }

        if (buffer[len - 1] == '\n')
            buffer[len - 1] = '\0';

        saveptr = NULL;
        char* tmpstr = buffer;

        while ((tmpstr = strtok_r(tmpstr, " ", &saveptr)) != NULL) {
            _argv[_argc] = tmpstr;
            _argc++;
            tmpstr = NULL;
        }

        if (_argc > 0) {
            ret = execute_command(manager_handle, _argc, _argv);
            memset(_argv, 0, sizeof(_argv));
            _argc = 0;
            if (ret == -2)
                break;
        }
    }

    free(buffer);
    //exit(1);
    return 0;
}