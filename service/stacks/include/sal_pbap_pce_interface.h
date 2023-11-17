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
#ifndef __SAL_PBAP_PCE_INTERFACE_H__
#define __SAL_PBAP_PCE_INTERFACE_H__

#include <stdint.h>

#include "bt_addr.h"
#include "bt_status.h"
#include "pbap_pce_service.h"

bt_status_t bt_sal_pce_init(void);
void bt_sal_pce_cleanup(void);
bt_status_t bt_sal_pce_connect(bt_address_t* addr);
bt_status_t bt_sal_pce_disconnect(bt_address_t* addr);
bt_status_t bt_sal_pce_change_directory(bt_address_t* addr, const char* dir);
bt_status_t bt_sal_pce_pull_vcard_listing(bt_address_t* addr, pbap_search_property_t property, const char* value);
bt_status_t bt_sal_pce_pull_vcard(bt_address_t* addr, const char* object, uint64_t filter);

#endif /* __SAL_PBAP_PCE_INTERFACE_H__ */
