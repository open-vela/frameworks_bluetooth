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

#include "a2dp_codec_aac.h"

typedef struct {
    uint8_t object_type;
    uint16_t sample_rate;
    uint8_t channel_mode;
    uint8_t variable_bit_rate;
    uint32_t bit_rate;
} a2dp_aac_info_t;

extern int a2dp_parse_aac_info(a2dp_aac_info_t* info, uint8_t* codec_info);
extern int a2dp_get_aac_samplerate(a2dp_aac_info_t* info);
extern int a2dp_get_aac_number_of_channels(a2dp_aac_info_t* info);

#include "cm_a2dp_codec_aac.h"

/****************************************************************************
 * Setup / Teardown
 ****************************************************************************/

int test_a2dp_codec_aac_setup(FAR void** state)
{
    return 0;
}

int test_a2dp_codec_aac_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Test Cases: a2dp_parse_aac_info (static)
 ****************************************************************************/

void test_aac_parse_info_44k_stereo(FAR void** state)
{
    /* Build codec_info for 44.1kHz stereo, MPEG-2 LC, VBR, bitrate=320000
     * Octet 0: object_type = 0x80 (MPEG-2 LC)
     * Octet 1: sample_rate low byte = 0x01 (44100)
     * Octet 2: sample_rate high nibble=0x00, channel=stereo(0x04),
     *          VBR=0x00 => 0x04
     * Octet 3-5: bit_rate = 320000 = 0x04E200
     *   Octet 3: (0x04E200 >> 16) & 0x7F = 0x04
     *   Octet 4: (0x04E200 >> 8) & 0xFF = 0xE2
     *   Octet 5: 0x04E200 & 0xFF = 0x00
     */

    a2dp_aac_info_t info;
    uint8_t codec_info[6];
    memset(&info, 0, sizeof(info));
    codec_info[0] = A2DP_AAC_OBJECT_TYPE_MPEG2_LC;
    codec_info[1] = A2DP_AAC_SAMPLING_FREQ_44100;
    codec_info[2] = A2DP_AAC_CHANNEL_MODE_STEREO;
    codec_info[3] = 0x04;
    codec_info[4] = 0xE2;
    codec_info[5] = 0x00;

    int ret = a2dp_parse_aac_info(&info, codec_info);
    assert_int_equal(ret, 0);
    assert_int_equal(info.object_type, A2DP_AAC_OBJECT_TYPE_MPEG2_LC);
    assert_int_equal(info.channel_mode, A2DP_AAC_CHANNEL_MODE_STEREO);
    assert_true(info.bit_rate > 0);
}

void test_aac_parse_info_48k_mono(FAR void** state)
{
    /* 48kHz is in the high nibble of octet 2 */

    a2dp_aac_info_t info;
    uint8_t codec_info[6];
    memset(&info, 0, sizeof(info));
    codec_info[0] = A2DP_AAC_OBJECT_TYPE_MPEG4_LC;
    codec_info[1] = 0x00; /* low byte of sample_rate = 0 */
    codec_info[2] = 0x80 /* high nibble: 48kHz */
        | A2DP_AAC_CHANNEL_MODE_MONO;
    codec_info[3] = 0x01;
    codec_info[4] = 0x00;
    codec_info[5] = 0x00;

    int ret = a2dp_parse_aac_info(&info, codec_info);
    assert_int_equal(ret, 0);
    assert_int_equal(info.object_type, A2DP_AAC_OBJECT_TYPE_MPEG4_LC);
    assert_int_equal(info.channel_mode, A2DP_AAC_CHANNEL_MODE_MONO);
}

void test_aac_parse_info_null_params(FAR void** state)
{
    a2dp_aac_info_t info;
    uint8_t codec_info[6] = { 0 };

    assert_int_equal(a2dp_parse_aac_info(NULL, codec_info), -1);
    assert_int_equal(a2dp_parse_aac_info(&info, NULL), -1);
    assert_int_equal(a2dp_parse_aac_info(NULL, NULL), -1);
}

void test_aac_parse_info_zero_fields(FAR void** state)
{
    /* All zeros => object_type=0, sample_rate=0, channel_mode=0 => -1 */

    a2dp_aac_info_t info;
    uint8_t codec_info[6] = { 0 };

    int ret = a2dp_parse_aac_info(&info, codec_info);
    assert_int_equal(ret, -1);
}

/****************************************************************************
 * Test Cases: a2dp_get_aac_samplerate (static)
 ****************************************************************************/

void test_aac_samplerate_44100(FAR void** state)
{
    a2dp_aac_info_t info;
    memset(&info, 0, sizeof(info));
    info.sample_rate = A2DP_AAC_SAMPLING_FREQ_44100;
    assert_int_equal(a2dp_get_aac_samplerate(&info), 44100);
}

void test_aac_samplerate_48000(FAR void** state)
{
    a2dp_aac_info_t info;
    memset(&info, 0, sizeof(info));
    info.sample_rate = A2DP_AAC_SAMPLING_FREQ_48000;
    assert_int_equal(a2dp_get_aac_samplerate(&info), 48000);
}

void test_aac_samplerate_null(FAR void** state)
{
    assert_int_equal(a2dp_get_aac_samplerate(NULL), -1);
}

void test_aac_samplerate_invalid(FAR void** state)
{
    a2dp_aac_info_t info;
    memset(&info, 0, sizeof(info));
    info.sample_rate = 0xFFFF;
    assert_int_equal(a2dp_get_aac_samplerate(&info), -1);
}

/****************************************************************************
 * Test Cases: a2dp_get_aac_number_of_channels (static)
 ****************************************************************************/

void test_aac_channels_mono(FAR void** state)
{
    a2dp_aac_info_t info;
    memset(&info, 0, sizeof(info));
    info.channel_mode = A2DP_AAC_CHANNEL_MODE_MONO;
    assert_int_equal(a2dp_get_aac_number_of_channels(&info), 1);
}

void test_aac_channels_stereo(FAR void** state)
{
    a2dp_aac_info_t info;
    memset(&info, 0, sizeof(info));
    info.channel_mode = A2DP_AAC_CHANNEL_MODE_STEREO;
    assert_int_equal(a2dp_get_aac_number_of_channels(&info), 2);
}

void test_aac_channels_null(FAR void** state)
{
    assert_int_equal(a2dp_get_aac_number_of_channels(NULL), -1);
}

void test_aac_channels_invalid(FAR void** state)
{
    a2dp_aac_info_t info;
    memset(&info, 0, sizeof(info));
    info.channel_mode = 0xFF;
    assert_int_equal(a2dp_get_aac_number_of_channels(&info), -1);
}

/****************************************************************************
 * Test Cases: a2dp_codec_parse_aac_param (public)
 ****************************************************************************/

void test_aac_parse_param_44k_stereo(FAR void** state)
{
    uint8_t codec_info[6];
    codec_info[0] = A2DP_AAC_OBJECT_TYPE_MPEG2_LC;
    codec_info[1] = A2DP_AAC_SAMPLING_FREQ_44100;
    codec_info[2] = A2DP_AAC_CHANNEL_MODE_STEREO;
    codec_info[3] = 0x04;
    codec_info[4] = 0xE2;
    codec_info[5] = 0x00;

    aac_encoder_param_t param;
    memset(&param, 0, sizeof(param));
    int ret = a2dp_codec_parse_aac_param(&param, codec_info, 0);
    assert_int_equal(ret, 0);
    assert_int_equal(param.u32SampleRate, 44100);
    assert_int_equal(param.u16NumOfChannels, 2);
    assert_true(param.u32BitRate > 0);
}

void test_aac_parse_param_with_mtu(FAR void** state)
{
    uint8_t codec_info[6];
    codec_info[0] = A2DP_AAC_OBJECT_TYPE_MPEG2_LC;
    codec_info[1] = A2DP_AAC_SAMPLING_FREQ_44100;
    codec_info[2] = A2DP_AAC_CHANNEL_MODE_STEREO;
    /* Set a high bit_rate so MTU-based rate is lower */
    codec_info[3] = 0x07;
    codec_info[4] = 0xFF;
    codec_info[5] = 0xFF;

    aac_encoder_param_t param;
    memset(&param, 0, sizeof(param));
    int ret = a2dp_codec_parse_aac_param(&param, codec_info, 663);
    assert_int_equal(ret, 0);
    /* With MTU, bitrate should be capped */
    assert_true(param.u32BitRate > 0);
}

void test_aac_parse_param_null_params(FAR void** state)
{
    uint8_t codec_info[6] = { 0 };
    aac_encoder_param_t param;

    assert_int_equal(a2dp_codec_parse_aac_param(NULL, codec_info, 0), -1);
    assert_int_equal(a2dp_codec_parse_aac_param(&param, NULL, 0), -1);
}
