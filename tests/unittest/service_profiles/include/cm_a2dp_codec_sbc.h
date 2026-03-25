/****************************************************************************
 * tests/unittest/service_profiles/include/cm_a2dp_codec_sbc.h
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __TESTING_CMOCKA_SERVICE_PROFILES_CM_A2DP_CODEC_SBC_H
#define __TESTING_CMOCKA_SERVICE_PROFILES_CM_A2DP_CODEC_SBC_H

int test_a2dp_codec_sbc_setup(FAR void** state);
int test_a2dp_codec_sbc_teardown(FAR void** state);

/* a2dp_sbc_sample_frequency */

void test_sbc_sample_frequency_16k(FAR void** state);
void test_sbc_sample_frequency_32k(FAR void** state);
void test_sbc_sample_frequency_44k(FAR void** state);
void test_sbc_sample_frequency_48k(FAR void** state);

/* a2dp_sbc_frame_length */

void test_sbc_frame_length_normal(FAR void** state);
void test_sbc_frame_length_null_param(FAR void** state);
void test_sbc_frame_length_mono(FAR void** state);

/* a2dp_sbc_bit_rate */

void test_sbc_bit_rate_normal(FAR void** state);

/* a2dp_codec_parse_sbc_param */

void test_codec_parse_sbc_param_44k_joint_stereo(FAR void** state);
void test_codec_parse_sbc_param_48k_stereo(FAR void** state);
void test_codec_parse_sbc_param_null_info(FAR void** state);

/* a2dp_sbc_frame_sample */

void test_sbc_frame_sample_normal(FAR void** state);

/* a2dp_sbc_encoded_audio_bitrate */

void test_sbc_encoded_audio_bitrate_normal(FAR void** state);

#endif /* __TESTING_CMOCKA_SERVICE_PROFILES_CM_A2DP_CODEC_SBC_H */
