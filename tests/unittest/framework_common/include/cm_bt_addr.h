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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_ADDR_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_ADDR_H

int test_bt_addr_setup(FAR void** state);
int test_bt_addr_teardown(FAR void** state);

/* bt_addr_is_empty */

void test_bt_addr_is_empty_normal(FAR void** state);
void test_bt_addr_is_empty_nonempty(FAR void** state);

/* bt_addr_set_empty */

void test_bt_addr_set_empty_normal(FAR void** state);

/* bt_addr_compare */

void test_bt_addr_compare_equal(FAR void** state);
void test_bt_addr_compare_not_equal(FAR void** state);

/* bt_addr_ba2str */

void test_bt_addr_ba2str_normal(FAR void** state);

/* bt_addr_bastr */

void test_bt_addr_bastr_normal(FAR void** state);
void test_bt_addr_bastr_null_param(FAR void** state);

/* bt_addr_str2ba */

void test_bt_addr_str2ba_normal(FAR void** state);
void test_bt_addr_str2ba_invalid_input(FAR void** state);
void test_bt_addr_str2ba_null_str(FAR void** state);
void test_bt_addr_str2ba_short_str(FAR void** state);
void test_bt_addr_str2ba_bad_separator(FAR void** state);
void test_bt_addr_str2ba_non_hex(FAR void** state);

/* bt_addr_set */

void test_bt_addr_set_normal(FAR void** state);

/* bt_addr_swap */

void test_bt_addr_swap_normal(FAR void** state);

/* bachk (static) */

void test_bachk_valid(FAR void** state);
void test_bachk_null(FAR void** state);
void test_bachk_wrong_length(FAR void** state);
void test_bachk_bad_format(FAR void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_BT_ADDR_H */
