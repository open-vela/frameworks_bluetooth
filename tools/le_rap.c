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

static int rap_connect_cmd(void* handle, int argc, char* argv[]);
static int rap_disconnect_cmd(void* handle, int argc, char* argv[]);
static int rap_enable_realtime_cmd(void* handle, int argc, char* argv[]);
static int rap_enable_ondemand_cmd(void* handle, int argc, char* argv[]);
static int rap_disable_mode_cmd(void* handle, int argc, char* argv[]);
static int rap_get_data_cmd(void* handle, int argc, char* argv[]);
static int rap_abort_cmd(void* handle, int argc, char* argv[]);
static int rap_set_filter_cmd(void* handle, int argc, char* argv[]);
static int rap_get_state_cmd(void* handle, int argc, char* argv[]);

static void* rap_callbacks = NULL;

static bt_command_t g_rap_tables[] = {
    { "connect", rap_connect_cmd, 0, "\"connect <address>\" - Connect to RAP device" },
    { "disconnect", rap_disconnect_cmd, 0, "\"disconnect <address>\" - Disconnect from RAP device" },
    { "realtime", rap_enable_realtime_cmd, 0, "\"realtime <address>\" - Enable real-time ranging mode" },
    { "ondemand", rap_enable_ondemand_cmd, 0, "\"ondemand <address>\" - Enable on-demand ranging mode" },
    { "disable", rap_disable_mode_cmd, 0, "\"disable <address>\" - Disable ranging mode" },
    { "getdata", rap_get_data_cmd, 0, "\"getdata <address> <counter>\" - Get ranging data" },
    { "abort", rap_abort_cmd, 0, "\"abort <address>\" - Abort current operation" },
    { "filter", rap_set_filter_cmd, 0, "\"filter <address> <mode> <mask>\" - Set ranging data filter" },
    { "state", rap_get_state_cmd, 0, "\"state <address>\" - Get current RAP state" },
};

static void usage(void)
{
    printf("Usage:\n");
    printf("\taddress: peer device address like 00:01:02:03:04:05\n");
    printf("Commands:\n");
    for (int i = 0; i < ARRAY_SIZE(g_rap_tables); i++) {
        printf("\t%-12s\t%s\n", g_rap_tables[i].cmd, g_rap_tables[i].help);
    }
}

static const char* rap_state_to_string(cs_rap_state_t state)
{
    switch (state) {
    case CS_RAP_STATE_DISCONNECTED:
        return "DISCONNECTED";
    case CS_RAP_STATE_CONNECTING:
        return "CONNECTING";
    case CS_RAP_STATE_CONNECTED:
        return "CONNECTED";
    case CS_RAP_STATE_DISCOVERING:
        return "DISCOVERING";
    case CS_RAP_STATE_READY:
        return "READY";
    case CS_RAP_STATE_RANGING:
        return "RANGING";
    default:
        return "UNKNOWN";
    }
}

/* Callback implementations */
static void le_rap_connection_state_cb(void* cookie, bt_address_t* addr, cs_rap_state_t state)
{
    PRINT("RAP connection state changed: addr=%s, state=%s",
        bt_addr_bastr(addr), rap_state_to_string(state));
}

static void le_rap_features_cb(void* cookie, bt_address_t* addr, uint32_t features)
{
    PRINT("RAP features received: addr=%s, features=0x%08lx", bt_addr_bastr(addr), features);
    PRINT("  Real-time Ranging Data: %s", (features & 0x01) ? "Yes" : "No");
    PRINT("  Retrieve Lost Data Seg: %s", (features & 0x02) ? "Yes" : "No");
    PRINT("  Abort Operation: %s", (features & 0x04) ? "Yes" : "No");
    PRINT("  On-demand Ranging Data: %s", (features & (1 << 24)) ? "Yes" : "No");
}

static void le_rap_ranging_data_ready_cb(void* cookie, bt_address_t* addr, uint16_t ranging_counter)
{
    PRINT("RAP data ready: addr=%s, counter=%d", bt_addr_bastr(addr), ranging_counter);
}

static void le_rap_distance_result_cb(void* cookie, bt_address_t* addr, cs_rap_distance_result_t* result)
{
    PRINT("RAP distance measurement result: addr=%s", bt_addr_bastr(addr));
    PRINT("  Ranging Counter: %d", result->ranging_counter);
    if (result->rtt_valid) {
        PRINT("  RTT Distance: %.2f meters (%d samples)", result->rtt_distance, result->mode1_samples);
    }
    if (result->phase_valid) {
        PRINT("  Phase Distance: %.2f meters (%d samples)", result->phase_distance, result->mode2_samples);
    }
    if (!result->rtt_valid && !result->phase_valid) {
        PRINT("  No valid distance estimate available");
    }
}

static const cs_callbacks_t le_rap_cbs = {
    .size = sizeof(le_rap_cbs),
    .cs_distance_measure_started_cb = NULL,
    .cs_distance_measure_stopped_cb = NULL,
    .cs_distance_measure_result_cb = NULL,
    .rap_connection_state_cb = le_rap_connection_state_cb,
    .rap_features_cb = le_rap_features_cb,
    .rap_ranging_data_ready_cb = le_rap_ranging_data_ready_cb,
    .rap_distance_result_cb = le_rap_distance_result_cb,
};

int le_rap_command_init(void* handle)
{
    rap_callbacks = bt_cs_register_callbacks(handle, &le_rap_cbs);
    PRINT("RAP command init.");
    return 0;
}

void le_rap_command_uninit(void* handle)
{
    bt_cs_unregister_callbacks(handle, rap_callbacks);
    rap_callbacks = NULL;
}

int le_rap_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0) {
        ret = execute_command_in_table_offset(handle, g_rap_tables, ARRAY_SIZE(g_rap_tables), argc, argv, 0);
    }

    if (ret < 0) {
        usage();
    }

    return ret;
}

/* Command implementations */
static int rap_connect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1) {
        PRINT("Usage: connect <address>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    bt_status_t status = bt_cs_rap_connect(handle, &addr);
    PRINT("RAP connect: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_disconnect_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1) {
        PRINT("Usage: disconnect <address>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    bt_status_t status = bt_cs_rap_disconnect(handle, &addr);
    PRINT("RAP disconnect: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_enable_realtime_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1) {
        PRINT("Usage: realtime <address>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    bt_status_t status = bt_cs_rap_enable_ranging_mode(handle, &addr, CS_RANGING_MODE_REAL_TIME);
    PRINT("RAP enable real-time mode: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_enable_ondemand_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1) {
        PRINT("Usage: ondemand <address>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    bt_status_t status = bt_cs_rap_enable_ranging_mode(handle, &addr, CS_RANGING_MODE_ON_DEMAND);
    PRINT("RAP enable on-demand mode: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_disable_mode_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1) {
        PRINT("Usage: disable <address>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    bt_status_t status = bt_cs_rap_disable_ranging_mode(handle, &addr);
    PRINT("RAP disable mode: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_get_data_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    uint16_t counter;

    if (argc < 2) {
        PRINT("Usage: getdata <address> <counter>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    counter = (uint16_t)strtoul(argv[1], NULL, 0);

    bt_status_t status = bt_cs_rap_get_ranging_data(handle, &addr, counter);
    PRINT("RAP get data: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_abort_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1) {
        PRINT("Usage: abort <address>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    bt_status_t status = bt_cs_rap_abort_operation(handle, &addr);
    PRINT("RAP abort: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_set_filter_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    cs_filter_config_t config;

    if (argc < 3) {
        PRINT("Usage: filter <address> <mode> <mask>");
        PRINT("  mode: 0-3 (CS mode)");
        PRINT("  mask: filter bit mask (hex)");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    config.mode = (uint8_t)strtoul(argv[1], NULL, 0);
    config.filter_mask = (uint16_t)strtoul(argv[2], NULL, 0);

    bt_status_t status = bt_cs_rap_set_filter(handle, &addr, &config);
    PRINT("RAP set filter: status=%d", status);
    return status == BT_STATUS_SUCCESS ? CMD_OK : CMD_ERROR;
}

static int rap_get_state_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;

    if (argc < 1) {
        PRINT("Usage: state <address>");
        return CMD_PARAM_NOT_ENOUGH;
    }

    if (bt_addr_str2ba(argv[0], &addr) < 0) {
        PRINT("Invalid address format");
        return CMD_INVALID_ADDR;
    }

    cs_rap_state_t state = bt_cs_rap_get_state(handle, &addr);
    PRINT("RAP state: %s (%d)", rap_state_to_string(state), state);
    return CMD_OK;
}
