/****************************************************************************
 * tests/unittest/framework_api/include/cm_bt_avrcp_control.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_AVRCP_CONTROL_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_AVRCP_CONTROL_H

int test_bt_avrcp_control_setup(FAR void **state);
int test_bt_avrcp_control_teardown(FAR void **state);

void test_bt_avrcp_control_register_callbacks_normal(FAR void **state);
void test_bt_avrcp_control_register_callbacks_fail(FAR void **state);
void test_bt_avrcp_control_unregister_callbacks_normal(FAR void **state);
void test_bt_avrcp_control_unregister_callbacks_fail(FAR void **state);
void test_bt_avrcp_control_get_element_attributes_normal(FAR void **state);
void test_bt_avrcp_control_get_element_attributes_fail(FAR void **state);
void test_bt_avrcp_control_send_passthrough_cmd_normal(FAR void **state);
void test_bt_avrcp_control_send_passthrough_cmd_fail(FAR void **state);
void test_bt_avrcp_control_get_unit_info_normal(FAR void **state);
void test_bt_avrcp_control_get_unit_info_fail(FAR void **state);
void test_bt_avrcp_control_get_subunit_info_normal(FAR void **state);
void test_bt_avrcp_control_get_subunit_info_fail(FAR void **state);
void test_bt_avrcp_control_get_playback_state_normal(FAR void **state);
void test_bt_avrcp_control_get_playback_state_fail(FAR void **state);
void test_bt_avrcp_control_register_notification_normal(FAR void **state);
void test_bt_avrcp_control_register_notification_fail(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_AVRCP_CONTROL_H */
