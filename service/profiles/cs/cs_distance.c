/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#define LOG_TAG "cs_distance"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "cs_distance.h"
#include "utils/log.h"

/**
 * @brief Calculate complex product: z_out = z_a * z_b
 *
 * For z_a = a + bi and z_b = c + di:
 * z_a * z_b = (ac - bd) + (ad + bc)i
 */
static void calc_complex_product(int32_t z_a_real, int32_t z_a_imag,
    int32_t z_b_real, int32_t z_b_imag,
    int32_t* z_out_real, int32_t* z_out_imag)
{
    *z_out_real = z_a_real * z_b_real - z_a_imag * z_b_imag;
    *z_out_imag = z_a_real * z_b_imag + z_a_imag * z_b_real;
}

/**
 * @brief Linear regression to estimate slope
 *
 * Estimates b in y = a + b*x using least squares method
 */
static float linear_regression(float* x_values, float* y_values, uint8_t n_samples)
{
    if (n_samples == 0) {
        return 0.0f;
    }

    float y_mean = 0.0f;
    float x_mean = 0.0f;

    /* Calculate means using cumulative moving average */
    for (uint8_t i = 0; i < n_samples; i++) {
        y_mean += (y_values[i] - y_mean) / (i + 1);
        x_mean += (x_values[i] - x_mean) / (i + 1);
    }

    float b_est_upper = 0.0f;
    float b_est_lower = 0.0f;

    for (uint8_t i = 0; i < n_samples; i++) {
        b_est_upper += (x_values[i] - x_mean) * (y_values[i] - y_mean);
        b_est_lower += (x_values[i] - x_mean) * (x_values[i] - x_mean);
    }

    if (b_est_lower == 0.0f) {
        return 0.0f;
    }

    return b_est_upper / b_est_lower;
}

/**
 * @brief Paired data point for sorting frequencies with associated phases
 */
typedef struct {
    float frequency;
    float theta;
} cs_freq_phase_pair_t;

static int cs_freq_phase_cmp(const void* a, const void* b)
{
    float fa = ((const cs_freq_phase_pair_t*)a)->frequency;
    float fb = ((const cs_freq_phase_pair_t*)b)->frequency;

    if (fa < fb) return -1;
    if (fa > fb) return 1;
    return 0;
}

/**
 * @brief Step data header (packed, 1-byte aligned)
 */
typedef struct __attribute__((packed)) {
    uint8_t step_mode;
    uint8_t step_channel;
    uint8_t step_data_len;
    uint8_t data[];
} cs_step_header_t;

/* Step data field offsets for Mode 1 */
#define CS_STEP_PKT_QUALITY_OFFSET      0
#define CS_STEP_PKT_RSSI_OFFSET         2
#define CS_STEP_TIME_DIFF_OFFSET        3

/* Invalid time difference marker per BT spec */
#define CS_TIME_DIFF_NOT_AVAILABLE      ((int16_t)0x8000)

/* Packet quality AA-check mask */
#define CS_PKT_QUALITY_AA_MASK          0x0F
/* Mode 3 packet quality AA-check mask */
#define CS_MODE3_PKT_QUALITY_AA_MASK    0x03

/* RSSI not available marker */
#define CS_RSSI_NOT_AVAILABLE           0x7F

static cs_iq_sample_t cs_distance_parse_pct(const uint8_t* pct_data)
{
    cs_iq_sample_t sample;

    /* PCT is 24-bit data: 12-bit I + 12-bit Q */
    int32_t raw_i = pct_data[0] | ((pct_data[1] & 0x0F) << 8);
    int32_t raw_q = ((pct_data[1] >> 4) & 0x0F) | (pct_data[2] << 4);

    /* Sign extension for 12-bit signed values */
    if (raw_i & 0x800) raw_i |= 0xFFFFF000;
    if (raw_q & 0x800) raw_q |= 0xFFFFF000;

    sample.i = (int16_t)raw_i;
    sample.q = (int16_t)raw_q;

    return sample;
}

static void cs_distance_process_mode1(const uint8_t* step_data, uint8_t step_channel,
    uint8_t quality_mask, cs_step_parse_ctx_t* ctx)
{
    if (ctx->mode1_idx >= CS_DISTANCE_MAX_SAMPLES || !ctx->mode1_data) {
        return;
    }

    cs_mode1_sample_t* m1 = &ctx->mode1_data[ctx->mode1_idx];

    if ((step_data[CS_STEP_PKT_QUALITY_OFFSET] & quality_mask) != 0x00) {
        m1->failed = true;
    }

    if (step_data[CS_STEP_PKT_RSSI_OFFSET] == CS_RSSI_NOT_AVAILABLE) {
        m1->failed = true;
    }

    int16_t time_diff = (int16_t)(step_data[CS_STEP_TIME_DIFF_OFFSET] |
        (step_data[CS_STEP_TIME_DIFF_OFFSET + 1] << 8));
    if (time_diff == CS_TIME_DIFF_NOT_AVAILABLE) {
        m1->failed = true;
    }

    if (ctx->is_local) {
        if (ctx->role == CS_ROLE_INITIATOR) {
            m1->toa_tod_initiator = time_diff;
        } else {
            m1->tod_toa_reflector = time_diff;
        }
    } else {
        if (ctx->role == CS_ROLE_INITIATOR) {
            m1->tod_toa_reflector = time_diff;
        } else {
            m1->toa_tod_initiator = time_diff;
        }
    }

    ctx->mode1_idx++;
}

static void cs_distance_process_mode2(const uint8_t* step_data, uint8_t step_channel,
    cs_step_parse_ctx_t* ctx)
{
    if (!ctx->mode2_data) {
        return;
    }

    uint8_t num_tones = ctx->n_ap + 1;

    for (uint8_t i = 0; i < num_tones && ctx->mode2_idx < CS_DISTANCE_MAX_SAMPLES; i++) {
        const uint8_t* tone_info = &step_data[1 + i * 4];
        uint8_t quality_ext = tone_info[3];
        uint8_t quality = quality_ext & 0x0F;
        uint8_t extension = (quality_ext >> 4) & 0x0F;

        if (extension != 0x00) {
            continue;
        }

        cs_mode2_sample_t* m2 = &ctx->mode2_data[ctx->mode2_idx];

        if (ctx->is_local) {
            m2->channel = step_channel;
        }
        m2->antenna_permutation = step_data[0];

        if (ctx->is_local) {
            m2->local_iq = cs_distance_parse_pct(&tone_info[0]);
        } else {
            m2->peer_iq = cs_distance_parse_pct(&tone_info[0]);
        }

        if (quality == 0x02 || quality == 0x03) {
            m2->failed = true;
        }

        ctx->mode2_idx++;
    }
}

static void cs_distance_parse_step_data(const uint8_t* data, uint16_t len,
    cs_step_parse_ctx_t* ctx)
{
    uint16_t offset = 0;

    while (offset + sizeof(cs_step_header_t) <= len) {
        const cs_step_header_t* hdr = (const cs_step_header_t*)&data[offset];

        if (offset + sizeof(cs_step_header_t) + hdr->step_data_len > len) {
            BT_LOGW("Incomplete step data at offset %d", offset);
            break;
        }

        const uint8_t* step_data = hdr->data;

        switch (hdr->step_mode) {
        case CS_STEP_MODE_1:
            cs_distance_process_mode1(step_data, hdr->step_channel,
                CS_PKT_QUALITY_AA_MASK, ctx);
            break;

        case CS_STEP_MODE_2:
            cs_distance_process_mode2(step_data, hdr->step_channel, ctx);
            break;

        case CS_STEP_MODE_3: {
            cs_distance_process_mode1(step_data, hdr->step_channel,
                CS_MODE3_PKT_QUALITY_AA_MASK, ctx);

            uint8_t mode2_offset = 14;
            if (ctx->mode2_data) {
                uint8_t num_tones = ctx->n_ap + 1;

                for (uint8_t i = 0; i < num_tones && ctx->mode2_idx < CS_DISTANCE_MAX_SAMPLES; i++) {
                    cs_mode2_sample_t* m2 = &ctx->mode2_data[ctx->mode2_idx];

                    m2->channel = hdr->step_channel;
                    m2->antenna_permutation = step_data[mode2_offset];

                    const uint8_t* pct = &step_data[mode2_offset + 1 + i * 3];

                    if (ctx->is_local) {
                        m2->local_iq = cs_distance_parse_pct(pct);
                    } else {
                        m2->peer_iq = cs_distance_parse_pct(pct);
                    }

                    ctx->mode2_idx++;
                }
            }
            break;
        }

        default:
            break;
        }

        offset += sizeof(cs_step_header_t) + hdr->step_data_len;
    }
}

static float cs_distance_estimate_phase_slope(const cs_mode2_sample_t* data, uint8_t len)
{
    int32_t combined_i;
    int32_t combined_q;
    uint16_t num_angles = 0;
    cs_freq_phase_pair_t* pairs;
    float* frequencies;
    float* theta;
    float distance;

    pairs = (cs_freq_phase_pair_t*)malloc(len * sizeof(cs_freq_phase_pair_t));
    if (!pairs) {
        return 0.0f;
    }

    for (uint8_t i = 0; i < len; i++) {
        if (!data[i].failed) {
            calc_complex_product(
                data[i].local_iq.i, data[i].local_iq.q,
                data[i].peer_iq.i, data[i].peer_iq.q,
                &combined_i, &combined_q);

            pairs[num_angles].theta = atan2f(1.0f * combined_q, 1.0f * combined_i);
            pairs[num_angles].frequency = 1.0f * CS_FREQUENCY_MHZ(data[i].channel);
            num_angles++;
        }
    }

    if (num_angles < 2) {
        free(pairs);
        return 0.0f;
    }

    qsort(pairs, num_angles, sizeof(cs_freq_phase_pair_t), cs_freq_phase_cmp);

    /* One-dimensional phase unwrapping */
    for (uint16_t i = 1; i < num_angles; i++) {
        float difference = pairs[i].theta - pairs[i - 1].theta;

        if (difference > CS_PI) {
            for (uint16_t j = i; j < num_angles; j++) {
                pairs[j].theta -= 2.0f * CS_PI;
            }
        } else if (difference < -CS_PI) {
            for (uint16_t j = i; j < num_angles; j++) {
                pairs[j].theta += 2.0f * CS_PI;
            }
        }
    }

    frequencies = (float*)malloc(num_angles * sizeof(float));
    theta = (float*)malloc(num_angles * sizeof(float));
    if (!frequencies || !theta) {
        free(pairs);
        free(frequencies);
        free(theta);
        return 0.0f;
    }

    for (uint16_t i = 0; i < num_angles; i++) {
        frequencies[i] = pairs[i].frequency;
        theta[i] = pairs[i].theta;
    }
    free(pairs);

    float phase_slope = linear_regression(frequencies, theta, num_angles);
    distance = -phase_slope * (SPEED_OF_LIGHT_M_PER_S / (4 * CS_PI));

    free(frequencies);
    free(theta);

    /* Scale to meters (frequency was in MHz) */
    return distance / 1000000.0f;
}

static float cs_distance_estimate_rtt(const cs_mode1_sample_t* data, uint8_t len)
{
    float tof;
    float tof_mean = 0.0f;
    uint8_t valid_samples = 0;

    for (uint8_t i = 0; i < len; i++) {
        if (!data[i].failed) {
            tof = (data[i].toa_tod_initiator - data[i].tod_toa_reflector) / 2.0f;
            valid_samples++;
            tof_mean += (tof - tof_mean) / valid_samples;
        }
    }

    if (valid_samples == 0) {
        return 0.0f;
    }

    /* Convert to nanoseconds and calculate distance */
    float tof_mean_ns = tof_mean / 2.0f;
    return tof_mean_ns * SPEED_OF_LIGHT_NM_PER_S;
}

int cs_distance_calculate(const uint8_t* local_data, uint16_t local_len,
    const uint8_t* peer_data, uint16_t peer_len,
    uint8_t n_ap, uint8_t role,
    cs_distance_result_t* result)
{
    if (!local_data || !peer_data || !result) {
        return -1;
    }

    cs_mode1_sample_t* mode1_data = (cs_mode1_sample_t*)malloc(
        CS_DISTANCE_MAX_SAMPLES * sizeof(cs_mode1_sample_t));
    cs_mode2_sample_t* mode2_data = (cs_mode2_sample_t*)malloc(
        CS_DISTANCE_MAX_SAMPLES * sizeof(cs_mode2_sample_t));
    if (!mode1_data || !mode2_data) {
        free(mode1_data);
        free(mode2_data);
        return -1;
    }

    memset(mode1_data, 0, CS_DISTANCE_MAX_SAMPLES * sizeof(cs_mode1_sample_t));
    memset(mode2_data, 0, CS_DISTANCE_MAX_SAMPLES * sizeof(cs_mode2_sample_t));
    memset(result, 0, sizeof(cs_distance_result_t));

    /* Parse local step data */
    cs_step_parse_ctx_t ctx = {
        .is_local = true,
        .mode1_idx = 0,
        .mode2_idx = 0,
        .n_ap = n_ap,
        .role = role,
        .mode1_data = mode1_data,
        .mode2_data = mode2_data,
    };

    cs_distance_parse_step_data(local_data, local_len, &ctx);

    uint8_t local_mode1_count = ctx.mode1_idx;
    uint8_t local_mode2_count = ctx.mode2_idx;

    /* Parse peer step data */
    ctx.is_local = false;
    ctx.mode1_idx = 0;
    ctx.mode2_idx = 0;

    cs_distance_parse_step_data(peer_data, peer_len, &ctx);

    uint8_t peer_mode1_count = ctx.mode1_idx;
    uint8_t peer_mode2_count = ctx.mode2_idx;

    BT_LOGD("Parsed: local_m1=%d, local_m2=%d, peer_m1=%d, peer_m2=%d",
        local_mode1_count, local_mode2_count, peer_mode1_count, peer_mode2_count);

    /* Calculate RTT distance (Mode 1) */
    uint8_t mode1_samples = (local_mode1_count < peer_mode1_count) ?
                            local_mode1_count : peer_mode1_count;
    if (mode1_samples > 0) {
        result->rtt_distance = cs_distance_estimate_rtt(mode1_data, mode1_samples);
        result->mode1_samples = mode1_samples;
        result->rtt_valid = (result->rtt_distance != 0.0f);
    }

    /* Calculate phase distance (Mode 2) */
    uint8_t mode2_samples = (local_mode2_count < peer_mode2_count) ?
                            local_mode2_count : peer_mode2_count;
    if (mode2_samples > 0) {
        result->phase_distance = cs_distance_estimate_phase_slope(mode2_data, mode2_samples);
        result->mode2_samples = mode2_samples;
        result->phase_valid = (result->phase_distance != 0.0f);
    }

    free(mode1_data);
    free(mode2_data);

    return 0;
}

