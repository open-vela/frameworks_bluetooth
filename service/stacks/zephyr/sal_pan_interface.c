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

#include <stdint.h>

#include "bt_addr.h"
#include "bt_pan.h"
#include "bt_status.h"
#include "sal_pan_interface.h"
#include "utils/log.h"

/* PAN profile requires BNEP which is not yet implemented in zblue stack.
 * These stubs allow compilation but return NOT_SUPPORTED at runtime. */

bt_status_t bt_sal_pan_init(uint8_t max_connections, pan_role_t role)
{
    BT_LOGW("PAN SAL: not implemented (BNEP not available in zblue)");
    return BT_STATUS_NOT_SUPPORTED;
}

void bt_sal_pan_cleanup(void)
{
}

bt_status_t bt_sal_pan_connect(bt_address_t* addr, pan_role_t dst_role, pan_role_t src_role)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_pan_disconnect(bt_address_t* addr)
{
    return BT_STATUS_NOT_SUPPORTED;
}

bt_status_t bt_sal_pan_write(bt_address_t* addr, uint16_t protocol,
    const uint8_t* dst_addr, const uint8_t* src_addr,
    const uint8_t* data, uint16_t length)
{
    return BT_STATUS_NOT_SUPPORTED;
}
