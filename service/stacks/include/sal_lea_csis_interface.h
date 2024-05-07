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
#ifndef __SAL_LEA_CSIS_INTERFACE_H__
#define __SAL_LEA_CSIS_INTERFACE_H__

#include <stdint.h>

#include "stack_adapter_gap.h"
#include "stack_adapter_lea_gaf.h"

#include "bt_addr.h"
#include "bt_status.h"

bool adpt_req_csis_info_callback(SERVICE_LEA_CSIS_S* info);

#endif /* __SAL_LEA_CSIP_INTERFACE_H__ */