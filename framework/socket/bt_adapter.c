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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bt_adapter.h"
#include "bt_socket.h"

void *bt_adapter_register_callback(bt_instance_t *ins, const adapter_callbacks_t *adapter_cbs)
{
  bt_message_packet_t packet;
  bt_status_t status;
  void *handle;

  if (ins->adapter_callbacks != NULL)
    {
      return NULL;
    }

  ins->adapter_callbacks = bt_callbacks_list_new(2);

  handle = bt_remote_callbacks_register(ins->adapter_callbacks, NULL, (void *)adapter_cbs);
  if (handle == NULL) {
    bt_callbacks_list_free(ins->adapter_callbacks);
    return handle;
  }

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_REGISTER_CALLBACK);
  if (status != BT_STATUS_SUCCESS || packet.adpt_r.status != BT_STATUS_SUCCESS)
    {
      bt_callbacks_list_free(ins->adapter_callbacks);
      return NULL;
    }

  return handle;
}

bool bt_adapter_unregister_callback(bt_instance_t *ins, void *cookie)
{
  bt_message_packet_t packet;
  bt_status_t status;

  if (!ins->adapter_callbacks)
      return false;

  bt_remote_callbacks_unregister(ins->adapter_callbacks, NULL, cookie);
  bt_callbacks_list_free(ins->adapter_callbacks);
  ins->adapter_callbacks = NULL;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_UNREGISTER_CALLBACK);
  if (status != BT_STATUS_SUCCESS || packet.adpt_r.status != BT_STATUS_SUCCESS)
    {
      return false;
    }

  return true;
}

bt_status_t bt_adapter_enable(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_ENABLE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_disable(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_DISABLE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_enable_le(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_ENABLE_LE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_disable_le(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_DISABLE_LE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_adapter_state_t bt_adapter_get_state(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_STATE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.state;
}

bool bt_adapter_is_le_enabled(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_IS_LE_ENABLED);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.bbool;
}

bt_device_type_t bt_adapter_get_type(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_TYPE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.dtype;
}

bt_status_t bt_adapter_set_discovery_filter(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_DISCOVERY_FILTER);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_start_discovery(bt_instance_t *ins, uint32_t timeout)
{
  bt_message_packet_t packet;
  bt_status_t status;

  packet.adpt_pl._bt_adapter_start_discovery.v32 = timeout;
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_START_DISCOVERY);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_cancel_discovery(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_CANCEL_DISCOVERY);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bool bt_adapter_is_discovering(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_IS_DISCOVERING);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.bbool;
}

void bt_adapter_get_address(bt_instance_t *ins, bt_address_t *addr)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_ADDRESS);
  if (status != BT_STATUS_SUCCESS)
    {
      memset(addr, 0, sizeof(*addr));
    }

  memcpy(addr, &packet.adpt_pl._bt_adapter_get_address.addr, sizeof(*addr));
}

bt_status_t bt_adapter_set_name(bt_instance_t *ins, const char *name)
{
  bt_message_packet_t packet;
  bt_status_t status;

  if (strlen(name) > sizeof(packet.adpt_pl._bt_adapter_set_name.name))
    {
      return BT_STATUS_PARM_INVALID;
    }

  strncpy(packet.adpt_pl._bt_adapter_set_name.name, name, sizeof(packet.adpt_pl._bt_adapter_set_name.name));
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_NAME);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

void bt_adapter_get_name(bt_instance_t *ins, char *name, int length)
{
  bt_message_packet_t packet;
  bt_status_t status;

  if (length < sizeof(packet.adpt_pl._bt_adapter_get_name.name))
    {
      return;
    }
    
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_NAME);
  if (status != BT_STATUS_SUCCESS)
    {
      return;
    }

  strncpy(name, packet.adpt_pl._bt_adapter_get_name.name, 64);
}

bt_status_t bt_adapter_get_uuids(bt_instance_t *ins, bt_uuid_t *uuids, uint16_t *size)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_UUIDS);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  *size = packet.adpt_pl._bt_adapter_get_uuids.size;

  if (*size > 0)
    memcpy(uuids, packet.adpt_pl._bt_adapter_get_uuids.uuids, sizeof(bt_uuid_t) * *size);

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_set_scan_mode(bt_instance_t *ins, bt_scan_mode_t mode, bool bondable)
{
  bt_message_packet_t packet;
  bt_status_t status;

  packet.adpt_pl._bt_adapter_set_scan_mode.mode = mode;
  packet.adpt_pl._bt_adapter_set_scan_mode.bondable = bondable;
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_SCAN_MODE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_scan_mode_t bt_adapter_get_scan_mode(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_SCAN_MODE);
  if (status != BT_STATUS_SUCCESS)
    {
      return BT_BR_SCAN_MODE_NONE;
    }

  return packet.adpt_r.mode;
}

bt_status_t bt_adapter_set_device_class(bt_instance_t *ins, uint32_t cod)
{
  bt_message_packet_t packet;
  bt_status_t status;

  packet.adpt_pl._bt_adapter_set_device_class.v32 = cod;
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_DEVICE_CLASS);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

uint32_t bt_adapter_get_device_class(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_DEVICE_CLASS);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.v32;
}

bt_status_t bt_adapter_set_io_capability(bt_instance_t *ins, bt_io_capability_t cap)
{
  bt_message_packet_t packet;
  bt_status_t status;

  packet.adpt_pl._bt_adapter_set_io_capability.cap = cap;
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_IO_CAPABILITY);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_io_capability_t bt_adapter_get_io_capability(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_IO_CAPABILITY);
  if (status != BT_STATUS_SUCCESS)
    {
      return BT_IO_CAPABILITY_DISPLAYONLY;
    }

  return packet.adpt_r.ioc;
}

bt_status_t bt_adapter_set_le_io_capability(bt_instance_t *ins, uint32_t le_io_cap)
{
  bt_message_packet_t packet;
  bt_status_t status;

  packet.adpt_pl._bt_adapter_set_le_io_capability.v32 = le_io_cap;
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_LE_IO_CAPABILITY);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

uint32_t bt_adapter_get_le_io_capability(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_LE_IO_CAPABILITY);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.v32;
}

bt_status_t bt_adapter_get_le_address(bt_instance_t *ins, bt_address_t *addr, ble_addr_type_t *type)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_LE_ADDRESS);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  if (packet.adpt_r.status == BT_STATUS_SUCCESS)
    {
      *type = packet.adpt_pl._bt_adapter_get_le_address.type;
      memcpy(addr, &packet.adpt_pl._bt_adapter_get_le_address.addr, sizeof(*addr));
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_set_le_address(bt_instance_t *ins, bt_address_t *addr)
{
  bt_message_packet_t packet;
  bt_status_t status;

  memcpy(&packet.adpt_pl._bt_adapter_set_le_address.addr, addr, sizeof(*addr));
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_LE_ADDRESS);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_set_le_identity_address(bt_instance_t *ins, bt_address_t *addr, bool public)
{
  bt_message_packet_t packet;
  bt_status_t status;

  memcpy(&packet.adpt_pl._bt_adapter_set_le_identity_address.addr, addr, sizeof(*addr));
  packet.adpt_pl._bt_adapter_set_le_identity_address.public = public;
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_LE_IDENTITY_ADDRESS);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_set_le_appearance(bt_instance_t *ins, uint16_t appearance)
{
  bt_message_packet_t packet;
  bt_status_t status;

  packet.adpt_pl._bt_adapter_set_le_appearance.v16 = appearance;
  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_SET_LE_APPEARANCE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.status;
}

uint16_t bt_adapter_get_le_appearance(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_LE_APPEARANCE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.v16;
}

bt_status_t bt_adapter_get_bonded_devices(bt_instance_t *ins, bt_address_t **addr, int *num, bt_allocator_t allocator)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_BONDED_DEVICES);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  *num = packet.adpt_pl._bt_adapter_get_bonded_devices.num;

  if (*num > 0)
    {
      *addr = malloc(sizeof(bt_address_t) * *num);
      if (*addr == NULL)
        return BT_STATUS_NOMEM;
      memcpy(*addr, packet.adpt_pl._bt_adapter_get_bonded_devices.addr,
          sizeof(bt_address_t) * *num);
    }

  return packet.adpt_r.status;
}

bt_status_t bt_adapter_get_connected_devices(bt_instance_t *ins, bt_address_t **addr, int *num, bt_allocator_t allocator)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_GET_CONNECTED_DEVICES);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  *num = packet.adpt_pl._bt_adapter_get_connected_devices.num;

  if (*num > 0)
    {
      *addr = malloc(sizeof(bt_address_t) * *num);
      if (*addr == NULL)
        return BT_STATUS_NOMEM;
      memcpy(*addr, packet.adpt_pl._bt_adapter_get_connected_devices.addr,
          sizeof(bt_address_t) * *num);
    }

  return packet.adpt_r.status;
}

void bt_adapter_disconnect_all_devices(bt_instance_t *ins)
{
  bt_message_packet_t packet;

  bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_DISCONNECT_ALL_DEVICES);
}

bool bt_adapter_is_support_bredr(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_IS_SUPPORT_BREDR);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.bbool;
}

bool bt_adapter_is_support_le(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_IS_SUPPORT_LE);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.bbool;
}

bool bt_adapter_is_support_leaudio(bt_instance_t *ins)
{
  bt_message_packet_t packet;
  bt_status_t status;

  status = bt_socket_client_sendrecv(ins, &packet, BT_ADAPTER_IS_SUPPORT_LEAUDIO);
  if (status != BT_STATUS_SUCCESS)
    {
      return status;
    }

  return packet.adpt_r.bbool;
}
