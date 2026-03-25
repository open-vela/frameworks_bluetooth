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

#include <string.h>

#include "bt_addr.h"

extern int bachk(const char* str);

#include "cm_bt_addr.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_addr_setup(FAR void** state)
{
    return 0;
}

int test_bt_addr_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_is_empty
 ****************************************************************************/

void test_bt_addr_is_empty_normal(FAR void** state)
{
    bt_address_t addr;

    memset(&addr, 0, sizeof(addr));
    assert_true(bt_addr_is_empty(&addr));
}

void test_bt_addr_is_empty_nonempty(FAR void** state)
{
    bt_address_t addr;
    uint8_t bd[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

    memcpy(addr.addr, bd, 6);
    assert_false(bt_addr_is_empty(&addr));
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_set_empty
 ****************************************************************************/

void test_bt_addr_set_empty_normal(FAR void** state)
{
    bt_address_t addr;
    uint8_t bd[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

    memcpy(addr.addr, bd, 6);
    bt_addr_set_empty(&addr);
    assert_true(bt_addr_is_empty(&addr));
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_compare
 ****************************************************************************/

void test_bt_addr_compare_equal(FAR void** state)
{
    bt_address_t a;
    bt_address_t b;
    uint8_t bd[6] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

    memcpy(a.addr, bd, 6);
    memcpy(b.addr, bd, 6);
    assert_int_equal(bt_addr_compare(&a, &b), 0);
}

void test_bt_addr_compare_not_equal(FAR void** state)
{
    bt_address_t a;
    bt_address_t b;

    memset(&a, 0x11, sizeof(a));
    memset(&b, 0x22, sizeof(b));
    assert_int_not_equal(bt_addr_compare(&a, &b), 0);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_ba2str
 ****************************************************************************/

void test_bt_addr_ba2str_normal(FAR void** state)
{
    bt_address_t addr;
    char str[18];
    int ret;

    /* addr bytes are in reverse order: addr[5]=0x11 .. addr[0]=0x66 */

    addr.addr[0] = 0x66;
    addr.addr[1] = 0x55;
    addr.addr[2] = 0x44;
    addr.addr[3] = 0x33;
    addr.addr[4] = 0x22;
    addr.addr[5] = 0x11;

    ret = bt_addr_ba2str(&addr, str);
    assert_true(ret > 0);
    assert_string_equal(str, "11:22:33:44:55:66");
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_bastr
 ****************************************************************************/

void test_bt_addr_bastr_normal(FAR void** state)
{
    bt_address_t addr;
    char* result;

    addr.addr[0] = 0x66;
    addr.addr[1] = 0x55;
    addr.addr[2] = 0x44;
    addr.addr[3] = 0x33;
    addr.addr[4] = 0x22;
    addr.addr[5] = 0x11;

    result = bt_addr_bastr(&addr);
    assert_non_null(result);
    assert_string_equal(result, "11:22:33:44:55:66");
}

void test_bt_addr_bastr_null_param(FAR void** state)
{
    char* result;

    result = bt_addr_bastr(NULL);
    assert_null(result);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_str2ba
 ****************************************************************************/

void test_bt_addr_str2ba_normal(FAR void** state)
{
    bt_address_t addr;
    int ret;

    ret = bt_addr_str2ba("11:22:33:44:55:66", &addr);
    assert_int_equal(ret, 0);
    assert_int_equal(addr.addr[5], 0x11);
    assert_int_equal(addr.addr[4], 0x22);
    assert_int_equal(addr.addr[3], 0x33);
    assert_int_equal(addr.addr[2], 0x44);
    assert_int_equal(addr.addr[1], 0x55);
    assert_int_equal(addr.addr[0], 0x66);
}

void test_bt_addr_str2ba_invalid_input(FAR void** state)
{
    bt_address_t addr;
    int ret;

    ret = bt_addr_str2ba("not-a-valid-addr!", &addr);
    assert_int_equal(ret, -1);
}

void test_bt_addr_str2ba_null_str(FAR void** state)
{
    bt_address_t addr;
    int ret;

    ret = bt_addr_str2ba(NULL, &addr);
    assert_int_equal(ret, -1);
}

void test_bt_addr_str2ba_short_str(FAR void** state)
{
    bt_address_t addr;
    int ret;

    ret = bt_addr_str2ba("11:22:33", &addr);
    assert_int_equal(ret, -1);
}

void test_bt_addr_str2ba_bad_separator(FAR void** state)
{
    bt_address_t addr;
    int ret;

    ret = bt_addr_str2ba("11-22-33-44-55-66", &addr);
    assert_int_equal(ret, -1);
}

void test_bt_addr_str2ba_non_hex(FAR void** state)
{
    bt_address_t addr;
    int ret;

    ret = bt_addr_str2ba("GG:HH:II:JJ:KK:LL", &addr);
    assert_int_equal(ret, -1);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_set
 ****************************************************************************/

void test_bt_addr_set_normal(FAR void** state)
{
    bt_address_t addr;
    uint8_t bd[6] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };

    bt_addr_set(&addr, bd);
    assert_memory_equal(addr.addr, bd, 6);
}

/****************************************************************************
 * Public Functions - Test Cases: bt_addr_swap
 ****************************************************************************/

void test_bt_addr_swap_normal(FAR void** state)
{
    bt_address_t src;
    bt_address_t dest;
    uint8_t expected[6] = { 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 };

    src.addr[0] = 0x11;
    src.addr[1] = 0x22;
    src.addr[2] = 0x33;
    src.addr[3] = 0x44;
    src.addr[4] = 0x55;
    src.addr[5] = 0x66;

    bt_addr_swap(&src, &dest);
    assert_memory_equal(dest.addr, expected, 6);
}

/****************************************************************************
 * Public Functions - Test Cases: bachk (static)
 ****************************************************************************/

void test_bachk_valid(FAR void** state)
{
    assert_int_equal(bachk("11:22:33:44:55:66"), 0);
    assert_int_equal(bachk("AA:BB:CC:DD:EE:FF"), 0);
    assert_int_equal(bachk("aa:bb:cc:dd:ee:ff"), 0);
}

void test_bachk_null(FAR void** state)
{
    assert_int_equal(bachk(NULL), -1);
}

void test_bachk_wrong_length(FAR void** state)
{
    assert_int_equal(bachk("11:22:33"), -1);
    assert_int_equal(bachk("11:22:33:44:55:66:77"), -1);
    assert_int_equal(bachk(""), -1);
}

void test_bachk_bad_format(FAR void** state)
{
    assert_int_equal(bachk("11-22-33-44-55-66"), -1);
    assert_int_equal(bachk("GG:HH:II:JJ:KK:LL"), -1);
    assert_int_equal(bachk("11:22:33:44:55:6"), -1);
}
