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

#include "cm_advertiser_data.h"
#include "cm_bt_addr.h"
#include "cm_bt_hash.h"
#include "cm_bt_list.h"
#include "cm_bt_time.h"
#include "cm_bt_uuid.h"
#include "cm_callbacks_list.h"
#include "cm_hci_parser.h"
#include "cm_index_allocator.h"
#include "cm_scan_filter.h"
#include "cm_scan_record.h"
#include "cm_state_machine.h"

int cmocka_framework_common_test_main(int argc, FAR char* argv[])
{
    const struct CMUnitTest framework_common_tests[] = {
        /* bt_addr tests */

        cmocka_unit_test_setup_teardown(test_bt_addr_is_empty_normal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_is_empty_nonempty,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_set_empty_normal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_compare_equal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_compare_not_equal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_ba2str_normal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_bastr_normal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_bastr_null_param,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_str2ba_normal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_str2ba_invalid_input,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_str2ba_null_str,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_str2ba_short_str,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_str2ba_bad_separator,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_str2ba_non_hex,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_set_normal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bt_addr_swap_normal,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bachk_valid,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bachk_null,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bachk_wrong_length,
            test_bt_addr_setup, test_bt_addr_teardown),
        cmocka_unit_test_setup_teardown(test_bachk_bad_format,
            test_bt_addr_setup, test_bt_addr_teardown),

        /* bt_uuid tests */

        cmocka_unit_test_setup_teardown(test_bt_uuid16_create_normal,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid32_create_normal,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid128_create_normal,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_create_common_uuid16,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_create_common_uuid32,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_create_common_uuid128,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_create_common_invalid_type,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_to_uuid128_from_uuid16,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_to_uuid128_from_uuid32,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_to_uuid128_from_uuid128,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_to_uuid16_from_uuid128,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_to_uuid16_from_uuid16,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_compare_equal,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_compare_not_equal,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_compare_cross_type,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_to_string_uuid16,
            test_bt_uuid_setup, test_bt_uuid_teardown),
        cmocka_unit_test_setup_teardown(test_bt_uuid_to_string_uuid128,
            test_bt_uuid_setup, test_bt_uuid_teardown),

        /* bt_list tests */

        cmocka_unit_test_setup_teardown(test_bt_list_new_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_new_with_free_cb,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_free_null,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_is_empty_new_list,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_length_empty,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_length_after_add,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_add_head_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_add_tail_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_add_order,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_head_tail_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_next_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_next_null,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_node_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_remove_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_remove_node_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_remove_not_found,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_clear_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_move_to_head,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_move_to_tail,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_foreach_normal,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_find_found,
            test_bt_list_setup, test_bt_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_list_find_not_found,
            test_bt_list_setup, test_bt_list_teardown),

        /* bt_hash tests */

        cmocka_unit_test_setup_teardown(test_bt_hash4_normal,
            test_bt_hash_setup, test_bt_hash_teardown),
        cmocka_unit_test_setup_teardown(test_bt_hash4_empty,
            test_bt_hash_setup, test_bt_hash_teardown),
        cmocka_unit_test_setup_teardown(test_bt_hash4_single_byte,
            test_bt_hash_setup, test_bt_hash_teardown),
        cmocka_unit_test_setup_teardown(test_bt_hash4_deterministic,
            test_bt_hash_setup, test_bt_hash_teardown),
        cmocka_unit_test_setup_teardown(test_bt_hash4_different_input,
            test_bt_hash_setup, test_bt_hash_teardown),

        /* bt_time tests */

        cmocka_unit_test_setup_teardown(test_bt_get_os_timestamp_us_normal,
            test_bt_time_setup, test_bt_time_teardown),
        cmocka_unit_test_setup_teardown(test_bt_get_os_timestamp_us_monotonic,
            test_bt_time_setup, test_bt_time_teardown),
        cmocka_unit_test_setup_teardown(test_bt_get_os_timestamp_ms_normal,
            test_bt_time_setup, test_bt_time_teardown),
        cmocka_unit_test_setup_teardown(test_bt_get_os_timestamp_ms_monotonic,
            test_bt_time_setup, test_bt_time_teardown),

        /* callbacks_list tests */

        cmocka_unit_test_setup_teardown(test_bt_callbacks_list_new_normal,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_list_new_zero_max,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_register_normal,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_register_duplicate,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_register_max_reached,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_unregister_normal,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_unregister_not_found,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_remote_callbacks_register_normal,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_remote_callbacks_register_duplicate_remote,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_remote_callbacks_unregister_normal,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_remote_callbacks_unregister_with_remote_out,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_list_free_normal,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_list_free_null,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_list_count_normal,
            test_callbacks_list_setup, test_callbacks_list_teardown),
        cmocka_unit_test_setup_teardown(test_bt_callbacks_list_count_after_register,
            test_callbacks_list_setup, test_callbacks_list_teardown),

        /* state_machine tests */

        cmocka_unit_test_setup_teardown(test_hsm_ctor_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_ctor_null,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_dtor_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_transition_to_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_transition_to_from_existing,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_current_state_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_current_state_null,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_previous_state_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_previous_state_null,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_current_state_name_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_current_state_name_null_sm,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_current_state_name_null_state,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_state_name_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_state_name_null,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_state_value_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_get_current_state_value_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_dispatch_event_normal,
            test_state_machine_setup, test_state_machine_teardown),
        cmocka_unit_test_setup_teardown(test_hsm_dispatch_event_null_state,
            test_state_machine_setup, test_state_machine_teardown),

        /* index_allocator tests */

        cmocka_unit_test_setup_teardown(test_index_allocator_create_normal,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_allocator_create_max_one,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_allocator_create_max_zero,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_allocator_create_max_boundary_32,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_allocator_create_max_large,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_allocator_delete_normal,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_allocator_delete_double_delete,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_alloc_normal,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_alloc_sequential,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_alloc_exhaust,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_alloc_wrap_around,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_alloc_after_free,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_alloc_single_slot,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_free_normal,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_free_resets_next,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_free_high_bit,
            test_index_allocator_setup, test_index_allocator_teardown),
        cmocka_unit_test_setup_teardown(test_index_free_and_realloc,
            test_index_allocator_setup, test_index_allocator_teardown),

        /* hci_parser tests */

        cmocka_unit_test_setup_teardown(test_hci_get_result_command_complete_success,
            test_hci_parser_setup, test_hci_parser_teardown),
        cmocka_unit_test_setup_teardown(test_hci_get_result_command_complete_error,
            test_hci_parser_setup, test_hci_parser_teardown),
        cmocka_unit_test_setup_teardown(test_hci_get_result_command_status_success,
            test_hci_parser_setup, test_hci_parser_teardown),
        cmocka_unit_test_setup_teardown(test_hci_get_result_command_status_error,
            test_hci_parser_setup, test_hci_parser_teardown),
        cmocka_unit_test_setup_teardown(test_hci_get_result_unexpected_event,
            test_hci_parser_setup, test_hci_parser_teardown),

        /* scan_record tests */

        cmocka_unit_test_setup_teardown(test_scan_record_parse_null_data,
            test_scan_record_setup, test_scan_record_teardown),
        cmocka_unit_test_setup_teardown(test_scan_record_parse_empty_data,
            test_scan_record_setup, test_scan_record_teardown),
        cmocka_unit_test_setup_teardown(test_scan_record_parse_uuid16_service_data,
            test_scan_record_setup, test_scan_record_teardown),
        cmocka_unit_test_setup_teardown(test_scan_record_parse_unknown_type,
            test_scan_record_setup, test_scan_record_teardown),
        cmocka_unit_test_setup_teardown(test_scan_record_parse_zero_field_len,
            test_scan_record_setup, test_scan_record_teardown),
        cmocka_unit_test_setup_teardown(test_scan_record_parse_truncated_data,
            test_scan_record_setup, test_scan_record_teardown),

        /* scan_filter tests */

        cmocka_unit_test_setup_teardown(test_scanner_match_filter_uuid_match,
            test_scan_filter_setup, test_scan_filter_teardown),
        cmocka_unit_test_setup_teardown(test_scanner_match_filter_uuid_no_match,
            test_scan_filter_setup, test_scan_filter_teardown),
        cmocka_unit_test_setup_teardown(test_scanner_match_filter_no_uuid,
            test_scan_filter_setup, test_scan_filter_teardown),
        cmocka_unit_test_setup_teardown(test_scanner_match_filter_second_slot_match,
            test_scan_filter_setup, test_scan_filter_teardown),
        cmocka_unit_test_setup_teardown(test_scanner_match_filter_empty_filter,
            test_scan_filter_setup, test_scan_filter_teardown),

        /* advertiser_data tests */

        cmocka_unit_test_setup_teardown(test_advertiser_data_new_normal,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_set_flags_normal,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_set_name_short,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_set_name_complete,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_set_appearance_normal,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_add_data_normal,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_add_manufacture_data_normal,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_add_service_uuid16,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_add_service_uuid128,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_add_service_uuid_invalid,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_build_empty,
            test_advertiser_data_setup, test_advertiser_data_teardown),
        cmocka_unit_test_setup_teardown(test_advertiser_data_build_with_flags,
            test_advertiser_data_setup, test_advertiser_data_teardown),
    };

    return cmocka_run_group_tests(framework_common_tests, NULL, NULL);
}
