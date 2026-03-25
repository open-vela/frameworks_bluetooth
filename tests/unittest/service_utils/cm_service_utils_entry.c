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

#include "cm_btsnoop_filter.h"

int cmocka_service_utils_test_main(int argc, FAR char* argv[])
{
    const struct CMUnitTest service_utils_tests[] = {
        /* filter_init / filter_uninit */

        cmocka_unit_test_setup_teardown(test_filter_init_normal,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(test_filter_uninit_normal,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),

        /* filter_set_filter_flag */

        cmocka_unit_test_setup_teardown(test_filter_set_flag_a2dp,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(test_filter_set_flag_spp,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(test_filter_set_flag_invalid,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(test_filter_set_flag_unfilter,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),

        /* filter_remove_filter_flag */

        cmocka_unit_test_setup_teardown(test_filter_remove_flag_normal,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(test_filter_remove_flag_invalid,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(test_filter_remove_flag_unfilter,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),

        /* filter_can_filter */

        cmocka_unit_test_setup_teardown(test_filter_can_filter_hci_command,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(
            test_filter_can_filter_hci_event_nocp,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(
            test_filter_can_filter_hci_event_connect,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(test_filter_can_filter_sco_data,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
        cmocka_unit_test_setup_teardown(
            test_filter_can_filter_unknown_type,
            test_btsnoop_filter_setup, test_btsnoop_filter_teardown),
    };

    return cmocka_run_group_tests(service_utils_tests, NULL, NULL);
}
