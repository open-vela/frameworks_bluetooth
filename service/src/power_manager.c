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
#define LOG_TAG "pm_mgr"

#include <stdlib.h>
#include <string.h>

#include "adapter_internel.h"
#include "bt_list.h"
#include "bt_profile.h"
#include "power_manager.h"
#include "sal_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#ifndef BT_PM_SNIFF_MAX
#define BT_PM_SNIFF_MAX 800
#define BT_PM_SNIFF_MIN 400
#define BT_PM_SNIFF_ATTEMPT 4
#define BT_PM_SNIFF_TIMEOUT 1
#endif

#ifndef BT_PM_SNIFF1_MAX
#define BT_PM_SNIFF1_MAX 400
#define BT_PM_SNIFF1_MIN 200
#define BT_PM_SNIFF1_ATTEMPT 4
#define BT_PM_SNIFF1_TIMEOUT 1
#endif

#ifndef BT_PM_SNIFF2_MAX
#define BT_PM_SNIFF2_MAX 54
#define BT_PM_SNIFF2_MIN 30
#define BT_PM_SNIFF2_ATTEMPT 4
#define BT_PM_SNIFF2_TIMEOUT 1
#endif

#ifndef BT_PM_SNIFF3_MAX
#define BT_PM_SNIFF3_MAX 150
#define BT_PM_SNIFF3_MIN 50
#define BT_PM_SNIFF3_ATTEMPT 4
#define BT_PM_SNIFF3_TIMEOUT 1
#endif

#ifndef BT_PM_SNIFF4_MAX
#define BT_PM_SNIFF4_MAX 18
#define BT_PM_SNIFF4_MIN 10
#define BT_PM_SNIFF4_ATTEMPT 4
#define BT_PM_SNIFF4_TIMEOUT 1
#endif

#ifndef BT_PM_SNIFF5_MAX
#define BT_PM_SNIFF5_MAX 36
#define BT_PM_SNIFF5_MIN 30
#define BT_PM_SNIFF5_ATTEMPT 2
#define BT_PM_SNIFF5_TIMEOUT 0
#endif

#ifndef BT_PM_SNIFF6_MAX
#define BT_PM_SNIFF6_MAX 18
#define BT_PM_SNIFF6_MIN 14
#define BT_PM_SNIFF6_ATTEMPT 1
#define BT_PM_SNIFF6_TIMEOUT 0
#endif

#define BT_PM_PREF_MODE_SNIFF 0x10
#define BT_PM_PREF_MODE_ACTIVE 0x20
#define BT_PM_PREF_MODE_MASK 0x0f

typedef enum {
    BT_PM_RESTART,
    BT_PM_NEW_REQ,
    BT_PM_EXECUTE,
} bt_pm_request_t;

typedef enum {
    BT_PM_STATE_CONN_OPEN,
    BT_PM_STATE_CONN_CLOSE,
    BT_PM_STATE_APP_OPEN,
    BT_PM_STATE_APP_CLOSE,
    BT_PM_STATE_SCO_OPEN,
    BT_PM_STATE_SCO_CLOSE,
    BT_PM_STATE_CONN_IDLE,
    BT_PM_STATE_CONN_BUSY,
    BT_PM_STATE_MAX,
} bt_pm_state_t;

typedef enum {
    BT_PM_NO_ACTION, /* no change to the current pm setting */
    BT_PM_NO_PREF, /* service has no preference on power mode setting. eg. connection to service got closed */
    BT_PM_SNIFF = BT_PM_PREF_MODE_SNIFF, /* prefers sniff mode */
    BT_PM_SNIFF1, /* prefers sniff1 mode */
    BT_PM_SNIFF2, /* prefers sniff2 mode */
    BT_PM_SNIFF3, /* prefers sniff3 mode */
    BT_PM_SNIFF4, /* prefers sniff4 mode */
    BT_PM_SNIFF5, /* prefers sniff5 mode */
    BT_PM_SNIFF6, /* prefers sniff6 mode */
    BT_PM_ACTIVE = BT_PM_PREF_MODE_ACTIVE, /* prefers active mode */
} bt_pm_prefer_mode_t;

typedef enum {
    BT_PM_MODE_INDEX_0,
    BT_PM_MODE_INDEX_1,
    BT_PM_MODE_INDEX_2,
    BT_PM_MODE_INDEX_3,
    BT_PM_MODE_INDEX_4,
    BT_PM_MODE_INDEX_5,
    BT_PM_MODE_INDEX_6,
    BT_PM_MODE_INDEX_MAX = BT_PM_MODE_INDEX_6,
} bt_pm_mode_index_t;

typedef enum {
    BT_PM_SPEC_INDEX_0,
    BT_PM_SPEC_INDEX_1,
    BT_PM_SPEC_INDEX_2,
    BT_PM_SPEC_INDEX_3,
    BT_PM_SPEC_INDEX_4,
    BT_PM_SPEC_INDEX_MAX = BT_PM_SPEC_INDEX_4,
} bt_pm_spec_index_t;

typedef struct {
    bt_pm_prefer_mode_t power_mode;
    uint16_t timeout;
} bt_pm_action_t;

typedef struct {
    uint8_t profile_id;
    uint8_t spec_idx; /* index of spec table to use */
} bt_pm_config_t;

typedef struct {
    uint8_t allow_mask; /* mask of sniff/hold/park modes to allow */
    uint8_t ssr; /* set SSR on conn open/unpark */

    bt_pm_action_t actn_tbl[BT_PM_STATE_MAX];
} bt_pm_spec_table_t;

typedef struct {
    struct list_node srv_node;

    uint8_t profile_id;
    bt_pm_state_t state;
    bt_address_t peer_addr;
} bt_pm_service_t;

typedef struct {
    service_timer_t* pm_timer;
    bt_address_t peer_addr;
    bool active;
    bt_pm_prefer_mode_t pm_action;
    uint16_t profile_id;
} bt_pm_timer_t;

typedef void (*bt_pm_hanlde_callback_t)(bt_pm_state_t state, uint8_t profile_id, bt_address_t* peer_addr);

typedef struct {
    struct list_node pm_services;
    struct list_node pm_devices;
    bool inited;
    uint16_t last_profile_id;

    bt_pm_timer_t pm_timer[CONFIG_BLUETOOTH_PM_MAX_TIMER_NUMBER];
    bt_pm_hanlde_callback_t pm_callback;
} bt_pm_manager_t;

typedef struct {
    struct list_node srv_node;

    bt_address_t peer_addr;
    uint8_t mode;
    uint16_t interval;
} bt_pm_device_t;

static const bt_pm_mode_t g_pm_mode[] = {
    /* sniff modes: max interval, min interval, attempt, timeout */
    { BT_PM_SNIFF_MAX, BT_PM_SNIFF_MIN, BT_PM_SNIFF_ATTEMPT, BT_PM_SNIFF_TIMEOUT, BT_LINK_MODE_SNIFF }, /* for BT_PM_SNIFF */
    { BT_PM_SNIFF1_MAX, BT_PM_SNIFF1_MIN, BT_PM_SNIFF1_ATTEMPT, BT_PM_SNIFF1_TIMEOUT, BT_LINK_MODE_SNIFF }, /* for BT_PM_SNIFF1 */
    { BT_PM_SNIFF2_MAX, BT_PM_SNIFF2_MIN, BT_PM_SNIFF2_ATTEMPT, BT_PM_SNIFF2_TIMEOUT, BT_LINK_MODE_SNIFF }, /* for BT_PM_SNIFF2 */
    { BT_PM_SNIFF3_MAX, BT_PM_SNIFF3_MIN, BT_PM_SNIFF3_ATTEMPT, BT_PM_SNIFF3_TIMEOUT, BT_LINK_MODE_SNIFF }, /* for BT_PM_SNIFF3 */
    { BT_PM_SNIFF4_MAX, BT_PM_SNIFF4_MIN, BT_PM_SNIFF4_ATTEMPT, BT_PM_SNIFF4_TIMEOUT, BT_LINK_MODE_SNIFF }, /* for BT_PM_SNIFF4 */
    { BT_PM_SNIFF5_MAX, BT_PM_SNIFF5_MIN, BT_PM_SNIFF5_ATTEMPT, BT_PM_SNIFF5_TIMEOUT, BT_LINK_MODE_SNIFF }, /* for BT_PM_SNIFF5 */
    { BT_PM_SNIFF6_MAX, BT_PM_SNIFF6_MIN, BT_PM_SNIFF6_ATTEMPT, BT_PM_SNIFF6_TIMEOUT, BT_LINK_MODE_SNIFF }, /* for BT_PM_SNIFF6 */
};

static const bt_pm_config_t g_pm_cfg[] = {
    { PROFILE_HFP_HF, BT_PM_SPEC_INDEX_0 }, /* HF spec table */
    { PROFILE_HFP_AG, BT_PM_SPEC_INDEX_0 }, /* AG spec table */
    { PROFILE_A2DP, BT_PM_SPEC_INDEX_1 }, /* AV spec table */
    { PROFILE_AVRCP_CT, BT_PM_SPEC_INDEX_1 }, /* AV spec table */
    { PROFILE_AVRCP_TG, BT_PM_SPEC_INDEX_1 }, /* AV spec table */
    { PROFILE_SPP, BT_PM_SPEC_INDEX_2 }, /* SPP spec table */
    { PROFILE_PANU, BT_PM_SPEC_INDEX_3 }, /* PAN spec table */
    { PROFILE_HID_DEV, BT_PM_SPEC_INDEX_4 }, /* HID spec table */
};

static const bt_pm_spec_table_t g_pm_spec[] = {
    /* HF AG: 0(BT_PM_SPEC_INDEX_0) */
    { (BT_PM_SNIFF), /* allow sniff */
        (0), /* the SSR entry */
        {
            { BT_PM_SNIFF, 7000 }, /* conn open */
            { BT_PM_NO_PREF, 0 }, /* conn close  */
            { BT_PM_NO_ACTION, 0 }, /* app open */
            { BT_PM_NO_ACTION, 0 }, /* app close */
            { BT_PM_SNIFF3, 7000 }, /* sco open */
            { BT_PM_SNIFF, 7000 }, /* sco close */
            { BT_PM_SNIFF, 7000 }, /* idle */
            { BT_PM_ACTIVE, 0 } /* busy */
        } },

    /* AV: 1(BT_PM_SPEC_INDEX_1) */
    { (BT_PM_SNIFF), /* allow sniff */
        (0), /* the SSR entry */
        {
            { BT_PM_SNIFF, 7000 }, /* conn open */
            { BT_PM_NO_PREF, 0 }, /* conn close */
            { BT_PM_NO_ACTION, 0 }, /* app open */
            { BT_PM_NO_ACTION, 0 }, /* app close */
            { BT_PM_NO_ACTION, 0 }, /* sco open */
            { BT_PM_NO_ACTION, 0 }, /* sco close */
            { BT_PM_SNIFF, 7000 }, /* idle */
            { BT_PM_ACTIVE, 0 } /* busy */
        } },

    /* SPP: 2(BT_PM_SPEC_INDEX_2) */
    { (BT_PM_SNIFF), /* allow sniff */
        (0), /* the SSR entry */
        {
            { BT_PM_ACTIVE, 0 }, /* conn open */
            { BT_PM_NO_PREF, 0 }, /* conn close */
            { BT_PM_ACTIVE, 0 }, /* app open */
            { BT_PM_NO_ACTION, 0 }, /* app close */
            { BT_PM_NO_ACTION, 0 }, /* sco open */
            { BT_PM_NO_ACTION, 0 }, /* sco close */
            { BT_PM_SNIFF, 1000 }, /* idle */
            { BT_PM_ACTIVE, 0 } /* busy */
        } },

    /* PAN: 3(BT_PM_SPEC_INDEX_3) */
    { (BT_PM_SNIFF), /* allow sniff */
        (0), /* the SSR entry */
        {
            { BT_PM_ACTIVE, 0 }, /* conn open */
            { BT_PM_NO_PREF, 0 }, /* conn close */
            { BT_PM_ACTIVE, 0 }, /* app open */
            { BT_PM_NO_ACTION, 0 }, /* app close */
            { BT_PM_NO_ACTION, 0 }, /* sco open */
            { BT_PM_NO_ACTION, 0 }, /* sco close */
            { BT_PM_SNIFF, 5000 }, /* idle */
            { BT_PM_ACTIVE, 0 } /* busy */
        } },

    /* HID: 4(BT_PM_SPEC_INDEX_4) */
    { (BT_PM_SNIFF), /* allow sniff */
        (0), /* the SSR entry */
        {
            { BT_PM_SNIFF, 5000 }, /* conn open */
            { BT_PM_NO_PREF, 0 }, /* conn close */
            { BT_PM_NO_ACTION, 0 }, /* app open */
            { BT_PM_NO_ACTION, 0 }, /* app close */
            { BT_PM_NO_ACTION, 0 }, /* sco open */
            { BT_PM_NO_ACTION, 0 }, /* sco close */
            { BT_PM_SNIFF2, 5000 }, /* idle */
            { BT_PM_SNIFF4, 200 } /* busy */
        } },
};

static bt_pm_manager_t g_pm_manager = { 0 };

static void pm_timeout_callback(service_timer_t* timer, void* data);

static bt_pm_service_t* pm_conn_service_find(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;
    struct list_node* node;
    struct list_node* tmp;
    bt_pm_service_t* service;

    list_for_every_safe(&manager->pm_services, node, tmp)
    {
        service = (bt_pm_service_t*)node;

        if (service->profile_id == profile_id && !bt_addr_compare(&service->peer_addr, peer_addr)) {
            return service;
        }
    }

    return NULL;
}

static bt_pm_service_t* pm_conn_service_add(uint8_t profile_id, uint8_t state, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;
    bt_pm_service_t* service;

    service = calloc(1, sizeof(bt_pm_service_t));
    if (!service) {
        return NULL;
    }

    service->profile_id = profile_id;
    service->state = state;
    memcpy(&service->peer_addr, peer_addr, sizeof(bt_address_t));
    list_add_tail(&manager->pm_services, &service->srv_node);
    return service;
}

static void pm_conn_service_remove(bt_pm_service_t* service)
{
    if (service) {
        list_delete(&service->srv_node);
        free(service);
    }
}

static bt_pm_device_t* pm_conn_device_find(bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;
    struct list_node* node;
    struct list_node* tmp;
    bt_pm_device_t* device;

    list_for_every_safe(&manager->pm_devices, node, tmp)
    {
        device = (bt_pm_device_t*)node;

        if (!bt_addr_compare(&device->peer_addr, peer_addr)) {
            return device;
        }
    }

    return NULL;
}

static bt_pm_device_t* pm_conn_device_add(bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;
    bt_pm_device_t* device;

    device = calloc(1, sizeof(bt_pm_device_t));
    if (!device) {
        return NULL;
    }

    memcpy(&device->peer_addr, peer_addr, sizeof(bt_address_t));
    device->mode = BT_LINK_MODE_ACTIVE;
    list_add_tail(&manager->pm_devices, &device->srv_node);
    return device;
}

static void pm_conn_device_remove(bt_pm_device_t* device)
{
    if (device) {
        list_delete(&device->srv_node);
        free(device);
    }
}

static bt_status_t pm_request_sniff(bt_address_t* peer_addr, bt_pm_mode_index_t index)
{
    bt_pm_device_t* device;
    bt_pm_mode_t mode;
    bt_status_t ret;

    if (index > BT_PM_MODE_INDEX_MAX) {
        BT_LOGE("%s, index:%d over max(%d)", __func__, index, BT_PM_MODE_INDEX_MAX);
        return BT_STATUS_PARM_INVALID;
    }

    device = pm_conn_device_find(peer_addr);
    if (!device) {
        BT_LOGE("%s, fail to find device:%s", __func__, bt_addr_str(peer_addr));
        return BT_STATUS_FAIL;
    }

    memcpy(&mode, &g_pm_mode[index], sizeof(bt_pm_mode_t));
    if (device->mode == BT_LINK_MODE_SNIFF && device->interval <= mode.max && device->interval >= mode.min) {
        return BT_STATUS_SUCCESS;
    }

    BT_LOGD("%s, peer_addr:%s, max:%d, min:%d, attempt:%d, timeout:%d", __func__, bt_addr_str(peer_addr), mode.max, mode.min, mode.attempt, mode.timeout);
    ret = bt_sal_set_power_mode(PRIMARY_ADAPTER, peer_addr, &mode);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, fail to set power mode, ret:%d", __func__, ret);
        return ret;
    }

    return ret;
}

static bt_status_t pm_request_active(bt_address_t* peer_addr)
{
    bt_pm_device_t* device;
    bt_status_t ret;
    bt_pm_mode_t mode = {
        .mode = BT_LINK_MODE_ACTIVE,
    };

    device = pm_conn_device_find(peer_addr);
    if (!device) {
        BT_LOGE("%s, fail to fail to find device:%s", __func__, bt_addr_str(peer_addr));
        return BT_STATUS_FAIL;
    }

    if (device->mode == BT_LINK_MODE_ACTIVE) {
        return BT_STATUS_SUCCESS;
    }

    BT_LOGD("%s, peer_addr:%s", __func__, bt_addr_str(peer_addr));
    ret = bt_sal_set_power_mode(PRIMARY_ADAPTER, peer_addr, &mode);
    if (ret != BT_STATUS_SUCCESS) {
        BT_LOGE("%s, fail to set power mode, ret:%d", __func__, ret);
        return ret;
    }

    return ret;
}

static bool pm_prefer_config(bt_address_t* peer_addr, bt_pm_prefer_mode_t* pm_action, uint32_t* timeout_ms, uint8_t* allowed_modes, uint16_t* profile_id)
{
    bt_pm_manager_t* manager = &g_pm_manager;
    struct list_node* node;
    struct list_node* tmp;
    bt_pm_prefer_mode_t power_mode = BT_PM_NO_ACTION;
    uint8_t allow_mask = 0;
    uint32_t timeout = 0;
    uint16_t id = 0;
    bool ret = false;

    list_for_every_safe(&manager->pm_services, node, tmp)
    {
        bt_pm_service_t* service;
        const bt_pm_config_t* config;
        const bt_pm_spec_table_t* table;
        const bt_pm_action_t* action;
        int j;

        service = (bt_pm_service_t*)node;

        if (bt_addr_compare(&service->peer_addr, peer_addr)) {
            continue;
        }

        for (j = 0; j < sizeof(g_pm_cfg) / sizeof(g_pm_cfg[0]); j++) {
            if (g_pm_cfg[j].profile_id == service->profile_id) {
                break;
            }
        }

        if (j == sizeof(g_pm_cfg) / sizeof(g_pm_cfg[0])) {
            return false;
        }

        config = &g_pm_cfg[j];
        table = &g_pm_spec[config->spec_idx];
        action = &table->actn_tbl[service->state];

        if (action->power_mode > power_mode || ((action->power_mode == power_mode) && (service->profile_id == *profile_id))) {
            power_mode = action->power_mode;
            timeout = action->timeout;
            id = config->profile_id;
            allow_mask = table->allow_mask;
            manager->last_profile_id = id;
            ret = true;
        }
    }

    *pm_action = power_mode;
    *timeout_ms = timeout;
    *allowed_modes = allow_mask;
    *profile_id = id;
    return ret;
}

static bool pm_start_timer(bt_address_t* peer_addr, uint32_t timeout, uint8_t profile_id, bt_pm_prefer_mode_t pm_action)
{
    bt_pm_manager_t* manager = &g_pm_manager;
    bt_pm_timer_t* timer;
    int i;

    for (i = 0; i < CONFIG_BLUETOOTH_PM_MAX_TIMER_NUMBER; i++) {
        timer = &manager->pm_timer[i];

        if (!timer->active) {
            timer->active = true;
            timer->pm_action = pm_action;
            timer->profile_id = profile_id;
            memcpy(&timer->peer_addr, peer_addr, sizeof(timer->peer_addr));
            if (timer->pm_timer) {
                service_loop_cancel_timer(timer->pm_timer);
            }

            timer->pm_timer = service_loop_timer(timeout, 0, pm_timeout_callback, timer);
            return true;
        }
    }

    BT_LOGE("%s, timer id:%d over max:%d", __func__, i, CONFIG_BLUETOOTH_PM_MAX_TIMER_NUMBER);
    return false;
}

static void pm_stop_timer(bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;
    bt_pm_timer_t* timer;
    int i;

    for (i = 0; i < CONFIG_BLUETOOTH_PM_MAX_TIMER_NUMBER; i++) {
        timer = &manager->pm_timer[i];
        if (timer->active && !bt_addr_compare(&timer->peer_addr, peer_addr)) {
            timer->active = false;
            service_loop_cancel_timer(timer->pm_timer);
            timer->pm_timer = NULL;
        }
    }
}

static void pm_mode_request(bt_address_t* peer_addr, uint8_t req, uint16_t profile_id)
{
    bool connected;
    bt_pm_prefer_mode_t pm_action;
    uint32_t timeout_ms;
    uint8_t allowed_modes;
    bool ret;

    connected = adapter_is_remote_connected(peer_addr, BT_TRANSPORT_BREDR);
    if (!connected) {
        BT_LOGE("%s, device:%s disconnected", __func__, bt_addr_str(peer_addr));
        pm_stop_timer(peer_addr);
        return;
    }

    ret = pm_prefer_config(peer_addr, &pm_action, &timeout_ms, &allowed_modes, &profile_id);
    if (!ret) {
        BT_LOGE("%s, device:%s prefer pm config fail, ret:%d", __func__, bt_addr_str(peer_addr), ret);
        return;
    }

    if (!(allowed_modes & BT_PM_SNIFF)) {
        BT_LOGE("%s, modes:0x%x not allowed", __func__, allowed_modes);
        return;
    }

    switch (req) {
    case BT_PM_EXECUTE: {
        if (pm_action & BT_PM_ACTIVE) {
            pm_request_active(peer_addr);
        } else if (pm_action & BT_PM_SNIFF) {
            pm_request_sniff(peer_addr, pm_action & BT_PM_PREF_MODE_MASK);
        }
    } break;
    case BT_PM_RESTART: {
        if (pm_action & BT_PM_ACTIVE) {
            pm_request_active(peer_addr);
        } else if (timeout_ms > 0) {
            pm_stop_timer(peer_addr);
            pm_start_timer(peer_addr, timeout_ms, profile_id, pm_action);
        }
    } break;
    default:
        break;
    }
}

static void pm_timeout_callback(service_timer_t* timer, void* data)
{
    bt_pm_timer_t* pm_timer = (bt_pm_timer_t*)data;

    if (pm_timer->active) {
        pm_timer->active = false;
    }

    BT_LOGD("%s, addr:%s, profile_id:%d, pm_action:%d", __func__, bt_addr_str(&pm_timer->peer_addr), pm_timer->profile_id, pm_timer->pm_action);
    pm_mode_request(&pm_timer->peer_addr, BT_PM_EXECUTE, pm_timer->profile_id);
}

static bool pm_check_prefer_action(uint8_t profile_id)
{
    int j;

    for (j = 0; j < sizeof(g_pm_cfg) / sizeof(g_pm_cfg[0]); j++) {
        if (g_pm_cfg[j].profile_id == profile_id) {
            break;
        }
    }

    if (j == sizeof(g_pm_cfg) / sizeof(g_pm_cfg[0])) {
        BT_LOGE("%s, invalid profile_id:%d", __func__, profile_id);
        return false;
    }

    if (g_pm_spec[g_pm_cfg[j].spec_idx].actn_tbl->power_mode == BT_PM_NO_PREF) {
        BT_LOGD("%s, BT_PM_NO_PREF", __func__);
        return false;
    }

    return true;
}

static void bt_pm_hanlde_callback(bt_pm_state_t state, uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_service_t* service;

    service = pm_conn_service_find(profile_id, peer_addr);
    if (!service) {
        service = pm_conn_service_add(profile_id, state, peer_addr);
        if (!service) {
            BT_LOGE("%s, fail to add service device:%s, profile_id:%d", __func__, bt_addr_str(peer_addr), profile_id);
            return;
        }
    } else {
        service->state = state;
    }

    if (!pm_check_prefer_action(profile_id)) {
        pm_conn_service_remove(service);
    }

    pm_mode_request(peer_addr, BT_PM_RESTART, profile_id);
}

static void bt_pm_register(bt_pm_hanlde_callback_t cb)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    manager->pm_callback = cb;
}

static void bt_pm_unregister()
{
    bt_pm_manager_t* manager = &g_pm_manager;

    manager->pm_callback = NULL;
}

void bt_pm_conn_open(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_CONN_OPEN, profile_id, peer_addr);
}

void bt_pm_conn_close(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_CONN_CLOSE, profile_id, peer_addr);
}

void bt_pm_app_open(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_APP_OPEN, profile_id, peer_addr);
}

void bt_pm_app_close(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_APP_CLOSE, profile_id, peer_addr);
}

void bt_pm_sco_open(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_SCO_OPEN, profile_id, peer_addr);
}

void bt_pm_sco_close(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_SCO_CLOSE, profile_id, peer_addr);
}

void bt_pm_idle(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_CONN_IDLE, profile_id, peer_addr);
}

void bt_pm_busy(uint8_t profile_id, bt_address_t* peer_addr)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->pm_callback) {
        return;
    }

    manager->pm_callback(BT_PM_STATE_CONN_BUSY, profile_id, peer_addr);
}

void bt_pm_init(void)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (manager->inited) {
        return;
    }

    manager->inited = true;
    manager->last_profile_id = PROFILE_UNKOWN;
    bt_pm_register(bt_pm_hanlde_callback);
    list_initialize(&manager->pm_services);
    list_initialize(&manager->pm_devices);
}

void bt_pm_cleanup(void)
{
    bt_pm_manager_t* manager = &g_pm_manager;

    if (!manager->inited) {
        return;
    }

    manager->inited = false;
    bt_pm_unregister();
    list_delete(&manager->pm_services);
    list_delete(&manager->pm_devices);
}

void bt_pm_remote_link_mode_changed(bt_address_t* addr, uint8_t mode, uint16_t sniff_interval)
{
    bt_pm_device_t* device;
    bt_pm_manager_t* manager = &g_pm_manager;

    BT_LOGD("%s, addr:%s, mode:%d, sniff_interval:%" PRId16, __func__, bt_addr_str(addr), mode, sniff_interval);
    device = pm_conn_device_find(addr);
    if (!device) {
        BT_LOGE("%s, fail to find device:%s", __func__, bt_addr_str(addr));
        return;
    }

    device->interval = sniff_interval;
    device->mode = mode;

    switch (mode) {
    case BT_LINK_MODE_ACTIVE: {
        pm_stop_timer(addr);
        pm_mode_request(addr, BT_PM_RESTART, manager->last_profile_id);
    } break;
    case BT_LINK_MODE_SNIFF: {
        pm_stop_timer(addr);
    } break;
    default:
        break;
    }
}

void bt_pm_remote_device_connected(bt_address_t* addr)
{
    bt_pm_device_t* device;

    device = pm_conn_device_add(addr);
    if (!device) {
        BT_LOGE("%s, fail to add device:%s", __func__, bt_addr_str(addr));
        return;
    }
}

void bt_pm_remote_device_disconnected(bt_address_t* addr)
{
    bt_pm_device_t* device;

    device = pm_conn_device_find(addr);
    if (!device) {
        BT_LOGE("%s, fail to find device:%s", __func__, bt_addr_str(addr));
        return;
    }
    pm_conn_device_remove(device);
}
