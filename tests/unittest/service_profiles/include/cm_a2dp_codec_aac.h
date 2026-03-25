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

#ifndef __TESTING_CM_SERVICE_PROFILES_A2DP_CODEC_AAC_H
#define __TESTING_CM_SERVICE_PROFILES_A2DP_CODEC_AAC_H

int test_a2dp_codec_aac_setup(FAR void** state);
int test_a2dp_codec_aac_teardown(FAR void** state);

/* a2dp_parse_aac_info */

void test_aac_parse_info_44k_stereo(FAR void** state);
void test_aac_parse_info_48k_mono(FAR void** state);
void test_aac_parse_info_null_params(FAR void** state);
void test_aac_parse_info_zero_fields(FAR void** state);

/* a2dp_get_aac_samplerate */

void test_aac_samplerate_44100(FAR void** state);
void test_aac_samplerate_48000(FAR void** state);
void test_aac_samplerate_null(FAR void** state);
void test_aac_samplerate_invalid(FAR void** state);

/* a2dp_get_aac_number_of_channels */

void test_aac_channels_mono(FAR void** state);
void test_aac_channels_stereo(FAR void** state);
void test_aac_channels_null(FAR void** state);
void test_aac_channels_invalid(FAR void** state);

/* a2dp_codec_parse_aac_param */

void test_aac_parse_param_44k_stereo(FAR void** state);
void test_aac_parse_param_with_mtu(FAR void** state);
void test_aac_parse_param_null_params(FAR void** state);

#endif /* __TESTING_CM_SERVICE_PROFILES_A2DP_CODEC_AAC_H */
