/****************************************************************************
 * tests/unittest/framework_common/include/cm_index_allocator.h
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

#ifndef __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_INDEX_ALLOCATOR_H
#define __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_INDEX_ALLOCATOR_H

int test_index_allocator_setup(void** state);
int test_index_allocator_teardown(void** state);

/* index_allocator_create tests */

void test_index_allocator_create_normal(void** state);
void test_index_allocator_create_max_one(void** state);
void test_index_allocator_create_max_zero(void** state);
void test_index_allocator_create_max_boundary_32(void** state);
void test_index_allocator_create_max_large(void** state);

/* index_allocator_delete tests */

void test_index_allocator_delete_normal(void** state);
void test_index_allocator_delete_double_delete(void** state);

/* index_alloc tests */

void test_index_alloc_normal(void** state);
void test_index_alloc_sequential(void** state);
void test_index_alloc_exhaust(void** state);
void test_index_alloc_wrap_around(void** state);
void test_index_alloc_after_free(void** state);
void test_index_alloc_single_slot(void** state);

/* index_free tests */

void test_index_free_normal(void** state);
void test_index_free_resets_next(void** state);
void test_index_free_high_bit(void** state);
void test_index_free_and_realloc(void** state);

#endif /* __TESTING_CMOCKA_FRAMEWORK_COMMON_CM_INDEX_ALLOCATOR_H */
