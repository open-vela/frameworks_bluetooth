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
#include "bluetooth.h"
#include "bt_cs.h"
#include "bt_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_BLUETOOTH_LE_CS

static int cs_start_distance_measurement_cmd(void* handle, int argc, char* argv[]);
static int cs_stop_distance_measurement_cmd(void* handle, int argc, char* argv[]);
static int cs_set_config_cmd(void* handle, int argc, char* argv[]);
#ifdef CONFIG_BT_CS_RAS_TEST
static int cs_test_cmd(void* handle, int argc, char* argv[]);
#endif

static void* cs_callbacks = NULL;

static bt_command_t g_cs_tables[] = {
    { "start", cs_start_distance_measurement_cmd, 0, "\"start distance measurement :\"" },
    { "stop", cs_stop_distance_measurement_cmd, 0, "\"stop distance measurement :\"" },
    { "config", cs_set_config_cmd, 1, "set CS parameters\n"
                                      "\t  -f or --feature, RAS feature bits (hex or decimal)\n"
                                      "\t      Bit 0 (0x01): Real-time Ranging Data\n"
                                      "\t      Bit 1 (0x02): Retrieve Lost Ranging Data Segments\n"
                                      "\t      Bit 2 (0x04): Abort Operation\n"
                                      "\t      Bit 3 (0x08): Filter Ranging Data\n"
                                      "\t  -r or --role, CS role bits (hex or decimal)\n"
                                      "\t      Bit 0 (0x01): Initiator\n"
                                      "\t      Bit 1 (0x02): Reflector\n"
                                      "\t  -a or --antenna, CS_SYNC antenna selection (hex or decimal)\n"
                                      "\t      0x01 (1): antenna identifier 1\n"
                                      "\t      0x02 (2): antenna identifier 2\n"
                                      "\t      0x03 (3): antenna identifier 3\n"
                                      "\t      0x04 (4): antenna identifier 4\n"
                                      "\t      0xFD (253): repetitive order 0x01 to Num_Antennae_Supported\n"
                                      "\t      0xFE (254): repetitive order 0x01 to 0x04\n"
                                      "\t      0xFF (255): no recommendation\n"
                                      "\t  -p or --power, max TX power in dBm (-127 to 20)\n"
                                      "\t  Examples:\n"
                                      "\t    set -f 0x07\n"
                                      "\t    set -f 0x07 -r 0x01 -a 2 -p 10\n" },
#ifdef CONFIG_BT_CS_RAS_TEST
    { "test", cs_test_cmd, 0, "\"Channel Sounding test mode :\"" },
#endif
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_cs_tables); i++) {
        printf("\t%-8s\t%s\n", g_cs_tables[i].cmd, g_cs_tables[i].help);
    }
}

static void le_cs_distance_measure_started_cb(void* cookie, bt_address_t* addr, uint8_t method)
{
    PRINT("cs distance measure started. cookie:%p, addr:%s, method:%d",
        cookie, bt_addr_bastr(addr), method);
}

static void le_cs_distance_measure_stopped_cb(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method)
{
    PRINT("cs distance measure stopped. cookie:%p, addr:%s, reason:%d, method:%d",
        cookie, bt_addr_bastr(addr), reason, method);
}

static void le_cs_distance_measure_result_cb(void* cookie, bt_address_t* addr, bt_distance_measurement_result_t* result)
{
}

static const cs_callbacks_t le_cs_cbs = {
    sizeof(le_cs_cbs),
    le_cs_distance_measure_started_cb,
    le_cs_distance_measure_stopped_cb,
    le_cs_distance_measure_result_cb,
};

int le_cs_command_init(void* handle)
{
    cs_callbacks = bt_cs_register_callbacks(handle, &le_cs_cbs);
    PRINT("cs command init.");
    return 0;
}

void le_cs_command_uninit(void* handle)
{
    bt_cs_unregister_callbacks(handle, cs_callbacks);
    cs_callbacks = NULL;
}

int le_cs_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table_offset(handle, g_cs_tables, ARRAY_SIZE(g_cs_tables), argc, argv, 0);

    if (ret < 0)
        usage();

    return ret;
}

static int cs_start_distance_measurement_cmd(void* handle, int argc, char* argv[])
{
    bt_distance_measurement_params_t params;

    memset(&params, 0, sizeof(bt_distance_measurement_params_t));
    params.method = METHOD_CS;
    bt_cs_start_distance_measurement(handle, &params);
    return 0;
}

static int cs_stop_distance_measurement_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr = { 0 };
    bt_cs_stop_distance_measurement(handle, &addr, METHOD_CS, false);
    return 0;
}

static int cs_set_config_cmd(void* handle, int argc, char* argv[])
{
    bt_cs_set_params_t params;
    bt_address_t addr = { 0 };
    memset(&params, 0, sizeof(params));

    for (int i = 0; i < argc; i++) {
        if ((strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--feature") == 0) && i + 1 < argc) {
            params.ras_feature = strtoul(argv[++i], NULL, 0);
        } else if ((strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--role") == 0) && i + 1 < argc) {
            params.role = (uint8_t)strtoul(argv[++i], NULL, 0);
        } else if ((strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--antenna") == 0) && i + 1 < argc) {
            params.cs_sync_antenna_selection = (uint8_t)strtoul(argv[++i], NULL, 0);
        } else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--power") == 0) && i + 1 < argc) {
            int power = atoi(argv[++i]);
            if (power < -127 || power > 20) {
                PRINT("error tx power, range must in -127~20");
                return CMD_INVALID_PARAM;
            }
            params.max_tx_power = (int8_t)power;
        }
    }

    PRINT("Setting CS parameters:");
    PRINT("  Feature:              0x%08" PRIx32, params.ras_feature);
    PRINT("    Real-time Ranging Data:              %s", (params.ras_feature & 0x01) ? "Enabled" : "Disabled");
    PRINT("    Retrieve Lost Ranging Data Segments: %s", (params.ras_feature & 0x02) ? "Enabled" : "Disabled");
    PRINT("    Abort Operation:                     %s", (params.ras_feature & 0x04) ? "Enabled" : "Disabled");
    PRINT("    Filter Ranging Data:                 %s", (params.ras_feature & 0x08) ? "Enabled" : "Disabled");
    PRINT("  Role:                 0x%02x", params.role);
    PRINT("    Initiator:                           %s", (params.role & 0x01) ? "Enabled" : "Disabled");
    PRINT("    Reflector:                           %s", (params.role & 0x02) ? "Enabled" : "Disabled");
    PRINT("  Antenna selection:    0x%02x", params.cs_sync_antenna_selection);
    PRINT("  Power:                %d dBm", params.max_tx_power);

    bt_status_t status = bt_cs_set_config(handle, &addr, &params);
    if (status == BT_STATUS_SUCCESS) {
        PRINT("Set CS parameters successfully");
    } else {
        PRINT("Failed to set CS parameters, status: %d", status);
    }

    return 0;
}

#ifdef CONFIG_BT_CS_RAS_TEST
static int cs_test_cmd(void* handle, int argc, char* argv[])
{
    uint8_t data[10] = { 0 };
    bt_cs_test(handle, (void*)data, sizeof(data));
    return 0;
}
#endif /* CONFIG_BT_CS_RAS_TEST */

#endif /* CONFIG_BLUETOOTH_LE_CS */