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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_adapter.h"
#include "bt_tools.h"
#include "bt_cs.h"

#ifdef CONFIG_BLUETOOTH_LE_CS

static int cs_start_distance_measurement_cmd(void* handle, int argc, char* argv[]);
static int cs_stop_distance_measurement_cmd(void* handle, int argc, char* argv[]);

static void* cs_callbacks = NULL;

static bt_command_t g_cs_tables[] = {
    { "start", cs_start_distance_measurement_cmd, 0, "\"start distance measurement :\"" },
    { "stop", cs_stop_distance_measurement_cmd, 0, "\"stop distance measurement :\"" },
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

static void le_cs_distance_measure_started_cb(void* cookie,bt_address_t* addr, uint8_t method)
{
    PRINT("cs distance measure started. cookie:%p, addr:%s, method:%d\n",
        cookie, bt_addr_bastr(addr), method);
    return;
}

static void le_cs_distance_measure_stopped_cb(void* cookie,bt_address_t* addr, uint8_t reason, uint8_t method)
{
    PRINT("cs distance measure started. cookie:%p, addr:%s, reason:%d, method:%d\n",
        cookie, bt_addr_bastr(addr), reason, method);
    return;
}

static void le_cs_distance_measure_result_cb(void* cookie,bt_address_t* addr, uint8_t centimeter, uint8_t errorCentimeter,
    uint8_t azimuthAngle, uint8_t errorAzimuthAngle, uint8_t altitudeAngle, uint8_t errorAltitudeAngle,
    long elapsedRealtimeNanos, uint8_t confidenceLevel, double delaySpreadMeters,
    uint8_t detectedAttackLevel, double velocityMetersPerSecond, uint8_t method)
{
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

    memset(&params, 0, sizeof(bt_distance_measurement_params_t));
    params.method = METHOD_CS;
    bt_cs_start_distance_measurement(handle, &params);
    return 0;
}

static int cs_stop_distance_measurement_cmd(void* handle, int argc, char* argv[])
{
    bt_address_t addr = {0};
    bt_cs_stop_distance_measurement(handle, &addr, METHOD_CS, 1000);
    return 0;
}

#endif /* CONFIG_BLUETOOTH_LE_CS */