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

#include <stdbool.h>
#include <stddef.h>

#include "bluetooth.h"
#include "bt_addr.h"

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

#define MAX_TEST_DATA 128

typedef struct {
    uint8_t centimeter;
    uint8_t error_centimeter;
    uint8_t azimuth_angle;
    uint8_t error_azimuthAngle;
    uint8_t altitude_angle;
    uint8_t error_altitudeAngle;
    long elapsed_realtime_nanos;
    uint8_t confidence_level;
    double delay_spread_meters;
    uint8_t detected_attack_level;
    double velocity_meters_persecond;
    uint8_t method;
} bt_distance_measurement_result_t;

typedef void (*cs_distance_measure_started_cb)(void* cookie, bt_address_t* addr, uint8_t method);
typedef void (*cs_distance_measure_stopped_cb)(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method);
typedef void (*cs_distance_measure_result_cb)(void* cookie, bt_address_t* addr, bt_distance_measurement_result_t* result);

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

/**
 * @brief register cs event callback
 *
 * register cs event callback.
 *
 * @param ins   bt instance.
 * @param callbacks  cs event callback function.
 * @return          cookie of cs event callback.
 */
void* BTSYMBOLS(bt_cs_register_callbacks)(bt_instance_t* ins, const cs_callbacks_t* callbacks);

/**
 * @brief unregister cs event callback
 *
 * unregister cs event callback.
 *
 * @param ins   bt instance.
 * @param cookie    cookie of cs event callback.
 * @return          true if success, false otherwise.
 */
bool BTSYMBOLS(bt_cs_unregister_callbacks)(bt_instance_t* ins, void* cookie);

/**
 * @brief start distance measurement
 *
 * start distance measurement.
 *
 * @param ins   bt instance.
 * @param params    distance measurement parameters.
 * @return          bt_status_t.
 */
bt_status_t BTSYMBOLS(bt_cs_start_distance_measurement)(bt_instance_t* ins, const bt_distance_measurement_params_t* params);

/**
 * @brief stop distance measurement
 *
 * stop distance measurement.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @param method    method of distance measurement.
 * @param timeout   timeout flag.
 * @return          bt_status_t.
 */
bt_status_t BTSYMBOLS(bt_cs_stop_distance_measurement)(bt_instance_t* ins, bt_address_t* addr, uint8_t method, bool timeout);

/**
 * @brief get max supported security level
 *
 * get max supported security level of remote device.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @return          bt_status_t.
 */
bt_status_t BTSYMBOLS(bt_get_cs_max_supported_security_level)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief RAS feature bits for ras_feature field
 */
#define BT_CS_RAS_REAL_TIME_RANGING_DATA 0x01 /**< Bit 0: Real-time Ranging Data */
#define BT_CS_RAS_RETRIEVE_LOST_DATA_SEGMENTS 0x02 /**< Bit 1: Retrieve Lost Ranging Data Segments */
#define BT_CS_RAS_ABORT_OPERATION 0x04 /**< Bit 2: Abort Operation */
#define BT_CS_RAS_FILTER_RANGING_DATA 0x08 /**< Bit 3: Filter Ranging Data */

/**
 * @brief CS_SYNC antenna selection values
 */
#define BT_CS_ANTENNA_SEL_1 0x01 /**< Use antenna identifier 1 */
#define BT_CS_ANTENNA_SEL_2 0x02 /**< Use antenna identifier 2 */
#define BT_CS_ANTENNA_SEL_3 0x03 /**< Use antenna identifier 3 */
#define BT_CS_ANTENNA_SEL_4 0x04 /**< Use antenna identifier 4 */
#define BT_CS_ANTENNA_SEL_SINGLE_REPEATE 0xFD /**< Antenna identifiers in repetitive order (0x01, 0x01, ..., Num_Antennae_Supported, Num_Antennae_Supported) */
#define BT_CS_ANTENNA_SEL_DOUBLE_REPEATE 0xFE /**< Antenna identifiers in repetitive order from 0x01 to Num_Antennae_Supported */
#define BT_CS_ANTENNA_SEL_NO_RECOMMEND 0xFF /**< Host does not have a recommendation */

/**
 * @brief CS configuration parameters for set command
 */
typedef struct {
    uint32_t ras_feature; /**< RAS feature bits: Bit 0 (0x01): Real-time Ranging Data,
                               Bit 1 (0x02): Retrieve Lost Ranging Data Segments,
                               Bit 2 (0x04): Abort Operation,
                               Bit 3 (0x08): Filter Ranging Data */
    uint8_t role; /**< CS role bits: Bit 0 (0x01): initiator, Bit 1 (0x02): reflector */
    uint8_t cs_sync_antenna_selection; /**< Antenna selection for CS_SYNC packets, see BT_CS_ANTENNA_SEL_* macros */
    int8_t max_tx_power; /**< Maximum TX power in dBm (-127 to 20) */
} bt_cs_set_params_t;

/**
 * @brief set CS configuration
 *
 * set CS configuration including RAS feature and default settings.
 *
 * @param ins     bt instance.
 * @param addr    remote device address.
 * @param params  CS configuration parameters, see @ref bt_cs_set_params_t.
 * @return        bt_status_t.
 */
bt_status_t BTSYMBOLS(bt_cs_set_config)(bt_instance_t* ins, bt_address_t* addr, const bt_cs_set_params_t* params);

#ifdef CONFIG_BT_CS_RAS_TEST
/**
 * @brief test function
 *
 * test function.
 *
 * @param ins   bt instance.
 * @param data  test subevent result data.
 * @param len   length of test data.
 * @return          bt_status_t.
 * @note            only for test.
 */
bt_status_t BTSYMBOLS(bt_cs_test)(bt_instance_t* ins, const void* data, uint16_t len);
#endif /* CONFIG_BT_CS_RAS_TEST */

#endif /* __BT_CS_H__ */
