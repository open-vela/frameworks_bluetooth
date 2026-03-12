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
#ifndef __ADVERTISING_INTERNAL_H__
#define __ADVERTISING_INTERNAL_H__

#include "advertising.h"
#include "bt_list.h"
#include "index_allocator.h"
#include "service_loop.h"

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

adv_manager_t* adv_manager_get_interface(void);

#endif /* __ADVERTISING_INTERNAL_H__ */
