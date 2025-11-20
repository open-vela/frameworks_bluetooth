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
#include "bt_config.h"
#include "bt_cs.h"
#include "bt_device.h"
#include "bt_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_BLUETOOTH_LE_CS

#define START_USAGE "start cs\n"                                                                                                                                                                                                          \
                    "\t\t\t- <addr>\n"                                                                                                                                                                                                    \
                    "\t\t\t- <method> 0: AUTO, 1: RSSI, 2: CS.\n"                                                                                                                                                                         \
                    "\t\t\t- <mode> 0: Real-Time, 1: On-Demand.\n"                                                                                                                                                                        \
                    "\t\t\t- <role> 0: initiator, 1: responder.\n"                                                                                                                                                                        \
                    "\t\t\t- <interval_ms> Gap between the start of two consecutive CS subevents (only used for Real-Time mode).\n"                                                                                                       \
                    "\t\t\t- <duration_ms> Max. number of connection events between consecutive CS procedures (0x0001 to 0xFFFF).\n"                                                                                                      \
                    "\t\t\t- <mainMode> 1: RTT, 2: PBR, 3: PBR+RTT.\n"                                                                                                                                                                    \
                    "\t\t\t- <submode> 0: UNUSED, 1: RTT, 2: PBR, 3: PBR+RTT.\n"                                                                                                                                                          \
                    "\t\t\t- <min_steps> Minimum number of CS main mode steps to be executed before a submode step is executed.\n"                                                                                                        \
                    "\t\t\t- <max_steps> Maximum number of CS main mode steps to be executed before a submode step is executed.\n"                                                                                                        \
                    "\t\t\t- <repetition> Number of main mode steps taken from the end of the last CS subevent to be repeated at the beginning of the current CS subevent directly after the last mode-0 step of that event\n"            \
                    "\t\t\t- <mode0_steps> Indicates the number of mode-0 CS steps to be includedat the beginning of each CS subevent.\n"                                                                                                 \
                    "\t\t\t- <rtt_type> 0: AA, 1: 32-bit sounding sequence, 2: 96-bit sounding sequence, 3: 32-bit random sequence, 4: 64-bit random sequence, 5: 96-bit random sequence, 6: 128-bit random sequence.\n"                  \
                    "\t\t\t- <sync_phy> 1: 1M-phy, 2: 2M-phy, 3: 3M-phy.\n"                                                                                                                                                               \
                    "\t\t\t- <channel_map> Indicates the channels to be used or unused during theCS procedure.\n"                                                                                                                         \
                    "\t\t\t- <channelSelectionType> 0: 3B, 1: 3C.\n"                                                                                                                                                                      \
                    "\t\t\t- <ch3cShape> 0: HAT, 1: X.\n"                                                                                                                                                                                 \
                    "\t\t\t- <ch3cJump> Number of channels skipped in each rising and falling sequence.\n"                                                                                                                                \
                    "\t\t\t- <antenna_paths_mask> Bit0: 1 if Antenna Path_1 included; 0 if not.Bit1: 1 if Antenna Path_2 included; 0 if not.Bit2: 1 if Antenna Path_3 included; 0 if not.Bit3: 1 if Antenna Path_4 included; 0 if not.\n" \
                    "\t\t\t- <preferredNumAntennas>\n"                                                                                                                                                                                    \
                    "\t\t\t- <vendor_specific>\n"                                                                                                                                                                                         \
                    "\t\t\t- <debug_flags>\n"
#define STOP_USAGE "stop cs\n"                                   \
                   "\t\t\t- <addr>\n"                            \
                   "\t\t\t- <method> 0: AUTO, 1: RSSI, 2: CS.\n" \
                   "\t\t\t- <timeout> timeout of stop\n"

static int cs_start_distance_measurement_cmd(void* handle, int argc, char* argv[]);
static int cs_stop_distance_measurement_cmd(void* handle, int argc, char* argv[]);
static int cs_test_cmd(void* handle, int argc, char* argv[]);
static int cs_get_state_cmd(void* handle, int argc, char* argv[]);

static void* cs_callbacks = NULL;

static bt_command_t g_cs_tables[] = {
    { "start", cs_start_distance_measurement_cmd, 0, START_USAGE },
    { "stop", cs_stop_distance_measurement_cmd, 0, STOP_USAGE },
    { "test", cs_test_cmd, 0, "\"Channel Sounding test mode \"" },
    { "state", cs_get_state_cmd, 0, "\"get cs state\"" },
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
    PRINT("cs distance measure started. cookie:%p, addr:%s, method:%d\n",
        cookie, bt_addr_bastr(addr), method);
    return;
}

static void le_cs_distance_measure_stopped_cb(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method)
{
    PRINT("cs distance measure started. cookie:%p, addr:%s, reason:%d, method:%d\n",
        cookie, bt_addr_bastr(addr), reason, method);
    return;
}

static void le_cs_distance_measure_result_cb(void* cookie, bt_address_t* addr, uint8_t centimeter, uint8_t errorCentimeter,
    uint8_t azimuthAngle, uint8_t errorAzimuthAngle, uint8_t altitudeAngle, uint8_t errorAltitudeAngle,
    uint16_t elapsedRealtimeNanos, uint8_t confidenceLevel, uint32_t delaySpreadMeters,
    uint8_t detectedAttackLevel, uint32_t velocityMetersPerSecond, uint8_t method)
{
    PRINT("cs distance measure result. cookie:%p, addr:%s, centimeter:%d, errorCentimeter:%d, azimuthAngle:%d, errorAzimuthAngle:%d, altitudeAngle:%d, errorAltitudeAngle:%d, elapsedRealtimeNanos:%" PRIu16 ", confidenceLevel:%d, delaySpreadMeters:%" PRIu32 ", detectedAttackLevel:%d, velocityMetersPerSecond:%" PRIu32 ", method:%d\n",
        cookie, bt_addr_bastr(addr), centimeter, errorCentimeter, azimuthAngle, errorAzimuthAngle, altitudeAngle, errorAltitudeAngle,
        elapsedRealtimeNanos, confidenceLevel, delaySpreadMeters, detectedAttackLevel, velocityMetersPerSecond, method);

    return;
}

static const cs_callbacks_t le_cs_cbs = {
    sizeof(le_cs_cbs),
    le_cs_distance_measure_started_cb,
    le_cs_distance_measure_stopped_cb,
    le_cs_distance_measure_result_cb,
};

int le_cs_commond_init(void* handle)
{
    cs_callbacks = bt_cs_register_callbacks(handle, &le_cs_cbs);
    PRINT("cs command init.");
    return 0;
}

int le_cs_commond_uninit(void* handle)
{
    bt_cs_unregister_callbacks(handle, cs_callbacks);

    return 0;
}

int le_cs_command_exec(void* handle, int argc, char* argv[])
{
    int ret = CMD_USAGE_FAULT;

    if (argc > 0)
        ret = execute_command_in_table(handle, g_cs_tables, ARRAY_SIZE(g_cs_tables), argc, argv);

    if (ret < 0)
        usage();

    return ret;
}

static int cs_start_distance_measurement_cmd(void* handle, int argc, char* argv[])
{
    bt_distance_measurement_params_t params;

    if (argc < 22)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &(params.addr)) < 0)
        return CMD_INVALID_ADDR;

    params.method = atoi(argv[1]);
    params.mode = atoi(argv[2]);
    params.role = atoi(argv[3]);
    params.interval_ms = atoi(argv[4]);
    params.duration_ms = atoi(argv[5]);
    params.mainMode = atoi(argv[6]);
    params.submode = atoi(argv[7]);
    params.min_steps = atoi(argv[8]);
    params.max_steps = atoi(argv[9]);
    params.repetition = atoi(argv[10]);
    params.mode0_steps = atoi(argv[11]);
    params.rtt_type = atoi(argv[12]);
    params.sync_phy = atoi(argv[13]);
    params.channel_map = atoi(argv[14]);
    params.channelSelectionType = atoi(argv[15]);
    params.ch3cShape = atoi(argv[16]);
    params.ch3cJump = atoi(argv[17]);
    params.antenna_paths_mask = atoi(argv[18]);
    params.preferredNumAntennas = atoi(argv[19]);
    params.vendor_specific = atoi(argv[20]);
    params.debug_flags = atoi(argv[21]);

    bt_cs_start_distance_measurement(handle, &params);
    return 0;
}

static int cs_stop_distance_measurement_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr;
    if (argc < 3)
        return CMD_PARAM_NOT_ENOUGH;

    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    bt_cs_stop_distance_measurement(handle, &addr, atoi(argv[1]), atoi(argv[2]));
    return 0;
}

static int cs_test_cmd(void* handle, int argc, char* argv[])
{
    uint8_t data[10] = { 0 };
    bt_cs_test(handle, (void*)data, sizeof(data));
    return 0;
}

static int cs_get_state_cmd(void* handle, int argc, char* argv[])
{
    if (argc < 1)
        return CMD_PARAM_NOT_ENOUGH;

    bt_address_t addr;
    if (bt_addr_str2ba(argv[0], &addr) < 0)
        return CMD_INVALID_ADDR;

    int state = bt_cs_get_state(handle, &addr);
    PRINT("cs state: %d", state);

    return CMD_OK;
}
#endif /* CONFIG_BLUETOOTH_LE_CS */