/***********************************************************************
 *
 * Copyright 2026 XiaoMi All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ***********************************************************************/

#include <nuttx/config.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <debug.h>
#include <syslog.h>
#include <unistd.h>

#include "z_api.h"
#include "z_api_manager.h"
#include "bt_gatts.h"
#include "bt_gattc.h"
#include "bt_gatt_defs.h"
#include "bluetooth.h"
#include "utils/log.h"

/* ================================================================
 * Zephyr type definitions (mirrored, no Zephyr headers needed)
 *
 * These must match the Zephyr structures exactly.
 * ================================================================ */

/* Zephyr UUID types - MUST match Zephyr's enum */
#define Z_BT_UUID_TYPE_16  0
#define Z_BT_UUID_TYPE_32  1
#define Z_BT_UUID_TYPE_128 2

struct z_bt_uuid {
    uint8_t type;
};

struct z_bt_uuid_16 {
    struct z_bt_uuid uuid;
    uint16_t val;
};

struct z_bt_uuid_128 {
    struct z_bt_uuid uuid;
    uint8_t val[16];
};

/* Well-known Zephyr UUIDs (16-bit) */
#define Z_BT_UUID_GATT_PRIMARY_VAL    0x2800
#define Z_BT_UUID_GATT_SECONDARY_VAL  0x2801
#define Z_BT_UUID_GATT_INCLUDE_VAL    0x2802
#define Z_BT_UUID_GATT_CHRC_VAL       0x2803
#define Z_BT_UUID_GATT_CCC_VAL        0x2902
#define Z_BT_UUID_GATT_CEP_VAL        0x2900

/* Zephyr GATT permissions */
#define Z_BT_GATT_PERM_READ           0x01
#define Z_BT_GATT_PERM_WRITE          0x02
#define Z_BT_GATT_PERM_READ_ENCRYPT   0x04
#define Z_BT_GATT_PERM_WRITE_ENCRYPT  0x08
#define Z_BT_GATT_PERM_READ_AUTHEN    0x10
#define Z_BT_GATT_PERM_WRITE_AUTHEN   0x20
#define Z_BT_GATT_PERM_PREPARE_WRITE  0x40

/* Zephyr GATT characteristic properties */
#define Z_BT_GATT_CHRC_BROADCAST      0x01
#define Z_BT_GATT_CHRC_READ           0x02
#define Z_BT_GATT_CHRC_WRITE_NR       0x04
#define Z_BT_GATT_CHRC_WRITE          0x08
#define Z_BT_GATT_CHRC_NOTIFY         0x10
#define Z_BT_GATT_CHRC_INDICATE       0x20
#define Z_BT_GATT_CHRC_AUTH           0x40
#define Z_BT_GATT_CHRC_EXT_PROP      0x80

/* Zephyr GATT CCC values */
#define Z_BT_GATT_CCC_NOTIFY    0x0001
#define Z_BT_GATT_CCC_INDICATE  0x0002

/* Zephyr GATT iterator return values */
#define Z_BT_GATT_ITER_STOP     0
#define Z_BT_GATT_ITER_CONTINUE 1

/* Zephyr ATT error codes */
#define Z_BT_ATT_ERR_UNLIKELY   0x0E

/* Zephyr bt_gatt_attr - must match Zephyr's layout exactly */
typedef ssize_t (*z_bt_gatt_attr_read_func_t)(void *conn, const void *attr,
    void *buf, uint16_t len, uint16_t offset);
typedef ssize_t (*z_bt_gatt_attr_write_func_t)(void *conn, const void *attr,
    const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

struct z_bt_gatt_attr {
    const struct z_bt_uuid *uuid;
    z_bt_gatt_attr_read_func_t read;
    z_bt_gatt_attr_write_func_t write;
    void *user_data;
    uint16_t handle;
    uint16_t perm;
};

/* Zephyr bt_gatt_service */
struct z_bt_gatt_service {
    void *hdev;
    struct z_bt_gatt_attr *attrs;
    size_t attr_count;
    /* sys_snode_t node - we don't need this */
};

/* Zephyr bt_gatt_chrc (characteristic declaration user_data) */
struct z_bt_gatt_chrc {
    const struct z_bt_uuid *uuid;
    uint16_t value_handle;
    uint8_t properties;
};

/* Zephyr bt_gatt_service_val (service declaration user_data) */
struct z_bt_gatt_service_val {
    const struct z_bt_uuid *uuid;
    uint16_t end_handle;
};

/* Zephyr bt_gatt_notify_params */
struct z_bt_gatt_notify_params {
    const struct z_bt_uuid *uuid;
    const struct z_bt_gatt_attr *attr;
    const void *data;
    uint16_t len;
    void (*func)(void *conn, void *user_data);
    void *user_data;
};

/* Zephyr bt_gatt_indicate_params */
struct z_bt_gatt_indicate_params {
    const struct z_bt_uuid *uuid;
    const struct z_bt_gatt_attr *attr;
    void (*func)(void *conn, void *params, uint8_t err);
    void (*destroy)(void *params);
    const void *data;
    uint16_t len;
};

/* Zephyr bt_gatt_exchange_params */
struct z_bt_gatt_exchange_params {
    void (*func)(void *conn, uint8_t err, void *params);
};

/* Zephyr bt_gatt_discover_params (simplified) */
struct z_bt_gatt_discover_params {
    const struct z_bt_uuid *uuid;
    uint8_t (*func)(void *conn, const void *attr, void *params);
    /* union { ... } - we access start_handle/end_handle directly */
    uint16_t start_handle;
    uint16_t end_handle;
    uint8_t type;
};

/* Zephyr bt_gatt_read_params (simplified) */
struct z_bt_gatt_read_params {
    uint8_t (*func)(void *conn, uint8_t err, void *params,
                    const void *data, uint16_t length);
    uint16_t handle_count;
    union {
        struct {
            uint16_t handle;
            uint16_t offset;
        } single;
        struct {
            const struct z_bt_uuid *uuid;
            uint16_t start_handle;
            uint16_t end_handle;
        } by_uuid;
        struct {
            uint16_t *handles;
            bool variable;
        } multiple;
    };
};

/* Zephyr bt_gatt_write_params */
struct z_bt_gatt_write_params {
    void (*func)(void *conn, uint8_t err, void *params);
    uint16_t handle;
    uint16_t offset;
    const void *data;
    uint16_t length;
};

/* Zephyr bt_gatt_subscribe_params (simplified) */
struct z_bt_gatt_subscribe_params {
    uint8_t (*notify)(void *conn, void *params, const void *data, uint16_t length);
    void (*subscribe)(void *conn, uint8_t err, void *params);
    void (*write)(void *conn, uint8_t err, void *params);
    uint16_t value_handle;
    uint16_t ccc_handle;
    uint16_t end_handle;
    /* atomic_t flags[1]; */
    uint32_t flags;
    uint16_t value;
    uint8_t min_security;
};

/* Zephyr bt_gatt_attr_func_t */
typedef uint8_t (*z_bt_gatt_attr_func_t)(const void *attr, uint16_t handle, void *user_data);

/* ================================================================
 * External references from z_api_gap.c
 * ================================================================ */
typedef int (*z_api_func_t)(void *arg);
extern int z_api_dispatch(z_api_func_t func, void *arg);
extern bt_instance_t *local_bt_ins;

extern gatts_handle_t g_gatts_handle;
extern gattc_handle_t g_gattc_handle;
extern gatts_callbacks_t g_gatts_cbs;
extern gattc_callbacks_t g_gattc_cbs;

/* Delegate to existing connection handlers in z_api_gap.c */
extern void gatts_on_connected(gatts_handle_t h, bt_address_t *addr);
extern void gatts_on_disconnected(gatts_handle_t h, bt_address_t *addr);

/* Connection type from z_api_gap.c */
typedef struct {
    bool in_use;
    uint8_t ref_count;
    uint8_t role;
    uint8_t sec_level;
    uint8_t enc_key_size;
    uint8_t addr[7];
    uint16_t interval;
    uint16_t latency;
    uint16_t timeout;
    bt_address_t fw_addr;
    gattc_handle_t gattc_handle;
    bool connected;
    bool connect_notified;
    bool disconnect_notified;
} z_conn_t;

extern z_conn_t g_conns[];
#define MAX_CONNECTIONS 4

static bt_instance_t *get_ins(void)
{
    if (local_bt_ins) return local_bt_ins;
    return (bt_instance_t *)z_api(bt_svc_ins_get)();
}

static z_conn_t *conn_lookup_by_zephyr(void *conn)
{
    if (!conn) return NULL;
    z_conn_t *c = (z_conn_t *)conn;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (&g_conns[i] == c && c->in_use) return c;
    }
    return NULL;
}

/* ================================================================
 * Service registration tracking
 * ================================================================ */

#define MAX_REGISTERED_SERVICES 10

typedef struct {
    bool in_use;
    struct z_bt_gatt_service *z_svc;
    uint16_t fw_base_handle;
    uint16_t attr_count;
    gatt_attr_db_t *fw_attrs;
    gatt_srv_db_t fw_srv_db;
    bool registered;
    gatts_handle_t own_gatts_handle;  /* Each service gets its own handle */
} registered_svc_t;

static registered_svc_t g_reg_svcs[MAX_REGISTERED_SERVICES];

static volatile bool g_svc_reg_done;
static volatile gatt_status_t g_svc_reg_status;
static volatile uint16_t g_svc_reg_handle;

/* ================================================================
 * UUID conversion: Zephyr → Framework
 * ================================================================ */
static bt_uuid_t zephyr_uuid_to_fw(const struct z_bt_uuid *z_uuid)
{
    bt_uuid_t fw;
    memset(&fw, 0, sizeof(fw));

    if (!z_uuid) return fw;

    if (z_uuid->type == Z_BT_UUID_TYPE_16) {
        const struct z_bt_uuid_16 *u16 = (const struct z_bt_uuid_16 *)z_uuid;
        fw.type = BT_UUID16_TYPE;
        fw.val.u16 = u16->val;
    } else if (z_uuid->type == Z_BT_UUID_TYPE_128) {
        const struct z_bt_uuid_128 *u128 = (const struct z_bt_uuid_128 *)z_uuid;
        fw.type = BT_UUID128_TYPE;
        memcpy(fw.val.u128, u128->val, 16);
    }
    return fw;
}

static bool is_uuid16(const struct z_bt_uuid *uuid, uint16_t val)
{
    if (!uuid || uuid->type != Z_BT_UUID_TYPE_16) return false;
    return ((const struct z_bt_uuid_16 *)uuid)->val == val;
}

/* ================================================================
 * Permission/Property conversion
 * ================================================================ */
static uint32_t zephyr_perm_to_fw(uint16_t z_perm)
{
    uint32_t fw = 0;
    if (z_perm & Z_BT_GATT_PERM_READ)          fw |= GATT_PERM_READ;
    if (z_perm & Z_BT_GATT_PERM_WRITE)         fw |= GATT_PERM_WRITE;
    if (z_perm & Z_BT_GATT_PERM_READ_ENCRYPT)  fw |= GATT_PERM_READ | GATT_PERM_ENCRYPT_REQUIRED;
    if (z_perm & Z_BT_GATT_PERM_WRITE_ENCRYPT) fw |= GATT_PERM_WRITE | GATT_PERM_ENCRYPT_REQUIRED;
    if (z_perm & Z_BT_GATT_PERM_READ_AUTHEN)   fw |= GATT_PERM_READ | GATT_PERM_AUTHEN_REQUIRED;
    if (z_perm & Z_BT_GATT_PERM_WRITE_AUTHEN)  fw |= GATT_PERM_WRITE | GATT_PERM_AUTHEN_REQUIRED;
    if (z_perm & Z_BT_GATT_PERM_PREPARE_WRITE) fw |= GATT_PERM_WRITE;
    return fw;
}

static uint32_t zephyr_prop_to_fw(uint8_t z_prop)
{
    uint32_t fw = 0;
    if (z_prop & Z_BT_GATT_CHRC_BROADCAST)  fw |= GATT_PROP_BROADCAST;
    if (z_prop & Z_BT_GATT_CHRC_READ)       fw |= GATT_PROP_READ;
    if (z_prop & Z_BT_GATT_CHRC_WRITE_NR)   fw |= GATT_PROP_WRITE_NR;
    if (z_prop & Z_BT_GATT_CHRC_WRITE)      fw |= GATT_PROP_WRITE;
    if (z_prop & Z_BT_GATT_CHRC_NOTIFY)     fw |= GATT_PROP_NOTIFY;
    if (z_prop & Z_BT_GATT_CHRC_INDICATE)   fw |= GATT_PROP_INDICATE;
    if (z_prop & Z_BT_GATT_CHRC_AUTH)       fw |= GATT_PROP_SIGNED_WRITE;
    if (z_prop & Z_BT_GATT_CHRC_EXT_PROP)  fw |= GATT_PROP_EXTENDED_PROPS;
    return fw;
}

static gatt_attr_type_t get_attr_type(const struct z_bt_uuid *uuid)
{
    if (!uuid) return GATT_DESCRIPTOR;

    if (uuid->type == Z_BT_UUID_TYPE_16) {
        uint16_t val = ((const struct z_bt_uuid_16 *)uuid)->val;
        _info("[z_api_gatt] get_attr_type: uuid16=0x%04x\n", val);
        if (val == Z_BT_UUID_GATT_PRIMARY_VAL)   return GATT_PRIMARY_SERVICE;
        if (val == Z_BT_UUID_GATT_SECONDARY_VAL) return GATT_SECONDARY_SERVICE;
        if (val == Z_BT_UUID_GATT_INCLUDE_VAL)   return GATT_INCLUDED_SERVICE;
        if (val == Z_BT_UUID_GATT_CHRC_VAL)      return GATT_CHARACTERISTIC;
    }
    return GATT_DESCRIPTOR;
}

/* ================================================================
 * GATTS callbacks
 * ================================================================ */

static uint16_t gatts_read_cb(gatts_handle_t srv_handle, bt_address_t *addr,
                               uint16_t attr_handle, uint32_t req_handle)
{
    _info("[z_api_gatt] gatts_read_cb: attr_handle=%u req_handle=%u\n",
          attr_handle, req_handle);

    /* Find the connection object for this address */
    void *z_conn = NULL;
    if (addr) {
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (g_conns[i].in_use && g_conns[i].connected &&
                memcmp(&g_conns[i].fw_addr, addr, sizeof(bt_address_t)) == 0) {
                z_conn = &g_conns[i];
                break;
            }
        }
    }

    for (int s = 0; s < MAX_REGISTERED_SERVICES; s++) {
        registered_svc_t *rsvc = &g_reg_svcs[s];
        if (!rsvc->in_use || !rsvc->registered) continue;
        if (attr_handle >= rsvc->attr_count) continue;

        struct z_bt_gatt_attr *z_attr = &rsvc->z_svc->attrs[attr_handle];
        if (!z_attr->read) {
            _info("[z_api_gatt] gatts_read_cb: no read handler for attr[%u]\n", attr_handle);
            bt_gatts_response(srv_handle, addr, req_handle, NULL, 0);
            return 0;
        }

        uint8_t buf[512];
        ssize_t len = z_attr->read(z_conn, z_attr, buf, sizeof(buf), 0);
        if (len < 0) {
            _info("[z_api_gatt] gatts_read_cb: read returned %d\n", (int)len);
            bt_gatts_response(srv_handle, addr, req_handle, NULL, 0);
            return 0;
        }

        _info("[z_api_gatt] gatts_read_cb: read %d bytes\n", (int)len);
        bt_gatts_response(srv_handle, addr, req_handle, buf, (uint16_t)len);
        return 0;
    }

    _info("[z_api_gatt] gatts_read_cb: no matching service for attr_handle=%u\n", attr_handle);
    bt_gatts_response(srv_handle, addr, req_handle, NULL, 0);
    return 0;
}

static uint16_t gatts_write_cb(gatts_handle_t srv_handle, bt_address_t *addr,
                                uint16_t attr_handle, const uint8_t *value,
                                uint16_t length, uint16_t offset)
{
    _info("[z_api_gatt] gatts_write_cb: attr_handle=%u len=%u offset=%u\n",
          attr_handle, length, offset);

    /* Find the connection object for this address */
    void *z_conn = NULL;
    if (addr) {
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (g_conns[i].in_use && g_conns[i].connected &&
                memcmp(&g_conns[i].fw_addr, addr, sizeof(bt_address_t)) == 0) {
                z_conn = &g_conns[i];
                break;
            }
        }
    }

    for (int s = 0; s < MAX_REGISTERED_SERVICES; s++) {
        registered_svc_t *rsvc = &g_reg_svcs[s];
        if (!rsvc->in_use || !rsvc->registered) continue;
        if (attr_handle >= rsvc->attr_count) continue;

        struct z_bt_gatt_attr *z_attr = &rsvc->z_svc->attrs[attr_handle];
        if (!z_attr->write) {
            _info("[z_api_gatt] gatts_write_cb: no write handler for attr[%u]\n", attr_handle);
            return 0;
        }

        ssize_t ret = z_attr->write(z_conn, z_attr, value, length, offset, 0);
        _info("[z_api_gatt] gatts_write_cb: write returned %d\n", (int)ret);
        return (ret >= 0) ? (uint16_t)ret : 0;
    }

    _info("[z_api_gatt] gatts_write_cb: no matching service for attr_handle=%u\n", attr_handle);
    return 0;
}

static void gatts_attr_table_added_cb(gatts_handle_t srv_handle,
                                       gatt_status_t status, uint16_t attr_handle)
{
    _info("[z_api_gatt] attr_table_added: status=%d handle=0x%04x srv_handle=%p\n",
          status, attr_handle, srv_handle);
    g_svc_reg_status = status;
    g_svc_reg_handle = attr_handle;
    g_svc_reg_done = true;

    /* Update the registered service's base handle and Zephyr attr handles */
    if (status == GATT_STATUS_SUCCESS) {
        for (int s = 0; s < MAX_REGISTERED_SERVICES; s++) {
            registered_svc_t *rsvc = &g_reg_svcs[s];
            if (rsvc->in_use && rsvc->registered &&
                rsvc->own_gatts_handle == srv_handle) {
                rsvc->fw_base_handle = attr_handle;
                /* Update Zephyr attr handles with real framework handles */
                for (uint16_t i = 0; i < rsvc->attr_count; i++) {
                    rsvc->z_svc->attrs[i].handle = attr_handle + i + 1;
                }
                _info("[z_api_gatt] attr_table_added: updated svc[%d] base=0x%04x\n", s, attr_handle);
                break;
            }
        }
    }
}

static void gatts_attr_table_removed_cb(gatts_handle_t srv_handle,
                                         gatt_status_t status, uint16_t attr_handle)
{
    _info("[z_api_gatt] attr_table_removed: status=%d handle=0x%04x\n", status, attr_handle);
}

static void gatts_notify_complete_cb(gatts_handle_t srv_handle, bt_address_t *addr,
                                      gatt_status_t status, uint16_t attr_handle)
{
    _info("[z_api_gatt] notify_complete: status=%d handle=0x%04x\n", status, attr_handle);
}

/* ================================================================
 * Per-service GATTS callbacks
 *
 * Each service registered via bt_gatts_register_service() gets this
 * callback table.  on_connected / on_disconnected delegate to the
 * existing connection management in z_api_gap.c.
 * ================================================================ */

static void svc_gatts_on_connected(gatts_handle_t h, bt_address_t *addr)
{
    _info("[z_api_gatt] svc_gatts_on_connected: h=%p\n", h);
    gatts_on_connected(h, addr);
}

static void svc_gatts_on_disconnected(gatts_handle_t h, bt_address_t *addr)
{
    _info("[z_api_gatt] svc_gatts_on_disconnected: h=%p\n", h);
    gatts_on_disconnected(h, addr);
}

static void svc_gatts_attr_table_added(gatts_handle_t srv_handle,
                                        gatt_status_t status, uint16_t attr_handle)
{
    _info("[z_api_gatt] svc_attr_table_added: status=%d handle=0x%04x srv_handle=%p\n",
          status, attr_handle, srv_handle);
    g_svc_reg_status = status;
    g_svc_reg_handle = attr_handle;
    g_svc_reg_done = true;

    if (status == GATT_STATUS_SUCCESS) {
        for (int s = 0; s < MAX_REGISTERED_SERVICES; s++) {
            registered_svc_t *rsvc = &g_reg_svcs[s];
            if (rsvc->in_use && rsvc->registered &&
                rsvc->own_gatts_handle == srv_handle) {
                rsvc->fw_base_handle = attr_handle;
                for (uint16_t i = 0; i < rsvc->attr_count; i++) {
                    rsvc->z_svc->attrs[i].handle = attr_handle + i + 1;
                }
                _info("[z_api_gatt] svc_attr_table_added: updated svc[%d] base=0x%04x\n",
                      s, attr_handle);
                break;
            }
        }
    }
}

static void svc_gatts_attr_table_removed(gatts_handle_t srv_handle,
                                          gatt_status_t status, uint16_t attr_handle)
{
    _info("[z_api_gatt] svc_attr_table_removed: status=%d handle=0x%04x\n",
          status, attr_handle);
}

static void svc_gatts_notify_complete(gatts_handle_t srv_handle, bt_address_t *addr,
                                       gatt_status_t status, uint16_t attr_handle)
{
    _info("[z_api_gatt] svc_notify_complete: status=%d handle=0x%04x\n",
          status, attr_handle);
}

static gatts_callbacks_t svc_gatts_cbs = {
    .size = sizeof(gatts_callbacks_t),
    .on_connected = svc_gatts_on_connected,
    .on_disconnected = svc_gatts_on_disconnected,
    .on_attr_table_added = svc_gatts_attr_table_added,
    .on_attr_table_removed = svc_gatts_attr_table_removed,
    .on_notify_complete = svc_gatts_notify_complete,
};

/* ================================================================
 * GATTC callbacks
 * ================================================================ */

/* Zephyr discover types - must match enum in gatt.h */
#define Z_BT_GATT_DISCOVER_PRIMARY        0
#define Z_BT_GATT_DISCOVER_SECONDARY      1
#define Z_BT_GATT_DISCOVER_INCLUDE        2
#define Z_BT_GATT_DISCOVER_CHARACTERISTIC 3
#define Z_BT_GATT_DISCOVER_DESCRIPTOR     4
#define Z_BT_GATT_DISCOVER_ATTRIBUTE      5
#define Z_BT_GATT_DISCOVER_STD_CHAR_DESC  6

/* Zephyr bt_gatt_include (user_data for DISCOVER_INCLUDE) */
struct z_bt_gatt_include {
    const struct z_bt_uuid *uuid;
    uint16_t start_handle;
    uint16_t end_handle;
};

static struct z_bt_gatt_discover_params *g_pending_discover;
static void *g_pending_discover_conn;

/* Helper: convert framework uuid to Zephyr uuid (static storage) */
static struct z_bt_uuid_16 g_tmp_uuid16;
static struct z_bt_uuid_128 g_tmp_uuid128;

static const struct z_bt_uuid *fw_uuid_to_zephyr(bt_uuid_t *fw_uuid)
{
    if (!fw_uuid || fw_uuid->type == 0) return NULL;
    if (fw_uuid->type == BT_UUID16_TYPE) {
        g_tmp_uuid16.uuid.type = Z_BT_UUID_TYPE_16;
        g_tmp_uuid16.val = fw_uuid->val.u16;
        return &g_tmp_uuid16.uuid;
    } else {
        g_tmp_uuid128.uuid.type = Z_BT_UUID_TYPE_128;
        memcpy(g_tmp_uuid128.val, fw_uuid->val.u128, 16);
        return &g_tmp_uuid128.uuid;
    }
}

static void gattc_discover_cb(gattc_handle_t conn_handle, gatt_status_t status,
                               bt_uuid_t *uuid, uint16_t start_handle, uint16_t end_handle)
{
    _info("[z_api_gatt] gattc_discover_cb: status=%d start=0x%04x end=0x%04x\n",
          status, start_handle, end_handle);

    if (!g_pending_discover || !g_pending_discover->func) return;

    if (!uuid || uuid->type == 0 || status != GATT_STATUS_SUCCESS) {
        /* Discovery complete - now handle post-discovery for chrc/desc/include */
        uint8_t disc_type = g_pending_discover->type;

        if (disc_type == Z_BT_GATT_DISCOVER_PRIMARY ||
            disc_type == Z_BT_GATT_DISCOVER_SECONDARY) {
            /* Service discovery complete, signal end */
            g_pending_discover->func(g_pending_discover_conn, NULL, g_pending_discover);
            g_pending_discover = NULL;
            return;
        }

        /* For chrc/desc/include/attribute discovery, iterate cached attributes */
        gattc_handle_t h = conn_handle;
        uint16_t sh = g_pending_discover->start_handle;
        uint16_t eh = g_pending_discover->end_handle;
        const struct z_bt_uuid *filter_uuid = g_pending_discover->uuid;

        _info("[z_api_gatt] post-discover: type=%d range=0x%04x-0x%04x\n",
              disc_type, sh, eh);

        for (uint16_t handle = sh; handle <= eh; handle++) {
            gatt_attr_desc_t desc;
            memset(&desc, 0, sizeof(desc));
            bt_status_t ret = bt_gattc_get_attribute_by_handle(h, handle, &desc);
            if (ret != BT_STATUS_SUCCESS) continue;

            bool match = false;
            if (disc_type == Z_BT_GATT_DISCOVER_CHARACTERISTIC) {
                match = (desc.type == GATT_CHARACTERISTIC);
            } else if (disc_type == Z_BT_GATT_DISCOVER_DESCRIPTOR) {
                match = (desc.type == GATT_DESCRIPTOR);
            } else if (disc_type == Z_BT_GATT_DISCOVER_INCLUDE) {
                match = (desc.type == GATT_INCLUDED_SERVICE);
            } else if (disc_type == Z_BT_GATT_DISCOVER_ATTRIBUTE ||
                       disc_type == Z_BT_GATT_DISCOVER_STD_CHAR_DESC) {
                match = true;
            }

            if (!match) continue;

            /* UUID filter check */
            if (filter_uuid) {
                const struct z_bt_uuid *attr_uuid = fw_uuid_to_zephyr(&desc.uuid);
                if (attr_uuid) {
                    bool uuid_match = false;
                    if (filter_uuid->type == Z_BT_UUID_TYPE_16 && attr_uuid->type == Z_BT_UUID_TYPE_16) {
                        uuid_match = ((const struct z_bt_uuid_16 *)filter_uuid)->val ==
                                     ((const struct z_bt_uuid_16 *)attr_uuid)->val;
                    } else if (filter_uuid->type == Z_BT_UUID_TYPE_128 && attr_uuid->type == Z_BT_UUID_TYPE_128) {
                        uuid_match = memcmp(((const struct z_bt_uuid_128 *)filter_uuid)->val,
                                           ((const struct z_bt_uuid_128 *)attr_uuid)->val, 16) == 0;
                    }
                    if (!uuid_match) continue;
                }
            }

            /* Build Zephyr attr for callback */
            struct z_bt_gatt_attr attr;
            memset(&attr, 0, sizeof(attr));
            attr.handle = desc.handle;

            const struct z_bt_uuid *z_uuid = fw_uuid_to_zephyr(&desc.uuid);

            /* Static storage for callback data */
            static struct z_bt_uuid_16 cb_uuid16;
            static struct z_bt_uuid_128 cb_uuid128;
            static struct z_bt_gatt_chrc cb_chrc;
            static struct z_bt_gatt_include cb_include;

            if (z_uuid && z_uuid->type == Z_BT_UUID_TYPE_16) {
                cb_uuid16 = g_tmp_uuid16;
                attr.uuid = &cb_uuid16.uuid;
            } else if (z_uuid) {
                cb_uuid128 = g_tmp_uuid128;
                attr.uuid = &cb_uuid128.uuid;
            }

            if (disc_type == Z_BT_GATT_DISCOVER_CHARACTERISTIC) {
                /* user_data = bt_gatt_chrc */
                memset(&cb_chrc, 0, sizeof(cb_chrc));
                cb_chrc.uuid = attr.uuid;
                cb_chrc.properties = (uint8_t)desc.properties;
                cb_chrc.value_handle = desc.handle + 1;
                attr.user_data = &cb_chrc;
                /* For chrc discovery, attr.uuid should be BT_UUID_GATT_CHRC */
                static struct z_bt_uuid_16 chrc_uuid = { .uuid = { .type = Z_BT_UUID_TYPE_16 }, .val = Z_BT_UUID_GATT_CHRC_VAL };
                attr.uuid = &chrc_uuid.uuid;
            } else if (disc_type == Z_BT_GATT_DISCOVER_INCLUDE) {
                memset(&cb_include, 0, sizeof(cb_include));
                cb_include.uuid = attr.uuid;
                cb_include.start_handle = desc.handle;
                cb_include.end_handle = desc.handle;
                attr.user_data = &cb_include;
            } else {
                /* descriptor / attribute - user_data = NULL */
                attr.user_data = NULL;
            }

            _info("[z_api_gatt] discover iter: handle=0x%04x type=%d\n",
                  desc.handle, desc.type);

            uint8_t iter = g_pending_discover->func(
                g_pending_discover_conn, &attr, g_pending_discover);
            if (iter == Z_BT_GATT_ITER_STOP) {
                g_pending_discover = NULL;
                return;
            }
        }

        /* Signal discovery complete */
        if (g_pending_discover && g_pending_discover->func) {
            g_pending_discover->func(g_pending_discover_conn, NULL, g_pending_discover);
        }
        g_pending_discover = NULL;
        return;
    }

    /* Service discovery result - only for PRIMARY/SECONDARY discover types */
    struct z_bt_gatt_attr attr;
    struct z_bt_gatt_service_val svc_val;
    struct z_bt_uuid_16 uuid16;
    struct z_bt_uuid_128 uuid128;

    memset(&attr, 0, sizeof(attr));
    attr.handle = start_handle;

    if (uuid->type == BT_UUID16_TYPE) {
        uuid16.uuid.type = Z_BT_UUID_TYPE_16;
        uuid16.val = uuid->val.u16;
        attr.uuid = &uuid16.uuid;
    } else {
        uuid128.uuid.type = Z_BT_UUID_TYPE_128;
        memcpy(uuid128.val, uuid->val.u128, 16);
        attr.uuid = &uuid128.uuid;
    }

    svc_val.uuid = attr.uuid;
    svc_val.end_handle = end_handle;
    attr.user_data = &svc_val;

    uint8_t iter = g_pending_discover->func(g_pending_discover_conn, &attr, g_pending_discover);
    if (iter == Z_BT_GATT_ITER_STOP) {
        g_pending_discover = NULL;
    }
}

static struct z_bt_gatt_read_params *g_pending_read;
static void *g_pending_read_conn;

static void gattc_read_cb(gattc_handle_t conn_handle, gatt_status_t status,
                           uint16_t attr_handle, uint8_t *value, uint16_t length)
{
    _info("[z_api_gatt] gattc_read_cb: status=%d handle=0x%04x len=%u\n",
          status, attr_handle, length);

    if (!g_pending_read || !g_pending_read->func) return;

    uint8_t err = (status == GATT_STATUS_SUCCESS) ? 0 : Z_BT_ATT_ERR_UNLIKELY;

    if (status != GATT_STATUS_SUCCESS || !value || length == 0) {
        g_pending_read->func(g_pending_read_conn, err, g_pending_read, NULL, 0);
        g_pending_read = NULL;
        return;
    }

    uint8_t iter = g_pending_read->func(g_pending_read_conn, err, g_pending_read, value, length);
    if (iter == Z_BT_GATT_ITER_STOP || iter != Z_BT_GATT_ITER_CONTINUE) {
        /* Signal read complete */
        g_pending_read->func(g_pending_read_conn, 0, g_pending_read, NULL, 0);
        g_pending_read = NULL;
    }
}

static struct z_bt_gatt_write_params *g_pending_write;
static void *g_pending_write_conn;

static void gattc_write_cb(gattc_handle_t conn_handle, gatt_status_t status,
                            uint16_t attr_handle)
{
    _info("[z_api_gatt] gattc_write_cb: status=%d handle=0x%04x\n", status, attr_handle);

    if (!g_pending_write || !g_pending_write->func) return;

    uint8_t err = (status == GATT_STATUS_SUCCESS) ? 0 : Z_BT_ATT_ERR_UNLIKELY;
    g_pending_write->func(g_pending_write_conn, err, g_pending_write);
    g_pending_write = NULL;
}

static void gattc_subscribe_cb(gattc_handle_t conn_handle, gatt_status_t status,
                                uint16_t attr_handle, bool enable)
{
    _info("[z_api_gatt] gattc_subscribe_cb: status=%d handle=0x%04x enable=%d\n",
          status, attr_handle, enable);
}

static void gattc_notify_received_cb(gattc_handle_t conn_handle, uint16_t attr_handle,
                                      uint8_t *value, uint16_t length)
{
    _info("[z_api_gatt] gattc_notify_received: handle=0x%04x len=%u\n", attr_handle, length);
}

static struct z_bt_gatt_exchange_params *g_pending_mtu;
static void *g_pending_mtu_conn;

static void gattc_mtu_cb(gattc_handle_t conn_handle, gatt_status_t status, uint32_t mtu)
{
    _info("[z_api_gatt] gattc_mtu_cb: status=%d mtu=%u\n", status, mtu);

    if (g_pending_mtu && g_pending_mtu->func) {
        uint8_t err = (status == GATT_STATUS_SUCCESS) ? 0 : Z_BT_ATT_ERR_UNLIKELY;
        g_pending_mtu->func(g_pending_mtu_conn, err, g_pending_mtu);
        g_pending_mtu = NULL;
    }
}

/* ================================================================
 * GATT Server: Service Registration
 * ================================================================ */

typedef struct {
    struct z_bt_gatt_service *svc;
    int result;
} svc_reg_args_t;

static int svc_register_in_ipc(void *arg)
{
    svc_reg_args_t *a = (svc_reg_args_t *)arg;
    struct z_bt_gatt_service *svc = a->svc;

    _info("[z_api_gatt] svc_register: attr_count=%zu\n", svc->attr_count);

    bt_instance_t *ins = get_ins();
    if (!ins) {
        _info("[z_api_gatt] svc_register: no bt instance\n");
        a->result = -EIO;
        return -EIO;
    }

    /* Log first attr UUID type for debugging */
    if (svc->attr_count > 0 && svc->attrs) {
        struct z_bt_gatt_attr *first = &svc->attrs[0];
        _info("[z_api_gatt] svc_register: first attr uuid=%p uuid->type=%d\n",
              first->uuid, first->uuid ? first->uuid->type : -1);
    }

    registered_svc_t *rsvc = NULL;
    for (int i = 0; i < MAX_REGISTERED_SERVICES; i++) {
        if (!g_reg_svcs[i].in_use) { rsvc = &g_reg_svcs[i]; break; }
    }
    if (!rsvc) { a->result = -ENOMEM; return -ENOMEM; }

    /* Register a NEW gatts handle for this service (do NOT reuse g_gatts_handle) */
    gatts_handle_t svc_handle = NULL;
    bt_status_t st = bt_gatts_register_service(ins, &svc_handle, &svc_gatts_cbs);
    _info("[z_api_gatt] svc_register: gatts register=%d handle=%p\n", st, svc_handle);
    if (st != BT_STATUS_SUCCESS || !svc_handle) {
        _info("[z_api_gatt] svc_register: failed to register gatts handle\n");
        a->result = -EIO;
        return -EIO;
    }

    uint16_t count = (uint16_t)svc->attr_count;
    gatt_attr_db_t *fw_attrs = calloc(count, sizeof(gatt_attr_db_t));
    if (!fw_attrs) {
        bt_gatts_unregister_service(svc_handle);
        a->result = -ENOMEM;
        return -ENOMEM;
    }

    for (uint16_t i = 0; i < count; i++) {
        struct z_bt_gatt_attr *z_attr = &svc->attrs[i];
        gatt_attr_db_t *fw = &fw_attrs[i];

        fw->handle = i;
        fw->type = get_attr_type(z_attr->uuid);
        fw->permissions = zephyr_perm_to_fw(z_attr->perm);

        if (fw->type == GATT_CHARACTERISTIC) {
            struct z_bt_gatt_chrc *chrc = (struct z_bt_gatt_chrc *)z_attr->user_data;
            if (chrc) {
                fw->properties = zephyr_prop_to_fw(chrc->properties);
                fw->uuid = zephyr_uuid_to_fw(chrc->uuid);
            } else {
                fw->uuid = zephyr_uuid_to_fw(z_attr->uuid);
            }
        } else if (fw->type == GATT_PRIMARY_SERVICE || fw->type == GATT_SECONDARY_SERVICE) {
            const struct z_bt_uuid *svc_uuid = (const struct z_bt_uuid *)z_attr->user_data;
            fw->uuid = svc_uuid ? zephyr_uuid_to_fw(svc_uuid) : zephyr_uuid_to_fw(z_attr->uuid);
        } else {
            fw->uuid = zephyr_uuid_to_fw(z_attr->uuid);
        }

        if (z_attr->read || z_attr->write) {
            fw->rsp_type = ATTR_RSP_BY_APP;
            fw->read_cb = z_attr->read ? gatts_read_cb : NULL;
            fw->write_cb = z_attr->write ? gatts_write_cb : NULL;
        } else {
            fw->rsp_type = ATTR_AUTO_RSP;
        }

        /* CCC descriptors need app response for read/write */
        if (is_uuid16(z_attr->uuid, Z_BT_UUID_GATT_CCC_VAL)) {
            fw->rsp_type = ATTR_RSP_BY_APP;
            fw->read_cb = gatts_read_cb;
            fw->write_cb = gatts_write_cb;
        }

        _info("[z_api_gatt] attr[%u]: type=%d perm=0x%x prop=0x%x rsp=%d\n",
              i, fw->type, fw->permissions, fw->properties, fw->rsp_type);
    }

    rsvc->in_use = true;
    rsvc->z_svc = svc;
    rsvc->attr_count = count;
    rsvc->fw_attrs = fw_attrs;
    rsvc->fw_srv_db.attr_num = count;
    rsvc->fw_srv_db.attr_db = fw_attrs;
    rsvc->registered = false;
    rsvc->own_gatts_handle = svc_handle;

    g_svc_reg_done = false;
    bt_status_t ret = bt_gatts_add_attr_table(svc_handle, &rsvc->fw_srv_db);
    _info("[z_api_gatt] bt_gatts_add_attr_table: ret=%d\n", ret);

    if (ret != BT_STATUS_SUCCESS) {
        free(fw_attrs);
        bt_gatts_unregister_service(svc_handle);
        rsvc->in_use = false;
        a->result = -EIO;
        return -EIO;
    }

    /* Don't wait for callback here - it will come on this same IPC thread.
     * Mark as registered immediately. The attr_table_added callback will
     * update fw_base_handle when it arrives. */
    rsvc->registered = true;

    /* Assign handles to Zephyr attrs (use index-based handles for now) */
    for (uint16_t i = 0; i < count; i++) {
        if (svc->attrs[i].handle == 0) {
            svc->attrs[i].handle = i + 1;
        }
    }

    _info("[z_api_gatt] svc_register: done, base=0x%04x count=%u\n",
          rsvc->fw_base_handle, count);
    a->result = 0;
    return 0;
}

int z_api(bt_gatt_service_register)(void *svc)
{
    _info("[z_api_gatt] >>> bt_gatt_service_register: svc=%p\n", svc);
    if (!svc) return -EINVAL;

    svc_reg_args_t args = { .svc = (struct z_bt_gatt_service *)svc, .result = 0 };
    int ret = z_api_dispatch(svc_register_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

/* ================================================================
 * GATT Server: Service Unregistration
 * ================================================================ */

static int svc_unregister_in_ipc(void *arg)
{
    struct z_bt_gatt_service *svc = (struct z_bt_gatt_service *)arg;

    for (int i = 0; i < MAX_REGISTERED_SERVICES; i++) {
        if (g_reg_svcs[i].in_use && g_reg_svcs[i].z_svc == svc) {
            if (g_reg_svcs[i].registered && g_reg_svcs[i].own_gatts_handle) {
                bt_gatts_remove_attr_table(g_reg_svcs[i].own_gatts_handle,
                                           g_reg_svcs[i].fw_base_handle);
                bt_gatts_unregister_service(g_reg_svcs[i].own_gatts_handle);
            }
            free(g_reg_svcs[i].fw_attrs);
            memset(&g_reg_svcs[i], 0, sizeof(registered_svc_t));
            return 0;
        }
    }
    return -ENOENT;
}

int z_api(bt_gatt_service_unregister)(void *svc)
{
    _info("[z_api_gatt] >>> bt_gatt_service_unregister\n");
    if (!svc) return -EINVAL;
    return z_api_dispatch(svc_unregister_in_ipc, svc);
}

/* ================================================================
 * GATT Server: Notify / Indicate
 * ================================================================ */

typedef struct {
    void *conn;
    const struct z_bt_gatt_attr *attr;
    const void *data;
    uint16_t len;
    int result;
} notify_args_t;

static int notify_in_ipc(void *arg)
{
    notify_args_t *a = (notify_args_t *)arg;

    uint16_t fw_handle = 0;
    bool found = false;
    gatts_handle_t svc_handle = NULL;

    for (int s = 0; s < MAX_REGISTERED_SERVICES && !found; s++) {
        registered_svc_t *rsvc = &g_reg_svcs[s];
        if (!rsvc->in_use || !rsvc->registered) continue;

        for (uint16_t i = 0; i < rsvc->attr_count; i++) {
            if (&rsvc->z_svc->attrs[i] == a->attr ||
                (a->attr && a->attr->handle && rsvc->z_svc->attrs[i].handle == a->attr->handle)) {
                fw_handle = i;
                svc_handle = rsvc->own_gatts_handle;
                found = true;
                break;
            }
        }
    }

    if (!found || !svc_handle) { a->result = -ENOENT; return -ENOENT; }

    bt_address_t *addr = NULL;
    if (a->conn) {
        z_conn_t *c = conn_lookup_by_zephyr(a->conn);
        if (c) addr = &c->fw_addr;
    }
    if (!addr) {
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (g_conns[i].in_use && g_conns[i].connected) {
                addr = &g_conns[i].fw_addr;
                break;
            }
        }
    }
    if (!addr) { a->result = -ENOTCONN; return -ENOTCONN; }

    _info("[z_api_gatt] notify: fw_handle=%u len=%u\n", fw_handle, a->len);
    bt_status_t ret = bt_gatts_notify(svc_handle, addr, fw_handle,
                                       (uint8_t *)a->data, a->len);
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_notify)(void *conn, const void *attr,
                          const void *data, uint16_t len)
{
    _info("[z_api_gatt] >>> bt_gatt_notify: len=%u\n", len);
    notify_args_t args = { .conn = conn, .attr = attr, .data = data, .len = len, .result = 0 };
    int ret = z_api_dispatch(notify_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

int z_api(bt_gatt_notify_cb)(void *conn, void *params)
{
    struct z_bt_gatt_notify_params *p = (struct z_bt_gatt_notify_params *)params;
    if (!p) return -EINVAL;
    int ret = z_api(bt_gatt_notify)(conn, p->attr, p->data, p->len);
    if (p->func) p->func(conn, p->user_data);
    return ret;
}

typedef struct {
    void *conn;
    struct z_bt_gatt_indicate_params *params;
    int result;
} indicate_args_t;

static int indicate_in_ipc(void *arg)
{
    indicate_args_t *a = (indicate_args_t *)arg;
    struct z_bt_gatt_indicate_params *p = a->params;

    if (!p) { a->result = -EIO; return -EIO; }

    uint16_t fw_handle = 0;
    bool found = false;
    gatts_handle_t svc_handle = NULL;

    for (int s = 0; s < MAX_REGISTERED_SERVICES && !found; s++) {
        registered_svc_t *rsvc = &g_reg_svcs[s];
        if (!rsvc->in_use || !rsvc->registered) continue;

        for (uint16_t i = 0; i < rsvc->attr_count; i++) {
            if (&rsvc->z_svc->attrs[i] == (struct z_bt_gatt_attr *)p->attr ||
                (p->attr && p->attr->handle && rsvc->z_svc->attrs[i].handle == p->attr->handle)) {
                fw_handle = i;
                svc_handle = rsvc->own_gatts_handle;
                found = true;
                break;
            }
        }
    }

    if (!found || !svc_handle) { a->result = -ENOENT; return -ENOENT; }

    bt_address_t *addr = NULL;
    if (a->conn) {
        z_conn_t *c = conn_lookup_by_zephyr(a->conn);
        if (c) addr = &c->fw_addr;
    }
    if (!addr) {
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (g_conns[i].in_use && g_conns[i].connected) {
                addr = &g_conns[i].fw_addr;
                break;
            }
        }
    }
    if (!addr) { a->result = -ENOTCONN; return -ENOTCONN; }

    bt_status_t ret = bt_gatts_indicate(svc_handle, addr, fw_handle,
                                         (uint8_t *)p->data, p->len);
    if (p->func) {
        uint8_t err = (ret == BT_STATUS_SUCCESS) ? 0 : Z_BT_ATT_ERR_UNLIKELY;
        p->func(a->conn, p, err);
    }
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_indicate)(void *conn, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_indicate\n");
    if (!params) return -EINVAL;
    indicate_args_t args = { .conn = conn, .params = params, .result = 0 };
    int ret = z_api_dispatch(indicate_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

/* ================================================================
 * GATT Client: Discover / Read / Write / Subscribe / MTU
 * ================================================================ */

typedef struct { void *conn; void *params; int result; } gattc_op_args_t;

static int discover_in_ipc(void *arg)
{
    gattc_op_args_t *a = (gattc_op_args_t *)arg;
    struct z_bt_gatt_discover_params *p = a->params;

    z_conn_t *conn = conn_lookup_by_zephyr(a->conn);
    if (!conn || !g_gattc_handle) { a->result = -ENOTCONN; return -ENOTCONN; }

    g_pending_discover = p;
    g_pending_discover_conn = a->conn;

    gattc_handle_t h = conn->gattc_handle ? conn->gattc_handle : g_gattc_handle;

    _info("[z_api_gatt] discover_in_ipc: type=%d start=0x%04x end=0x%04x\n",
          p->type, p->start_handle, p->end_handle);

    /* For service discovery, call bt_gattc_discover_service directly.
     * For chrc/desc/include discovery, we first need to discover services
     * to populate the attribute cache, then iterate in the callback. */
    bt_uuid_t *filter = NULL;
    bt_uuid_t fw_uuid;
    if (p->uuid && (p->type == Z_BT_GATT_DISCOVER_PRIMARY ||
                    p->type == Z_BT_GATT_DISCOVER_SECONDARY)) {
        fw_uuid = zephyr_uuid_to_fw(p->uuid);
        filter = &fw_uuid;
    }

    /* Always call discover_service first - the callback will handle
     * post-processing for chrc/desc/include types */
    bt_status_t ret = bt_gattc_discover_service(h, filter);
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_discover)(void *conn, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_discover\n");
    if (!conn || !params) return -EINVAL;
    gattc_op_args_t args = { .conn = conn, .params = params, .result = 0 };
    int ret = z_api_dispatch(discover_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

static int read_in_ipc(void *arg)
{
    gattc_op_args_t *a = (gattc_op_args_t *)arg;
    struct z_bt_gatt_read_params *p = a->params;

    z_conn_t *conn = conn_lookup_by_zephyr(a->conn);
    if (!conn || !g_gattc_handle) { a->result = -ENOTCONN; return -ENOTCONN; }

    g_pending_read = p;
    g_pending_read_conn = a->conn;

    gattc_handle_t h = conn->gattc_handle ? conn->gattc_handle : g_gattc_handle;
    uint16_t handle = (p->handle_count == 1) ? p->single.handle :
                      (p->handle_count == 0 && p->by_uuid.uuid) ? p->by_uuid.start_handle :
                      p->multiple.handles[0];

    bt_status_t ret = bt_gattc_read(h, handle);
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_read)(void *conn, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_read\n");
    if (!conn || !params) return -EINVAL;
    gattc_op_args_t args = { .conn = conn, .params = params, .result = 0 };
    int ret = z_api_dispatch(read_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

static int write_in_ipc(void *arg)
{
    gattc_op_args_t *a = (gattc_op_args_t *)arg;
    struct z_bt_gatt_write_params *p = a->params;

    z_conn_t *conn = conn_lookup_by_zephyr(a->conn);
    if (!conn || !g_gattc_handle) { a->result = -ENOTCONN; return -ENOTCONN; }

    g_pending_write = p;
    g_pending_write_conn = a->conn;

    gattc_handle_t h = conn->gattc_handle ? conn->gattc_handle : g_gattc_handle;
    bt_status_t ret = bt_gattc_write(h, p->handle, (uint8_t *)p->data, p->length);
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_write)(void *conn, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_write\n");
    if (!conn || !params) return -EINVAL;
    gattc_op_args_t args = { .conn = conn, .params = params, .result = 0 };
    int ret = z_api_dispatch(write_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

typedef struct {
    void *conn; uint16_t handle; const void *data; uint16_t length; bool sign; int result;
} write_nr_args_t;

static int write_nr_in_ipc(void *arg)
{
    write_nr_args_t *a = (write_nr_args_t *)arg;
    z_conn_t *conn = conn_lookup_by_zephyr(a->conn);
    if (!conn || !g_gattc_handle) { a->result = -ENOTCONN; return -ENOTCONN; }

    gattc_handle_t h = conn->gattc_handle ? conn->gattc_handle : g_gattc_handle;
    bt_status_t ret;
    if (a->sign) {
        ret = bt_gattc_write_with_signed(h, a->handle, (uint8_t *)a->data, a->length);
    } else {
        ret = bt_gattc_write_without_response(h, a->handle, (uint8_t *)a->data, a->length);
    }
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_write_without_response)(void *conn, uint16_t handle,
                                          const void *data, uint16_t length, bool sign)
{
    _info("[z_api_gatt] >>> bt_gatt_write_without_response: handle=0x%04x\n", handle);
    if (!conn) return -EINVAL;
    write_nr_args_t args = { .conn = conn, .handle = handle, .data = data,
                             .length = length, .sign = sign, .result = 0 };
    int ret = z_api_dispatch(write_nr_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

static int subscribe_in_ipc(void *arg)
{
    gattc_op_args_t *a = (gattc_op_args_t *)arg;
    struct z_bt_gatt_subscribe_params *p = a->params;

    z_conn_t *conn = conn_lookup_by_zephyr(a->conn);
    if (!conn || !g_gattc_handle) { a->result = -ENOTCONN; return -ENOTCONN; }

    gattc_handle_t h = conn->gattc_handle ? conn->gattc_handle : g_gattc_handle;
    uint16_t ccc = 0;
    if (p->value & Z_BT_GATT_CCC_NOTIFY)   ccc |= GATT_CCC_NOTIFY;
    if (p->value & Z_BT_GATT_CCC_INDICATE) ccc |= GATT_CCC_INDICATE;

    bt_status_t ret = bt_gattc_subscribe(h, p->ccc_handle, ccc);
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_subscribe)(void *conn, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_subscribe\n");
    if (!conn || !params) return -EINVAL;
    gattc_op_args_t args = { .conn = conn, .params = params, .result = 0 };
    int ret = z_api_dispatch(subscribe_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

static int unsubscribe_in_ipc(void *arg)
{
    gattc_op_args_t *a = (gattc_op_args_t *)arg;
    struct z_bt_gatt_subscribe_params *p = a->params;

    z_conn_t *conn = conn_lookup_by_zephyr(a->conn);
    if (!conn || !g_gattc_handle) { a->result = -ENOTCONN; return -ENOTCONN; }

    gattc_handle_t h = conn->gattc_handle ? conn->gattc_handle : g_gattc_handle;
    bt_status_t ret = bt_gattc_unsubscribe(h, p->ccc_handle);
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_unsubscribe)(void *conn, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_unsubscribe\n");
    if (!conn || !params) return -EINVAL;
    gattc_op_args_t args = { .conn = conn, .params = params, .result = 0 };
    int ret = z_api_dispatch(unsubscribe_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

static int exchange_mtu_in_ipc(void *arg)
{
    gattc_op_args_t *a = (gattc_op_args_t *)arg;

    z_conn_t *conn = conn_lookup_by_zephyr(a->conn);
    if (!conn || !g_gattc_handle) { a->result = -ENOTCONN; return -ENOTCONN; }

    g_pending_mtu = a->params;
    g_pending_mtu_conn = a->conn;

    gattc_handle_t h = conn->gattc_handle ? conn->gattc_handle : g_gattc_handle;
    bt_status_t ret = bt_gattc_exchange_mtu(h, 247);
    a->result = (ret == BT_STATUS_SUCCESS) ? 0 : -EIO;
    return a->result;
}

int z_api(bt_gatt_exchange_mtu)(void *conn, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_exchange_mtu\n");
    if (!conn || !params) return -EINVAL;
    gattc_op_args_t args = { .conn = conn, .params = params, .result = 0 };
    int ret = z_api_dispatch(exchange_mtu_in_ipc, &args);
    return (ret != 0) ? ret : args.result;
}

/* ================================================================
 * GATT Server: Attribute helpers
 * ================================================================ */

ssize_t z_api(bt_gatt_attr_read)(void *conn, const void *attr,
                                 void *buf, uint16_t buf_len, uint16_t offset,
                                 const void *value, uint16_t value_len)
{
    uint16_t len;
    if (offset > value_len) return -EINVAL;
    len = value_len - offset;
    if (len > buf_len) len = buf_len;
    memcpy(buf, (uint8_t *)value + offset, len);
    return len;
}

void z_api(bt_gatt_foreach_attr)(uint16_t start_handle, uint16_t end_handle,
                                 void *func, void *user_data)
{
    z_bt_gatt_attr_func_t cb = (z_bt_gatt_attr_func_t)func;
    if (!cb) return;

    for (int s = 0; s < MAX_REGISTERED_SERVICES; s++) {
        registered_svc_t *rsvc = &g_reg_svcs[s];
        if (!rsvc->in_use || !rsvc->registered) continue;

        for (uint16_t i = 0; i < rsvc->attr_count; i++) {
            struct z_bt_gatt_attr *attr = &rsvc->z_svc->attrs[i];
            uint16_t handle = attr->handle;
            if (handle < start_handle || handle > end_handle) continue;
            if (cb(attr, handle, user_data) == Z_BT_GATT_ITER_STOP) return;
        }
    }
}

int z_api(bt_gatt_notify_multiple)(void *conn, uint16_t num_params, void *params)
{
    _info("[z_api_gatt] >>> bt_gatt_notify_multiple: num=%u\n", num_params);
    struct z_bt_gatt_notify_params *p = (struct z_bt_gatt_notify_params *)params;
    int ret = 0;
    for (uint16_t i = 0; i < num_params; i++) {
        int r = z_api(bt_gatt_notify)(conn, p[i].attr, p[i].data, p[i].len);
        if (r != 0 && ret == 0) ret = r;
    }
    return ret;
}

/* ================================================================
 * GATT layer initialization
 * ================================================================ */

void z_api_gatt_init(void)
{
    _info("[z_api_gatt] z_api_gatt_init: installing callbacks\n");

    g_gatts_cbs.on_attr_table_added = gatts_attr_table_added_cb;
    g_gatts_cbs.on_attr_table_removed = gatts_attr_table_removed_cb;
    g_gatts_cbs.on_notify_complete = gatts_notify_complete_cb;

    g_gattc_cbs.on_discovered = gattc_discover_cb;
    g_gattc_cbs.on_read = gattc_read_cb;
    g_gattc_cbs.on_written = gattc_write_cb;
    g_gattc_cbs.on_subscribed = gattc_subscribe_cb;
    g_gattc_cbs.on_notified = gattc_notify_received_cb;
    g_gattc_cbs.on_mtu_updated = gattc_mtu_cb;

    /* Clear service registration state */
    memset(g_reg_svcs, 0, sizeof(g_reg_svcs));

    _info("[z_api_gatt] z_api_gatt_init: done\n");
}
