
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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#ifdef CONFIG_BLUETOOTH_PBAP_PCE
#include "sal_interface.h"
#include "sal_pbap_pce_interface.h"

#include "utils/log.h"

bt_status_t bt_sal_pbap_pce_init(void)
{
    return BT_STATUS_UNSUPPORTED;
}

void bt_sal_pbap_pce_cleanup(void)
{
    return;
}

bt_status_t bt_sal_pbap_pce_connect(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_pbap_pce_disconnect(bt_address_t* addr)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_pbap_pce_change_directory(bt_address_t* addr, const char* dir)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_pbap_pce_pull_vcard_listing(bt_address_t* addr,
    bt_pbap_search_property_t property, const char* value)
{
    return BT_STATUS_UNSUPPORTED;
}

bt_status_t bt_sal_pbap_pce_pull_vcard(bt_address_t* addr, const char* object, uint64_t filter)
{
    return BT_STATUS_UNSUPPORTED;
}
#endif
