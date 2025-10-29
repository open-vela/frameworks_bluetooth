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

#include "pbap_pce_blacklist.h"
#include "storage.h"
#include <stdlib.h>

#define PBAP_PCE_BLACKLIST_INIT_SIZE 16

typedef struct {
    bool is_valid;
    bt_address_t addr;
} pbap_pce_blacklist_block;

typedef struct {
    int count;
    int max_count;
    pbap_pce_blacklist_block* list;
} pbap_pce_blacklist_t;

pbap_pce_blacklist_t g_pbap_pce_blacklist = { 0 };

static bt_status_t expand_blacklist(int n)
{
    pbap_pce_blacklist_block* new_list;
    int new_max_count = g_pbap_pce_blacklist.max_count + n;

    new_list = (pbap_pce_blacklist_block*)realloc(g_pbap_pce_blacklist.list,
        new_max_count * sizeof(pbap_pce_blacklist_block));
    if (new_list == NULL) {
        return BT_STATUS_NOMEM;
    }

    g_pbap_pce_blacklist.list = new_list;
    g_pbap_pce_blacklist.max_count = new_max_count;

    return BT_STATUS_SUCCESS;
}

bt_status_t pbap_pce_blacklist_init(void)
{
    bt_status_t status;

    if (g_pbap_pce_blacklist.list != NULL) {
        return BT_STATUS_SUCCESS;
    }

    status = expand_blacklist(PBAP_PCE_BLACKLIST_INIT_SIZE);

    if (status != BT_STATUS_SUCCESS)
        return status;

    status = bt_storage_load_pbap_pce_blacklist(pbap_pce_add_item_to_blacklist);

    if (status != BT_STATUS_SUCCESS)
        return status;

    return BT_STATUS_SUCCESS;
}

void pbap_pce_blacklist_deinit(void)
{
    g_pbap_pce_blacklist.count = 0;
    g_pbap_pce_blacklist.max_count = 0;
    free(g_pbap_pce_blacklist.list);
    g_pbap_pce_blacklist.list = NULL;
}

static int find_blacklist_slot()
{
    int i;

    for (i = 0; i < g_pbap_pce_blacklist.max_count; i++) {
        if (g_pbap_pce_blacklist.list[i].is_valid == false) {
            return i;
        }
    }

    return -1;
}

static int find_blacklist_slot_by_addr(bt_address_t* addr)
{
    int i;

    for (i = 0; i < g_pbap_pce_blacklist.max_count; i++) {
        if (g_pbap_pce_blacklist.list[i].is_valid == true
            && memcmp(&g_pbap_pce_blacklist.list[i].addr, addr, sizeof(bt_address_t)) == 0) {
            return i;
        }
    }

    return -1;
}

bt_status_t pbap_pce_add_item_to_blacklist(bt_address_t* addr)
{
    int slot;
    int status;

    if (g_pbap_pce_blacklist.count >= g_pbap_pce_blacklist.max_count) {
        status = expand_blacklist(1);

        if (status != BT_STATUS_SUCCESS)
            return status;

        slot = g_pbap_pce_blacklist.count;
    } else {
        slot = find_blacklist_slot();
    }

    assert(slot >= 0);

    g_pbap_pce_blacklist.list[slot].is_valid = true;
    memcpy(&g_pbap_pce_blacklist.list[slot].addr, addr, sizeof(bt_address_t));
    g_pbap_pce_blacklist.count++;

    return BT_STATUS_SUCCESS;
}

bt_status_t pbap_pce_add_to_blacklist(bt_address_t* addr)
{
    int slot;
    int status = BT_STATUS_SUCCESS;

    status = pbap_pce_add_item_to_blacklist(addr);

    if (status != BT_STATUS_SUCCESS)
        return status;

    status = bt_storage_save_pbap_pce_blacklist_item(addr);

    if (status != BT_STATUS_SUCCESS) {
        slot = find_blacklist_slot_by_addr(addr);
        g_pbap_pce_blacklist.list[slot].is_valid = false;
    }

    return status;
}

bt_status_t pbap_pce_remove_from_blacklist(bt_address_t* addr)
{
    int slot;
    int status;

    slot = find_blacklist_slot_by_addr(addr);

    if (slot < 0) {
        return BT_STATUS_FAIL;
    }

    g_pbap_pce_blacklist.list[slot].is_valid = false;
    g_pbap_pce_blacklist.count--;

    status = bt_storage_delete_pbap_pce_blacklist_item(addr);

    return status;
}

bool pbap_pce_is_in_blacklist(bt_address_t* addr)
{
    int slot;

    slot = find_blacklist_slot_by_addr(addr);

    if (slot < 0) {
        return false;
    }

    return true;
}