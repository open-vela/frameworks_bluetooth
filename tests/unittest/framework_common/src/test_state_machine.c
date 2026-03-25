/****************************************************************************
 * tests/unittest/framework_common/src/test_state_machine.c
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
#include <string.h>

#include "../../../../framework/include/state_machine.h"

#include "cm_state_machine.h"

/****************************************************************************
 * Private Data - Mock state callbacks
 ****************************************************************************/

static int g_enter_count;
static int g_exit_count;
static int g_event_count;
static uint32_t g_last_event;

static void mock_enter(state_machine_t *sm)
{
  g_enter_count++;
}

static void mock_exit(state_machine_t *sm)
{
  g_exit_count++;
}

static bool mock_process_event(state_machine_t *sm,
                               uint32_t event, void *data)
{
  g_event_count++;
  g_last_event = event;
  return true;
}

static bool mock_process_event_false(state_machine_t *sm,
                                     uint32_t event, void *data)
{
  return false;
}

static const state_t g_state_a =
{
  .state_name = "StateA",
  .state_value = 1,
  .enter = mock_enter,
  .exit = mock_exit,
  .process_event = mock_process_event,
};

static const state_t g_state_b =
{
  .state_name = "StateB",
  .state_value = 2,
  .enter = mock_enter,
  .exit = mock_exit,
  .process_event = mock_process_event_false,
};

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_state_machine_setup(FAR void **state)
{
  g_enter_count = 0;
  g_exit_count = 0;
  g_event_count = 0;
  g_last_event = 0;
  return 0;
}

int test_state_machine_teardown(FAR void **state)
{
  return 0;
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_ctor
 ****************************************************************************/

void test_hsm_ctor_normal(FAR void **state)
{
  state_machine_t sm;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);
  assert_ptr_equal(sm.initial_state, &g_state_a);
  assert_ptr_equal(sm.current_state, &g_state_a);
  assert_true(g_enter_count > 0);
}

void test_hsm_ctor_null(FAR void **state)
{
  /* should not crash */

  hsm_ctor(NULL, &g_state_a);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_dtor
 ****************************************************************************/

void test_hsm_dtor_normal(FAR void **state)
{
  state_machine_t sm;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);

  /* hsm_dtor is a no-op, just verify no crash */

  hsm_dtor(&sm);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_transition_to
 ****************************************************************************/

void test_hsm_transition_to_normal(FAR void **state)
{
  state_machine_t sm;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);

  g_enter_count = 0;
  g_exit_count = 0;

  hsm_transition_to(&sm, &g_state_b);
  assert_ptr_equal(sm.current_state, &g_state_b);
  assert_ptr_equal(sm.previous_state, &g_state_a);
  assert_int_equal(g_exit_count, 1);
  assert_int_equal(g_enter_count, 1);
}

void test_hsm_transition_to_from_existing(FAR void **state)
{
  state_machine_t sm;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);
  hsm_transition_to(&sm, &g_state_b);

  g_enter_count = 0;
  g_exit_count = 0;

  hsm_transition_to(&sm, &g_state_a);
  assert_ptr_equal(sm.current_state, &g_state_a);
  assert_ptr_equal(sm.previous_state, &g_state_b);
  assert_int_equal(g_exit_count, 1);
  assert_int_equal(g_enter_count, 1);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_get_current_state
 ****************************************************************************/

void test_hsm_get_current_state_normal(FAR void **state)
{
  state_machine_t sm;
  const state_t *cur;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);

  cur = hsm_get_current_state(&sm);
  assert_ptr_equal(cur, &g_state_a);
}

void test_hsm_get_current_state_null(FAR void **state)
{
  const state_t *cur;

  cur = hsm_get_current_state(NULL);
  assert_null(cur);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_get_previous_state
 ****************************************************************************/

void test_hsm_get_previous_state_normal(FAR void **state)
{
  state_machine_t sm;
  const state_t *prev;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);
  hsm_transition_to(&sm, &g_state_b);

  prev = hsm_get_previous_state(&sm);
  assert_ptr_equal(prev, &g_state_a);
}

void test_hsm_get_previous_state_null(FAR void **state)
{
  const state_t *prev;

  prev = hsm_get_previous_state(NULL);
  assert_null(prev);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_get_current_state_name
 ****************************************************************************/

void test_hsm_get_current_state_name_normal(FAR void **state)
{
  state_machine_t sm;
  const char *name;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);

  name = hsm_get_current_state_name(&sm);
  assert_string_equal(name, "StateA");
}

void test_hsm_get_current_state_name_null_sm(FAR void **state)
{
  const char *name;

  name = hsm_get_current_state_name(NULL);
  assert_string_equal(name, HSM_NONE_STR);
}

void test_hsm_get_current_state_name_null_state(FAR void **state)
{
  state_machine_t sm;
  const char *name;

  memset(&sm, 0, sizeof(sm));

  name = hsm_get_current_state_name(&sm);
  assert_string_equal(name, HSM_NONE_STR);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_get_state_name
 ****************************************************************************/

void test_hsm_get_state_name_normal(FAR void **state)
{
  const char *name;

  name = hsm_get_state_name(&g_state_a);
  assert_string_equal(name, "StateA");
}

void test_hsm_get_state_name_null(FAR void **state)
{
  const char *name;

  name = hsm_get_state_name(NULL);
  assert_string_equal(name, HSM_NONE_STR);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_get_state_value
 ****************************************************************************/

void test_hsm_get_state_value_normal(FAR void **state)
{
  uint16_t val;

  val = hsm_get_state_value(&g_state_a);
  assert_int_equal(val, 1);

  val = hsm_get_state_value(&g_state_b);
  assert_int_equal(val, 2);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_get_current_state_value
 ****************************************************************************/

void test_hsm_get_current_state_value_normal(FAR void **state)
{
  state_machine_t sm;
  uint16_t val;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);

  val = hsm_get_current_state_value(&sm);
  assert_int_equal(val, 1);
}

/****************************************************************************
 * Public Functions - Test Cases: hsm_dispatch_event
 ****************************************************************************/

void test_hsm_dispatch_event_normal(FAR void **state)
{
  state_machine_t sm;
  bool ret;

  memset(&sm, 0, sizeof(sm));
  hsm_ctor(&sm, &g_state_a);

  ret = hsm_dispatch_event(&sm, 100, NULL);
  assert_true(ret);
  assert_int_equal(g_event_count, 1);
  assert_int_equal(g_last_event, 100);
}

void test_hsm_dispatch_event_null_state(FAR void **state)
{
  state_machine_t sm;
  bool ret;

  memset(&sm, 0, sizeof(sm));

  ret = hsm_dispatch_event(&sm, 100, NULL);
  assert_false(ret);
}
