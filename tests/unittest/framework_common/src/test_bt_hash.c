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
#include <string.h>

#include "bt_hash.h"

#include "cm_bt_hash.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_hash_setup(FAR void** state)
{
    return 0;
}

int test_bt_hash_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

void test_bt_hash4_normal(FAR void** state)
{
    const char* key = "bluetooth";
    uint32_t hash;

    hash = bt_hash4(key, strlen(key));

    /* Hash should be non-zero for non-empty input */

    assert_int_not_equal(hash, 0);
}

void test_bt_hash4_empty(FAR void** state)
{
    const char* key = "";
    uint32_t hash;

    hash = bt_hash4(key, 0);
    assert_int_equal(hash, 0);
}

void test_bt_hash4_single_byte(FAR void** state)
{
    uint8_t key = 0x42;
    uint32_t hash;

    hash = bt_hash4(&key, 1);
    assert_int_not_equal(hash, 0);
}

void test_bt_hash4_deterministic(FAR void** state)
{
    const char* key = "test_key";
    uint32_t hash1;
    uint32_t hash2;

    hash1 = bt_hash4(key, strlen(key));
    hash2 = bt_hash4(key, strlen(key));
    assert_int_equal(hash1, hash2);
}

void test_bt_hash4_different_input(FAR void** state)
{
    const char* key1 = "hello";
    const char* key2 = "world";
    uint32_t hash1;
    uint32_t hash2;

    hash1 = bt_hash4(key1, strlen(key1));
    hash2 = bt_hash4(key2, strlen(key2));

    /* Different inputs should (very likely) produce different hashes */

    assert_int_not_equal(hash1, hash2);
}
