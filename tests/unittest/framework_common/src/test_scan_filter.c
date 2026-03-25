/****************************************************************************
 * tests/unittest/framework_common/src/test_scan_filter.c
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

// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on

#include <string.h>

/* Include source directly to test static functions */

#include "../../../../service/src/scan_filter.c"

#include "cm_scan_filter.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_scan_filter_setup(FAR void** state)
{
    return 0;
}

int test_scan_filter_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

void test_scanner_match_filter_uuid_match(FAR void** state)
{
    scan_record_t record;
    ble_scan_filter_t filter;

    memset(&record, 0, sizeof(record));
    memset(&filter, 0, sizeof(filter));

    record.uuid = 0x180D;
    filter.uuids[0] = 0x180D;

    assert_true(scanner_match_filter(&record, &filter));
}

void test_scanner_match_filter_uuid_no_match(FAR void** state)
{
    scan_record_t record;
    ble_scan_filter_t filter;

    memset(&record, 0, sizeof(record));
    memset(&filter, 0, sizeof(filter));

    record.uuid = 0x180D;
    filter.uuids[0] = 0x180F;
    filter.uuids[1] = 0x1810;

    assert_false(scanner_match_filter(&record, &filter));
}

void test_scanner_match_filter_no_uuid(FAR void** state)
{
    scan_record_t record;
    ble_scan_filter_t filter;

    memset(&record, 0, sizeof(record));
    memset(&filter, 0, sizeof(filter));

    /* record.uuid == 0, so first condition fails */

    filter.uuids[0] = 0x180D;

    assert_false(scanner_match_filter(&record, &filter));
}

void test_scanner_match_filter_second_slot_match(FAR void** state)
{
    scan_record_t record;
    ble_scan_filter_t filter;

    memset(&record, 0, sizeof(record));
    memset(&filter, 0, sizeof(filter));

    record.uuid = 0x180F;
    filter.uuids[0] = 0x180D;
    filter.uuids[1] = 0x180F;

    assert_true(scanner_match_filter(&record, &filter));
}

void test_scanner_match_filter_empty_filter(FAR void** state)
{
    scan_record_t record;
    ble_scan_filter_t filter;

    memset(&record, 0, sizeof(record));
    memset(&filter, 0, sizeof(filter));

    /* Both uuids are 0, record.uuid is non-zero but match_uuid
     * skips entries where uuids[i] == 0 */

    record.uuid = 0x180D;

    assert_false(scanner_match_filter(&record, &filter));
}
