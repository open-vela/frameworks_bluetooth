/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#define LOG_TAG "hci_parser"

#include <assert.h>

#include "hci_parser.h"
#include "utils/log.h"

hci_error_t hci_get_result(bt_hci_event_t* event)
{
    hci_error_t result = HCI_ERR_UNSPECIFIED_ERROR;
    bt_hci_event_command_complete_t* cmd_complete_ev;
    bt_hci_event_command_status_t* cmd_status_ev;

    assert(event);

    switch (event->evt_code) {
    case HCI_EV_COMMAND_COMPLETE:
        cmd_complete_ev = (bt_hci_event_command_complete_t*)(event->params);
        result = cmd_complete_ev->return_param[0];
        break;

    case HCI_EV_COMMAND_STATUS:
        cmd_status_ev = (bt_hci_event_command_status_t*)(event->params);
        result = cmd_status_ev->status;

        /* A success in Command Status event indicates a command is pending rather than success. */

        break;

    default:
        BT_LOGW("Unexpected event code: 0x%0x", event->evt_code);
        break;
    }

    return result;
}