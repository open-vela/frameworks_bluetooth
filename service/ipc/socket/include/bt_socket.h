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
#ifndef _BT_SOCKET_H__
#define _BT_SOCKET_H__

#include "bluetooth.h"
#include "bt_message.h"

#define BT_SOCKET_INS_VALID(ins, ret) \
    do {                              \
        if (ins == NULL)              \
            return ret;               \
    } while (0)

#define BT_SOCKET_PTR_VALID(cb, ret) \
    do {                            \
        if (cb == NULL)             \
            return ret;             \
    } while (0)

/* Macros for number of items.
 * (aka. ARRAY_SIZE, ArraySize, Size of an Array)
 */

#ifndef nitems
#define nitems(_a) (sizeof(_a) / sizeof(0 [(_a)]))
#endif /* nitems */

typedef struct {
    void* user_data;
    uv_loop_t* loop;
    uv_connect_t conn_req;

    uv_pipe_t* pipe;
    bt_instance_t* ins;
    bt_ipc_connected_cb_t connected;
    bt_ipc_disconnected_cb_t disconnected;

    bt_message_packet_t* packet;
    uint32_t offset;

    bt_list_t* pending_queue;
    callbacks_list_t* adapter_callbacks;
    callbacks_list_t* a2dp_sink_callbacks;
    callbacks_list_t* a2dp_source_callbacks;
    callbacks_list_t* avrcp_target_callbacks;
    callbacks_list_t* avrcp_control_callbacks;
    callbacks_list_t* hfp_ag_callbacks;
    callbacks_list_t* hfp_hf_callbacks;
    callbacks_list_t* panu_callbacks;
    callbacks_list_t* spp_callbacks;
    callbacks_list_t* hidd_callbacks;
    callbacks_list_t* l2cap_callbacks;
    callbacks_list_t* cs_callbacks;

    bt_list_t* gattc_remote_list;
    bt_list_t* gatts_remote_list;

} bt_socket_async_client_t;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BLUETOOTH_SOCKADDR_NAME "bt:%s"
#define BLUETOOTH_SERVER_MAXCONN 10

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/* Client */
typedef void (*bt_socket_reply_cb_t)(bt_instance_t* ins, bt_message_packet_t* packet, void* cb, void* userdata);
int bt_socket_async_client_init(bt_instance_t* ins, uv_loop_t* loop, int family,
    const char* name, const char* cpu, int port, bt_ipc_connected_cb_t connected,
    bt_ipc_disconnected_cb_t disconnected, void* user_data);
void bt_socket_async_client_deinit(bt_instance_t* ins);

int bt_socket_client_init(bt_instance_t* ins, int family,
    const char* name, const char* cpu,
    int port);

void bt_socket_client_deinit(bt_instance_t* ins);

void bt_socket_client_free_callbacks(bt_instance_t* ins, callbacks_list_t* cbsl);

int bt_socket_client_sendrecv(bt_instance_t* ins,
    bt_message_packet_t* packet,
    uint32_t code);

int bt_socket_client_send_with_reply(bt_instance_t* ins, bt_message_packet_t* packet,
    bt_message_type_t code, bt_socket_reply_cb_t reply, void* cb, void* userdata);
/* Server */

int bt_socket_server_init(const char* name, int port);

int bt_socket_server_send(bt_instance_t* ins, bt_message_packet_t* packet,
    uint32_t code);

bool bt_socket_server_is_busy(void);

/* Manager */
void bt_socket_server_manager_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_manager_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* Adapter */

void bt_socket_server_adapter_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_adapter_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* Device */

void bt_socket_server_device_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

/*A2DP Source*/
void bt_socket_server_a2dp_source_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_a2dp_source_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);
/*A2DP Sink*/
void bt_socket_server_a2dp_sink_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_a2dp_sink_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* AVRCP Target */

void bt_socket_server_avrcp_target_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_avrcp_target_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* AVRCP Control */
void bt_socket_server_avrcp_control_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_avrcp_control_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* HFP */
void bt_socket_server_hfp_ag_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_hfp_ag_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

void bt_socket_server_hfp_hf_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_hfp_hf_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* Advertiser */

void bt_socket_server_advertiser_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_advertiser_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);
/* Scan */

void bt_socket_server_scan_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_scan_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);
/* Gatt client */

void bt_socket_server_gattc_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_gattc_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);
/* Gatt server */

void bt_socket_server_gatts_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_gatts_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);
/* Spp */

void bt_socket_server_spp_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_spp_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);
/* Pan */

void bt_socket_server_pan_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_pan_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* HID device */

void bt_socket_server_hid_device_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_hid_device_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

/* L2CAP */

void bt_socket_server_l2cap_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

int bt_socket_client_l2cap_callback(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet, bool is_async);

void bt_socket_server_log_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);

/* Channel Sounding */
void bt_socket_server_cs_process(service_poll_t* poll,
    int fd, bt_instance_t* ins, bt_message_packet_t* packet);
#ifdef __cplusplus
}
#endif

#ifdef CONFIG_NET_SOCKOPTS
void setSocketBuf(int fd, int option);
#endif

#endif /* _BT_SOCKET_H__ */
