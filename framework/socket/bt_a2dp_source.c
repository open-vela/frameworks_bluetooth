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
#define LOG_TAG "a2dp_source_api"

#include <stdint.h>

#include "a2dp_source_service.h"
#include "bt_a2dp_source.h"
#include "bt_profile.h"
#include "service_manager.h"
#include "utils/log.h"

void *bt_a2dp_source_register_callbacks(bt_instance_t *ins, const a2dp_source_callbacks_t *callbacks)
{
  return NULL;
}

bool bt_a2dp_source_unregister_callbacks(bt_instance_t *ins, void *cookie)
{
  return false;
}

bt_status_t bt_a2dp_source_connect(bt_instance_t *ins, bt_address_t *addr)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_a2dp_source_disconnect(bt_instance_t *ins, bt_address_t *addr)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_a2dp_source_set_silence_device(bt_address_t *addr, bool silence)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_a2dp_source_set_active_device(bt_address_t *addr)
{
  return BT_STATUS_SUCCESS;
}
