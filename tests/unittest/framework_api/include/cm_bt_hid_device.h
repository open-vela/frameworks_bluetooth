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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_HID_DEVICE_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_HID_DEVICE_H

int test_bt_hid_device_setup(FAR void** state);
int test_bt_hid_device_teardown(FAR void** state);

void test_bt_hid_device_register_callbacks_normal(FAR void** state);
void test_bt_hid_device_register_callbacks_fail(FAR void** state);
void test_bt_hid_device_unregister_callbacks_normal(FAR void** state);
void test_bt_hid_device_unregister_callbacks_fail(FAR void** state);
void test_bt_hid_device_register_app_normal(FAR void** state);
void test_bt_hid_device_register_app_fail(FAR void** state);
void test_bt_hid_device_unregister_app_normal(FAR void** state);
void test_bt_hid_device_unregister_app_fail(FAR void** state);
void test_bt_hid_device_connect_normal(FAR void** state);
void test_bt_hid_device_connect_fail(FAR void** state);
void test_bt_hid_device_disconnect_normal(FAR void** state);
void test_bt_hid_device_disconnect_fail(FAR void** state);
void test_bt_hid_device_send_report_normal(FAR void** state);
void test_bt_hid_device_send_report_fail(FAR void** state);
void test_bt_hid_device_response_report_normal(FAR void** state);
void test_bt_hid_device_response_report_fail(FAR void** state);
void test_bt_hid_device_report_error_normal(FAR void** state);
void test_bt_hid_device_report_error_fail(FAR void** state);
void test_bt_hid_device_virtual_unplug_normal(FAR void** state);
void test_bt_hid_device_virtual_unplug_fail(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_HID_DEVICE_H */
