/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"

static const bt_address_t bt_addr_empty = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static char g_bdaddr_str[18];

static int bachk(const char* str)
{
    if (!str)
        return -1;

    if (strlen(str) != 17)
        return -1;

    while (*str) {
        if (!isxdigit(*str++))
            return -1;

        if (!isxdigit(*str++))
            return -1;

        if (*str == 0)
            break;

        if (*str++ != ':')
            return -1;
    }

    return 0;
}

bool bt_addr_is_empty(const bt_address_t* addr)
{
    return memcmp(addr->addr, bt_addr_empty.addr, BT_ADDR_LENGTH) == 0;
}

void bt_addr_set_empty(bt_address_t* addr)
{
    assert(addr != NULL);

    memcpy(addr, &bt_addr_empty, sizeof(bt_address_t));
}

int bt_addr_compare(const bt_address_t* a, const bt_address_t* b)
{
    return memcmp(a, b, sizeof(bt_address_t));
}

int bt_addr_ba2str(const bt_address_t* addr, char* str)
{
    return sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
        addr->addr[5], addr->addr[4], addr->addr[3],
        addr->addr[2], addr->addr[1], addr->addr[0]);
}

char* bt_addr_bastr(const bt_address_t* addr)
{
    if (!addr) {
        return NULL;
    }

    bt_addr_ba2str(addr, g_bdaddr_str);
    g_bdaddr_str[17] = '\0';

    return g_bdaddr_str;
}

int bt_addr_str2ba(const char* str, bt_address_t* addr)
{
    int i;

    if (bachk(str) < 0) {
        memset(addr, 0, sizeof(bt_address_t));
        return -1;
    }

    for (i = 5; i >= 0; i--, str += 3)
        addr->addr[i] = strtol(str, NULL, 16);

    return 0;
}

void bt_addr_set(bt_address_t* addr, const uint8_t* bd)
{
    memcpy(addr->addr, bd, 6);
}

void bt_addr_swap(const bt_address_t* src, bt_address_t* dest)
{
    for (int i = 0; i < 6; i++)
        dest->addr[5 - i] = src->addr[i];
}
