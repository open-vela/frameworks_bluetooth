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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_LE_SCAN_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_LE_SCAN_H

int test_bt_le_scan_setup(FAR void** state);
int test_bt_le_scan_teardown(FAR void** state);

void test_bt_le_start_scan_normal(FAR void** state);
void test_bt_le_start_scan_fail(FAR void** state);
void test_bt_le_start_scan_null_cbs(FAR void** state);
void test_bt_le_start_scan_settings_normal(FAR void** state);
void test_bt_le_start_scan_settings_fail(FAR void** state);
void test_bt_le_start_scan_settings_null_settings(FAR void** state);
void test_bt_le_start_scan_with_filters_normal(FAR void** state);
void test_bt_le_start_scan_with_filters_fail(FAR void** state);
void test_bt_le_start_scan_with_filters_null_filter(FAR void** state);
void test_bt_le_stop_scan_normal(FAR void** state);
void test_bt_le_stop_scan_null_scanner(FAR void** state);
void test_bt_le_scan_is_supported_true(FAR void** state);
void test_bt_le_scan_is_supported_false(FAR void** state);
void test_bt_le_start_scan_returns_valid_scanner(FAR void** state);
void test_bt_le_start_scan_settings_returns_valid_scanner(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_LE_SCAN_H */
