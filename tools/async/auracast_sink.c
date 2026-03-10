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

#include "bt_auracast_sink.h"
#include "bt_le_scan.h"
#include "bt_pa_sync.h"
#include "bt_tools.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif

#define LOG_TAG "[bttool_async]"

typedef struct {
    bt_address_t addr;
    ble_addr_type_t type;
    int8_t rssi;
    uint8_t life;
    uint8_t sid;
} bttool_auracast_pa_record_t;

typedef struct {
    bt_le_address_t addr;
    uint8_t sid;
} bttool_auracast_remote_t;

typedef struct {
    bttool_auracast_remote_t remote; /**< keep this the first member */
} bttool_auracast_sync_t;

typedef struct {
    bttool_auracast_remote_t remote; /**< keep this the first member */
    int rssi;
    bool base_parsed;
    bool auracast_ready;
} bttool_auracast_pa_sync_t;

typedef struct {
    bt_scanner_t* scanner;
    bttool_auracast_pa_record_t* nearby_pa;
    void* auracast_cbs_cookie;
    bt_list_t* sync_list; /**< bttool_auracast_pa_sync_t* */
    bt_list_t* sink_list; /**< bttool_auracast_sync_t* */
} bttool_auracast_sink_t;

static int scan_start_cmd(void* handle, int argc, char* argv[]);
static int scan_stop_cmd(void* handle, int argc, char* argv[]);
static int sync_create_cmd(void* handle, int argc, char* argv[]);
static int sync_terminate_cmd(void* handle, int argc, char* argv[]);
static int auracast_receive_cmd(void* handle, int argc, char* argv[]);
static int auracast_terminate_cmd(void* handle, int argc, char* argv[]);

static bttool_auracast_sink_t* g_auracast_sink = NULL;

static bt_command_t g_auracast_sink_tables[] = {
    { "scan", scan_start_cmd, 0, "\"search for nearby Auracast sources\"" },
    { "stopscan", scan_stop_cmd, 0, "\"stop searching\"" },
    { "sync", sync_create_cmd, 1, "\"sync to a specific periodic advertising via extended "
                                  "advertising, params:\n"
                                  "\t -a or --addr\n"
                                  "\t\t\t the address of the advertiser, e.g., 00:01:02:03:04:05\n"
                                  "\t\t\t the most recent device is selected if addr is not "
                                  "provided\n"
                                  "\t -t or --type\n"
                                  "\t\t\t the address type, 0: public, 1: random "
                                  "(public by default)\n"
                                  "\t -s or --sid\n"
                                  "\t\t\t the advertising sid (0x0-0xF) from the scan result\n"
                                  "\t -o or --timeout\n"
                                  "\t\t\t synchronization timeout for the periodic advertising "
                                  "train\n"
                                  "\t\t\t unit: 10ms\n"
                                  "\t\t\t range: 0x000a to 0x4000 (100ms to 163.84s)\n"
                                  "\t -k or --skip\n"
                                  "\t\t\t the maximum number of periodic advertising events that "
                                  "can be skipped\n"
                                  "\t\t\t range: 0x0000 to 0x01f3\n"
                                  "\t -f or --filter\n"
                                  "\t\t\t duplicate filtering enabled\n"
                                  "\t -n or --no-report\n"
                                  "\t\t\t reporting disabled\"" },
    { "termsync", sync_terminate_cmd, 1, "\"terminate a sync to a periodic advertising, params:\n"
                                         "\t -a or --addr\n"
                                         "\t\t\t the address of the advertiser, e.g., "
                                         "00:01:02:03:04:05\n"
                                         "\t\t\t mandatory if there are multiple sync exist\n"
                                         "\t -t or --type\n"
                                         "\t\t\t the address type, 0: public, 1: random "
                                         "(public by default)\n"
                                         "\t -s or --sid\n"
                                         "\t\t\t the advertising sid (0x0-0xF)\"" },
    { "recv", auracast_receive_cmd, 1, "\"sync to a specific auracast source via periodic "
                                       "advertising, params:\n"
                                       "\t -a or --addr\n"
                                       "\t\t\t the address of the advertiser, e.g., "
                                       "00:01:02:03:04:05\n"
                                       "\t\t\t mandatory if there are multiple sync exist\n"
                                       "\t -t or --type\n"
                                       "\t\t\t the address type, 0: public, 1: random "
                                       "(public by default)\n"
                                       "\t -s or --sid\n"
                                       "\t\t\t the advertising sid (0x0-0xF)\n"
                                       "\t -b or --bis\n"
                                       "\t\t\t bitwise value on which bis is selected, "
                                       "bit[x] refers to bis with index x + 1, for example:\n"
                                       "\t\t\t\t 0x00000001 - the 1st stream\n"
                                       "\t\t\t\t 0x00000003 - the 1st & 2nd streams\n"
                                       "\t -e or --encryption\n"
                                       "\t\t\t the broadcast code\"" },
    { "stoprecv", auracast_terminate_cmd, 1, "\"terminate sync to auracast source, params:\n"
                                             "\t -a or --addr\n"
                                             "\t\t\t the address of the advertiser, e.g., "
                                             "00:01:02:03:04:05\n"
                                             "\t\t\t mandatory if there are multiple sync exist\n"
                                             "\t -t or --type\n"
                                             "\t\t\t the address type, 0: public, 1: random "
                                             "(public by default)\n"
                                             "\t -s or --sid\n"
                                             "\t\t\t the advertising sid (0x0-0xF)\"" },
};

static void on_scan_result(bt_scanner_t* scanner, ble_scan_result_t* result)
{
}

static void on_scan_status(bt_scanner_t* scanner, uint8_t status)
{
}

static void on_scan_stopped(bt_scanner_t* scanner)
{
}

static const scanner_callbacks_t scanner_cbs = {
    .size = sizeof(scanner_cbs),
    .on_scan_result = on_scan_result,
    .on_scan_start_status = on_scan_status,
    .on_scan_stopped = on_scan_stopped,
};

static const ble_scan_settings_t default_scan_settings = {
    .scan_mode = BT_SCAN_MODE_LOW_LATENCY,
    .legacy = false,
    .scan_type = BT_LE_SCAN_TYPE_PASSIVE,
    .scan_phy = BT_LE_1M_PHY,
    .policy.policy = 0, /**< Unfiltered */
};

static void scan_start_cb(bt_instance_t* ins, bt_status_t status, void* scan, void* userdata)
{
    if (!g_auracast_sink) {
        PRINT("not initialized");
        return;
    }

    if (status != BT_STATUS_SUCCESS) {
        PRINT("failed to start scan, status = %d", status);
        if (g_auracast_sink->scanner == AURACAST_SINK_PTR_PENDING)
            g_auracast_sink->scanner = NULL;

        return;
    }

    if (g_auracast_sink == userdata && g_auracast_sink->scanner == AURACAST_SINK_PTR_PENDING) {
        PRINT("scan started, scanner = %p", scan);
        g_auracast_sink->scanner = scan;
        return;
    }

    PRINT("unexpected scan started, scanner = %p", scan);
    bt_le_stop_scan_async(ins, scan, NULL, NULL);
}

static int scan_start_cmd(void* handle, int argc, char* argv[])
{
    bt_status_t status;
    ble_scan_settings_t settings;

    if (!g_auracast_sink) {
        PRINT("not initialized");
        return CMD_INVALID_OPT;
    }

    if (g_auracast_sink->scanner) {
        PRINT("already scanning");
        return CMD_USAGE_FAULT;
    }

    if (g_auracast_sink->scanner == AURACAST_SINK_PTR_PENDING) {
        PRINT("previous scan is starting");
        return CMD_USAGE_FAULT;
    }

    memcpy(&settings, &default_scan_settings, sizeof(settings));

    status = bt_le_start_scan_settings_async(handle, &settings, &scanner_cbs, scan_start_cb,
        g_auracast_sink);
    if (status != BT_STATUS_SUCCESS) {
        PRINT("failed to start a scan");
        return CMD_ERROR;
    }

    g_auracast_sink->scanner = AURACAST_SINK_PTR_PENDING;
    PRINT("starting scan");

    return CMD_OK;
}

int auracast_sink_command_init_async(void* handle)
{
    return CMD_OK;
}

void auracast_sink_command_uninit_async(void* handle)
{
}

int auracast_sink_command_exec_async(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_auracast_sink_tables,
            ARRAY_SIZE(g_auracast_sink_tables), argc, argv, 0);

    if (ret < 0)
        usage();

    return ret;
}
