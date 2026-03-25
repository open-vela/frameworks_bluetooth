/****************************************************************************
 * tests/unittest/framework_api/include/cm_bt_l2cap.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_L2CAP_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_L2CAP_H

int test_bt_l2cap_setup(FAR void **state);
int test_bt_l2cap_teardown(FAR void **state);

void test_bt_l2cap_register_callbacks_normal(FAR void **state);
void test_bt_l2cap_register_callbacks_fail(FAR void **state);
void test_bt_l2cap_unregister_callbacks_normal(FAR void **state);
void test_bt_l2cap_unregister_callbacks_fail(FAR void **state);
void test_bt_l2cap_listen_normal(FAR void **state);
void test_bt_l2cap_listen_fail(FAR void **state);
void test_bt_l2cap_connect_normal(FAR void **state);
void test_bt_l2cap_connect_fail(FAR void **state);
void test_bt_l2cap_disconnect_normal(FAR void **state);
void test_bt_l2cap_disconnect_fail(FAR void **state);
void test_bt_l2cap_stop_listen_normal(FAR void **state);
void test_bt_l2cap_stop_listen_fail(FAR void **state);
void test_bt_l2cap_stop_listen_with_transport_normal(FAR void **state);
void test_bt_l2cap_stop_listen_with_transport_fail(FAR void **state);
void test_bt_l2cap_register_callbacks_null_cbs(FAR void **state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BT_L2CAP_H */
