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

#include "sal_zblue.h"
#include "sal_interface.h"
#include "sal_zephyr_interface.h"
#include "utils/log.h"

static bt_conn_info_t g_conn_info[CONFIG_BT_MAX_CONN];

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

    if (info.type == BT_CONN_TYPE_LE) {
        return get_le_addr_from_conn(conn, addr);
    }

    bt_addr_set(addr, info.br.dst->val);
    return BT_STATUS_SUCCESS;
}

bt_conn_info_t* bt_conn_find(const bt_address_t* addr, uint8_t transport)
{
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if ((!bt_addr_compare(&g_conn_info[i].addr, addr))
            && (transport == g_conn_info[i].transport)) {
            return &g_conn_info[i];
        }
    }

    return NULL;
}

bt_address_t* bt_conn_get_addr(struct bt_conn* conn)
{
    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (g_conn_info[i].conn == conn) {
            return &g_conn_info[i].addr;
        }
    }

    return NULL;
}

bt_conn_info_t* bt_conn_add(const bt_address_t* addr, uint8_t transport)
{
    bt_conn_info_t* info = bt_conn_find(addr, transport);
    if (info) {
        return info;
    }

    for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
        if (!g_conn_info[i].conn && bt_addr_is_empty(&g_conn_info[i].addr)) {
            memcpy(g_conn_info[i].addr.addr, addr->addr, BT_ADDR_LENGTH);
            g_conn_info[i].transport = transport;
            return &g_conn_info[i];
        }
    }

    BT_LOGE("%s, no free entry", __func__);
    return NULL;
}

bt_status_t bt_conn_remove(bt_address_t* addr, uint8_t transport)
{
    bt_conn_info_t* info = bt_conn_find(addr, transport);
    if (info) {
        memset(info, 0, sizeof(*info));
        return BT_STATUS_SUCCESS;
    }

    BT_LOGD("%s, addr not found", __func__);
    return BT_STATUS_FAIL;
}

bt_status_t bt_conn_set_role(bt_transport_t transport, bt_address_t* addr, uint8_t flag)
{
    bt_conn_info_t* info;

    if (bt_addr_is_empty(addr) || !flag) {
        BT_LOGE("%s, invalid addr or flag", __func__);
        return BT_STATUS_FAIL;
    }

    info = bt_conn_find(addr, transport);
    if (info) {
        if (info->role & flag) {
            BT_LOGD("conn flag already set, skip");
            return BT_STATUS_DONE;
        }

        info->role |= flag;

        if (info->conn) {
            if ((info->role & GATT_ROLE_CLIENT) && flag == GATT_ROLE_SERVER) {
#ifdef CONFIG_BLUETOOTH_GATT_SERVER
                bt_sal_gatt_server_connection_state_changed_callback(PRIMARY_ADAPTER, &info->addr, PROFILE_STATE_CONNECTED);
#endif
            } else if ((info->role & GATT_ROLE_SERVER) && flag == GATT_ROLE_CLIENT) {
#ifdef CONFIG_BLUETOOTH_GATT_CLIENT
                bt_sal_gatt_client_connection_state_changed_callback(PRIMARY_ADAPTER, &info->addr, PROFILE_STATE_CONNECTED);
#endif
            }

            return BT_STATUS_DONE;
        }

        return BT_STATUS_SUCCESS;
    }

    info = bt_conn_add(addr, transport);
    if (info) {
        info->role = flag;
        return BT_STATUS_SUCCESS;
    }

    return BT_STATUS_FAIL;
}
