/****************************************************************************
 * tests/unittest/framework_common/include/cm_state_machine.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_STATE_MACHINE_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_STATE_MACHINE_H

int test_state_machine_setup(FAR void **state);
int test_state_machine_teardown(FAR void **state);

void test_hsm_ctor_normal(FAR void **state);
void test_hsm_ctor_null(FAR void **state);
void test_hsm_dtor_normal(FAR void **state);
void test_hsm_transition_to_normal(FAR void **state);
void test_hsm_transition_to_from_existing(FAR void **state);
void test_hsm_get_current_state_normal(FAR void **state);
void test_hsm_get_current_state_null(FAR void **state);
void test_hsm_get_previous_state_normal(FAR void **state);
void test_hsm_get_previous_state_null(FAR void **state);
void test_hsm_get_current_state_name_normal(FAR void **state);
void test_hsm_get_current_state_name_null_sm(FAR void **state);
void test_hsm_get_current_state_name_null_state(FAR void **state);
void test_hsm_get_state_name_normal(FAR void **state);
void test_hsm_get_state_name_null(FAR void **state);
void test_hsm_get_state_value_normal(FAR void **state);
void test_hsm_get_current_state_value_normal(FAR void **state);
void test_hsm_dispatch_event_normal(FAR void **state);
void test_hsm_dispatch_event_null_state(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_STATE_MACHINE_H */
