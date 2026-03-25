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

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bt_list.h"

#include "cm_bt_list.h"

/****************************************************************************
 * Private Functions - Helpers
 ****************************************************************************/

static int g_free_count;

static void test_free_cb(void* data)
{
    g_free_count++;
}

static void iter_sum_cb(void* data, void* context)
{
    int* sum = (int*)context;
    *sum += *(int*)data;
}

static bool find_value_cb(void* data, void* context)
{
    return *(int*)data == *(int*)context;
}

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_list_setup(FAR void** state)
{
    g_free_count = 0;
    return 0;
}

int test_bt_list_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_new / bt_list_free
 ****************************************************************************/

void test_bt_list_new_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);

    assert_non_null(list);
    assert_true(bt_list_is_empty(list));
    assert_int_equal(bt_list_length(list), 0);
    bt_list_free(list);
}

void test_bt_list_new_with_free_cb(FAR void** state)
{
    bt_list_t* list = bt_list_new(test_free_cb);
    int val = 42;

    assert_non_null(list);
    bt_list_add_tail(list, &val);
    bt_list_free(list);
    assert_int_equal(g_free_count, 1);
}

void test_bt_list_free_null(FAR void** state)
{
    /* Should not crash */

    bt_list_free(NULL);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_is_empty / bt_list_length
 ****************************************************************************/

void test_bt_list_is_empty_new_list(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);

    assert_true(bt_list_is_empty(list));
    bt_list_free(list);
}

void test_bt_list_length_empty(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);

    assert_int_equal(bt_list_length(list), 0);
    bt_list_free(list);
}

void test_bt_list_length_after_add(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 1;
    int b = 2;
    int c = 3;

    bt_list_add_tail(list, &a);
    assert_int_equal(bt_list_length(list), 1);
    bt_list_add_tail(list, &b);
    assert_int_equal(bt_list_length(list), 2);
    bt_list_add_tail(list, &c);
    assert_int_equal(bt_list_length(list), 3);
    bt_list_free(list);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_add_head / bt_list_add_tail
 ****************************************************************************/

void test_bt_list_add_head_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 1;
    int b = 2;

    bt_list_add_head(list, &a);
    bt_list_add_head(list, &b);

    /* b was added to head last, so head should be b */

    bt_list_node_t* head = bt_list_head(list);
    assert_int_equal(*(int*)bt_list_node(head), 2);
    bt_list_free(list);
}

void test_bt_list_add_tail_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 1;
    int b = 2;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);

    /* a was added first, so head should be a */

    bt_list_node_t* head = bt_list_head(list);
    assert_int_equal(*(int*)bt_list_node(head), 1);

    bt_list_node_t* tail = bt_list_tail(list);
    assert_int_equal(*(int*)bt_list_node(tail), 2);
    bt_list_free(list);
}

void test_bt_list_add_order(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int vals[3] = { 10, 20, 30 };

    bt_list_add_tail(list, &vals[0]);
    bt_list_add_tail(list, &vals[1]);
    bt_list_add_tail(list, &vals[2]);

    bt_list_node_t* node = bt_list_head(list);
    assert_int_equal(*(int*)bt_list_node(node), 10);
    node = bt_list_next(list, node);
    assert_int_equal(*(int*)bt_list_node(node), 20);
    node = bt_list_next(list, node);
    assert_int_equal(*(int*)bt_list_node(node), 30);
    bt_list_free(list);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_head / tail / next / node
 ****************************************************************************/

void test_bt_list_head_tail_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 100;
    int b = 200;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);

    assert_int_equal(*(int*)bt_list_node(bt_list_head(list)), 100);
    assert_int_equal(*(int*)bt_list_node(bt_list_tail(list)), 200);
    bt_list_free(list);
}

void test_bt_list_next_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 1;
    int b = 2;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);

    bt_list_node_t* node = bt_list_head(list);
    node = bt_list_next(list, node);
    assert_non_null(node);
    assert_int_equal(*(int*)bt_list_node(node), 2);

    /* Next of tail should be NULL */

    node = bt_list_next(list, node);
    assert_null(node);
    bt_list_free(list);
}

void test_bt_list_next_null(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);

    assert_null(bt_list_next(list, NULL));
    bt_list_free(list);
}

void test_bt_list_node_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int val = 42;

    bt_list_add_tail(list, &val);
    bt_list_node_t* node = bt_list_head(list);
    assert_int_equal(*(int*)bt_list_node(node), 42);
    bt_list_free(list);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_remove / bt_list_remove_node
 ****************************************************************************/

void test_bt_list_remove_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 1;
    int b = 2;
    int c = 3;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);
    bt_list_add_tail(list, &c);

    bt_list_remove(list, &b);
    assert_int_equal(bt_list_length(list), 2);

    bt_list_node_t* node = bt_list_head(list);
    assert_int_equal(*(int*)bt_list_node(node), 1);
    node = bt_list_next(list, node);
    assert_int_equal(*(int*)bt_list_node(node), 3);
    bt_list_free(list);
}

void test_bt_list_remove_node_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 1;
    int b = 2;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);

    bt_list_node_t* node = bt_list_head(list);
    bt_list_remove_node(list, node);
    assert_int_equal(bt_list_length(list), 1);
    assert_int_equal(*(int*)bt_list_node(bt_list_head(list)), 2);
    bt_list_free(list);
}

void test_bt_list_remove_not_found(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 1;
    int b = 2;

    bt_list_add_tail(list, &a);

    /* Removing data not in list should not crash or change length */

    bt_list_remove(list, &b);
    assert_int_equal(bt_list_length(list), 1);
    bt_list_free(list);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_clear
 ****************************************************************************/

void test_bt_list_clear_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(test_free_cb);
    int a = 1;
    int b = 2;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);

    g_free_count = 0;
    bt_list_clear(list);
    assert_true(bt_list_is_empty(list));
    assert_int_equal(bt_list_length(list), 0);
    assert_int_equal(g_free_count, 2);
    bt_list_free(list);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_move
 ****************************************************************************/

void test_bt_list_move_to_head(FAR void** state)
{
    bt_list_t* src = bt_list_new(NULL);
    bt_list_t* dst = bt_list_new(NULL);
    int a = 1;
    int b = 2;

    bt_list_add_tail(src, &a);
    bt_list_add_tail(src, &b);

    bt_list_move(src, dst, &a, true);
    assert_int_equal(bt_list_length(src), 1);
    assert_int_equal(bt_list_length(dst), 1);
    assert_int_equal(*(int*)bt_list_node(bt_list_head(dst)), 1);
    bt_list_free(src);
    bt_list_free(dst);
}

void test_bt_list_move_to_tail(FAR void** state)
{
    bt_list_t* src = bt_list_new(NULL);
    bt_list_t* dst = bt_list_new(NULL);
    int a = 1;
    int b = 2;

    bt_list_add_tail(src, &a);
    bt_list_add_tail(dst, &b);

    bt_list_move(src, dst, &a, false);
    assert_int_equal(bt_list_length(src), 0);
    assert_int_equal(bt_list_length(dst), 2);
    assert_int_equal(*(int*)bt_list_node(bt_list_tail(dst)), 1);
    bt_list_free(src);
    bt_list_free(dst);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_foreach
 ****************************************************************************/

void test_bt_list_foreach_normal(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 10;
    int b = 20;
    int c = 30;
    int sum = 0;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);
    bt_list_add_tail(list, &c);

    bt_list_foreach(list, iter_sum_cb, &sum);
    assert_int_equal(sum, 60);
    bt_list_free(list);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_list_find
 ****************************************************************************/

void test_bt_list_find_found(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 10;
    int b = 20;
    int c = 30;
    int target = 20;

    bt_list_add_tail(list, &a);
    bt_list_add_tail(list, &b);
    bt_list_add_tail(list, &c);

    void* result = bt_list_find(list, find_value_cb, &target);
    assert_non_null(result);
    assert_int_equal(*(int*)result, 20);
    bt_list_free(list);
}

void test_bt_list_find_not_found(FAR void** state)
{
    bt_list_t* list = bt_list_new(NULL);
    int a = 10;
    int target = 99;

    bt_list_add_tail(list, &a);

    void* result = bt_list_find(list, find_value_cb, &target);
    assert_null(result);
    bt_list_free(list);
}
