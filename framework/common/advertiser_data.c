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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bt_addr.h"
#include "bt_debug.h"
#include "bt_list.h"
#include "bt_utils.h"

#include "advertiser_data.h"

typedef struct advertiser_data_ {
    bt_list_t* data;
    uint8_t* buffer;
} advertiser_data_t;

static void advertiser_data_calc_len(void* data, void* context)
{
    adv_data_t* adata = data;

    *(uint16_t*)context += adata->len + 1;
}

advertiser_data_t* advertiser_data_new(void)
{
    advertiser_data_t* ad = malloc(sizeof(advertiser_data_t));
    if (!ad)
        return NULL;

    ad->data = bt_list_new((bt_list_free_cb_t)free);
    ad->buffer = NULL;

    return ad;
}

void advertiser_data_free(advertiser_data_t* ad)
{
    if (ad->buffer)
        free(ad->buffer);

    bt_list_free(ad->data);
    free(ad);
}

uint8_t* advertiser_data_build(advertiser_data_t* ad, uint16_t* len)
{
    uint16_t total_len = 0;
    bt_list_node_t* node;
    uint8_t* p;

    if (ad->buffer)
        free(ad->buffer);

    if (!bt_list_length(ad->data))
        return NULL;

    bt_list_foreach(ad->data, advertiser_data_calc_len, &total_len);

    *len = total_len;
    ad->buffer = malloc(total_len);
    p = ad->buffer;

    for (node = bt_list_head(ad->data); node != NULL;
         node = bt_list_next(ad->data, node)) {
        adv_data_t* adata = bt_list_node(node);
        memcpy(p, adata, adata->len + 1);
        p += adata->len + 1;
    }

    return ad->buffer;
}

void advertiser_data_set_name(advertiser_data_t* ad, const char* name)
{
    uint8_t name_len = strlen(name);
    adv_data_t* data = zalloc(sizeof(adv_data_t) + name_len + 1);

    if (name_len > BT_LE_AD_NAME_LEN) {
        name_len = BT_LE_AD_NAME_LEN;
        data->type = BT_AD_NAME_SHORT;
    } else
        data->type = BT_AD_NAME_COMPLETE;

    data->len = name_len + 1;
    memcpy(data->data, name, name_len);

    bt_list_add_tail(ad->data, data);
}

void advertiser_data_set_flags(advertiser_data_t* ad, uint8_t flags)
{
    adv_data_t* data = malloc(sizeof(adv_data_t) + 1);

    data->len = 2;
    data->type = BT_AD_FLAGS;
    data->data[0] = flags;

    bt_list_add_tail(ad->data, data);
}

void advertiser_data_set_appearance(advertiser_data_t* ad, uint16_t appearance)
{
    adv_data_t* data = malloc(sizeof(adv_data_t) + 2);
    uint8_t* p = data->data;

    data->len = 3;
    data->type = BT_AD_GAP_APPEARANCE;
    UINT16_TO_STREAM(p, appearance);

    bt_list_add_tail(ad->data, data);
}

void advertiser_data_add_data(advertiser_data_t* ad, uint8_t type, uint8_t* data, uint8_t len)
{
    adv_data_t* adata = malloc(sizeof(adv_data_t) + len);

    adata->type = type;
    adata->len = len;
    memcpy(adata->data, data, len);

    bt_list_add_tail(ad->data, adata);
}

void advertiser_data_remove_data(advertiser_data_t* ad, uint8_t type, uint8_t* data, uint8_t len)
{
}

void advertiser_data_add_manufacture_data(advertiser_data_t* ad,
    uint16_t manufacture_id,
    uint8_t* data, uint8_t length)
{
    adv_data_t* mdata = malloc(sizeof(adv_data_t) + 2 + length);
    uint8_t* p = mdata->data;

    mdata->len = length + 2 + 1;
    mdata->type = BT_AD_MANUFACTURER_DATA;
    UINT16_TO_STREAM(p, manufacture_id);
    memcpy(p, data, length);

    bt_list_add_tail(ad->data, mdata);
}

bool advertiser_data_add_service_uuid(advertiser_data_t* ad, const bt_uuid_t* uuid)
{
    adv_data_t* data;
    uint8_t* p;

    switch (uuid->type) {
    case BT_UUID16_TYPE:
        data = malloc(sizeof(adv_data_t) + 2);
        data->len = 2 + 1;
        data->type = BT_AD_UUID16_ALL;
        p = data->data;
        UINT16_TO_STREAM(p, uuid->val.u16);
        break;
    case BT_UUID32_TYPE:
        data = malloc(sizeof(adv_data_t) + 4);
        data->len = 4 + 1;
        data->type = BT_AD_UUID32_ALL;
        p = data->data;
        UINT32_TO_STREAM(p, uuid->val.u32);
    case BT_UUID128_TYPE:
        data = malloc(sizeof(adv_data_t) + 16);
        data->len = 16 + 1;
        data->type = BT_AD_UUID128_ALL;
        memcpy(data->data, uuid->val.u128, 16);
        break;
    default:
        return false;
    }

    bt_list_add_tail(ad->data, data);

    return true;
}

bool advertiser_data_add_service_data(advertiser_data_t* ad,
    const bt_uuid_t* uuid,
    uint8_t* data, uint8_t len)
{
    adv_data_t* sdata;
    uint8_t* p;

    switch (uuid->type) {
    case BT_UUID16_TYPE:
        sdata = malloc(sizeof(adv_data_t) + len + 2);
        sdata->len = 2 + 1;
        sdata->type = BT_AD_SERVICE_DATA16;
        p = sdata->data;
        UINT16_TO_STREAM(p, uuid->val.u16);
        break;
    case BT_UUID32_TYPE:
        sdata = malloc(sizeof(adv_data_t) + len + 4);
        sdata->len = 4 + 1;
        sdata->type = BT_AD_SERVICE_DATA32;
        p = sdata->data;
        UINT32_TO_STREAM(p, uuid->val.u32);
    case BT_UUID128_TYPE:
        sdata = malloc(sizeof(adv_data_t) + len + 16);
        sdata->len = 16 + 1;
        sdata->type = BT_AD_SERVICE_DATA128;
        memcpy(sdata->data, uuid->val.u128, 16);
        p = sdata->data + 16;
        break;
    default:
        return false;
    }

    memcpy(p, data, len);
    bt_list_add_tail(ad->data, sdata);

    return true;
}
