/****************************************************************************
 * tests/unittest/service_manager/include/cm_service_manager.h
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

#ifndef __TESTING_CMOCKA_SERVICE_MANAGER_CM_SERVICE_MANAGER_H
#define __TESTING_CMOCKA_SERVICE_MANAGER_CM_SERVICE_MANAGER_H

int test_service_manager_setup(FAR void** state);
int test_service_manager_teardown(FAR void** state);

/* register_service */

void test_register_service_normal(FAR void** state);
void test_register_service_duplicate(FAR void** state);

/* service_manager_init */

void test_service_manager_init_normal(FAR void** state);
void test_service_manager_init_no_profiles(FAR void** state);

/* service_manager_startup */

void test_service_manager_startup_normal(FAR void** state);
void test_service_manager_startup_all_already_started(FAR void** state);

/* service_manager_shutdown */

void test_service_manager_shutdown_normal(FAR void** state);
void test_service_manager_shutdown_all_already_off(FAR void** state);

/* service_manager_get_uuid */

void test_service_manager_get_uuid_normal(FAR void** state);
void test_service_manager_get_uuid_empty(FAR void** state);

/* service_manager_processmsg */

void test_service_manager_processmsg_normal(FAR void** state);

/* service_manager_control */

void test_service_manager_control_start(FAR void** state);
void test_service_manager_control_stop(FAR void** state);
void test_service_manager_control_not_supported(FAR void** state);
void test_service_manager_control_dump(FAR void** state);

/* service_manager_cleanup */

void test_service_manager_cleanup_normal(FAR void** state);
void test_service_manager_cleanup_null_cleanup(FAR void** state);

#endif /* __TESTING_CMOCKA_SERVICE_MANAGER_CM_SERVICE_MANAGER_H */
