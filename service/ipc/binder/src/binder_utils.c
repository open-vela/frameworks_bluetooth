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

#include "binder_utils.h"

bool AParcelUtils_btCommonAllocator(void** data, uint32_t size)
{
    *data = malloc(size);
    if (!(*data))
        return false;

    return true;
}

bool AParcelUtils_stringAllocator(void* stringData, int32_t length, char** buffer)
{
    if (length == -1 || buffer == NULL)
        return true;

    char* p = malloc(length);
    *(char**)stringData = p;
    *buffer = p;

    return true;
}

bool AParcelUtils_stringArrayAllocator(void* arrayData, int32_t length)
{
    return true;
}
bool AParcelUtils_stringArrayElementAllocator(void* arrayData, size_t index, int32_t length,
    char** buffer)
{
    return true;
}
const char* AParcelUtils_stringArrayElementGetter(const void* arrayData, size_t index,
    int32_t* outLength)
{
    return NULL;
}
bool AParcelUtils_parcelableArrayAllocator(void* arrayData, int32_t length)
{
    return true;
}
binder_status_t AParcelUtils_writeParcelableElement(AParcel* parcel, const void* arrayData,
    size_t index)
{
    return STATUS_OK;
}
binder_status_t AParcelUtils_readParcelableElement(const AParcel* parcel, void* arrayData,
    size_t index)
{
    return STATUS_OK;
}
bool AParcelUtils_int32ArrayAllocator(void* arrayData, int32_t length, int32_t** outBuffer)
{
    return true;
}
bool AParcelUtils_uint32ArrayAllocator(void* arrayData, int32_t length, uint32_t** outBuffer)
{
    return true;
}
bool AParcelUtils_int64ArrayAllocator(void* arrayData, int32_t length, int64_t** outBuffer)
{
    return true;
}
bool AParcelUtils_uint64ArrayAllocator(void* arrayData, int32_t length, uint64_t** outBuffer)
{
    return true;
}
bool AParcelUtils_floatArrayAllocator(void* arrayData, int32_t length, float** outBuffer)
{
    return true;
}
bool AParcelUtils_doubleArrayAllocator(void* arrayData, int32_t length, double** outBuffer)
{
    return true;
}
bool AParcelUtils_boolArrayAllocator(void* arrayData, int32_t length)
{
    return true;
}
bool AParcelUtils_boolArrayGetter(const void* arrayData, size_t index)
{
    return true;
}
void AParcelUtils_boolArraySetter(void* arrayData, size_t index, bool value)
{
    return;
}
bool AParcelUtils_charArrayAllocator(void* arrayData, int32_t length, char16_t** outBuffer)
{
    return true;
}
bool AParcelUtils_byteArrayAllocator(void* arrayData, int32_t length, int8_t** outBuffer)
{
    if (length == -1 || outBuffer == NULL)
        return true;

    int8_t* p = malloc(length);
    *(int8_t**)arrayData = p;
    *outBuffer = p;

    return true;
}
