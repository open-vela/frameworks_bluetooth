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
// #include "bt_internal.h"
#include <stddef.h>

#ifndef BTSYMBOLS
#define BTSYMBOLS(s) s
#endif

typedef void (*cs_started_cb)(void* cookie, bt_address_t* addr, uint8_t method);
typedef void (*cs_stopped_cb)(void* cookie, bt_address_t* addr, uint8_t reason, uint8_t method);
typedef void (*cs_result_cb)(void* cookie, bt_address_t* addr, uint8_t centimeter, uint8_t error_centimeter,
    uint8_t azimuth_angle, uint8_t errorazimuth_angle, uint8_t altitude_angle, uint8_t erroraltitude_angle,
    uint16_t elapsed_realtime_nanos, uint8_t confidence_level, uint32_t delay_spread_meters,
    uint8_t detected_attack_level, uint32_t velocity_meters_per_second, uint8_t method);

/**
 * @cond
 */

typedef struct {
    bt_address_t addr;
    uint8_t method; // 0: AUTO, 1: RSSI, 2: CS.
    uint8_t mode; // 0: Real-Time, 1: On-Demand.
    uint8_t role; // 0: initiator, 1: responder.
    uint16_t interval_ms; // Gap between the start of two consecutive CS subevents (only used for Real-Time mode).
    uint16_t duration_ms; // Max. number of connection events between consecutive CS procedures (0x0001 to 0xFFFF).
    uint8_t mainMode; // 1: RTT, 2: PBR, 3: PBR+RTT.
    uint8_t submode; // 0: UNUSED, 1: RTT, 2: PBR, 3: PBR+RTT.
    uint8_t min_steps; // Minimum number of CS main mode steps to be executed before a submode step is executed.
    uint8_t max_steps; // Maximum number of CS main mode steps to be executed before a submode step is executed.
    uint8_t repetition; // Number of main mode steps taken from the end of the last CS subevent to be repeated at the beginning of the current CS subevent directly after the last mode-0 step of that event
    uint8_t mode0_steps; // Indicates the number of mode-0 CS steps to be includedat the beginning of each CS subevent.
    uint8_t rtt_type; // 0: AA, 1: 32-bit sounding sequence, 2: 96-bit sounding sequence, 3: 32-bit random sequence, 4: 64-bit random sequence, 5: 96-bit random sequence, 6: 128-bit random sequence.
    uint8_t sync_phy; // 1: 1M-phy, 2: 2M-phy, 3: 3M-phy.
    uint8_t channel_map; // Indicates the channels to be used or unused during the CS procedure.
    uint8_t channelSelectionType; // 0: 3B, 1: 3C.
    uint8_t ch3cShape; // 0: HAT, 1: X.
    uint8_t ch3cJump; // Number of channels skipped in each rising and falling sequence.
    uint8_t antenna_paths_mask; // Bit0: 1 if Antenna Path_1 included; 0 if not.Bit1: 1 if Antenna Path_2 included; 0 if not.Bit2: 1 if Antenna Path_3 included; 0 if not.Bit3: 1 if Antenna Path_4 included; 0 if not.
    uint8_t preferredNumAntennas;
    uint8_t vendor_specific;
    uint8_t debug_flags;
} bt_distance_measurement_params_t;

typedef struct {
    size_t size;
    cs_started_cb cs_started_cb;
    cs_stopped_cb cs_stopped_cb;
    cs_result_cb cs_result_cb;
} cs_callbacks_t;

typedef enum {
    METHOD_AUTO,
    METHOD_RSSI,
    METHOD_CS,
} cs_method_t;

typedef enum {
    CS_STOPPED,
    CS_STARTED,
} cs_state_t;

/**
 * @endcond
 */
void* BTSYMBOLS(bt_cs_register_callbacks)(bt_instance_t* ins, const cs_callbacks_t* callbacks);
bool BTSYMBOLS(bt_cs_unregister_callbacks)(bt_instance_t* ins, void* cookie);
bt_status_t BTSYMBOLS(bt_cs_start_distance_measurement)(bt_instance_t* ins, bt_distance_measurement_params_t* params);
bt_status_t BTSYMBOLS(bt_cs_stop_distance_measurement)(bt_instance_t* ins, bt_address_t* addr, uint8_t method, bool timeout);
bt_status_t BTSYMBOLS(bt_get_cs_max_supported_security_level)(bt_instance_t* ins, bt_address_t* addr);
bt_status_t BTSYMBOLS(bt_cs_test)(bt_instance_t* ins, void* data, uint16_t len);
cs_state_t BTSYMBOLS(bt_cs_get_state)(bt_instance_t* ins, bt_address_t* addr);

#endif /* __BT_CS_H__ */
