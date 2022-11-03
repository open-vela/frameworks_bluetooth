/****************************************************************************
 *
 *   Copyright (C) 2021 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "btm_manager.h"
#include "utils.h"

static char g_bdaddr_str[18];

int bachk(const char* str)
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

int ba2str(bt_address addr, char* str)
{
    return sprintf(str, "%2.2X:%2.2X:%2.2X:%2.2X:%2.2X:%2.2X",
        addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

int str2ba(const char* str, bt_address addr)
{
    int i;

    if (bachk(str) < 0) {
        memset(addr, 0, sizeof(bt_address));
        return -1;
    }

    for (i = 5; i >= 0; i--, str += 3)
        addr[i] = strtol(str, NULL, 16);

    return 0;
}

char* addr_str(bt_address addr)
{
    ba2str(addr, g_bdaddr_str);
    g_bdaddr_str[17] = '\0';

    return g_bdaddr_str;
}

bool addr_is_empty(bt_address addr)
{
    for (int i = 0; i< sizeof(bt_address); i++) {
        if (addr[i] != 0) {
            return false;
        }
    }
    return true;
}

void clear_addr(bt_address addr)
{
    memset(addr, 0, sizeof(bt_address));
}

bool same_addr(bt_address addr1, bt_address addr2)
{
    return (memcmp(addr1, addr2, sizeof(bt_address)) == 0);
}

int format_addr(char* addr_str, bt_address addr)
{
    return sscanf(addr_str, "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]);
}

void addr_cpy(bt_address addr_dst, bt_address addr_src)
{
    memcpy(addr_dst, addr_src, sizeof(bt_address));
}

char * uuid_str(bt_uuid_t uuid)
{
    static char uuid_s[36];
    memset(uuid_s, 0, 36);
    for(int i= 0; i < UUID_SIZE; i++) {
        snprintf(uuid_s + i * 2 , 36, "%02X", uuid[i]);
    }
    return uuid_s;
}

bool uuid_is_empty(bt_uuid_t uuid)
{
    for(int i= 0; i < UUID_SIZE; i++) {
        if (uuid[i] != 0) {
            return false;
        }
    }
    return true;
}

uint64_t get_os_timestamp_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);

    return (uint64_t)(((uint64_t)ts.tv_sec * 1000000L) + ((uint64_t)ts.tv_nsec / 1000));
}
