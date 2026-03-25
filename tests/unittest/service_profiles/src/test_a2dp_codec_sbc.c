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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <string.h>

/****************************************************************************
 * Include the headers for the unit under test.
 ****************************************************************************/

#include "a2dp_codec_sbc.h"

#include "cm_a2dp_codec_sbc.h"

/****************************************************************************
 * Setup / Teardown
 ****************************************************************************/

int test_a2dp_codec_sbc_setup(FAR void** state)
{
    return 0;
}

int test_a2dp_codec_sbc_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Test Cases: a2dp_sbc_sample_frequency
 ****************************************************************************/

void test_sbc_sample_frequency_16k(FAR void** state)
{
    uint16_t freq = a2dp_sbc_sample_frequency(SBC_SF_16000);
    assert_int_equal(freq, 16000);
}

void test_sbc_sample_frequency_32k(FAR void** state)
{
    uint16_t freq = a2dp_sbc_sample_frequency(SBC_SF_32000);
    assert_int_equal(freq, 32000);
}

void test_sbc_sample_frequency_44k(FAR void** state)
{
    uint16_t freq = a2dp_sbc_sample_frequency(SBC_SF_44100);
    assert_int_equal(freq, 44100);
}

void test_sbc_sample_frequency_48k(FAR void** state)
{
    uint16_t freq = a2dp_sbc_sample_frequency(SBC_SF_48000);
    assert_int_equal(freq, 48000);
}

/****************************************************************************
 * Test Cases: a2dp_sbc_frame_length
 ****************************************************************************/

void test_sbc_frame_length_normal(FAR void** state)
{
    /* Joint stereo, 8 subbands, 16 blocks, bitpool=32 */

    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    param.s16ChannelMode = SBC_JOINT_STEREO;
    param.s16NumOfSubBands = SUB_BANDS_8;
    param.s16NumOfChannels = SBC_MAX_NUM_OF_CHANNELS;
    param.s16NumOfBlocks = SBC_BLOCK_3; /* 16 blocks */
    param.s16BitPool = 32;

    uint32_t len = a2dp_sbc_frame_length(&param);
    assert_true(len > 0);
}

void test_sbc_frame_length_null_param(FAR void** state)
{
    uint32_t len = a2dp_sbc_frame_length(NULL);
    assert_int_equal(len, 0);
}

void test_sbc_frame_length_mono(FAR void** state)
{
    /* Mono mode should return 0 (not supported) */

    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    param.s16ChannelMode = SBC_MONO;
    param.s16NumOfSubBands = SUB_BANDS_8;
    param.s16NumOfChannels = 1;
    param.s16NumOfBlocks = SBC_BLOCK_3;
    param.s16BitPool = 32;

    uint32_t len = a2dp_sbc_frame_length(&param);
    assert_int_equal(len, 0);
}

/****************************************************************************
 * Test Cases: a2dp_sbc_bit_rate
 ****************************************************************************/

void test_sbc_bit_rate_normal(FAR void** state)
{
    /* Joint stereo, 8 subbands, 16 blocks, bitpool=53, 44.1kHz */

    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    param.s16SamplingFreq = SBC_SF_44100;
    param.s16ChannelMode = SBC_JOINT_STEREO;
    param.s16NumOfSubBands = SUB_BANDS_8;
    param.s16NumOfChannels = SBC_MAX_NUM_OF_CHANNELS;
    param.s16NumOfBlocks = SBC_BLOCK_3; /* 16 blocks */
    param.s16BitPool = 53;

    uint32_t rate = a2dp_sbc_bit_rate(&param);
    assert_true(rate > 0);
}

/****************************************************************************
 * Test Cases: a2dp_codec_parse_sbc_param
 ****************************************************************************/

void test_codec_parse_sbc_param_44k_joint_stereo(FAR void** state)
{
    /* Build codec_info: 44.1kHz + Joint Stereo, 16 blocks + 8 subbands
     * + Loudness, min_bitpool=2, max_bitpool=53 */

    uint8_t codec_info[4];
    codec_info[0] = BT_A2DP_SBC_SAMP_FREQ_44 | BT_A2DP_SBC_CH_MD_JOINT;
    codec_info[1] = BT_A2DP_SBC_BLOCKS_16 | BT_A2DP_SBC_SUBBAND_8
        | BT_A2DP_SBC_ALLOC_MD_L;
    codec_info[2] = 2; /* min_bitpool */
    codec_info[3] = 53; /* max_bitpool */

    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    a2dp_codec_parse_sbc_param(&param, codec_info);

    assert_int_equal(param.s16SamplingFreq, SBC_SF_44100);
    assert_int_equal(param.s16ChannelMode, SBC_JOINT_STEREO);
    assert_int_equal(param.s16NumOfSubBands, SUB_BANDS_8);
    assert_int_equal(param.s16NumOfBlocks, SBC_BLOCK_3); /* 16 blocks */
    assert_int_equal(param.s16AllocationMethod, SBC_LOUDNESS);
    assert_int_equal(param.s16BitPool, 53);
    assert_true(param.u32BitRate > 0);
}

void test_codec_parse_sbc_param_48k_stereo(FAR void** state)
{
    /* 48kHz + Stereo, 12 blocks + 4 subbands + SNR,
     * min_bitpool=2, max_bitpool=35 */

    uint8_t codec_info[4];
    codec_info[0] = BT_A2DP_SBC_SAMP_FREQ_48 | BT_A2DP_SBC_CH_MD_STEREO;
    codec_info[1] = BT_A2DP_SBC_BLOCKS_12 | BT_A2DP_SBC_SUBBAND_4
        | BT_A2DP_SBC_ALLOC_MD_S;
    codec_info[2] = 2; /* min_bitpool */
    codec_info[3] = 35; /* max_bitpool */

    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    a2dp_codec_parse_sbc_param(&param, codec_info);

    assert_int_equal(param.s16SamplingFreq, SBC_SF_48000);
    assert_int_equal(param.s16ChannelMode, SBC_STEREO);
    assert_int_equal(param.s16NumOfSubBands, SUB_BANDS_4);
    assert_int_equal(param.s16NumOfBlocks, SBC_BLOCK_2); /* 12 blocks */
    assert_int_equal(param.s16AllocationMethod, SBC_SNR);
    assert_int_equal(param.s16BitPool, 35);
    assert_true(param.u32BitRate > 0);
}

void test_codec_parse_sbc_param_null_info(FAR void** state)
{
    /* NULL codec_info should not crash (a2dp_parse_sbc_info returns -1) */

    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    a2dp_codec_parse_sbc_param(&param, NULL);

    /* param should remain zeroed since parse failed */

    assert_int_equal(param.s16SamplingFreq, 0);
    assert_int_equal(param.s16BitPool, 0);
}

/****************************************************************************
 * Test Cases: a2dp_sbc_frame_sample
 ****************************************************************************/

void test_sbc_frame_sample_normal(FAR void** state)
{
    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    param.s16NumOfSubBands = SUB_BANDS_8;
    param.s16NumOfBlocks = SBC_BLOCK_3; /* 16 */

    uint16_t samples = a2dp_sbc_frame_sample(&param);
    assert_int_equal(samples, 8 * 16); /* 128 */
}

/****************************************************************************
 * Test Cases: a2dp_sbc_encoded_audio_bitrate
 ****************************************************************************/

void test_sbc_encoded_audio_bitrate_normal(FAR void** state)
{
    sbc_param_t param;
    memset(&param, 0, sizeof(param));
    param.s16SamplingFreq = SBC_SF_44100;
    param.s16ChannelMode = SBC_JOINT_STEREO;
    param.s16NumOfSubBands = SUB_BANDS_8;
    param.s16NumOfChannels = SBC_MAX_NUM_OF_CHANNELS;
    param.s16NumOfBlocks = SBC_BLOCK_3;
    param.s16BitPool = 53;

    uint32_t rate = a2dp_sbc_encoded_audio_bitrate(&param);
    assert_true(rate > 0);
    /* Should equal a2dp_sbc_bit_rate */
    assert_int_equal(rate, a2dp_sbc_bit_rate(&param));
}
