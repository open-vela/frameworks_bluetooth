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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_ADVERTISER_DATA_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_ADVERTISER_DATA_H

int test_advertiser_data_setup(FAR void** state);
int test_advertiser_data_teardown(FAR void** state);

void test_advertiser_data_new_normal(FAR void** state);
void test_advertiser_data_set_flags_normal(FAR void** state);
void test_advertiser_data_set_name_short(FAR void** state);
void test_advertiser_data_set_name_complete(FAR void** state);
void test_advertiser_data_set_appearance_normal(FAR void** state);
void test_advertiser_data_add_data_normal(FAR void** state);
void test_advertiser_data_add_manufacture_data_normal(FAR void** state);
void test_advertiser_data_add_service_uuid16(FAR void** state);
void test_advertiser_data_add_service_uuid128(FAR void** state);
void test_advertiser_data_add_service_uuid_invalid(FAR void** state);
void test_advertiser_data_build_empty(FAR void** state);
void test_advertiser_data_build_with_flags(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_ADVERTISER_DATA_H */
