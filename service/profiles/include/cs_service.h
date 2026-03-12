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
#ifndef __CS_SERVICE_H__
#define __CS_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bt_cs.h"
#include "cs_msg.h"

typedef enum {
    CS_BT_SRV_CONN_LE_CS_PROCEDURES_DISABLED,
    CS_BT_SRV_CONN_LE_CS_PROCEDURES_ENABLED,
} bt_srv_conn_le_cs_procedure_enable_state_t;

/** CS Test Tone Antennna Config Selection.
 *
 *  These enum values are indices in the following table, where N_AP is the maximum
 *  number of antenna paths (in the range [1, 4]).
 *
 * +--------------+-------------+-------------------+-------------------+--------+
 * | Config Index | Total Paths | Dev A: # Antennas | Dev B: # Antennas | Config |
 * +--------------+-------------+-------------------+-------------------+--------+
 * |            0 |           1 |                 1 |                 1 | 1:1    |
 * |            1 |           2 |                 2 |                 1 | N_AP:1 |
 * |            2 |           3 |                 3 |                 1 | N_AP:1 |
 * |            3 |           4 |                 4 |                 1 | N_AP:1 |
 * |            4 |           2 |                 1 |                 2 | 1:N_AP |
 * |            5 |           3 |                 1 |                 3 | 1:N_AP |
 * |            6 |           4 |                 1 |                 4 | 1:N_AP |
 * |            7 |           4 |                 2 |                 2 | 2:2    |
 * +--------------+-------------+-------------------+-------------------+--------+
 *
 *  There are therefore four groups of possible antenna configurations:
 *
 *  - 1:1 configuration, where both A and B support 1 antenna each
 *  - 1:N_AP configuration, where A supports 1 antenna, B supports N_AP antennas, and
 *    N_AP is a value in the range [2, 4]
 *  - N_AP:1 configuration, where A supports N_AP antennas, B supports 1 antenna, and
 *    N_AP is a value in the range [2, 4]
 *  - 2:2 configuration, where both A and B support 2 antennas and N_AP = 4
 */
typedef enum {
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ZERO,
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_ONE,
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_TWO,
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_THREE,
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FOUR,
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_FIVE,
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SIX,
    CS_BT_SRV_LE_CS_TONE_ANTENNA_CONFIGURATION_INDEX_SEVEN,
} bt_srv_conn_le_cs_tone_antenna_config_selection_t;

/** Channel sounding main mode */
typedef enum {
    /** Mode-1 (RTT) */
    CS_BT_SRV_CONN_LE_CS_MAIN_MODE_1 = 0x01,
    /** Mode-2 (PBR) */
    CS_BT_SRV_CONN_LE_CS_MAIN_MODE_2,
    /** Mode-3 (RTT and PBR) */
    CS_BT_SRV_CONN_LE_CS_MAIN_MODE_3,
} bt_srv_conn_le_cs_main_mode_t;

/** Channel sounding sub mode */
typedef enum {
    /** Mode-1 (RTT) */
    CS_BT_SRV_CONN_LE_CS_SUB_MODE_1 = 0x01,
    /** Mode-2 (PBR) */
    CS_BT_SRV_CONN_LE_CS_SUB_MODE_2,
    /** Mode-3 (RTT and PBR) */
    CS_BT_SRV_CONN_LE_CS_SUB_MODE_3,
    /** Unused */
    CS_BT_SRV_CONN_LE_CS_SUB_MODE_UNUSED = 0xFF,
} bt_srv_conn_le_cs_sub_mode_t;

/** Channel sounding role */
typedef enum {
    /** CS initiator role */
    CS_BT_SRV_CONN_LE_CS_ROLE_INITIATOR,
    /** CS reflector role */
    CS_BT_SRV_CONN_LE_CS_ROLE_REFLECTOR,
} bt_srv_conn_le_cs_role_t;

/** Channel sounding RTT type */
typedef enum {
    /** RTT AA only */
    CS_BT_SRV_CONN_LE_CS_RTT_TYPE_AA_ONLY,
    /** RTT with 32-bit sounding sequence */
    CS_BT_SRV_CONN_LE_CS_RTT_TYPE_32_BIT_SOUNDING,
    /** RTT with 96-bit sounding sequence */
    CS_BT_SRV_CONN_LE_CS_RTT_TYPE_96_BIT_SOUNDING,
    /** RTT with 32-bit random sequence */
    CS_BT_SRV_CONN_LE_CS_RTT_TYPE_32_BIT_RANDOM,
    /** RTT with 64-bit random sequence */
    CS_BT_SRV_CONN_LE_CS_RTT_TYPE_64_BIT_RANDOM,
    /** RTT with 96-bit random sequence */
    CS_BT_SRV_CONN_LE_CS_RTT_TYPE_96_BIT_RANDOM,
    /** RTT with 128-bit random sequence */
    CS_BT_SRV_CONN_LE_CS_RTT_TYPE_128_BIT_RANDOM,
} bt_srv_conn_le_cs_rtt_type_t;

/** Channel sounding PHY used for CS sync */
typedef enum {
    /** LE 1M PHY */
    CS_BT_SRV_CONN_LE_CS_SYNC_1M_PHY = 0x01,
    /** LE 2M PHY */
    CS_BT_SRV_CONN_LE_CS_SYNC_2M_PHY,
    /** LE 2M 2BT PHY */
    CS_BT_SRV_CONN_LE_CS_SYNC_2M_2BT_PHY,
} bt_srv_conn_le_cs_sync_phy_t;

/** Channel sounding channel selection type */
typedef enum {
    /** Use Channel Selection Algorithm #3b for non-mode-0 CS steps */
    CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3B,
    /** Use Channel Selection Algorithm #3c for non-mode-0 CS steps */
    CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3C,
} bt_srv_conn_le_cs_chsel_type_t;

/** Channel sounding channel sequence shape */
typedef enum {
    /** Use Hat shape for user-specified channel sequence */
    CS_BT_SRV_CONN_LE_CS_CH3C_SHAPE_HAT,
    /** Use X shape for user-specified channel sequence */
    CS_BT_SRV_CONN_LE_CS_CH3C_SHAPE_X,
} bt_srv_conn_le_cs_ch3c_shape_t;

/** Supported AA-Only RTT precision. */
typedef enum {
    /** AA-Only RTT variant is not supported. */
    CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_NOT_SUPP = 0,
    /** 10ns time-of-flight accuracy. */
    CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_10NS,
    /** 150ns time-of-flight accuracy. */
    CS_BT_SRV_CONN_LE_CS_RTT_AA_ONLY_150NS,
} bt_srv_conn_le_cs_capability_rtt_aa_only_t;

/** Supported Sounding Sequence RTT precision. */
typedef enum {
    /** Sounding Sequence RTT variant is not supported. */
    CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_NOT_SUPP = 0,
    /** 10ns time-of-flight accuracy. */
    CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_10NS,
    /** 150ns time-of-flight accuracy. */
    CS_BT_SRV_CONN_LE_CS_RTT_SOUNDING_150NS,
} bt_srv_conn_le_cs_capability_rtt_sounding_t;

/** Supported Random Payload RTT precision. */
typedef enum {
    /** Random Payload RTT variant is not supported. */
    CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_NOT_SUPP = 0,
    /** 10ns time-of-flight accuracy. */
    CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_10NS,
    /** 150ns time-of-flight accuracy. */
    CS_BT_SRV_CONN_LE_CS_RTT_RANDOM_PAYLOAD_150NS,
} bt_srv_conn_le_cs_capability_rtt_random_payload_t;

typedef enum {
    /** Use antenna identifier 1 for CS_SYNC packets. */
    BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_ONE = 0x01,
    /** Use antenna identifier 2 for CS_SYNC packets. */
    BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_TWO = 0x02,
    /** Use antenna identifier 3 for CS_SYNC packets. */
    BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_THREE = 0x03,
    /** Use antenna identifier 4 for CS_SYNC packets. */
    BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_FOUR = 0x04,
    /** Use antennas in repetitive order from 0x01 to Num_Antennae_Supported for CS_SYNC packets. */
    BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_REPETITIVE_1_TO_N = 0xFD,
    /** Use antennas in repetitive order from 0x01 to 0x04 for CS_SYNC packets. */
    BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_REPETITIVE_1_TO_4 = 0xFE,
    /** No recommendation for local controller antenna selection. */
    BT_LE_SRV_CS_ANTENNA_SELECTION_OPT_NO_RECOMMENDATION = 0xFF,
} bt_le_srv_cs_sync_antenna_selection_t;

/** CS Test Initiator SNR control options */
typedef enum {
    BT_LE_SRV_CS_SNR_CONTROL_18dB,
    BT_LE_SRV_CS_SNR_CONTROL_21dB,
    BT_LE_SRV_CS_SNR_CONTROL_24dB,
    BT_LE_SRV_CS_SNR_CONTROL_27dB,
    BT_LE_SRV_CS_SNR_CONTROL_30dB,
    BT_LE_SRV_CS_SNR_CONTROL_NOT_USED = 0xFF,
} bt_le_srv_cs_snr_control_t;

typedef enum {
    BT_LE_SRV_CS_PROCEDURE_PHY_1M = 0x01,
    BT_LE_SRV_CS_PROCEDURE_PHY_2M,
    BT_LE_SRV_CS_PROCEDURE_PHY_CODED_S8,
    BT_LE_SRV_CS_PROCEDURE_PHY_CODED_S2,
} bt_le_srv_cs_procedure_phy_t;

/** CS config creation context */
typedef enum {
    /** Write CS configuration in local Controller only  */
    BT_LE_SRV_CS_CREATE_CONFIG_CONTEXT_LOCAL_ONLY,
    /** Write CS configuration in both local and remote Controller using Channel Sounding
     * Configuration procedure
     */
    BT_LE_SRV_CS_CREATE_CONFIG_CONTEXT_LOCAL_AND_REMOTE,
} bt_le_srv_cs_create_config_context_t;

/** Procedure done status */
typedef enum {
    BT_LE_SRV_CS_PROCEDURE_COMPLETE = 0x0,
    BT_LE_SRV_CS_PROCEDURE_INCOMPLETE = 0x1,
    BT_LE_SRV_CS_PROCEDURE_ABORTED = 0xF,
} bt_le_srv_cs_procedure_done_status_t;

/** Subevent done status */
typedef enum {
    BT_LE_SRV_CS_SUBEVENT_COMPLETE = 0x0,
    BT_LE_SRV_CS_SUBEVENT_INCOMPLETE = 0x1,
    BT_LE_SRV_CS_SUBEVENT_ABORTED = 0x2,
} bt_le_srv_le_cs_subevent_done_status_t;

/** Procedure abort reason */
typedef enum {
    BT_LE_SRV_CS_PROCEDURE_NOT_ABORTED = 0x0,
    BT_LE_SRV_CS_PROCEDURE_ABORT_REQUESTED = 0x1,
    BT_LE_SRV_CS_PROCEDURE_ABORT_TOO_FEW_CHANNELS = 0x2,
    BT_LE_SRV_CS_PROCEDURE_ABORT_CHMAP_INSTANT_PASSED = 0x3,
    BT_LE_SRV_CS_PROCEDURE_ABORT_UNSPECIFIED = 0xF,
} bt_le_srv_cs_procedure_abort_reason_t;

/** Subevent abort reason */
typedef enum {
    BT_LE_SRV_CS_SUBEVENT_NOT_ABORTED = 0x0,
    BT_LE_SRV_CS_SUBEVENT_ABORT_REQUESTED = 0x1,
    BT_LE_SRV_CS_SUBEVENT_ABORT_NO_CS_SYNC = 0x2,
    BT_LE_SRV_CS_SUBEVENT_ABORT_SCHED_CONFLICT = 0x3,
    BT_LE_SRV_CS_SUBEVENT_ABORT_UNSPECIFIED = 0xF,
} bt_le_srv_cs_subevent_abort_reason_t;

typedef struct {
    /** CS configuration ID, Range 0 - 3 */
    uint8_t id;
    /** Main CS mode type */
    bt_srv_conn_le_cs_main_mode_t main_mode_type;
    /** Sub CS mode type */
    bt_srv_conn_le_cs_sub_mode_t sub_mode_type;
    /** Minimum number of CS main mode steps to be executed before a submode step is executed */
    uint8_t min_main_mode_steps;
    /** Maximum number of CS main mode steps to be executed before a submode step is executed */
    uint8_t max_main_mode_steps;
    /** Number of main mode steps taken from the end of the last CS subevent to be repeated
     * at the beginning of the current CS subevent directly after the last mode-0 step of that
     * event
     */
    uint8_t main_mode_repetition;
    /** Number of CS mode-0 steps to be included at the beginning of each CS subevent */
    uint8_t mode_0_steps;
    /** CS role */
    bt_srv_conn_le_cs_role_t role;
    /** RTT type */
    bt_srv_conn_le_cs_rtt_type_t rtt_type;
    /** CS Sync PHY */
    bt_srv_conn_le_cs_sync_phy_t cs_sync_phy;
    /** Channel map used for CS procedure
     *  Channels n = 0, 1, 23, 24, 25, 77, and 78 are not allowed and shall be set to zero.
     *  Channel 79 is reserved for future use and shall be set to zero.
     *  At least 15 channels shall be enabled.
     */
    uint8_t channel_map[10];
    /** The number of times the Channel_Map field will be cycled through for non-mode-0 steps
     * within a CS procedure
     */
    uint8_t channel_map_repetition;
    /** Channel selection type */
    bt_srv_conn_le_cs_chsel_type_t channel_selection_type;
    /** User-specified channel sequence shape */
    bt_srv_conn_le_cs_ch3c_shape_t ch3c_shape;
    /** Number of channels skipped in each rising and falling sequence
     * Range 0x02 to 0x08, valid when channel_selection_type is CS_BT_SRV_CONN_LE_CS_CHSEL_TYPE_3C.
     */
    uint8_t ch3c_jump;
} bt_le_srv_cs_create_config_params_t;

typedef struct {
    uint8_t config_id;
    uint8_t enable;
} bt_le_srv_cs_procedure_enable_param_t;

typedef struct {
    /* The ID associated with the desired configuration (0 to 3) */
    uint8_t config_id;

    /* Max. duration for each CS procedure, where T = N * 0.625 ms (0x0001 to 0xFFFF) */
    uint16_t max_procedure_len;

    /* Min. number of connection events between consecutive CS procedures (0x0001 to 0xFFFF) */
    uint16_t min_procedure_interval;

    /* Max. number of connection events between consecutive CS procedures (0x0001 to 0xFFFF) */
    uint16_t max_procedure_interval;

    /* Max. number of procedures to be scheduled (0x0000 for no limit; otherwise 0x0001
     * to 0xFFFF)
     */
    uint16_t max_procedure_count;

    /* Min. suggested duration for each CS subevent in microseconds [1250 us to 4 s) */
    uint32_t min_subevent_len;

    /* Max. suggested duration for each CS subevent in microseconds [1250 us to 4 s) */
    uint32_t max_subevent_len;

    /* Antenna configuration index */
    bt_srv_conn_le_cs_tone_antenna_config_selection_t tone_antenna_config_selection;

    /* Phy */
    bt_le_srv_cs_procedure_phy_t phy;

    /* Transmit power delta, in signed dB, to indicate the recommended difference between the
     * remote device's power level for the CS tones and RTT packets and the existing power
     * level for the Phy indicated by the Phy parameter (0x80 for no recommendation)
     */
    int8_t tx_power_delta;

    /* Preferred peer antenna (Bitmask of BT_LE_CS_PROCEDURE_PREFERRED_PEER_ANTENNA_*) */
    uint8_t preferred_peer_antenna;

    /* Initiator SNR control adjustment */
    bt_le_srv_cs_snr_control_t snr_control_initiator;

    /* Reflector SNR control adjustment */
    bt_le_srv_cs_snr_control_t snr_control_reflector;
} bt_le_srv_cs_set_procedure_parameters_param_t;

/** Default CS settings in the local Controller */
typedef struct {
    /** Enable CS initiator role. */
    bool enable_initiator_role;
    /** Enable CS reflector role. */
    bool enable_reflector_role;
    /** Antenna identifier to be used for CS_SYNC packets by the local controller.
     */
    bt_le_srv_cs_sync_antenna_selection_t cs_sync_antenna_selection;
    /** The maximum transmit power level to be used for all CS transmissions.
     *  for all CS transmissions.
     *
     *  Value range is -127 to 20.
     */
    int8_t max_tx_power;
} bt_le_srv_cs_set_default_settings_param_t;

/** Remote channel sounding capabilities for LE connections supporting CS */
typedef struct {
    /** Number of CS configurations.
     *  Range 1 - 4
     */
    uint8_t num_config_supported;
    /** Maximum number of consecutive CS procedures.
     *
     * When set to zero, indicates support for both fixed and indefinite
     * numbers of CS procedures before termination.
     */
    uint16_t max_consecutive_procedures_supported;
    /** Number of antennas.
     *  Range 1 - 4
     */
    uint8_t num_antennas_supported;
    /** Maximum number of antenna paths.
     *  Range 1 - 4
     */
    uint8_t max_antenna_paths_supported;
    /** Initiator role. */
    bool initiator_supported;
    /** Reflector role. */
    bool reflector_supported;
    /** Mode-3 */
    bool mode_3_supported;
    /** RTT AA-Only */
    bt_srv_conn_le_cs_capability_rtt_aa_only_t rtt_aa_only_precision;
    /** RTT Sounding */
    bt_srv_conn_le_cs_capability_rtt_sounding_t rtt_sounding_precision;
    /** RTT Random Payload */
    bt_srv_conn_le_cs_capability_rtt_random_payload_t rtt_random_payload_precision;
    /** Number of CS steps needed to achieve the
     * accuracy requirements for RTT AA Only.
     *
     * Set to 0 if RTT AA Only isn't supported.
     */
    uint8_t rtt_aa_only_n;
    /** Number of CS steps needed to achieve the
     * accuracy requirements for RTT Sounding.
     *
     * Set to 0 if RTT Sounding isn't supported
     */
    uint8_t rtt_sounding_n;
    /** Number of CS steps needed to achieve the
     * accuracy requirements for RTT Random Payload.
     *
     * Set to 0 if RTT Random Payload isn't supported.
     */
    uint8_t rtt_random_payload_n;
    /** Phase-based normalized attack detector metric
     * when a CS_SYNC with sounding sequence is received.
     */
    bool amplitude_based_nadm_sounding_supported;
    /** Phase-based normalized attack detector metric
     * when a CS_SYNC with random sequence is received.
     */
    bool amplitude_based_nadm_random_supported;
    /** CS_SYNC LE 2M PHY. */
    bool cs_sync_2m_phy_supported;
    /** CS_SYNC LE 2M 2BT PHY. */
    bool cs_sync_2m_2bt_phy_supported;
    /** Subfeature: CS with no Frequency Actuation Error. */
    bool cs_without_fae_supported;
    /** Subfeature: Channel Selection Algorithm #3c */
    bool chsel_alg_3c_supported;
    /** Subfeature: Phase-based Ranging from RTT sounding sequence. */
    bool pbr_from_rtt_sounding_seq_supported;
    /** Optional T_IP1 time durations during CS steps.
     *
     *  - Bit 0: 10 us
     *  - Bit 1: 20 us
     *  - Bit 2: 30 us
     *  - Bit 3: 40 us
     *  - Bit 4: 50 us
     *  - Bit 5: 60 us
     *  - Bit 6: 80 us
     */
    uint16_t t_ip1_times_supported;
    /** Optional T_IP2 time durations during CS steps.
     *
     *  - Bit 0: 10 us
     *  - Bit 1: 20 us
     *  - Bit 2: 30 us
     *  - Bit 3: 40 us
     *  - Bit 4: 50 us
     *  - Bit 5: 60 us
     *  - Bit 6: 80 us
     */
    uint16_t t_ip2_times_supported;
    /** Optional T_FCS time durations during CS steps.
     *
     *  - Bit 0: 15 us
     *  - Bit 1: 20 us
     *  - Bit 2: 30 us
     *  - Bit 3: 40 us
     *  - Bit 4: 50 us
     *  - Bit 5: 60 us
     *  - Bit 6: 80 us
     *  - Bit 7: 100 us
     *  - Bit 8: 120 us
     */
    uint16_t t_fcs_times_supported;
    /** Optional T_PM time durations during CS steps.
     *
     *  - Bit 0: 10 us
     *  - Bit 1: 20 us
     */
    uint16_t t_pm_times_supported;
    /** Time in microseconds for the antenna switch period of the CS tones.
     *  Values in 0x00, 0x01, 0x02, 0x04, 0x0A.
     */
    uint8_t t_sw_time;
    /** Supported SNR levels used in RTT packets.
     *
     *  - Bit 0: 18dB
     *  - Bit 1: 21dB
     *  - Bit 2: 24dB
     *  - Bit 3: 27dB
     *  - Bit 4: 30dB
     */
    uint8_t tx_snr_capability;
} bt_srv_conn_le_cs_capabilities_t;

typedef struct {
    /** CS configuration ID */
    uint8_t id;
    /** Main CS mode type */
    bt_srv_conn_le_cs_main_mode_t main_mode_type;
    /** Sub CS mode type */
    bt_srv_conn_le_cs_sub_mode_t sub_mode_type;
    /** Minimum number of CS main mode steps to be executed before a submode step is executed */
    uint8_t min_main_mode_steps;
    /** Maximum number of CS main mode steps to be executed before a submode step is executed */
    uint8_t max_main_mode_steps;
    /** Number of main mode steps taken from the end of the last CS subevent to be repeated
     *  at the beginning of the current CS subevent directly after the last mode-0 step of that
     *  event
     */
    uint8_t main_mode_repetition;
    /** Number of CS mode-0 steps to be included at the beginning of each CS subevent */
    uint8_t mode_0_steps;
    /** CS role */
    bt_srv_conn_le_cs_role_t role;
    /** RTT type */
    bt_srv_conn_le_cs_rtt_type_t rtt_type;
    /** CS Sync PHY */
    bt_srv_conn_le_cs_sync_phy_t cs_sync_phy;
    /** Channel map used for CS procedure
     *  Channels n = 0, 1, 23, 24, 25, 77, and 78 are not allowed and be set to zero.
     *  Channel 79 is reserved for future use and shall be set to zero.
     *  At least 15 channels shall be enabled.
     */
    uint8_t channel_map[10];
    /** The number of times the Channel_Map field will be cycled through for non-mode-0 steps
     *  within a CS procedure
     */
    uint8_t channel_map_repetition;
    /** Channel selection type */
    bt_srv_conn_le_cs_chsel_type_t channel_selection_type;
    /** User-specified channel sequence shape */
    bt_srv_conn_le_cs_ch3c_shape_t ch3c_shape;
    /** Number of channels skipped in each rising and falling sequence  */
    uint8_t ch3c_jump;
    /** Interlude time in microseconds between the RTT packets */
    uint8_t t_ip1_time_us;
    /** Interlude time in microseconds between the CS tones */
    uint8_t t_ip2_time_us;
    /** Time in microseconds for frequency changes */
    uint8_t t_fcs_time_us;
    /** Time in microseconds for the phase measurement period of the CS tones */
    uint8_t t_pm_time_us;
} bt_srv_conn_le_cs_config_t;

typedef struct {
    /* The ID associated with the desired configuration (0 to 3) */
    uint8_t config_id;

    /* State of the CS procedure */
    bt_srv_conn_le_cs_procedure_enable_state_t state;

    /* Antenna configuration index */
    bt_srv_conn_le_cs_tone_antenna_config_selection_t tone_antenna_config_selection;

    /* Transmit power level used for CS procedures (-127 to 20 dB; 0x7F if unavailable) */
    int8_t selected_tx_power;

    /* Duration of each CS subevent in microseconds (1250 us to 4 s) */
    uint32_t subevent_len;

    /* Number of CS subevents anchored off the same ACL connection event (0x01 to 0x20) */
    uint8_t subevents_per_event;

    /* Time between consecutive CS subevents anchored off the same ACL connection event in
     * units of 0.625 ms
     */
    uint16_t subevent_interval;

    /* Number of ACL connection events between consecutive CS event anchor points */
    uint16_t event_interval;

    /* Number of ACL connection events between consecutive CS procedure anchor points */
    uint16_t procedure_interval;

    /* Number of CS procedures to be scheduled (0 if procedures to continue until disabled) */
    uint16_t procedure_count;

    /* Maximum duration for each procedure in units of 0.625 ms (0x0001 to 0xFFFF) */
    uint16_t max_procedure_len;
} bt_srv_conn_le_cs_procedure_enable_complete_t;

typedef struct {
    struct {
        /** CS configuration identifier.
         *
         *  Range: 0 to 3
         *
         *  If these results were generated by a CS Test,
         *  this value will be set to 0 and has no meaning.
         */
        uint8_t config_id;
        /** Starting ACL connection event counter.
         *
         *  If these results were generated by a CS Test,
         *  this value will be set to 0 and has no meaning.
         */
        uint16_t start_acl_conn_event_counter;
        /** CS procedure count associated with these results.
         *
         *  This is the CS procedure count since the completion of
         *  the Channel Sounding Security Start procedure.
         */
        uint16_t procedure_counter;
        /** Frequency compensation value in units of 0.01 ppm.
         *
         *  This is a 15-bit signed integer in the range [-100, 100] ppm.
         *
         *  indicates that the role is not the initiator, or that the
         *  frequency compensation value is unavailable.
         */
        uint16_t frequency_compensation;
        /** Reference power level in dBm.
         *
         *  Range: -127 to 20
         *
         *  0x7F indicates that the reference power level was not available during a subevent.
         */
        int8_t reference_power_level;
        /** Procedure status. */
        bt_le_srv_cs_procedure_done_status_t procedure_done_status;
        /** Subevent status
         *
         *  For aborted subevents,
         *  and abort_step will contain the step number on which the subevent was aborted.
         *  Consider the following example:
         *
         *  subevent_done_status = @ref SAL_LE_CS_SUBEVENT_ABORTED
         *  num_steps_reported = 160
         *  abort_step = 100
         *
         *  this would mean that steps from 0 to 99 are complete and steps from 100 to 159
         *  are aborted.
         */
        bt_le_srv_le_cs_subevent_done_status_t subevent_done_status;
        /** Abort reason.
         *
         *  If the procedure status is
         *  @ref SAL_LE_CS_PROCEDURE_ABORTED, this field will
         *  specify the reason for the abortion.
         */
        bt_le_srv_cs_procedure_abort_reason_t procedure_abort_reason;
        /** Abort reason.
         *
         *  If the subevent status is
         *  @ref SAL_LE_CS_SUBEVENT_ABORTED, this field will
         *  specify the reason for the abortion.
         */
        bt_le_srv_cs_subevent_abort_reason_t subevent_abort_reason;
        /** Number of antenna paths used during the phase measurement stage.
         */
        uint8_t num_antenna_paths;
        /** Number of CS steps in the subevent.
         */
        uint8_t num_steps_reported;
        /** Step number, on which the subevent was aborted
         *  if subevent_done_status is @ref SAL_LE_CS_SUBEVENT_COMPLETE
         *  then abort_step will be unused and set to 255
         */
        uint8_t abort_step;
    } header;

    uint16_t len;
    uint8_t* step_data_buf;
} bt_srv_conn_le_cs_subevent_result_t;

typedef struct {
    size_t size;

    /**
     * @brief Register the cs event callback
     * @param[in] callbacks  cs event callback function.
     */
    void* (*register_callbacks)(void* remote, const cs_callbacks_t* callbacks);

    /**
     * @brief Unregister the cs event callback
     */
    bool (*unregister_callbacks)(void** remote, void* cookie);
    bt_status_t (*start_distance_measurement)(bt_distance_measurement_params_t* params);

    bt_status_t (*stop_distance_measurement)(bt_address_t* addr, int method, bool timeout);

    bt_status_t (*set_config)(bt_address_t* addr, const bt_cs_set_params_t* params);

#ifdef CONFIG_BT_CS_RAS_TEST
    bt_status_t (*cs_test)(void* data, uint16_t len);
#endif /* CONFIG_BT_CS_RAS_TEST */

} bt_cs_interface_t;

typedef void (*subevent_result_cb_t)(bt_address_t* addr, bt_srv_conn_le_cs_subevent_result_t* result);

/*
 * register profile to service manager
 */
void register_cs_service(void);

void bt_sal_cs_event_callback(cs_msg_t* msg);

void bt_cs_register_subevent_cb(subevent_result_cb_t cb);

#endif /* __CS_SERVICE_H__ */
