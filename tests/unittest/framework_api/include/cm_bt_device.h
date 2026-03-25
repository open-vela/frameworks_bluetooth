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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_DEVICE_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_DEVICE_H

int test_bt_device_setup(FAR void** state);
int test_bt_device_teardown(FAR void** state);

void test_bt_device_get_identity_address_normal(FAR void** state);
void test_bt_device_get_identity_address_fail(FAR void** state);
void test_bt_device_get_address_type_normal(FAR void** state);
void test_bt_device_get_address_type_unknown(FAR void** state);
void test_bt_device_get_device_type_normal(FAR void** state);
void test_bt_device_get_device_type_not_found(FAR void** state);
void test_bt_device_get_name_normal(FAR void** state);
void test_bt_device_get_name_fail(FAR void** state);
void test_bt_device_get_device_class_normal(FAR void** state);
void test_bt_device_get_device_class_not_found(FAR void** state);
void test_bt_device_get_uuids_normal(FAR void** state);
void test_bt_device_get_uuids_fail(FAR void** state);
void test_bt_device_get_appearance_normal(FAR void** state);
void test_bt_device_get_appearance_zero(FAR void** state);
void test_bt_device_get_rssi_normal(FAR void** state);
void test_bt_device_get_rssi_zero(FAR void** state);
void test_bt_device_get_alias_normal(FAR void** state);
void test_bt_device_get_alias_fail(FAR void** state);
void test_bt_device_set_alias_normal(FAR void** state);
void test_bt_device_set_alias_fail(FAR void** state);
void test_bt_device_is_connected_true(FAR void** state);
void test_bt_device_is_connected_false(FAR void** state);
void test_bt_device_is_encrypted_true(FAR void** state);
void test_bt_device_is_encrypted_false(FAR void** state);
void test_bt_device_is_bond_initiate_local_true(FAR void** state);
void test_bt_device_is_bond_initiate_local_false(FAR void** state);
void test_bt_device_get_bond_state_bonded(FAR void** state);
void test_bt_device_get_bond_state_none(FAR void** state);
void test_bt_device_is_bonded_true(FAR void** state);
void test_bt_device_is_bonded_false(FAR void** state);
void test_bt_device_connect_normal(FAR void** state);
void test_bt_device_connect_fail(FAR void** state);
void test_bt_device_disconnect_normal(FAR void** state);
void test_bt_device_disconnect_fail(FAR void** state);
void test_bt_device_connect_le_normal(FAR void** state);
void test_bt_device_connect_le_fail(FAR void** state);
void test_bt_device_disconnect_le_normal(FAR void** state);
void test_bt_device_disconnect_le_fail(FAR void** state);
void test_bt_device_connect_request_reply_accept(FAR void** state);
void test_bt_device_connect_request_reply_reject(FAR void** state);
void test_bt_device_connect_all_profile_normal(FAR void** state);
void test_bt_device_disconnect_all_profile_normal(FAR void** state);
void test_bt_device_set_le_phy_normal(FAR void** state);
void test_bt_device_set_le_phy_fail(FAR void** state);
void test_bt_device_create_bond_normal(FAR void** state);
void test_bt_device_create_bond_fail(FAR void** state);
void test_bt_device_set_security_level_normal(FAR void** state);
void test_bt_device_set_security_level_fail(FAR void** state);
void test_bt_device_set_bondable_le_normal(FAR void** state);
void test_bt_device_set_bondable_le_fail(FAR void** state);
void test_bt_device_remove_bond_normal(FAR void** state);
void test_bt_device_remove_bond_fail(FAR void** state);
void test_bt_device_cancel_bond_normal(FAR void** state);
void test_bt_device_cancel_bond_fail(FAR void** state);
void test_bt_device_pair_request_reply_accept(FAR void** state);
void test_bt_device_pair_request_reply_reject(FAR void** state);
void test_bt_device_set_pairing_confirmation_accept(FAR void** state);
void test_bt_device_set_pairing_confirmation_reject(FAR void** state);
void test_bt_device_set_pin_code_normal(FAR void** state);
void test_bt_device_set_pin_code_fail(FAR void** state);
void test_bt_device_set_pass_key_normal(FAR void** state);
void test_bt_device_set_pass_key_fail(FAR void** state);
void test_bt_device_set_le_legacy_tk_normal(FAR void** state);
void test_bt_device_set_le_legacy_tk_fail(FAR void** state);
void test_bt_device_set_le_sc_remote_oob_data_normal(FAR void** state);
void test_bt_device_set_le_sc_remote_oob_data_fail(FAR void** state);
void test_bt_device_get_le_sc_local_oob_data_normal(FAR void** state);
void test_bt_device_get_le_sc_local_oob_data_fail(FAR void** state);
void test_bt_device_enable_enhanced_mode_unsupported(FAR void** state);
void test_bt_device_disable_enhanced_mode_unsupported(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_DEVICE_H */
