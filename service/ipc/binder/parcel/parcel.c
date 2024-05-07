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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parcel.h"

static bool AParcelUtils_nameAllocator(void* stringData, int32_t length, char** buffer)
{
    return true;
}

static bool AParcelUtils_stringNoAlloc(void* stringData, int32_t length, char** buffer)
{
    if (length == -1 || buffer == NULL)
        return true;

    *buffer = stringData;

    return true;
}

/**
 * @brief Address parcel
 *
 * @param arrayData
 * @param length
 * @param outBuffer
 * @return true
 * @return false
 */

static bool AParcelUtils_addressAllocator(void* arrayData, int32_t length, int8_t** outBuffer)
{
    assert(length == BT_ADDR_LENGTH);
    *outBuffer = arrayData;

    return true;
}

binder_status_t AParcel_writeAddress(AParcel* parcel, bt_address_t* addr)
{
    return AParcel_writeByteArray(parcel, (const int8_t*)addr->addr, BT_ADDR_LENGTH);
}

binder_status_t AParcel_readAddress(const AParcel* parcel, bt_address_t* addr)
{
    return AParcel_readByteArray(parcel, addr->addr, AParcelUtils_addressAllocator);
}

static binder_status_t AParcel_writeParcelableAddress(AParcel* parcel, const void* arrayData,
    size_t index)
{
    bt_address_t* addr = (bt_address_t*)arrayData + index;

    return AParcel_writeAddress(parcel, addr);
}

binder_status_t AParcel_writeAddressArray(AParcel* parcel, bt_address_t* addr, int32_t length)
{
    binder_status_t stat = AParcel_writeInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    return AParcel_writeParcelableArray(parcel, (void*)addr, length, AParcel_writeParcelableAddress);
}

static binder_status_t AParcel_readParcelableAddress(const AParcel* parcel, void* arrayData,
    size_t index)
{
    bt_address_t* addr = (bt_address_t*)arrayData + index;

    return AParcel_readAddress(parcel, addr);
}

static bool AParcel_parcelableAddressAllocator(void* arrayData, int32_t length)
{
    return true;
}

binder_status_t AParcel_readAddressArray(const AParcel* parcel, bt_address_t** addr, int32_t* length, bt_allocator_t allocator)
{
    binder_status_t stat = AParcel_readInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    if (*length == 0)
        return STATUS_OK;

    if (!allocator((void**)addr, sizeof(bt_address_t) * (*length)))
        return STATUS_NO_MEMORY;

    return AParcel_readParcelableArray(parcel, (void*)*addr,
        AParcel_parcelableAddressAllocator,
        AParcel_readParcelableAddress);
}

binder_status_t AParcel_readName(AParcel* parcel, char* name)
{
    return AParcel_readString(parcel, name, AParcelUtils_nameAllocator);
}

/**
 * @brief UUID parcel
 *
 * @param arrayData
 * @param length
 * @param outBuffer
 * @return true
 * @return false
 */
static bool AParcelUtils_uuidAllocator(void* arrayData, int32_t length, int8_t** outBuffer)
{
    assert(length == 16);
    *outBuffer = arrayData;

    return true;
}

binder_status_t AParcel_writeUuid(AParcel* parcel, bt_uuid_t* uuid)
{
    binder_status_t stat;

    if (uuid == NULL) {
        stat = AParcel_writeUint32(parcel, 0);
        return stat;
    }

    stat = AParcel_writeUint32(parcel, uuid->type);
    if (stat != STATUS_OK)
        return stat;

    if (uuid->type == BT_UUID16_TYPE)
        stat = AParcel_writeUint32(parcel, (uint32_t)uuid->val.u16);
    else if (uuid->type == BT_UUID32_TYPE)
        stat = AParcel_writeUint32(parcel, uuid->val.u32);
    else if (uuid->type == BT_UUID128_TYPE)
        stat = AParcel_writeByteArray(parcel, (const int8_t*)uuid->val.u128, 16);
    else
        stat = STATUS_BAD_TYPE;

    return stat;
}

binder_status_t AParcel_readUuid(const AParcel* parcel, bt_uuid_t* uuid)
{
    binder_status_t stat;
    uint32_t uuid32;

    stat = AParcel_readUint32(parcel, &uuid->type);
    if (stat != STATUS_OK)
        return stat;

    if (uuid->type == 0) {
        stat = STATUS_OK;
    } else if (uuid->type == BT_UUID16_TYPE) {
        stat = AParcel_readUint32(parcel, &uuid32);
        uuid->val.u16 = uuid32;
    } else if (uuid->type == BT_UUID32_TYPE) {
        stat = AParcel_readUint32(parcel, &uuid32);
        uuid->val.u32 = uuid32;
    } else if (uuid->type == BT_UUID128_TYPE)
        stat = AParcel_readByteArray(parcel, uuid->val.u128, AParcelUtils_uuidAllocator);
    else
        stat = STATUS_BAD_TYPE;

    return stat;
}

static binder_status_t AParcel_writeParcelableUuid(AParcel* parcel, const void* arrayData,
    size_t index)
{
    bt_uuid_t* uuid = (bt_uuid_t*)arrayData + index;

    return AParcel_writeUuid(parcel, uuid);
}

binder_status_t AParcel_writeUuidArray(AParcel* parcel, bt_uuid_t* uuid, int32_t length)
{
    binder_status_t stat = AParcel_writeInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    return AParcel_writeParcelableArray(parcel, (void*)uuid, length, AParcel_writeParcelableUuid);
}

static binder_status_t AParcel_readParcelableUuid(const AParcel* parcel, void* arrayData,
    size_t index)
{
    bt_uuid_t* uuid = *(bt_uuid_t**)arrayData + index;

    return AParcel_readUuid(parcel, uuid);
}

static bool AParcel_parcelableUuidAllocator(void* arrayData, int32_t length)
{
    char* p = malloc(sizeof(bt_uuid_t) * length);
    *(char**)arrayData = p;

    return true;
}

binder_status_t AParcel_readUuidArray(const AParcel* parcel, bt_uuid_t* uuid, int32_t* length)
{
    binder_status_t stat = AParcel_readInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    return AParcel_readParcelableArray(parcel, (void*)uuid,
        AParcel_parcelableUuidAllocator,
        AParcel_readParcelableUuid);
}

/**
 * @brief Ble connect param parcel
 *
 * @param parcel
 * @param param
 * @return binder_status_t
 */
binder_status_t AParcel_writeBleConnectParam(AParcel* parcel, ble_connect_params_t* param)
{
    binder_status_t stat = STATUS_OK;

    stat = AParcel_writeUint32(parcel, param->filter_policy);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeBool(parcel, param->use_default_params);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, param->init_phy);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->scan_interval);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->scan_window);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->connection_interval_min);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->connection_interval_max);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->connection_latency);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->supervision_timeout);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->min_ce_length);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)param->max_ce_length);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

binder_status_t AParcel_readBleConnectParam(const AParcel* parcel, ble_connect_params_t* param)
{
    binder_status_t stat = STATUS_OK;
    uint32_t u32Val;

    stat = AParcel_readUint32(parcel, &param->filter_policy);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readBool(parcel, &param->use_default_params);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &param->init_phy);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->scan_interval = (uint16_t)u32Val;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->scan_window = (uint16_t)u32Val;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->connection_interval_min = (uint16_t)u32Val;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->connection_interval_max = (uint16_t)u32Val;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->connection_latency = (uint16_t)u32Val;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->supervision_timeout = (uint16_t)u32Val;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->min_ce_length = (uint16_t)u32Val;

    stat = AParcel_readUint32(parcel, &u32Val);
    if (stat != STATUS_OK)
        return stat;
    param->max_ce_length = (uint16_t)u32Val;

    return stat;
}

binder_status_t AParcel_writeCall(AParcel* parcel, hfp_current_call_t* call)
{
    binder_status_t stat = STATUS_OK;

    stat = AParcel_writeInt32(parcel, call->index);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, call->dir);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, call->state);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, call->mpty);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeString(parcel, call->number, strlen(call->number));
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeString(parcel, call->name, call->name ? strlen(call->name) : -1);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

binder_status_t AParcel_readCall(const AParcel* parcel, hfp_current_call_t* call)
{
    binder_status_t stat = STATUS_OK;

    stat = AParcel_readInt32(parcel, &call->index);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &call->dir);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &call->state);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &call->mpty);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readString(parcel, (void*)call->number, AParcelUtils_stringNoAlloc);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readString(parcel, (void*)call->name, AParcelUtils_stringNoAlloc);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

static binder_status_t AParcel_writeParcelableCall(AParcel* parcel, const void* arrayData,
    size_t index)
{
    hfp_current_call_t* call = (hfp_current_call_t*)arrayData + index;

    return AParcel_writeCall(parcel, call);
}

binder_status_t AParcel_writeCallArray(AParcel* parcel, hfp_current_call_t* calls, int32_t length)
{
    binder_status_t stat = AParcel_writeInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    return AParcel_writeParcelableArray(parcel, (void*)calls, length, AParcel_writeParcelableCall);
}

static binder_status_t AParcel_readParcelableCall(const AParcel* parcel, void* arrayData,
    size_t index)
{
    hfp_current_call_t* call = (hfp_current_call_t*)arrayData + index;

    return AParcel_readCall(parcel, call);
}

static bool AParcel_parcelableCallAllocator(void* arrayData, int32_t length)
{
    return true;
}

binder_status_t AParcel_readCallArray(const AParcel* parcel, hfp_current_call_t** calls, int32_t* length, bt_allocator_t allocator)
{
    binder_status_t stat = AParcel_readInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    if (*length == 0)
        return STATUS_OK;

    if (!allocator((void**)calls, sizeof(hfp_current_call_t) * (*length)))
        return STATUS_NO_MEMORY;

    return AParcel_readParcelableArray(parcel, (void*)*calls,
        AParcel_parcelableCallAllocator,
        AParcel_readParcelableCall);
}

/**
 * @brief Ble adv param parcel
 *
 * @param parcel
 * @param param
 * @return binder_status_t
 */

binder_status_t AParcel_writeBleAdvParam(AParcel* parcel, ble_adv_params_t* param)
{
    binder_status_t stat = STATUS_OK;

    stat = AParcel_writeUint32(parcel, param->adv_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeAddress(parcel, &param->peer_addr);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, param->peer_addr_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeAddress(parcel, &param->own_addr);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, param->own_addr_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, param->interval);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeByte(parcel, param->tx_power);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, param->channel_map);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, param->filter_policy);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, param->duration);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

binder_status_t AParcel_readBleAdvParam(const AParcel* parcel, ble_adv_params_t* param)
{
    binder_status_t stat = STATUS_OK;

    stat = AParcel_readUint32(parcel, &param->adv_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readAddress(parcel, &param->peer_addr);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &param->peer_addr_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readAddress(parcel, &param->own_addr);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &param->own_addr_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &param->interval);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readByte(parcel, &param->tx_power);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &param->channel_map);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &param->filter_policy);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &param->duration);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

/*
bt_address_t addr;
    bt_device_type_t dev_type;
    int8_t rssi;
    ble_addr_type_t addr_type;
    ble_adv_type_t adv_type;
    uint8_t length;
    char adv_data[1];
*/

static bool AParcelUtils_advDataAllocator(void* arrayData, int32_t length, int8_t** outBuffer)
{
    assert(length <= 0xFF);
    *outBuffer = arrayData;

    return true;
}

binder_status_t AParcel_writeBleScanResult(AParcel* parcel, ble_scan_result_t* result)
{
    binder_status_t stat = STATUS_OK;

    stat = AParcel_writeUint32(parcel, result->length);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeByteArray(parcel, (const int8_t*)result->adv_data, result->length);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, result->adv_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, result->dev_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, result->addr_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeAddress(parcel, &result->addr);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeByte(parcel, result->rssi);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

binder_status_t AParcel_readBleScanResult(const AParcel* parcel, ble_scan_result_t** outResult)
{
    binder_status_t stat = STATUS_OK;
    uint32_t length = 0;
    ble_scan_result_t* result;

    stat = AParcel_readUint32(parcel, &length);
    if (stat != STATUS_OK)
        return stat;

    result = malloc(sizeof(ble_scan_result_t) + length);
    if (!result)
        return STATUS_NO_MEMORY;

    result->length = length;
    stat = AParcel_readByteArray(parcel, result->adv_data, AParcelUtils_advDataAllocator);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &result->adv_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &result->dev_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &result->addr_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readAddress(parcel, &result->addr);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readByte(parcel, &result->rssi);
    if (stat != STATUS_OK)
        return stat;

    *outResult = result;

    return stat;
}

static binder_status_t AParcel_writeAttribute(AParcel* parcel, gatt_attr_db_t* attribute)
{
    binder_status_t stat;

    stat = AParcel_writeByte(parcel, attribute->handle);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUuid(parcel, attribute->uuid);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, attribute->type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, attribute->properties);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, attribute->permissions);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, attribute->rsp_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)attribute->read_cb);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, (uint32_t)attribute->write_cb);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeByteArray(parcel, (const int8_t*)attribute->attr_value, attribute->attr_length);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_writeUint32(parcel, attribute->attr_length);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

static binder_status_t AParcel_writeParcelableAttribute(AParcel* parcel, const void* arrayData,
    size_t index)
{
    gatt_attr_db_t* attribute = (gatt_attr_db_t*)arrayData + index;

    return AParcel_writeAttribute(parcel, attribute);
}

binder_status_t AParcel_writeServiceTable(AParcel* parcel, gatt_attr_db_t* attribute, int32_t length)
{
    binder_status_t stat = AParcel_writeInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    return AParcel_writeParcelableArray(parcel, (void*)attribute, length, AParcel_writeParcelableAttribute);
}

static binder_status_t AParcel_readAttribute(const AParcel* parcel, gatt_attr_db_t* attribute)
{
    binder_status_t stat = STATUS_OK;

    stat = AParcel_readByte(parcel, (int8_t*)&attribute->handle);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUuid(parcel, attribute->uuid);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &attribute->type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &attribute->properties);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &attribute->permissions);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &attribute->rsp_type);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, (uint32_t*)&attribute->read_cb);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, (uint32_t*)&attribute->write_cb);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readByteArray(parcel, (void*)&attribute->attr_value, AParcelUtils_byteArrayAllocator);
    if (stat != STATUS_OK)
        return stat;

    stat = AParcel_readUint32(parcel, &attribute->attr_length);
    if (stat != STATUS_OK)
        return stat;

    return stat;
}

static binder_status_t AParcel_readParcelableAttribute(const AParcel* parcel, void* arrayData,
    size_t index)
{
    gatt_attr_db_t* attribute = (gatt_attr_db_t*)arrayData + index;

    attribute->uuid = malloc(sizeof(bt_uuid_t));
    if (!attribute->uuid)
        return STATUS_NO_MEMORY;

    return AParcel_readAttribute(parcel, attribute);
}

static bool AParcel_parcelableAttributeAllocator(void* arrayData, int32_t length)
{
    return true;
}

binder_status_t AParcel_readServiceTable(const AParcel* parcel, gatt_attr_db_t** attribute, int32_t* length)
{
    binder_status_t stat = AParcel_readInt32(parcel, length);
    if (stat != STATUS_OK)
        return stat;

    if (*length == 0)
        return STATUS_OK;

    *attribute = malloc(sizeof(gatt_attr_db_t) * (*length));
    if (!(*attribute))
        return STATUS_NO_MEMORY;

    memset(*attribute, 0, sizeof(gatt_attr_db_t) * (*length));
    return AParcel_readParcelableArray(parcel, (void*)*attribute,
        AParcel_parcelableAttributeAllocator,
        AParcel_readParcelableAttribute);
}
