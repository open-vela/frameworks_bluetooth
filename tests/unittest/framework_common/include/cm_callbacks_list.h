/****************************************************************************
 * tests/unittest/framework_common/include/cm_callbacks_list.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_CALLBACKS_LIST_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_CALLBACKS_LIST_H

int test_callbacks_list_setup(FAR void **state);
int test_callbacks_list_teardown(FAR void **state);

void test_bt_callbacks_list_new_normal(FAR void **state);
void test_bt_callbacks_list_new_zero_max(FAR void **state);
void test_bt_callbacks_register_normal(FAR void **state);
void test_bt_callbacks_register_duplicate(FAR void **state);
void test_bt_callbacks_register_max_reached(FAR void **state);
void test_bt_callbacks_unregister_normal(FAR void **state);
void test_bt_callbacks_unregister_not_found(FAR void **state);
void test_bt_remote_callbacks_register_normal(FAR void **state);
void test_bt_remote_callbacks_register_duplicate_remote(FAR void **state);
void test_bt_remote_callbacks_unregister_normal(FAR void **state);
void test_bt_remote_callbacks_unregister_with_remote_out(FAR void **state);
void test_bt_callbacks_list_free_normal(FAR void **state);
void test_bt_callbacks_list_free_null(FAR void **state);
void test_bt_callbacks_list_count_normal(FAR void **state);
void test_bt_callbacks_list_count_after_register(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_CALLBACKS_LIST_H */
