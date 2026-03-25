/****************************************************************************
 * tests/unittest/framework_api/include/cm_bt_gatts.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_GATTS_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_GATTS_H

int test_bt_gatts_setup(FAR void **state);
int test_bt_gatts_teardown(FAR void **state);

void test_bt_gatts_register_service_normal(FAR void **state);
void test_bt_gatts_register_service_fail(FAR void **state);
void test_bt_gatts_unregister_service_normal(FAR void **state);
void test_bt_gatts_unregister_service_fail(FAR void **state);
void test_bt_gatts_connect_normal(FAR void **state);
void test_bt_gatts_connect_fail(FAR void **state);
void test_bt_gatts_connect_bear_normal(FAR void **state);
void test_bt_gatts_connect_bear_fail(FAR void **state);
void test_bt_gatts_disconnect_normal(FAR void **state);
void test_bt_gatts_disconnect_fail(FAR void **state);
void test_bt_gatts_add_attr_table_normal(FAR void **state);
void test_bt_gatts_add_attr_table_fail(FAR void **state);
void test_bt_gatts_remove_attr_table_normal(FAR void **state);
void test_bt_gatts_remove_attr_table_fail(FAR void **state);
void test_bt_gatts_set_attr_value_normal(FAR void **state);
void test_bt_gatts_set_attr_value_fail(FAR void **state);
void test_bt_gatts_get_attr_value_normal(FAR void **state);
void test_bt_gatts_get_attr_value_fail(FAR void **state);
void test_bt_gatts_response_normal(FAR void **state);
void test_bt_gatts_response_fail(FAR void **state);
void test_bt_gatts_notify_normal(FAR void **state);
void test_bt_gatts_notify_fail(FAR void **state);
void test_bt_gatts_indicate_normal(FAR void **state);
void test_bt_gatts_indicate_fail(FAR void **state);
void test_bt_gatts_read_phy_normal(FAR void **state);
void test_bt_gatts_read_phy_fail(FAR void **state);
void test_bt_gatts_update_phy_normal(FAR void **state);
void test_bt_gatts_update_phy_fail(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_GATTS_H */
