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
#define LOG_TAG "adapter_debug"

#include "adapter_debug.h"

#include "adapter_internel.h"
#include "bt_addr.h"
#include "bt_device.h"
#include "device.h"
#include "utils/log.h"

void adapter_dump_whitelist(void)
{
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
    bt_list_t* list = adapter_get_le_device_list();
    bt_list_node_t* node;
    int cnt = 0;
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };

    BT_LOGD("%s", __func__);

    for (node = bt_list_head(list); node != NULL; node = bt_list_next(list, node)) {
        bt_device_t* device = bt_list_node(node);
        if (device_check_flag(device, DFLAG_WHITELIST_ADDED)) {
            bt_addr_ba2str(device_get_address(device), addr_str);
            BT_LOGD("\twhitelist[%d] addr:%s, addr_type:%d", cnt++, addr_str,
                device_get_address_type(device));
        }
    }

    BT_LOGD("whitelist dump end, cnt = %d", cnt);
#endif
}
