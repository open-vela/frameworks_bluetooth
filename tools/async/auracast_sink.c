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

#define BTTOOL_AURACAST_SINK_LOG_SIZE (256)
#define BTTOOL_PA_SYNC_PA_REPORT_LIFE (10)
#define BTTOOL_PA_SYNC_DEFAULT_TIMEOUT_MS (1000)
#define BTTOOL_PA_SYNC_DEFAULT_SKIP (1)

#define SEARCH_TYPE_SYNC (0)
#define SEARCH_TYPE_SINK (1)

#define AURACAST_SINK_PTR_PENDING ((void*)-1)

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
    bt_le_address_t addr;
    uint8_t sid;
    uint8_t cnt;
    uint8_t type;
    void* out;
} bttool_auracast_sync_iter_t;

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

static const struct option sync_select_options[] = {
    { "addr", required_argument, 0, 'a' },
    { "type", required_argument, 0, 't' },
    { "sid", required_argument, 0, 's' },
    { 0, 0, 0, 0 },
};

static const struct option auracast_recv_options[] = {
    { "bis", required_argument, 0, 'b' },
    { "encryption", required_argument, 0, 'e' },
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

    if (!g_auracast_sink || !bt_list_find(g_auracast_sink->sync_list, sync_cmp, sync))
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
            bt_audio_context_to_str(log + strlen(log), size - strlen(log),
                subgroup->metadata.context);
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

    if (!g_auracast_sink || !bt_list_find(g_auracast_sink->sync_list, sync_cmp, sync))
        return;

    log = zalloc(size); /**< for print log */
    if (!log)
        return;

    BTTOOL_STRCAT(log, size, "%s from [%02x:%02x:%02x:%02x:%02x:%02x][%s(%d)], sid:0x%x, "
                             "cnt = %d, len = %d",
        __func__, addr->addr[5], addr->addr[4], addr->addr[3], addr->addr[2], addr->addr[1],
        addr->addr[0], parse_addr_type(addr->addr_type), addr->addr_type, sid, report->cnt,
        report->adv_data_len);

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

    if (sync->base_parsed)
        goto exit; /**< avoid spam */

    sync->base_parsed = true;
    dump_auracast_audio_info(info);

exit:
    free(info);
    free(log);
}

static void on_auracast_ready(const bt_le_address_t* addr, uint8_t sid, bool encrypted,
    void* context)
{
    bttool_auracast_pa_sync_t* sync = (bttool_auracast_pa_sync_t*)context;

    if (!g_auracast_sink || !bt_list_find(g_auracast_sink->sync_list, sync_cmp, sync))
        return;

    if (sync->auracast_ready)
        return; /**< avoid spam */

    sync->auracast_ready = true;

    PRINT_ADDR("on_auracast_ready, addr:[%s][%s], sid:0x%x%s", (const bt_address_t*)addr->addr,
        parse_addr_type(addr->addr_type), sid, encrypted ? ", encrypted" : "");
}

static const bt_pa_sync_callbacks_t pa_sync_cbs = {
    .on_sync_established = on_sync_established,
    .on_sync_terminated = on_sync_terminated,
    .on_sync_report = on_sync_report,
    .on_auracast_ready = on_auracast_ready,
};

static const bt_pa_sync_create_param_t default_sync_params = {
    .skip = BTTOOL_PA_SYNC_DEFAULT_SKIP,
    .timeout = BTTOOL_PA_SYNC_DEFAULT_TIMEOUT_MS / 10,
    .filter = false,
    .no_report = false,
};

static void cnt_sync(void* data, void* context)
{
    bttool_auracast_remote_t* remote = (bttool_auracast_remote_t*)data;
    bttool_auracast_sync_iter_t* iter = (bttool_auracast_sync_iter_t*)context;

    if (!bt_addr_is_empty((bt_address_t*)iter->addr.addr)
        && memcmp(&remote->addr.addr, &iter->addr.addr, BT_ADDR_LENGTH)) {
        /** addr provided but not match */
        return;
    }

    if (iter->addr.addr_type != BT_LE_ADDR_TYPE_UNKNOWN
        && iter->addr.addr_type != remote->addr.addr_type) {
        /** addr_type provided but not match */
        return;
    }

    if ((iter->sid != BLE_SCAN_SID_NOT_PROVIDED) && iter->sid != remote->sid) {
        /** sid provided but not match */
        return;
    }

    if (iter->out == NULL)
        iter->out = remote; /**< record the first sync matched */

    iter->cnt++;
}

/** @brief Find one sync with full or insufficient information */
static bttool_auracast_remote_t* find_sync(const bt_le_address_t* addr, uint8_t sid, uint8_t type)
{
    bt_list_t* list;
    bttool_auracast_sync_iter_t iter = { 0 };

    list = type == SEARCH_TYPE_SINK ? g_auracast_sink->sink_list : g_auracast_sink->sync_list;
    if (bt_list_is_empty(list))
        return NULL;

    iter.type = type;
    iter.sid = sid;
    iter.addr.addr_type = BT_LE_ADDR_TYPE_UNKNOWN;
    if (addr) {
        memcpy(&iter.addr, addr, sizeof(bt_le_address_t));
        iter.addr.addr_type = addr->addr_type;
    }

    bt_list_foreach(list, cnt_sync, &iter);

    if (!iter.out)
        return NULL; /**< nothing matched */

    if (iter.cnt == 1)
        return iter.out; /**< the only one matched */

    if (addr == NULL || bt_addr_is_empty((bt_address_t*)iter.addr.addr)) {
        PRINT("input address by -a <addr>");
        return NULL;
    }

    if (sid == BLE_SCAN_SID_NOT_PROVIDED) {
        PRINT("input sid by -s <sid>");
        return NULL;
    }

    if (addr->addr_type == BT_LE_ADDR_TYPE_UNKNOWN) {
        PRINT("input address type by -t <type>");
        return NULL;
    }

    return iter.out;
}

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

static void scan_stop_cb(bt_instance_t* ins, void* userdata)
{
    PRINT("scan stopped");
}

static int scan_stop_cmd(void* handle, int argc, char* argv[])
{
    bt_status_t status;

    if (!g_auracast_sink) {
        PRINT("not initialized");
        return CMD_INVALID_OPT;
    }

    if (!g_auracast_sink->scanner) {
        PRINT("not scanning");
        return CMD_USAGE_FAULT;
    }

    if (g_auracast_sink->scanner == AURACAST_SINK_PTR_PENDING) {
        PRINT("scan is starting");
        return CMD_USAGE_FAULT;
    }

    PRINT("stop scan, scanner = %p", g_auracast_sink->scanner);

    status = bt_le_stop_scan_async(handle, g_auracast_sink->scanner, scan_stop_cb, g_auracast_sink);
    if (status != BT_STATUS_SUCCESS) {
        PRINT("failed to stop a scan");
        return CMD_ERROR;
    }

    g_auracast_sink->scanner = NULL;

    return CMD_OK;
}

static void sync_create_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    bttool_auracast_pa_sync_t* sync = userdata;

    if (!g_auracast_sink) {
        PRINT("not initialized");
        return;
    }

    if (status != BT_STATUS_SUCCESS) {
        PRINT("failed to create sync, status = %d", status);
        bt_list_remove(g_auracast_sink->sync_list, sync);
    }

    PRINT("sync create success: %p", sync);
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
        PRINT("not initialized");
        return CMD_INVALID_OPT;
    }

    sync = zalloc(sizeof(bttool_auracast_pa_sync_t));
    if (!sync) {
        PRINT("malloc failed");
        return CMD_ERROR;
    }

    sync->remote.sid = BLE_SCAN_SID_NOT_PROVIDED;

    memcpy(&params, &default_sync_params, sizeof(bt_pa_sync_create_param_t));

    while ((opt = getopt_long(argc, argv, "a:t:s:o:k:fn", sync_options, NULL)) != -1) {
        switch (opt) {
        case 'a':
            if (bt_addr_str2ba(optarg, (bt_address_t*)sync->remote.addr.addr) != 0) {
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

            sync->remote.addr.addr_type = val;
            break;
        case 's':
            val = strtoul(optarg, NULL, 16);
            if (val > BLE_SCAN_SID_MAX) {
                PRINT("invalid sid %s", optarg);
                ret = CMD_INVALID_PARAM;
                goto exit;
            }

            sync->remote.sid = val;
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

    if (bt_addr_is_empty((bt_address_t*)sync->remote.addr.addr)) {
        PRINT("sync to a nearby device");
        if (!g_auracast_sink->nearby_pa) {
            PRINT("device not found");
            ret = CMD_PARAM_NOT_ENOUGH;
            goto exit;
        }

        memcpy(sync->remote.addr.addr, g_auracast_sink->nearby_pa->addr.addr, BT_ADDR_LENGTH);
        sync->remote.addr.addr_type = g_auracast_sink->nearby_pa->type;
        sync->remote.sid = g_auracast_sink->nearby_pa->sid;
    }

    if (sync->remote.sid == BLE_SCAN_SID_NOT_PROVIDED) {
        PRINT("sid not provided, input by -s <sid>");
        ret = CMD_PARAM_NOT_ENOUGH;
        goto exit;
    }

    status = bt_pa_sync_create_async(handle, &sync->remote.addr, sync->remote.sid, &params,
        &pa_sync_cbs, sync, sync_create_cb, sync);
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

static int general_find_sync(void** out, int argc, char* const argv[], uint8_t type)
{
    int opt;
    bt_le_address_t addr = { 0 };
    uint8_t sid;
    uint32_t val;
    bttool_auracast_remote_t* sync;

    addr.addr_type = BT_ADDR_TYPE_UNKNOWN;
    sid = BLE_SCAN_SID_NOT_PROVIDED;

    while ((opt = getopt_long(argc, argv, "a:t:s:", sync_select_options, NULL)) != -1) {
        switch (opt) {
        case 'a':
            if (bt_addr_str2ba(optarg, (bt_address_t*)addr.addr) != 0) {
                PRINT("invalid address %s", optarg);
                return CMD_INVALID_ADDR;
            }

            break;
        case 't':
            val = strtoul(optarg, NULL, 10);
            if (val > 1) {
                PRINT("invalid address type %s", optarg);
                return CMD_INVALID_PARAM;
            }

            addr.addr_type = val;
            break;
        case 's':
            val = strtoul(optarg, NULL, 16);
            if (val > BLE_SCAN_SID_MAX) {
                PRINT("invalid sid %s", optarg);
                return CMD_INVALID_PARAM;
            }

            sid = val;
            break;
        }
    }

    sync = find_sync(&addr, sid, type);
    if (!sync) {
        PRINT("sync not found");
        return CMD_INVALID_PARAM;
    }

    *out = sync;

    return CMD_OK;
}

static void sync_terminate_cb(bt_instance_t* ins, bt_status_t status, void* userdata)
{
    bttool_auracast_pa_sync_t* sync = userdata;

    if (!g_auracast_sink) {
        PRINT("not initialized");
        return;
    }

    if (status != BT_STATUS_SUCCESS) {
        PRINT("failed to terminate sync, status = %d", status);
        return;
    }

    PRINT("sync terminate success: %p", sync);
}

static int sync_terminate_cmd(void* handle, int argc, char* argv[])
{
    int ret;
    bt_status_t status;
    bttool_auracast_pa_sync_t* sync;

    PRINT("%s", __func__);

    ret = general_find_sync((void**)&sync, argc, argv, SEARCH_TYPE_SYNC);
    if (ret != CMD_OK)
        return ret;

    status = bt_pa_sync_terminate_async(handle, &sync->remote.addr, sync->remote.sid,
        sync_terminate_cb, sync);
    if (status != BT_STATUS_SUCCESS) {
        PRINT("failed to terminate sync, status = %d", status);
        return CMD_ERROR;
    }

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
