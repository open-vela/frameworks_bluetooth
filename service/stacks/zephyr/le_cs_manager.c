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

 #include <math.h>
 #include <zephyr/bluetooth/cs.h>
 #include "le_cs_manager.h"

#define LE_CS_FREQUENCY_MHZ(ch)    (2402u + 1u * (ch))
#define LE_CS_FREQUENCY_HZ(ch)     (LE_CS_FREQUENCY_MHZ(ch) * 1000000.0f)
#define LE_CS_SPEED_OF_LIGHT_M_PER_S  (299792458.0f)
#define LE_CS_SPEED_OF_LIGHT_NM_PER_S (LE_CS_SPEED_OF_LIGHT_M_PER_S / 1000000000.0f)
#define PI                      3.14159265358979323846f
#define LE_CS_MAX_NUM_SAMPLES         256

struct iq_sample_and_channel {
	bool failed;
	uint8_t channel;
	uint8_t antenna_permutation;
	struct bt_le_cs_iq_sample local_iq_sample;
	struct bt_le_cs_iq_sample peer_iq_sample;
};

struct rtt_timing {
	bool failed;
	int16_t initiator_toa_tod;
	int16_t reflector_tod_toa;
};

static struct iq_sample_and_channel mode_2_data[LE_CS_MAX_NUM_SAMPLES];
static struct rtt_timing tof_data_records[LE_CS_MAX_NUM_SAMPLES];

struct cs_processing_state {
	bool processing_local_data;
	uint8_t mode_1_data_index;
	uint8_t mode_2_data_index;
	uint8_t n_ap;
	enum bt_conn_le_cs_role role;
};

static void le_cs_compute_complex_mul(int32_t real1, int32_t imag1,
                                int32_t real2, int32_t imag2,
                                int32_t *result_real, int32_t *result_imag)
{
    *result_real = real1 * real2 - imag1 * imag2;
    *result_imag = real1 * imag2 + imag1 * real2;
}

static float le_cs_perform_linear_fit(float *x, float *y, uint8_t count)
{
    if (count == 0) {
        return 0.0f;
    }

    float mean_x = 0.0f, mean_y = 0.0f;

    for (uint8_t i = 0; i < count; i++) {
        mean_x += (x[i] - mean_x) / (i + 1);
        mean_y += (y[i] - mean_y) / (i + 1);
    }

    float numerator = 0.0f, denominator = 0.0f;

    for (uint8_t i = 0; i < count; i++) {
        float dx = x[i] - mean_x;
        float dy = y[i] - mean_y;
        numerator += dx * dy;
        denominator += dx * dx;
    }

    return numerator / denominator;
}

static void le_cs_sort_by_reference_array(float *keys, float *values, uint16_t size)
{
    for (uint16_t i = 0; i < size - 1; i++) {
        bool swapped = false;
        for (uint16_t j = 0; j < size - i - 1; j++) {
            if (keys[j] > keys[j + 1]) {
                float tmp_key = keys[j];
                float tmp_val = values[j];
                keys[j] = keys[j + 1];
                values[j] = values[j + 1];
                keys[j + 1] = tmp_key;
                values[j + 1] = tmp_val;
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

static float le_cs_compute_distance_from_phase_gradient(struct iq_sample_and_channel *samples, 
                                                        uint8_t sample_count)
{
    int32_t i_combined, q_combined;
    uint16_t valid_count = 0;
    static float phase_array[LE_CS_MAX_NUM_SAMPLES];
    static float freq_array[LE_CS_MAX_NUM_SAMPLES];

    for (uint8_t idx = 0; idx < sample_count; idx++) {
        if (!samples[idx].failed) {
            le_cs_compute_complex_mul(samples[idx].local_iq_sample.i, samples[idx].local_iq_sample.q,
                                samples[idx].peer_iq_sample.i, samples[idx].peer_iq_sample.q,
                                &i_combined, &q_combined);

            phase_array[valid_count] = atan2f((float)q_combined, (float)i_combined);
            freq_array[valid_count] = (float)LE_CS_FREQUENCY_MHZ(samples[idx].channel);
            valid_count++;
        }
    }

    if (valid_count < 2) {
        return 0.0f;
    }

    sort_by_reference_array(freq_array, phase_array, valid_count);

    for (uint8_t i = 1; i < valid_count; i++) {
        float delta = phase_array[i] - phase_array[i - 1];
        if (delta > PI) {
            for (uint8_t j = i; j < valid_count; j++) {
                phase_array[j] -= 2.0f * PI;
            }
        } else if (delta < -PI) {
            for (uint8_t j = i; j < valid_count; j++) {
                phase_array[j] += 2.0f * PI;
            }
        }
    }

    float slope = le_cs_perform_linear_fit(freq_array, phase_array, valid_count);
    float estimated_distance = -slope * (LE_CS_SPEED_OF_LIGHT_NM_PER_S / (4.0f * PI));

    return estimated_distance / 1000000.0f;  // Convert to meters
}

static float calculate_distance_via_tof(uint8_t sample_count)
{
    float time_diff;
    float avg_tof = 0.0f;

    // Cumulative Moving Average (CMA)
    for (uint8_t index = 0; index < sample_count; index++) {
        if (!tof_data_records[index].is_invalid) {
            time_diff = (tof_data_records[index].initiator_toa_tod -
                         tof_data_records[index].reflector_tod_toa) / 2.0f;

            avg_tof += (time_diff - avg_tof) / (index + 1);
        }
    }

    float avg_tof_ns = avg_tof / 2.0f;

    return avg_tof_ns * LE_CS_SPEED_OF_LIGHT_NM_PER_S;
}

static bool handle_subevent_step_data(struct bt_le_cs_subevent_step *subevent_step, void *arg)
{
    struct cs_processing_state *state = (struct cs_processing_state *)arg;

    if (subevent_step->mode == BT_CONN_LE_CS_MAIN_MODE_2) {
        struct bt_hci_le_cs_step_data_mode_2 *mode2_raw =
            (struct bt_hci_le_cs_step_data_mode_2 *)subevent_step->data;

        if (state->processing_local_data) {
            for (uint8_t idx = 0; idx < (state->antenna_count + 1); idx++) {
                if (mode2_raw->tone_info[idx].extension_indicator !=
                    BT_HCI_LE_CS_NOT_TONE_EXT_SLOT) {
                    continue;
                }

                phase_data_mode2[state->index_mode2].channel = subevent_step->channel;
                phase_data_mode2[state->index_mode2].antenna_permutation =
                    mode2_raw->antenna_permutation_index;
                phase_data_mode2[state->index_mode2].local_iq =
                    bt_le_cs_parse_pct(mode2_raw->tone_info[idx].phase_correction_term);

                if (mode2_raw->tone_info[idx].quality_indicator ==
                        BT_HCI_LE_CS_TONE_QUALITY_LOW ||
                    mode2_raw->tone_info[idx].quality_indicator ==
                        BT_HCI_LE_CS_TONE_QUALITY_UNAVAILABLE) {
                    phase_data_mode2[state->index_mode2].invalid = true;
                }

                state->index_mode2++;
            }
        } else {
            for (uint8_t idx = 0; idx < (state->antenna_count + 1); idx++) {
                if (mode2_raw->tone_info[idx].extension_indicator !=
                    BT_HCI_LE_CS_NOT_TONE_EXT_SLOT) {
                    continue;
                }

                phase_data_mode2[state->index_mode2].peer_iq =
                    bt_le_cs_parse_pct(mode2_raw->tone_info[idx].phase_correction_term);

                if (mode2_raw->tone_info[idx].quality_indicator ==
                        BT_HCI_LE_CS_TONE_QUALITY_LOW ||
                    mode2_raw->tone_info[idx].quality_indicator ==
                        BT_HCI_LE_CS_TONE_QUALITY_UNAVAILABLE) {
                    phase_data_mode2[state->index_mode2].invalid = true;
                }

                state->index_mode2++;
            }
        }
    } else if (subevent_step->mode == BT_HCI_OP_LE_CS_MAIN_MODE_1) {
        struct bt_hci_le_cs_step_data_mode_1 *mode1_raw =
            (struct bt_hci_le_cs_step_data_mode_1 *)subevent_step->data;

        if (mode1_raw->packet_quality_aa_check !=
                BT_HCI_LE_CS_PACKET_QUALITY_AA_CHECK_SUCCESSFUL ||
            mode1_raw->packet_rssi == BT_HCI_LE_CS_PACKET_RSSI_NOT_AVAILABLE ||
            mode1_raw->tod_toa_reflector == BT_HCI_LE_CS_TIME_DIFFERENCE_NOT_AVAILABLE) {
            tof_data_mode1[state->index_mode1].invalid = true;
        }

        if (state->processing_local_data) {
            if (state->role == BT_CONN_LE_CS_ROLE_INITIATOR) {
                tof_data_mode1[state->index_mode1].initiator_toa_tod =
                    mode1_raw->toa_tod_initiator;
            } else if (state->role == BT_CONN_LE_CS_ROLE_REFLECTOR) {
                tof_data_mode1[state->index_mode1].reflector_tod_toa =
                    mode1_raw->tod_toa_reflector;
            }
        } else {
            if (state->role == BT_CONN_LE_CS_ROLE_INITIATOR) {
                tof_data_mode1[state->index_mode1].reflector_tod_toa =
                    mode1_raw->tod_toa_reflector;
            } else if (state->role == BT_CONN_LE_CS_ROLE_REFLECTOR) {
                tof_data_mode1[state->index_mode1].initiator_toa_tod =
                    mode1_raw->toa_tod_initiator;
            }
        }

        state->index_mode1++;
    }

    return true;
}

void bt_le_cs_run_distance_estimation(uint8_t *local_step_data, uint16_t local_data_len,
                             uint8_t *peer_step_data, uint16_t peer_data_len,
                             uint8_t antenna_count, enum bt_conn_le_cs_role device_role)
{
    struct net_buf_simple buffer;

    struct cs_processing_state state = {
        .processing_local_data = true,
        .index_mode_1 = 0,
        .index_mode_2 = 0,
        .antenna_count = antenna_count,
        .role = device_role,
    };

    memset(tof_data_mode1, 0, sizeof(tof_data_mode1));
    memset(phase_data_mode2, 0, sizeof(phase_data_mode2));

    net_buf_simple_init_with_data(&buffer, local_step_data, local_data_len);
    bt_le_cs_step_data_parse(&buffer, handle_subevent_step_data, &state);

    state.index_mode_1 = 0;
    state.index_mode_2 = 0;
    state.processing_local_data = false;

    net_buf_simple_init_with_data(&buffer, peer_step_data, peer_data_len);
    bt_le_cs_step_data_parse(&buffer, handle_subevent_step_data, &state);

    float distance_by_phase = compute_distance_from_phase_gradient(
        phase_data_mode2, state.index_mode_2);

    float distance_by_tof = calculate_distance_via_tof(state.index_mode_1);

    if (distance_by_tof == 0.0f && distance_by_phase == 0.0f) {
        BT_LOGI("A reliable distance estimate could not be computed.");
    } else {
        BT_LOGI("Estimated distance to reflector:");
    }

    if (distance_by_tof != 0.0f) {
        BT_LOGI("- Round-Trip Timing method: %f meters (derived from %d samples)\n",
               (double)distance_by_tof, state.index_mode_1);
    }

    if (distance_by_phase != 0.0f) {
        BT_LOGI("- Phase-Based Ranging method: %f meters (derived from %d samples)\n",
               (double)distance_by_phase, state.index_mode_2);
    }

    return;
}




