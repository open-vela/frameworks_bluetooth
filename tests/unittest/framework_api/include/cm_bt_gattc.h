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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_GATTC_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_GATTC_H

int test_bt_gattc_setup(FAR void** state);
int test_bt_gattc_teardown(FAR void** state);

void test_bt_gattc_create_connect_normal(FAR void** state);
void test_bt_gattc_create_connect_fail(FAR void** state);
void test_bt_gattc_delete_connect_normal(FAR void** state);
void test_bt_gattc_delete_connect_fail(FAR void** state);
void test_bt_gattc_connect_normal(FAR void** state);
void test_bt_gattc_connect_fail(FAR void** state);
void test_bt_gattc_disconnect_normal(FAR void** state);
void test_bt_gattc_disconnect_fail(FAR void** state);
void test_bt_gattc_discover_service_normal(FAR void** state);
void test_bt_gattc_discover_service_fail(FAR void** state);
void test_bt_gattc_get_attribute_by_handle_normal(FAR void** state);
void test_bt_gattc_get_attribute_by_handle_fail(FAR void** state);
void test_bt_gattc_get_attribute_by_uuid_normal(FAR void** state);
void test_bt_gattc_get_attribute_by_uuid_fail(FAR void** state);
void test_bt_gattc_read_normal(FAR void** state);
void test_bt_gattc_read_fail(FAR void** state);
void test_bt_gattc_write_normal(FAR void** state);
void test_bt_gattc_write_fail(FAR void** state);
void test_bt_gattc_write_without_response_normal(FAR void** state);
void test_bt_gattc_write_without_response_fail(FAR void** state);
void test_bt_gattc_write_with_signed_normal(FAR void** state);
void test_bt_gattc_write_with_signed_fail(FAR void** state);
void test_bt_gattc_subscribe_normal(FAR void** state);
void test_bt_gattc_subscribe_fail(FAR void** state);
void test_bt_gattc_unsubscribe_normal(FAR void** state);
void test_bt_gattc_unsubscribe_fail(FAR void** state);
void test_bt_gattc_exchange_mtu_normal(FAR void** state);
void test_bt_gattc_exchange_mtu_fail(FAR void** state);
void test_bt_gattc_update_connection_parameter_normal(FAR void** state);
void test_bt_gattc_update_connection_parameter_fail(FAR void** state);
void test_bt_gattc_read_phy_normal(FAR void** state);
void test_bt_gattc_read_phy_fail(FAR void** state);
void test_bt_gattc_update_phy_normal(FAR void** state);
void test_bt_gattc_update_phy_fail(FAR void** state);
void test_bt_gattc_read_rssi_normal(FAR void** state);
void test_bt_gattc_read_rssi_fail(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_GATTC_H */
