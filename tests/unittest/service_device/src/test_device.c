/****************************************************************************
 * tests/unittest/service_device/src/test_device.c
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
#include <stdlib.h>

/****************************************************************************
 * Mock: do_in_service_loop (called by device_set_bond_state)
 * We use a static mock + macro redirect to avoid multiple definition
 * errors in the flat NuttX build (the real do_in_service_loop lives in
 * libbluetooth.a).
 ****************************************************************************/

static int g_mock_do_in_service_loop_called;
static void *g_mock_do_in_service_loop_data;

/* Forward declare the function type used by service_loop */

typedef void (*service_func_t)(void *data);

static void mock_do_in_service_loop(service_func_t func, void *data)
{
  g_mock_do_in_service_loop_called++;
  g_mock_do_in_service_loop_data = data;

  /* Free the data allocated by device_set_bond_state to avoid leak */

  if (data)
    {
      free(data);
    }
}

/* Redirect calls from device.c to our static mock */

#define do_in_service_loop mock_do_in_service_loop

/****************************************************************************
 * Include device.c directly to access internal structs and static functions.
 * We must prevent device.c from including headers that pull in too many
 * dependencies. We pre-include what's needed and block problematic headers.
 ****************************************************************************/

/* Prevent device.c from including problematic headers */

#define _BT_SERVICE_LOOP_H__
#define _BT_ADAPTER_INTERNAL_H__
#define LOG_TAG "test_device"

/* Include the headers that device.c needs and that we can safely include */

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_addr.h"
#include "bt_config.h"
#include "bt_device.h"
#include "bt_list.h"
#include "bt_utils.h"
#include "bt_uuid.h"

/* Provide the bond_state_change_message_t that device_set_bond_state uses */

typedef struct
{
  void *device;
  bond_state_t previous_state;
  bool is_ctkd;
} bond_state_change_message_t;

/****************************************************************************
 * Provide missing macros that device_get_remote_uuids uses.
 * We won't test that static function, but we need it to compile.
 ****************************************************************************/

#ifndef BT_HEAD_UUID16_TYPE
#define BT_HEAD_UUID16_TYPE 0
#endif

#ifndef BT_HEAD_UUID128_TYPE
#define BT_HEAD_UUID128_TYPE 1
#endif

/* Now include device.c - the guards above prevent re-including
 * service_loop.h and adapter_internel.h.
 * The #define do_in_service_loop above redirects calls to our mock. */

#include "../../../../service/src/device.c"

/* Remove the redirect macro so it doesn't affect the rest of the file */

#undef do_in_service_loop

#include "cm_device.h"

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bt_device_t *g_br_device;
static bt_device_t *g_le_device;

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_device_setup(FAR void **state)
{
  bt_address_t addr;

  memset(&addr, 0, sizeof(addr));
  addr.addr[0] = 0x11;
  addr.addr[1] = 0x22;
  addr.addr[2] = 0x33;
  addr.addr[3] = 0x44;
  addr.addr[4] = 0x55;
  addr.addr[5] = 0x66;

  g_br_device = br_device_create(&addr);
  g_le_device = le_device_create(&addr, BT_LE_ADDR_TYPE_PUBLIC);

  g_mock_do_in_service_loop_called = 0;
  g_mock_do_in_service_loop_data = NULL;

  return 0;
}

int test_device_teardown(FAR void **state)
{
  if (g_br_device)
    {
      device_delete(g_br_device);
      g_br_device = NULL;
    }

  if (g_le_device)
    {
      device_delete(g_le_device);
      g_le_device = NULL;
    }

  return 0;
}

/****************************************************************************
 * Test Cases: br_device_create / le_device_create / device_delete
 ****************************************************************************/

void test_br_device_create_normal(FAR void **state)
{
  assert_non_null(g_br_device);
  assert_int_equal(device_get_transport(g_br_device), BT_TRANSPORT_BREDR);
  assert_int_equal(device_get_address_type(g_br_device),
                   BT_LE_ADDR_TYPE_UNKNOWN);
  assert_int_equal(device_get_connection_state(g_br_device),
                   CONNECTION_STATE_DISCONNECTED);
  assert_int_equal(device_get_bond_state(g_br_device), BOND_STATE_NONE);
  assert_string_equal(device_get_name(g_br_device), "");
  assert_string_equal(device_get_alias(g_br_device), "");

  /* Verify address was copied correctly */

  bt_address_t *addr = device_get_address(g_br_device);
  assert_int_equal(addr->addr[0], 0x11);
  assert_int_equal(addr->addr[5], 0x66);
}

void test_le_device_create_normal(FAR void **state)
{
  assert_non_null(g_le_device);
  assert_int_equal(device_get_transport(g_le_device), BT_TRANSPORT_BLE);
  assert_int_equal(device_get_address_type(g_le_device),
                   BT_LE_ADDR_TYPE_PUBLIC);
  assert_int_equal(device_get_connection_state(g_le_device),
                   CONNECTION_STATE_DISCONNECTED);
  assert_int_equal(device_get_local_role(g_le_device),
                   BT_LINK_ROLE_UNKNOWN);
}

void test_device_delete_with_uuids(FAR void **state)
{
  bt_address_t addr;
  bt_uuid_t uuids[2];

  memset(&addr, 0, sizeof(addr));
  bt_device_t *dev = br_device_create(&addr);
  assert_non_null(dev);

  /* Set some UUIDs so device_delete has to free them */

  memset(uuids, 0, sizeof(uuids));
  uuids[0].type = BT_UUID16_TYPE;
  uuids[0].val.u16 = 0x1101;
  uuids[1].type = BT_UUID16_TYPE;
  uuids[1].val.u16 = 0x110A;
  device_set_uuids(dev, uuids, 2);

  /* Should not crash */

  device_delete(dev);
}

/****************************************************************************
 * Test Cases: transport
 ****************************************************************************/

void test_device_get_transport_bredr(FAR void **state)
{
  assert_int_equal(device_get_transport(g_br_device), BT_TRANSPORT_BREDR);
}

void test_device_get_transport_ble(FAR void **state)
{
  assert_int_equal(device_get_transport(g_le_device), BT_TRANSPORT_BLE);
}

/****************************************************************************
 * Test Cases: address
 ****************************************************************************/

void test_device_get_address_normal(FAR void **state)
{
  bt_address_t *addr = device_get_address(g_br_device);
  assert_non_null(addr);
  assert_int_equal(addr->addr[0], 0x11);
  assert_int_equal(addr->addr[1], 0x22);
  assert_int_equal(addr->addr[2], 0x33);
  assert_int_equal(addr->addr[3], 0x44);
  assert_int_equal(addr->addr[4], 0x55);
  assert_int_equal(addr->addr[5], 0x66);
}

/****************************************************************************
 * Test Cases: identity address
 ****************************************************************************/

void test_device_set_identity_address_normal(FAR void **state)
{
  bt_address_t id_addr;

  memset(&id_addr, 0xAA, sizeof(id_addr));
  device_set_identity_address(g_le_device, &id_addr);

  bt_address_t *result = device_get_identity_address(g_le_device);
  assert_memory_equal(result->addr, id_addr.addr, 6);
}

void test_device_set_identity_address_null(FAR void **state)
{
  bt_address_t id_addr;

  memset(&id_addr, 0xBB, sizeof(id_addr));
  device_set_identity_address(g_le_device, &id_addr);

  /* Setting NULL should clear to empty */

  device_set_identity_address(g_le_device, NULL);

  bt_address_t *result = device_get_identity_address(g_le_device);
  assert_true(bt_addr_is_empty(result));
}

/****************************************************************************
 * Test Cases: address type
 ****************************************************************************/

void test_device_get_set_address_type(FAR void **state)
{
  device_set_address_type(g_le_device, BT_LE_ADDR_TYPE_RANDOM);
  assert_int_equal(device_get_address_type(g_le_device),
                   BT_LE_ADDR_TYPE_RANDOM);

  device_set_address_type(g_le_device, BT_LE_ADDR_TYPE_PUBLIC);
  assert_int_equal(device_get_address_type(g_le_device),
                   BT_LE_ADDR_TYPE_PUBLIC);
}

/****************************************************************************
 * Test Cases: device type
 ****************************************************************************/

void test_device_get_set_device_type(FAR void **state)
{
  device_set_device_type(g_br_device, BT_DEVICE_TYPE_BREDR);
  assert_int_equal(device_get_device_type(g_br_device),
                   BT_DEVICE_TYPE_BREDR);

  device_set_device_type(g_br_device, BT_DEVICE_TYPE_BLE);
  assert_int_equal(device_get_device_type(g_br_device),
                   BT_DEVICE_TYPE_BLE);

  device_set_device_type(g_br_device, BT_DEVICE_TYPE_DUAL);
  assert_int_equal(device_get_device_type(g_br_device),
                   BT_DEVICE_TYPE_DUAL);
}

/****************************************************************************
 * Test Cases: name
 ****************************************************************************/

void test_device_set_name_normal(FAR void **state)
{
  bool ret = device_set_name(g_br_device, "TestDevice");
  assert_true(ret);
  assert_string_equal(device_get_name(g_br_device), "TestDevice");
}

void test_device_set_name_same_returns_false(FAR void **state)
{
  device_set_name(g_br_device, "SameName");
  bool ret = device_set_name(g_br_device, "SameName");
  assert_false(ret);
}

void test_device_set_name_updates_alias_if_empty(FAR void **state)
{
  /* Alias starts as "" (empty), so setting name should also set alias */

  device_set_name(g_br_device, "NewName");
  assert_string_equal(device_get_alias(g_br_device), "NewName");
}

void test_device_set_name_no_update_alias_if_set(FAR void **state)
{
  /* First set a name (which also sets alias since alias is empty) */

  device_set_name(g_br_device, "FirstName");
  assert_string_equal(device_get_alias(g_br_device), "FirstName");

  /* Now set alias to something different */

  device_set_alias(g_br_device, "MyAlias");

  /* Setting a new name should NOT overwrite the non-empty alias */

  device_set_name(g_br_device, "SecondName");
  assert_string_equal(device_get_name(g_br_device), "SecondName");
  assert_string_equal(device_get_alias(g_br_device), "MyAlias");
}

/****************************************************************************
 * Test Cases: device class
 ****************************************************************************/

void test_device_set_device_class_normal(FAR void **state)
{
  bool ret = device_set_device_class(g_br_device, 0x00280704);
  assert_true(ret);
  assert_int_equal(device_get_device_class(g_br_device), 0x00280704);
}

void test_device_set_device_class_same_returns_false(FAR void **state)
{
  device_set_device_class(g_br_device, 0x00280704);
  bool ret = device_set_device_class(g_br_device, 0x00280704);
  assert_false(ret);
}

/****************************************************************************
 * Test Cases: UUIDs
 ****************************************************************************/

void test_device_set_uuids_normal(FAR void **state)
{
  bt_uuid_t uuids[3];

  memset(uuids, 0, sizeof(uuids));
  uuids[0].type = BT_UUID16_TYPE;
  uuids[0].val.u16 = 0x1101;
  uuids[1].type = BT_UUID16_TYPE;
  uuids[1].val.u16 = 0x110A;
  uuids[2].type = BT_UUID16_TYPE;
  uuids[2].val.u16 = 0x110B;

  bool ret = device_set_uuids(g_br_device, uuids, 3);
  assert_true(ret);
  assert_int_equal(device_get_uuids_size(g_br_device), 3);

  bt_uuid_t out[3];
  uint16_t cnt = device_get_uuids(g_br_device, out, 3);
  assert_int_equal(cnt, 3);
  assert_int_equal(out[0].val.u16, 0x1101);
  assert_int_equal(out[1].val.u16, 0x110A);
  assert_int_equal(out[2].val.u16, 0x110B);
}

void test_device_get_uuids_partial(FAR void **state)
{
  bt_uuid_t uuids[3];

  memset(uuids, 0, sizeof(uuids));
  uuids[0].type = BT_UUID16_TYPE;
  uuids[0].val.u16 = 0x1101;
  uuids[1].type = BT_UUID16_TYPE;
  uuids[1].val.u16 = 0x110A;
  uuids[2].type = BT_UUID16_TYPE;
  uuids[2].val.u16 = 0x110B;

  device_set_uuids(g_br_device, uuids, 3);

  /* Request only 2 out of 3 */

  bt_uuid_t out[2];
  uint16_t cnt = device_get_uuids(g_br_device, out, 2);
  assert_int_equal(cnt, 2);
  assert_int_equal(out[0].val.u16, 0x1101);
  assert_int_equal(out[1].val.u16, 0x110A);
}

void test_device_get_uuids_empty(FAR void **state)
{
  bt_uuid_t out[1];
  uint16_t cnt = device_get_uuids(g_br_device, out, 1);
  assert_int_equal(cnt, 0);
  assert_int_equal(device_get_uuids_size(g_br_device), 0);
}

/****************************************************************************
 * Test Cases: appearance
 ****************************************************************************/

void test_device_get_set_appearance(FAR void **state)
{
  device_set_appearance(g_le_device, 0x0040);
  assert_int_equal(device_get_appearance(g_le_device), 0x0040);

  device_set_appearance(g_le_device, 0x0000);
  assert_int_equal(device_get_appearance(g_le_device), 0x0000);
}

/****************************************************************************
 * Test Cases: RSSI
 ****************************************************************************/

void test_device_get_set_rssi(FAR void **state)
{
  device_set_rssi(g_br_device, -50);
  assert_int_equal(device_get_rssi(g_br_device), -50);
}

void test_device_get_set_rssi_negative(FAR void **state)
{
  device_set_rssi(g_br_device, -127);
  assert_int_equal(device_get_rssi(g_br_device), -127);

  device_set_rssi(g_br_device, 0);
  assert_int_equal(device_get_rssi(g_br_device), 0);
}

/****************************************************************************
 * Test Cases: alias
 ****************************************************************************/

void test_device_set_alias_normal(FAR void **state)
{
  bool ret = device_set_alias(g_br_device, "MyAlias");
  assert_true(ret);
  assert_string_equal(device_get_alias(g_br_device), "MyAlias");
}

void test_device_set_alias_same_returns_false(FAR void **state)
{
  device_set_alias(g_br_device, "SameAlias");
  bool ret = device_set_alias(g_br_device, "SameAlias");
  assert_false(ret);
}

/****************************************************************************
 * Test Cases: connection state
 ****************************************************************************/

void test_device_get_set_connection_state(FAR void **state)
{
  device_set_connection_state(g_br_device, CONNECTION_STATE_CONNECTED);
  assert_int_equal(device_get_connection_state(g_br_device),
                   CONNECTION_STATE_CONNECTED);
}

void test_device_is_connected_true(FAR void **state)
{
  device_set_connection_state(g_br_device, CONNECTION_STATE_CONNECTED);
  assert_true(device_is_connected(g_br_device));
}

void test_device_is_connected_false(FAR void **state)
{
  device_set_connection_state(g_br_device,
                              CONNECTION_STATE_DISCONNECTED);
  assert_false(device_is_connected(g_br_device));
}

void test_device_is_encrypted_true(FAR void **state)
{
  device_set_connection_state(g_br_device, CONNECTION_STATE_ENCRYPTED_BREDR);
  assert_true(device_is_encrypted(g_br_device));
}

void test_device_is_encrypted_false(FAR void **state)
{
  device_set_connection_state(g_br_device, CONNECTION_STATE_CONNECTED);
  assert_false(device_is_encrypted(g_br_device));
}

/****************************************************************************
 * Test Cases: ACL handle
 ****************************************************************************/

void test_device_get_set_acl_handle(FAR void **state)
{
  device_set_acl_handle(g_br_device, 0x0042);
  assert_int_equal(device_get_acl_handle(g_br_device), 0x0042);

  device_set_acl_handle(g_br_device, 0xFFFF);
  assert_int_equal(device_get_acl_handle(g_br_device), 0xFFFF);
}

/****************************************************************************
 * Test Cases: local role
 ****************************************************************************/

void test_device_get_set_local_role(FAR void **state)
{
  device_set_local_role(g_br_device, BT_LINK_ROLE_MASTER);
  assert_int_equal(device_get_local_role(g_br_device), BT_LINK_ROLE_MASTER);

  device_set_local_role(g_br_device, BT_LINK_ROLE_SLAVE);
  assert_int_equal(device_get_local_role(g_br_device), BT_LINK_ROLE_SLAVE);
}

/****************************************************************************
 * Test Cases: bond initiate local
 ****************************************************************************/

void test_device_bond_initiate_local(FAR void **state)
{
  assert_false(device_is_bond_initiate_local(g_br_device));

  device_set_bond_initiate_local(g_br_device, true);
  assert_true(device_is_bond_initiate_local(g_br_device));

  device_set_bond_initiate_local(g_br_device, false);
  assert_false(device_is_bond_initiate_local(g_br_device));
}

/****************************************************************************
 * Test Cases: bond state
 ****************************************************************************/

void test_device_get_bond_state_default(FAR void **state)
{
  assert_int_equal(device_get_bond_state(g_br_device), BOND_STATE_NONE);
}

void test_device_set_bond_state_no_notify(FAR void **state)
{
  /* Pass NULL for notify_change so do_in_service_loop is not called */

  device_set_bond_state(g_br_device, BOND_STATE_BONDING, false, NULL);
  assert_int_equal(device_get_bond_state(g_br_device), BOND_STATE_BONDING);
  assert_int_equal(g_mock_do_in_service_loop_called, 0);
}

void test_device_set_bond_state_same_no_change(FAR void **state)
{
  device_set_bond_state(g_br_device, BOND_STATE_BONDING, false, NULL);
  device_set_bond_state(g_br_device, BOND_STATE_BONDING, false, NULL);

  /* State should remain the same, no extra calls */

  assert_int_equal(device_get_bond_state(g_br_device), BOND_STATE_BONDING);
}

void test_device_is_bonded(FAR void **state)
{
  assert_false(device_is_bonded(g_br_device));

  device_set_bond_state(g_br_device, BOND_STATE_BONDED, false, NULL);
  assert_true(device_is_bonded(g_br_device));

  device_set_bond_state(g_br_device, BOND_STATE_NONE, false, NULL);
  assert_false(device_is_bonded(g_br_device));
}

/****************************************************************************
 * Test Cases: link key
 ****************************************************************************/

void test_device_set_get_link_key(FAR void **state)
{
  bt_128key_t key;

  memset(key, 0xAB, sizeof(bt_128key_t));
  device_set_link_key(g_br_device, key);

  uint8_t *result = device_get_link_key(g_br_device);
  assert_memory_equal(result, key, sizeof(bt_128key_t));
}

void test_device_delete_link_key(FAR void **state)
{
  bt_128key_t key;
  bt_128key_t zero_key;

  memset(key, 0xCD, sizeof(bt_128key_t));
  memset(zero_key, 0, sizeof(bt_128key_t));

  device_set_link_key(g_br_device, key);
  device_delete_link_key(g_br_device);

  uint8_t *result = device_get_link_key(g_br_device);
  assert_memory_equal(result, zero_key, sizeof(bt_128key_t));
}

/****************************************************************************
 * Test Cases: link key type
 ****************************************************************************/

void test_device_get_set_link_key_type(FAR void **state)
{
  device_set_link_key_type(g_br_device,
    BT_LINKKEY_TYPE_AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256);
  assert_int_equal(device_get_link_key_type(g_br_device),
    BT_LINKKEY_TYPE_AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256);
}

/****************************************************************************
 * Test Cases: link policy
 ****************************************************************************/

void test_device_get_set_link_policy(FAR void **state)
{
  /* Default should be ENABLE_ROLE_SWITCH_AND_SNIFF from device_create */

  assert_int_equal(device_get_link_policy(g_br_device),
                   BT_BR_LINK_POLICY_ENABLE_ROLE_SWITCH_AND_SNIFF);

  device_set_link_policy(g_br_device, BT_BR_LINK_POLICY_DISABLE_ALL);
  assert_int_equal(device_get_link_policy(g_br_device),
                   BT_BR_LINK_POLICY_DISABLE_ALL);
}

/****************************************************************************
 * Test Cases: LE PHY
 ****************************************************************************/

void test_device_get_set_le_phy(FAR void **state)
{
  ble_phy_type_t tx;
  ble_phy_type_t rx;

  device_set_le_phy(g_le_device, BT_LE_2M_PHY, BT_LE_CODED_PHY);
  device_get_le_phy(g_le_device, &tx, &rx);
  assert_int_equal(tx, BT_LE_2M_PHY);
  assert_int_equal(rx, BT_LE_CODED_PHY);
}

/****************************************************************************
 * Test Cases: flags
 ****************************************************************************/

void test_device_set_check_flags(FAR void **state)
{
  assert_false(device_check_flag(g_br_device, DFLAG_NAME_SET));

  device_set_flags(g_br_device, DFLAG_NAME_SET);
  assert_true(device_check_flag(g_br_device, DFLAG_NAME_SET));
}

void test_device_clear_flag(FAR void **state)
{
  device_set_flags(g_br_device, DFLAG_CONNECTED);
  assert_true(device_check_flag(g_br_device, DFLAG_CONNECTED));

  device_clear_flag(g_br_device, DFLAG_CONNECTED);
  assert_false(device_check_flag(g_br_device, DFLAG_CONNECTED));
}

void test_device_flags_multiple(FAR void **state)
{
  device_set_flags(g_br_device, DFLAG_NAME_SET | DFLAG_BONDED);
  assert_true(device_check_flag(g_br_device, DFLAG_NAME_SET));
  assert_true(device_check_flag(g_br_device, DFLAG_BONDED));
  assert_false(device_check_flag(g_br_device, DFLAG_LE_KEY_SET));

  device_clear_flag(g_br_device, DFLAG_NAME_SET);
  assert_false(device_check_flag(g_br_device, DFLAG_NAME_SET));
  assert_true(device_check_flag(g_br_device, DFLAG_BONDED));
}

/****************************************************************************
 * Test Cases: SMP key
 ****************************************************************************/

void test_device_set_get_smp_key(FAR void **state)
{
  uint8_t smp_key[80];

  memset(smp_key, 0x42, sizeof(smp_key));
  device_set_smp_key(g_le_device, smp_key);

  /* set_smp_key should also set DFLAG_LE_KEY_SET */

  assert_true(device_check_flag(g_le_device, DFLAG_LE_KEY_SET));

  uint8_t *result = device_get_smp_key(g_le_device);
  assert_memory_equal(result, smp_key, 80);
}

void test_device_delete_smp_key(FAR void **state)
{
  uint8_t smp_key[80];
  uint8_t zero_key[80];

  memset(smp_key, 0x42, sizeof(smp_key));
  memset(zero_key, 0, sizeof(zero_key));

  device_set_smp_key(g_le_device, smp_key);
  assert_true(device_check_flag(g_le_device, DFLAG_LE_KEY_SET));

  device_delete_smp_key(g_le_device);
  assert_false(device_check_flag(g_le_device, DFLAG_LE_KEY_SET));

  uint8_t *result = device_get_smp_key(g_le_device);
  assert_memory_equal(result, zero_key, 80);
}

/****************************************************************************
 * Test Cases: local CSRK
 ****************************************************************************/

void test_device_set_get_local_csrk(FAR void **state)
{
  uint8_t csrk[16];

  memset(csrk, 0xEF, sizeof(csrk));
  device_set_local_csrk(g_le_device, csrk);

  uint8_t *result = device_get_local_csrk(g_le_device);
  assert_memory_equal(result, csrk, 16);
}

/****************************************************************************
 * Test Cases: device_get_le_property
 ****************************************************************************/

void test_device_get_le_property(FAR void **state)
{
  uint8_t smp_key[80];
  uint8_t csrk[16];
  remote_device_le_properties_t prop;

  memset(smp_key, 0x11, sizeof(smp_key));
  memset(csrk, 0x22, sizeof(csrk));

  device_set_smp_key(g_le_device, smp_key);
  device_set_local_csrk(g_le_device, csrk);
  device_set_device_type(g_le_device, BT_DEVICE_TYPE_BLE);

  memset(&prop, 0, sizeof(prop));
  device_get_le_property(g_le_device, &prop);

  /* Verify address was copied */

  bt_address_t *dev_addr = device_get_address(g_le_device);
  assert_memory_equal(&prop.addr, dev_addr, sizeof(bt_address_t));

  /* Verify addr_type */

  assert_int_equal(prop.addr_type, BT_LE_ADDR_TYPE_PUBLIC);

  /* Verify smp_key */

  assert_memory_equal(prop.smp_key, smp_key, 80);

  /* Verify local_csrk */

  assert_memory_equal(prop.local_csrk, csrk, 16);

  /* Verify device_type */

  assert_int_equal(prop.device_type, BT_DEVICE_TYPE_BLE);
}
