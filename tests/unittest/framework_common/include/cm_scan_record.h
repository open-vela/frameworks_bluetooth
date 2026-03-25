/****************************************************************************
 * tests/unittest/framework_common/include/cm_scan_record.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_SCAN_RECORD_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_SCAN_RECORD_H

int test_scan_record_setup(FAR void** state);
int test_scan_record_teardown(FAR void** state);

void test_scan_record_parse_null_data(FAR void** state);
void test_scan_record_parse_empty_data(FAR void** state);
void test_scan_record_parse_uuid16_service_data(FAR void** state);
void test_scan_record_parse_unknown_type(FAR void** state);
void test_scan_record_parse_zero_field_len(FAR void** state);
void test_scan_record_parse_truncated_data(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_SCAN_RECORD_H */
