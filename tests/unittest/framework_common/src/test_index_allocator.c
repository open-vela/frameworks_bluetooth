/****************************************************************************
 * tests/unittest/service_common/src/test_index_allocator.c
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
#include <stdlib.h>
#include <string.h>

#include "index_allocator.h"
#include "cm_index_allocator.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_index_allocator_setup(void** state)
{
    return 0;
}

int test_index_allocator_teardown(void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

/* index_allocator_create tests */

void test_index_allocator_create_normal(void** state)
{
    index_allocator_t* alloc = index_allocator_create(10);
    assert_non_null(alloc);
    assert_int_equal(alloc->id_max, 10);
    assert_int_equal(alloc->id_next, 0);
    index_allocator_delete(&alloc);
    assert_null(alloc);
}

void test_index_allocator_create_max_one(void** state)
{
    /* max=1 means IDs 0 and 1 are valid (id_max is inclusive) */

    index_allocator_t* alloc = index_allocator_create(1);
    assert_non_null(alloc);
    assert_int_equal(alloc->id_max, 1);

    int id = index_alloc(alloc);
    assert_int_equal(id, 0);

    id = index_alloc(alloc);
    assert_int_equal(id, 1);

    /* Third alloc should fail */

    id = index_alloc(alloc);
    assert_int_equal(id, -1);

    index_allocator_delete(&alloc);
}

void test_index_allocator_create_max_zero(void** state)
{
    /* max=0 means no valid IDs can be allocated (id_next wraps immediately) */

    index_allocator_t* alloc = index_allocator_create(0);
    assert_non_null(alloc);
    assert_int_equal(alloc->id_max, 0);
    index_allocator_delete(&alloc);
}

void test_index_allocator_create_max_boundary_32(void** state)
{
    /* Test boundary at 32 (bitmap word boundary).
     * max=32 means IDs 0..32 are valid (33 total).
     */

    index_allocator_t* alloc = index_allocator_create(32);
    assert_non_null(alloc);
    assert_int_equal(alloc->id_max, 32);

    /* Should be able to allocate 33 IDs (0..32) */

    int i;
    for (i = 0; i <= 32; i++) {
        int id = index_alloc(alloc);
        assert_int_equal(id, i);
    }

    /* 34th should fail */

    int id = index_alloc(alloc);
    assert_int_equal(id, -1);

    index_allocator_delete(&alloc);
}

void test_index_allocator_create_max_large(void** state)
{
    index_allocator_t* alloc = index_allocator_create(128);
    assert_non_null(alloc);
    assert_int_equal(alloc->id_max, 128);
    index_allocator_delete(&alloc);
}

/* index_allocator_delete tests */

void test_index_allocator_delete_normal(void** state)
{
    index_allocator_t* alloc = index_allocator_create(8);
    assert_non_null(alloc);
    index_allocator_delete(&alloc);
    assert_null(alloc);
}

void test_index_allocator_delete_double_delete(void** state)
{
    index_allocator_t* alloc = index_allocator_create(8);
    assert_non_null(alloc);
    index_allocator_delete(&alloc);
    assert_null(alloc);

    /* Second delete should be safe (free(NULL) is no-op) */

    index_allocator_delete(&alloc);
    assert_null(alloc);
}

/* index_alloc tests */

void test_index_alloc_normal(void** state)
{
    index_allocator_t* alloc = index_allocator_create(10);
    assert_non_null(alloc);

    int id = index_alloc(alloc);
    assert_int_equal(id, 0);

    index_allocator_delete(&alloc);
}

void test_index_alloc_sequential(void** state)
{
    index_allocator_t* alloc = index_allocator_create(5);
    assert_non_null(alloc);

    int i;
    for (i = 0; i < 5; i++) {
        int id = index_alloc(alloc);
        assert_int_equal(id, i);
    }

    index_allocator_delete(&alloc);
}

void test_index_alloc_exhaust(void** state)
{
    /* max=3 means IDs 0..3 are valid (4 total) */

    index_allocator_t* alloc = index_allocator_create(3);
    assert_non_null(alloc);

    /* Allocate all 4 slots */

    assert_int_equal(index_alloc(alloc), 0);
    assert_int_equal(index_alloc(alloc), 1);
    assert_int_equal(index_alloc(alloc), 2);
    assert_int_equal(index_alloc(alloc), 3);

    /* Next alloc should return -1 */

    assert_int_equal(index_alloc(alloc), -1);

    index_allocator_delete(&alloc);
}

void test_index_alloc_wrap_around(void** state)
{
    /* max=2 means IDs 0,1,2 are valid (3 total) */

    index_allocator_t* alloc = index_allocator_create(2);
    assert_non_null(alloc);

    /* Allocate all */

    assert_int_equal(index_alloc(alloc), 0);
    assert_int_equal(index_alloc(alloc), 1);
    assert_int_equal(index_alloc(alloc), 2);

    /* Free slot 0 */

    index_free(alloc, 0);

    /* Next alloc should wrap around and find slot 0 */

    int id = index_alloc(alloc);
    assert_int_equal(id, 0);

    index_allocator_delete(&alloc);
}

void test_index_alloc_after_free(void** state)
{
    index_allocator_t* alloc = index_allocator_create(5);
    assert_non_null(alloc);

    /* Allocate 3 */

    assert_int_equal(index_alloc(alloc), 0);
    assert_int_equal(index_alloc(alloc), 1);
    assert_int_equal(index_alloc(alloc), 2);

    /* Free middle one */

    index_free(alloc, 1);

    /* Next alloc should give slot 1 since id_next was reset */

    int id = index_alloc(alloc);
    assert_int_equal(id, 1);

    index_allocator_delete(&alloc);
}

void test_index_alloc_single_slot(void** state)
{
    /* max=0 means only ID 0 is valid (1 total) */

    index_allocator_t* alloc = index_allocator_create(0);
    assert_non_null(alloc);

    assert_int_equal(index_alloc(alloc), 0);
    assert_int_equal(index_alloc(alloc), -1);

    index_free(alloc, 0);
    assert_int_equal(index_alloc(alloc), 0);

    index_allocator_delete(&alloc);
}

/* index_free tests */

void test_index_free_normal(void** state)
{
    index_allocator_t* alloc = index_allocator_create(10);
    assert_non_null(alloc);

    int id = index_alloc(alloc);
    assert_int_equal(id, 0);

    /* Free should not crash */

    index_free(alloc, 0);

    /* Should be able to re-allocate */

    id = index_alloc(alloc);
    assert_int_equal(id, 0);

    index_allocator_delete(&alloc);
}

void test_index_free_resets_next(void** state)
{
    index_allocator_t* alloc = index_allocator_create(10);
    assert_non_null(alloc);

    /* Allocate 0, 1, 2 */

    index_alloc(alloc);
    index_alloc(alloc);
    index_alloc(alloc);

    /* id_next should be 3 now */

    assert_int_equal(alloc->id_next, 3);

    /* Free id=1, which is < id_next, so id_next should reset to 1 */

    index_free(alloc, 1);
    assert_int_equal(alloc->id_next, 1);

    index_allocator_delete(&alloc);
}

void test_index_free_high_bit(void** state)
{
    index_allocator_t* alloc = index_allocator_create(64);
    assert_non_null(alloc);

    /* Allocate up to index 33 (crosses bitmap word boundary) */

    int i;
    for (i = 0; i < 34; i++) {
        assert_int_equal(index_alloc(alloc), i);
    }

    /* Free index 33 (in second bitmap word) */

    index_free(alloc, 33);

    /* Re-allocate should give 33 back */

    int id = index_alloc(alloc);
    assert_int_equal(id, 33);

    index_allocator_delete(&alloc);
}

void test_index_free_and_realloc(void** state)
{
    /* max=3 means IDs 0,1,2,3 are valid (4 total) */

    index_allocator_t* alloc = index_allocator_create(3);
    assert_non_null(alloc);

    /* Fill all */

    assert_int_equal(index_alloc(alloc), 0);
    assert_int_equal(index_alloc(alloc), 1);
    assert_int_equal(index_alloc(alloc), 2);
    assert_int_equal(index_alloc(alloc), 3);
    assert_int_equal(index_alloc(alloc), -1);

    /* Free 2 and 0 */

    index_free(alloc, 2);
    index_free(alloc, 0);

    /* id_next should be 0 (lowest freed) */

    assert_int_equal(alloc->id_next, 0);

    /* Allocate should give 0 first, then 2 */

    assert_int_equal(index_alloc(alloc), 0);
    assert_int_equal(index_alloc(alloc), 2);
    assert_int_equal(index_alloc(alloc), -1);

    index_allocator_delete(&alloc);
}
