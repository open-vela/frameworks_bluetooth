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

#include "bt_le_scan.h"
#include "bt_pa_sync.h"
#include "bt_tools.h"

typedef struct {
    bt_scanner_t* scanner;
} bttool_auracast_sink_t;

static int scan_cmd(void* handle, int argc, char* argv[]);

static bttool_auracast_sink_t* g_auracast_sink = NULL;
static bt_command_t g_auracast_sink_tables[] = {
    { "scan", scan_cmd, 0, "\"Search for nearby Auracast sources\"" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_auracast_sink_tables); i++) {
        printf("\t%-8s\t%s\n", g_auracast_sink_tables[i].cmd, g_auracast_sink_tables[i].help);
    }
}

static void on_scan_result(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    bt_status_t status;
    bt_pa_sync_info_t* info;

    if (g_auracast_sink->scanner != scanner)
        return;

    info = malloc(sizeof(bt_pa_sync_info_t));
    if (info == NULL)
        return;

    status = bt_pa_sync_parse_adv_data(info, result);
    if (status != BT_STATUS_SUCCESS)
        goto exit;

    PRINT("%s, device name = %s, broadcast name = %s, id = %" PRIu32 ", sid = %d, "
          "txpower = %d dBm, rssi = %d dBm",
        __func__, info->name, info->broadcast_name, info->broadcast_id, result->sid,
        result->tx_power, result->rssi);

exit:
    free(info);
}

static void on_scan_status(bt_scanner_t* scanner, uint8_t status)
{
    PRINT("%s, status = %d", __func__, status);
    if (g_auracast_sink->scanner != scanner)
        return;

    if (status != BT_STATUS_SUCCESS)
        g_auracast_sink->scanner = NULL;
}

static void on_scan_stopped(bt_scanner_t* scanner)
{
    PRINT("%s", __func__);
    if (g_auracast_sink->scanner != scanner)
        return;

    g_auracast_sink->scanner = NULL;
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

int scan_cmd(void* handle, int argc, char* argv[])
{
    ble_scan_settings_t settings;

    if (g_auracast_sink->scanner) {
        PRINT("Already scanning");
        return CMD_USAGE_FAULT;
    }

    memcpy(&settings, &default_scan_settings, sizeof(settings));

    g_auracast_sink->scanner = bt_le_start_scan_settings(handle, &settings, &scanner_cbs);
    if (g_auracast_sink->scanner == NULL)
        return CMD_ERROR;

    return CMD_OK;
}

int auracast_sink_command_init(void* handle)
{
    g_auracast_sink = zalloc(sizeof(bttool_auracast_sink_t));
    if (!g_auracast_sink)
        return CMD_ERROR;

    return CMD_OK;
}

void auracast_sink_command_uninit(void* handle)
{
    free(g_auracast_sink);
    g_auracast_sink = NULL;
}

int auracast_sink_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_auracast_sink_tables,
            ARRAY_SIZE(g_auracast_sink_tables), argc, argv, 0);

    if (ret < 0)
        usage();

    return ret;
}
