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
#define LOG_TAG "adver"

#include <stdlib.h>
#include <string.h>

#include "adapter_internel.h"
#include "advertising.h"
#include "bluetooth.h"
#include "bt_list.h"
#include "index_allocator.h"
#include "sal_interface.h"
#include "sal_le_advertise_interface.h"
#include "service_loop.h"
#include "utils/log.h"

#ifndef CONFIG_BLUETOOTH_LE_ADVERTISER_MAX_NUM
#define CONFIG_BLUETOOTH_LE_ADVERTISER_MAX_NUM 2
#endif

typedef struct {
    uint8_t* adv_data;
    uint16_t adv_len;
    uint8_t* scan_rsp_data;
    uint16_t scan_rsp_len;
    ble_adv_params_t params;
} advertising_info_t;

typedef struct advertiser {
    struct list_node adver_node;
    void* remote;
    uint8_t adv_id;
    advertiser_callback_t callbacks;
    service_timer_t* adv_start;
} advertiser_t;

typedef struct {
    bool started;
    index_allocator_t* adv_allocator;
    struct list_node advertiser_list;
} adv_manager_t;

typedef struct {
    advertiser_t* adver;
    uint8_t adv_id;
    advertising_info_t* adv_info;
    uint8_t state;
} adv_event_t;

static adv_manager_t adv_manager;

static void* get_adver(advertiser_t* adver)
{
    return adver->remote ? adver->remote : adver;
}

static void advertiser_info_free(advertising_info_t* adv_info)
{
    if (adv_info) {
        if (adv_info->adv_data)
            free(adv_info->adv_data);
        if (adv_info->scan_rsp_data)
            free(adv_info->scan_rsp_data);
        free(adv_info);
    }
}

static advertising_info_t* advertiser_info_copy(ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len)
{
    advertising_info_t* adv_info = calloc(1, sizeof(advertising_info_t));
    if (!adv_info)
        goto fail;

    if (adv_len && adv_data) {
        adv_info->adv_data = malloc(adv_len);
        if (!adv_info->adv_data)
            goto fail;
        memcpy(adv_info->adv_data, adv_data, adv_len);
        adv_info->adv_len = adv_len;
    }

    if (scan_rsp_len && scan_rsp_data) {
        adv_info->scan_rsp_data = malloc(scan_rsp_len);
        if (!adv_info->scan_rsp_data)
            goto fail;
        memcpy(adv_info->scan_rsp_data, scan_rsp_data, scan_rsp_len);
        adv_info->scan_rsp_len = scan_rsp_len;
    }
    memcpy(&adv_info->params, params, sizeof(ble_adv_params_t));

    return adv_info;
fail:
    advertiser_info_free(adv_info);
    return NULL;
}

static advertiser_t* alloc_new_advertiser(void* remote, const advertiser_callback_t* cbs)
{
    advertiser_t* adver = malloc(sizeof(advertiser_t));
    if (!adver)
        return NULL;

    adver->remote = remote;
    adver->adv_id = 0;
    adver->adv_start = NULL;
    memcpy(&adver->callbacks, cbs, sizeof(advertiser_callback_t));

    return adver;
}

static void destroy_advertiser(advertiser_t* adver)
{
    if (adver->adv_id)
        index_free(adv_manager.adv_allocator, adver->adv_id - 1);
    free(adver);
}

static void delete_advertiser(advertiser_t* adver)
{
    service_loop_cancel_timer(adver->adv_start);
    adver->adv_start = NULL;
    list_delete(&adver->adver_node);
}

static bool is_advertiser_exist(advertiser_t* adver)
{
    struct list_node* node;

    list_for_every(&adv_manager.advertiser_list, node)
    {
        if ((advertiser_t*)node == adver)
            return true;
    }

    return false;
}

static advertiser_t* get_advertiser_if_exist(uint8_t adv_id)
{
    struct list_node* node;

    list_for_every(&adv_manager.advertiser_list, node)
    {
        advertiser_t* adver = (advertiser_t*)node;
        if (adver->adv_id == adv_id)
            return adver;
    }

    return NULL;
}

static void start_advertising_timeout(service_timer_t* timer, void* userdata)
{
    advertiser_t* adver = (advertiser_t*)userdata;

    if (!is_advertiser_exist(adver)) {
        BT_LOGE("%s, timer expeared, adver not found", __func__);
        return;
    }

    delete_advertiser(adver);
    adver->callbacks.on_advertising_start(get_adver(adver), 0, BT_ADV_STATUS_START_TIMEOUT);
    destroy_advertiser(adver);
}

static void advertiser_start_event(void* data)
{
    assert(data);
    adv_event_t* start = (adv_event_t*)data;
    advertiser_t* adver = start->adver;
    advertising_info_t* adv_info = start->adv_info;
    int adv_id;

    free(start);
    if (!adv_manager.started)
        return;

    adv_id = index_alloc(adv_manager.adv_allocator);
    if (adv_id < 0) {
        adver->callbacks.on_advertising_start(get_adver(adver), 0, BT_ADV_STATUS_START_NOMEM);
        goto fail;
    }

    adver->adv_id = adv_id + 1;
    if (bt_sal_le_start_adv(PRIMARY_ADAPTER, adver->adv_id, &adv_info->params, adv_info->adv_data,
            adv_info->adv_len, adv_info->scan_rsp_data,
            adv_info->scan_rsp_len)
        != BT_STATUS_SUCCESS) {
        adver->callbacks.on_advertising_start(get_adver(adver), 0, BT_ADV_STATUS_STACK_ERR);
        goto fail;
    }

    list_add_tail(&adv_manager.advertiser_list, &adver->adver_node);
    advertiser_info_free(adv_info);
    adver->adv_start = service_loop_timer_no_repeating(1000, start_advertising_timeout, adver);

    return;
fail:
    destroy_advertiser(adver);
    advertiser_info_free(adv_info);
}

static void advertiser_stop_event(void* data)
{
    assert(data);
    adv_event_t* stop = (adv_event_t*)data;
    advertiser_t* adver = stop->adver;
    uint8_t adv_id = stop->adv_id;

    free(stop);
    if (!adv_manager.started)
        return;

    if (adver) {
        if (!is_advertiser_exist(adver)) {
            BT_LOGD("%s, advertiser: %p not exist", __func__, adver);
            return;
        }
    } else {
        adver = get_advertiser_if_exist(adv_id);
        if (!adver) {
            BT_LOGD("%s, adver_id: %d not exist", __func__, adv_id);
            return;
        }
    }

    bt_sal_le_stop_adv(PRIMARY_ADAPTER, adver->adv_id);
}

static void advertiser_notify_state(void* data)
{
    adv_event_t* advstate = (adv_event_t*)data;
    advertiser_t* adver;

    if (!adv_manager.started) {
        goto exit;
    }

    adver = get_advertiser_if_exist(advstate->adv_id);
    if (!adver) {
        goto exit;
    }

    if (advstate->state == LE_ADVERTISING_STARTED) {
        service_loop_cancel_timer(adver->adv_start);
        adver->adv_start = NULL;
        adver->callbacks.on_advertising_start(get_adver(adver), advstate->adv_id, BT_ADV_STATUS_SUCCESS);
    } else if (advstate->state == LE_ADVERTISING_STOPPED) {
        delete_advertiser(adver);
        adver->callbacks.on_advertising_stopped(get_adver(adver), advstate->adv_id);
        destroy_advertiser(adver);
    }

exit:
    free(advstate);
}

static void advertisers_cleanup(void* data)
{
    struct list_node* node;
    struct list_node* tmp;

    if (!adv_manager.started)
        return;

    list_for_every_safe(&adv_manager.advertiser_list, node, tmp)
    {
        advertiser_t* adver = (advertiser_t*)node;
        bt_sal_le_stop_adv(PRIMARY_ADAPTER, adver->adv_id);
        delete_advertiser(adver);
        adver->callbacks.on_advertising_stopped(get_adver(adver), adver->adv_id);
        destroy_advertiser(adver);
    }

    list_delete(&adv_manager.advertiser_list);
    index_allocator_delete(&adv_manager.adv_allocator);
    adv_manager.started = false;
}

void advertising_on_state_changed(uint8_t adv_id, uint8_t state)
{
    adv_event_t* advstate = malloc(sizeof(adv_event_t));

    if (!advstate) {
        BT_LOGE("adv_id: %d state malloc failed", adv_id);
        return;
    }

    advstate->adv_id = adv_id;
    advstate->state = state;
    do_in_service_loop(advertiser_notify_state, advstate);
}

bt_advertiser_t* start_advertising(void* remote,
    ble_adv_params_t* params,
    uint8_t* adv_data,
    uint16_t adv_len,
    uint8_t* scan_rsp_data,
    uint16_t scan_rsp_len,
    const advertiser_callback_t* cbs)
{
    if (!adapter_is_le_enabled())
        return NULL;

    advertiser_t* adver = alloc_new_advertiser(remote, cbs);
    if (!adver)
        return NULL;

    adv_event_t* start = malloc(sizeof(adv_event_t));
    if (!start) {
        destroy_advertiser(adver);
        return NULL;
    }

    start->adver = adver;
    start->adv_info = advertiser_info_copy(params, adv_data, adv_len,
        scan_rsp_data, scan_rsp_len);
    do_in_service_loop(advertiser_start_event, start);

    return (bt_advertiser_t*)adver;
}

void stop_advertising(bt_advertiser_t* adver)
{
    if (!adapter_is_le_enabled())
        return;

    adv_event_t* stop = malloc(sizeof(adv_event_t));
    if (!stop)
        return;

    stop->adver = (advertiser_t*)adver;
    do_in_service_loop(advertiser_stop_event, stop);
}

void stop_advertising_id(uint8_t adv_id)
{
    if (!adapter_is_le_enabled())
        return;

    adv_event_t* stop = malloc(sizeof(adv_event_t));
    if (!stop)
        return;

    stop->adver = NULL;
    stop->adv_id = adv_id;
    do_in_service_loop(advertiser_stop_event, stop);
}

bool advertising_is_supported(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_ADV
    return true;
#endif
    return false;
}

/** release remote related resources when client detaches */
void advertising_on_remote_detached(void* remote)
{
}

void adv_manager_init(void)
{
    memset(&adv_manager, 0, sizeof(adv_manager));
    adv_manager.adv_allocator = index_allocator_create(CONFIG_BLUETOOTH_LE_ADVERTISER_MAX_NUM);
    assert(adv_manager.adv_allocator);
    list_initialize(&adv_manager.advertiser_list);
    adv_manager.started = true;
}

void adv_manager_cleanup(void)
{
    do_in_service_loop(advertisers_cleanup, NULL);
}
