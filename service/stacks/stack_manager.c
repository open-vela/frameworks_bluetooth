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
#include "sal_interface.h"
#include "service_loop.h"

#define LOG_TAG "stack_manager"
#include "utils/log.h"
#include "vhal/bt_vhal.h"

bt_status_t stack_manager_init(void)
{
    bt_status_t ret;
    const bt_vhal_interface* vhal;
    bt_stack_info_t info;

    vhal = get_bt_vhal_interface();

    bt_sal_get_stack_info(&info);
    BT_LOGI("Stack Info: %s Ver:%d.%d Sal:%d", info.name,
        info.stack_ver_major, info.stack_ver_minor, info.sal_ver);
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    ret = bt_sal_init(vhal);
    if (ret != BT_STATUS_SUCCESS)
        return ret;
#endif

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    ret = bt_sal_le_init(vhal);
    if (ret != BT_STATUS_SUCCESS)
        return ret;
#endif
    BT_LOGD("%s done", __func__);
    return ret;
}

void stack_manager_cleanup(void)
{
#ifdef CONFIG_BLUETOOTH_BREDR_SUPPORT
    bt_sal_cleanup();
#endif

#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    bt_sal_le_cleanup();
#endif
    BT_LOGD("%s done", __func__);
}