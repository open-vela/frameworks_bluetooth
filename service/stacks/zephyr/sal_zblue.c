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
#define LOG_TAG "sal_zblue"

#include "sal_interface.h"

#undef BT_LE_SCAN_TYPE_PASSIVE
#undef BT_LE_SCAN_TYPE_ACTIVE

#include "sal_zblue.h"
#include "utils/log.h"

void bt_sal_get_stack_info(bt_stack_info_t* info)
{
    snprintf(info->name, 32, "Zblue");
    info->stack_ver_major = 5;
    info->stack_ver_minor = 4;
    info->sal_ver = 2;
}

bt_status_t bt_sal_get_remote_address(struct bt_conn* conn, bt_address_t* addr)
{
    struct bt_conn_info info;

    if (conn == NULL)
        return BT_STATUS_FAIL;

    if (bt_conn_get_info(conn, &info) != 0) {
        BT_LOGE("%s, failed to get address", __func__);
        return BT_STATUS_FAIL;
    }

    bt_addr_set(addr, info.br.dst->val);
    return BT_STATUS_SUCCESS;
}
