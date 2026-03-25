/****************************************************************************
 * tests/unittest/framework_api/include/cm_bt_le_advertiser.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_LE_ADVERTISER_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_LE_ADVERTISER_H

int test_bt_le_advertiser_setup(FAR void **state);
int test_bt_le_advertiser_teardown(FAR void **state);

void test_bt_le_start_advertising_normal(FAR void **state);
void test_bt_le_start_advertising_fail(FAR void **state);
void test_bt_le_start_advertising_null_params(FAR void **state);
void test_bt_le_start_advertising_null_adv_data(FAR void **state);
void test_bt_le_start_advertising_null_scan_rsp(FAR void **state);
void test_bt_le_start_advertising_returns_valid(FAR void **state);
void test_bt_le_stop_advertising_normal(FAR void **state);
void test_bt_le_stop_advertising_null_adver(FAR void **state);
void test_bt_le_stop_advertising_id_normal(FAR void **state);
void test_bt_le_stop_advertising_id_zero(FAR void **state);
void test_bt_le_advertising_is_supported_true(FAR void **state);
void test_bt_le_advertising_is_supported_false(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_LE_ADVERTISER_H */
