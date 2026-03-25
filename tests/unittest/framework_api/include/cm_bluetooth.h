/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#ifndef __TESTING_CMOCKA_FRAMEWORK_API_CM_BLUETOOTH_H
#define __TESTING_CMOCKA_FRAMEWORK_API_CM_BLUETOOTH_H

int test_bluetooth_setup(FAR void** state);
int test_bluetooth_teardown(FAR void** state);

void test_bluetooth_create_instance_normal(FAR void** state);
void test_bluetooth_create_instance_alloc_fail(FAR void** state);
void test_bluetooth_create_instance_manager_fail(FAR void** state);
void test_bluetooth_get_instance_existing(FAR void** state);
void test_bluetooth_get_instance_new(FAR void** state);
void test_bluetooth_get_proxy_hfp_hf(FAR void** state);
void test_bluetooth_get_proxy_default(FAR void** state);
void test_bluetooth_delete_instance_normal(FAR void** state);
void test_bluetooth_start_service_normal(FAR void** state);
void test_bluetooth_start_service_fail(FAR void** state);
void test_bluetooth_stop_service_normal(FAR void** state);
void test_bluetooth_stop_service_fail(FAR void** state);
void test_bluetooth_create_instance_sets_app_id(FAR void** state);
void test_bluetooth_get_instance_returns_existing(FAR void** state);
void test_bluetooth_get_proxy_null_proxy(FAR void** state);
void test_bluetooth_start_service_invalid_profile(FAR void** state);
void test_bluetooth_stop_service_invalid_profile(FAR void** state);
void test_bluetooth_delete_instance_frees_memory(FAR void** state);
void test_bluetooth_create_instance_getpid_called(FAR void** state);
void test_bluetooth_get_instance_getpid_called(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_API_CM_BLUETOOTH_H */
