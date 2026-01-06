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
#define LOG_TAG "pa_sync"

#include "pa_sync_service.h"

#include "bluetooth.h"

#include "utils/log.h"

typedef struct pa_sync_info {
    int nothing_but_wait_for_the_first_variable;
} pa_sync_info_t;

static pa_sync_info_t* g_pa_sync_info = NULL;

bt_status_t pa_sync_init(void)
{
    BT_LOGD("%s", __func__);

    if (g_pa_sync_info)
        return BT_STATUS_BUSY;

    g_pa_sync_info = zalloc(sizeof(pa_sync_info_t));

    return BT_STATUS_SUCCESS;
}

bt_status_t pa_sync_cleanup(void)
{
    BT_LOGD("%s", __func__);

    if (!g_pa_sync_info)
        return BT_STATUS_DONE;

    free(g_pa_sync_info);
    g_pa_sync_info = NULL;

    return BT_STATUS_SUCCESS;
}

bt_status_t pa_sync_create(void)
{
    BT_LOGD("%s", __func__);

    return BT_STATUS_UNSUPPORTED;
}

bt_status_t pa_sync_terminate(void)
{
    BT_LOGD("%s", __func__);

    return BT_STATUS_UNSUPPORTED;
}
