/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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
#include <stdio.h>
#include <string.h>

#include "bt_memory.h"

void test_leak()
{
    void* p1 = bt_malloc(128);
    void* p2 = bt_malloc(256);
}

void test_overflow()
{
    char* buf = (char*)bt_malloc(16);
    memset(buf, 0, 20);
    bt_free(buf);
}

void test_double_free()
{
    void* p = bt_malloc(64);
    bt_free(p);
    bt_free(p);
}

int main()
{
    test_leak();

    test_overflow();

    test_double_free();

    void* p2 = bt_malloc(500);

    bt_report_leak();
    return 0;
}