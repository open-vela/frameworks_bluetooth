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
#ifndef __BT_SAL_H__
#define __BT_SAL_H__

#include "bluetooth_define.h"
#include "utils/log.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

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

#define SAL_ASSERT_PARAM(cond) \
    {                          \
        assert(cond);          \
    }

void bt_sal_debug_init(void);
void bt_sal_debug_cleanup(void);
bt_status_t bt_sal_debug_enable(void);
bt_status_t bt_sal_debug_disable(void);
bt_status_t bt_sal_debug_set_log_level(uint32_t level);
bool bt_sal_debug_is_type_support(bt_debug_type_t type);
bt_status_t bt_sal_debug_set_log_enable(bt_debug_type_t type, bool enable);
bt_status_t bt_sal_debug_update_log_mask(int mask);

#endif /* __BT_SAL_H__ */
