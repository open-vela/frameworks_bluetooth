/****************************************************************************
 * tests/unittest/service_device/include/cm_device.h
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

#ifndef __TESTING_CMOCKA_SERVICE_DEVICE_CM_DEVICE_H
#define __TESTING_CMOCKA_SERVICE_DEVICE_CM_DEVICE_H

int test_device_setup(FAR void **state);
int test_device_teardown(FAR void **state);

/* br_device_create / le_device_create / device_delete */

void test_br_device_create_normal(FAR void **state);
void test_le_device_create_normal(FAR void **state);
void test_device_delete_with_uuids(FAR void **state);

/* device_get/set_transport */

void test_device_get_transport_bredr(FAR void **state);
void test_device_get_transport_ble(FAR void **state);

/* device_get/set_address */

void test_device_get_address_normal(FAR void **state);

/* device_get/set_identity_address */

void test_device_set_identity_address_normal(FAR void **state);
void test_device_set_identity_address_null(FAR void **state);

/* device_get/set_address_type */

void test_device_get_set_address_type(FAR void **state);

/* device_get/set_device_type */

void test_device_get_set_device_type(FAR void **state);

/* device_get/set_name */

void test_device_set_name_normal(FAR void **state);
void test_device_set_name_same_returns_false(FAR void **state);
void test_device_set_name_updates_alias_if_empty(FAR void **state);
void test_device_set_name_no_update_alias_if_set(FAR void **state);

/* device_get/set_device_class */

void test_device_set_device_class_normal(FAR void **state);
void test_device_set_device_class_same_returns_false(FAR void **state);

/* device_get/set_uuids */

void test_device_set_uuids_normal(FAR void **state);
void test_device_get_uuids_partial(FAR void **state);
void test_device_get_uuids_empty(FAR void **state);

/* device_get/set_appearance */

void test_device_get_set_appearance(FAR void **state);

/* device_get/set_rssi */

void test_device_get_set_rssi(FAR void **state);
void test_device_get_set_rssi_negative(FAR void **state);

/* device_get/set_alias */

void test_device_set_alias_normal(FAR void **state);
void test_device_set_alias_same_returns_false(FAR void **state);

/* device_get/set_connection_state */

void test_device_get_set_connection_state(FAR void **state);
void test_device_is_connected_true(FAR void **state);
void test_device_is_connected_false(FAR void **state);
void test_device_is_encrypted_true(FAR void **state);
void test_device_is_encrypted_false(FAR void **state);

/* device_get/set_acl_handle */

void test_device_get_set_acl_handle(FAR void **state);

/* device_get/set_local_role */

void test_device_get_set_local_role(FAR void **state);

/* device_get/set_bond_initiate_local */

void test_device_bond_initiate_local(FAR void **state);

/* device_get/set_bond_state */

void test_device_get_bond_state_default(FAR void **state);
void test_device_set_bond_state_no_notify(FAR void **state);
void test_device_set_bond_state_same_no_change(FAR void **state);
void test_device_is_bonded(FAR void **state);

/* device_get/set_link_key */

void test_device_set_get_link_key(FAR void **state);
void test_device_delete_link_key(FAR void **state);

/* device_get/set_link_key_type */

void test_device_get_set_link_key_type(FAR void **state);

/* device_get/set_link_policy */

void test_device_get_set_link_policy(FAR void **state);

/* device_get/set_le_phy */

void test_device_get_set_le_phy(FAR void **state);

/* device_set/clear/check_flags */

void test_device_set_check_flags(FAR void **state);
void test_device_clear_flag(FAR void **state);
void test_device_flags_multiple(FAR void **state);

/* device_get/set/delete_smp_key */

void test_device_set_get_smp_key(FAR void **state);
void test_device_delete_smp_key(FAR void **state);

/* device_get/set_local_csrk */

void test_device_set_get_local_csrk(FAR void **state);

/* device_get_le_property */

void test_device_get_le_property(FAR void **state);

#endif /* __TESTING_CMOCKA_SERVICE_DEVICE_CM_DEVICE_H */
