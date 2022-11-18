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

#include "utils/log.h"

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

#define SAL_CHECK_RET(ret, expect)                    \
    {                                                 \
        if (ret != expect) {                          \
            BT_LOGE("[%s] return:%d", __func__, ret); \
            return BT_STATUS_FAIL;                    \
        }                                             \
    }

#define SAL_ASSERT_PARAM(cond) \
    {                          \
        assert(cond);          \
    }

#endif /* __BT_SAL_H__ */