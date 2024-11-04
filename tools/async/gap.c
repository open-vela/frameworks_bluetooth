/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#include "bt_async.h"
#include "bt_tools.h"
#include "utils.h"

static void usage(void);
static int usage_cmd(void* handle, int argc, char** argv);
static int enable_cmd(void* handle, int argc, char** argv);
static int disable_cmd(void* handle, int argc, char** argv);
static int discovery_cmd(void* handle, int argc, char** argv);
static int get_state_cmd(void* handle, int argc, char** argv);
static int set_adapter_cmd(void* handle, int argc, char** argv);
static int get_adapter_cmd(void* handle, int argc, char** argv);
static int set_scanmode_cmd(void* handle, int argc, char** argv);
static int get_scanmode_cmd(void* handle, int argc, char** argv);
static int set_iocap_cmd(void* handle, int argc, char** argv);
static int get_iocap_cmd(void* handle, int argc, char** argv);
static int get_local_addr_cmd(void* handle, int argc, char** argv);
static int get_appearance_cmd(void* handle, int argc, char** argv);
static int set_appearance_cmd(void* handle, int argc, char** argv);
static int set_le_addr_cmd(void* handle, int argc, char** argv);
static int get_le_addr_cmd(void* handle, int argc, char** argv);
static int set_identity_addr_cmd(void* handle, int argc, char** argv);
static int set_scan_parameters_cmd(void* handle, int argc, char** argv);
static int get_local_name_cmd(void* handle, int argc, char** argv);
static int set_local_name_cmd(void* handle, int argc, char** argv);
static int get_local_cod_cmd(void* handle, int argc, char** argv);
static int set_local_cod_cmd(void* handle, int argc, char** argv);
static int pair_cmd(void* handle, int argc, char** argv);
static int pair_set_auto_cmd(void* handle, int argc, char** argv);
static int pair_reply_cmd(void* handle, int argc, char** argv);
static int pair_set_pincode_cmd(void* handle, int argc, char** argv);
static int pair_set_passkey_cmd(void* handle, int argc, char** argv);
static int pair_set_confirm_cmd(void* handle, int argc, char** argv);
static int pair_set_tk_cmd(void* handle, int argc, char** argv);
static int pair_set_oob_cmd(void* handle, int argc, char** argv);
static int pair_get_oob_cmd(void* handle, int argc, char** argv);
static int connect_cmd(void* handle, int argc, char** argv);
static int disconnect_cmd(void* handle, int argc, char** argv);
static int le_connect_cmd(void* handle, int argc, char** argv);
static int le_disconnect_cmd(void* handle, int argc, char** argv);
static int create_bond_cmd(void* handle, int argc, char** argv);
static int cancel_bond_cmd(void* handle, int argc, char** argv);
static int remove_bond_cmd(void* handle, int argc, char** argv);
static int device_show_cmd(void* handle, int argc, char** argv);
static int device_set_alias_cmd(void* handle, int argc, char** argv);
static int get_bonded_devices_cmd(void* handle, int argc, char** argv);
static int get_connected_devices_cmd(void* handle, int argc, char** argv);
static int search_cmd(void* handle, int argc, char** argv);
static int start_service_cmd(void* handle, int argc, char** argv);
static int stop_service_cmd(void* handle, int argc, char** argv);
static int set_phy_cmd(void* handle, int argc, char** argv);
static int dump_cmd(void* handle, int argc, char** argv);
static int quit_cmd(void* handle, int argc, char** argv);

static struct option le_conn_options[] = {
    { "addr", required_argument, 0, 'a' },
    { "type", required_argument, 0, 't' },
    { "defaults", no_argument, 0, 'd' },
    { "filter", required_argument, 0, 'f' },
    { "phy", required_argument, 0, 'p' },
    { "latency", required_argument, 0, 'l' },
    { "conn_interval_min", required_argument, 0, 0 },
    { "conn_interval_max", required_argument, 0, 0 },
    { "timeout", required_argument, 0, 'T' },
    { "scan_interval", required_argument, 0, 0 },
    { "scan_window", required_argument, 0, 0 },
    { "min_ce_length", required_argument, 0, 0 },
    { "max_ce_length", required_argument, 0, 0 },
    { "help", no_argument, 0, 'h' },
    { 0, 0, 0, 0 }
};

#define LE_CONN_USAGE "\n"                                                                                                      \
                      "\t -a or --addr, peer le device address\n"                                                               \
                      "\t -t or --type, peer le device address type, address type(0:public,1:random,2:public_id,3:random_id)\n" \
                      "\t -d or --default, use default parameter\n"                                                             \
                      "\t -f or --filter, connection filter policy, (0:addr,1:whitelist)\n"                                     \
                      "\t -p or --phy, init phy type, (0:1M,1:2M,2:Coded)\n"                                                    \
                      "\t -l or --latency, connection latency Range: 0x0000 to 0x01F3\n"                                        \
                      "\t --conn_interval_min, Range: 0x0006 to 0x0C80\n"                                                       \
                      "\t --conn_interval_max, Range: 0x0006 to 0x0C80\n"                                                       \
                      "\t -T or --timeout, supervision timeout Range: 0x000A to 0x0C80\n"                                       \
                      "\t --scan_interval, Range: 0x0004 to 0x4000\n"                                                           \
                      "\t --scan_window, Range: 0x0004 to 0x4000\n"                                                             \
                      "\t --min_ce_length, Range: 0x0000 to 0xFFFF\n"                                                           \
                      "\t --max_ce_length, Range: 0x0000 to 0xFFFF\n"

#define INQUIRY_USAGE "inquiry device\n"                                          \
                      "\t\t\t- start <timeout>(Range: 1-48, i.e., 1.28-61.44s)\n" \
                      "\t\t\t- stop"

#define SET_LE_PHY_USAGE "set le tx and rx phy, params: <addr><txphy><rxphy>(0:1M, 1:2M, 2:CODED)"

static bt_command_t g_async_cmd_tables[] = {
    { "enable", enable_cmd, 0, "enable stack" },
    { "disable", disable_cmd, 0, "disable stack" },
    { "state", get_state_cmd, 0, "get adapter state" },
    { "inquiry", discovery_cmd, 0, INQUIRY_USAGE },
    { "set", set_adapter_cmd, 0, "set adapter information, input \'set help\' show usage" },
    { "get", get_adapter_cmd, 0, "get adapter information, input \'get help\' show usage" },
    { "pair", pair_cmd, 0, "reply pair request, input \'pair help\' show usage" },
    { "connect", connect_cmd, 0, "connect classic peer device, params: <addr>" },
    { "disconnect", disconnect_cmd, 0, "disconnect peer device, params: <addr>" },
    { "leconnect", le_connect_cmd, 1, "connect le peer device, input \'leconnect -h\' show usage" },
    { "ledisconnect", le_disconnect_cmd, 0, "disconnect le peer device, params: <addr>" },
    { "createbond", create_bond_cmd, 0, "create bond, params: <addr> <transport>(0:BLE, 1:BREDR)" },
    { "cancelbond", cancel_bond_cmd, 0, "cancel bond, params: <addr>" },
    { "removebond", remove_bond_cmd, 0, "remove bond, params: <addr> <transport>(0:BLE, 1:BREDR)" },
    { "setalias", device_set_alias_cmd, 0, "set device alias, params: <addr>" },
    { "device", device_show_cmd, 0, "show device information, params: <addr>" },
    { "search", search_cmd, 0, "service serach <addr>, Not implemented" },
    { "start", start_service_cmd, 0, "start profile service, Not implemented" },
    { "stop", stop_service_cmd, 0, "stop profile service,  Not implemented" },
    { "setphy", set_phy_cmd, 0, SET_LE_PHY_USAGE },
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    { "adv", adv_command_exec, 0, "advertising cmd,   input \'adv\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    { "scan", scan_command_exec, 0, "scan cmd,          input \'scan\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    { "a2dpsnk", a2dp_sink_command_exec, 0, "a2dp sink cmd,    input \'a2dpsnk\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    { "a2dpsrc", a2dp_src_command_exec, 0, "a2dp source cmd,    input \'a2dpsrc\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    { "hf", hfp_hf_command_exec, 0, "hands-free cmd,    input \'hf\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_HFP_AG
    { "ag", hfp_ag_command_exec, 0, "audio-gateway cmd, input \'ag\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_SPP
    { "spp", spp_command_exec, 0, "serial port cmd,   input \'spp\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_HID_DEVICE
    { "hidd", hidd_command_exec, 0, "hid device cmd,    input \'hidd\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_PAN
    { "pan", pan_command_exec, 0, "pan cmd,           input \'pan\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_GATT
    { "gattc", gattc_command_exec, 0, "gatt client cmd    input \'gattc\' show usage" },
    { "gatts", gatts_command_exec, 0, "gatt server cmd    input \'gatts\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_SERVER
    { "leas", leas_command_exec, 0, "lea server cmd, input \'leas\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCP
    { "mcp", lea_mcp_command_exec, 0, "leaudio mcp cmd,  input \'mcp\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CCP
    { "ccp", lea_ccp_command_exec, 0, "lea ccp cmd, input \'ccp\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICS
    { "vmics", vmics_command_exec, 0, "vcp/micp server cmd, input \'vmics\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_CLIENT
    { "leac", leac_command_exec, 0, "lea client cmd, input \'leac\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_MCS
    { "mcs", lea_mcs_command_exec, 0, "leaudio mcp cmd,  input \'mcs\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_TBS
    { "tbs", lea_tbs_command_exec, 0, "lea tbs cmd, input \'tbs\' show usage" },
#endif
#ifdef CONFIG_BLUETOOTH_LEAUDIO_VMICP
    { "vmicp", vmicp_command_exec, 0, "vcp/micp client cmd, input \'vmicp\' show usage" },
#endif
    { "dump", dump_cmd, 0, "dump adapter state" },
    { "log", log_command, 0, "log control command" },
    { "help", usage_cmd, 0, "Usage for bttools" },
    { "quit", quit_cmd, 0, "Quit" },
    { "q", quit_cmd, 0, "Quit" },
};

#define SET_IOCAP_USAGE "params: <io capability> (0:displayonly, 1:yes&no, 2:keyboardonly, 3:no-in/no-out 4:keyboard&display)"
#define SET_CLASS_USAGE "params: <local class of device>, range in 0x0-0xFFFFFC, the 2 least significant shall be 0b00, example: 0x00640404"
#define SET_SCANPARAMS_USAGE "set scan parameters, params: <mode>(0: INQUIRY, 1: PAGE), <type>(0: standard, 1: interlaced), <interval>(range in 18-4096), <window>(range in 17-4096)"

static bt_command_t g_set_cmd_tables[] = {
    { "scanmode", set_scanmode_cmd, 0, "params: <scan mode> (0:none, 1:connectable 2:connectable&discoverable)" },
    { "iocap", set_iocap_cmd, 0, SET_IOCAP_USAGE },
    { "name", set_local_name_cmd, 0, "params: <local name>, example \"vela-bt\"" },
    { "class", set_local_cod_cmd, 0, SET_CLASS_USAGE },
    { "appearance", set_appearance_cmd, 0, "set le adapter appearance, params: <appearance>" },
    { "leaddr", set_le_addr_cmd, 0, "set ble adapter addr, params: <leaddr>" },
    { "id", set_identity_addr_cmd, 0, "set ble identity addr, params: <identity addr> <addr type>" },
    { "scanparams", set_scan_parameters_cmd, 0, SET_SCANPARAMS_USAGE },
    { "help", NULL, 0, "show set help info" },
    //{ "", , "set " },
};

static bt_command_t g_get_cmd_tables[] = {
    { "scanmode", get_scanmode_cmd, 0, "get adapter scan mode" },
    { "iocap", get_iocap_cmd, 0, "get adapter io capability" },
    { "addr", get_local_addr_cmd, 0, "get adapter local addr" },
    { "leaddr", get_le_addr_cmd, 0, "get ble adapter addr" },
    { "name", get_local_name_cmd, 0, "get adapter local name" },
    { "appearance", get_appearance_cmd, 0, "get le adapter appearance" },
    { "class", get_local_cod_cmd, 0, "get adapter local class of device" },
    { "bonded", get_bonded_devices_cmd, 0, "get bonded devices, params:<transport>(0:BLE, 1:BREDR)" },
    { "connected", get_connected_devices_cmd, 0, "get connected devices params:<transport>(0:BLE, 1:BREDR)" },
    { "help", NULL, 0, "show get help info" },
    //{ "", , "get " },
};

#define PAIR_PASSKEY_USAGE "input ssp passkey, params: <addr> <transport>(0:BLE, 1:BREDR)<reply>(0 :reject, 1: accept)<passkey>"
#define PAIR_CONFIRM_USAGE "set ssp confirmation, params: <addr> <transport> (0:BLE, 1:BREDR)<conform>(0 :reject, 1: accept)"

static bt_command_t g_pair_cmd_tables[] = {
    { "auto", pair_set_auto_cmd, 0, "enable pair auto reply, params: <enable>(0:disable, 1:enable)" },
    { "reply", pair_reply_cmd, 0, "reply the pair request, params: <addr><accept?>(0 :reject, 1: accept)" },
    { "pin", pair_set_pincode_cmd, 0, "input pin code, params: <addr><accept?>(0 :reject, 1: accept)<pincode>" },
    { "passkey", pair_set_passkey_cmd, 0, PAIR_PASSKEY_USAGE },
    { "confirm", pair_set_confirm_cmd, 0, PAIR_CONFIRM_USAGE },
    { "set_tk", pair_set_tk_cmd, 0, "set oob temporary key for le legacy pairing: <addr><tk_val>" },
    { "set_oob", pair_set_oob_cmd, 0, "set remote oob data for le sc pairing: <addr><c_val><r_val>" },
    { "get_oob", pair_get_oob_cmd, 0, "get local oob data for le sc pairing: <addr>" },
    { "help", NULL, 0, "show pair help info" },
    //{ "", , "set " },
};

static void* adapter_callback_async = NULL;
static bool g_cmd_had_inited = false;

extern bt_instance_t* g_bttool_ins;
extern bool g_auto_accept_pair;
extern bond_state_t g_bond_state;

static void status_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    PRINT("%s status: %d", __func__, status);
}

static void bt_tool_init(void* handle)
{
    if (g_cmd_had_inited)
        return;

    g_cmd_had_inited = true;
}

static void bt_tool_uninit(void* handle)
{
    if (!g_cmd_had_inited)
        return;

    g_cmd_had_inited = false;
}

static int enable_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_enable_async(handle, status_cb, NULL);
    return CMD_OK;
}

static int disable_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_disable_async(handle, status_cb, NULL);
    return CMD_OK;
}

static void get_state_cb(bt_instance_t* ins, bt_status_t status, bt_adapter_state_t state, void* userdata)
{
    PRINT("%s state: %d", __func__, state);
}

static int get_state_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_state_async(handle, get_state_cb, NULL);
    return CMD_OK;
}

static int discovery_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (!strcmp(argv[0], "start")) {
        if (argc < 2)
            return CMD_PARAM_NOT_ENOUGH;

        int timeout = atoi(argv[1]);
        if (timeout <= 0 || timeout > 48) {
            PRINT("%s, invalid timeout value:%d", __func__, timeout);
            return CMD_INVALID_PARAM;
        }

        PRINT("start discovery timeout:%d", timeout);
        if (bt_adapter_start_discovery_async(handle, timeout, status_cb, NULL) != BT_STATUS_SUCCESS)
            return CMD_ERROR;
    } else if (!strcmp(argv[0], "stop")) {
        if (bt_adapter_cancel_discovery_async(handle, status_cb, NULL) != BT_STATUS_SUCCESS)
            return CMD_ERROR;
    } else {
        return CMD_USAGE_FAULT;
    }

    return CMD_OK;
}

static void set_usage(void)
{
    printf("Usage:\n"
           "\tset [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_set_cmd_tables); i++) {
        printf("\t%-8s\t%s\n", g_set_cmd_tables[i].cmd, g_set_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tset help\n");
}

static void get_usage(void)
{
    printf("Usage:\n"
           "\tget [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_get_cmd_tables); i++) {
        printf("\t%-8s\t%s\n", g_get_cmd_tables[i].cmd, g_get_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tget help\n");
}

static void pair_usage(void)
{
    printf("Usage:\n"
           "\tpair [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_pair_cmd_tables); i++) {
        printf("\t%-8s\t%s\n", g_pair_cmd_tables[i].cmd, g_pair_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tpair help\n");
}

static int set_adapter_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        set_usage();
        return CMD_PARAM_NOT_ENOUGH;
    }

    int ret = execute_command_in_table(handle, g_set_cmd_tables, ARRAY_SIZE(g_set_cmd_tables), argc, argv);
    if (ret != CMD_OK)
        set_usage();

    return ret;
}

static int get_adapter_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        get_usage();
        return CMD_PARAM_NOT_ENOUGH;
    }

    int ret = execute_command_in_table(handle, g_get_cmd_tables, ARRAY_SIZE(g_get_cmd_tables), argc, argv);
    if (ret != CMD_OK)
        get_usage();

    return ret;
}

static int set_scanmode_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int scanmode = atoi(argv[0]);
    if (scanmode > BT_BR_SCAN_MODE_CONNECTABLE_DISCOVERABLE)
        return CMD_INVALID_PARAM;

    if (bt_adapter_set_scan_mode_async(handle, scanmode, 1, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Scan Mode:%d set success", scanmode);
    return CMD_OK;
}

static void get_scanmode_cb(bt_instance_t* ins, bt_status_t status, bt_scan_mode_t mode, void* userdata)
{
    PRINT("Scan Mode:%d", mode);
}

static int get_scanmode_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_scan_mode_async(handle, get_scanmode_cb, NULL);
    return CMD_OK;
}

static int set_iocap_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    if (strlen(argv[0]) > 1) {
        return CMD_INVALID_PARAM;
    }

    int iocap = *argv[0] - '0';
    if (iocap < BT_IO_CAPABILITY_DISPLAYONLY || iocap > BT_IO_CAPABILITY_KEYBOARDDISPLAY)
        return CMD_INVALID_PARAM;

    if (bt_adapter_set_io_capability_async(handle, iocap, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("IO Capability:%d set success", iocap);
    return CMD_OK;
}

static void get_iocap_cb(bt_instance_t* ins, bt_status_t status, bt_io_capability_t iocap, void* userdata)
{
    PRINT("IO Capability:%d", iocap);
}

static int get_iocap_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_io_capability_async(handle, get_iocap_cb, NULL);
    return CMD_OK;
}

static void get_local_addr_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addr, void* userdata)
{
    PRINT_ADDR("Local Address:[%s]", addr);
}

static int get_local_addr_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_address_async(handle, get_local_addr_cb, NULL);
    return CMD_OK;
}

static void get_appearance_cb(bt_instance_t* ins, bt_status_t status, uint16_t appearance, void* userdata)
{
    PRINT("Le appearance:0x%04x", appearance);
}

static int get_appearance_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_le_appearance_async(handle, get_appearance_cb, NULL);
    return CMD_OK;
}

static int set_appearance_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint32_t appearance = strtoul(argv[0], NULL, 16);
    bt_adapter_set_le_appearance_async(handle, appearance, status_cb, NULL);
    PRINT("Set Le appearance:0x%04" PRIx32 "", appearance);

    return CMD_OK;
}

static int set_le_addr_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    bt_adapter_set_le_address_async(handle, &addr, status_cb, NULL);

    return CMD_OK;
}

static void get_le_addr_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addr, ble_addr_type_t type, void* userdata)
{
    PRINT_ADDR("LE Address:%s, type:%d", addr, type);
}

static int get_le_addr_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_le_address_async(handle, get_le_addr_cb, NULL);
    return CMD_OK;
}

static int set_identity_addr_cmd(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int type = atoi(argv[1]);
    if (type != 0 && type != 1) {
        return CMD_INVALID_PARAM;
    }

    bt_adapter_set_le_identity_address_async(handle, &addr, type, status_cb, NULL);

    return CMD_OK;
}

static int set_scan_parameters_cmd(void* handle, int argc, char** argv)
{
    if (argc < 4)
        return CMD_PARAM_NOT_ENOUGH;

    int is_page = atoi(argv[0]);
    if (is_page != 0 && is_page != 1)
        return CMD_INVALID_PARAM;

    int type = atoi(argv[1]);
    if (type != 0 && type != 1)
        return CMD_INVALID_PARAM;

    int interval = atoi(argv[2]);
    if (interval < 0x12 || interval > 0x1000)
        return CMD_INVALID_PARAM;

    int window = atoi(argv[3]);
    if (window < 0x11 || window > 0x1000)
        return CMD_INVALID_PARAM;

    if (!is_page)
        bt_adapter_set_inquiry_scan_parameters_async(handle, type, interval, window, status_cb, NULL);
    else
        bt_adapter_set_page_scan_parameters_async(handle, type, interval, window, status_cb, NULL);

    return CMD_OK;
}

static void get_local_name_cb(bt_instance_t* ins, bt_status_t status, const char* name, void* userdata)
{
    PRINT("Local Name:%s", name);
}

static int get_local_name_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_name_async(handle, get_local_name_cb, NULL);
    return CMD_OK;
}

static int set_local_name_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    char* name = argv[0];
    if (strlen(name) > 63) {
        PRINT("name length to long");
        return CMD_INVALID_PARAM;
    }

    if (bt_adapter_set_name_async(handle, name, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Local Name:%s set success", name);
    return CMD_OK;
}

static void get_local_cod_cb(bt_instance_t* ins, bt_status_t status, uint32_t cod, void* userdata)
{
    PRINT("Local class of device: 0x%08" PRIx32 ", is HEADSET: %s", cod, IS_HEADSET(cod) ? "true" : "false");
}

static int get_local_cod_cmd(void* handle, int argc, char** argv)
{
    bt_adapter_get_device_class_async(handle, get_local_cod_cb, NULL);
    return CMD_OK;
}

static int set_local_cod_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    uint32_t cod = strtol(argv[0], NULL, 16);

    if (cod > 0xFFFFFF || cod & 0x3)
        return CMD_INVALID_PARAM;

    if (bt_adapter_set_device_class_async(handle, cod, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Local class of device:0x%08" PRIx32 " set success", cod);
    return CMD_OK;
}

static int pair_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        pair_usage();
        return CMD_PARAM_NOT_ENOUGH;
    }

    int ret = execute_command_in_table(handle, g_pair_cmd_tables, ARRAY_SIZE(g_pair_cmd_tables), argc, argv);
    if (ret != CMD_OK)
        pair_usage();

    return ret;
}

extern bool g_auto_accept_pair;

static int pair_set_auto_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;
    if (strlen(argv[0]) > 1) {
        return CMD_INVALID_PARAM;
    }
    switch (*argv[0]) {
    case '0':
        g_auto_accept_pair = false;
        break;
    case '1':
        g_auto_accept_pair = true;
        break;
    default:
        return CMD_INVALID_PARAM;
        break;
    }

    PRINT("Auto accept pair:%s", g_auto_accept_pair ? "Enable" : "Disable");

    return CMD_OK;
}

static int pair_reply_cmd(void* handle, int argc, char** argv)
{
    bt_address_t addr;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int reply = atoi(argv[1]);
    if (reply != 0 && reply != 1)
        return CMD_INVALID_PARAM;

    /* TODO: Check bond state*/
    if (bt_device_pair_request_reply_async(handle, &addr, reply, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device [%s] pair request %s", argv[0], reply ? "Accept" : "Reject");
    return CMD_OK;
}

static int pair_set_pincode_cmd(void* handle, int argc, char** argv)
{
    bt_address_t addr;
    char* pincode = NULL;
    uint8_t pincode_len = 0;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int reply = atoi(argv[1]);
    if (reply != 0 && reply != 1)
        return CMD_INVALID_PARAM;

    if (reply) {
        if (argc < 3)
            return CMD_PARAM_NOT_ENOUGH;
        pincode = argv[2];
        pincode_len = strlen(pincode);
    }

    /* TODO: Check bond state*/
    if (bt_device_set_pin_code_async(handle, &addr, reply, pincode, pincode_len, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device [%s] pincode request %s, code:%s", argv[0], reply ? "Accept" : "Reject", pincode);
    return CMD_OK;
}

static int pair_set_passkey_cmd(void* handle, int argc, char** argv)
{
    bt_address_t addr;
    int passkey = 0;

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int transport = atoi(argv[1]);
    if (transport != BT_TRANSPORT_BREDR && transport != BT_TRANSPORT_BLE)
        return CMD_INVALID_PARAM;

    int reply = atoi(argv[2]);
    if (reply != 0 && reply != 1)
        return CMD_INVALID_PARAM;

    if (reply) {
        if (argc < 4)
            return CMD_PARAM_NOT_ENOUGH;

        char tmp[7] = { 0 };
        strncpy(tmp, argv[3], 6);
        passkey = atoi(tmp);
        if (passkey > 1000000) {
            PRINT("Invalid passkey");
            return CMD_INVALID_PARAM;
        }
    }

    /* TODO: Check bond state*/
    if (bt_device_set_pass_key_async(handle, &addr, transport, reply, passkey, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device [%s] passkey request %s, passkey:%d", argv[0], reply ? "Accept" : "Reject", passkey);
    return CMD_OK;
}

static int pair_set_confirm_cmd(void* handle, int argc, char** argv)
{
    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int transport = atoi(argv[1]);
    if (transport != BT_TRANSPORT_BREDR && transport != BT_TRANSPORT_BLE)
        return CMD_INVALID_PARAM;

    int reply = atoi(argv[2]);
    if (reply != 0 && reply != 1)
        return CMD_INVALID_PARAM;

    /* TODO: Check bond state*/
    if (bt_device_set_pairing_confirmation_async(handle, &addr, transport, reply, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device [%s] ssp confirmation %s", argv[0], reply ? "Accept" : "Reject");
    return CMD_OK;
}

static void str2hex(char* src_str, uint8_t* dest_buf, uint8_t hex_number)
{
    uint8_t i;
    uint8_t lb, hb;

    for (i = 0; i < hex_number; i++) {
        lb = src_str[(i << 1) + 1];
        hb = src_str[i << 1];
        if (hb >= '0' && hb <= '9') {
            dest_buf[i] = hb - '0';
        } else if (hb >= 'A' && hb < 'G') {
            dest_buf[i] = hb - 'A' + 10;
        } else if (hb >= 'a' && hb < 'g') {
            dest_buf[i] = hb - 'a' + 10;
        } else {
            dest_buf[i] = 0;
        }

        dest_buf[i] <<= 4;
        if (lb >= '0' && lb <= '9') {
            dest_buf[i] += lb - '0';
        } else if (lb >= 'A' && lb < 'G') {
            dest_buf[i] += lb - 'A' + 10;
        } else if (lb >= 'a' && lb < 'g') {
            dest_buf[i] += lb - 'a' + 10;
        }
    }
}

static int pair_set_tk_cmd(void* handle, int argc, char** argv)
{
    bt_128key_t tk_val;

    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (strlen(argv[1]) < (sizeof(bt_128key_t) * 2)) {
        PRINT("length of temporary key is insufficient");
        return CMD_INVALID_PARAM;
    }

    str2hex(argv[1], tk_val, sizeof(bt_128key_t));

    if (bt_device_set_le_legacy_tk_async(handle, &addr, tk_val, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Set oob temporary key for le legacy pairing with [%s]", argv[0]);
    return CMD_OK;
}

static int pair_set_oob_cmd(void* handle, int argc, char** argv)
{
    bt_128key_t c_val;
    bt_128key_t r_val;

    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (strlen(argv[1]) < (sizeof(bt_128key_t) * 2)) {
        PRINT("length of confirmation value is insufficient");
        return CMD_INVALID_PARAM;
    }

    if (strlen(argv[2]) < (sizeof(bt_128key_t) * 2)) {
        PRINT("length of random value is insufficient");
        return CMD_INVALID_PARAM;
    }

    str2hex(argv[1], c_val, sizeof(bt_128key_t));
    str2hex(argv[2], r_val, sizeof(bt_128key_t));

    if (bt_device_set_le_sc_remote_oob_data_async(handle, &addr, c_val, r_val, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Set remote oob data for le secure connection pairing with [%s]", argv[0]);
    return CMD_OK;
}

static int pair_get_oob_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_device_get_le_sc_local_oob_data_async(handle, &addr, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Get local oob data for le secure connection pairing with [%s]", argv[0]);
    return CMD_OK;
}

static int connect_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_device_connect_async(handle, &addr, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device[%s] connecting", argv[0]);
    return CMD_OK;
}

static int disconnect_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_device_disconnect_async(handle, &addr, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device[%s] disconnecting", argv[0]);
    return CMD_OK;
}

static int le_connect_cmd(void* handle, int argc, char** argv)
{
    int opt, index = 0;
    bt_address_t addr;
    ble_addr_type_t addrtype = BT_LE_ADDR_TYPE_PUBLIC;
    ble_connect_params_t params = {
        .use_default_params = false,
        .filter_policy = BT_LE_CONNECT_FILTER_POLICY_ADDR,
        .init_phy = BT_LE_1M_PHY,
        .scan_interval = 20, /* 12.5 ms */
        .scan_window = 20, /* 12.5 ms */
        .connection_interval_min = 24, /* 30 ms */
        .connection_interval_max = 24, /* 30 ms */
        .connection_latency = 0,
        .supervision_timeout = 18, /* 180 ms */
        .min_ce_length = 0,
        .max_ce_length = 0,
    };

    bt_addr_set_empty(&addr);
    optind = 1;
    while ((opt = getopt_long(argc, argv, "a:t:f:p:l:T:dh", le_conn_options,
                &index))
        != -1) {
        switch (opt) {
        case 'a': {
            if (bt_addr_str2ba(optarg, &addr) < 0) {
                PRINT("Invalid addr:%s", optarg);
                return CMD_INVALID_ADDR;
            }

        } break;
        case 't': {
            int32_t type = atoi(optarg);
            addrtype = type;
        } break;
        case 'd': {
            params.use_default_params = true;
        } break;
        case 'f': {
            int32_t filter = atoi(optarg);
            if (filter != BT_LE_CONNECT_FILTER_POLICY_ADDR && filter != BT_LE_CONNECT_FILTER_POLICY_WHITE_LIST) {
                PRINT("Invalid filter:%s", optarg);
                return CMD_INVALID_PARAM;
            }

            params.filter_policy = filter;
        } break;
        case 'p': {
            int32_t phy = atoi(optarg);
            if (!phy_is_vaild(phy)) {
                PRINT("Invalid phy:%s", optarg);
                return CMD_INVALID_PARAM;
            }
            params.init_phy = phy;
        } break;
        case 'l': {
            int32_t latency = atoi(optarg);
            if (latency < 0 || latency > 0x01F3) {
                PRINT("Invalid latency:%s", optarg);
                return CMD_INVALID_PARAM;
            }
            params.connection_latency = latency;
        } break;
        case 'T': {
            int32_t timeout = atoi(optarg);
            if (timeout < 0x0A || timeout > 0x0C80) {
                PRINT("Invalid supervision_timeout:%s", optarg);
                return CMD_INVALID_PARAM;
            }
            params.supervision_timeout = timeout;
        } break;
        case 'h': {
            PRINT("%s", LE_CONN_USAGE);
        } break;
        case 0: {
            const char* curopt = le_conn_options[index].name;
            int32_t val = atoi(optarg);

            if (strncmp(curopt, "conn_interval_min", strlen("conn_interval_min")) == 0) {
                if (val < 0x06 || val > 0x0C80) {
                    PRINT("Invalid conn_interval_min:%s", optarg);
                    return CMD_INVALID_PARAM;
                }
                params.connection_interval_min = val;
            } else if (strncmp(curopt, "conn_interval_max", strlen("conn_interval_max")) == 0) {
                if (val < 0x06 || val > 0x0C80) {
                    PRINT("Invalid conn_interval_max:%s", optarg);
                    return CMD_INVALID_PARAM;
                }
                params.connection_interval_max = val;
            } else if (strncmp(curopt, "scan_interval", strlen("scan_interval")) == 0) {
                if (val < 0x04 || val > 0x4000) {
                    PRINT("Invalid scan_interval:%s", optarg);
                    return CMD_INVALID_PARAM;
                }
                params.scan_interval = val;
            } else if (strncmp(curopt, "scan_window", strlen("scan_window")) == 0) {
                if (val < 0x04 || val > 0x4000) {
                    PRINT("Invalid scan_window:%s", optarg);
                    return CMD_INVALID_PARAM;
                }
                params.scan_window = val;
            } else if (strncmp(curopt, "min_ce_length", strlen("min_ce_length")) == 0) {
                if (val < 0x0A || val > 0x0C80) {
                    PRINT("Invalid min_ce_length:%s", optarg);
                    return CMD_INVALID_PARAM;
                }
                params.min_ce_length = val;
            } else if (strncmp(curopt, "max_ce_length", strlen("max_ce_length")) == 0) {
                if (val < 0x0A || val > 0x0C80) {
                    PRINT("Invalid max_ce_length:%s", optarg);
                    return CMD_INVALID_PARAM;
                }
                params.max_ce_length = val;
            } else {
                return CMD_INVALID_OPT;
            }
        } break;
        default:
            return CMD_INVALID_OPT;
        }
    }

    if (bt_addr_is_empty(&addr))
        return CMD_INVALID_ADDR;

    if (bt_device_connect_le_async(handle, &addr, addrtype, &params, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    return CMD_OK;
}

static int le_disconnect_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (bt_device_disconnect_le_async(handle, &addr, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("LE Device[%s] disconnecting", argv[0]);
    return CMD_OK;
}

static int create_bond_cmd(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int transport = atoi(argv[1]);
    if (transport != BT_TRANSPORT_BREDR && transport != BT_TRANSPORT_BLE)
        return CMD_INVALID_PARAM;

    /* TODO: Check bond state*/
    if (bt_device_create_bond_async(handle, &addr, transport, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device [%s] create bond", argv[0]);
    return CMD_OK;
}

static int cancel_bond_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    /* TODO: Check bond state*/
    if (bt_device_cancel_bond_async(handle, &addr, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device [%s] cancel bond", argv[0]);
    return CMD_OK;
}

static int remove_bond_cmd(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int transport = atoi(argv[1]);
    if (transport != BT_TRANSPORT_BREDR && transport != BT_TRANSPORT_BLE)
        return CMD_INVALID_PARAM;

    /* TODO: Check bond state*/
    if (bt_device_remove_bond_async(handle, &addr, transport, status_cb, NULL) != BT_STATUS_SUCCESS)
        return CMD_ERROR;

    PRINT("Device [%s] remove bond", argv[0]);
    return CMD_OK;
}

static int set_phy_cmd(void* handle, int argc, char** argv)
{
    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int tx_phy, rx_phy;
    tx_phy = atoi(argv[1]);
    rx_phy = atoi(argv[2]);
    if (!phy_is_vaild(tx_phy) || !phy_is_vaild(rx_phy)) {
        PRINT("Invalid phy parameter, tx:%d, rx:%d", tx_phy, rx_phy);
        return CMD_INVALID_PARAM;
    }

    bt_device_set_le_phy_async(handle, &addr, tx_phy, rx_phy, status_cb, NULL);

    return CMD_OK;
}

static const char* bond_state_to_string(bond_state_t state)
{
    switch (state) {
    case BOND_STATE_NONE:
        return "BOND_NONE";
    case BOND_STATE_BONDING:
        return "BONDING";
    case BOND_STATE_BONDED:
        return "BONDED";
    default:
        return "UNKNOWN";
    }
}

static void get_device_alias_cb(bt_instance_t* ins, bt_status_t status, const char* alias, void* userdata)
{
    PRINT("\tAlias: %s", alias);
}

static void get_device_name_cb(bt_instance_t* ins, bt_status_t status, const char* alias, void* userdata)
{
    PRINT("\tNmae: %s", alias);
}

static void get_device_class_cb(bt_instance_t* ins, bt_status_t status, uint32_t class, void* userdata)
{
    PRINT("\tClass: 0x%08" PRIx32 "", class);
}

static void get_device_type_cb(bt_instance_t* ins, bt_status_t status, bt_device_type_t type, void* userdata)
{
    PRINT("\tDeviceType: %d", type);
}

static void is_connected_cb(bt_instance_t* ins, bt_status_t status, bool connected, void* userdata)
{
    PRINT("\tIsConnected: %d", connected);
}

static void is_encrypted_cb(bt_instance_t* ins, bt_status_t status, bool encrypted, void* userdata)
{
    PRINT("\tIsEncrypted: %d", encrypted);
}

static void is_bonded_cb(bt_instance_t* ins, bt_status_t status, bool bonded, void* userdata)
{
    PRINT("\tIsBonded: %d", bonded);
}

static void get_bond_state_cb(bt_instance_t* ins, bt_status_t status, bond_state_t state, void* userdata)
{
    PRINT("\tBondState: %s", bond_state_to_string(state));
}

static void is_bond_initiate_local_cb(bt_instance_t* ins, bt_status_t status, bool initiate, void* userdata)
{
    PRINT("\tIsBondInitiateLocal: %d", initiate);
}

static void get_uuids_cb(bt_instance_t* ins, bt_status_t status, bt_uuid_t* uuids, uint16_t uuid_cnt, void* userdata)
{
    PRINT("\tUUIDs:[%d]", uuid_cnt);
    for (int i = 0; i < uuid_cnt; i++) {
        char uuid_str[40] = { 0 };
        bt_uuid_to_string(uuids + i, uuid_str, 40);
        PRINT("\t\tuuid[%-2d]: %s", i, uuid_str);
    }
}

static void device_dump(void* handle, bt_address_t* addr, bt_transport_t transport)
{
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    PRINT("device [%s]", addr_str);
    if (transport == BT_TRANSPORT_BREDR) {
        bt_device_get_name_async(handle, addr, get_device_name_cb, NULL);
        bt_device_get_alias_async(handle, addr, get_device_alias_cb, NULL);
        bt_device_get_device_class_async(handle, addr, get_device_class_cb, NULL);
        bt_device_get_device_type_async(handle, addr, get_device_type_cb, NULL);
        bt_device_is_connected_async(handle, addr, transport, is_connected_cb, NULL);
        bt_device_is_encrypted_async(handle, addr, transport, is_encrypted_cb, NULL);
        bt_device_is_bonded_async(handle, addr, transport, is_bonded_cb, NULL);
        bt_device_get_bond_state_async(handle, addr, transport, get_bond_state_cb, NULL);
        bt_device_is_bond_initiate_local_async(handle, addr, transport, is_bond_initiate_local_cb, NULL);
        bt_device_get_uuids_async(handle, addr, get_uuids_cb, NULL);
    } else {
        bt_device_is_connected_async(handle, addr, transport, is_connected_cb, NULL);
        bt_device_is_encrypted_async(handle, addr, transport, is_encrypted_cb, NULL);
        bt_device_is_bonded_async(handle, addr, transport, is_bonded_cb, NULL);
        bt_device_get_bond_state_async(handle, addr, transport, get_bond_state_cb, NULL);
        bt_device_is_bond_initiate_local_async(handle, addr, transport, is_bond_initiate_local_cb, NULL);
    }
}

static int device_show_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    device_dump(handle, &addr, BT_TRANSPORT_BREDR);

    return CMD_OK;
}

static int device_set_alias_cmd(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    if (strlen(argv[1]) > 63) {
        PRINT("alias length too long");
        return CMD_INVALID_PARAM;
    }

    bt_device_set_alias_async(handle, &addr, argv[1], status_cb, NULL);
    PRINT("Device: [%s] alias:%s set success", argv[0], argv[1]);
    return CMD_OK;
}

static void get_devices_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addrs, int num, int transport, void* userdata)
{
    for (int i = 0; i < num; i++) {
        device_dump(ins, addrs + i, transport);
    }
}

static void get_br_bonded_devices_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addrs, int num, void* userdata)
{
    PRINT("BREDR bonded device cnt:%d", num);
    get_devices_cb(ins, status, addrs, num, BT_TRANSPORT_BREDR, userdata);
}

static void get_le_bonded_devices_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addrs, int num, void* userdata)
{
    PRINT("LE bonded device cnt:%d", num);
    get_devices_cb(ins, status, addrs, num, BT_TRANSPORT_BREDR, userdata);
}

static int get_bonded_devices_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int transport = atoi(argv[0]);
    if (transport != BT_TRANSPORT_BREDR && transport != BT_TRANSPORT_BLE)
        return CMD_INVALID_PARAM;

    bt_adapter_get_bonded_devices_async(handle, transport,
        transport == BT_TRANSPORT_BREDR ? get_br_bonded_devices_cb : get_le_bonded_devices_cb, NULL);

    return CMD_OK;
}

static void get_br_connected_devices_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addrs, int num, void* userdata)
{
    PRINT("BREDR connected device cnt:%d", num);
    get_devices_cb(ins, status, addrs, num, BT_TRANSPORT_BREDR, userdata);
}

static void get_le_connected_devices_cb(bt_instance_t* ins, bt_status_t status, bt_address_t* addrs, int num, void* userdata)
{
    PRINT("LE connected device cnt:%d", num);
    get_devices_cb(ins, status, addrs, num, BT_TRANSPORT_BREDR, userdata);
}
static int get_connected_devices_cmd(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    int transport = atoi(argv[0]);
    if (transport != BT_TRANSPORT_BREDR && transport != BT_TRANSPORT_BLE)
        return CMD_INVALID_PARAM;

    bt_adapter_get_connected_devices_async(handle, transport,
        transport == BT_TRANSPORT_BREDR ? get_br_connected_devices_cb : get_le_connected_devices_cb, NULL);
    return CMD_OK;
}

static int search_cmd(void* handle, int argc, char** argv)
{
    PRINT("%s", __func__);
    return CMD_OK;
}

static int start_service_cmd(void* handle, int argc, char** argv)
{
    return CMD_OK;
}

static int stop_service_cmd(void* handle, int argc, char** argv)
{
    return CMD_OK;
}

static int dump_cmd(void* handle, int argc, char** argv)
{
    return CMD_OK;
}

static int usage_cmd(void* handle, int argc, char** argv)
{
    if (argc == 2 && !strcmp(argv[1], "me!!!"))
        return -2;

    usage();

    return CMD_OK;
}

static int quit_cmd(void* handle, int argc, char** argv)
{
    return -2;
}

static void usage(void)
{
    printf("Usage:\n"
           "\tbttool [options] <command> [command parameters]\n");
    printf("Options:\n"
           "\t--help\tDisplay help\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_async_cmd_tables); i++) {
        printf("\t%-8s\t%s\n", g_async_cmd_tables[i].cmd, g_async_cmd_tables[i].help);
    }
    printf("\n"
           "For more information on the usage of each command use:\n"
           "\tbttool <command> --help\n");
}

int execute_async_command(void* handle, int argc, char* argv[])
{
    int ret;

    for (int i = 0; i < ARRAY_SIZE(g_async_cmd_tables); i++) {
        if (strlen(g_async_cmd_tables[i].cmd) == strlen(argv[0]) && strncmp(g_async_cmd_tables[i].cmd, argv[0], strlen(argv[0])) == 0) {
            if (g_async_cmd_tables[i].func) {
                if (g_async_cmd_tables[i].opt)
                    ret = g_async_cmd_tables[i].func(handle, argc, &argv[0]);
                else
                    ret = g_async_cmd_tables[i].func(handle, argc - 1, &argv[1]);
                if (g_async_cmd_tables[i].func == quit_cmd)
                    return -2;
                return ret;
            }
        }
    }

    PRINT("UnKnow command %s", argv[0]);
    usage();

    return CMD_UNKNOWN;
}

static void on_adapter_state_changed_cb(void* cookie, bt_adapter_state_t state)
{
    PRINT("Context:%p, Adapter state changed: %d", cookie, state);
    if (state == BT_ADAPTER_STATE_ON) {

        bt_tool_init(g_bttool_ins);
        /* get name */
        bt_adapter_get_name_async(g_bttool_ins, get_local_name_cb, NULL);
        /* get io cap */
        bt_adapter_get_io_capability_async(g_bttool_ins, get_iocap_cb, NULL);
        /* get class */
        bt_adapter_get_device_class_async(g_bttool_ins, get_local_cod_cb, NULL);
        /* get scan mode */
        bt_adapter_get_scan_mode_async(g_bttool_ins, get_scanmode_cb, NULL);
        /* enable key derivation */
        bt_adapter_le_enable_key_derivation_async(g_bttool_ins, true, true, status_cb, NULL);
        bt_adapter_set_page_scan_parameters_async(g_bttool_ins, BT_BR_SCAN_TYPE_INTERLACED, 0x400, 0x24, status_cb, NULL);
    } else if (state == BT_ADAPTER_STATE_TURNING_OFF) {
        /* code */
        bt_tool_uninit(g_bttool_ins);
    } else if (state == BT_ADAPTER_STATE_OFF) {
        /* do something */
    }
}

static void on_discovery_state_changed_cb(void* cookie, bt_discovery_state_t state)
{
    PRINT("Discovery state: %s", state == BT_DISCOVERY_STATE_STARTED ? "Started" : "Stopped");
}

static void on_discovery_result_cb(void* cookie, bt_discovery_result_t* result)
{
    PRINT_ADDR("Inquiring: device [%s], name: %s, cod: %08" PRIx32 ", is HEADSET: %s, rssi: %d",
        &result->addr, result->name, result->cod, IS_HEADSET(result->cod) ? "true" : "false", result->rssi);
}

static void on_scan_mode_changed_cb(void* cookie, bt_scan_mode_t mode)
{
    PRINT("Adapter new scan mode: %d", mode);
}

static void on_device_name_changed_cb(void* cookie, const char* device_name)
{
    PRINT("Adapter update device name: %s", device_name);
}

static void on_pair_request_cb(void* cookie, bt_address_t* addr)
{
    if (g_auto_accept_pair)
        bt_device_pair_request_reply_async(g_bttool_ins, addr, true, status_cb, NULL);

    PRINT_ADDR("Incoming pair request from [%s] %s", addr, g_auto_accept_pair ? "auto accepted" : "please reply");
}

#define LINK_TYPE(trans_) (trans_ == BT_TRANSPORT_BREDR ? "BREDR" : "LE")

static void on_pair_display_cb(void* cookie, bt_address_t* addr, bt_transport_t transport, bt_pair_type_t type, uint32_t passkey)
{
    uint8_t ret = 0;
    char buff[128] = { 0 };
    char buff1[64] = { 0 };
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    bt_addr_ba2str(addr, addr_str);
    sprintf(buff, "Pair Display [%s][%s]", addr_str, LINK_TYPE(transport));
    switch (type) {
    case PAIR_TYPE_PASSKEY_CONFIRMATION:
        if (!g_auto_accept_pair) {
            sprintf(buff1, "[SSP][CONFIRM][%" PRIu32 "] please reply:", passkey);
            break;
        }
        ret = bt_device_set_pairing_confirmation_async(g_bttool_ins, addr, transport, true, status_cb, NULL);
        sprintf(buff1, "[SSP][CONFIRM] Auto confirm [%" PRIu32 "] %s", passkey, ret == BT_STATUS_SUCCESS ? "SUCCESS" : "FAILED");
        break;
    case PAIR_TYPE_PASSKEY_ENTRY:
        sprintf(buff1, "[SSP][ENTRY][%" PRIu32 "], please reply:", passkey);
        break;
    case PAIR_TYPE_CONSENT:
        sprintf(buff1, "[SSP][CONSENT]");
        break;
    case PAIR_TYPE_PASSKEY_NOTIFICATION:
        sprintf(buff1, "[SSP][NOTIFY][%" PRIu32 "]", passkey);
        break;
    case PAIR_TYPE_PIN_CODE:
        sprintf(buff1, "[PIN] please reply:");
        break;
    }
    strcat(buff, buff1);
    PRINT("%s", buff);
}

static void on_connect_request_cb(void* cookie, bt_address_t* addr)
{
    bt_device_connect_request_reply_async(g_bttool_ins, addr, true, status_cb, NULL);
    PRINT_ADDR("Incoming connect request from [%s], auto accepted", addr);
}

static void on_connection_state_changed_cb(void* cookie, bt_address_t* addr, bt_transport_t transport, connection_state_t state)
{
    PRINT_ADDR("Device [%s][%s] connection state: %d", addr, LINK_TYPE(transport), state);
}

static void on_bond_state_changed_cb(void* cookie, bt_address_t* addr, bt_transport_t transport, bond_state_t state, bool is_ctkd)
{
    g_bond_state = state;
    PRINT_ADDR("Device [%s][%s] bond state: %s, is_ctkd: %d", addr, LINK_TYPE(transport), bond_state_to_string(state), is_ctkd);
}

static void on_le_sc_local_oob_data_got_cb(void* cookie, bt_address_t* addr, bt_128key_t c_val, bt_128key_t r_val)
{
    PRINT_ADDR("Generate local oob data for le secure connection pairing with [%s]:", addr);

    printf("\tConfirmation value: ");
    for (int i = 0; i < sizeof(bt_128key_t); i++) {
        printf("%02x", c_val[i]);
    }
    printf("\n");

    printf("\tRandom value: ");
    for (int i = 0; i < sizeof(bt_128key_t); i++) {
        printf("%02x", r_val[i]);
    }
    printf("\n");
}

static void on_remote_name_changed_cb(void* cookie, bt_address_t* addr, const char* name)
{
    PRINT_ADDR("Device [%s] name changed: %s", addr, name);
}

static void on_remote_alias_changed_cb(void* cookie, bt_address_t* addr, const char* alias)
{
    PRINT_ADDR("Device [%s] alias changed: %s", addr, alias);
}

static void on_remote_cod_changed_cb(void* cookie, bt_address_t* addr, uint32_t cod)
{
    PRINT_ADDR("Device [%s] class changed: 0x%08" PRIx32 "", addr, cod);
}

static void on_remote_uuids_changed_cb(void* cookie, bt_address_t* addr, bt_uuid_t* uuids, uint16_t size)
{
    char uuid_str[40] = { 0 };

    PRINT_ADDR("Device [%s] uuids changed", addr);

    if (size) {
        PRINT("UUIDs:[%d]", size);
        for (int i = 0; i < size; i++) {
            bt_uuid_to_string(uuids + i, uuid_str, 40);
            PRINT("\tuuid[%-2d]: %s", i, uuid_str);
        }
    }
}

const static adapter_callbacks_t g_adapter_async_cbs = {
    .on_adapter_state_changed = on_adapter_state_changed_cb,
    .on_discovery_state_changed = on_discovery_state_changed_cb,
    .on_discovery_result = on_discovery_result_cb,
    .on_scan_mode_changed = on_scan_mode_changed_cb,
    .on_device_name_changed = on_device_name_changed_cb,
    .on_pair_request = on_pair_request_cb,
    .on_pair_display = on_pair_display_cb,
    .on_connect_request = on_connect_request_cb,
    .on_connection_state_changed = on_connection_state_changed_cb,
    .on_bond_state_changed = on_bond_state_changed_cb,
    .on_le_sc_local_oob_data_got = on_le_sc_local_oob_data_got_cb,
    .on_remote_name_changed = on_remote_name_changed_cb,
    .on_remote_alias_changed = on_remote_alias_changed_cb,
    .on_remote_cod_changed = on_remote_cod_changed_cb,
    .on_remote_uuids_changed = on_remote_uuids_changed_cb,
};

static void register_callback_cb(bt_instance_t* ins, bt_status_t status, void* cookie, void* userdata)
{
    *(void**)userdata = cookie;
}

static void state_on_cb(bt_instance_t* ins, bt_status_t status, bt_adapter_state_t state, void* userdata)
{
    PRINT("%s state: %d", __func__, state);

    if (state == BT_ADAPTER_STATE_ON)
        bt_tool_init(g_bttool_ins);
}

static void ipc_connected(bt_instance_t* ins, void* userdata)
{
    PRINT("ipc connected");

    bt_adapter_register_callback_async(ins, &g_adapter_async_cbs, register_callback_cb, &adapter_callback_async);
    bt_adapter_get_state_async(ins, state_on_cb, NULL);
}

static void ipc_disconnected(bt_instance_t* ins, void* userdata, int status)
{
    PRINT("ipc disconnected");
}

int bttool_async_ins_init(bttool_t* bttool)
{
    g_bttool_ins = bluetooth_create_async_instance(&bttool->loop, ipc_connected, ipc_disconnected, (void*)bttool);
    if (g_bttool_ins == NULL) {
        PRINT("create instance error\n");
        return -1;
    }

    return 0;
}

void bttool_async_ins_uninit(bttool_t* bttool)
{
    bt_tool_uninit(g_bttool_ins);
    bt_adapter_unregister_callback_async(g_bttool_ins, adapter_callback_async, NULL, NULL);
    bluetooth_delete_async_instance(g_bttool_ins);
    g_bttool_ins = NULL;
    adapter_callback_async = NULL;
}