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
#define LOG_TAG "gattc"

#include <stdint.h>

#include "bt_gattc.h"
#include "bt_profile.h"
#include "gattc_service.h"
#include "service_manager.h"
#include "utils/log.h"

bt_status_t bt_gattc_create_connect(bt_instance_t *ins, gattc_handle_t *phandle, gattc_callbacks_t *callbacks)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_delete_connect(gattc_handle_t conn_handle)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_connect(gattc_handle_t conn_handle, bt_address_t *addr, ble_addr_type_t addr_type)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_disconnect(gattc_handle_t conn_handle)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_discover_service(gattc_handle_t conn_handle, bt_uuid_t *filter_uuid)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_get_attribute_by_handle(gattc_handle_t conn_handle, uint16_t attr_handle, gatt_attr_desc_t *attr_desc)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_get_attribute_by_uuid(gattc_handle_t conn_handle, bt_uuid_t *attr_uuid, gatt_attr_desc_t *attr_desc)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_read(gattc_handle_t conn_handle, uint16_t attr_handle)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_write(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_write_without_response(gattc_handle_t conn_handle, uint16_t attr_handle, uint8_t *value, uint16_t length)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_subscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle, gattc_notify_cb_t notify_cb)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_unsubscribe(gattc_handle_t conn_handle, uint16_t value_handle, uint16_t cccd_handle)
{
  return BT_STATUS_SUCCESS;
}

bt_status_t bt_gattc_exchange_mtu(gattc_handle_t conn_handle, uint32_t mtu)
{
  return BT_STATUS_SUCCESS;
}
