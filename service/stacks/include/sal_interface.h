/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __SAL_ADAPTER_H_
#define __SAL_ADAPTER_H_

#include "bluetooth_define.h"

#include "sal_adapter_classic_interface.h"
#ifdef CONFIG_BLUETOOTH_BLE_SUPPORT
#include "sal_adapter_le_interface.h"
#ifdef CONFIG_BLUETOOTH_BLE_ADV
#include "sal_le_advertise_interface.h"
#endif
#ifdef CONFIG_BLUETOOTH_BLE_SCAN
#include "sal_le_scan_interface.h"
#endif
#endif
#include "sal_debug_interface.h"

#if defined(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET) || defined(CONFIG_BLUETOOTH_STACK_LE_BLUELET)
#include "sal_adapter_interface.h"
#endif

typedef struct bt_stack_info {
    char name[32];
    uint8_t stack_ver_major;
    uint8_t stack_ver_minor;
    uint8_t sal_ver;
    /* data */
} bt_stack_info_t;

#define SAL_ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define SAL_CHECK(cond, expect)                         \
    {                                                   \
        int __ret = cond;                               \
        if (__ret != expect) {                          \
            BT_LOGE("[%s] return:%d", __func__, __ret); \
        }                                               \
    }

#define SAL_NOT_SUPPORT                                    \
    {                                                      \
        BT_LOGW("interface [%s] not supported", __func__); \
        return BT_STATUS_NOT_SUPPORTED;                    \
    }

#define SAL_CHECK_PARAM(cond)              \
    {                                      \
        if (!(cond))                       \
            return BT_STATUS_PARM_INVALID; \
    }

#define SAL_CHECK_RET(cond, expect)                     \
    {                                                   \
        int __ret = cond;                               \
        if (__ret != expect) {                          \
            BT_LOGE("[%s] return:%d", __func__, __ret); \
            return BT_STATUS_FAIL;                      \
        }                                               \
    }

#define SAL_ASSERT(cond) \
    {                    \
        assert(cond);    \
    }

void bt_sal_get_stack_info(bt_stack_info_t* info);

#endif /* __SAL_ADAPTER_V2_H_ */
