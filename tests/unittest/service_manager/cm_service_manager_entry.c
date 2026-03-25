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

#include "cm_service_manager.h"

int cmocka_service_manager_test_main(int argc, FAR char* argv[])
{
    const struct CMUnitTest service_manager_tests[] = {
        /* register_service */

        cmocka_unit_test_setup_teardown(test_register_service_normal,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(test_register_service_duplicate,
            test_service_manager_setup, test_service_manager_teardown),

        /* service_manager_init */

        cmocka_unit_test_setup_teardown(test_service_manager_init_normal,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(test_service_manager_init_no_profiles,
            test_service_manager_setup, test_service_manager_teardown),

        /* service_manager_startup */

        cmocka_unit_test_setup_teardown(test_service_manager_startup_normal,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(
            test_service_manager_startup_all_already_started,
            test_service_manager_setup, test_service_manager_teardown),

        /* service_manager_shutdown */

        cmocka_unit_test_setup_teardown(test_service_manager_shutdown_normal,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(
            test_service_manager_shutdown_all_already_off,
            test_service_manager_setup, test_service_manager_teardown),

        /* service_manager_get_uuid */

        cmocka_unit_test_setup_teardown(
            test_service_manager_get_uuid_normal,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(
            test_service_manager_get_uuid_empty,
            test_service_manager_setup, test_service_manager_teardown),

        /* service_manager_processmsg */

        cmocka_unit_test_setup_teardown(
            test_service_manager_processmsg_normal,
            test_service_manager_setup, test_service_manager_teardown),

        /* service_manager_control */

        cmocka_unit_test_setup_teardown(
            test_service_manager_control_start,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(
            test_service_manager_control_stop,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(
            test_service_manager_control_not_supported,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(
            test_service_manager_control_dump,
            test_service_manager_setup, test_service_manager_teardown),

        /* service_manager_cleanup */

        cmocka_unit_test_setup_teardown(
            test_service_manager_cleanup_normal,
            test_service_manager_setup, test_service_manager_teardown),
        cmocka_unit_test_setup_teardown(
            test_service_manager_cleanup_null_cleanup,
            test_service_manager_setup, test_service_manager_teardown),
    };

    return cmocka_run_group_tests(service_manager_tests, NULL, NULL);
}
