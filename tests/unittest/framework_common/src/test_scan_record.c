/****************************************************************************
 * tests/unittest/framework_common/src/test_scan_record.c
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

#include "../../../../service/src/scan_record.c"

#include "cm_scan_record.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_scan_record_setup(FAR void** state)
{
    return 0;
}

int test_scan_record_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

void test_scan_record_parse_null_data(FAR void** state)
{
    scan_record_t record;

    memset(&record, 0, sizeof(record));
    scan_record_parse(&record, NULL, 10);

    /* Should not crash, record unchanged */

    assert_int_equal(record.uuid, 0);
}

void test_scan_record_parse_empty_data(FAR void** state)
{
    scan_record_t record;
    uint8_t eir_data[] = { 0 };

    memset(&record, 0, sizeof(record));
    scan_record_parse(&record, eir_data, 1);

    /* field_len == 0 triggers break */

    assert_int_equal(record.uuid, 0);
}

void test_scan_record_parse_uuid16_service_data(FAR void** state)
{
    scan_record_t record;

    /* EIR: length=3, type=0x16 (SVC_DATA_16), uuid16=0x1234 (LE) */

    uint8_t eir_data[] = { 0x03, 0x16, 0x34, 0x12 };

    memset(&record, 0, sizeof(record));
    scan_record_parse(&record, eir_data, sizeof(eir_data));

    assert_int_equal(record.uuid, 0x1234);
}

void test_scan_record_parse_unknown_type(FAR void** state)
{
    scan_record_t record;

    /* EIR: length=2, type=0xFF (manufacturer data), data=0xAA */

    uint8_t eir_data[] = { 0x02, 0xFF, 0xAA };

    memset(&record, 0, sizeof(record));
    scan_record_parse(&record, eir_data, sizeof(eir_data));

    /* Unknown type is ignored, uuid stays 0 */

    assert_int_equal(record.uuid, 0);
}

void test_scan_record_parse_zero_field_len(FAR void** state)
{
    scan_record_t record;

    /* First field valid, second field has length 0 -> break */

    uint8_t eir_data[] = { 0x03, 0x16, 0x56, 0x78, 0x00 };

    memset(&record, 0, sizeof(record));
    scan_record_parse(&record, eir_data, sizeof(eir_data));

    /* First field parsed, then stops at zero-length */

    assert_int_equal(record.uuid, 0x7856);
}

void test_scan_record_parse_truncated_data(FAR void** state)
{
    scan_record_t record;

    /* field_len says 5 bytes follow, but eir_len is only 3 */

    uint8_t eir_data[] = { 0x05, 0x16, 0x34 };

    memset(&record, 0, sizeof(record));
    scan_record_parse(&record, eir_data, 3);

    /* len + field_len + 1 > eir_len, should break early */

    assert_int_equal(record.uuid, 0);
}
