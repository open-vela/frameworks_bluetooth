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
#ifndef __CS_RAP_H__
#define __CS_RAP_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "bt_addr.h"
#include "bt_cs.h"
#include "bt_status.h"

/**
 * @brief CS RAP data buffer maximum length
 */
#define CS_RAP_STEP_DATA_BUF_LEN    2048

/**
 * @brief CS RAP maximum stored procedures
 */
#define CS_RAP_STORE_PROCEDURE_NUM_MAX  10

/**
 * @brief CS RAP segment header size (1 byte)
 *
 * Segment header format:
 * - Bit 0: First segment flag
 * - Bit 1: Last segment flag
 * - Bits 2-7: Segment index (0-63)
 */
#define CS_RAP_SEG_HEADER_SIZE  1

/**
 * @brief CS RAP sub-procedure header size (12 bytes)
 *
 * Header format:
 * - Ranging Counter (12 bits)
 * - Configuration ID (4 bits)
 * - Selected TX Power (8 bits)
 * - Antenna Paths Mask (8 bits)
 * - Start ACL Connection Event (16 bits)
 * - Frequency Compensation (16 bits)
 * - Procedure Done Status (4 bits)
 * - Subevent Done Status (4 bits)
 * - Procedure Abort Reason (4 bits)
 * - Subevent Abort Reason (4 bits)
 * - Reference Power Level (8 bits)
 * - Number of Steps Reported (8 bits)
 */
#define CS_RAP_SUB_PROCEDURE_HEAD   12

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

/**
 * @brief RAS Features bit definitions
 */
#define RAS_FEATURE_REAL_TIME_DATA          BIT(0)
#define RAS_FEATURE_RETRIEVE_LOST_SEG       BIT(1)
#define RAS_FEATURE_ABORT_OPERATION         BIT(2)
#define RAS_FEATURE_ON_DEMAND_DATA          BIT(24)
#define RAS_FEATURE_DATA_READY              BIT(25)
#define RAS_FEATURE_DATA_OVERWRITTEN        BIT(26)

/**
 * @brief CS RAP ranging data header structure
 */
typedef struct {
    uint16_t ranging_counter;       /**< Ranging counter (lower 12 bits) */
    uint8_t config_id;              /**< CS configuration ID (0-3) */
    int8_t selected_tx_power;       /**< TX power (-127 to 20 dBm) */
    uint8_t antenna_paths_mask;     /**< Antenna paths bitmask */
    uint16_t start_acl_conn_event;  /**< Starting ACL connection event counter */
    int16_t frequency_compensation; /**< Frequency compensation (0.01 ppm) */
    uint8_t procedure_done_status;  /**< Procedure completion status */
    uint8_t subevent_done_status;   /**< Subevent completion status */
    uint8_t procedure_abort_reason; /**< Procedure abort reason */
    uint8_t subevent_abort_reason;  /**< Subevent abort reason */
    int8_t reference_power_level;   /**< Reference power level */
    uint8_t num_steps_reported;     /**< Number of steps reported */
    uint8_t num_antenna_paths;      /**< Number of antenna paths */
} cs_rap_ranging_header_t;

/**
 * @brief CS RAP subevent data structure
 */
typedef struct {
    bool valid;                         /**< Data validity flag */
    uint16_t ranging_counter;           /**< Ranging counter */
    cs_rap_ranging_header_t header;     /**< Ranging data header */
    uint16_t step_data_len;             /**< Step data length */
    uint8_t step_data[];                /**< Step data buffer (variable length) */
} cs_rap_subevent_data_t;

/**
 * @brief CS RAP internal distance result structure (with address)
 *
 * This is used internally by cs_rap module. The public API uses
 * cs_rap_distance_result_t defined in bt_cs.h
 */
typedef struct {
    bt_address_t addr;          /**< Device address */
    uint16_t ranging_counter;   /**< Ranging counter */
    float rtt_distance;         /**< RTT-based distance (meters) */
    float phase_distance;       /**< Phase-based distance (meters) */
    uint8_t mode1_samples;      /**< Mode 1 sample count */
    uint8_t mode2_samples;      /**< Mode 2 sample count */
    bool rtt_valid;             /**< RTT result validity */
    bool phase_valid;           /**< Phase result validity */
} cs_rap_internal_distance_result_t;

/**
 * @brief CS RAP internal distance measurement result callback type
 */
typedef void (*cs_rap_internal_distance_cb)(bt_address_t* addr, cs_rap_internal_distance_result_t* result);

/**
 * @brief Initialize CS RAP module
 *
 * This function initializes the CS RAP module and registers the subevent
 * callback with the CS service to receive local ranging data.
 *
 * @param cb    Distance measurement result callback
 * @return      0 on success, negative on failure
 */
int cs_rap_init(cs_rap_internal_distance_cb cb);

/**
 * @brief Deinitialize CS RAP module
 */
void cs_rap_deinit(void);

/**
 * @brief Set local CS role
 *
 * @param role  Role (CS_ROLE_INITIATOR or CS_ROLE_REFLECTOR)
 */
void cs_rap_set_role(uint8_t role);

/**
 * @brief Get local CS role
 *
 * @return Current role
 */
uint8_t cs_rap_get_role(void);

/**
 * @brief Set ranging mode
 *
 * According to RAS specification, the client enables one of these modes by
 * subscribing to either notifications or indications of the corresponding
 * characteristic (Real-time or On-demand) via CCCD.
 *
 * @param addr  Device address
 * @param mode  Ranging mode (CS_RANGING_MODE_REAL_TIME or CS_RANGING_MODE_ON_DEMAND)
 * @return      bt_status_t
 */
bt_status_t cs_rap_set_ranging_mode(bt_address_t* addr, uint8_t mode);

/**
 * @brief Get current ranging mode
 *
 * @param addr  Device address
 * @return      Current ranging mode
 */
uint8_t cs_rap_get_ranging_mode(bt_address_t* addr);

/**
 * @brief Process remote RAP ranging data
 *
 * Called when RAP GATTC receives ranging data from remote RAS server.
 * This function parses the data and combines it with local data for
 * distance calculation.
 *
 * @param addr  Device address
 * @param data  Raw ranging data
 * @param len   Data length
 * @return      bt_status_t
 */
bt_status_t cs_rap_process_remote_data(bt_address_t* addr, uint8_t* data, uint16_t len);

/**
 * @brief Internal function to retrieve lost segments
 *
 * Called internally when data segment loss is detected.
 * This function sends Retrieve_Lost_Ranging_Data_Segments command
 * to the remote RAS server.
 *
 * @param addr              Device address
 * @param ranging_counter   Ranging counter
 * @param first_seg_idx     First lost segment index
 * @param last_seg_idx      Last lost segment index
 * @return                  bt_status_t
 */
bt_status_t cs_rap_retrieve_lost_segments(bt_address_t* addr, uint16_t ranging_counter,
    uint8_t first_seg_idx, uint8_t last_seg_idx);

/**
 * @brief Clear ranging data
 *
 * @param addr  Device address
 */
void cs_rap_clear_data(bt_address_t* addr);

/**
 * @brief Check if data is ready for distance calculation
 *
 * @param addr  Device address
 * @return      true if ready, false otherwise
 */
bool cs_rap_is_data_ready(bt_address_t* addr);

/**
 * @brief Calculate distance
 *
 * Called when both local and remote data are ready.
 *
 * @param addr  Device address
 * @return      bt_status_t
 */
bt_status_t cs_rap_calculate_distance(bt_address_t* addr);

#endif /* __CS_RAP_H__ */
