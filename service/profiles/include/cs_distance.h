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
#ifndef __CS_DISTANCE_H__
#define __CS_DISTANCE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#include "bt_cs.h"

/**
 * @brief Physical constants
 */
#define CS_FREQUENCY_MHZ(ch)        (2402u + 1u * (ch))
#define CS_FREQUENCY_HZ(ch)         (CS_FREQUENCY_MHZ(ch) * 1000000.0f)
#define SPEED_OF_LIGHT_M_PER_S      (299792458.0f)
#define SPEED_OF_LIGHT_NM_PER_S     (SPEED_OF_LIGHT_M_PER_S / 1000000000.0f)
#define CS_PI                       3.14159265358979323846f

/**
 * @brief Maximum number of samples for distance calculation
 */
#define CS_DISTANCE_MAX_SAMPLES     256

/**
 * @brief CS step mode definitions
 */
#define CS_STEP_MODE_0  0
#define CS_STEP_MODE_1  1
#define CS_STEP_MODE_2  2
#define CS_STEP_MODE_3  3

/**
 * @brief IQ sample structure
 */
typedef struct {
    int16_t i;  /**< In-phase component */
    int16_t q;  /**< Quadrature component */
} cs_iq_sample_t;

/**
 * @brief Mode 2 data structure (Phase-based ranging)
 */
typedef struct {
    bool failed;                    /**< Data validity flag */
    uint8_t channel;                /**< Channel index */
    uint8_t antenna_permutation;    /**< Antenna permutation index */
    cs_iq_sample_t local_iq;        /**< Local IQ sample */
    cs_iq_sample_t peer_iq;         /**< Peer IQ sample */
} cs_mode2_sample_t;

/**
 * @brief Mode 1 data structure (RTT-based ranging)
 */
typedef struct {
    bool failed;                /**< Data validity flag */
    int16_t toa_tod_initiator;  /**< Initiator ToA-ToD */
    int16_t tod_toa_reflector;  /**< Reflector ToD-ToA */
} cs_mode1_sample_t;

/**
 * @brief Distance calculation result
 */
typedef struct {
    float rtt_distance;         /**< RTT-based distance in meters */
    float phase_distance;       /**< Phase-based distance in meters */
    uint8_t mode1_samples;      /**< Number of Mode 1 samples used */
    uint8_t mode2_samples;      /**< Number of Mode 2 samples used */
    bool rtt_valid;             /**< RTT result validity */
    bool phase_valid;           /**< Phase result validity */
} cs_distance_result_t;

/**
 * @brief Step data parsing context
 */
typedef struct {
    bool is_local;              /**< Processing local or peer data */
    uint8_t mode1_idx;          /**< Mode 1 data index */
    uint8_t mode2_idx;          /**< Mode 2 data index */
    uint8_t n_ap;               /**< Number of antenna paths */
    uint8_t role;               /**< Local role (Initiator/Reflector) */
    cs_mode1_sample_t* mode1_data;  /**< Mode 1 data buffer */
    cs_mode2_sample_t* mode2_data;  /**< Mode 2 data buffer */
} cs_step_parse_ctx_t;

/**
 * @brief Calculate distance from local and peer step data
 *
 * @param local_data        Local step data
 * @param local_len         Local step data length
 * @param peer_data         Peer step data
 * @param peer_len          Peer step data length
 * @param n_ap              Number of antenna paths
 * @param role              Local role (CS_ROLE_INITIATOR or CS_ROLE_REFLECTOR)
 * @param result            Output distance result
 * @return 0 on success, negative on error
 */
int cs_distance_calculate(const uint8_t* local_data, uint16_t local_len,
    const uint8_t* peer_data, uint16_t peer_len,
    uint8_t n_ap, uint8_t role,
    cs_distance_result_t* result);

#endif /* __CS_DISTANCE_H__ */
