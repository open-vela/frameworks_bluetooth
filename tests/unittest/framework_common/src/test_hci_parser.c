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

// clang-format off
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
// clang-format on

#include <string.h>

#include "../../../../service/src/hci_parser.c"

#include "cm_hci_parser.h"

/****************************************************************************
 * Public Functions - Setup/Teardown
 ****************************************************************************/

int test_hci_parser_setup(FAR void** state)
{
    return 0;
}

int test_hci_parser_teardown(FAR void** state)
{
    return 0;
}

/****************************************************************************
 * Public Functions - Test Cases: hci_get_result
 ****************************************************************************/

void test_hci_get_result_command_complete_success(FAR void** state)
{
    uint8_t buf[sizeof(bt_hci_event_t) + sizeof(bt_hci_event_command_complete_t) + 1];
    bt_hci_event_t* event = (bt_hci_event_t*)buf;
    bt_hci_event_command_complete_t* cc;

    memset(buf, 0, sizeof(buf));
    event->evt_code = HCI_EV_COMMAND_COMPLETE;
    event->length = sizeof(bt_hci_event_command_complete_t) + 1;
    cc = (bt_hci_event_command_complete_t*)(event->params);
    cc->num_packets = 1;
    cc->opcode = 0x0C03;
    cc->return_param[0] = HCI_SUCCESS;

    assert_int_equal(hci_get_result(event), HCI_SUCCESS);
}

void test_hci_get_result_command_complete_error(FAR void** state)
{
    uint8_t buf[sizeof(bt_hci_event_t) + sizeof(bt_hci_event_command_complete_t) + 1];
    bt_hci_event_t* event = (bt_hci_event_t*)buf;
    bt_hci_event_command_complete_t* cc;

    memset(buf, 0, sizeof(buf));
    event->evt_code = HCI_EV_COMMAND_COMPLETE;
    event->length = sizeof(bt_hci_event_command_complete_t) + 1;
    cc = (bt_hci_event_command_complete_t*)(event->params);
    cc->num_packets = 1;
    cc->opcode = 0x0C03;
    cc->return_param[0] = HCI_ERR_HARDWARE_FAILURE;

    assert_int_equal(hci_get_result(event), HCI_ERR_HARDWARE_FAILURE);
}

void test_hci_get_result_command_status_success(FAR void** state)
{
    uint8_t buf[sizeof(bt_hci_event_t) + sizeof(bt_hci_event_command_status_t)];
    bt_hci_event_t* event = (bt_hci_event_t*)buf;
    bt_hci_event_command_status_t* cs;

    memset(buf, 0, sizeof(buf));
    event->evt_code = HCI_EV_COMMAND_STATUS;
    event->length = sizeof(bt_hci_event_command_status_t);
    cs = (bt_hci_event_command_status_t*)(event->params);
    cs->status = HCI_SUCCESS;
    cs->num_packets = 1;
    cs->opcode = 0x0406;

    assert_int_equal(hci_get_result(event), HCI_SUCCESS);
}

void test_hci_get_result_command_status_error(FAR void** state)
{
    uint8_t buf[sizeof(bt_hci_event_t) + sizeof(bt_hci_event_command_status_t)];
    bt_hci_event_t* event = (bt_hci_event_t*)buf;
    bt_hci_event_command_status_t* cs;

    memset(buf, 0, sizeof(buf));
    event->evt_code = HCI_EV_COMMAND_STATUS;
    event->length = sizeof(bt_hci_event_command_status_t);
    cs = (bt_hci_event_command_status_t*)(event->params);
    cs->status = HCI_ERR_COMMAND_DISALLOWED;
    cs->num_packets = 1;
    cs->opcode = 0x0406;

    assert_int_equal(hci_get_result(event), HCI_ERR_COMMAND_DISALLOWED);
}

void test_hci_get_result_unexpected_event(FAR void** state)
{
    uint8_t buf[sizeof(bt_hci_event_t) + 4];
    bt_hci_event_t* event = (bt_hci_event_t*)buf;

    memset(buf, 0, sizeof(buf));
    event->evt_code = 0xFF;
    event->length = 4;

    assert_int_equal(hci_get_result(event), HCI_ERR_UNSPECIFIED_ERROR);
}
