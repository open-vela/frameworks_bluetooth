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

/**
 * @brief CS Role definitions
 */
typedef enum {
    CS_ROLE_INITIATOR = 0,  /**< CS Initiator role */
    CS_ROLE_REFLECTOR = 1,  /**< CS Reflector role */
} cs_role_t;

/**
 * @brief CS Ranging Mode
 */
typedef enum {
    CS_RANGING_MODE_REAL_TIME = 0x01,  /**< Real-time ranging data mode */
    CS_RANGING_MODE_ON_DEMAND = 0x02,  /**< On-demand ranging data mode */
} cs_ranging_mode_t;

/**
 * @brief CS RAP Connection State
 */
typedef enum {
    CS_RAP_STATE_DISCONNECTED = 0,
    CS_RAP_STATE_CONNECTING,
    CS_RAP_STATE_CONNECTED,
    CS_RAP_STATE_DISCOVERING,
    CS_RAP_STATE_READY,
    CS_RAP_STATE_RANGING,
} cs_rap_state_t;

/**
 * @brief CS Filter Configuration
 */
typedef struct {
    uint8_t mode;           /**< CS mode (0-3) */
    uint16_t filter_mask;   /**< Filter bit mask */
} cs_filter_config_t;

/**
 * @brief CS Configuration Parameters
 */
typedef struct {
    cs_role_t role;                 /**< Local CS role */
    cs_ranging_mode_t ranging_mode; /**< Ranging mode (real-time/on-demand) */
    uint16_t interval_ms;           /**< Ranging interval in milliseconds */
    uint8_t antenna_paths_mask;     /**< Antenna paths bitmask */
} cs_config_t;

/**
 * @brief CS RAP Distance Measurement Result
 */
typedef struct {
    uint16_t ranging_counter;   /**< Ranging counter */
    float rtt_distance;         /**< RTT-based distance in meters */
    float phase_distance;       /**< Phase-based distance in meters */
    uint8_t mode1_samples;      /**< Number of Mode 1 samples used */
    uint8_t mode2_samples;      /**< Number of Mode 2 samples used */
    bool rtt_valid;             /**< RTT result validity */
    bool phase_valid;           /**< Phase result validity */
} cs_rap_distance_result_t;

typedef struct {
    uint32_t centimeter;
    uint32_t error_centimeter;
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
 * @brief CS RAP callback function types
 */
typedef void (*cs_rap_connection_state_cb)(void* cookie, bt_address_t* addr, cs_rap_state_t state);
typedef void (*cs_rap_features_cb)(void* cookie, bt_address_t* addr, uint32_t features);
typedef void (*cs_rap_ranging_data_ready_cb)(void* cookie, bt_address_t* addr, uint16_t ranging_counter);
typedef void (*cs_rap_distance_result_cb)(void* cookie, bt_address_t* addr, cs_rap_distance_result_t* result);

/**
 * @cond
 */
typedef struct {
    bt_address_t addr;
    uint8_t method;
    cs_role_t role;
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
    /* RAP callbacks */
    cs_rap_connection_state_cb rap_connection_state_cb;
    cs_rap_features_cb rap_features_cb;
    cs_rap_ranging_data_ready_cb rap_ranging_data_ready_cb;
    cs_rap_distance_result_cb rap_distance_result_cb;
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
    cs_role_t role; /**< CS role: CS_ROLE_INITIATOR or CS_ROLE_REFLECTOR */
    uint8_t cs_sync_antenna_selection; /**< Antenna selection for CS_SYNC packets, see BT_CS_ANTENNA_SEL_* macros */
    int8_t max_tx_power; /**< Maximum TX power in dBm (-127 to 20) */
    bool is_ras; /**< true: RAS (server side), false: RAP (client side). RAS and RAP are mutually exclusive */
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

/**
 * @brief Connect to a remote RAS server via RAP (Ranging Application Profile).
 *
 * Initiates a BLE connection and discovers the RAS service on the remote device.
 * The connection state is reported via cs_rap_connection_state_cb.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @return      bt_status_t BT_STATUS_SUCCESS on success.
 */
bt_status_t BTSYMBOLS(bt_cs_rap_connect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Disconnect from a remote RAS server.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @return      bt_status_t BT_STATUS_SUCCESS on success.
 */
bt_status_t BTSYMBOLS(bt_cs_rap_disconnect)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Enable a ranging mode on the remote RAS server.
 *
 * Subscribes to the corresponding RAS characteristic (Real-time or On-demand)
 * via CCCD. Only one mode can be active at a time.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @param mode  ranging mode to enable, see @ref cs_ranging_mode_t.
 * @return      bt_status_t BT_STATUS_SUCCESS on success.
 */
bt_status_t BTSYMBOLS(bt_cs_rap_enable_ranging_mode)(bt_instance_t* ins, bt_address_t* addr, cs_ranging_mode_t mode);

/**
 * @brief Disable the current ranging mode.
 *
 * Unsubscribes from the active RAS characteristic notifications.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @return      bt_status_t BT_STATUS_SUCCESS on success.
 */
bt_status_t BTSYMBOLS(bt_cs_rap_disable_ranging_mode)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Request on-demand ranging data from the remote RAS server.
 *
 * Writes a Get_Ranging_Data command to the RAS Control Point.
 * The data is delivered via cs_rap_ranging_data_ready_cb.
 *
 * @param ins              bt instance.
 * @param addr             remote device address.
 * @param ranging_counter  the ranging counter to retrieve.
 * @return                 bt_status_t BT_STATUS_SUCCESS on success.
 */
bt_status_t BTSYMBOLS(bt_cs_rap_get_ranging_data)(bt_instance_t* ins, bt_address_t* addr, uint16_t ranging_counter);

/**
 * @brief Abort the current ranging operation on the remote RAS server.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @return      bt_status_t BT_STATUS_SUCCESS on success.
 */
bt_status_t BTSYMBOLS(bt_cs_rap_abort_operation)(bt_instance_t* ins, bt_address_t* addr);

/**
 * @brief Set a ranging data filter on the remote RAS server.
 *
 * @param ins     bt instance.
 * @param addr    remote device address.
 * @param config  filter configuration, see @ref cs_filter_config_t.
 * @return        bt_status_t BT_STATUS_SUCCESS on success.
 */
bt_status_t BTSYMBOLS(bt_cs_rap_set_filter)(bt_instance_t* ins, bt_address_t* addr, const cs_filter_config_t* config);

/**
 * @brief Get the current RAP connection state.
 *
 * @param ins   bt instance.
 * @param addr  remote device address.
 * @return      current RAP connection state, see @ref cs_rap_state_t.
 */
cs_rap_state_t BTSYMBOLS(bt_cs_rap_get_state)(bt_instance_t* ins, bt_address_t* addr);

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
