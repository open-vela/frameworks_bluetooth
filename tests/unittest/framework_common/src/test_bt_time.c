/****************************************************************************
 * tests/unittest/framework_common/src/test_bt_time.c
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

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <unistd.h>

#include "bt_time.h"

#include "cm_bt_time.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_bt_time_setup(FAR void **state)
{
  return 0;
}

int test_bt_time_teardown(FAR void **state)
{
  return 0;
}

/****************************************************************************
 * Public Functions - Test Cases
 ****************************************************************************/

void test_bt_get_os_timestamp_us_normal(FAR void **state)
{
  uint64_t ts;

  ts = bt_get_os_timestamp_us();

  /* Timestamp should be non-zero after boot */

  assert_true(ts > 0);
}

void test_bt_get_os_timestamp_us_monotonic(FAR void **state)
{
  uint64_t ts1;
  uint64_t ts2;

  ts1 = bt_get_os_timestamp_us();
  usleep(1000); /* sleep 1ms */
  ts2 = bt_get_os_timestamp_us();

  /* Second timestamp must be greater */

  assert_true(ts2 > ts1);
}

void test_bt_get_os_timestamp_ms_normal(FAR void **state)
{
  uint32_t ts;

  ts = bt_get_os_timestamp_ms();
  assert_true(ts > 0);
}

void test_bt_get_os_timestamp_ms_monotonic(FAR void **state)
{
  uint32_t ts1;
  uint32_t ts2;

  ts1 = bt_get_os_timestamp_ms();
  usleep(10000); /* sleep 10ms */
  ts2 = bt_get_os_timestamp_ms();

  assert_true(ts2 >= ts1);
}
