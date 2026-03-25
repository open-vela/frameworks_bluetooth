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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_SCAN_FILTER_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_SCAN_FILTER_H

int test_scan_filter_setup(FAR void** state);
int test_scan_filter_teardown(FAR void** state);

void test_scanner_match_filter_uuid_match(FAR void** state);
void test_scanner_match_filter_uuid_no_match(FAR void** state);
void test_scanner_match_filter_no_uuid(FAR void** state);
void test_scanner_match_filter_second_slot_match(FAR void** state);
void test_scanner_match_filter_empty_filter(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_SCAN_FILTER_H */
