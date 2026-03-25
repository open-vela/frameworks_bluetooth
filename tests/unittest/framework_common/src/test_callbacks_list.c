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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdlib.h>
#include <string.h>

#include <stdbool.h>

#include "bt_list.h"
#include "callbacks_list.h"

#include "cm_callbacks_list.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_callbacks_list_setup(FAR void** state)
{
    return 0;
}

int test_callbacks_list_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases: bt_callbacks_list_new
 ****************************************************************************/

void test_bt_callbacks_list_new_normal(FAR void** state)
{
    callbacks_list_t* cbsl;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);
    assert_non_null(cbsl->list);
    assert_int_equal(cbsl->max_reg, 5);
    assert_int_equal(cbsl->registed, 0);
    bt_callbacks_list_free(cbsl);
}

void test_bt_callbacks_list_new_zero_max(FAR void** state)
{
    callbacks_list_t* cbsl;

    cbsl = bt_callbacks_list_new(0);
    assert_non_null(cbsl);
    assert_int_equal(cbsl->max_reg, 0);
    assert_int_equal(cbsl->registed, 0);
    bt_callbacks_list_free(cbsl);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_callbacks_register
 ****************************************************************************/

void test_bt_callbacks_register_normal(FAR void** state)
{
    callbacks_list_t* cbsl;
    int dummy_cb = 42;
    remote_callback_t* rcbk;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    rcbk = bt_callbacks_register(cbsl, &dummy_cb);
    assert_non_null(rcbk);
    assert_ptr_equal(rcbk->callbacks, &dummy_cb);
    assert_null(rcbk->remote);
    assert_int_equal(cbsl->registed, 1);

    bt_callbacks_list_free(cbsl);
}

void test_bt_callbacks_register_duplicate(FAR void** state)
{
    callbacks_list_t* cbsl;
    int dummy_cb = 42;
    remote_callback_t* rcbk1;
    remote_callback_t* rcbk2;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    rcbk1 = bt_callbacks_register(cbsl, &dummy_cb);
    assert_non_null(rcbk1);

    rcbk2 = bt_callbacks_register(cbsl, &dummy_cb);
    assert_null(rcbk2);
    assert_int_equal(cbsl->registed, 1);

    bt_callbacks_list_free(cbsl);
}

void test_bt_callbacks_register_max_reached(FAR void** state)
{
    callbacks_list_t* cbsl;
    int cb1 = 1;
    int cb2 = 2;
    remote_callback_t* rcbk;

    cbsl = bt_callbacks_list_new(1);
    assert_non_null(cbsl);

    rcbk = bt_callbacks_register(cbsl, &cb1);
    assert_non_null(rcbk);

    rcbk = bt_callbacks_register(cbsl, &cb2);
    assert_null(rcbk);
    assert_int_equal(cbsl->registed, 1);

    bt_callbacks_list_free(cbsl);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_callbacks_unregister
 ****************************************************************************/

void test_bt_callbacks_unregister_normal(FAR void** state)
{
    callbacks_list_t* cbsl;
    int dummy_cb = 42;
    remote_callback_t* rcbk;
    bool ret;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    rcbk = bt_callbacks_register(cbsl, &dummy_cb);
    assert_non_null(rcbk);
    assert_int_equal(cbsl->registed, 1);

    ret = bt_callbacks_unregister(cbsl, rcbk);
    assert_true(ret);
    assert_int_equal(cbsl->registed, 0);

    bt_callbacks_list_free(cbsl);
}

void test_bt_callbacks_unregister_not_found(FAR void** state)
{
    callbacks_list_t* cbsl;
    remote_callback_t fake;
    bool ret;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    memset(&fake, 0, sizeof(fake));
    ret = bt_callbacks_unregister(cbsl, &fake);
    assert_false(ret);

    bt_callbacks_list_free(cbsl);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_remote_callbacks_register
 ****************************************************************************/

void test_bt_remote_callbacks_register_normal(FAR void** state)
{
    callbacks_list_t* cbsl;
    int dummy_cb = 42;
    int dummy_remote = 99;
    remote_callback_t* rcbk;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    rcbk = bt_remote_callbacks_register(cbsl, &dummy_remote, &dummy_cb);
    assert_non_null(rcbk);
    assert_ptr_equal(rcbk->callbacks, &dummy_cb);
    assert_ptr_equal(rcbk->remote, &dummy_remote);
    assert_int_equal(cbsl->registed, 1);

    bt_callbacks_list_free(cbsl);
}

void test_bt_remote_callbacks_register_duplicate_remote(FAR void** state)
{
    callbacks_list_t* cbsl;
    int dummy_cb1 = 42;
    int dummy_cb2 = 43;
    int dummy_remote = 99;
    remote_callback_t* rcbk1;
    remote_callback_t* rcbk2;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    rcbk1 = bt_remote_callbacks_register(cbsl, &dummy_remote, &dummy_cb1);
    assert_non_null(rcbk1);

    rcbk2 = bt_remote_callbacks_register(cbsl, &dummy_remote, &dummy_cb2);
    assert_null(rcbk2);
    assert_int_equal(cbsl->registed, 1);

    bt_callbacks_list_free(cbsl);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_remote_callbacks_unregister
 ****************************************************************************/

void test_bt_remote_callbacks_unregister_normal(FAR void** state)
{
    callbacks_list_t* cbsl;
    int dummy_cb = 42;
    int dummy_remote = 99;
    remote_callback_t* rcbk;
    bool ret;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    rcbk = bt_remote_callbacks_register(cbsl, &dummy_remote, &dummy_cb);
    assert_non_null(rcbk);

    ret = bt_remote_callbacks_unregister(cbsl, NULL, rcbk);
    assert_true(ret);
    assert_int_equal(cbsl->registed, 0);

    bt_callbacks_list_free(cbsl);
}

void test_bt_remote_callbacks_unregister_with_remote_out(FAR void** state)
{
    callbacks_list_t* cbsl;
    int dummy_cb = 42;
    int dummy_remote = 99;
    remote_callback_t* rcbk;
    void* remote_out = NULL;
    bool ret;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    rcbk = bt_remote_callbacks_register(cbsl, &dummy_remote, &dummy_cb);
    assert_non_null(rcbk);

    ret = bt_remote_callbacks_unregister(cbsl, &remote_out, rcbk);
    assert_true(ret);
    assert_ptr_equal(remote_out, &dummy_remote);
    assert_int_equal(cbsl->registed, 0);

    bt_callbacks_list_free(cbsl);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_callbacks_list_free
 ****************************************************************************/

void test_bt_callbacks_list_free_normal(FAR void** state)
{
    callbacks_list_t* cbsl;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);
    bt_callbacks_list_free(cbsl);
}

void test_bt_callbacks_list_free_null(FAR void** state)
{
    /* should not crash */

    bt_callbacks_list_free(NULL);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_callbacks_list_count
 ****************************************************************************/

void test_bt_callbacks_list_count_normal(FAR void** state)
{
    callbacks_list_t* cbsl;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);
    assert_int_equal(bt_callbacks_list_count(cbsl), 0);
    bt_callbacks_list_free(cbsl);
}

void test_bt_callbacks_list_count_after_register(FAR void** state)
{
    callbacks_list_t* cbsl;
    int cb1 = 1;
    int cb2 = 2;

    cbsl = bt_callbacks_list_new(5);
    assert_non_null(cbsl);

    bt_callbacks_register(cbsl, &cb1);
    assert_int_equal(bt_callbacks_list_count(cbsl), 1);

    bt_callbacks_register(cbsl, &cb2);
    assert_int_equal(bt_callbacks_list_count(cbsl), 2);

    bt_callbacks_list_free(cbsl);
}
