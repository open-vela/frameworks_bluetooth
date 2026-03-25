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

#include <cmocka.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/****************************************************************************
 * Include btsnoop_filter.c directly to access internal structs.
 * We need to mock the log macros and provide bt_list dependencies.
 ****************************************************************************/

#include "bluetooth.h"
#include "bluetooth_define.h"
#include "bt_list.h"
#include "bt_trace.h"

/* Include the source under test */

#include "../../../../service/utils/btsnoop_filter.c"

#include "cm_btsnoop_filter.h"

/****************************************************************************
 * Setup / Teardown
 ****************************************************************************/

int test_btsnoop_filter_setup(FAR void** state)
{
    filter_init();
    return 0;
}

int test_btsnoop_filter_teardown(FAR void** state)
{
    filter_uninit();
    return 0;
}

/****************************************************************************
 * Test Cases: filter_init / filter_uninit
 ****************************************************************************/

void test_filter_init_normal(FAR void** state)
{
    /* filter_init was called in setup, verify state */

    assert_non_null(g_snoop_filter.acl_connection_list);
    assert_int_equal(g_snoop_filter.filter_items, 0);
}

void test_filter_uninit_normal(FAR void** state)
{
    /* Uninit and verify list is freed */

    filter_uninit();
    assert_null(g_snoop_filter.acl_connection_list);

    /* Re-init for teardown */

    filter_init();
}

/****************************************************************************
 * Test Cases: filter_set_filter_flag
 ****************************************************************************/

void test_filter_set_flag_a2dp(FAR void** state)
{
    int ret = filter_set_filter_flag(BTSNOOP_FILTER_A2DP_AUDIO);
    assert_int_equal(ret, 0);
    assert_true(g_snoop_filter.filter_items & (1ULL << BTSNOOP_FILTER_A2DP_AUDIO));
}

void test_filter_set_flag_spp(FAR void** state)
{
    int ret = filter_set_filter_flag(BTSNOOP_FILTER_SPP);
    assert_int_equal(ret, 0);
    assert_true(g_snoop_filter.filter_items & (1ULL << BTSNOOP_FILTER_SPP));
}

void test_filter_set_flag_invalid(FAR void** state)
{
    int ret = filter_set_filter_flag(BTSNOOP_FILTER_MAX);
    assert_int_not_equal(ret, 0);
}

void test_filter_set_flag_unfilter(FAR void** state)
{
    int ret = filter_set_filter_flag(BTSNOOP_FILTER_UNFILTER);
    assert_int_not_equal(ret, 0);
}

/****************************************************************************
 * Test Cases: filter_remove_filter_flag
 ****************************************************************************/

void test_filter_remove_flag_normal(FAR void** state)
{
    /* Set then remove */

    filter_set_filter_flag(BTSNOOP_FILTER_A2DP_AUDIO);
    assert_true(g_snoop_filter.filter_items & (1ULL << BTSNOOP_FILTER_A2DP_AUDIO));

    int ret = filter_remove_filter_flag(BTSNOOP_FILTER_A2DP_AUDIO);
    assert_int_equal(ret, 0);
    assert_false(g_snoop_filter.filter_items & (1ULL << BTSNOOP_FILTER_A2DP_AUDIO));
}

void test_filter_remove_flag_invalid(FAR void** state)
{
    int ret = filter_remove_filter_flag(BTSNOOP_FILTER_MAX);
    assert_int_not_equal(ret, 0);
}

void test_filter_remove_flag_unfilter(FAR void** state)
{
    int ret = filter_remove_filter_flag(BTSNOOP_FILTER_UNFILTER);
    assert_int_not_equal(ret, 0);
}

/****************************************************************************
 * Test Cases: filter_can_filter - HCI command
 ****************************************************************************/

void test_filter_can_filter_hci_command(FAR void** state)
{
    /* HCI command type = 0x01, should never be filtered */

    uint8_t pkt[] = { BTSNOOP_HCI_TYPE_HCI_COMMAND, 0x01, 0x10, 0x00 };
    bool result = filter_can_filter(0, pkt, sizeof(pkt));
    assert_false(result);
}

/****************************************************************************
 * Test Cases: filter_can_filter - HCI event
 ****************************************************************************/

void test_filter_can_filter_hci_event_nocp(FAR void** state)
{
    /* Set NOCP filter flag */

    filter_set_filter_flag(BTSNOOP_FILTER_NOCP);

    /* Build a Number of Completed Packets event (0x13) */
    /* H4 type (0x04=event) + event code (0x13) + param len + data */

    uint8_t pkt[] = { BTSNOOP_HCI_TYPE_HCI_EVENT,
        BTSNOOP_NUMBER_OF_COMPLETED_PACKETS,
        0x05, 0x01, 0x01, 0x00, 0x01, 0x00 };
    bool result = filter_can_filter(1, pkt, sizeof(pkt));
    assert_true(result);
}

void test_filter_can_filter_hci_event_connect(FAR void** state)
{
    /* Connect complete event should not be filtered (it updates state) */

    uint8_t pkt[16];
    memset(pkt, 0, sizeof(pkt));
    pkt[0] = BTSNOOP_HCI_TYPE_HCI_EVENT;
    pkt[1] = BTSNOOP_CONNECT_COMPLETE;
    pkt[2] = 11; /* param length */
    pkt[3] = BTSNOOP_HCI_EVENT_STATUS_SUCCESS; /* status */
    /* connection handle = 0x0001 */
    pkt[4] = 0x01;
    pkt[5] = 0x00;

    bool result = filter_can_filter(1, pkt, 14);
    assert_false(result);
}

/****************************************************************************
 * Test Cases: filter_can_filter - SCO data
 ****************************************************************************/

void test_filter_can_filter_sco_data(FAR void** state)
{
    /* SCO data type = 0x03, handle_sco_data always returns 1 (filtered) */

    uint8_t pkt[] = { BTSNOOP_HCI_TYPE_SCO_DATA, 0x01, 0x00, 0x10 };
    bool result = filter_can_filter(0, pkt, sizeof(pkt));
    assert_true(result);
}

/****************************************************************************
 * Test Cases: filter_can_filter - unknown type
 ****************************************************************************/

void test_filter_can_filter_unknown_type(FAR void** state)
{
    /* Unknown HCI type should return false (not filtered) */

    uint8_t pkt[] = { 0xFF, 0x01, 0x00, 0x00 };
    bool result = filter_can_filter(0, pkt, sizeof(pkt));
    assert_false(result);
}
