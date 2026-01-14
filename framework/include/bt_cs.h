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
#ifndef __BT_CS_H__
#define __BT_CS_H__

#include "bt_addr.h"
#include "bt_device.h"
#include "bt_internal.h"
#include <stddef.h>

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

typedef void (*cs_distance_measure_started_cb)(void* cookie, bt_address_t* addr, uint8_t method);
typedef void (*cs_distance_measure_stopped_cb)(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method);
typedef void (*cs_distance_measure_result_cb)(void* cookie, bt_address_t* addr, uint8_t centimeter, uint8_t errorCentimeter,
    uint8_t azimuthAngle, uint8_t errorAzimuthAngle, uint8_t altitudeAngle, uint8_t errorAltitudeAngle,
    long elapsedRealtimeNanos, uint8_t confidenceLevel, double delaySpreadMeters,
    uint8_t detectedAttackLevel, double velocityMetersPerSecond, uint8_t method);

/**
 * @cond
 */
typedef struct {
    bt_address_t addr;
    uint8_t method;
    uint8_t role;
    uint16_t interval_ms;
    uint16_t duration_ms;
    uint8_t submode;
    uint8_t max_steps;
    uint8_t mode0_steps;
    uint8_t rtt_type;
    uint8_t sync_phy;
    uint8_t channel_map;
    uint8_t antenna_paths_mask;
    uint8_t vendor_specific;
    uint8_t debug_flags;
} bt_distance_measurement_params_t;

typedef struct {
    size_t size;
    cs_distance_measure_started_cb cs_distance_measure_started_cb;
    cs_distance_measure_stopped_cb cs_distance_measure_stopped_cb;
    cs_distance_measure_result_cb cs_distance_measure_result_cb;
} cs_callbacks_t;

typedef enum {
    METHOD_AUTO,
    METHOD_RSSI,
    METHOD_CS,
} cs_method_t;

/**
 * @endcond
 */
void* BTSYMBOLS(bt_cs_register_callbacks)(bt_instance_t* ins, const cs_callbacks_t* callbacks);
bool BTSYMBOLS(bt_cs_unregister_callbacks)(bt_instance_t* ins, void* cookie);
bt_status_t BTSYMBOLS(bt_cs_start_distance_measurement)(bt_instance_t* ins, bt_distance_measurement_params_t* params);
bt_status_t BTSYMBOLS(bt_cs_stop_distance_measurement)(bt_instance_t* ins, bt_address_t* addr, uint8_t method, bool timeout);
bt_status_t BTSYMBOLS(bt_get_cs_max_supported_security_level)(bt_instance_t* ins, bt_address_t* addr);
bt_status_t BTSYMBOLS(bt_cs_test)(bt_instance_t* ins, void* data, uint16_t len);

#endif /* __BT_CS_H__ */
