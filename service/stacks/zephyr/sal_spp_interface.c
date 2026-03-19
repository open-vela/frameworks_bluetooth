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

#include <debug.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/sys/byteorder.h>

#include "bt_addr.h"
#include "service_loop.h"
#include "spp_service.h"
#include "utils/log.h"

#include "sal_connection_manager.h"
#include "sal_interface.h"
#include "sal_spp_interface.h"
#include "sal_zblue.h"

#define PORT2DLCI(_port, _accept) (_accept ? (_port & 0x3E) : ((_port & 0x3E) + 1))
#define PORT2SCN(_port) ((_port & 0x3E) >> 1)

#define STACK_SVR_PORT(scn) (((scn << 1) & 0x3E) + 1)

#define SAL_SPP_RFCOMM_MFS 990
#define SPP_DEFAULT_CREDITS 10
#define SPP_MFS_EXTRA_SIZE 14
#define SDP_CLIENT_BUF_LEN 512

typedef struct {
    struct bt_rfcomm_server rfcomm_server;
    const char* name;
    bt_uuid_t uuid;
    uint16_t scn;
    struct bt_sdp_record* sdp_record;
} sal_spp_server_t;

typedef struct {
    struct bt_sdp_discover_params sdp_discover;
    uint16_t scn;
    struct bt_uuid_128 uuid_128;
    bool discovered;
} sal_spp_client_t;

typedef struct {
    struct bt_rfcomm_dlc rfcomm_dlc;
    sal_spp_server_t* spp_server;
    sal_spp_client_t* spp_client;
    struct bt_conn* conn;
    bt_address_t addr;
    uint16_t scn;
    uint16_t conn_port;
    bt_uuid_t uuid;
    bt_list_t* tx_list;
    bt_list_t* rx_list;
} sal_spp_connection_t;

typedef struct {
    uint16_t conn_port;
    uint8_t* buf;
} sal_spp_buffer_t;

typedef struct {
    struct bt_sdp_record record;
    struct bt_sdp_attribute* attrs;
    uint8_t uuid128[BT_UUID_SIZE_128];
    uint16_t channel;
    struct bt_sdp_data_elem svclass_id_list[1];
    struct bt_sdp_data_elem proto_desc_list[2];
    struct bt_sdp_data_elem proto_desc_rfcomm[2];
} spp_sdp_record_t;

typedef struct {
    bt_list_t* servers;
    bt_list_t* connections;
    pthread_mutex_t mutex;
} sal_spp_manager_t;

extern struct net_buf_pool sdp_pool;

NET_BUF_POOL_FIXED_DEFINE(rfcomm_tx_pool, SPP_DEFAULT_CREDITS,
    SAL_SPP_RFCOMM_MFS + SPP_MFS_EXTRA_SIZE, CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static struct bt_sdp_attribute spp_attrs_template[] = {
    BT_SDP_NEW_SERVICE,
    BT_SDP_LIST(
        BT_SDP_ATTR_SVCLASS_ID_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
        BT_SDP_DATA_ELEM_LIST(
            {
                BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
                NULL /* uuid128_buf value will be set later */
            }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROTO_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) }, ) },
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) },
                    {
                        BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
                        0 /* spp channel will be set later */
                    }, ) }, )),
    BT_SDP_LIST(
        BT_SDP_ATTR_PROFILE_DESC_LIST,
        BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
        BT_SDP_DATA_ELEM_LIST(
            { BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
                BT_SDP_DATA_ELEM_LIST(
                    { BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
                        BT_SDP_ARRAY_16(BT_SDP_SERIAL_PORT_SVCLASS) },
                    { BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
                        BT_SDP_ARRAY_16(0x0102) }, ) }, )),
    BT_SDP_SERVICE_NAME("Serial Port"),
};

sal_spp_manager_t g_spp_manager = {
    .servers = NULL,
    .connections = NULL,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

static bt_status_t spp_disconnect_handler(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data);

static inline void spp_conn_lock(void)
{
    pthread_mutex_lock(&g_spp_manager.mutex);
}

static inline void spp_conn_unlock(void)
{
    pthread_mutex_unlock(&g_spp_manager.mutex);
}

static sal_spp_connection_t* spp_find_connection_by_scn(const bt_address_t* addr, uint16_t scn)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_connection_t* spp_conn;
    bt_list_node_t* node;

    if (!spp_mgr->connections || !addr) {
        return NULL;
    }

    for (node = bt_list_head(spp_mgr->connections); node != NULL;
         node = bt_list_next(spp_mgr->connections, node)) {
        spp_conn = bt_list_node(node);
        if (spp_conn->scn == scn && bt_addr_compare(&spp_conn->addr, addr) == 0) {
            return spp_conn;
        }
    }

    return NULL;
}

static sal_spp_connection_t* spp_find_connection_by_port(uint16_t conn_port)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_connection_t* spp_conn;
    bt_list_node_t* node;

    if (!spp_mgr->connections) {
        return NULL;
    }

    for (node = bt_list_head(spp_mgr->connections); node != NULL;
         node = bt_list_next(spp_mgr->connections, node)) {
        spp_conn = bt_list_node(node);
        if (spp_conn->conn_port == conn_port) {
            return spp_conn;
        }
    }

    return NULL;
}

static sal_spp_connection_t* spp_find_connection_by_dlc(struct bt_rfcomm_dlc* rfcomm_dlc)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_connection_t* spp_conn;
    bt_list_node_t* node;

    if (!spp_mgr->connections || !rfcomm_dlc) {
        return NULL;
    }

    for (node = bt_list_head(spp_mgr->connections); node != NULL;
         node = bt_list_next(spp_mgr->connections, node)) {
        spp_conn = bt_list_node(node);
        if (&spp_conn->rfcomm_dlc == rfcomm_dlc) {
            return spp_conn;
        }
    }

    return NULL;
}

static sal_spp_connection_t* spp_find_connection_by_sdp_param(struct bt_conn* conn, const struct bt_sdp_discover_params* param)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    bt_list_node_t* node;

    if (!conn) {
        return NULL;
    }

    for (node = bt_list_head(spp_mgr->connections); node != NULL;
         node = bt_list_next(spp_mgr->connections, node)) {
        sal_spp_connection_t* spp_conn;
        sal_spp_client_t* spp_client;

        spp_conn = bt_list_node(node);
        spp_client = spp_conn->spp_client;
        if ((spp_conn->conn == conn) && (spp_client && (&spp_client->sdp_discover == param))) {
            return spp_conn;
        }
    }

    return NULL;
}

static sal_spp_connection_t* spp_find_connection_by_dlci(const bt_address_t* addr, uint16_t dlci)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_connection_t* spp_conn;
    bt_list_node_t* node;

    if (!spp_mgr->connections || !addr) {
        return NULL;
    }

    for (node = bt_list_head(spp_mgr->connections); node != NULL;
         node = bt_list_next(spp_mgr->connections, node)) {
        spp_conn = bt_list_node(node);
        if (spp_conn->rfcomm_dlc.dlci == dlci && bt_addr_compare(&spp_conn->addr, addr) == 0) {
            return spp_conn;
        }
    }

    return NULL;
}

static sal_spp_server_t* spp_find_server_by_scn(uint16_t scn)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_server_t* spp_server;
    bt_list_node_t* node;

    if (!spp_mgr->servers) {
        return NULL;
    }

    for (node = bt_list_head(spp_mgr->servers); node != NULL;
         node = bt_list_next(spp_mgr->servers, node)) {
        spp_server = bt_list_node(node);
        if (spp_server->scn == scn) {
            return spp_server;
        }
    }

    return NULL;
}

struct bt_sdp_record* spp_sdp_create_record(uint16_t channel, bt_uuid_t* uuid)
{
    spp_sdp_record_t* spp_record;
    size_t attrs_count;

    if (!uuid) {
        BT_LOGE("Invalid uuid");
        return NULL;
    }

    spp_record = zalloc(sizeof(spp_sdp_record_t));
    if (!spp_record) {
        BT_LOGE("Failed to allocate memory for SPP SDP value with uuid");
        return NULL;
    }

    spp_record->attrs = zalloc(sizeof(spp_attrs_template));
    if (!spp_record->attrs) {
        BT_LOGE("Failed to allocate memory for SPP SDP attributes with uuid");
        free(spp_record);
        return NULL;
    }

    attrs_count = ARRAY_SIZE(spp_attrs_template);
    memcpy(spp_record->attrs, spp_attrs_template, sizeof(spp_attrs_template));

    memcpy(spp_record->uuid128, uuid->val.u128, BT_UUID_SIZE_128);
    spp_record->channel = channel;

    for (int i = 0; i < attrs_count; i++) {
        if (spp_record->attrs[i].id == BT_SDP_ATTR_SVCLASS_ID_LIST) {
            spp_record->svclass_id_list[0] = (struct bt_sdp_data_elem) {
                BT_SDP_TYPE_SIZE(BT_SDP_UUID128),
                .data = spp_record->uuid128,
            };

            spp_record->attrs[i].val.data = spp_record->svclass_id_list;
        } else if (spp_record->attrs[i].id == BT_SDP_ATTR_PROTO_DESC_LIST) {
            struct bt_sdp_data_elem* list_tmpl = (struct bt_sdp_data_elem*)spp_record->attrs[i].val.data;
            struct bt_sdp_data_elem* rfcomm_tmpl = NULL;

            if (!list_tmpl) {
                BT_LOGE("SPP Descriptor List PROTO_DESC is NULL");
                goto fail;
            }

            spp_record->proto_desc_list[0] = list_tmpl[0];
            spp_record->proto_desc_list[1] = list_tmpl[1];

            rfcomm_tmpl = (struct bt_sdp_data_elem*)spp_record->proto_desc_list[1].data;
            if (!rfcomm_tmpl) {
                BT_LOGE("SPP Descriptor List Channel is NULL");
                goto fail;
            }

            spp_record->proto_desc_rfcomm[0] = rfcomm_tmpl[0];
            spp_record->proto_desc_rfcomm[1] = rfcomm_tmpl[1];
            spp_record->proto_desc_rfcomm[1].data = &spp_record->channel;

            spp_record->proto_desc_list[1].data = spp_record->proto_desc_rfcomm;
            spp_record->attrs[i].val.data = spp_record->proto_desc_list;
        }
    }

    spp_record->record.attr_count = attrs_count;
    spp_record->record.attrs = spp_record->attrs;

    return &spp_record->record;

fail:
    free(spp_record->attrs);
    free(spp_record);
    return NULL;
}

void spp_sdp_remove_record(struct bt_sdp_record* record)
{
    spp_sdp_record_t* spp_record = CONTAINER_OF(record, spp_sdp_record_t, record);

    free(spp_record->attrs);
    free(spp_record);
}

static void spp_rfcomm_connected(struct bt_rfcomm_dlc* rfcomm_dlc)
{
    sal_spp_connection_t* spp_conn;

    BT_LOGD("%s, rfcomm_dlc: %p", __func__, rfcomm_dlc);

    bt_rfcomm_dlc_set_rx_credit_mode(rfcomm_dlc, BT_RFCOMM_RX_CREDIT_MANUAL);

    spp_conn_lock();
    spp_conn = spp_find_connection_by_dlc(rfcomm_dlc);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("SPP connection not found for rfcomm_dlc");
        return;
    }

    spp_on_connection_state_changed(&spp_conn->addr, spp_conn->conn_port, PROFILE_STATE_CONNECTED);
    spp_on_connection_mfs_update(spp_conn->conn_port, rfcomm_dlc->mtu);

    bt_sal_cm_profile_connected_callback(&spp_conn->addr, PROFILE_SPP, spp_conn->conn_port);
    bt_sal_profile_disconnect_register(&spp_conn->addr, PROFILE_SPP, spp_conn->conn_port, PRIMARY_ADAPTER, spp_disconnect_handler, spp_conn);

    spp_conn_unlock();
}

static void spp_disconnected_defer_handler(void* context)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    struct bt_rfcomm_dlc* rfcomm_dlc = (struct bt_rfcomm_dlc*)context;
    sal_spp_connection_t* spp_conn;

    spp_conn_lock();
    spp_conn = spp_find_connection_by_dlc(rfcomm_dlc);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("SPP connection not found for rfcomm_dlc");
        return;
    }

    BT_LOGD("%s, conn_port: %d", __func__, spp_conn->conn_port);
    bt_list_remove(spp_mgr->connections, spp_conn);
    spp_conn_unlock();
}

static void spp_rfcomm_disconnected(struct bt_rfcomm_dlc* rfcomm_dlc)
{
    sal_spp_connection_t* spp_conn;

    BT_LOGD("%s, rfcomm_dlc: %p", __func__, rfcomm_dlc);

    spp_conn_lock();
    spp_conn = spp_find_connection_by_dlc(rfcomm_dlc);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("SPP connection not found for rfcomm_dlc");
        return;
    }

    spp_on_connection_state_changed(&spp_conn->addr, spp_conn->conn_port, PROFILE_STATE_DISCONNECTED);
    bt_sal_cm_profile_disconnected_callback(&spp_conn->addr, PROFILE_SPP, spp_conn->conn_port);

    do_in_service_loop_deffered(spp_disconnected_defer_handler, rfcomm_dlc, false);
    spp_conn_unlock();
}

static void spp_rfcomm_recv(struct bt_rfcomm_dlc* rfcomm_dlc, struct net_buf* buf)
{
    sal_spp_connection_t* spp_conn;

    BT_DUMPBUFFER("SPP RX:", buf->data, buf->len);

    spp_conn_lock();
    spp_conn = spp_find_connection_by_dlc(rfcomm_dlc);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("SPP connection not found for rfcomm_dlc");
        return;
    }

    bt_list_add_tail(spp_conn->rx_list, net_buf_ref(buf));
    spp_on_data_received(&spp_conn->addr, spp_conn->conn_port, buf->data, buf->len);
    spp_conn_unlock();
}

static void spp_rfcomm_sent(struct bt_rfcomm_dlc* rfcomm_dlc, int err)
{
    sal_spp_connection_t* spp_conn;

    if (err < 0) {
        BT_LOGE("Failed to send data on RFCOMM rfcomm_dlc %p, error: %d", rfcomm_dlc, err);
        return;
    }

    spp_conn_lock();
    spp_conn = spp_find_connection_by_dlc(rfcomm_dlc);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("SPP connection not found for rfcomm_dlc");
        return;
    }

    bt_list_remove_node(spp_conn->tx_list, bt_list_head(spp_conn->tx_list));
    spp_conn_unlock();
}

static struct bt_rfcomm_dlc_ops g_rfcomm_ops = {
    .connected = spp_rfcomm_connected,
    .disconnected = spp_rfcomm_disconnected,
    .recv = spp_rfcomm_recv,
    .sent = spp_rfcomm_sent,
};

static void spp_tx_clean(void* data)
{
    sal_spp_buffer_t* tx_buf = (sal_spp_buffer_t*)data;

    if (!tx_buf) {
        return;
    }

    /* Notify SPP service that data has been sent */
    spp_on_data_sent(tx_buf->conn_port, tx_buf->buf, 0, 0);
    free(tx_buf);
}

static void spp_rx_buf_free(void* data)
{
    struct net_buf* nbuf = (struct net_buf*)data;

    if (nbuf) {
        net_buf_unref(nbuf);
    }
}

static void spp_connection_free(void* data)
{
    sal_spp_connection_t* spp_conn = (sal_spp_connection_t*)data;

    if (!data) {
        return;
    }

    if (spp_conn->tx_list) {
        bt_list_free(spp_conn->tx_list);
    }

    if (spp_conn->rx_list) {
        bt_list_free(spp_conn->rx_list);
    }

    if (spp_conn->conn) {
        bt_conn_unref(spp_conn->conn);
        spp_conn->conn = NULL;
    }

    if (spp_conn->spp_client) {
        free(spp_conn->spp_client);
        spp_conn->spp_client = NULL;
    }

    free(spp_conn);
}

static sal_spp_connection_t* spp_connection_new(bt_address_t* addr, uint16_t conn_port, uint16_t scn)
{
    sal_spp_connection_t* spp_conn;

    spp_conn = malloc(sizeof(sal_spp_connection_t));
    if (!spp_conn) {
        BT_LOGE("Failed to allocate memory for SPP connection");
        return NULL;
    }

    memset(spp_conn, 0, sizeof(sal_spp_connection_t));
    spp_conn->scn = scn;
    spp_conn->conn_port = conn_port;
    spp_conn->tx_list = bt_list_new(spp_tx_clean);
    if (!spp_conn->tx_list) {
        BT_LOGE("Failed to create SPP connection transmission list");
        spp_connection_free(spp_conn);
        return NULL;
    }

    spp_conn->rx_list = bt_list_new(spp_rx_buf_free);
    if (!spp_conn->rx_list) {
        BT_LOGE("Failed to create SPP connection reception list");
        spp_connection_free(spp_conn);
        return NULL;
    }

    spp_conn->rfcomm_dlc.ops = &g_rfcomm_ops;
    spp_conn->rfcomm_dlc.mtu = SAL_SPP_RFCOMM_MFS;
    memcpy(&spp_conn->addr, addr, sizeof(bt_address_t));

    return spp_conn;
}

static int spp_rfcomm_accept(struct bt_conn* conn, struct bt_rfcomm_server* server, struct bt_rfcomm_dlc** rfcomm_dlc)
{
    bt_address_t addr;
    bt_status_t status;
    sal_spp_connection_t* spp_conn;
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_server_t* spp_server = CONTAINER_OF(server, sal_spp_server_t, rfcomm_server);

    status = bt_sal_get_remote_address(conn, &addr);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("Failed to get remote address, error: %d", status);
        return -ENXIO;
    }

    spp_conn = spp_connection_new(&addr, 0, server->channel);
    if (!spp_conn) {
        BT_LOGE("Failed to create SPP connection for DLCI %d", server->channel);
        return -ENOMEM;
    }

    spp_conn->spp_server = spp_server;
    spp_conn->conn = conn;
    bt_conn_ref(conn);

    spp_conn_lock();
    bt_list_add_tail(spp_mgr->connections, spp_conn);
    *rfcomm_dlc = &spp_conn->rfcomm_dlc;
    spp_conn_unlock();

    spp_on_server_recieve_connect_request(&addr, STACK_SVR_PORT(server->channel));

    BT_LOGD("RFCOMM DLC accept, dlci: %d", server->channel);

    return 0;
}

bt_status_t bt_sal_spp_init(void)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;

    spp_mgr->servers = bt_list_new(NULL);
    if (!spp_mgr->servers) {
        BT_LOGE("Failed to create SPP servers list");
        return BT_STATUS_NOMEM;
    }

    spp_mgr->connections = bt_list_new(spp_connection_free);
    if (!spp_mgr->connections) {
        BT_LOGE("Failed to create SPP connections list");
        bt_list_free(spp_mgr->servers);
        spp_mgr->servers = NULL;
        return BT_STATUS_NOMEM;
    }

    return BT_STATUS_SUCCESS;
}

void bt_sal_spp_cleanup(void)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    bt_list_t* connections;
    bt_list_t* servers;
    bt_list_node_t* node;

    spp_conn_lock();

    connections = spp_mgr->connections;
    for (node = bt_list_head(connections); node != NULL;
         node = bt_list_next(connections, node)) {
        sal_spp_connection_t* spp_conn = bt_list_node(node);

        bt_rfcomm_dlc_disconnect(&spp_conn->rfcomm_dlc);
    }

    servers = spp_mgr->servers;
    for (node = bt_list_head(servers); node != NULL;
         node = bt_list_next(servers, node)) {
        sal_spp_server_t* spp_server = bt_list_node(node);

        bt_sal_spp_server_stop(STACK_SVR_PORT(spp_server->scn));
    }

    spp_conn_unlock();
}

bt_status_t bt_sal_spp_server_start(uint16_t port, bt_uuid_t* uuid, uint8_t max_connection)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_server_t* server;
    uint16_t scn = PORT2SCN(port);
    int ret;

    if (scn < BT_RFCOMM_CHAN_SPP || scn > 30) {
        BT_LOGE("Invalid port number: %d", port);
        return BT_STATUS_PARM_INVALID;
    }

    server = zalloc(sizeof(sal_spp_server_t));
    if (!server) {
        return BT_STATUS_NOMEM;
    }

    server->scn = scn;
    server->rfcomm_server.channel = scn;
    server->rfcomm_server.accept = spp_rfcomm_accept;
    ret = bt_rfcomm_server_register(&server->rfcomm_server);
    if (ret < 0) {
        BT_LOGE("Failed to register RFCOMM server: %d", ret);
        free(server);
        return BT_STATUS_FAIL;
    }

    server->sdp_record = (struct bt_sdp_record*)spp_sdp_create_record(scn, uuid);
    ret = bt_sdp_register_service(server->sdp_record);
    if (ret < 0) {
        BT_LOGE("Failed to register SDP record: %d", ret);
        bt_rfcomm_server_unregister(&server->rfcomm_server);
        spp_sdp_remove_record(server->sdp_record);
        free(server);
        return BT_STATUS_FAIL;
    }

    bt_list_add_tail(spp_mgr->servers, server);
    BT_LOGD("SDP record registered for RFCOMM server on channel %d", scn);

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_server_stop(uint16_t port)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_server_t* server;
    uint16_t scn = PORT2SCN(port);

    server = spp_find_server_by_scn(scn);
    if (!server) {
        BT_LOGE("No SPP server found for SCN %d", scn);
        return BT_STATUS_PARM_INVALID;
    }

    bt_sdp_unregister_service(server->sdp_record);
    spp_sdp_remove_record((void*)server->sdp_record);

    bt_rfcomm_server_unregister(&server->rfcomm_server);

    bt_list_remove(spp_mgr->servers, server);
    free(server);

    BT_LOGD("SDP record unregistered for RFCOMM server on channel %d", scn);
    return BT_STATUS_SUCCESS;
}

static int spp_connect_with_channel(sal_spp_connection_t* spp_conn, uint16_t scn)
{
    int err;

    if (!spp_conn) {
        BT_LOGE("Invalid parameters: spp_conn is null");
        return BT_STATUS_PARM_INVALID;
    }

    spp_conn->spp_client->scn = scn;

    err = bt_rfcomm_dlc_connect(spp_conn->conn, &spp_conn->rfcomm_dlc, scn);
    if (err < 0) {
        BT_LOGE("Failed to connect RFCOMM DLC: %d", err);
        return err;
    }

    return 0;
}

static uint8_t sdp_discovered_cb(struct bt_conn* conn, struct bt_sdp_client_result* result,
    const struct bt_sdp_discover_params* param)
{
    int err;
    uint8_t ret = BT_SDP_DISCOVER_UUID_STOP;
    uint16_t scn;
    sal_spp_connection_t* spp_conn;

    spp_conn = spp_find_connection_by_sdp_param(conn, param);
    if (!spp_conn) {
        BT_LOGE("SPP connection not found for conn");
        return BT_SDP_DISCOVER_UUID_STOP;
    }

    if (!result->resp_buf) {
        BT_LOGE("SPP SDP discover response buffer is null");
        ret = BT_SDP_DISCOVER_UUID_CONTINUE;
        goto fail;
    }

    err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &scn);
    if (err < 0) {
        BT_LOGE("Failed to get RFCOMM channel from SDP response: %d", err);
        ret = BT_SDP_DISCOVER_UUID_CONTINUE;
        goto fail;
    }

    BT_LOGD("SPP SDP record found: scn=%d", scn);

    err = spp_connect_with_channel(spp_conn, scn);
    if (err < 0) {
        BT_LOGE("SPP connect RFCOMM fail, err:%d", err);
        ret = BT_SDP_DISCOVER_UUID_STOP;
        goto fail;
    }

    spp_conn->spp_client->discovered = true;
    return BT_SDP_DISCOVER_UUID_STOP;

fail:
    spp_conn->spp_client->discovered = false;
    return ret;
}

static void sdp_disconnected_cb(struct bt_conn* conn, const struct bt_sdp_discover_params* param)
{
    sal_spp_connection_t* spp_conn;
    sal_spp_client_t* spp_client;

    spp_conn = spp_find_connection_by_sdp_param(conn, param);
    if (!spp_conn) {
        BT_LOGE("SPP connection not found for conn");
        return;
    }

    BT_LOGD("SPP SDP discover disconnected");
    spp_client = spp_conn->spp_client;
    if (!spp_client) {
        BT_LOGE("SPP client not found for conn");
        return;
    }

    if (spp_client->discovered == false) {
        spp_rfcomm_disconnected(&spp_conn->rfcomm_dlc);
    }
}

static bt_status_t spp_connect_with_uuid(sal_spp_connection_t* spp_conn, bt_uuid_t* uuid)
{
    sal_spp_client_t* spp_client;
    int err;
    bt_uuid_t uuid_128;

    if (!spp_conn || !uuid) {
        BT_LOGE("Invalid parameters: spp_conn=%p, uuid=%p", spp_conn, uuid);
        return BT_STATUS_PARM_INVALID;
    }

    spp_client = spp_conn->spp_client;
    if (!spp_client) {
        BT_LOGE("SPP client not found for conn");
        return BT_STATUS_PARM_INVALID;
    }

    sys_memcpy_swap(uuid_128.val.u128, uuid->val.u128, sizeof(uuid->val.u128));

    err = bt_uuid_create((struct bt_uuid*)&spp_client->uuid_128, uuid_128.val.u128, BT_UUID_SIZE_128);
    if (err < 0) {
        BT_LOGE("Failed to create UUID: %d", err);
        return err;
    }

    spp_client->sdp_discover.func = sdp_discovered_cb;
    spp_client->sdp_discover.disconnected = sdp_disconnected_cb;
    spp_client->sdp_discover.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;
    spp_client->sdp_discover.pool = &sdp_pool;
    spp_client->sdp_discover.uuid = (const struct bt_uuid*)&spp_client->uuid_128;

    err = bt_sdp_discover(spp_conn->conn, &spp_client->sdp_discover);
    if (err < 0) {
        BT_LOGE("Failed to discover service: %d", err);
        return err;
    }

    return 0;
}

static bt_status_t spp_connect_handler(bt_controller_id_t id, bt_address_t* addr, void* user_data)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_connection_t* spp_conn = (sal_spp_connection_t*)user_data;
    struct bt_conn* conn;

    BT_LOGD("Initiating SPP connection to addr:%s", bt_addr_str(addr));
    spp_on_connection_state_changed(addr, spp_conn->conn_port, PROFILE_STATE_CONNECTING);

    conn = bt_conn_lookup_addr_br((bt_addr_t*)addr);
    if (!conn) {
        BT_LOGE("No ACL connection found for address: %s", bt_addr_str(addr));
        goto fail;
    }

    spp_conn->conn = conn;

    if (spp_conn->conn_port & 0x3F) {
        int err;

        err = spp_connect_with_channel(spp_conn, spp_conn->scn);
        if (err < 0) {
            BT_LOGE("Failed to connect with scn: %d", err);
            goto fail;
        }
    } else {
        int err;

        err = spp_connect_with_uuid(spp_conn, &spp_conn->uuid);
        if (err < 0) {
            BT_LOGE("Failed to connect with uuid, err: %d", err);
            goto fail;
        }
    }

    spp_conn_lock();
    bt_list_add_tail(spp_mgr->connections, spp_conn);
    spp_conn_unlock();

    return BT_STATUS_SUCCESS;

fail:
    spp_on_connection_state_changed(addr, spp_conn->conn_port, PROFILE_STATE_DISCONNECTED);
    bt_sal_cm_profile_disconnected_callback(&spp_conn->addr, PROFILE_SPP, spp_conn->conn_port);
    spp_connection_free(spp_conn);
    return BT_STATUS_FAIL;
}

bt_status_t bt_sal_spp_connect(bt_address_t* addr, uint16_t conn_port, bt_uuid_t* uuid)
{
    sal_spp_connection_t* spp_conn;
    uint16_t scn = PORT2SCN(conn_port);
    char addr_str[BT_ADDR_STR_LENGTH] = { 0 };
    char uuid_str[BT_UUID_STR_LENGTH] = { 0 };
    sal_spp_client_t* spp_client;
    bt_status_t status;

    if (!addr || scn > 30) {
        BT_LOGE("Invalid parameters: addr=%p, scn=%d", addr, scn);
        return BT_STATUS_PARM_INVALID;
    }

    bt_addr_ba2str(addr, addr_str);
    bt_uuid_to_string(uuid, uuid_str, BT_UUID_STR_LENGTH);
    BT_LOGD("%s, addr:%s, scn:%d, uuid:%s", __func__, addr_str, scn, uuid_str);

    spp_conn_lock();

    /* check connection exists */
    spp_conn = spp_find_connection_by_port(conn_port);
    if (!spp_conn && (conn_port & 0x3F)) {
        spp_conn = spp_find_connection_by_dlci(addr, PORT2DLCI(conn_port, 0));
    }

    if (spp_conn) {
        spp_conn_unlock();
        BT_LOGE("SPP connection already exists for port %d", conn_port);
        return BT_STATUS_BUSY;
    }

    spp_conn_unlock();

    /* create spp connection object */
    spp_conn = spp_connection_new((bt_address_t*)addr, conn_port, scn);
    if (!spp_conn) {
        BT_LOGE("Failed to allocate memory for SPP connection");
        return BT_STATUS_NOMEM;
    }

    spp_client = zalloc(sizeof(sal_spp_client_t));
    if (!spp_client) {
        spp_connection_free(spp_conn);
        return BT_STATUS_NOMEM;
    }

    spp_conn->spp_client = spp_client;
    memcpy(&spp_conn->uuid, uuid, sizeof(bt_uuid_t));

    status = bt_sal_profile_connect_request(&spp_conn->addr, PROFILE_SPP, spp_conn->conn_port, PRIMARY_ADAPTER, spp_connect_handler, spp_conn);
    if (status != BT_STATUS_SUCCESS) {
        BT_LOGE("Failed to connect SPP, status: %d", status);
        spp_connection_free(spp_conn);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

static bt_status_t spp_disconnect_handler(bt_controller_id_t id, bt_address_t* bd_addr, void* user_data)
{
    struct bt_rfcomm_dlc* rfcomm_dlc = (struct bt_rfcomm_dlc*)user_data;
    sal_spp_connection_t* spp_conn;
    int ret;

    BT_LOGD("%s, rfcomm_dlc: %p", __func__, rfcomm_dlc);

    spp_conn_lock();
    spp_conn = spp_find_connection_by_dlc(rfcomm_dlc);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("SPP connection not found for rfcomm_dlc");
        return BT_STATUS_FAIL;
    }

    spp_conn_unlock();

    /* Disconnect the RFCOMM DLC */
    ret = bt_rfcomm_dlc_disconnect(&spp_conn->rfcomm_dlc);
    if (ret < 0) {
        BT_LOGE("Failed to disconnect RFCOMM DLC: %d", ret);
        return BT_STATUS_FAIL;
    }

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_disconnect(uint16_t conn_port)
{
    sal_spp_connection_t* spp_conn;

    spp_conn_lock();
    spp_conn = spp_find_connection_by_port(conn_port);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("No SPP connection found for port %d", conn_port);
        return BT_STATUS_PARM_INVALID;
    }

    spp_conn_unlock();

    return bt_sal_profile_disconnect_request(&spp_conn->addr, PROFILE_SPP, spp_conn->conn_port, PRIMARY_ADAPTER, spp_disconnect_handler, &spp_conn->rfcomm_dlc);
}

bt_status_t bt_sal_spp_data_received_response(uint16_t conn_port, uint8_t* buf)
{
    sal_spp_connection_t* spp_conn;
    bt_list_node_t* node;

    spp_conn_lock();

    spp_conn = spp_find_connection_by_port(conn_port);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("No SPP connection found for port %d", conn_port);
        return BT_STATUS_PARM_INVALID;
    }

    node = bt_list_head(spp_conn->rx_list);
    if (node) {
        bt_list_remove_node(spp_conn->rx_list, node);
    }

    spp_conn_unlock();

    bt_rfcomm_dlc_update_credits(&spp_conn->rfcomm_dlc);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_write(uint16_t conn_port, uint8_t* buf, uint16_t size)
{
    sal_spp_connection_t* spp_conn;
    struct net_buf* nbuf = NULL;
    sal_spp_buffer_t* spp_buf;
    int ret;

    spp_conn_lock();
    spp_conn = spp_find_connection_by_port(conn_port);
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("No SPP connection found for port %d", conn_port);
        return BT_STATUS_PARM_INVALID;
    }

    spp_conn_unlock();

    nbuf = net_buf_alloc(&rfcomm_tx_pool, K_NO_WAIT);
    if (!nbuf) {
        BT_LOGW("rfcomm_tx_pool exhausted");
        return BT_STATUS_NOMEM;
    }

    net_buf_reserve(nbuf, SPP_MFS_EXTRA_SIZE - 1); /* exclude trailing FCS byte */
    net_buf_add_mem(nbuf, buf, size);

    ret = bt_rfcomm_dlc_send(&spp_conn->rfcomm_dlc, nbuf);
    if (ret < 0) {
        BT_LOGE("Failed to send data on RFCOMM DLC: %d", ret);
        net_buf_unref(nbuf);
        return BT_STATUS_FAIL;
    }

    BT_DUMPBUFFER("SPP TX", buf, size);

    spp_buf = malloc(sizeof(sal_spp_buffer_t));
    if (!spp_buf) {
        BT_LOGE("Failed to allocate memory for SPP buffer");
        net_buf_unref(nbuf);
        return BT_STATUS_NOMEM;
    }

    spp_buf->conn_port = conn_port;
    spp_buf->buf = buf;

    spp_conn_lock();
    bt_list_add_tail(spp_conn->tx_list, spp_buf);
    spp_conn_unlock();

    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_connect_request_reply(bt_address_t* addr, uint16_t port, bool accept)
{
    sal_spp_manager_t* spp_mgr = &g_spp_manager;
    sal_spp_connection_t* spp_conn;

    spp_conn_lock();
    spp_conn = spp_find_connection_by_scn(addr, PORT2SCN(port));
    if (!spp_conn) {
        spp_conn_unlock();
        BT_LOGE("No SPP connection found for port %d", port);
        return BT_STATUS_PARM_INVALID;
    }

    if (!accept) {
        bt_rfcomm_dlc_disconnect(&spp_conn->rfcomm_dlc);
        bt_list_remove(spp_mgr->connections, spp_conn);
        spp_conn_unlock();

        BT_LOGD("SPP connection on port %d rejected", port);
        return BT_STATUS_SUCCESS;
    }

    spp_conn->conn_port = port;
    spp_conn_unlock();

    BT_LOGD("Accepting SPP connection on port %d", port);
    return BT_STATUS_SUCCESS;
}

bt_status_t bt_sal_spp_connect_with_option(bt_address_t* addr, uint16_t conn_port, bt_uuid_t* uuid128, uint8_t insecure)
{
    if (insecure) {
        BT_LOGW("%s: insecure connection not supported yet", __func__);
        return BT_STATUS_UNSUPPORTED;
    }

    return bt_sal_spp_connect(addr, conn_port, uuid128);
}
