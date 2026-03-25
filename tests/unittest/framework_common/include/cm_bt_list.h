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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_LIST_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_LIST_H

int test_bt_list_setup(FAR void** state);
int test_bt_list_teardown(FAR void** state);

/* bt_list_new / bt_list_free */

void test_bt_list_new_normal(FAR void** state);
void test_bt_list_new_with_free_cb(FAR void** state);
void test_bt_list_free_null(FAR void** state);

/* bt_list_is_empty / bt_list_length */

void test_bt_list_is_empty_new_list(FAR void** state);
void test_bt_list_length_empty(FAR void** state);
void test_bt_list_length_after_add(FAR void** state);

/* bt_list_add_head / bt_list_add_tail */

void test_bt_list_add_head_normal(FAR void** state);
void test_bt_list_add_tail_normal(FAR void** state);
void test_bt_list_add_order(FAR void** state);

/* bt_list_head / bt_list_tail / bt_list_next / bt_list_node */

void test_bt_list_head_tail_normal(FAR void** state);
void test_bt_list_next_normal(FAR void** state);
void test_bt_list_next_null(FAR void** state);
void test_bt_list_node_normal(FAR void** state);

/* bt_list_remove / bt_list_remove_node */

void test_bt_list_remove_normal(FAR void** state);
void test_bt_list_remove_node_normal(FAR void** state);
void test_bt_list_remove_not_found(FAR void** state);

/* bt_list_clear */

void test_bt_list_clear_normal(FAR void** state);

/* bt_list_move */

void test_bt_list_move_to_head(FAR void** state);
void test_bt_list_move_to_tail(FAR void** state);

/* bt_list_foreach */

void test_bt_list_foreach_normal(FAR void** state);

/* bt_list_find */

void test_bt_list_find_found(FAR void** state);
void test_bt_list_find_not_found(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_LIST_H */
