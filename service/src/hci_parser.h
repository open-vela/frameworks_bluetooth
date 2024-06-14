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
#ifndef __HCI_PARSER_H__
#define __HCI_PARSER_H__

#include "bluetooth.h"
#include "hci_error.h"

typedef enum {
    HCI_EV_COMMAND_COMPLETE = 0x0E,
    HCI_EV_COMMAND_STATUS = 0x0F,
} hci_event_code_t;

typedef struct __attribute__((packed)) {
    uint8_t num_packets;
    uint16_t opcode;
    uint8_t return_param[0]; /* variable length */
} bt_hci_event_command_complete_t;

typedef struct __attribute__((packed)) {
    uint8_t status;
    uint8_t num_packets;
    uint16_t opcode;
} bt_hci_event_command_status_t;

hci_error_t hci_get_result(bt_hci_event_t* event);

#endif /* __HCI_PARSER_H__ */