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

#include <nuttx/config.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include "cm_device.h"

int cmocka_service_device_test_main(int argc, FAR char* argv[])
{
    const struct CMUnitTest service_device_tests[] = {
        /* br_device_create / le_device_create / device_delete */

        cmocka_unit_test_setup_teardown(test_br_device_create_normal,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_le_device_create_normal,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_delete_with_uuids,
            test_device_setup, test_device_teardown),

        /* transport */

        cmocka_unit_test_setup_teardown(test_device_get_transport_bredr,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_get_transport_ble,
            test_device_setup, test_device_teardown),

        /* address */

        cmocka_unit_test_setup_teardown(test_device_get_address_normal,
            test_device_setup, test_device_teardown),

        /* identity address */

        cmocka_unit_test_setup_teardown(test_device_set_identity_address_normal,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_identity_address_null,
            test_device_setup, test_device_teardown),

        /* address type */

        cmocka_unit_test_setup_teardown(test_device_get_set_address_type,
            test_device_setup, test_device_teardown),

        /* device type */

        cmocka_unit_test_setup_teardown(test_device_get_set_device_type,
            test_device_setup, test_device_teardown),

        /* name */

        cmocka_unit_test_setup_teardown(test_device_set_name_normal,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_name_same_returns_false,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_name_updates_alias_if_empty,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_name_no_update_alias_if_set,
            test_device_setup, test_device_teardown),

        /* device class */

        cmocka_unit_test_setup_teardown(test_device_set_device_class_normal,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_device_class_same_returns_false,
            test_device_setup, test_device_teardown),

        /* UUIDs */

        cmocka_unit_test_setup_teardown(test_device_set_uuids_normal,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_get_uuids_partial,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_get_uuids_empty,
            test_device_setup, test_device_teardown),

        /* appearance */

        cmocka_unit_test_setup_teardown(test_device_get_set_appearance,
            test_device_setup, test_device_teardown),

        /* RSSI */

        cmocka_unit_test_setup_teardown(test_device_get_set_rssi,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_get_set_rssi_negative,
            test_device_setup, test_device_teardown),

        /* alias */

        cmocka_unit_test_setup_teardown(test_device_set_alias_normal,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_alias_same_returns_false,
            test_device_setup, test_device_teardown),

        /* connection state */

        cmocka_unit_test_setup_teardown(test_device_get_set_connection_state,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_is_connected_true,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_is_connected_false,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_is_encrypted_true,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_is_encrypted_false,
            test_device_setup, test_device_teardown),

        /* ACL handle */

        cmocka_unit_test_setup_teardown(test_device_get_set_acl_handle,
            test_device_setup, test_device_teardown),

        /* local role */

        cmocka_unit_test_setup_teardown(test_device_get_set_local_role,
            test_device_setup, test_device_teardown),

        /* bond initiate local */

        cmocka_unit_test_setup_teardown(test_device_bond_initiate_local,
            test_device_setup, test_device_teardown),

        /* bond state */

        cmocka_unit_test_setup_teardown(test_device_get_bond_state_default,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_bond_state_no_notify,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_set_bond_state_same_no_change,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_is_bonded,
            test_device_setup, test_device_teardown),

        /* link key */

        cmocka_unit_test_setup_teardown(test_device_set_get_link_key,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_delete_link_key,
            test_device_setup, test_device_teardown),

        /* link key type */

        cmocka_unit_test_setup_teardown(test_device_get_set_link_key_type,
            test_device_setup, test_device_teardown),

        /* link policy */

        cmocka_unit_test_setup_teardown(test_device_get_set_link_policy,
            test_device_setup, test_device_teardown),

        /* LE PHY */

        cmocka_unit_test_setup_teardown(test_device_get_set_le_phy,
            test_device_setup, test_device_teardown),

        /* flags */

        cmocka_unit_test_setup_teardown(test_device_set_check_flags,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_clear_flag,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_flags_multiple,
            test_device_setup, test_device_teardown),

        /* SMP key */

        cmocka_unit_test_setup_teardown(test_device_set_get_smp_key,
            test_device_setup, test_device_teardown),
        cmocka_unit_test_setup_teardown(test_device_delete_smp_key,
            test_device_setup, test_device_teardown),

        /* local CSRK */

        cmocka_unit_test_setup_teardown(test_device_set_get_local_csrk,
            test_device_setup, test_device_teardown),

        /* LE property */

        cmocka_unit_test_setup_teardown(test_device_get_le_property,
            test_device_setup, test_device_teardown),
    };

    return cmocka_run_group_tests(service_device_tests, NULL, NULL);
}
