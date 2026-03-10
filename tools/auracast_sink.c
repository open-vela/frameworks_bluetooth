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

#define BTTOOL_AURACAST_SINK_LOG_SIZE (256)
#define BTTOOL_PA_SYNC_PA_REPORT_LIFE (10)
#define BTTOOL_PA_SYNC_DEFAULT_TIMEOUT_MS (1000)
#define BTTOOL_PA_SYNC_DEFAULT_SKIP (1)

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
    int rssi;
} bttool_auracast_pa_sync_t;

typedef struct {
    bt_scanner_t* scanner;
    bttool_auracast_pa_record_t* nearby_pa;
    bt_list_t* sync_list; /**< bttool_auracast_pa_sync_t* */
} bttool_auracast_sink_t;

static int scan_start_cmd(void* handle, int argc, char* argv[]);
static int scan_stop_cmd(void* handle, int argc, char* argv[]);
static int sync_create_cmd(void* handle, int argc, char* argv[]);

static bttool_auracast_sink_t* g_auracast_sink = NULL;

static const struct option sync_options[] = {
    { "addr", required_argument, 0, 'a' },
    { "type", required_argument, 0, 't' },
    { "sid", required_argument, 0, 's' },
    { "timeout", required_argument, 0, 'o' },
    { "skip", required_argument, 0, 'k' },
    { "filter", no_argument, 0, 'f' },
    { "no-report", no_argument, 0, 'n' },
    { 0, 0, 0, 0 },
};

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

static const char* parse_addr_type(ble_addr_type_t type)
{
    switch (type) {
    case BT_LE_ADDR_TYPE_PUBLIC:
        return "Public";
    case BT_LE_ADDR_TYPE_RANDOM:
        return "Random";
    case BT_LE_ADDR_TYPE_PUBLIC_ID:
        return "Public ID";
    case BT_LE_ADDR_TYPE_RANDOM_ID:
        return "Random ID";
    case BT_LE_ADDR_TYPE_ANONYMOUS:
        return "Anonymous";
    default:
        break;
    }

    return "Unknown";
}

static void update_neaby_pa(const ble_scan_result_t* result)
{
    bttool_auracast_pa_record_t* prev;

    if (!g_auracast_sink->nearby_pa)
        g_auracast_sink->nearby_pa = zalloc(sizeof(bttool_auracast_pa_record_t));

    prev = g_auracast_sink->nearby_pa;

    if (!prev)
        return;

    if (prev->life)
        prev->life--;

    if (!prev->life)
        prev->rssi = INT8_MIN;

    if ((bt_addr_compare(&prev->addr, &result->addr) == 0) && (prev->type == result->addr_type)
        && (prev->sid == result->sid)) {
        prev->life = BTTOOL_PA_SYNC_PA_REPORT_LIFE;
        prev->rssi = result->rssi;
        return;
    }

    if ((prev->life > 0) && (prev->rssi > result->rssi))
        return;

    bt_addr_set(&prev->addr, result->addr.addr);
    prev->life = BTTOOL_PA_SYNC_PA_REPORT_LIFE;
    prev->type = result->addr_type;
    prev->sid = result->sid;
}

static void on_scan_result(bt_scanner_t* scanner, ble_scan_result_t* result)
{
    bt_status_t status;
    bt_pa_sync_info_t* info = NULL;
    char* log = NULL;
    size_t size = BTTOOL_AURACAST_SINK_LOG_SIZE;

    if (!g_auracast_sink || g_auracast_sink->scanner != scanner)
        return;

    info = malloc(sizeof(bt_pa_sync_info_t));
    if (info == NULL)
        return;

    status = bt_pa_sync_parse_adv_data(info, result);
    if (status != BT_STATUS_SUCCESS)
        goto exit;

    log = zalloc(size); /**< for print log */
    if (!log)
        goto exit;

    BTTOOL_STRCAT(log, size, "%s from [%02x:%02x:%02x:%02x:%02x:%02x][%s(%d)]", __func__,
        result->addr.addr[5], result->addr.addr[4], result->addr.addr[3], result->addr.addr[2],
        result->addr.addr[1], result->addr.addr[0], parse_addr_type(result->addr_type),
        result->addr_type);

    if (info->name[0] != '\0')
        BTTOOL_STRCAT(log, size, ", device:%s", info->name);

    if (info->broadcast_name[0] != '\0')
        BTTOOL_STRCAT(log, size, ", broadcast name:%s", info->broadcast_name);

    if (info->broadcast_id != BT_INVALID_BROADCAST_ID)
        BTTOOL_STRCAT(log, size, ", id:0x%06" PRIx32, info->broadcast_id);

    if (result->sid != 0xFF)
        BTTOOL_STRCAT(log, size, ", sid:0x%x", result->sid);

    if (result->tx_power != BT_POWER_UNAVAILABLE)
        BTTOOL_STRCAT(log, size, ", txpower:%d", result->tx_power);

    if (result->rssi != BT_POWER_UNAVAILABLE)
        BTTOOL_STRCAT(log, size, ", rssi:%d", result->rssi);

    PRINT("%s", log);
    update_neaby_pa(result);

exit:
    free(info);
    free(log);
}

static void on_scan_status(bt_scanner_t* scanner, uint8_t status)
{
    PRINT("%s, status = %d", __func__, status);

    if (!g_auracast_sink || g_auracast_sink->scanner != scanner) {
        PRINT("%s, scanner(%p) mismatch", __func__, scanner);
        return;
    }

    if (status != BT_STATUS_SUCCESS)
        g_auracast_sink->scanner = NULL;
}

static void on_scan_stopped(bt_scanner_t* scanner)
{
    PRINT("%s", __func__);

    if (!g_auracast_sink || g_auracast_sink->scanner != scanner) {
        PRINT("%s, scanner(%p) mismatch", __func__, scanner);
        return;
    }

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

static bool sync_cmp(void* data, void* context)
{
    const bttool_auracast_pa_sync_t* sync_record = (const bttool_auracast_pa_sync_t*)data;
    const bttool_auracast_pa_sync_t* sync_in = (const bttool_auracast_pa_sync_t*)context;

    if (!sync_in)
        return false;

    return sync_record == sync_in;
}

static void on_sync_established(const bt_le_address_t* addr, uint8_t sid, void* context)
{
    bttool_auracast_pa_sync_t* sync = (bttool_auracast_pa_sync_t*)context;

    if (!g_auracast_sink || !bt_list_find(g_auracast_sink->sync_list, sync_cmp, sync))
        return;

    PRINT_ADDR("on_sync_established, addr:[%s][%s], sid:0x%x", (const bt_address_t*)addr->addr,
        parse_addr_type(addr->addr_type), sid);
}

static void on_sync_terminated(const bt_le_address_t* addr, uint8_t sid, void* context)
{
    bttool_auracast_pa_sync_t* sync = (bttool_auracast_pa_sync_t*)context;

    if (!g_auracast_sink ||!bt_list_find(g_auracast_sink->sync_list, sync_cmp, sync))
        return;

    PRINT_ADDR("on_sync_terminated, addr:[%s][%s], sid:0x%x", (const bt_address_t*)addr->addr,
        parse_addr_type(addr->addr_type), sid);

    bt_list_remove(g_auracast_sink->sync_list, sync);
}

static void dump_auracast_lc3_info(const char* prefix, const bt_auracast_audio_lc3_config_t* lc3)
{
    size_t size = BTTOOL_AURACAST_SINK_LOG_SIZE;
    char* log = zalloc(size); /**< for print log */
    if (!log)
        return;

    BTTOOL_STRCAT(log, size, "%sLC3: ", prefix);
    BTTOOL_STRCAT(log, size, "Freq[%sHz(0x%02x)]",
        bt_audio_sampling_frequency_to_str(lc3->sampling_frequency), lc3->sampling_frequency);
    BTTOOL_STRCAT(log, size, ", Duration[%sms(0x%02x)]", bt_audio_duration_to_str(lc3->duration),
        lc3->duration);
    BTTOOL_STRCAT(log, size, ", Location[");
    bt_audio_location_to_str(log + strlen(log), size - strlen(log), lc3->location);
    BTTOOL_STRCAT(log, size, "(0x%08" PRIx32 ")]", lc3->location);
    if (lc3->octets_per_frame)
        BTTOOL_STRCAT(log, size, ", Octets/Frame:%d", lc3->octets_per_frame);

    if (lc3->blocks_per_sdu)
        BTTOOL_STRCAT(log, size, ", Blocks/SDU:%d", lc3->blocks_per_sdu);

    PRINT("%s", log);

    free(log);
}

static void dump_auracast_audio_info(const bt_auracast_audio_info_t* info)
{
    size_t size = BTTOOL_AURACAST_SINK_LOG_SIZE;
    char* log = zalloc(size); /**< for print log */
    if (!log)
        return;

    PRINT("%s, Presentation Delay %" PRIu32 " us, %d Subgroups", __func__, info->presentation_delay,
        info->num_subgroups);
    for (uint8_t i = 0; i < info->num_subgroups; i++) {
        const bt_auracast_audio_subgroup_t* subgroup = &info->subgroup[i];
        PRINT("Group[%d]:", i);
        PRINT("\tNum_bis[%d]", subgroup->num_bis);

        /** Codec ID */
        log[0] = '\0';
        BTTOOL_STRCAT(log, size, "\tCodec[%s(0x%02x)]",
            bt_audio_codec_id_to_str(subgroup->codec_id.coding_format),
            subgroup->codec_id.coding_format);
        if (subgroup->codec_id.coding_format == BT_CODEC_ID_VENDOR)
            BTTOOL_STRCAT(log, size, ", company[0x%04x], id[0x%04x]", subgroup->codec_id.company_id,
                subgroup->codec_id.vendor_id);

        PRINT("%s", log);

        /** Codec Specific Configuration */
        if (subgroup->codec_id.coding_format == BT_CODEC_ID_LC3)
            dump_auracast_lc3_info("\t", &subgroup->config.lc3);

        /** Metadata */
        log[0] = '\0';
        BTTOOL_STRCAT(log, size, "\tMetadata");
        if (subgroup->metadata.context) {
            BTTOOL_STRCAT(log, size, ", context[");
            bt_audio_context_to_str(log + strlen(log), size - strlen(log), subgroup->metadata.context);
            BTTOOL_STRCAT(log, size, "(0x%04x)]", subgroup->metadata.context);
        }

        if (strlen(subgroup->metadata.language))
            BTTOOL_STRCAT(log, size, ", language[%s]", subgroup->metadata.language);

        PRINT("%s", log);

        /** BIS info */
        for (uint8_t k = 0; k < subgroup->num_bis; k++) {
            const bt_auracast_audio_bis_info_t* bis = &subgroup->bis[k];
            PRINT("\tBIS[%d]:", k);
            PRINT("\t\tIndex[%d]", bis->index);
            if (subgroup->codec_id.coding_format == BT_CODEC_ID_LC3)
                dump_auracast_lc3_info("\t\t", &bis->config.lc3);
        }

        PRINT("\n");
    }

    free(log);
}

static void on_sync_report(const bt_le_address_t* addr, uint8_t sid,
    const bt_pa_sync_report_t* report, void* context)
{
    bt_status_t status;
    bt_auracast_audio_info_t* info = NULL;
    bttool_auracast_pa_sync_t* sync = (bttool_auracast_pa_sync_t*)context;
    char* log = NULL;
    size_t size = BTTOOL_AURACAST_SINK_LOG_SIZE;

    if (!g_auracast_sink ||!bt_list_find(g_auracast_sink->sync_list, sync_cmp, sync))
        return;

    PRINT_ADDR("on_sync_report, addr:[%s][%s], sid:0x%x", (const bt_address_t*)addr->addr,
        parse_addr_type(addr->addr_type), sid);

    log = zalloc(size); /**< for print log */
    if (!log)
        return;

    BTTOOL_STRCAT(log, size, "\tcnt = %d, len = %d", report->cnt, report->adv_data_len);
    if (report->tx_power != BT_POWER_UNAVAILABLE)
        BTTOOL_STRCAT(log, size, ", txpower:%d", report->tx_power);

    if (report->rssi != BT_POWER_UNAVAILABLE) {
        BTTOOL_STRCAT(log, size, ", rssi:%d", report->rssi);
        sync->rssi = report->rssi;
    }

    PRINT("%s", log);

    info = malloc(sizeof(bt_auracast_audio_info_t));
    if (info == NULL)
        goto exit;

    status = bt_auracast_sink_parse_adv_data(info, report);
    if (status != BT_STATUS_SUCCESS)
        goto exit;

    dump_auracast_audio_info(info);

exit:
    free(info);
    free(log);
}

static const bt_pa_sync_callbacks_t pa_sync_cbs = {
    .on_sync_established = on_sync_established,
    .on_sync_terminated = on_sync_terminated,
    .on_sync_report = on_sync_report,
};

static const bt_pa_sync_create_param_t default_sync_params = {
    .skip = BTTOOL_PA_SYNC_DEFAULT_SKIP,
    .timeout = BTTOOL_PA_SYNC_DEFAULT_TIMEOUT_MS / 10,
    .filter = false,
    .no_report = false,
};

static int scan_start_cmd(void* handle, int argc, char* argv[])
{
    ble_scan_settings_t settings;

    if (!g_auracast_sink) {
        PRINT("Not initialized");
        return CMD_INVALID_OPT;
    }

    if (g_auracast_sink->scanner) {
        PRINT("Already scanning");
        return CMD_USAGE_FAULT;
    }

    memcpy(&settings, &default_scan_settings, sizeof(settings));

    g_auracast_sink->scanner = bt_le_start_scan_settings(handle, &settings, &scanner_cbs);
    if (g_auracast_sink->scanner == NULL) {
        PRINT("Failed to start a scan");
        return CMD_ERROR;
    }

    PRINT("Starting scan, scanner = %p", g_auracast_sink->scanner);

    return CMD_OK;
}

static int scan_stop_cmd(void* handle, int argc, char* argv[])
{
    if (!g_auracast_sink) {
        PRINT("Not initialized");
        return CMD_INVALID_OPT;
    }

    if (!g_auracast_sink->scanner) {
        PRINT("Not scanning");
        return CMD_USAGE_FAULT;
    }

    PRINT("Stop scan, scanner = %p", g_auracast_sink->scanner);

    bt_le_stop_scan(handle, g_auracast_sink->scanner);
    g_auracast_sink->scanner = NULL;

    return CMD_OK;
}

static int sync_create_cmd(void* handle, int argc, char* argv[])
{
    int opt;
    int ret = CMD_OK;
    uint32_t val;
    bt_status_t status;
    bt_pa_sync_create_param_t params = { 0 };
    bttool_auracast_pa_sync_t* sync;

    PRINT("%s", __func__);

    if (!g_auracast_sink) {
        PRINT("Not initialized");
        return CMD_INVALID_OPT;
    }

    sync = zalloc(sizeof(bttool_auracast_pa_sync_t));
    if (!sync) {
        PRINT("malloc failed");
        return CMD_ERROR;
    }

    sync->sid = BLE_SCAN_SID_NOT_PROVIDED;

    memcpy(&params, &default_sync_params, sizeof(bt_pa_sync_create_param_t));

    while ((opt = getopt_long(argc, argv, "a:t:s:o:k:fn", sync_options, NULL)) != -1) {
        switch (opt) {
        case 'a':
            if (bt_addr_str2ba(optarg, (bt_address_t*)sync->addr.addr) != 0) {
                PRINT("invalid address %s", optarg);
                ret = CMD_INVALID_ADDR;
                goto exit;
            }

            break;
        case 't':
            val = strtoul(optarg, NULL, 10);
            if (val > 1) {
                PRINT("invalid address type %s", optarg);
                ret = CMD_INVALID_PARAM;
                goto exit;
            }

            sync->addr.addr_type = val;
            break;
        case 's':
            val = strtoul(optarg, NULL, 16);
            if (val > BLE_SCAN_SID_MAX) {
                PRINT("invalid sid %s", optarg);
                ret = CMD_INVALID_PARAM;
                goto exit;
            }

            sync->sid = val;
            break;
        case 'o':
            val = strtoul(optarg, NULL, 10);
            if (val < BT_PA_SYNC_TIMEOUT_MIN || val > BT_PA_SYNC_TIMEOUT_MAX) {
                PRINT("invalid timeout %s", optarg);
                ret = CMD_INVALID_PARAM;
                goto exit;
            }

            params.timeout = val;
            break;
        case 'k':
            val = strtoul(optarg, NULL, 10);
            if (val > BT_PA_SYNC_SKIP_MAX) {
                PRINT("invalid skip %s", optarg);
                ret = CMD_INVALID_PARAM;
                goto exit;
            }

            params.skip = val;
            break;
        case 'f':
            params.filter = true;
            break;
        case 'n':
            params.no_report = true;
            break;
        }
    }

    if (bt_addr_is_empty((bt_address_t*)sync->addr.addr)) {
        PRINT("sync to a nearby device");
        if (!g_auracast_sink->nearby_pa) {
            PRINT("device not found");
            ret = CMD_PARAM_NOT_ENOUGH;
            goto exit;
        }

        memcpy(sync->addr.addr, g_auracast_sink->nearby_pa->addr.addr, BT_ADDR_LENGTH);
        sync->addr.addr_type = g_auracast_sink->nearby_pa->type;
        sync->sid = g_auracast_sink->nearby_pa->sid;
    }

    if (sync->sid == BLE_SCAN_SID_NOT_PROVIDED) {
        PRINT("sid not provided, input by -s <sid>");
        ret = CMD_PARAM_NOT_ENOUGH;
        goto exit;
    }

    status = bt_pa_sync_create(handle, &sync->addr, sync->sid, &params, &pa_sync_cbs, sync);
    if (status != BT_STATUS_SUCCESS) {
        PRINT("failed to create sync, status = %d", status);
        ret = CMD_ERROR;
    }

exit:
    if (ret == CMD_OK)
        bt_list_add_tail(g_auracast_sink->sync_list, sync);
    else
        free(sync);

    return ret;
}

int auracast_sink_command_init(void* handle)
{
    g_auracast_sink = zalloc(sizeof(bttool_auracast_sink_t));
    if (!g_auracast_sink)
        return CMD_ERROR;

    g_auracast_sink->sync_list = bt_list_new(free);
    return CMD_OK;
}

void auracast_sink_command_uninit(void* handle)
{
    if (!g_auracast_sink)
        return;

    bt_list_free(g_auracast_sink->sync_list);
    free(g_auracast_sink->nearby_pa);
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
