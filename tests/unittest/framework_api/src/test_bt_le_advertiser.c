/****************************************************************************
 * tests/unittest/framework_api/src/test_bt_le_advertiser.c
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

#include "bt_le_advertiser.h"
#include "bt_status.h"

#include "cm_bt_le_advertiser.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/


/****************************************************************************
 * Private Functions - Mock functions
 ****************************************************************************/

static void *g_mock_adver = (void *)0xADDE;

bt_advertiser_t *start_advertising(void *remote,
    ble_adv_params_t *params, uint8_t *adv_data, uint16_t adv_len,
    uint8_t *scan_rsp_data, uint16_t scan_rsp_len,
    advertiser_callback_t *cbs)
{
    return mock_ptr_type(bt_advertiser_t *);
}

void stop_advertising(bt_advertiser_t *adver)
{
    check_expected_ptr(adver);
}

void stop_advertising_id(uint8_t adv_id)
{
    check_expected(adv_id);
}

bool advertising_is_supported(void)
{
    return mock_type(bool);
}

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_le_advertiser_setup(FAR void **state)
{
    bt_instance_t *ins = test_calloc(1, sizeof(bt_instance_t));
    assert_non_null(ins);
    ins->app_id = 1;
    *state = ins;
    return 0;
}

int test_bt_le_advertiser_teardown(FAR void **state)
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

void test_bt_le_start_advertising_normal(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_adv_params_t params;
    uint8_t adv_data[10] = {0};
    uint8_t scan_rsp[10] = {0};
    advertiser_callback_t cbs;

    memset(&params, 0, sizeof(params));
    memset(&cbs, 0, sizeof(cbs));
    will_return(start_advertising, g_mock_adver);

    bt_advertiser_t *adver = bt_le_start_advertising(ins, &params,
        adv_data, 10, scan_rsp, 10, &cbs);
    assert_ptr_equal(adver, g_mock_adver);
}

void test_bt_le_start_advertising_fail(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_adv_params_t params;
    advertiser_callback_t cbs;

    memset(&params, 0, sizeof(params));
    memset(&cbs, 0, sizeof(cbs));
    will_return(start_advertising, NULL);

    bt_advertiser_t *adver = bt_le_start_advertising(ins, &params,
        NULL, 0, NULL, 0, &cbs);
    assert_null(adver);
}

void test_bt_le_start_advertising_null_params(FAR void **state)
{
    bt_instance_t *ins = *state;
    advertiser_callback_t cbs;

    memset(&cbs, 0, sizeof(cbs));
    will_return(start_advertising, NULL);

    bt_advertiser_t *adver = bt_le_start_advertising(ins, NULL,
        NULL, 0, NULL, 0, &cbs);
    assert_null(adver);
}

void test_bt_le_start_advertising_null_adv_data(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_adv_params_t params;
    advertiser_callback_t cbs;

    memset(&params, 0, sizeof(params));
    memset(&cbs, 0, sizeof(cbs));
    will_return(start_advertising, g_mock_adver);

    bt_advertiser_t *adver = bt_le_start_advertising(ins, &params,
        NULL, 0, NULL, 0, &cbs);
    assert_ptr_equal(adver, g_mock_adver);
}

void test_bt_le_start_advertising_null_scan_rsp(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_adv_params_t params;
    uint8_t adv_data[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
    advertiser_callback_t cbs;

    memset(&params, 0, sizeof(params));
    memset(&cbs, 0, sizeof(cbs));
    will_return(start_advertising, g_mock_adver);

    bt_advertiser_t *adver = bt_le_start_advertising(ins, &params,
        adv_data, 5, NULL, 0, &cbs);
    assert_ptr_equal(adver, g_mock_adver);
}

void test_bt_le_start_advertising_returns_valid(FAR void **state)
{
    bt_instance_t *ins = *state;
    ble_adv_params_t params;
    advertiser_callback_t cbs;

    memset(&params, 0, sizeof(params));
    memset(&cbs, 0, sizeof(cbs));
    will_return(start_advertising, g_mock_adver);

    bt_advertiser_t *adver = bt_le_start_advertising(ins, &params,
        NULL, 0, NULL, 0, &cbs);
    assert_non_null(adver);
}

void test_bt_le_stop_advertising_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(stop_advertising, adver);
    bt_le_stop_advertising(ins, g_mock_adver);
}

void test_bt_le_stop_advertising_null_adver(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_any(stop_advertising, adver);
    bt_le_stop_advertising(ins, NULL);
}

void test_bt_le_stop_advertising_id_normal(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(stop_advertising_id, adv_id, 1);
    bt_le_stop_advertising_id(ins, 1);
}

void test_bt_le_stop_advertising_id_zero(FAR void **state)
{
    bt_instance_t *ins = *state;

    expect_value(stop_advertising_id, adv_id, 0);
    bt_le_stop_advertising_id(ins, 0);
}

void test_bt_le_advertising_is_supported_true(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(advertising_is_supported, true);
    assert_true(bt_le_advertising_is_supported(ins));
}

void test_bt_le_advertising_is_supported_false(FAR void **state)
{
    bt_instance_t *ins = *state;

    will_return(advertising_is_supported, false);
    assert_false(bt_le_advertising_is_supported(ins));
}
