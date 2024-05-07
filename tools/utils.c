/****************************************************************************
 *  Copyright (C) 2023 Xiaomi Corporation
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

#include "utils.h"
#include "bt_tools.h"

bool phy_is_vaild(uint8_t phy)
{
    if (phy != BT_LE_1M_PHY && phy != BT_LE_2M_PHY && phy != BT_LE_CODED_PHY)
        return false;

    return true;
}

int le_addr_type(const char* str, ble_addr_type_t* type)
{
    *type = BT_LE_ADDR_TYPE_UNKNOWN;

    if (!strncasecmp(str, "public_id", strlen("public_id")))
        *type = BT_LE_ADDR_TYPE_PUBLIC_ID;
    else if (!strncasecmp(str, "random_id", strlen("random_id")))
        *type = BT_LE_ADDR_TYPE_RANDOM_ID;
    else if (!strncasecmp(str, "public", strlen("public")))
        *type = BT_LE_ADDR_TYPE_PUBLIC;
    else if (!strncasecmp(str, "random", strlen("random")))
        *type = BT_LE_ADDR_TYPE_RANDOM;
    else if (!strncasecmp(str, "anonymous", strlen("anonymous")))
        *type = BT_LE_ADDR_TYPE_ANONYMOUS;
    else
        return CMD_INVALID_PARAM;

    return CMD_OK;
}

bool bttool_allocator(void** data, uint32_t size)
{
    *data = malloc(size);
    if (!(*data))
        return false;

    return true;
}

uint32_t get_timestamp_msec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_BOOTTIME, &ts);

    return (uint32_t)((ts.tv_sec * 1000L) + (ts.tv_nsec / 1000000));
}