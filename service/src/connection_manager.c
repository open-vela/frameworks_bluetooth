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
#define LOG_TAG "connection_manager"

#include "connection_manager.h"
#include "adapter_internel.h"
#include "bluetooth.h"
#include "hci_error.h"
#include "service_loop.h"
#include "service_manager.h"

#ifdef CONFIG_BLUETOOTH_HFP_HF
#include "hfp_hf_service.h"
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
#include "a2dp_sink_service.h"
#endif

#ifdef CONFIG_LE_DLF_SUPPORT
#include "connection_manager_dlf.h"
#endif

#include "utils/log.h"

#define CM_RECONNECT_INTERVAL (12000) /* reconnect Interval */
#define PROFILE_CONNECT_INTERVAL (500) /* Interval between HFP and A2DP */
#define CM_RECONNECT_TIMES ((60 * 30) / 8) /* Continuous 30-mins reconnect */

#define FLAG_NONE (0)
#define FLAG_HFP_HF (1 << (PROFILE_HFP_HF))
#define FLAG_A2DP_SINK (1 << (PROFILE_A2DP_SINK))

typedef struct {
    service_timer_t* timer;
    bt_address_t peer_addr;
    bool active;
    uint32_t retry_times;
} bt_cm_timer_t;

typedef struct {
    bool inited;
    bool busy;
    bool reconnect_enable;
    bool connect_a2dp_flag;
    bt_address_t connecting_addr;
    uint32_t profile_flags;
    bt_cm_timer_t cm_timer;
    service_timer_t* a2dp_conn_timer;
} bt_connection_manager_t;

static bt_connection_manager_t g_connection_manager;

static bt_status_t bt_cm_profile_connect(bt_address_t* addr, uint8_t transport);

static void bt_cm_set_flags(uint32_t flags)
{
    bt_connection_manager_t* manager = &g_connection_manager;

    manager->profile_flags |= flags;
}

static void bt_cm_clear_flags(uint32_t flags)
{
    bt_connection_manager_t* manager = &g_connection_manager;

    manager->profile_flags &= ~flags;
}

static void bt_cm_stop_timer(void)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_cm_timer_t* cm_timer = &manager->cm_timer;

    if (!cm_timer->active)
        return;

    cm_timer->active = false;
    cm_timer->retry_times = 0;
    bt_addr_set_empty(&cm_timer->peer_addr);
    service_loop_cancel_timer(cm_timer->timer);
    cm_timer->timer = NULL;
}

static void bt_cm_enable_conn(void)
{
    bt_connection_manager_t* manager = &g_connection_manager;

    bt_cm_clear_flags(FLAG_HFP_HF | FLAG_A2DP_SINK);
    bt_cm_stop_timer();
    manager->connect_a2dp_flag = false;
    manager->busy = false;
}

static bool bt_cm_is_busy(void)
{
    bt_connection_manager_t* manager = &g_connection_manager;

    return manager->busy;
}

static void bt_cm_disable_conn()
{
    bt_connection_manager_t* manager = &g_connection_manager;

    manager->busy = true;
    manager->profile_flags = 0;
}

#ifdef CONFIG_BLUETOOTH_HFP_HF
static bt_status_t bt_cm_hfp_connect(bt_address_t* addr)
{
    hfp_hf_interface_t* hfp_hf_profile;

    hfp_hf_profile = (hfp_hf_interface_t*)service_manager_get_profile(PROFILE_HFP_HF);
    return hfp_hf_profile->connect(addr);
}

static bt_status_t bt_cm_hfp_disconnect(bt_address_t* addr)
{
    hfp_hf_interface_t* hfp_hf_profile;

    hfp_hf_profile = (hfp_hf_interface_t*)service_manager_get_profile(PROFILE_HFP_HF);
    return hfp_hf_profile->disconnect(addr);
}
#endif

#ifdef CONFIG_BLUETOOTH_A2DP_SINK
static bt_status_t bt_cm_a2dpsnk_connect(bt_address_t* addr)
{
    a2dp_sink_interface_t* a2dp_snk_profile;

    a2dp_snk_profile = (a2dp_sink_interface_t*)service_manager_get_profile(PROFILE_A2DP_SINK);
    return a2dp_snk_profile->connect(addr);
}

static bt_status_t bt_cm_a2dpsnk_disconnect(bt_address_t* addr)
{
    a2dp_sink_interface_t* a2dp_snk_profile;

    a2dp_snk_profile = (a2dp_sink_interface_t*)service_manager_get_profile(PROFILE_A2DP_SINK);
    return a2dp_snk_profile->disconnect(addr);
}
#endif

static void bt_cm_connect_a2dp_cb(service_timer_t* timer, void* userdata)
{
    bt_connection_manager_t* manager = (bt_connection_manager_t*)userdata;

    bt_cm_profile_connect(&manager->connecting_addr, BT_TRANSPORT_BREDR);
}

static bt_status_t bt_cm_profile_connect(bt_address_t* addr, uint8_t transport)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_status_t status = BT_STATUS_SUCCESS;

    if (transport == BT_TRANSPORT_BLE) {
        BT_LOGD("%s To be realized", __func__);
        return BT_STATUS_FAIL;
    }

    if ((manager->profile_flags & FLAG_HFP_HF) && !manager->connect_a2dp_flag) { /* connect HFP_HF */
#ifdef CONFIG_BLUETOOTH_HFP_HF
        status = bt_cm_hfp_connect(addr);

        if (status != BT_STATUS_SUCCESS) {
            BT_LOGE("%s connect HFP_HF failed", __func__);
            return status;
        }
#endif
        if ((manager->profile_flags & FLAG_A2DP_SINK) == 0)
            return BT_STATUS_SUCCESS;

        /* delay 500ms to ensure HFP connection is established first*/
        manager->connect_a2dp_flag = true;
        memcpy(&manager->connecting_addr, addr, sizeof(bt_address_t));
        if (manager->a2dp_conn_timer) {
            service_loop_cancel_timer(manager->a2dp_conn_timer);
            manager->a2dp_conn_timer = NULL;
        }

        manager->a2dp_conn_timer = service_loop_timer_no_repeating(PROFILE_CONNECT_INTERVAL, bt_cm_connect_a2dp_cb, manager);
    } else if (manager->profile_flags & FLAG_A2DP_SINK) { /* connect A2DP_SINK */
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
        manager->connect_a2dp_flag = false;
        status = bt_cm_a2dpsnk_connect(addr);
#endif
    }

    return status;
}

static bt_status_t bt_cm_profile_disconnect(bt_address_t* addr, uint8_t transport)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_status_t status = BT_STATUS_SUCCESS;

    if (transport == BT_TRANSPORT_BLE) {
        BT_LOGD("%s To be realized", __func__);
        return BT_STATUS_FAIL;
    }

#ifdef CONFIG_BLUETOOTH_HFP_HF
    status = bt_cm_hfp_disconnect(addr);

    if (status != BT_STATUS_SUCCESS)
        return status;
#endif

    if (manager->a2dp_conn_timer) {
        manager->connect_a2dp_flag = false;
        service_loop_cancel_timer(manager->a2dp_conn_timer);
        bt_addr_set_empty(&manager->connecting_addr);
        manager->a2dp_conn_timer = NULL;
    }
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    status = bt_cm_a2dpsnk_disconnect(addr);
#endif

    return status;
}

void bt_cm_init(void)
{
    bt_connection_manager_t* manager = &g_connection_manager;

    if (manager->inited)
        return;

    bt_cm_enable_conn();
    bt_cm_set_flags(FLAG_NONE);
    manager->reconnect_enable = false;
    manager->inited = true;
}

void bt_cm_cleanup(void)
{
    bt_connection_manager_t* manager = &g_connection_manager;

    if (!manager->inited)
        return;

#ifdef CONFIG_LE_DLF_SUPPORT
    bt_cm_dlf_cleanup();
#endif

    manager->inited = false;
}

static bool bt_cm_allocator(void** data, uint32_t size)
{
    *data = malloc(size);
    if (!(*data))
        return false;

    return true;
}

static void bt_cm_profile_falgs_set(void)
{
#if 0
    bt_uuid_t* uuids = NULL;
    uint16_t uuid_cnt = 0;
    bt_uuid_t uuid16_hfp_ag;
    bt_uuid_t uuid16_a2dp;

    bt_uuid16_create(&uuid16_hfp_ag, 0x111F); /* HFP AG UUID */
    bt_uuid16_create(&uuid16_a2dp, 0x110A); /* A2DP UUID */
    adapter_get_remote_uuids(addr, &uuids, &uuid_cnt, bt_cm_allocator);
    if (uuid_cnt) {
        for (int i = 0; i < uuid_cnt; i++) {
            if (!bt_uuid_compare(&uuids[i], &uuid16_hfp_ag)) {
#ifdef CONFIG_BLUETOOTH_HFP_HF
                bt_cm_set_flags(FLAG_HFP_HF);
#endif
            } else if (!bt_uuid_compare(&uuids[i], &uuid16_a2dp)) {
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
                bt_cm_set_flags(FLAG_A2DP_SINK);
#endif
            }
        }
    }

    free(uuids);
    uuids = NULL;
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    bt_cm_set_flags(FLAG_HFP_HF);
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    bt_cm_set_flags(FLAG_A2DP_SINK);
#endif
}

bt_status_t bt_cm_device_connect(bt_address_t* peer_addr, uint8_t transport)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_address_t* addrs = NULL;
    bt_address_t addr;
    int num = 0;

    if (transport == BT_TRANSPORT_BLE) {
        BT_LOGD("%s To be realized", __func__);
        return BT_STATUS_FAIL;
    }

    if (!manager->reconnect_enable)
        manager->reconnect_enable = true;

    if (bt_cm_is_busy()) {
        BT_LOGE("%s processing reconnecting", __func__);
        return BT_STATUS_BUSY;
    }

    if (bt_addr_is_empty(peer_addr)) {
        adapter_get_bonded_devices(transport, &addrs, &num, bt_cm_allocator);
        if (!num) {
            BT_LOGE("%s no device to connect", __func__);
            return BT_STATUS_FAIL;
        }

        memcpy(&addr, &addrs[num - 1], sizeof(bt_address_t)); /* connect last device in bonded list */
        free(addrs);
        BT_LOGD("%s connect to last device", __func__);
    } else {
        memcpy(&addr, peer_addr, sizeof(bt_address_t));
    }

    bt_cm_profile_falgs_set();
    return bt_cm_profile_connect(&addr, transport);
}

bt_status_t bt_cm_device_disconnect(bt_address_t* addr, uint8_t transport)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_cm_timer_t* cm_timer = &manager->cm_timer;

    if (transport == BT_TRANSPORT_BLE) {
        BT_LOGD("%s To be realized", __func__);
        return BT_STATUS_FAIL;
    }

    if (!bt_addr_compare(&cm_timer->peer_addr, addr)) {
        bt_cm_enable_conn();
    }

    return bt_cm_profile_disconnect(addr, BT_TRANSPORT_BREDR);
}

void bt_cm_connected(bt_address_t* addr, uint8_t profile_id)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_cm_timer_t* cm_timer = &manager->cm_timer;

    if (bt_addr_compare(&cm_timer->peer_addr, addr)) {
        return;
    }

    BT_LOGD("%s connect success, profile_id: %d", __func__, profile_id);
    switch (profile_id) {
    case PROFILE_HFP_HF: {
        bt_cm_clear_flags(FLAG_HFP_HF);
        break;
    }
    case PROFILE_A2DP_SINK: {
        bt_cm_clear_flags(FLAG_A2DP_SINK);
        service_loop_cancel_timer(manager->a2dp_conn_timer);
        bt_addr_set_empty(&manager->connecting_addr);
        manager->a2dp_conn_timer = NULL;
        break;
    }
    default:
        break;
    }

    if ((manager->profile_flags & (FLAG_HFP_HF | FLAG_A2DP_SINK)) == 0) {
        BT_LOGD("%s no profile to connect", __func__);
        bt_cm_enable_conn();
    }
}

void bt_cm_disconnected(bt_address_t* addr, uint8_t profile_id)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_cm_timer_t* cm_timer = &manager->cm_timer;

    if (bt_addr_compare(&cm_timer->peer_addr, addr)) {
        return;
    }

    BT_LOGD("%s connect failed, profile_id: %d", __func__, profile_id);
    switch (profile_id) {
    case PROFILE_A2DP_SINK: {
        service_loop_cancel_timer(manager->a2dp_conn_timer);
        bt_addr_set_empty(&manager->connecting_addr);
        manager->a2dp_conn_timer = NULL;
        break;
    }
    default:
        break;
    }
}

static void bt_cm_timeout_cb(service_timer_t* timer, void* userdata)
{
    bt_cm_timer_t* cm_timer = (bt_cm_timer_t*)userdata;

    if (!cm_timer->active) {
        BT_LOGE("%s timer is not actice", __func__);
        return;
    }

    cm_timer->retry_times++;
    if (cm_timer->retry_times > CM_RECONNECT_TIMES) {
        bt_cm_enable_conn();
    }

    BT_LOGD("%s reconnect start, retry_times = %" PRIu32, __func__, cm_timer->retry_times);
    bt_cm_profile_connect(&cm_timer->peer_addr, BT_TRANSPORT_BREDR);
}

static bool bt_cm_start_timer(bt_address_t* peer_addr)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_cm_timer_t* cm_timer = &manager->cm_timer;

    if (cm_timer->active) {
        BT_LOGW("%s timer is active", __func__);
        return false;
    }

    BT_LOGD("%s CM connect start", __func__);
    cm_timer->active = true;
    memcpy(&cm_timer->peer_addr, peer_addr, sizeof(bt_address_t));
    if (cm_timer->timer) {
        service_loop_cancel_timer(cm_timer->timer);
    }

    cm_timer->timer = service_loop_timer(CM_RECONNECT_INTERVAL, CM_RECONNECT_INTERVAL, bt_cm_timeout_cb, cm_timer);

    return true;
}

static void bt_cm_process_reconnection(bt_address_t* addr, uint32_t hci_reason_code)
{
    bt_connection_manager_t* manager = &g_connection_manager;
    bt_cm_timer_t* cm_timer = &manager->cm_timer;

    if (hci_reason_code == HCI_ERR_CONNECTION_TERMINATED_BY_LOCAL_HOST
        || hci_reason_code == HCI_ERR_REMOTE_USER_TERMINATED_CONNECTION) {
        if (!bt_addr_compare(&cm_timer->peer_addr, addr)) {
            bt_cm_enable_conn();
        }

        return;
    }

    if (bt_cm_is_busy() || (hci_reason_code != HCI_ERR_CONNECTION_TIMEOUT))
        return;

    BT_LOGD("%s reconnect start", __func__);
    bt_cm_disable_conn();
    bt_cm_profile_falgs_set();
    if (bt_cm_start_timer(addr))
        bt_cm_profile_connect(addr, BT_TRANSPORT_BREDR);
}

void bt_cm_process_disconnect_event(bt_address_t* addr, uint8_t transport, uint32_t hci_reason_code)
{
    bt_connection_manager_t* manager = &g_connection_manager;

    if (transport == BT_TRANSPORT_BLE) {
#ifdef CONFIG_LE_DLF_SUPPORT
        bt_cm_disable_dlf(addr);
#endif
        return;
    }

    if (manager->reconnect_enable)
        bt_cm_process_reconnection(addr, hci_reason_code);
}

bt_status_t bt_cm_enable_enhanced_mode(bt_address_t* addr, uint8_t mode)
{
    switch (mode) {
    case EM_LE_LOW_LATENCY: {
#ifdef CONFIG_LE_DLF_SUPPORT
        return bt_cm_enable_dlf(addr);
#endif
    }
    default:
        return BT_STATUS_NOT_SUPPORTED;
    }
}

bt_status_t bt_cm_disable_enhanced_mode(bt_address_t* addr, uint8_t mode)
{
    switch (mode) {
    case EM_LE_LOW_LATENCY: {
#ifdef CONFIG_LE_DLF_SUPPORT
        return bt_cm_disable_dlf(addr);
#endif
    }
    default:
        return BT_STATUS_NOT_SUPPORTED;
    }
}