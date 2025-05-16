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
#include "sal_connection_manager.h"
#include "bt_list.h"
#include "sal_interface.h"
#include "service_loop.h"

#include <zephyr/bluetooth/conn.h>

#include "utils/log.h"

#define FLAG_NONE (0)
#define FLAG_A2DP_SINK (1UL << (PROFILE_A2DP_SINK))
#define FLAG_A2DP_SOURCE (1UL << (PROFILE_A2DP))
#define FLAG_AVRCP_TARGET (1UL << (PROFILE_AVRCP_TG))
#define FLAG_AVRCP_CONTROL (1UL << (PROFILE_AVRCP_CT))

typedef struct {
    bt_address_t device_addr;
    uint32_t profile_flags;
    bool is_unpair;
} bt_profile_connection_manager_t;

static bt_list_t* bt_sal_disconnecting_list = NULL;

static void flags_set(uint32_t* profile_flags, uint32_t flags)
{
    *profile_flags |= flags;
}

static void flags_clear(uint32_t* profile_flags, uint32_t flags)
{
    *profile_flags &= ~flags;
}

static void flags_reset(uint32_t* profile_flags)
{
    *profile_flags = FLAG_NONE;
}

static void bt_connection_manager_destory(void* data)
{
    free(data);
}

static bool bt_cm_disconnect_find(void* data, void* context)
{
    bt_profile_connection_manager_t* manager = (bt_profile_connection_manager_t*)data;
    if (!manager)
        return false;

    return memcmp(&manager->device_addr, context, sizeof(bt_address_t)) == 0;
}

cm_data_t* cm_data_new(bt_address_t* addr, uint8_t profile_id)
{
    cm_data_t* data = (cm_data_t*)zalloc(sizeof(cm_data_t));
    if (!data)
        return NULL;

    if (addr != NULL)
        memcpy(&data->addr, addr, sizeof(bt_address_t));

    data->profile_id = profile_id;
    return data;
}

void bt_sal_cm_conn_init(void)
{
    bt_sal_disconnecting_list = bt_list_new(bt_connection_manager_destory);
}

void cm_data_destory(cm_data_t* data)
{
    free(data);
}

static bt_status_t bt_try_disconnect_acl(bt_profile_connection_manager_t* manager)
{
    struct bt_conn* conn;
    int ret;

    if (manager->profile_flags != FLAG_NONE) {
        BT_LOGI("%s, Disconnecting profile.", __func__);
        return BT_STATUS_BUSY;
    }

    if (manager->is_unpair) {
        return bt_br_unpair((bt_addr_t*)&manager->device_addr);
    }

    conn = bt_conn_lookup_addr_br((bt_addr_t*)&manager->device_addr);
    if (conn == NULL) {
        BT_LOGE("%s, conn not found.", __func__);
        return BT_STATUS_FAIL;
    }

    ret = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    bt_conn_unref(conn);

    if (ret) {
        BT_LOGE("%s, bt_conn_disconnect failed.", __func__);
        return BT_STATUS_FAIL;
    }
    return BT_STATUS_SUCCESS;
}

static void bt_sal_cm_profile_disconnected(void* data)
{
    if (data == NULL)
        return;

    cm_data_t* cm_data = data;
    bt_profile_connection_manager_t* manager;

    if (bt_sal_disconnecting_list == NULL)
        return;

    manager = bt_list_find(bt_sal_disconnecting_list, bt_cm_disconnect_find, &cm_data->addr);
    if (manager == NULL) {
        BT_LOGW("%s, manager not found.", __func__);
        return;
    }

    switch (cm_data->profile_id) {
    case PROFILE_A2DP_SINK: {
        flags_clear(&manager->profile_flags, FLAG_A2DP_SINK);
        break;
    }
    case PROFILE_A2DP: {
        flags_clear(&manager->profile_flags, FLAG_A2DP_SOURCE);
        break;
    }
    case PROFILE_AVRCP_TG: {
        flags_clear(&manager->profile_flags, FLAG_AVRCP_TARGET);
        break;
    }
    case PROFILE_AVRCP_CT: {
        flags_clear(&manager->profile_flags, FLAG_AVRCP_CONTROL);
        break;
    }
    default:
        break;
    }

    bt_try_disconnect_acl(manager);

    cm_data_destory(cm_data);
}

static void bt_sal_cm_acl_disconnected(void* data)
{
    if (data == NULL)
        return;

    cm_data_t* cm_data = data;
    bt_profile_connection_manager_t* manager;

    if (bt_sal_disconnecting_list == NULL)
        return;

    manager = bt_list_find(bt_sal_disconnecting_list, bt_cm_disconnect_find, &cm_data->addr);
    if (manager == NULL) {
        BT_LOGW("%s, manager not found.", __func__);
        return;
    }

    bt_list_remove(bt_sal_disconnecting_list, manager);

    cm_data_destory(cm_data);
}

void bt_sal_cm_profile_disconnected_callback(cm_data_t* data)
{
    if (data == NULL)
        return;

    do_in_service_loop(bt_sal_cm_profile_disconnected, data);
}

void bt_sal_cm_acl_disconnected_callback(cm_data_t* data)
{
    if (data == NULL)
        return;

    do_in_service_loop(bt_sal_cm_acl_disconnected, data);
}

bt_status_t bt_sal_cm_try_disconnect_profiles(bt_address_t* addr, bool is_unpair)
{
    bt_profile_connection_manager_t* manager;
    uint32_t flag = FLAG_NONE;

    if (bt_sal_disconnecting_list == NULL)
        return BT_STATUS_FAIL;

    manager = bt_list_find(bt_sal_disconnecting_list, bt_cm_disconnect_find, addr);
    if (manager) {
        BT_LOGW("%s, Disconnecting.", __func__);
        return BT_STATUS_BUSY;
    }

    manager = (bt_profile_connection_manager_t*)zalloc(sizeof(bt_profile_connection_manager_t));
    if (!manager) {
        BT_LOGE("%s, malloc failed", __func__);
        return BT_STATUS_NOMEM;
    }

    memcpy(&manager->device_addr, addr, sizeof(bt_address_t));
    flags_reset(&manager->profile_flags);
    manager->is_unpair = is_unpair;
    bt_list_add_tail(bt_sal_disconnecting_list, manager);

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (bt_sal_a2dp_try_disconnect_a2dp_sink(PRIMARY_ADAPTER, addr))
        flag |= FLAG_A2DP_SINK;
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SOURCE
    if (bt_sal_a2dp_try_disconnect_a2dp_srouce(PRIMARY_ADAPTER, addr))
        flag |= FLAG_A2DP_SOURCE;
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CONTROL
    if (bt_sal_avrcp_try_disconnect_avrcp_control(PRIMARY_ADAPTER, addr))
        flag |= FLAG_AVRCP_CONTROL;
#endif

    flags_set(&manager->profile_flags, flag);

    return bt_try_disconnect_acl(manager);
}

void bt_sal_cm_conn_cleanup(void)
{
    bt_list_free(bt_sal_disconnecting_list);
    bt_sal_disconnecting_list = NULL;
}