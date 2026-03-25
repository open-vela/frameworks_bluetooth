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

#ifndef __TESTING_CMOCKA_SERVICE_UTILS_CM_BTSNOOP_FILTER_H
#define __TESTING_CMOCKA_SERVICE_UTILS_CM_BTSNOOP_FILTER_H

int test_btsnoop_filter_setup(FAR void** state);
int test_btsnoop_filter_teardown(FAR void** state);

/* filter_init / filter_uninit */

void test_filter_init_normal(FAR void** state);
void test_filter_uninit_normal(FAR void** state);

/* filter_set_filter_flag */

void test_filter_set_flag_a2dp(FAR void** state);
void test_filter_set_flag_spp(FAR void** state);
void test_filter_set_flag_invalid(FAR void** state);
void test_filter_set_flag_unfilter(FAR void** state);

/* filter_remove_filter_flag */

void test_filter_remove_flag_normal(FAR void** state);
void test_filter_remove_flag_invalid(FAR void** state);
void test_filter_remove_flag_unfilter(FAR void** state);

/* filter_can_filter - HCI command */

void test_filter_can_filter_hci_command(FAR void** state);

/* filter_can_filter - HCI event */

void test_filter_can_filter_hci_event_nocp(FAR void** state);
void test_filter_can_filter_hci_event_connect(FAR void** state);

/* filter_can_filter - SCO data */

void test_filter_can_filter_sco_data(FAR void** state);

/* filter_can_filter - unknown type */

void test_filter_can_filter_unknown_type(FAR void** state);

#endif /* __TESTING_CMOCKA_SERVICE_UTILS_CM_BTSNOOP_FILTER_H */
