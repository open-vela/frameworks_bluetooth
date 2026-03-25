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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_TIME_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_TIME_H

int test_bt_time_setup(FAR void** state);
int test_bt_time_teardown(FAR void** state);

void test_bt_get_os_timestamp_us_normal(FAR void** state);
void test_bt_get_os_timestamp_us_monotonic(FAR void** state);
void test_bt_get_os_timestamp_ms_normal(FAR void** state);
void test_bt_get_os_timestamp_ms_monotonic(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_TIME_H */
