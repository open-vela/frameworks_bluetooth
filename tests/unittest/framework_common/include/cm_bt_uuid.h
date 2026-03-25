/****************************************************************************
 * tests/unittest/framework_common/include/cm_bt_uuid.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_UUID_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_UUID_H

int test_bt_uuid_setup(FAR void **state);
int test_bt_uuid_teardown(FAR void **state);

/* bt_uuid16_create */

void test_bt_uuid16_create_normal(FAR void **state);

/* bt_uuid32_create */

void test_bt_uuid32_create_normal(FAR void **state);

/* bt_uuid128_create */

void test_bt_uuid128_create_normal(FAR void **state);

/* bt_uuid_create_common */

void test_bt_uuid_create_common_uuid16(FAR void **state);
void test_bt_uuid_create_common_uuid32(FAR void **state);
void test_bt_uuid_create_common_uuid128(FAR void **state);
void test_bt_uuid_create_common_invalid_type(FAR void **state);

/* bt_uuid_to_uuid128 */

void test_bt_uuid_to_uuid128_from_uuid16(FAR void **state);
void test_bt_uuid_to_uuid128_from_uuid32(FAR void **state);
void test_bt_uuid_to_uuid128_from_uuid128(FAR void **state);

/* bt_uuid_to_uuid16 */

void test_bt_uuid_to_uuid16_from_uuid128(FAR void **state);
void test_bt_uuid_to_uuid16_from_uuid16(FAR void **state);

/* bt_uuid_compare */

void test_bt_uuid_compare_equal(FAR void **state);
void test_bt_uuid_compare_not_equal(FAR void **state);
void test_bt_uuid_compare_cross_type(FAR void **state);

/* bt_uuid_to_string */

void test_bt_uuid_to_string_uuid16(FAR void **state);
void test_bt_uuid_to_string_uuid128(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_UUID_H */
