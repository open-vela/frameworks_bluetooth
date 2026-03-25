/****************************************************************************
 * tests/unittest/service_profiles/cm_service_profiles_entry.c
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

// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on

#include "cm_a2dp_codec_sbc.h"
#include "cm_a2dp_codec_aac.h"
#include "cm_a2dp_event.h"
#include "cm_avrcp_msg.h"
#include "cm_hfp_hf_event.h"
#include "cm_hfp_ag_event.h"
#include "cm_gatt_event.h"

int cmocka_service_profiles_test_main(int argc, FAR char* argv[])
{
    const struct CMUnitTest service_profiles_tests[] =
    {
        /* a2dp_sbc_sample_frequency */

        cmocka_unit_test_setup_teardown(test_sbc_sample_frequency_16k,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),
        cmocka_unit_test_setup_teardown(test_sbc_sample_frequency_32k,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),
        cmocka_unit_test_setup_teardown(test_sbc_sample_frequency_44k,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),
        cmocka_unit_test_setup_teardown(test_sbc_sample_frequency_48k,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),

        /* a2dp_sbc_frame_length */

        cmocka_unit_test_setup_teardown(test_sbc_frame_length_normal,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),
        cmocka_unit_test_setup_teardown(test_sbc_frame_length_null_param,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),
        cmocka_unit_test_setup_teardown(test_sbc_frame_length_mono,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),

        /* a2dp_sbc_bit_rate */

        cmocka_unit_test_setup_teardown(test_sbc_bit_rate_normal,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),

        /* a2dp_codec_parse_sbc_param */

        cmocka_unit_test_setup_teardown(
            test_codec_parse_sbc_param_44k_joint_stereo,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),
        cmocka_unit_test_setup_teardown(
            test_codec_parse_sbc_param_48k_stereo,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),
        cmocka_unit_test_setup_teardown(
            test_codec_parse_sbc_param_null_info,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),

        /* a2dp_sbc_frame_sample */

        cmocka_unit_test_setup_teardown(test_sbc_frame_sample_normal,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),

        /* a2dp_sbc_encoded_audio_bitrate */

        cmocka_unit_test_setup_teardown(
            test_sbc_encoded_audio_bitrate_normal,
            test_a2dp_codec_sbc_setup, test_a2dp_codec_sbc_teardown),

        /* a2dp_parse_aac_info */

        cmocka_unit_test_setup_teardown(test_aac_parse_info_44k_stereo,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_parse_info_48k_mono,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_parse_info_null_params,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_parse_info_zero_fields,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),

        /* a2dp_get_aac_samplerate */

        cmocka_unit_test_setup_teardown(test_aac_samplerate_44100,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_samplerate_48000,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_samplerate_null,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_samplerate_invalid,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),

        /* a2dp_get_aac_number_of_channels */

        cmocka_unit_test_setup_teardown(test_aac_channels_mono,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_channels_stereo,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_channels_null,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_channels_invalid,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),

        /* a2dp_codec_parse_aac_param */

        cmocka_unit_test_setup_teardown(test_aac_parse_param_44k_stereo,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_parse_param_with_mtu,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),
        cmocka_unit_test_setup_teardown(test_aac_parse_param_null_params,
            test_a2dp_codec_aac_setup, test_a2dp_codec_aac_teardown),

        /* a2dp_event */

        cmocka_unit_test_setup_teardown(test_a2dp_event_new_normal,
            test_a2dp_event_setup, test_a2dp_event_teardown),
        cmocka_unit_test_setup_teardown(test_a2dp_event_new_null_addr,
            test_a2dp_event_setup, test_a2dp_event_teardown),
        cmocka_unit_test_setup_teardown(test_a2dp_event_new_ext_with_data,
            test_a2dp_event_setup, test_a2dp_event_teardown),
        cmocka_unit_test_setup_teardown(test_a2dp_event_new_ext_zero_size,
            test_a2dp_event_setup, test_a2dp_event_teardown),

        /* avrcp_msg */

        cmocka_unit_test_setup_teardown(test_avrcp_msg_new_normal,
            test_avrcp_msg_setup, test_avrcp_msg_teardown),
        cmocka_unit_test_setup_teardown(test_avrcp_msg_new_null_addr,
            test_avrcp_msg_setup, test_avrcp_msg_teardown),
        cmocka_unit_test_setup_teardown(test_avrcp_msg_destroy_with_attrs,
            test_avrcp_msg_setup, test_avrcp_msg_teardown),
        cmocka_unit_test_setup_teardown(test_avrcp_msg_destroy_no_attrs,
            test_avrcp_msg_setup, test_avrcp_msg_teardown),

        /* hfp_hf_event */

        cmocka_unit_test_setup_teardown(test_hfp_hf_msg_new_normal,
            test_hfp_hf_event_setup, test_hfp_hf_event_teardown),
        cmocka_unit_test_setup_teardown(test_hfp_hf_msg_new_null_addr,
            test_hfp_hf_event_setup, test_hfp_hf_event_teardown),
        cmocka_unit_test_setup_teardown(test_hfp_hf_msg_new_ext_with_data,
            test_hfp_hf_event_setup, test_hfp_hf_event_teardown),
        cmocka_unit_test_setup_teardown(test_hfp_hf_msg_destroy_null,
            test_hfp_hf_event_setup, test_hfp_hf_event_teardown),

        /* hfp_ag_event */

        cmocka_unit_test_setup_teardown(test_hfp_ag_msg_new_normal,
            test_hfp_ag_event_setup, test_hfp_ag_event_teardown),
        cmocka_unit_test_setup_teardown(test_hfp_ag_msg_new_null_addr,
            test_hfp_ag_event_setup, test_hfp_ag_event_teardown),
        cmocka_unit_test_setup_teardown(
            test_hfp_ag_event_new_ext_with_data,
            test_hfp_ag_event_setup, test_hfp_ag_event_teardown),
        cmocka_unit_test_setup_teardown(
            test_hfp_ag_event_new_ext_zero_size,
            test_hfp_ag_event_setup, test_hfp_ag_event_teardown),

        /* gatt events */

        cmocka_unit_test_setup_teardown(test_gatts_msg_new_normal,
            test_gatt_event_setup, test_gatt_event_teardown),
        cmocka_unit_test_setup_teardown(test_gatts_msg_new_with_payload,
            test_gatt_event_setup, test_gatt_event_teardown),
        cmocka_unit_test_setup_teardown(test_gatts_op_new_normal,
            test_gatt_event_setup, test_gatt_event_teardown),
        cmocka_unit_test_setup_teardown(test_gattc_msg_new_normal,
            test_gatt_event_setup, test_gatt_event_teardown),
        cmocka_unit_test_setup_teardown(test_gattc_msg_new_with_payload,
            test_gatt_event_setup, test_gatt_event_teardown),
        cmocka_unit_test_setup_teardown(test_gattc_op_new_normal,
            test_gatt_event_setup, test_gatt_event_teardown),
    };

    return cmocka_run_group_tests(service_profiles_tests, NULL, NULL);
}
