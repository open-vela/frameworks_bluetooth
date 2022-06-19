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
static int set_inquiry_scan_parameters(void* handle, int argc, char** argv);
static int set_page_scan_parameters(void* handle, int argc, char** argv);
static int get_local_address(void* handle, int argc, char** argv);
static int set_local_io_capability(void* handle, int argc, char** argv);
static int get_local_name(void* handle, int argc, char** argv);
static int set_local_name(void* handle, int argc, char** argv);
static int set_afh_channel_classification(void* handle, int argc, char** argv);

static int get_remote_name(void* handle, int argc, char** argv);
static int reply_pair_request(void* handle, int argc, char** argv);
static int ssp_reply(void* handle, int argc, char** argv);

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
static int set_auto_accept_pair(void* handle, int argc, char** argv);
static int bt_reset_btinfo(void* handle, int argc, char** argv);
static int ble_set_public_address(void* handle, int argc, char** argv);
static int add_whitelist_device(void* handle, int argc, char** argv);
static int add_resolving_device(void* handle, int argc, char** argv);
static int remove_whitelist_device(void* handle, int argc, char** argv);
static int remove_resolving_device(void* handle, int argc, char** argv);
static int get_ble_bonded_devices(void* handle, int argc, char** argv);
static int get_ble_connected_devices(void* handle, int argc, char** argv);
static int get_ble_whitelist_devices(void* handle, int argc, char** argv);
static int get_ble_resolvinglist_devices(void* handle, int argc, char** argv);
static int ble_create_encrypted_connect(void* handle, int argc, char** argv);

static btm_gap_interface_t* gap_test_interface = NULL;
static btm_interface_t* manager;
static void* manager_handle = NULL;
static void* g_gap_handle = NULL;
static uint8_t daemon_enable = 0;
static uint16_t auto_accept = 0;
static bool btinfo_reset = true;

static struct option main_options[] = {
    { "help", 0, 0, 'h' },
    { "version", 0, 0, 'v' },
    { "daemon", 0, 0, 'd' },
    { "set", 0, 0, 's' },
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
#ifdef CONFIG_BLUETOOTH_PAN
    { "pan", pan_command, "<PAN> Personal Area Networking Profile" },
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    { "hfp", hfp_client_command, "<HFP> HandFree Profile --Client" },
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    { "a2dpsrc", a2dp_source_command, "<A2DP> Advanced Audio Distribution Profile --Source" },
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    { "a2dpsnk", a2dp_sink_command, "<A2DP> Advanced Audio Distribution Profile --Sink" },
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_TG
    { "avrcp", avrcp_target_command, "<AVRCP> AVRCP --Target" },
#endif
    { "gap", gap_cmd, "<GAP> General profile" },
#ifdef CONFIG_BLUETOOTH_GATT_SERVER
    { "gatts", gatt_server_command, "<GATT> gatt server and le advertise" },
#endif
#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
    { "gattc", gatt_client_command, "<GATT> gatt server and le scan" },
#endif
#if defined(CONFIG_BLUETOOTH_HIDDEV)
    { "hidd", hid_device_command, "hid device profile" },
#endif
    { "log", log_command, "log control" },
    { "help", usage_cmd, "Usage for bttools" },
    //{ "quit", quit_cmd, "Quit" },
};

static int le_start_advertising2(void* handle, int argc, char** argv)
{
    if (argc < 5) {
        return -1;
    }
    int adv_type = atoi(argv[0]);
    if (adv_type < BLE_EVENT_ADV_IND || adv_type > BLE_EVENT_SCAN_RSP) {
        BT_LOGE("invalid adv_type:%d", adv_type);
        return 0;
    }

    int interval = atoi(argv[1]);
    int duration = atoi(argv[2]);
    int filter_type = atoi(argv[3]);
    uint8_t adv_id = atoi(argv[4]);
    BT_LOGD("start ble adv type:%d, interval:%d, duration:%d, filter_type:%d, adv_id:%u", adv_type, interval, duration, filter_type, adv_id);
    uint8_t s_adv_data[] = { 0x02, 0x01, 0x08, 0x03, 0x02, 0x00, 0xFF };
    uint8_t s_rsp_data[] = { 0x09, 0x09, 0x42, 0x52, 0x54, 0x2D, 0x49, 0x44, 0x4D, 0x30 };
    s_rsp_data[9] = 0x30 + adv_id;

    advertise_param_t adv_para;
    memset(&adv_para, 0, sizeof(advertise_param_t));
    adv_para.params.adv_type = adv_type;
    adv_para.params.channel_map = BLE_ADV_CHANNEL_DEFAULT;
    adv_para.params.interval = interval;
    adv_para.params.tx_power = -10;
    adv_para.params.own_addr_type = BLE_ADDR_TYPE_UNKNOWN;
    adv_para.duration = duration;
    adv_para.adv_length = sizeof(s_adv_data);
    adv_para.adv_data = (char*)s_adv_data;
    adv_para.scan_rsp_data = (char*)s_rsp_data;
    adv_para.scan_rsp_length = sizeof(s_rsp_data);
    adv_para.params.filter_policy = filter_type;
    bt_result_code ret = gap_test_interface->ble_start_advertising(g_gap_handle, &adv_para);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("start_advertising  fail, ret: %d", ret);
        return 0;
    }
    return 0;
}

static int le_start_advertising(void* handle, int argc, char** argv)
{
    if (argc < 5) {
        return -1;
    }
    int adv_type = atoi(argv[0]);
    if (adv_type < BLE_EVENT_ADV_IND || adv_type > BLE_EVENT_EXT_SCAN_RSP) {
        BT_LOGE("invalid adv_type:%d", adv_type);
        return 0;
    }

    int interval = atoi(argv[1]);
    int duration = atoi(argv[2]);
    int filter_type = atoi(argv[3]);
    uint8_t adv_id = atoi(argv[4]);
    BT_LOGD("start ble adv type:%d, interval:%d, duration:%d, filter_type:%d, adv_id:%u", adv_type, interval, duration, filter_type, adv_id);
    uint8_t s_adv_data[] = { 0x02, 0x01, 0x08, 0x09, 0x09, 0x42, 0x52, 0x54, 0x2D, 0x49, 0x44, 0x4D, 0x30, 0x03, 0x02, 0x00, 0xFF };
    s_adv_data[12] = 0x30 + adv_id;

    advertise_param_t adv_para;
    memset(&adv_para, 0, sizeof(advertise_param_t));
    adv_para.params.adv_type = adv_type;
    adv_para.params.channel_map = BLE_ADV_CHANNEL_DEFAULT;
    adv_para.params.interval = interval;
    adv_para.params.tx_power = -10;
    adv_para.params.own_addr_type = BLE_ADDR_TYPE_UNKNOWN;
    adv_para.duration = duration;
    adv_para.adv_length = sizeof(s_adv_data);
    adv_para.adv_data = (char*)s_adv_data;
    adv_para.scan_rsp_data = (char*)s_adv_data;
    adv_para.scan_rsp_length = sizeof(s_adv_data);
    adv_para.params.filter_policy = filter_type;
    bt_result_code ret = gap_test_interface->ble_start_advertising(g_gap_handle, &adv_para);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("start_advertising  fail, ret: %d", ret);
        return 0;
    }
    return 0;
}

static int le_stop_advertising(void* handle, int argc, char** argv)
{
    if (argc < 1) {
        return -1;
    }

    int adv_id = atoi(argv[0]);
    BT_LOGD("stop ble adv, adv_id:%d", adv_id);

    bt_result_code ret = gap_test_interface->ble_stop_advertising(g_gap_handle, adv_id);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGD("start_advertising  fail, ret: %d", ret);
        return 0;
    }
    return 0;
}

static bt_command_t g_gap_tables[] = {
    { "scan_mode", set_scan_mode, "\"set scan mode                                    param: <mode>  <bondable> \"" },
    { "inquiryparas", set_inquiry_scan_parameters, "\"set inquiry parameters                 param: <type>  <scan_interval>  <scan_window> \"" },
    { "pageparas", set_page_scan_parameters, "\"set page parameters                 param: <type>  <scan_interval>  <scan_window> \"" },
    { "discovery", start_discovery, "\"start bluetooth discovery             param: <timer(n*1.28s)> \"" },
    { "stopdiscovery", stop_discovery, "\"stop bluetooth discovery \"" },
    { "getaddr", get_local_address, "\"get local address      \"" },
    { "setIO", set_local_io_capability, "\"set local capaliblity                        param: <iocapability> \"" },
    { "getname", get_local_name, "\"get local name  \"" },
    { "setname", set_local_name, "\"change local name \"" },
    { "remotename", get_remote_name, "\"get remote name                            param: <addr> \"" },
    { "replypair", reply_pair_request, "\"replay pair request                         param: <addr> <accept>\"" },
    { "createbond", create_bond, "\"create bond device                         param: <addr> \"" },
    { "cancelbond", cancel_bond, "\"cancel create bond                          param: <addr> \"" },
    { "removebond", remove_bond, "\"remove bond device                       param: <addr> \"" },
    { "getbonded", get_bonded_devices, "\"get bonded device  \"" },
    { "getconnected", get_connected_devices, "\"get connected device   \"" },
    { "servicediscovery", start_service_discovery, "\"start service discovery                   param: <addr> <uuid>\"" },
    { "stopservicediscovery", stop_service_discovery, "\"stop service discovery                  param: <addr> \"" },
    { "getremoteservice", get_remote_services, "\"get remote service                           param: <addr> \"" },
    { "setclass", set_local_device_class, "\"set local class                                    param: <class> \"" },
    { "getclass", get_local_device_class, "\"get local class \"" },
    { "setblepubaddr", ble_set_public_address, "\"set ble  publica ddress     param: <addr> \"" },
    { "setbleaddr", ble_set_address, "\"set ble address                                  param: <addr> \"" },
    { "autoaccept", set_auto_accept_pair, "\"auto accept pair                               param: <accept:0 auto accpet, 1 not auto accept> \"" },
    { "addwhitelist", add_whitelist_device, "\"add whitle list device                    param: <addr> \"" },
    { "removewhitelist", remove_whitelist_device, "\"remove whitle list device            param: <addr> \"" },
    { "addresolvinglist", add_resolving_device, "\"add resovling list device               param: <addr> \"" },
    { "removesolvinglist", remove_resolving_device, "\"remove resovling list device      param: <addr> \"" },
    { "sspreply", ssp_reply, "\"reply remote relpyrequest      param: <addr> <keycode> \"" },
    { "getblebonded", get_ble_bonded_devices, "\"get ble bonded device  \"" },
    { "getbleconnected", get_ble_connected_devices, "\"get ble connected device  \"" },
    { "getwhitelist", get_ble_whitelist_devices, "\"get ble whitelist device  \"" },
    { "getresolvinglist", get_ble_resolvinglist_devices, "\"get ble resolvinglist device  \"" },
    { "btinforeset", bt_reset_btinfo, "\"bluetooth device info reset                              param: <0 use last device info, 1 use default device info> \"" },
    { "bleenccon", ble_create_encrypted_connect, "\"ble create encrypted connect      param: <addr> \"" },
    { "start_adv", le_start_advertising, "\"start le adv: <type (0:ADV_IND, 1:DIRECT_IND, 2:SCAN_IND, 3:NONCONN_IND, 4:SCAN_RSP)> <interval> <duration> <filter_type> <adv_id (0:legacy, 1~K: extend)>\"" },
    { "start_adv2", le_start_advertising2, "\"start le adv: <type (0:ADV_IND, 1:DIRECT_IND, 2:SCAN_IND, 3:NONCONN_IND, 4:SCAN_RSP)> <interval> <duration> <filter_type> <adv_id (0:legacy, 1~K: extend)>\"" },
    { "stop_adv", le_stop_advertising, "\"stop le adv <adv_id>\"" },
    { "setafh", set_afh_channel_classification, "\"bt set afh channel  : <freq_channal (0~13)> <band_width(20/22/40Mbit)\"" },
};

static struct option gap_options[] = {
    { "help", 0, 0, 'h' },
    { 0, 0, 0, 0 }
};

static void bttool_command_init(void)
{
#ifdef CONFIG_BLUETOOTH_SPP
    spp_command_init();
#endif
#ifdef CONFIG_BLUETOOTH_PAN
    pan_command_init();
#endif
}

static void bttool_command_uninit(void)
{
#ifdef CONFIG_BLUETOOTH_SPP
    spp_command_uninit();
#endif
#ifdef CONFIG_BLUETOOTH_PAN
    pan_command_uninit();
#endif
}

static int start_discovery(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    uint16_t timerout = atoi(argv[0]);
    gap_test_interface->bt_start_discovery(g_gap_handle, timerout);

    return 0;
}

static int stop_discovery(void* handle, int argc, char** argv)
{
    gap_test_interface->bt_stop_discovery(g_gap_handle);

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

    gap_test_interface->bt_set_scan_mode(g_gap_handle, scanMode, bondable);

    return 0;
}

static int set_inquiry_scan_parameters(void* handle, int argc, char** argv)
{
    if (argc < 3)
        return -1;

    bt_scan_type scan_type = BR_SCAN_TYPE_STANDARD;
    uint16_t type = atoi(argv[0]);
    uint16_t scan_interval = atoi(argv[1]);
    uint16_t scan_window = atoi(argv[2]);
    switch (type) {
    case 0:
        scan_type = BR_SCAN_TYPE_STANDARD;
        break;
    case 1:
        scan_type = BR_SCAN_TYPE_INTERLACED;
        break;
    default:
        break;
    }
    BT_LOGD("%s, scan_type:%d, scan_interval:%d, scan_window:%d", __func__, scan_type, scan_interval, scan_window);

    gap_test_interface->bt_set_inquiry_scan_parameters(g_gap_handle, scan_type, scan_interval, scan_window);
    return 0;
}

static int set_page_scan_parameters(void* handle, int argc, char** argv)
{
    if (argc < 3)
        return -1;

    bt_scan_type scan_type = BR_SCAN_TYPE_STANDARD;
    uint16_t type = atoi(argv[0]);
    uint16_t scan_interval = atoi(argv[1]);
    uint16_t scan_window = atoi(argv[2]);
    switch (type) {
    case 0:
        scan_type = BR_SCAN_TYPE_STANDARD;
        break;
    case 1:
        scan_type = BR_SCAN_TYPE_INTERLACED;
        break;
    default:
        break;
    }
    BT_LOGD("%s, scan_type:%d, scan_interval:%d, scan_window:%d", __func__, scan_type, scan_interval, scan_window);

    gap_test_interface->bt_set_page_scan_parameters(g_gap_handle, scan_type, scan_interval, scan_window);
    return 0;
}

static int set_afh_channel_classification(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return -1;

    uint16_t ch = atoi(argv[0]);
    uint16_t bondwidth= atoi(argv[1]);

    if (ch > 13 || ch <= 0) {
        return -1;
    }

    if (bondwidth != 20 && bondwidth != 22 && bondwidth != 40) {
        return -1;
    }

    bt_afh_radio_channel_info_t channel = {
        .central_frequency = AFH_WIFI_CHANNEL_TO_FREQ(ch),
        .band_width = bondwidth,
    };

    BT_LOGD("%s, central_frequency:%d, bandwidth:%d", __func__, (int)channel.central_frequency, (int)channel.band_width);
    gap_test_interface->bt_set_afh_channel_classification(g_gap_handle, &channel, 1);
    return 0;
}

static int get_local_address(void* handle, int argc, char** argv)
{
    bt_address addr;
    gap_test_interface->bt_get_local_address(g_gap_handle, addr);
    BT_LOGD("%s, bt_address :%s", __func__, addr_str(addr));
    return 0;
}

static int set_local_io_capability(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    uint16_t iocap = atoi(argv[0]);
    gap_test_interface->bt_set_local_io_capability(g_gap_handle, iocap);

    return 0;
}

static int get_local_name(void* handle, int argc, char** argv)
{

    char* name = gap_test_interface->bt_get_local_name(g_gap_handle);
    BT_LOGD("%s, name: %s", __func__, name);

    return 0;
}

static int set_local_name(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    int len = strlen(argv[0]) + 1;
    char* name = malloc(len);
    if (!name) {
        BT_LOGE("error, name malloc failed");
        return 0;
    }
    memcpy(name, argv[0], len);
    gap_test_interface->bt_set_local_name(g_gap_handle, argv[0], len);
    free(name);

    return 0;
}

static int get_remote_name(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));
    str2ba(argv[0], device->addr);

    gap_test_interface->bt_get_remote_name(g_gap_handle, device);
    free(device);

    return 0;
}

static int ble_set_public_address(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->ble_set_public_identity(g_gap_handle, device);
    free(device);
    return 0;
}

static int reply_pair_request(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    uint16_t accept = atoi(argv[0]);
    gap_test_interface->bt_reply_pair_request(g_gap_handle, device, accept);
    free(device);
    return 0;
}

static int ssp_reply(void* handle, int argc, char** argv)
{
    if (argc < 2)
        return -1;

    spp_reply_data_t reply;
    str2ba(argv[0], reply.remote_addr);
    reply.accept = 1;
    reply.type = SPP_TYPE_PASSKEY_ENTRY;
    reply.passkey = atoi((const char*)argv[1]);
    gap_test_interface->bt_ssp_reply(g_gap_handle, &reply);
    return 0;
}

static int create_bond(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->bt_create_bond(g_gap_handle, device);
    free(device);
    return 0;
}

static int cancel_bond(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->bt_cancel_bond(g_gap_handle, device);
    free(device);

    return 0;
}

static int remove_bond(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));
    str2ba(argv[0], device->addr);
    gap_test_interface->bt_remove_bond(g_gap_handle, device);
    free(device);

    return 0;
}

static int get_bonded_devices(void* handle, int argc, char** argv)
{
    int num = gap_test_interface->bt_get_bonded_devices(g_gap_handle, NULL, 0);
    if (num < 1) {
        BT_LOGD("%s, num : %d", __func__, num);
        return 0;
    }
    bt_device_t* device_list = malloc(sizeof(bt_device_t) * num);
    if (!device_list) {
        BT_LOGE("error, device_list malloc failed");
        return 0;
    }

    num = gap_test_interface->bt_get_bonded_devices(g_gap_handle, device_list, num);
    for (int i = 0; i < num; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s, name :%s, code %" PRIu32, __func__, i, addr_str(device->addr), device->name, device->cod);
    }
    free(device_list);
    return 0;
}

static int get_connected_devices(void* handle, int argc, char** argv)
{

    int num = gap_test_interface->bt_get_connected_devices(g_gap_handle, NULL, 0);
    if (num < 1) {
        BT_LOGD("%s, num : %d", __func__, num);
        return 0;
    }
    bt_device_t* device_list = malloc(sizeof(bt_device_t) * num);
    if (!device_list) {
        BT_LOGE("error, device_list malloc failed");
        return 0;
    }

    num = gap_test_interface->bt_get_connected_devices(g_gap_handle, device_list, num);

    for (int i = 0; i < num; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s, name: %s", __func__, i, addr_str(device->addr), device->name);
    }
    free(device_list);
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
    uint32_t class = atoi(argv[0]);
    gap_test_interface->bt_set_local_device_class(g_gap_handle, class);

    return 0;
}

static int get_local_device_class(void* handle, int argc, char** argv)
{
    uint32_t class = gap_test_interface->bt_get_local_device_class(g_gap_handle);
    BT_LOGD("%s, class: %" PRIu32, __func__, class);

    return 0;
}

static int ble_set_address(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));

    str2ba(argv[0], device->addr);
    gap_test_interface->ble_set_address(g_gap_handle, device);
    free(device);
    return 0;
}

static int set_auto_accept_pair(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    auto_accept = atoi(argv[0]);
    return 0;
}

static int bt_reset_btinfo(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;
    bool reset = true;
    int choose = atoi(argv[0]);
    switch (choose) {
    case 0:
        reset = false;
        break;
    default:
        break;
    }
    btinfo_reset = reset;
    BT_LOGD("%s, btinfo_reset:%d", __func__, btinfo_reset);
    return 0;
}

static int add_whitelist_device(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));

    str2ba(argv[0], device->addr);
    gap_test_interface->ble_add_white_list(g_gap_handle, device);
    free(device);
    return 0;
}

static int add_resolving_device(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));

    str2ba(argv[0], device->addr);
    gap_test_interface->ble_add_resolving_list(g_gap_handle, device);
    free(device);
    return 0;
}
static int remove_whitelist_device(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));

    str2ba(argv[0], device->addr);
    gap_test_interface->ble_remove_white_list(g_gap_handle, device);
    free(device);
    return 0;
}

static int remove_resolving_device(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }
    memset(device, 0, sizeof(bt_device_t));

    str2ba(argv[0], device->addr);
    gap_test_interface->ble_remove_resolving_list(g_gap_handle, device);
    free(device);
    return 0;
}
static int get_ble_bonded_devices(void* handle, int argc, char** argv)
{
    int num = gap_test_interface->ble_get_bonded_devices(g_gap_handle, NULL, 0);
    if (num < 1) {
        BT_LOGD("%s, num : %d", __func__, num);
        return 0;
    }
    bt_device_t* device_list = malloc(sizeof(bt_device_t) * num);
    if (!device_list) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }

    num = gap_test_interface->ble_get_bonded_devices(g_gap_handle, device_list, num);
    BT_LOGD("%s, num : %d", __func__, num);
    for (int i = 0; i < num; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s", __func__, i, addr_str(device->addr));
    }
    free(device_list);
    return 0;
}

static int get_ble_connected_devices(void* handle, int argc, char** argv)
{
    int num = gap_test_interface->ble_get_connected_devices(g_gap_handle, NULL, 0);
    if (num < 1) {
        BT_LOGD("%s, num : %d", __func__, num);
        return 0;
    }
    bt_device_t* device_list = malloc(sizeof(bt_device_t) * num);
    if (!device_list) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }

    num = gap_test_interface->ble_get_connected_devices(g_gap_handle, device_list, num);
    BT_LOGD("%s, num : %d", __func__, num);

    for (int i = 0; i < num; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s", __func__, i, addr_str(device->addr));
    }
    free(device_list);
    return 0;
}

static int get_ble_whitelist_devices(void* handle, int argc, char** argv)
{
    int num = gap_test_interface->ble_get_whitelist_devices(g_gap_handle, NULL, 0);
    if (num < 1) {
        BT_LOGD("%s, num : %d", __func__, num);
        return 0;
    }

    bt_device_t* device_list = malloc(sizeof(bt_device_t) * num);
    if (!device_list) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }

    num = gap_test_interface->ble_get_whitelist_devices(g_gap_handle, device_list, num);
    BT_LOGD("%s, num : %d", __func__, num);
    for (int i = 0; i < num; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s", __func__, i, addr_str(device->addr));
    }
    free(device_list);
    return 0;
}

static int get_ble_resolvinglist_devices(void* handle, int argc, char** argv)
{
    int num = gap_test_interface->ble_get_resolvinglist_devices(g_gap_handle, NULL, 0);
    if (num < 1) {
        BT_LOGD("%s, num : %d", __func__, num);
        return 0;
    }
    bt_device_t* device_list = malloc(sizeof(bt_device_t) * num);
    if (!device_list) {
        BT_LOGE("error, device malloc failed");
        return 0;
    }

    num = gap_test_interface->ble_get_resolvinglist_devices(g_gap_handle, device_list, num);
    BT_LOGD("%s, num : %d", __func__, num);

    for (int i = 0; i < num; i++) {
        bt_device_t* device = &device_list[i];
        BT_LOGD("%s, device [%d]: %s", __func__, i, addr_str(device->addr));
    }
    free(device_list);
    return 0;
}

static int ble_create_encrypted_connect(void* handle, int argc, char** argv)
{
    if (argc < 1)
        return -1;

    ble_connect_params_t conn_param;
    memset(&conn_param, 0, sizeof(ble_connect_params_t));

    conn_param.filter_policy = BLE_CONNECT_FILTER_ADDR;
    str2ba(argv[0], conn_param.peer_addr);
    conn_param.peer_addr_type = BLE_ADDR_ANONYMOUS;
    conn_param.use_default_params = true;

    BT_LOGD("%s, addr:%s", __func__, addr_str(conn_param.peer_addr));

    gap_test_interface->ble_connect(g_gap_handle, &conn_param);

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
    }
    if (!gap_test_interface)
        return 0;
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

static void display_services(uint8_t* services, uint8_t count)
{
    bt_uuid_t all_0;
    uint8_t* current = services;
    uint8_t* end = current + count * 16;

    memset(all_0, 0, 16);
    while (current < end) {
        if (!memcmp(all_0, current, 16)) {
            break;
        }
        if (services == current) {
            BT_LOGD("[uuid_list]");
        }
        BT_LOGD("[0x%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x]",
            current[15], current[14], current[13], current[12], current[11], current[10], current[9],
            current[8],
            current[7], current[6], current[5], current[4], current[3], current[2], current[1], current[0]);
        current += 16;
    }
    if (services != current) {
        BT_LOGD("[uuid_list-end]");
    }
}

static void manager_state_changed_callback(btm_bt_state state)
{
    BT_LOGD("%s, state:%d", __func__, state);
    if (state == BTM_STATE_ON) {
        if (daemon_enable) {
            char local_name[] = "BLUELET_NUTTX_Sim";
            gap_test_interface->bt_set_local_name(g_gap_handle, local_name, sizeof(local_name));
            gap_test_interface->bt_set_local_device_class(g_gap_handle, COD_SERVICE_RENDERING | COD_SERVICE_AUDIO | COD_SERVICE_TELEPHONY | COD_AV_HEADSET);
            gap_test_interface->bt_set_local_io_capability(g_gap_handle, SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT);
        }
        if (!btinfo_reset) {
            BT_LOGD("%s, btinfo_reset:%d", __func__, btinfo_reset);
            return;
        }
        gap_test_interface->bt_set_local_device_class(g_gap_handle, COD_SERVICE_RENDERING | COD_SERVICE_AUDIO | COD_SERVICE_TELEPHONY | COD_AV_HEADSET | COD_PERIPHERAL_KEYORPOINT);
        gap_test_interface->bt_set_scan_mode(g_gap_handle, SCAN_MODE_CONNECTABLE_DISCOVERABLE, true);
        gap_test_interface->bt_set_local_io_capability(g_gap_handle, SERVICE_BT_IO_CAPABILITY_NOINPUTNOOUTPUT);
    }
}

static void test_discovery_state_changed_callback(void* handle, bt_discovery_state state)
{
    BT_LOGD("%s, state %d", __func__, state);
}

static void test_device_found_callback(void* handle, bt_device_t* device)
{
    BT_LOGD("%s, device name : %s, device %s, device class : %" PRIu32", rssi: %d ", __func__, device->name, addr_str(device->addr), device->cod, device->rssi);
    display_services((uint8_t*)(device->uuids), MAX_UUID_NUM);
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
    BT_LOGD("%s, : request_data->ssp_type: %d, name : %s", __func__, request_data->ssp_type, request_data->bt_name);
    spp_reply_data_t reply;

    if (request_data->ssp_type == SPP_TYPE_PASSKEY_CONFIRMATION) {
        memcpy(reply.remote_addr, request_data->remote_addr, 6);
        reply.accept = true;
        reply.type = SPP_TYPE_PASSKEY_CONFIRMATION;
        gap_test_interface->bt_ssp_reply(g_gap_handle, &reply);
    }
    if (request_data->ssp_type == SPP_TYPE_PASSKEY_ENTRY) {
        BT_LOGD("please input code with sspreply command!");
    }
    if (request_data->ssp_type == SPP_TYPE_PASSKEY_NOTIFICATION) {
        BT_LOGD("%s, device %s, psss key: %" PRIu32, __func__, addr_str(request_data->remote_addr), request_data->cod);
        BT_LOGD("please input with sspreply command!");
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
    BT_LOGD("%s,  device_class is %" PRIu32 "\r\n", __func__, device_class);
}

void test_smp_request_callback(void* handle, ssp_request_data_t* request_data)
{
    BT_LOGD("%s, addr:%s", __func__, addr_str(request_data->remote_addr));
    switch (request_data->ssp_type) {
    case SPP_TYPE_PASSKEY_CONFIRMATION: {
        BT_LOGD(" SMP User confirmation request:  %" PRIu32, request_data->pass_key);
        spp_reply_data_t reply;
        memcpy(reply.remote_addr, request_data->remote_addr, 6);
        reply.accept = TRUE;
        reply.type = SPP_TYPE_PASSKEY_CONFIRMATION;
        gap_test_interface->ble_smp_reply(g_gap_handle, &reply);
        break;
    }
    case SPP_TYPE_PASSKEY_ENTRY:
        BT_LOGD(" SMP User passkey entry request");
        break;
    case SPP_TYPE_CONSENT:
        break;
    case SPP_TYPE_PASSKEY_NOTIFICATION:
        BT_LOGD(" SMP User passkey entry for remote: %" PRIu32, request_data->pass_key);
        break;
    }
}

void test_pairing_request_callback(void* handle, bt_address remote_addr, bool local_initiate, bool is_bondable)
{
    char* buffer = malloc(CONFIG_NSH_LINELEN);
    if (!buffer) {
        BT_LOGE("error, buffer malloc failed");
        return;
    }
    memset(buffer, 0, CONFIG_NSH_LINELEN);

    BT_LOGD("%s,local_initiate: %d, is_bondable:%d", __func__, local_initiate, is_bondable);
    bt_device_t* device = malloc(sizeof(bt_device_t));
    if (!device) {
        free(buffer);
        BT_LOGE("error, device malloc failed");
        return;
    }
    memset(device, 0, sizeof(bt_device_t));

    memcpy(device->addr, remote_addr, BD_ADDR_SIZE);
    if ((daemon_enable) || (!auto_accept && (local_initiate || is_bondable))) {
        gap_test_interface->bt_reply_pair_request(g_gap_handle, device, 0);
        goto exit;
    }
    while (1) {
        BT_LOGD("auto accept not open, please input y or n -----------------");
        int len = readline(buffer, CONFIG_NSH_LINELEN, stdin, stdout);
        buffer[len] = '\0';
        if (len < 0)
            goto exit;
        if (buffer[0] == 'y') {
            gap_test_interface->bt_reply_pair_request(g_gap_handle, device, 0);
            goto exit;
        } else if (buffer[0] == 'n') {
            gap_test_interface->bt_reply_pair_request(g_gap_handle, device, 1);
            goto exit;
        }
    }
exit:
    free(device);
    free(buffer);
    return;
}

void test_delete_linkey_callback(void* gap_handle, bt_address remote_addr, bt_status reason)
{
    BT_LOGD("%s, addr:%s, reason:%" PRIu32, __func__, addr_str(remote_addr), reason);
}

static void le_adv_started_callback(void* gap_handle, uint8_t adv_id)
{
    BT_LOGD("%s, adv_id:%d", __func__, adv_id);
}

static void le_adv_stopped_callback(void* gap_handle, uint8_t adv_id)
{
    BT_LOGD("%s, adv_id:%d", __func__, adv_id);
}

btm_gap_callbacks_t gap_test_tool_callbacks = {
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
    .delete_linkey_cb = test_delete_linkey_callback,
    .ble_adv_started_cb = le_adv_started_callback,
    .ble_adv_stopped_cb = le_adv_stopped_callback,
};

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
};

static int enable_cmd(void* handle, int argc, char** argv)
{
    manager->enable(handle);
    bttool_command_init();

    return 0;
}

static int disable_cmd(void* handle, int argc, char** argv)
{
    bttool_command_uninit();
    manager->disable(handle);

    return 0;
}

static int get_state_cmd(void* handle, int argc, char** argv)
{
    btm_bt_state state = manager->bt_get_state(handle);
    BT_LOGD("%s, state: %d", __func__, state);
    return 0;
}

static int get_ble_state_cmd(void* handle, int argc, char** argv)
{
    btm_ble_state state = manager->ble_get_state(handle);
    BT_LOGD("%s, state: %d", __func__, state);
    return 0;
}
static int usage_cmd(void* handle, int argc, char** argv)
{
    if (argc == 2 && !strcmp(argv[1], "me!!!"))
        return -2;

    usage();

    return 0;
}

static int quit_cmd(void* handle, int argc, char** argv)
{
    manager->cleanup(handle);
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
    int ret;

    for (int i = 0; i < ARRAY_SIZE(g_cmd_tables); i++) {
        if (strcmp(g_cmd_tables[i].cmd, argv[0]) == 0) {
            if (g_cmd_tables[i].func) {
                ret = g_cmd_tables[i].func(handle, argc, &argv[0]);
                if (g_cmd_tables[i].func == quit_cmd)
                    return -2;
                return ret;
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

    while ((opt = getopt_long(argc, argv, "h-v-d-s", main_options, NULL)) != -1) {
        switch (opt) {
        case 'h':
            usage();
            exit(0);
        case 'v':
            show_version();
            exit(0);
        case 'd':
            daemon_enable = 1;
            break;
        case 's':
            exit(0);
        default:
            break;
        }
    }

    //btm_manager init
    manager = get_bt_manager_interface();
    manager->init(&manager_handle, &mgt_cb);
    if (daemon_enable) {
        manager->enable(manager_handle);
        bttool_command_init();
    }
    gap_test_interface = get_gap_instance();
    gap_test_interface->gap_register_callbacks(manager_handle, &g_gap_handle, &gap_test_tool_callbacks);

    buffer = malloc(CONFIG_NSH_LINELEN);
    if (!buffer)
        return -ENOMEM;

    while (1) {
        if (daemon_enable) {
            sleep(10000);
            continue;
        }
        printf("bttool> ");
        fflush(stdout);

        memset(_argv, 0, sizeof(_argv));
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
            _argc = 0;
            if (ret == -2)
                break;
        }
    }

    free(buffer);
    //exit(1);
    return 0;
}
