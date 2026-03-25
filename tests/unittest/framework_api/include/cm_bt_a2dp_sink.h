/****************************************************************************
 * tests/unittest/framework_api/include/cm_bt_a2dp_sink.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_A2DP_SINK_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_A2DP_SINK_H

int test_bt_a2dp_sink_setup(FAR void **state);
int test_bt_a2dp_sink_teardown(FAR void **state);

void test_bt_a2dp_sink_register_callbacks_normal(FAR void **state);
void test_bt_a2dp_sink_register_callbacks_fail(FAR void **state);
void test_bt_a2dp_sink_unregister_callbacks_normal(FAR void **state);
void test_bt_a2dp_sink_unregister_callbacks_fail(FAR void **state);
void test_bt_a2dp_sink_is_connected_true(FAR void **state);
void test_bt_a2dp_sink_is_connected_false(FAR void **state);
void test_bt_a2dp_sink_is_playing_true(FAR void **state);
void test_bt_a2dp_sink_is_playing_false(FAR void **state);
void test_bt_a2dp_sink_get_connection_state_connected(FAR void **state);
void test_bt_a2dp_sink_get_connection_state_disconnected(FAR void **state);
void test_bt_a2dp_sink_connect_normal(FAR void **state);
void test_bt_a2dp_sink_connect_fail(FAR void **state);
void test_bt_a2dp_sink_disconnect_normal(FAR void **state);
void test_bt_a2dp_sink_disconnect_fail(FAR void **state);
void test_bt_a2dp_sink_set_active_device_normal(FAR void **state);
void test_bt_a2dp_sink_set_active_device_fail(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_A2DP_SINK_H */
