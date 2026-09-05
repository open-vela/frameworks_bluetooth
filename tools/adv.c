/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "advertiser_data.h"
#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_le_advertiser.h"
#include "bt_tools.h"

static int start_adv_cmd(void* handle, int argc, char* argv[]);
static int stop_adv_cmd(void* handle, int argc, char* argv[]);
static int set_adv_data_cmd(void* handle, int argc, char* argv[]);
static int dump_adv_cmd(void* handle, int argc, char* argv[]);

static struct option adv_options[] = {
    { "adv_type", required_argument, 0, 't' },
    { "mode", required_argument, 0, 'm' },
    { "interval", required_argument, 0, 'i' },
    { "peer_addr", required_argument, 0, 'P' },
    { "peer_addr_type", required_argument, 0, 'T' },
    { "own_addr", required_argument, 0, 'O' },
    { "own_addr_type", required_argument, 0, 'R' },
    { "tx_power", required_argument, 0, 'p' },
    { "channel", required_argument, 0, 'c' },
    { "filter", required_argument, 0, 'f' },
    { "duration", required_argument, 0, 'd' },
    { "default", no_argument, 0, 'D' },
    { "raw_data", required_argument, 0, 'r' },
    { 0, 0, 0, 0 }
};

static struct option adv_stop_options[] = {
    { "advid", required_argument, 0, 'i' },
    { "handle", required_argument, 0, 'h' },
    { 0, 0, 0, 0 }
};

static bt_command_t g_adv_tables[] = {
    { "start", start_adv_cmd, 1, "start advertising\n"
                                 "\t  -t or --adv_type, advertising type opt(adv_ind/direct_ind/nonconn_ind/scan_ind)\n"
                                 "\t  -m or --mode,     advertising mode opt(legacy/ext/auto, default auto)\n"
                                 "\t  -i or --interval, advertising intervel range 0x20~0x4000\n"
                                 "\t  -n or --name,     advertising name no more than 29 bytes \n"
                                 "\t  -a or --appearance, advertising appearance range 0000~FFFF \n"
                                 "\t  -P or --peer_addr, if directed advertising is performed, shall be valid\n"
                                 "\t  -T or --peer_addr_type, if directed advertising is performed, shall be valid\n"
                                 "\t  -O or --own_addr, update own random address for this advertising, mandatory when own addr type is random\n"
                                 "\t  -R or --own_addr_type, address type(public/random/public_id/random_id/anonymous)\n"
                                 "\t  -p or --tx_power, advertising tx power range -20~10 dBm\n"
                                 "\t  -c or --channel, advertising channel map opt (37/38/39, 0 means default)\n"
                                 "\t  -f or --filter, advertising white list filter policy(none/scan/conn/all)\n"
                                 "\t  -d or --duration, advertising duration, only extended adv valid, range 0x0~0xFFFF\n"
                                 "\t  -D or --default, use default advertising data and scan response data\n"
                                 "\t  -r or --raw_data, advertising data by design <adv_data>\n"
                                 "\t\t\t  e.g., 02010803FF8F03\n" },
    { "stop", stop_adv_cmd, 1, "stop  advertising  \n"
                               "\t  -i or --advid, advertising ID, advertising_start_cb notify \n"
                               "\t  -h or --handle, advertising handle, bt_le_start_advertising return \n" },
    { "set_data", set_adv_data_cmd, 1, "set advertising data, not implemented" },
    { "dump", dump_adv_cmd, 0, "dump adv current state" },
};

static uint8_t s_adv_data[] = { 0x02, 0x01, 0x08, 0x03, 0xFF, 0x8F, 0x03 }; /* flags: LE & BREDR, Manufacturer ID:0x038F */
static uint8_t s_rsp_data[] = { 0x08, 0x09, 0x56, 0x65, 0x6C, 0x61, 0x2D, 0x42, 0x54 }; /* Complete Local Name:Vela-BT */

static void usage(void)
{
    printf("Usage:\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_adv_tables); i++) {
        printf("\t%-4s\t%s\n", g_adv_tables[i].cmd, g_adv_tables[i].help);
    }
}

static void on_advertising_start_cb(bt_advertiser_t* adv, uint8_t adv_id, uint8_t status)
{
    PRINT("%s, handle:%p, adv_id:%d, status:%d", __func__, adv, adv_id, status);
}

static void on_advertising_stopped_cb(bt_advertiser_t* adv, uint8_t adv_id)
{
    PRINT("%s, handle:%p, adv_id:%d", __func__, adv, adv_id);
}

static advertiser_callback_t adv_callback = {
    sizeof(adv_callback),
    on_advertising_start_cb,
    on_advertising_stopped_cb
};

static int ensure_adapter_ble_on(void* handle)
{
    bt_adapter_state_t state;
    int retry;

    state = bt_adapter_get_state(handle);
    if (state == BT_ADAPTER_STATE_ON || state == BT_ADAPTER_STATE_BLE_ON)
        return CMD_OK;

    PRINT("Adapter not enabled, enable BLE before advertising state:%d", state);
#if defined(CONFIG_BLUETOOTH_BLE_SUPPORT) && !defined(CONFIG_BLUETOOTH_BREDR_SUPPORT)
    bt_adapter_enable_le(handle);
#else
    bt_adapter_enable(handle);
#endif

    for (retry = 0; retry < 50; retry++) {
        usleep(100 * 1000);
        state = bt_adapter_get_state(handle);
        if (state == BT_ADAPTER_STATE_ON || state == BT_ADAPTER_STATE_BLE_ON) {
            PRINT("Adapter BLE ready state:%d", state);
            return CMD_OK;
        }
    }

    PRINT("Adapter BLE enable timeout state:%d", state);
    return CMD_ERROR;
}

static int data_check(const char* str)
{
    while (*str) {
        if (!isxdigit(*str++))
            return -1;
    }

    return 0;
}

static uint8_t* str_to_array(const char* str, uint16_t* adv_len)
{
    int len, i;
    char tmp_byte[3] = { 0 };
    uint8_t* array_data;

    if ((strlen(str) & 1)) {
        PRINT("error hex string length, should be even.");
        return NULL;
    }

    if (data_check(str) < 0) {
        PRINT("error hex string length.");
        return NULL;
    }

    len = strlen(str) / 2;
    array_data = (uint8_t*)malloc(len);
    if (!array_data) {
        PRINT("No memory");
        return NULL;
    }

    for (i = 0; i < len; i++) {
        tmp_byte[0] = str[i * 2];
        tmp_byte[1] = str[i * 2 + 1];
        tmp_byte[2] = '\0';
        array_data[i] = (uint8_t)(strtol(tmp_byte, NULL, 16) & 0xFF);
    }

    *adv_len = len;

    return array_data;
}

static int start_adv_cmd(void* handle, int argc, char* argv[])
{
    uint8_t adv_mode = 0;
    ble_adv_params_t params = { 0 };
    advertiser_data_t *adv = NULL, *scan_rsp = NULL;
    uint8_t *p_adv_data = NULL, *p_scan_rsp_data = NULL, *p_raw_adv_data = NULL;
    uint16_t adv_len = 0, scan_rsp_len = 0;
    bt_advertiser_t* adv_handle;
    char* name = "VELA_BT";
    uint16_t appearance = 0;
    int opt;

    if (ensure_adapter_ble_on(handle) != CMD_OK)
        return CMD_ERROR;

    params.adv_type = BT_LE_ADV_IND;
    bt_addr_set_empty(&params.peer_addr);
    params.peer_addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    bt_addr_set_empty(&params.own_addr);
    params.own_addr_type = BT_LE_ADDR_TYPE_PUBLIC;
    params.interval = 320;
    params.tx_power = 0;
    params.channel_map = BT_LE_ADV_CHANNEL_DEFAULT;
    params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE;
    params.duration = 0;

    optind = 0;
    while ((opt = getopt_long(argc, argv, "+t:m:i:n:a:p:c:f:d:P:T:O:R:Dr:", adv_options,
                NULL))
        != -1) {
        switch (opt) {
        case 't':
            if (strncasecmp(optarg, "adv_ind", strlen("adv_ind")) == 0)
                params.adv_type = BT_LE_ADV_IND;
            else if (strncasecmp(optarg, "direct_ind", strlen("direct_ind")) == 0)
                params.adv_type = BT_LE_ADV_DIRECT_IND;
            else if (strncasecmp(optarg, "nonconn_ind", strlen("nonconn_ind")) == 0)
                params.adv_type = BT_LE_ADV_NONCONN_IND;
            else if (strncasecmp(optarg, "scan_ind", strlen("scan_ind")) == 0)
                params.adv_type = BT_LE_ADV_SCAN_IND;
            else if (strncasecmp(optarg, "scan_rsp_ind", strlen("scan_rsp_ind")) == 0)
                params.adv_type = BT_LE_SCAN_RSP;
            else {
                PRINT("error adv type: %s", optarg);
                return CMD_INVALID_PARAM;
            }

            PRINT("adv type: %s", optarg);
            break;
        case 'm':
            if (strncasecmp(optarg, "legacy", strlen("legacy")) == 0)
                adv_mode = 1;
            else if (strncasecmp(optarg, "ext", strlen("ext")) == 0)
                adv_mode = 2;
            else if (strncasecmp(optarg, "auto", strlen("auto")) == 0)
                adv_mode = 0;
            else {
                PRINT("erro adv mode: %s", optarg);
                return CMD_INVALID_PARAM;
            }

            PRINT("adv type: %s", optarg);
            break;
        case 'i': {
            int32_t interval = atoi(optarg);
            if (interval < 0x20 || interval > 0x4000) {
                PRINT("error interval, range must in 0x20~0x4000");
                return CMD_INVALID_PARAM;
            }

            params.interval = interval;
            PRINT("interval: %f ms", (float)interval * 0.625);
        } break;
        case 'n': {
            name = optarg;
            PRINT("adv name: %s ", optarg);
        } break;
        case 'a': {
            appearance = strtol(optarg, NULL, 16);
            PRINT("adv appearance: 0x%04x ", appearance);
        }
        case 'p': {
            int32_t power = atoi(optarg);
            if (power < -20 || power > 10) {
                PRINT("error tx power, range must in -20~10");
                return CMD_INVALID_PARAM;
            }

            params.tx_power = power;
            PRINT("tx_power: %" PRId32 " dBm", power);
        } break;
        case 'c': {
            int32_t channel = atoi(optarg);
            if (channel != 0 && channel != 37 && channel != 38 && channel != 39) {
                PRINT("error channel selected:%s, please choose \
                       one from 37,38,30, 0 means default",
                    optarg);
                return CMD_INVALID_PARAM;
            }

            if (channel == 0)
                params.channel_map = BT_LE_ADV_CHANNEL_DEFAULT;
            else if (channel == 37)
                params.channel_map = BT_LE_ADV_CHANNEL_37_ONLY;
            else if (channel == 38)
                params.channel_map = BT_LE_ADV_CHANNEL_38_ONLY;
            else if (channel == 39)
                params.channel_map = BT_LE_ADV_CHANNEL_39_ONLY;

            PRINT("channel map: %s", channel == 0 ? "default" : optarg);
        } break;
        case 'f': {
            if (strncasecmp(optarg, "none", strlen("none")) == 0)
                params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_NONE;
            else if (strncasecmp(optarg, "scan", strlen("scan")) == 0)
                params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_SCAN;
            else if (strncasecmp(optarg, "conn", strlen("conn")) == 0)
                params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_CONNECTION;
            else if (strncasecmp(optarg, "all", strlen("all")) == 0)
                params.filter_policy = BT_LE_ADV_FILTER_WHITE_LIST_FOR_ALL;
            else {
                PRINT("error filter policy: %s", optarg);
                return CMD_INVALID_PARAM;
            }

            PRINT("filter policy: %s", optarg);
        } break;
        case 'd': {
            int32_t duration = atoi(optarg);
            if (duration < 0 || duration > 0xFFFF) {
                PRINT("error duration, range in 0x0000~0xFFFF");
                return CMD_INVALID_PARAM;
            }
            PRINT("duration: %" PRId32 " ms", duration * 10);
            params.duration = duration;
        } break;
        case 'P': {
            bt_address_t peeraddr;
            if (bt_addr_str2ba(optarg, &peeraddr) != 0) {
                PRINT("unrecognizable address format %s", optarg);
                return CMD_INVALID_PARAM;
            }

            memcpy(&params.peer_addr, &peeraddr, sizeof(bt_address_t));
            PRINT("peer address: %s", optarg);
        } break;
        case 'T': {
            ble_addr_type_t type;

            if (le_addr_type(optarg, &type) < 0) {
                PRINT("unrecognizable address type: %s", optarg);
                return CMD_INVALID_PARAM;
            }
            params.peer_addr_type = type;
            PRINT("peer address type: %s", optarg);
        } break;
        case 'O': {
            bt_address_t ownaddr;
            if (bt_addr_str2ba(optarg, &ownaddr) != 0) {
                PRINT("unrecognizable address format %s", optarg);
                return CMD_INVALID_PARAM;
            }

            memcpy(&params.own_addr, &ownaddr, sizeof(bt_address_t));
            PRINT("own address: %s", optarg);
        } break;
        case 'R': {
            ble_addr_type_t type;

            if (le_addr_type(optarg, &type) < 0) {
                PRINT("unrecognizable address type: %s", optarg);
                return CMD_INVALID_PARAM;
            }
            params.own_addr_type = type;
            PRINT("own address type: %s", optarg);
        } break;
        case 'D': {
            p_adv_data = s_adv_data;
            adv_len = sizeof(s_adv_data);
            p_scan_rsp_data = s_rsp_data;
            scan_rsp_len = sizeof(s_rsp_data);
        } break;
        case 'r': {
            PRINT("adv_data: %s", optarg);
            p_raw_adv_data = str_to_array(optarg, &adv_len);
            if (!p_raw_adv_data) {
                PRINT("error raw adv data");
                return CMD_INVALID_PARAM;
            }
        } break;
        default:
            PRINT("%s, default opt:%c, arg:%s", __func__, opt, optarg);
            break;
        }
    }

    if (params.own_addr_type == BT_LE_ADDR_TYPE_RANDOM && bt_addr_is_empty(&params.own_addr)) {
        PRINT("should set own address using \"-O\" option");
        if (p_raw_adv_data)
            free(p_raw_adv_data);
        return CMD_INVALID_ADDR;
    }

    if (p_adv_data && p_raw_adv_data) {
        PRINT("should not set both \"-D\" and \"-r\" option");
        free(p_raw_adv_data);
        return CMD_INVALID_PARAM;
    }

    if (adv_mode == 1)
        params.adv_type += BT_LE_LEGACY_ADV_IND;
    else if (adv_mode == 2)
        params.adv_type += BT_LE_EXT_ADV_IND;

    if (p_raw_adv_data)
        p_adv_data = p_raw_adv_data;

    if (!p_adv_data) {
        bt_uuid_t uuid;

        adv = advertiser_data_new();

        /* set adv flags 0x08 */
        advertiser_data_set_flags(adv, BT_AD_FLAG_DUAL_MODE | BT_AD_FLAG_GENERAL_DISCOVERABLE);

        /* add spp uuid */
        bt_uuid16_create(&uuid, 0x1101);
        advertiser_data_add_service_uuid(adv, &uuid);

        /* add handsfree uuid */
        bt_uuid16_create(&uuid, 0x111E);
        advertiser_data_add_service_uuid(adv, &uuid);

        /* set adv appearance */
        if (appearance)
            advertiser_data_set_appearance(adv, appearance);

        /* build adverser data */
        p_adv_data = advertiser_data_build(adv, &adv_len);

        scan_rsp = advertiser_data_new();

        /* set adv complete name */
        advertiser_data_set_name(scan_rsp, name);

        /* build scan response data */
        p_scan_rsp_data = advertiser_data_build(scan_rsp, &scan_rsp_len);
    }

    if (p_adv_data)
        advertiser_data_dump(p_adv_data, adv_len, NULL);

    if (p_scan_rsp_data)
        advertiser_data_dump(p_scan_rsp_data, scan_rsp_len, NULL);

    adv_handle = bt_le_start_advertising(handle, &params,
        p_adv_data, adv_len,
        p_scan_rsp_data, scan_rsp_len,
        &adv_callback);

    PRINT("Advertising handle:%p", adv_handle);

    /* free advertiser data */
    if (adv)
        advertiser_data_free(adv);

    if (p_raw_adv_data)
        free(p_raw_adv_data);

    /* free scan response data */
    if (scan_rsp)
        advertiser_data_free(scan_rsp);

    return adv_handle ? CMD_OK : CMD_ERROR;
}

static int stop_adv_cmd(void* handle, int argc, char* argv[])
{
    int opt;

    optind = 0;
    while ((opt = getopt_long(argc, argv, "i:h:", adv_stop_options,
                NULL))
        != -1) {
        switch (opt) {
        case 'i': {
            char* endptr;
            long id;

            errno = 0;
            id = strtol(optarg, &endptr, 10);
            if (errno != 0 || *optarg == '\0' || *endptr != '\0'
                || id < 0 || id > UINT8_MAX) {
                PRINT("Invalid ID:%s", optarg);
                return CMD_INVALID_PARAM;
            }
            PRINT("Stop adv ID:%ld", id);
            bt_le_stop_advertising_id(handle, (uint8_t)id);
            return CMD_OK;
        } break;
        case 'h': {
            uint32_t advhandle = strtoul(optarg, NULL, 16);
            if (!advhandle) {
                PRINT("Invalid handle:0x%08" PRIx32 "", advhandle);
                return CMD_INVALID_PARAM;
            }
            PRINT("Stop adv handle:0x%08" PRIx32 "", advhandle);
            bt_le_stop_advertising(handle, INT2PTR(bt_advertiser_t*) advhandle);
            return CMD_OK;
        } break;
        default:
            break;
        }
    }

    return CMD_INVALID_OPT;
}

static int set_adv_data_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

static int dump_adv_cmd(void* handle, int argc, char* argv[])
{
    return CMD_OK;
}

int adv_command_init(void* handle)
{
    return 0;
}

void adv_command_uninit(void* handle)
{
}

int adv_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_adv_tables, ARRAY_SIZE(g_adv_tables), argc, argv, 0);

    if (ret < 0)
        usage();

    return ret;
}
