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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_HFP_AG_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_HFP_AG_H

int test_bt_hfp_ag_setup(FAR void** state);
int test_bt_hfp_ag_teardown(FAR void** state);

void test_bt_hfp_ag_register_callbacks_normal(FAR void** state);
void test_bt_hfp_ag_register_callbacks_fail(FAR void** state);
void test_bt_hfp_ag_unregister_callbacks_normal(FAR void** state);
void test_bt_hfp_ag_unregister_callbacks_fail(FAR void** state);
void test_bt_hfp_ag_is_connected_true(FAR void** state);
void test_bt_hfp_ag_is_connected_false(FAR void** state);
void test_bt_hfp_ag_is_audio_connected_true(FAR void** state);
void test_bt_hfp_ag_is_audio_connected_false(FAR void** state);
void test_bt_hfp_ag_get_connection_state_connected(FAR void** state);
void test_bt_hfp_ag_get_connection_state_disconnected(FAR void** state);
void test_bt_hfp_ag_connect_normal(FAR void** state);
void test_bt_hfp_ag_connect_fail(FAR void** state);
void test_bt_hfp_ag_disconnect_normal(FAR void** state);
void test_bt_hfp_ag_disconnect_fail(FAR void** state);
void test_bt_hfp_ag_connect_audio_normal(FAR void** state);
void test_bt_hfp_ag_connect_audio_fail(FAR void** state);
void test_bt_hfp_ag_disconnect_audio_normal(FAR void** state);
void test_bt_hfp_ag_disconnect_audio_fail(FAR void** state);
void test_bt_hfp_ag_start_virtual_call_normal(FAR void** state);
void test_bt_hfp_ag_start_virtual_call_fail(FAR void** state);
void test_bt_hfp_ag_stop_virtual_call_normal(FAR void** state);
void test_bt_hfp_ag_stop_virtual_call_fail(FAR void** state);
void test_bt_hfp_ag_start_voice_recognition_normal(FAR void** state);
void test_bt_hfp_ag_start_voice_recognition_fail(FAR void** state);
void test_bt_hfp_ag_stop_voice_recognition_normal(FAR void** state);
void test_bt_hfp_ag_stop_voice_recognition_fail(FAR void** state);
void test_bt_hfp_ag_volume_control_normal(FAR void** state);
void test_bt_hfp_ag_volume_control_fail(FAR void** state);
void test_bt_hfp_ag_send_at_command_normal(FAR void** state);
void test_bt_hfp_ag_send_at_command_fail(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_HFP_AG_H */
