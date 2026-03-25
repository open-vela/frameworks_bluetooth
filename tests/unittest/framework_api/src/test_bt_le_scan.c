/****************************************************************************
 * tests/unittest/framework_api/src/test_bt_le_scan.c
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

/* clang-format off */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
/* clang-format on */

#include <stdlib.h>
#include <string.h>

#include "bt_le_scan.h"
#include "bt_status.h"

#include "cm_bt_le_scan.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/


/****************************************************************************
 * Private Functions - Mock functions
 ****************************************************************************/

static void *g_mock_scanner = (void *)0xBEEF;

bt_scanner_t *scanner_start_scan(void *remote, const scanner_callbacks_t *cbs)
{
    check_expected_ptr(cbs);
    return mock_ptr_type(bt_scanner_t *);
}

bt_scanner_t *scanner_start_scan_settings(void *remote,
    ble_scan_settings_t *settings, const scanner_callbacks_t *cbs)
{
    check_expected_ptr(settings);
    check_expected_ptr(cbs);
    return mock_ptr_type(bt_scanner_t *);
}

bt_scanner_t *scanner_start_scan_with_filters(void *remote,
    ble_scan_settings_t *settings, ble_scan_filter_t *filter,
    const scanner_callbacks_t *cbs)
{
    check_expected_ptr(settings);
    check_expected_ptr(filter);
    check_expected_ptr(cbs);
    return mock_ptr_type(bt_scanner_t *);
}

void scanner_stop_scan(bt_scanner_t *scanner)
{
    check_expected_ptr(scanner);
}

bool scan_is_supported(void)
{
    return mock_type(bool);
}

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_le_scan_setup(FAR void **state)
{
    bt_instance_t *ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    *state = ins;
    return 0;
}

int test_bt_le_scan_teardown(FAR void **state)
{
    if (*state) {
        test_free(*state);
        *state = NULL;
    }
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

void test_bt_le_start_scan_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    scanner_callbacks_t cbs;

    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan, cbs);
    will_return(scanner_start_scan, g_mock_scanner);

    bt_scanner_t *scanner = bt_le_start_scan(ins, &cbs);
    assert_ptr_equal(scanner, g_mock_scanner);
}

void test_bt_le_start_scan_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    scanner_callbacks_t cbs;

    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan, cbs);
    will_return(scanner_start_scan, NULL);

    bt_scanner_t *scanner = bt_le_start_scan(ins, &cbs);
    assert_null(scanner);
}

void test_bt_le_start_scan_null_cbs(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(scanner_start_scan, cbs);
    will_return(scanner_start_scan, NULL);

    bt_scanner_t *scanner = bt_le_start_scan(ins, NULL);
    assert_null(scanner);
}

void test_bt_le_start_scan_settings_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_scan_settings_t settings;
    scanner_callbacks_t cbs;

    memset(&settings, 0, sizeof(settings));
    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan_settings, settings);
    expect_any(scanner_start_scan_settings, cbs);
    will_return(scanner_start_scan_settings, g_mock_scanner);

    bt_scanner_t *scanner = bt_le_start_scan_settings(ins, &settings, &cbs);
    assert_ptr_equal(scanner, g_mock_scanner);
}

void test_bt_le_start_scan_settings_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_scan_settings_t settings;
    scanner_callbacks_t cbs;

    memset(&settings, 0, sizeof(settings));
    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan_settings, settings);
    expect_any(scanner_start_scan_settings, cbs);
    will_return(scanner_start_scan_settings, NULL);

    bt_scanner_t *scanner = bt_le_start_scan_settings(ins, &settings, &cbs);
    assert_null(scanner);
}

void test_bt_le_start_scan_settings_null_settings(FAR void **state)
{
    bt_instance_t *ins = *state;
    scanner_callbacks_t cbs;

    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan_settings, settings);
    expect_any(scanner_start_scan_settings, cbs);
    will_return(scanner_start_scan_settings, NULL);

    bt_scanner_t *scanner = bt_le_start_scan_settings(ins, NULL, &cbs);
    assert_null(scanner);
}

void test_bt_le_start_scan_with_filters_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_scan_settings_t settings;
    ble_scan_filter_t filter;
    scanner_callbacks_t cbs;

    memset(&settings, 0, sizeof(settings));
    memset(&filter, 0, sizeof(filter));
    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan_with_filters, settings);
    expect_any(scanner_start_scan_with_filters, filter);
    expect_any(scanner_start_scan_with_filters, cbs);
    will_return(scanner_start_scan_with_filters, g_mock_scanner);

    bt_scanner_t *scanner = bt_le_start_scan_with_filters(ins,
        &settings, &filter, &cbs);
    assert_ptr_equal(scanner, g_mock_scanner);
}

void test_bt_le_start_scan_with_filters_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_scan_settings_t settings;
    ble_scan_filter_t filter;
    scanner_callbacks_t cbs;

    memset(&settings, 0, sizeof(settings));
    memset(&filter, 0, sizeof(filter));
    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan_with_filters, settings);
    expect_any(scanner_start_scan_with_filters, filter);
    expect_any(scanner_start_scan_with_filters, cbs);
    will_return(scanner_start_scan_with_filters, NULL);

    bt_scanner_t *scanner = bt_le_start_scan_with_filters(ins,
        &settings, &filter, &cbs);
    assert_null(scanner);
}

void test_bt_le_start_scan_with_filters_null_filter(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_scan_settings_t settings;
    scanner_callbacks_t cbs;

    memset(&settings, 0, sizeof(settings));
    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan_with_filters, settings);
    expect_any(scanner_start_scan_with_filters, filter);
    expect_any(scanner_start_scan_with_filters, cbs);
    will_return(scanner_start_scan_with_filters, NULL);

    bt_scanner_t *scanner = bt_le_start_scan_with_filters(ins,
        &settings, NULL, &cbs);
    assert_null(scanner);
}

void test_bt_le_stop_scan_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(scanner_stop_scan, scanner);
    bt_le_stop_scan(ins, g_mock_scanner);
}

void test_bt_le_stop_scan_null_scanner(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(scanner_stop_scan, scanner);
    bt_le_stop_scan(ins, NULL);
}

void test_bt_le_scan_is_supported_true(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(scan_is_supported, true);
    assert_true(bt_le_scan_is_supported(ins));
}

void test_bt_le_scan_is_supported_false(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(scan_is_supported, false);
    assert_false(bt_le_scan_is_supported(ins));
}

void test_bt_le_start_scan_returns_valid_scanner(FAR void **state)
{
    bt_instance_t *ins = *state;
    scanner_callbacks_t cbs;

    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan, cbs);
    will_return(scanner_start_scan, g_mock_scanner);

    bt_scanner_t *scanner = bt_le_start_scan(ins, &cbs);
    assert_non_null(scanner);
}

void test_bt_le_start_scan_settings_returns_valid_scanner(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_scan_settings_t settings;
    scanner_callbacks_t cbs;

    memset(&settings, 0, sizeof(settings));
    memset(&cbs, 0, sizeof(cbs));
    expect_any(scanner_start_scan_settings, settings);
    expect_any(scanner_start_scan_settings, cbs);
    will_return(scanner_start_scan_settings, g_mock_scanner);

    bt_scanner_t *scanner = bt_le_start_scan_settings(ins, &settings, &cbs);
    assert_non_null(scanner);
}
