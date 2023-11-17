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

#ifndef __BT_PBAP_H__
#define __BT_PBAP_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_addr.h"
#include "bt_device.h"
#include <stddef.h>

/* According to Generic Object Exchange Profile (GOEP) v2.1.1:
   The GOEP implementations shall support a minimum Maximum OBEX Packet Length (MOPL)
   size of 255 Bytes. */
#define PBAP_PKT_LEN_MAX 512

/**
 * @brief PCE SearchProperty header to indicate to the Server with vCard property
 *        the search operation shall be carrid out on.
 */
typedef enum {
    PBAP_SEARCH_PROPERTY_NAME = 0,
    PBAP_SEARCH_PROPERTY_NUMBER = 1,
    PBAP_SEARCH_PROPERTY_SOUND = 2,

    PCE_SEARCH_PROPERTY_NONE = 0xFF
} pbap_search_property_t;

#ifdef __cplusplus
}
#endif

#endif /* __BT_PBAP_H__ */
