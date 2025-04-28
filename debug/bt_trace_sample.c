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
#include "bt_sched_trace.h"

#include <stdlib.h>

void test_func1()
{
    bt_timepoint_t tp1;

    bt_trace_begin("test_func1", &tp1);
    usleep(rand() % 1000);
    bt_trace_end("test_func1", &tp1);
}

void test_func2()
{
    bt_timepoint_t tp1;

    bt_trace_begin("test_func2", &tp1);
    usleep(rand() % 1000);
    bt_trace_end("test_func2", &tp1);
}

int main(int argc, char* argv[])
{

    bt_trace_start();

    test_func1();
    test_func2();

    test_func1();
    test_func2();

    test_func1();
    test_func2();

    bt_trace_stop();
    return 0;
}