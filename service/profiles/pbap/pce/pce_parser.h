/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#ifndef __PCE_PARSER_H__
#define __PCE_PARSER_H__

#include "bt_addr.h"
#include "bt_device.h"
#include "pbap_pce_service.h"

typedef struct {
    char card_handle[BT_PBAP_PCE_VCARD_HANDLE_MAX_LEN];
    char contact_name[BT_PBAP_PCE_PROPERTY_MAX_LEN];
} pce_vcard_entry_t;

bt_status_t pce_parse_card_list(uint16_t data_len, char* data, bt_list_t** card_list);
bt_status_t pce_parse_card_v2_1(uint16_t data_len, char* data, bt_pce_contact_t* contact);
bt_status_t pce_parse_card_v3_0(uint16_t data_len, char* data, bt_pce_contact_t* contact);

#endif  // __PCE_PARSER_H__