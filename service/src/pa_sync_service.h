/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#ifndef __PA_SYNC_SERVICE_H__
#define __PA_SYNC_SERVICE_H__

#include "bt_status.h"

bt_status_t pa_sync_init(void);
bt_status_t pa_sync_cleanup(void);
bt_status_t pa_sync_create(void);
bt_status_t pa_sync_terminate(void);

#endif /* __PA_SYNC_SERVICE_H__ */