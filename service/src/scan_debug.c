/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#define LOG_TAG "scan_debug"

#include "scan_debug.h"

#include "bt_time.h"
#include "bt_utils.h"
#include "service_loop.h"
#include "utils/log.h"

#define SCAN_DUMP_INTERVAL_MS (60 * 1000)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static service_timer_t* scan_result_dump_timer = NULL;

static const char* scan_type_to_string(ble_scan_type_t type)
{
    switch (type) {
        CASE_RETURN_STR(BT_LE_SCAN_TYPE_PASSIVE)
        CASE_RETURN_STR(BT_LE_SCAN_TYPE_ACTIVE)
        DEFAULT_BREAK()
    }

    return "UNKNOWN";
}

/** TODO: this shall be a bit-wise value, and LE 2M are not allowed on primary channel */
static const char* scan_phy_to_string(ble_phy_type_t scan_phy)
{
    switch (scan_phy) {
        CASE_RETURN_STR(BT_LE_1M_PHY)
        CASE_RETURN_STR(BT_LE_2M_PHY)
        CASE_RETURN_STR(BT_LE_CODED_PHY)
        DEFAULT_BREAK()
    }

    return "UNKNOWN";
}

void scan_dump_params(const ble_scan_params_t* params)
{
    char interval_str[BT_SLOTS_TO_TIME_STR_LENGTH] = { 0 };
    char window_str[BT_SLOTS_TO_TIME_STR_LENGTH] = { 0 };

    if (!params)
        return;

    BT_LOGD("%s:\n"
            "\tinterval: %d slots (%s), window: %d slots (%s), type: %s(%d) phy: %s(%d)",
        __func__, params->scan_interval,
        bt_slots_to_time_str(interval_str, BT_SLOTS_TO_TIME_STR_LENGTH, params->scan_interval),
        params->scan_window,
        bt_slots_to_time_str(window_str, BT_SLOTS_TO_TIME_STR_LENGTH, params->scan_window),
        scan_type_to_string(params->scan_type), params->scan_type,
        scan_phy_to_string(params->scan_phy), params->scan_phy);
}

void scan_dump_scanners(void)
{
    scanner_manager_t* manager = scan_manager_get_interface();
    uint8_t cnt = 0;

    if (!manager)
        return;

    BT_LOGD("%s, %d scanners:", __func__, manager->scanner_cnt);

    for (int i = 0; i < CONFIG_BLUETOOTH_LE_SCANNER_MAX_NUM; i++) {
        scanner_t* scanner = manager->scanner_list[i];
        if (!scanner)
            continue;

        BT_LOGD("\tscanner[%d]: index[%d], state:[%s], ",
            cnt++, i, scanner->is_scanning ? "scanning" : "idle");

        if (BT_LE_SCAN_FILTER_POLICY(scanner->policy.policy, FILTERED)
            == BT_LE_SCAN_FILTERED_SCANNING_POLICY) {
            if (BT_LE_SCAN_FILTER_POLICY(scanner->policy.policy, EXTENDED)
                == BT_LE_SCAN_EXTENDED_FILTER_POLICY) {
                BT_LOGD("\t\tfiltered (extended)");
            } else {
                BT_LOGD("\t\tfiltered (basic)");
            }

            switch (BT_LE_SCAN_FILTER_POLICY(scanner->policy.policy, DECISION)) {
            case BT_LE_SCAN_ALL_PDUS_MODE:
                BT_LOGD("\t\tall-pdus mode");
                break;
            case BT_LE_SCAN_DECISIONS_ONLY_MODE:
                BT_LOGD("\t\tdecisions-only mode");
                break;
            default:
                break;
            }
        } else {
            BT_LOGD("\t\tunfiltered");
        }

        BT_LOGD("\t\tscan results since last dump(low power / balanced / low latency): "
                "%" PRIu32 " / %" PRIu32 " / %" PRIu32,
            scanner->result_cnt[BT_SCAN_MODE_LOW_POWER], scanner->result_cnt[BT_SCAN_MODE_BALANCED],
            scanner->result_cnt[BT_SCAN_MODE_LOW_LATENCY]);
        memset(scanner->result_cnt, 0x00, sizeof(scanner->result_cnt));
    }
}

void scan_update_statistics(scanner_t* scanner)
{
    scanner_manager_t* manager = scan_manager_get_interface();

    if (!manager || !scanner)
        return;

    if (manager->curr_scan_mode >= ARRAY_SIZE(scanner->result_cnt)) {
        BT_LOGW("Invalid scan mode %d", manager->curr_scan_mode);
        return;
    }

    scanner->result_cnt[manager->curr_scan_mode]++;
}

static void dump_scan_statistic(service_timer_t* timer, void* userdata)
{
    scan_dump_scanners();
}

void scan_debug_timer_start(void)
{
    if (scan_result_dump_timer == NULL) {
        scan_result_dump_timer = service_loop_timer(SCAN_DUMP_INTERVAL_MS,
            SCAN_DUMP_INTERVAL_MS, dump_scan_statistic, NULL);
    }
}

void scan_debug_timer_stop(void)
{
    if (scan_result_dump_timer) {
        dump_scan_statistic(NULL, NULL);
        service_loop_cancel_timer(scan_result_dump_timer);
        scan_result_dump_timer = NULL;
    }
}
