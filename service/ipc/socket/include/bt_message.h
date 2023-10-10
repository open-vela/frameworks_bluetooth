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
#ifndef _BT_MESSAGE_H__
#define _BT_MESSAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bluetooth.h"
#include "bt_message_manager.h"
#include "bt_message_adapter.h"
#include "bt_message_device.h"
#include "service_loop.h"

typedef enum {
#define __BT_MESSAGE_CODE__
#include "bt_message_manager.h"
#include "bt_message_adapter.h"
#include "bt_message_device.h"
#undef __BT_MESSAGE_CODE__
} bt_message_type_t;

typedef struct
{
  bt_message_type_t code;
  union {
    bt_manager_result_t manager_r;
    bt_adapter_result_t adpt_r;
    bt_device_result_t  devs_r;
  };
  union {
    bt_message_manager_t manager_pl;

    bt_message_adapter_t adpt_pl;
    bt_message_adapter_callbacks_t adpt_cb;

    bt_message_device_t devs_pl;
  };
} bt_message_packet_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_H__ */
