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
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "pce_parser.h"
#include "utils/log.h"

#define VCARD_BEGIN_PRO "BEGIN:VCARD"
#define VCARD_END_PRO "END:VCARD"
#define VCARD_VERSION_PRO "VERSION"
#define VCARD_NAME_PRO "N"
#define VCARD_FORMATTED_NAME_PRO "FN"
#define VCARD_TEL_PRO "TEL"
#define VCARD_VERSION_2_1 "2.1"
#define VCARD_VERSION_3_0 "3.0"

#define VCARD_LIST_CARD_ENTRY_STR "card handle"

#define min(a, b) ((a) < (b) ? (a) : (b))

typedef enum {
    VCARD_BEGIN,
    VCARD_END,
    VCARD_VERSION,
    VCARD_NAME,
    VCARD_FORMATTED_NAME,
    VCARD_TEL,
    VCARD_UNKNOWN
} bt_vcard_property_t;

static bool check_is_vcard_entry(char* line, char* end)
{
    if (end - line < strlen(VCARD_LIST_CARD_ENTRY_STR))
        return false;

    while (line[0] == ' ')
        line++;

    if (strncmp(line, VCARD_LIST_CARD_ENTRY_STR, sizeof(VCARD_LIST_CARD_ENTRY_STR) - 1) == 0) {
        return true;
    }

    return false;
}

static char* get_entry_value(char* data, char* property_end, char* parsed_value, uint16_t max_length)
{
    char* value_start;
    char* value_end;

    value_start = strchr(data, '\"');

    if (value_start == NULL || value_start >= property_end)
        return NULL;

    value_end = strchr(value_start + 1, '\"');

    if (value_end == NULL || value_end >= property_end)
        return NULL;

    strlcpy(parsed_value, value_start + 1,
        min(value_end - value_start, max_length));

    return value_end + 1;
}

static bt_status_t parse_card_entry(char* entry_start, char* entry_end, pce_vcard_entry_t* entry)
{
    char* data = entry_start;

    data = get_entry_value(data, entry_end, entry->card_handle, sizeof(entry->card_handle));
    if (data == NULL)
        return BT_STATUS_FAIL;

    data = get_entry_value(data, entry_end, entry->contact_name, BT_PBAP_PCE_PROPERTY_MAX_LEN);
    if (data == NULL)
        return BT_STATUS_FAIL;

    return BT_STATUS_SUCCESS;
}

bt_status_t pce_parse_card_list(uint16_t data_len, char* data, bt_list_t** card_list)
{
    int status;
    char* next_line = data;
    char* line;
    char* line_end;
    pce_vcard_entry_t* card_entry;

    *card_list = bt_list_new(free);
    while (next_line[0] != '\0') {
        line = strchr(next_line, '<');
        line_end = strchr(line, '>');

        if (line == NULL || line_end == NULL)
            return (line_end == NULL && line == NULL) ? BT_STATUS_SUCCESS : BT_STATUS_FAIL;

        if (check_is_vcard_entry(line + 1, line_end) == false) {
            next_line = line_end + 1;
            continue;
        }

        card_entry = zalloc(sizeof(pce_vcard_entry_t));

        status = parse_card_entry(line, line_end, card_entry);

        if (status != BT_STATUS_SUCCESS) {
            free(card_entry);
            continue;
        }

        bt_list_add_tail(*card_list, card_entry);

        next_line = line_end + 1;
    }

    return BT_STATUS_SUCCESS;
}

static bt_vcard_property_t get_vcard_property_T(char* property)
{
    switch (property[1])
    {
        case 'E':
            return VCARD_TEL;
        default:
            break;
    }

    return VCARD_UNKNOWN;
}

static bt_vcard_property_t get_vcard_property_N(char* property)
{
    switch (property[1])
    {
    case ':':
        return VCARD_NAME;
    default:
        break;
    }

    return VCARD_UNKNOWN;
}

static bt_vcard_property_t get_vcard_property_E(char* property)
{
    switch (property[1])
    {
    case 'N':
        return VCARD_END;
    default:
        break;
    }

    return VCARD_UNKNOWN;
}

static bt_vcard_property_t get_vcard_property_B(char* property)
{
    switch (property[1])
    {
    case 'E':
        return VCARD_BEGIN;
    default:
        break;
    }

    return VCARD_UNKNOWN;
}

static bt_vcard_property_t get_vcard_property(char* property)
{

    switch (property[0]) {
    case 'B':
        return get_vcard_property_B(property);
    case 'E':
        return get_vcard_property_E(property);
    case 'V':
        return VCARD_VERSION;
    case 'N':
        return get_vcard_property_N(property);
    case 'F':
        return VCARD_FORMATTED_NAME;
    case 'T':
        return get_vcard_property_T(property);
    default:
        return VCARD_UNKNOWN;
    }
}

static int get_property_length(char* property)
{
    int length = 0;

    while (property[length] != '\0' && property[length] != '\r' && property[length] != '\n') {
        length++;
    }

    return length;
}

static bt_status_t pce_parse_property(char* property_begin_pos, char* property_end_pos, char* dest, uint16_t dest_len)
{
    char* property_value;

    property_value = strchr(property_begin_pos, ':');
    if (property_value == NULL || property_value >= property_end_pos) {
        return BT_STATUS_FAIL;
    }

    property_value++;
    strlcpy(dest, property_value, min(dest_len, property_end_pos - property_value + 1));

    return BT_STATUS_SUCCESS;
}

bt_status_t pce_parse_card_v2_1(uint16_t data_len, char* data, bt_pce_contact_t* contact)
{
    char* curr_pos = data;
    bt_vcard_property_t property;
    uint16_t property_length;

    while (curr_pos[0] != '\0') {
        property = get_vcard_property(curr_pos);
        property_length = get_property_length(curr_pos);
        if (property_length < 0) {
            return BT_STATUS_FAIL;
        }

        switch (property) {
        case VCARD_BEGIN:
            break;
        case VCARD_END:
            break;
        case VCARD_VERSION:
            break;
        case VCARD_NAME:
            break;
        case VCARD_FORMATTED_NAME:
            pce_parse_property(curr_pos, curr_pos + property_length, contact->name, BT_PBAP_PCE_PROPERTY_MAX_LEN);
            break;
        case VCARD_TEL:
            pce_parse_property(curr_pos, curr_pos + property_length, contact->numbers[0], BT_PBAP_PCE_PROPERTY_MAX_LEN);
            break;
        default:
            break;
        }

        curr_pos += property_length + 1;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t pce_parse_card_v3_0(uint16_t data_len, char* data, bt_pce_contact_t* contact)
{
    /* TODO : PCE only needs to support one vCard version, and we prefer to support only 2.1*/
    return BT_STATUS_NOT_SUPPORTED;
}