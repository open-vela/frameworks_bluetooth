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

#include <nuttx/list.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_le_scan.h"
#include "sal_adapter_interface.h"
#include "scan_manager.h"
#include "service_loop.h"
#include "utils/log.h"

#ifndef CONFIG_OBELISK_LE_SCANNER_MAX_NUM
#define CONFIG_OBELISK_LE_SCANNER_MAX_NUM 2
#endif

typedef struct scanner {
    struct list_node scanning_node;
    void *remote;
    uint8_t scanner_id;
    bool is_scanning;
    const scanner_callbacks_t *callbacks;
} scanner_t;

typedef struct {
    scanner_t *scanner;
    bool use_setting;
    ble_scan_settings_t settings;
} scanner_ctrl_t;

typedef struct scanner_manager {
    scanner_t *scanner_list[CONFIG_OBELISK_LE_SCANNER_MAX_NUM];
    struct list_node scanning_list;
    uint8_t scanner_cnt;
    bool is_scanning;
} scanner_manager_t;

static scanner_manager_t scanner_manager;
static void stop_scan(void *data);

static bt_scanner_t *get_remote(scanner_t *scanner)
{
    return scanner->remote ? scanner->remote : scanner;
}

static scanner_t *alloc_new_scanner(void *remote, const scanner_callbacks_t *cbs)
{
    scanner_t *app = malloc(sizeof(scanner_t));

    if (!app)
        return NULL;

    app->remote = remote;
    app->callbacks = cbs;

    return app;
}

static void delete_scanner(scanner_t *scanner)
{
    if (scanner->is_scanning)
        list_delete(&scanner->scanning_node);
    free(scanner);
}

static bool scanner_compare(scanner_t *src, scanner_t *dest)
{
    if (dest->remote)
        return src->remote == dest->remote;
    else
        return src->callbacks == dest->callbacks;
}

static bool scanner_is_registered(scanner_t *scanner)
{
    for (int i = 0; i < CONFIG_OBELISK_LE_SCANNER_MAX_NUM; i++) {
        if (scanner_manager.scanner_list[i] == scanner)
            return true;
    }

    return false;
}

static void notify_scanners_scan_result(void *data)
{
    struct list_node *node;
    ble_scan_result_t *result = (ble_scan_result_t *)data;

    list_for_every(&scanner_manager.scanning_list, node)
    {
        scanner_t *scanner = (scanner_t *)node;
        scanner->callbacks->on_scan_result(get_remote(scanner), result);
    }

    free(data);
}

static uint32_t register_scanner(scanner_t *scanner)
{
    int i;

    if (!scanner)
        return BT_SCAN_STATUS_START_FAIL;

    if (scanner_manager.scanner_cnt == CONFIG_OBELISK_LE_SCANNER_MAX_NUM) {
        delete_scanner(scanner);
        return BT_SCAN_STATUS_SCANNER_REG_NOMEM;
    }

    for (i = 0; i < CONFIG_OBELISK_LE_SCANNER_MAX_NUM; i++) {
        if (scanner_manager.scanner_list[i] != NULL &&
            scanner_compare(scanner_manager.scanner_list[i], scanner)) {
            delete_scanner(scanner);
            return BT_SCAN_STATUS_SCANNER_EXISTED;
        }
    }

    for (i = 0; i < CONFIG_OBELISK_LE_SCANNER_MAX_NUM; i++) {
        if (!scanner_manager.scanner_list[i]) {
            scanner->scanner_id = i;
            scanner_manager.scanner_list[i] = scanner;
            scanner_manager.scanner_cnt++;
            return BT_SCAN_STATUS_SUCCESS;
        }
    }

    return BT_SCAN_STATUS_START_FAIL;
}

static void unregister_scanner(void *data)
{
    scanner_ctrl_t *stop = data;
    scanner_t *scanner = stop->scanner;

    free(data);
    if (!scanner)
        return;

    if (!scanner_is_registered(scanner))
        return;

    stop_scan((void *)scanner);
    scanner->callbacks->on_scan_stopped(get_remote(scanner));
    scanner_manager.scanner_list[scanner->scanner_id] = NULL;
    scanner_manager.scanner_cnt--;
    delete_scanner(scanner);
}

static void cleanup_scanner(void *data)
{
    for (int i = 0; i < CONFIG_OBELISK_LE_SCANNER_MAX_NUM; i++) {
        scanner_t *scanner = scanner_manager.scanner_list[i];
        if (scanner)
            unregister_scanner(scanner);
    }

    list_delete(&scanner_manager.scanning_list);
}

static int setup_scan_parameter(ble_scan_settings_t *settings, ble_scan_params_t *param)
{
    if (!settings || !param)
        return BT_SCAN_STATUS_START_FAIL;

    param->scan_phy = settings->scan_phy;

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

static void start_scan(void *data)
{
    scanner_ctrl_t *start = data;
    scanner_t *scanner = start->scanner;
    ble_scan_params_t params = { 100, 100, BT_LE_1M_PHY };

    uint32_t status = register_scanner(scanner);
    if (status != BT_SCAN_STATUS_SUCCESS) {
        scanner->callbacks->on_scan_start_status(get_remote(scanner), status);
        goto ret;
    }

    if (start->use_setting) {
        setup_scan_parameter(&start->settings, &params);
    }

    if (!scanner_manager.is_scanning && !list_length(&scanner_manager.scanning_list)) {
        bt_sal_le_set_scan_parameters(&params);
        if (bt_sal_le_start_scan() != BT_STATUS_SUCCESS) {
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

static void stop_scan(void *data)
{
    scanner_t *scanner = (scanner_t *)data;

    if (!scanner_is_registered(scanner))
        return;

    if (!scanner->is_scanning)
        return;

    list_delete(&scanner->scanning_node);
    scanner->is_scanning = false;
    if (scanner_manager.is_scanning && !list_length(&scanner_manager.scanning_list)) {
        bt_sal_le_stop_scan();
        scanner_manager.is_scanning = false;
    }
}

void scan_on_state_changed(uint8_t state)
{
    BT_LOGD("%s, state:%d", __func__, state);
}

void scan_on_result_data_update(ble_scan_result_t *result_info, char *adv_data)
{
    ble_scan_result_t *result = malloc(sizeof(ble_scan_result_t) + result_info->length);

    if (!result)
        return;

    /* TODO : gdb debug check */
    memcpy(result, result_info, sizeof(ble_scan_result_t));
    memcpy(result->adv_data, adv_data, result_info->length);

    do_in_service_loop(notify_scanners_scan_result, result);
}

bt_scanner_t *scanner_start_scan(void *remote, const scanner_callbacks_t *cbs)
{
    scanner_t *scanner = alloc_new_scanner(remote, cbs);
    if (!scanner)
        return NULL;

    scanner_ctrl_t *start = malloc(sizeof(scanner_ctrl_t));
    if (start == NULL) {
        free(scanner);
        return NULL;
    }

    start->scanner = scanner;
    start->use_setting = false;

    do_in_service_loop(start_scan, (void *)start);

    return (bt_scanner_t *)scanner;
}

bt_scanner_t *scanner_start_scan_settings(void *remote,
                                          ble_scan_settings_t *settings,
                                          const scanner_callbacks_t *cbs)
{
    scanner_t *scanner = alloc_new_scanner(remote, cbs);
    if (!scanner)
        return NULL;

    scanner_ctrl_t *start = malloc(sizeof(scanner_ctrl_t));
    if (start == NULL) {
        free(scanner);
        return NULL;
    }

    start->scanner = scanner;
    start->use_setting = true;
    memcpy(&start->settings, settings, sizeof(*settings));

    do_in_service_loop(start_scan, (void *)start);

    return (bt_scanner_t *)scanner;
}

void scanner_stop_scan(bt_scanner_t *scanner)
{
    scanner_ctrl_t *stop = malloc(sizeof(scanner_ctrl_t));
    if (stop == NULL)
        return;

    stop->scanner = (scanner_t *)scanner;
    do_in_service_loop(unregister_scanner, (void *)stop);
}

bool scan_is_supported(void)
{
#ifdef CONFIG_OBELISK_LE_SCAN_ENABLE
    return true;
#endif
    return false;
}

void scan_manager_init(void)
{
    memset(&scanner_manager, 0, sizeof(scanner_manager));
    list_initialize(&scanner_manager.scanning_list);
}

void scan_manager_cleanup(void)
{
    do_in_service_loop(cleanup_scanner, NULL);
}

void scanner_dump(bt_scanner_t *scanner)
{
}
