/*
 * Copyright (C) 2024 Xiaomi Corporation. All rights reserved.
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
 *
 */

#include "feature_bluetooth.h"
#include "feature_log.h"

void feature_bluetooth_deal_callback(int status, void* data){
    callback_info_t* info = (callback_info_t*)data;
    FEATURE_LOG_INFO("callback type:%d, feature:%p, callback id: %d", info->callback_id, info->feature, info->feature_callback_id);
    if (!FeatureInvokeCallback(info->feature,
            info->feature_callback_id, info->data)) {
        FEATURE_LOG_ERROR("callback type:%d, feature:%p, callback id: %d, invoke discoveryresult callback failed!",
        info->callback_id, info->feature, info->feature_callback_id);
    }
    free(data);
}

char* StringToFtString(const char* str)
{
    int len = strlen(str);
    char* ftStr = (char*)FeatureMalloc(len + 1, FT_CHAR);
    strcpy(ftStr, str);
    return ftStr;
}