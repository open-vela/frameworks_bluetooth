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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_ADAPTER_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_ADAPTER_H

int test_bt_adapter_setup(FAR void** state);
int test_bt_adapter_teardown(FAR void** state);

void test_bt_adapter_register_callback_normal(FAR void** state);
void test_bt_adapter_register_callback_null_cbs(FAR void** state);
void test_bt_adapter_unregister_callback_normal(FAR void** state);
void test_bt_adapter_unregister_callback_null_cookie(FAR void** state);
void test_bt_adapter_enable_normal(FAR void** state);
void test_bt_adapter_enable_fail(FAR void** state);
void test_bt_adapter_disable_normal(FAR void** state);
void test_bt_adapter_disable_fail(FAR void** state);
void test_bt_adapter_disable_safe_normal(FAR void** state);
void test_bt_adapter_enable_le_normal(FAR void** state);
void test_bt_adapter_disable_le_normal(FAR void** state);
void test_bt_adapter_get_state_normal(FAR void** state);
void test_bt_adapter_get_state_disabled(FAR void** state);
void test_bt_adapter_is_le_enabled_true(FAR void** state);
void test_bt_adapter_is_le_enabled_false(FAR void** state);
void test_bt_adapter_get_type_normal(FAR void** state);
void test_bt_adapter_set_discovery_filter_normal(FAR void** state);
void test_bt_adapter_start_discovery_normal(FAR void** state);
void test_bt_adapter_start_discovery_fail(FAR void** state);
void test_bt_adapter_start_discovery_zero_timeout(FAR void** state);
void test_bt_adapter_start_limited_discovery_normal(FAR void** state);
void test_bt_adapter_cancel_discovery_normal(FAR void** state);
void test_bt_adapter_cancel_discovery_fail(FAR void** state);
void test_bt_adapter_is_discovering_true(FAR void** state);
void test_bt_adapter_is_discovering_false(FAR void** state);
void test_bt_adapter_get_address_normal(FAR void** state);
void test_bt_adapter_set_name_normal(FAR void** state);
void test_bt_adapter_set_name_null(FAR void** state);
void test_bt_adapter_set_name_empty(FAR void** state);
void test_bt_adapter_get_name_normal(FAR void** state);
void test_bt_adapter_get_uuids_normal(FAR void** state);
void test_bt_adapter_set_scan_mode_normal(FAR void** state);
void test_bt_adapter_set_scan_mode_fail(FAR void** state);
void test_bt_adapter_get_scan_mode_normal(FAR void** state);
void test_bt_adapter_set_device_class_normal(FAR void** state);
void test_bt_adapter_get_device_class_normal(FAR void** state);
void test_bt_adapter_set_io_capability_normal(FAR void** state);
void test_bt_adapter_get_io_capability_normal(FAR void** state);
void test_bt_adapter_set_inquiry_scan_params_normal(FAR void** state);
void test_bt_adapter_set_page_scan_params_normal(FAR void** state);
void test_bt_adapter_set_le_io_capability_normal(FAR void** state);
void test_bt_adapter_get_le_io_capability_normal(FAR void** state);
void test_bt_adapter_get_le_address_normal(FAR void** state);
void test_bt_adapter_set_le_address_normal(FAR void** state);
void test_bt_adapter_set_le_identity_address_normal(FAR void** state);
void test_bt_adapter_set_le_appearance_normal(FAR void** state);
void test_bt_adapter_get_le_appearance_normal(FAR void** state);
void test_bt_adapter_le_enable_key_derivation_normal(FAR void** state);
void test_bt_adapter_le_add_whitelist_normal(FAR void** state);
void test_bt_adapter_le_add_whitelist_with_type_normal(FAR void** state);
void test_bt_adapter_le_remove_whitelist_normal(FAR void** state);
void test_bt_adapter_get_bonded_devices_normal(FAR void** state);
void test_bt_adapter_get_connected_devices_normal(FAR void** state);
void test_bt_adapter_set_afh_channel_normal(FAR void** state);
void test_bt_adapter_disconnect_all_devices_normal(FAR void** state);
void test_bt_adapter_is_support_bredr_true(FAR void** state);
void test_bt_adapter_is_support_bredr_false(FAR void** state);
void test_bt_adapter_is_support_le_true(FAR void** state);
void test_bt_adapter_is_support_le_false(FAR void** state);
void test_bt_adapter_is_support_leaudio_true(FAR void** state);
void test_bt_adapter_is_support_leaudio_false(FAR void** state);
void test_bt_adapter_set_debug_mode_normal(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_ADAPTER_H */
