/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#include <stdlib.h>
#include <string.h>

#include "advertiser_data.h"
#include "bt_le_scan.h"
#include "bt_tools.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "[bttool_async]"

static int start_scan_cmd(void* handle, int argc, char* argv[]);
static int stop_scan_cmd(void* handle, int argc, char* argv[]);
static int dump_scan_cmd(void* handle, int argc, char* argv[]);

static bt_scanner_t* g_scanner = NULL;

static struct option scan_options[] = {
    { "type", required_argument, 0, 't' },
    { "phy", required_argument, 0, 'p' },
    { "mode", required_argument, 0, 'm' },
    { "legacy", required_argument, 0, 'l' },
    { "filter", required_argument, 0, 'f' },
    { 0, 0, 0, 0 }
};

static bt_command_t g_scanner_async_tables[] = {
    { "start", start_scan_cmd, 0, "start scan\n"
                                  "\t  -t or --type, le scan type (0: passive, 1: active)\n"
                                  "\t  -p or --phy, le scan phy (1M/2M/Coded)\n"
                                  "\t  -m or --mode, scan mode (0:low power mode, 1:balance mode, 2:low latency mode)\n"
                                  "\t  -l or --legacy, is legacy scan (1: true, 0: false)\n"
                                  "\t  -f or --filter, filter advertiser :<uuid>\n" },
    { "stop", stop_scan_cmd, 0, "stop scan" },
    { "dump", dump_scan_cmd, 0, "dump scan state" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_scanner_async_tables); i++) {
        printf("\t%-4s\t%s\n", g_scanner_async_tables[i].cmd, g_scanner_async_tables[i].help);
    }
}

static void stop_scan_callback_cb(bt_instance_t* ins, void* userdata)
{
    if (g_scanner != userdata)
        return;

    g_scanner = NULL;
}

static void start_scan_callback_cb(bt_instance_t* ins, bt_status_t status, void* scan, void* userdata)
{
    if (!g_scanner) {
        g_scanner = scan;
        return;
    }

    PRINT("%s, Repeated scan.", __func__);

    if (status == BT_STATUS_SUCCESS) {
        bt_le_stop_scan_async(ins, g_scanner, stop_scan_callback_cb, g_scanner);
        g_scanner = scan;
    }

    return;
}

static void on_scan_result_cb(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    PRINT_ADDR("ScanResult ------[%s]------", &result->addr);
    PRINT("AddrType:%d", result->addr_type);
    PRINT("Rssi:%d", result->rssi);
    PRINT("Type:%d", result->adv_type);
    advertiser_data_dump((uint8_t*)result->adv_data, result->length, NULL);
    PRINT("\n");
}

static void on_scan_start_status_cb(bt_scanner_t* scanner, uint8_t status)
{
    PRINT("%s, scanner:%p, status:%d", __func__, scanner, status);
}

static void on_scan_stopped_cb(bt_scanner_t* scanner)
{
    PRINT("%s, scanner:%p", __func__, scanner);
}

static const scanner_callbacks_t scanner_callbacks = {
    sizeof(scanner_callbacks_t),
    on_scan_result_cb,
    on_scan_start_status_cb,
    on_scan_stopped_cb
};

static int start_scan_cmd(void* handle, int argc, char* argv[])
{
    int opt;
    ble_scan_filter_t filter = {};
    ble_scan_settings_t settings = { BT_SCAN_MODE_LOW_POWER, 0, BT_LE_SCAN_TYPE_PASSIVE, BT_LE_1M_PHY, { 0 } };

    if (g_scanner)
        return CMD_ERROR;

    optind = 0;
    while ((opt = getopt_long(argc, argv, "t:p:m:l:f:", scan_options,
                NULL))
        != -1) {
        switch (opt) {
        case 't': {
            int type = atoi(optarg);
            if (type != 0 && type != 1) {
                PRINT("Invalid type:%s", optarg);
                return CMD_INVALID_OPT;
            }

            settings.scan_type = type;
        } break;
        case 'p': {
            if (strncmp(optarg, "1M", 2) == 0)
                settings.scan_phy = BT_LE_1M_PHY;
            else if (strncmp(optarg, "2M", 2) == 0)
                settings.scan_phy = BT_LE_2M_PHY;
            else if (strncmp(optarg, "Coded", 5) == 0)
                settings.scan_phy = BT_LE_CODED_PHY;
            else {
                PRINT("Invalid scan phy:%s", optarg);
                return CMD_INVALID_OPT;
            }
        } break;
        case 'm': {
            int scanmode = atoi(optarg);
            if (scanmode == 0)
                settings.scan_mode = BT_SCAN_MODE_LOW_POWER;
            else if (scanmode == 1)
                settings.scan_mode = BT_SCAN_MODE_BALANCED;
            else if (scanmode == 2)
                settings.scan_mode = BT_SCAN_MODE_LOW_LATENCY;
            else {
                PRINT("Invalid scan mode:%s", optarg);
                return CMD_INVALID_OPT;
            }
        } break;
        case 'l': {
            int legacy = atoi(optarg);
            if (legacy != 0 && legacy != 1) {
                PRINT("Invalid legacy:%s", optarg);
                return CMD_INVALID_OPT;
            }

            settings.legacy = legacy;
        } break;
        case 'f': {
            uint16_t uuid = atoi(optarg);
            PRINT("uuid: 0x%02x ", uuid);
            filter.active = true;
            filter.uuids[0] = uuid;
        } break;
        default:
            break;
        }
    }

    if (optind >= 1) {
        if (filter.active) {
            bt_le_start_scan_with_filters_async(handle, &settings, &filter, &scanner_callbacks, start_scan_callback_cb, g_scanner);
        } else
            bt_le_start_scan_settings_async(handle, &settings, &scanner_callbacks, start_scan_callback_cb, g_scanner);
    } else {
        bt_le_start_scan_async(handle, &scanner_callbacks, start_scan_callback_cb, g_scanner);
    }

    return CMD_OK;
}

static int stop_scan_cmd(void* handle, int argc, char* argv[])
{
    if (!g_scanner)
        return CMD_ERROR;

    bt_le_stop_scan_async(handle, g_scanner, stop_scan_callback_cb, g_scanner);

    return CMD_OK;
}

static int dump_scan_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

int scan_command_init_async(void* handle)
{
    g_scanner = NULL;
    return 0;
}

void scan_command_uninit_async(void* handle)
{
    g_scanner = NULL;
}

int scan_command_exec_async(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_scanner_async_tables,
            ARRAY_SIZE(g_scanner_async_tables),
            argc, argv, 0);

    if (ret < 0)
        usage();

    return ret;
}
