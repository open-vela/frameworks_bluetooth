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
#define LOG_TAG "scanner"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "adapter_internel.h"
#include "bluetooth.h"
#include "bt_hash.h"
#include "bt_le_scan.h"
#include "bt_list.h"
#include "bt_time.h"
#include "sal_interface.h"
#include "scan_filter.h"
#include "scan_manager.h"
#include "scan_record.h"
#include "service_loop.h"
#include "utils/log.h"

#ifndef CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM
#define CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM 2
#endif
#ifndef CONFIG_BT_LE_ADV_REPORT_SIZE
#define CONFIG_BT_LE_ADV_REPORT_SIZE 10
#endif
#define BT_LE_ADV_REPORT_DURATION_MS 500
#define BT_LE_ADV_REPORT_PERIOD_MS 5000

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

typedef struct {
    bt_address_t addr;
    ble_addr_type_t addr_type;
    uint32_t timestamp;
} scanner_device_t;

typedef struct scanner {
    struct list_node scanning_node;
    void* remote;
    uint8_t scanner_id;
    bool is_scanning;
    ble_scan_filter_policy_t policy;
    ble_scan_filter_t filter;
    const scanner_callbacks_t* callbacks;
} scanner_t;

typedef struct {
    scanner_t* scanner;
    bool use_setting;
    ble_scan_settings_t settings;
} scanner_ctrl_t;

typedef struct scanner_manager {
    scanner_t* scanner_list[CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM];
    struct list_node scanning_list;
    uint32_t hash_table[CONFIG_BT_LE_ADV_REPORT_SIZE];
    bt_list_t* devices;
    uint8_t scanner_cnt;
    bool is_scanning;
} scanner_manager_t;

static scanner_manager_t scanner_manager;
static void stop_scan(void* data);

static bt_scanner_t* get_remote(scanner_t* scanner)
{
    return scanner->remote ? scanner->remote : scanner;
}

static scanner_device_t* alloc_device(bt_address_t* addr, ble_addr_type_t addr_type)
{
    scanner_device_t* device;

    device = zalloc(sizeof(scanner_device_t));
    if (!device) {
        return NULL;
    }

    memcpy(&device->addr, addr, sizeof(*addr));
    device->addr_type = addr_type;

    return device;
}

static void free_device(void* data)
{
    scanner_device_t* device = data;

    free(device);
}

static scanner_device_t* scanner_find_device(const bt_address_t* addr, ble_addr_type_t addr_type)
{
    bt_list_node_t* node;
    bt_list_t* list = scanner_manager.devices;

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        scanner_device_t* device = bt_list_node(node);
        if (!memcmp(&device->addr, addr, sizeof(bt_address_t)) && (device->addr_type == addr_type))
            return device;
    }

    return NULL;
}

static scanner_device_t* scanner_add_device(bt_address_t* addr, ble_addr_type_t addr_type, uint32_t timestamp_ms)
{
    scanner_device_t* device;

    device = alloc_device(addr, addr_type);
    assert(device);

    device->timestamp = timestamp_ms;
    bt_list_add_tail(scanner_manager.devices, device);

    return device;
}

static scanner_t* alloc_new_scanner(void* remote, const scanner_callbacks_t* cbs)
{
    scanner_t* app = malloc(sizeof(scanner_t));

    if (!app)
        return NULL;

    memset(app, 0, sizeof(scanner_t));
    app->remote = remote;
    app->callbacks = cbs;

    return app;
}

static void delete_scanner(scanner_t* scanner)
{
    if (scanner->is_scanning)
        list_delete(&scanner->scanning_node);

    free(scanner);
}

static bool scanner_compare(scanner_t* src, scanner_t* dest)
{
    if (dest->remote)
        return src->remote == dest->remote;
    else
        return src->callbacks == dest->callbacks;
}

static bool scanner_is_registered(scanner_t* scanner)
{
    for (int i = 0; i < CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM; i++) {
        if (scanner_manager.scanner_list[i] == scanner)
            return true;
    }

    return false;
}

static bool scanner_match_duration(scanner_device_t* device, uint32_t duration, uint32_t period, uint32_t timestamp)
{
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;

    t1 = device->timestamp;
    t2 = t1 + duration;
    t3 = t1 + period;

    if (timestamp < t1) {
        /* Timestamp overflow */
        device->timestamp = timestamp;
        return false;
    } else if ((t1 <= timestamp) && timestamp < t2) {
        return true;
    } else if ((t2 <= timestamp) && timestamp < t3) {
        return false;
    } else {
        device->timestamp = timestamp;
        return true;
    }
}

static bool scanner_hsearch_find(const void* keyarg, size_t len)
{
    scanner_manager_t* manager = &scanner_manager;
    uint32_t hash;
    int i;

    hash = bt_hash4(keyarg, len);
    for (i = 0; i < ARRAY_SIZE(manager->hash_table); i++) {
        if (manager->hash_table[i] == hash) {
            return true;
        }

        if (manager->hash_table[i] == 0) {
            break;
        }
    }

    return false;
}

static void scanner_hsearch_add(const void* keyarg, size_t len)
{
    scanner_manager_t* manager = &scanner_manager;
    uint32_t hash;
    int i;

    hash = bt_hash4(keyarg, len);
    for (i = 0; i < ARRAY_SIZE(manager->hash_table); i++) {
        if (manager->hash_table[i] == 0) {
            manager->hash_table[i] = hash;
            break;
        }
    }
}

static void scanner_hsearch_free()
{
    scanner_manager_t* manager = &scanner_manager;

    memset(&manager->hash_table, 0, sizeof(manager->hash_table));
}

static void notify_scanners_scan_result(void* data)
{
    struct list_node* node;
    ble_scan_result_t* result = (ble_scan_result_t*)data;
    scan_record_t record = { 0 };
    scanner_device_t* device;
    uint32_t timestamp_ms;

    timestamp_ms = get_os_timestamp_ms();
    list_for_every(&scanner_manager.scanning_list, node)
    {
        scanner_t* scanner = (scanner_t*)node;

        if (!scanner->filter.active) {
            goto exit_filter;
        }

        if (!record.active) {
            scan_record_parse(&record, result->adv_data, result->length);
            record.active = true;
        }

        device = scanner_find_device(&result->addr, result->addr_type);
        if (!device && !scanner_match_filter(&record, &scanner->filter)) {
            continue;
        }

        if (!device) {
            device = scanner_add_device(&result->addr, result->addr_type, timestamp_ms);
        }

        if (scanner->filter.duplicated) {
            if (!scanner_match_duration(device, scanner->filter.duration, scanner->filter.period, timestamp_ms)) {
                continue;
            }

            if (scanner_hsearch_find(result->adv_data, result->length)) {
                BT_LOGD("scanner_hsearch_find addr:%s", bt_addr_str(&result->addr));
                continue;
            } else {
                scanner_hsearch_add(result->adv_data, result->length);
                BT_LOGD("scanner_hsearch_add addr:%s", bt_addr_str(&result->addr));
            }
        }

    exit_filter:
        scanner->callbacks->on_scan_result(get_remote(scanner), result);
    }

    free(data);
}

static uint32_t register_scanner(scanner_t* scanner)
{
    int i;

    if (!scanner)
        return BT_SCAN_STATUS_START_FAIL;

    if (scanner_manager.scanner_cnt == CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM) {
        delete_scanner(scanner);
        return BT_SCAN_STATUS_SCANNER_REG_NOMEM;
    }

    for (i = 0; i < CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM; i++) {
        if (scanner_manager.scanner_list[i] != NULL && scanner_compare(scanner_manager.scanner_list[i], scanner)) {
            delete_scanner(scanner);
            return BT_SCAN_STATUS_SCANNER_EXISTED;
        }
    }

    for (i = 0; i < CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM; i++) {
        if (!scanner_manager.scanner_list[i]) {
            scanner->scanner_id = i;
            scanner_manager.scanner_list[i] = scanner;
            scanner_manager.scanner_cnt++;
            return BT_SCAN_STATUS_SUCCESS;
        }
    }

    return BT_SCAN_STATUS_START_FAIL;
}

static void unregister_scanner(scanner_t* scanner)
{
    if (!scanner)
        return;

    if (!scanner_is_registered(scanner))
        return;

    stop_scan((void*)scanner);
    scanner->callbacks->on_scan_stopped(get_remote(scanner));
    scanner_manager.scanner_list[scanner->scanner_id] = NULL;
    scanner_manager.scanner_cnt--;
    delete_scanner(scanner);
}

static void stop_scanner(void* data)
{
    scanner_ctrl_t* stop = data;
    scanner_t* scanner = stop->scanner;

    unregister_scanner(scanner);

    free(data);
}

static void cleanup_scanner(void* data)
{
    for (int i = 0; i < CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM; i++) {
        scanner_t* scanner = scanner_manager.scanner_list[i];
        if (scanner)
            unregister_scanner(scanner);
    }

    list_delete(&scanner_manager.scanning_list);
    bt_list_free(scanner_manager.devices);
    scanner_manager.devices = NULL;
}

static int setup_scan_parameter(ble_scan_settings_t* settings, ble_scan_params_t* param)
{
    if (!settings || !param)
        return BT_SCAN_STATUS_START_FAIL;

    param->scan_phy = settings->scan_phy;
    param->scan_type = settings->scan_type;

    switch (settings->scan_mode) {
    case BT_SCAN_MODE_LOW_POWER:
        param->scan_interval = SCAN_MODE_LOW_POWER_INTERVAL;
        param->scan_window = SCAN_MODE_LOW_POWER_WINDOW;
        break;
    case BT_SCAN_MODE_BALANCED:
        param->scan_interval = SCAN_MODE_BALANCED_INTERVAL;
        param->scan_window = SCAN_MODE_BALANCED_WINDOW;
        break;
    case BT_SCAN_MODE_LOW_LATENCY:
        param->scan_interval = SCAN_MODE_LOW_LATENCY_INTERVAL;
        param->scan_window = SCAN_MODE_LOW_LATENCY_WINDOW;
        break;
    default:
        break;
    }

    return BT_SCAN_STATUS_SUCCESS;
}

static void start_scan(void* data)
{
    scanner_ctrl_t* start = data;
    scanner_t* scanner = start->scanner;
    ble_scan_params_t params = { 100, 100, BT_LE_SCAN_TYPE_PASSIVE, BT_LE_1M_PHY };

    uint32_t status = register_scanner(scanner);
    if (status != BT_SCAN_STATUS_SUCCESS) {
        scanner->callbacks->on_scan_start_status(get_remote(scanner), status);
        goto ret;
    }

    if (start->use_setting) {
        setup_scan_parameter(&start->settings, &params);
    }

    if (!scanner_manager.is_scanning && !list_length(&scanner_manager.scanning_list)) {
        bt_sal_le_set_scan_parameters(PRIMARY_ADAPTER, &params);
        if (bt_sal_le_start_scan(PRIMARY_ADAPTER) != BT_STATUS_SUCCESS) {
            scanner->callbacks->on_scan_start_status(get_remote(scanner), BT_SCAN_STATUS_START_FAIL);
            goto ret;
        }
        scanner_manager.is_scanning = true;
    }

    scanner->is_scanning = true;
    list_add_tail(&scanner_manager.scanning_list, &scanner->scanning_node);
    scanner->callbacks->on_scan_start_status(get_remote(scanner), BT_SCAN_STATUS_SUCCESS);

ret:
    free(start);
}

static void stop_scan(void* data)
{
    scanner_t* scanner = (scanner_t*)data;

    if (!scanner_is_registered(scanner))
        return;

    if (!scanner->is_scanning)
        return;

    list_delete(&scanner->scanning_node);
    scanner->is_scanning = false;
    if (scanner_manager.is_scanning && !list_length(&scanner_manager.scanning_list)) {
        bt_sal_le_stop_scan(PRIMARY_ADAPTER);
        bt_list_clear(scanner_manager.devices);
        scanner_hsearch_free();
        scanner_manager.is_scanning = false;
    }
}

void scan_on_state_changed(uint8_t state)
{
    BT_LOGD("%s, state:%d", __func__, state);
}

void scan_on_result_data_update(ble_scan_result_t* result_info, char* adv_data)
{
    ble_scan_result_t* result = malloc(sizeof(ble_scan_result_t) + result_info->length);

    if (!result)
        return;

    /* TODO : gdb debug check */
    memcpy(result, result_info, sizeof(ble_scan_result_t));
    memcpy(result->adv_data, adv_data, result_info->length);

    do_in_service_loop(notify_scanners_scan_result, result);
}

bt_scanner_t* scanner_start_scan(void* remote, const scanner_callbacks_t* cbs)
{
    if (!adapter_is_le_enabled())
        return NULL;

    scanner_t* scanner = alloc_new_scanner(remote, cbs);
    if (!scanner)
        return NULL;

    scanner_ctrl_t* start = malloc(sizeof(scanner_ctrl_t));
    if (start == NULL) {
        free(scanner);
        return NULL;
    }

    start->scanner = scanner;
    start->use_setting = false;

    do_in_service_loop(start_scan, (void*)start);

    return (bt_scanner_t*)scanner;
}

bt_scanner_t* scanner_start_scan_with_filters(void* remote,
    ble_scan_settings_t* settings,
    ble_scan_filter_t* filter,
    const scanner_callbacks_t* cbs)
{
    scanner_t* scanner;
    scanner_ctrl_t* start;

    if (!adapter_is_le_enabled())
        return NULL;

    scanner = alloc_new_scanner(remote, cbs);
    if (!scanner)
        return NULL;

    start = zalloc(sizeof(scanner_ctrl_t));
    if (start == NULL) {
        free(scanner);
        return NULL;
    }

    if (filter && filter->active) {
        filter->duration = BT_LE_ADV_REPORT_DURATION_MS;
        filter->period = BT_LE_ADV_REPORT_PERIOD_MS;
        filter->duplicated = 0;
        memcpy(&scanner->filter, filter, sizeof(*filter));
    }

    start->scanner = scanner;
    start->use_setting = true;
    memcpy(&start->settings, settings, sizeof(*settings));

    do_in_service_loop(start_scan, (void*)start);

    return (bt_scanner_t*)scanner;
}

bt_scanner_t* scanner_start_scan_settings(void* remote,
    ble_scan_settings_t* settings,
    const scanner_callbacks_t* cbs)
{
    return scanner_start_scan_with_filters(remote, settings, NULL, cbs);
}

void scanner_stop_scan(bt_scanner_t* scanner)
{
    if (!adapter_is_le_enabled())
        return;

    scanner_ctrl_t* stop = malloc(sizeof(scanner_ctrl_t));
    if (stop == NULL)
        return;

    stop->scanner = (scanner_t*)scanner;
    do_in_service_loop(stop_scanner, (void*)stop);
}

bool scan_is_supported(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
    return true;
#endif
    return false;
}

void scan_manager_init(void)
{
    memset(&scanner_manager, 0, sizeof(scanner_manager));
    list_initialize(&scanner_manager.scanning_list);
    scanner_manager.devices = bt_list_new(free_device);
}

void scan_manager_cleanup(void)
{
    do_in_service_loop(cleanup_scanner, NULL);
}

void scanner_dump(bt_scanner_t* scanner)
{
}
