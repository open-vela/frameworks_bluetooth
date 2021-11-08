
#include "bts_gatts.h"

#include <errno.h>
#include <string.h>

#include "bts_service.h"
#include "log.h"
#include "stack_adapter_gatt.h"

#define LOG_TAG "bts_gatts"

static struct list_node gatts_list = LIST_INITIAL_VALUE(gatts_list);

typedef struct
{
    enum {
        ON_SERVER_OPENED = 0,
        ON_SERVER_CLOSED,
        ON_SERVER_CONNECTION_CHANGED,
        ON_SERVER_ELEMENTS_ADD,
        ON_SERVER_ELEMENTS_REMOVE,
        ON_SERVER_PHY_READ,
        ON_SERVER_PHY_UPDATE,
        ON_SERVER_READ_REQUEST,
        ON_SERVER_WRITE_REQUEST,
        ON_SRRVER_MTU_CHANGED,
        ON_SERVER_NOTIFICATION_SENT,
    } event;

    gatts_hdl_t* handle;
    size_t size;
    void* data;
} gatt_server_msg_t;

static void send_msg(gatt_server_msg_t* msg);
static void handle_event(void* data, size_t size);

static gatts_hdl_t* find_gatts_handle(uint8_t server_if)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        if (gatts->server_if == server_if) {
            return gatts;
        }
    }
    return NULL;
}

static int8_t gen_gatts_id()
{
    uint8_t found = 0;
    gatts_hdl_t* gatts;
    for (uint8_t i = 1; i < 256; i++) {
        list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
        {
            if (gatts->server_if == i) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return i;
        }
    }
    BT_LOGE("gatts id overflow");
    return -1;
}

static int8_t add_gatt_handle(gatts_hdl_t server)
{
    gatts_hdl_t* gatts = (gatts_hdl_t*)malloc(sizeof(gatts_hdl_t));
    if (!gatts) {
        BT_LOGE("malloc gatts_hdl_t fail");
        return -1;
    }
    memset(gatts, 0, sizeof(gatts_hdl_t));

    gatts->server_if = gen_gatts_id();
    gatts->callbacks = server.callbacks;
    gatts->btm_handle = server.btm_handle;
    list_add_tail(&gatts_list, &gatts->node);
    return gatts->server_if;
}

static bool remove_gatts_handle(gatts_hdl_t* gatts)
{
    list_delete(&gatts->node);
    free(gatts);
    return true;
}

static bt_result_code gatt_server_is_valid(uint8_t server_if)
{
    gatts_hdl_t* server = find_gatts_handle(server_if);
    CHECK_PTR_RETURN(server, BT_RESULT_FAILED);
    return BT_RESULT_SUCCESS;
}

// static uint8_t find_gatts_interface(gatt_element_t* element)
// {
// }

typedef struct {
    bd_addr_t addr;
    bt_state_t state
} server_state_op_s;

static void on_server_connection_state_changed(bd_addr_t remote_addr, bt_state_t state)
{

    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_CONNECTION_CHANGED;
        msg->handle = gatts;

        server_state_op_s* value = (server_state_op_s*)malloc(sizeof(server_state_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }

        value->state = state;
        memcpy(value->addr, remote_addr, sizeof(bd_addr_t));
        msg->data = value;
        msg->size = sizeof(server_state_op_s);
        send_msg(msg);
    }
}

typedef struct {
    gatt_service_status_t status;
    gatt_element_t* elements;
    uint16_t size;
} server_element_op_s;

static void on_server_elements_added(gatt_service_status_t status, gatt_element_t* elements,
    uint16_t size)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_ELEMENTS_ADD;
        msg->handle = gatts;

        server_element_op_s* value = (server_element_op_s*)malloc(sizeof(server_element_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        value->elements = elements;
        value->size = size;
        value->status = status;
        msg->data = value;
        msg->size = sizeof(server_element_op_s);
        send_msg(msg);
    }
}

static void on_server_elements_removed(gatt_service_status_t status, gatt_element_t* elements,
    uint16_t size)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_ELEMENTS_REMOVE;
        msg->handle = gatts;

        server_element_op_s* value = (server_element_op_s*)malloc(sizeof(server_element_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        value->elements = elements;
        value->size = size;
        value->status = status;
        msg->data = value;
        msg->size = sizeof(server_element_op_s);
        send_msg(msg);
    }
}

typedef struct {
    bd_addr_t addr;
    ble_phy_type_t tx;
    ble_phy_type_t rx
} server_phy_op_s;

static void on_server_phy_read(bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_PHY_READ;
        msg->handle = gatts;

        server_phy_op_s* value = (server_phy_op_s*)malloc(sizeof(server_phy_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        memcpy(value->addr, remote_addr, sizeof(bd_addr_t));
        value->tx = tx;
        value->rx = rx;
        msg->data = value;
        msg->size = sizeof(server_phy_op_s);
        send_msg(msg);
    }
}

static void on_server_phy_update(bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx, gatt_service_status_t status)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_PHY_UPDATE;
        msg->handle = gatts;

        server_phy_op_s* value = (server_phy_op_s*)malloc(sizeof(server_phy_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        memcpy(value->addr, remote_addr, sizeof(bd_addr_t));
        value->tx = tx;
        value->rx = rx;
        msg->data = value;
        msg->size = sizeof(server_phy_op_s);
        send_msg(msg);
    }
}

typedef struct
{
    bd_addr_t addr;
    uint32_t request_id;
    gatt_element_t* element;
} server_read_op_s;

static void on_server_read_request(bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_READ_REQUEST;
        msg->handle = gatts;

        server_read_op_s* value = (server_read_op_s*)malloc(sizeof(server_read_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        memcpy(value->addr, remote_addr, sizeof(bd_addr_t));
        value->request_id = request_id;
        value->element = element;
        msg->data = value;
        msg->size = sizeof(server_read_op_s);
        send_msg(msg);
    }
}

typedef struct
{
    bd_addr_t addr;
    uint32_t request_id;
    gatt_element_t* element;
    uint8_t* value;
    uint16_t offset;
    uint16_t size;
} server_write_op_s;

static void on_server_write_request(bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_WRITE_REQUEST;
        msg->handle = gatts;

        server_write_op_s* value = (server_write_op_s*)malloc(sizeof(server_write_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        memcpy(value->addr, remote_addr, sizeof(bd_addr_t));
        value->request_id = request_id;
        value->element = element;
        value->value = (uint8_t*)malloc(size);
        memcpy(value->value, value, size);
        value->offset = offset;
        value->size = size;
        msg->data = value;
        msg->size = sizeof(server_write_op_s);
        send_msg(msg);
    }
}

typedef struct {
    bd_addr_t addr;
    uint32_t mtu
} server_mtu_op_s;

static void on_server_mtu_changed(bd_addr_t remote_addr, uint32_t mtu)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SRRVER_MTU_CHANGED;
        msg->handle = gatts;

        server_mtu_op_s* value = (server_mtu_op_s*)malloc(sizeof(server_mtu_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        memcpy(value->addr, remote_addr, sizeof(bd_addr_t));
        value->mtu = mtu;
        msg->data = value;
        msg->size = sizeof(server_mtu_op_s);
        send_msg(msg);
    }
}

typedef struct {
    bd_addr_t addr;
    gatt_service_status_t status;
} server_notify_op_s;

static void on_server_notify_sent(bd_addr_t remote_addr, gatt_service_status_t status)
{
    gatts_hdl_t* gatts;
    list_for_every_entry(&gatts_list, gatts, gatts_hdl_t, node)
    {
        gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
        memset(msg, 0, sizeof(gatt_server_msg_t));
        msg->event = ON_SERVER_NOTIFICATION_SENT;
        msg->handle = gatts;

        server_notify_op_s* value = (server_notify_op_s*)malloc(sizeof(server_notify_op_s));
        if (!value) {
            BT_LOGE("fail, malloc");
            return;
        }
        memcpy(value->addr, remote_addr, sizeof(bd_addr_t));
        value->status = status;
        msg->data = value;
        msg->size = sizeof(server_notify_op_s);
        send_msg(msg);
    }
}

static const stack_gatt_server_callbacks gatt_server_cbs = {
    sizeof(GATT_SERVER_CALLBACKS_S),
    .gatt_server_connection_state_changed_cb = on_server_connection_state_changed,
    .gatt_server_elements_added_cb = on_server_elements_added,
    .gatt_server_elements_removed_cb = on_server_elements_removed,
    .gatt_server_phy_read_cb = on_server_phy_read,
    .gatt_server_phy_update_cb = on_server_phy_update,
    .gatt_server_received_element_read_request_cb = on_server_read_request,
    .gatt_server_received_element_write_request_cb = on_server_write_request,
    .gatt_server_mtu_changed_cb = on_server_mtu_changed,
    .gatt_server_notification_sent_cb = on_server_notify_sent,
};

static bt_result_code gatt_init()
{
    SERVICE_GATT_STATUS ret = service_adapter_gatt_init();
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt_init ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void gatt_clean_up()
{
    service_adapter_gatt_cleanup();
}

static bt_result_code gatt_server_open(gatts_hdl_t server)
{
    SERVICE_GATT_STATUS ret = service_adapter_gatt_server_open(&gatt_server_cbs);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server open, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    int8_t gatt_if = add_gatt_handle(server);
    if (gatt_if < 0) {
        BT_LOGE("fail, gatt add_gatt_handle, gatt_if:%d", gatt_if);
        return BT_RESULT_FAILED;
    }

    gatts_hdl_t* handle = find_gatts_handle(gatt_if);
    if (!handle) {
        BT_LOGE("fail, null handle");
        return BT_RESULT_FAILED;
    }

    gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
    memset(msg, 0, sizeof(gatt_server_msg_t));
    msg->event = ON_SERVER_OPENED;
    msg->handle = handle;
    send_msg(msg);
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_close(uint8_t server_if)
{
    SERVICE_GATT_STATUS ret = service_adapter_gatt_server_close();
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server close, err:%d", ret);
    }

    gatts_hdl_t* handle = find_gatts_handle(server_if);
    if (!handle) {
        BT_LOGE("fail, null handle");
        return BT_RESULT_FAILED;
    }

    gatt_server_msg_t* msg = (gatt_server_msg_t*)malloc(sizeof(gatt_server_msg_t));
    memset(msg, 0, sizeof(gatt_server_msg_t));
    msg->event = ON_SERVER_CLOSED;
    msg->handle = handle;
    send_msg(msg);
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_connect(uint8_t server_if, bd_addr_t remote_addr, bool auto_connect)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_connect(remote_addr);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server connect, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_disconnect(uint8_t server_if, bd_addr_t remote_addr)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_cancel_connection(remote_addr);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server disconnect, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_add_element(uint8_t server_if, gatt_element_t* element, uint16_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_add_elements(element, size);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server add elements, err:%d", ret2);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_remove_element(uint8_t server_if, uint32_t* ids, uint16_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_remove_elements(ids, size);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_read_phy(uint8_t server_if, bd_addr_t remote_addr)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_read_phy(remote_addr);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static void gatt_server_update_phy(uint8_t server_if, bd_addr_t remote_addr, ble_phy_type_t tx_type, ble_phy_type_t rx_type)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_set_phy(remote_addr, tx_type, rx_type);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_notify(uint8_t server_if, bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_send_notification(remote_addr, characteristic, value, size);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_indicate(uint8_t server_if, bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_send_indication(remote_addr, characteristic, value, size);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_response(uint8_t server_if, bd_addr_t remote_addr, gatt_response_t* response)
{
    bt_result_code ret = gatt_server_is_valid(server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, stack_gatts_interface check");
        return ret;
    }

    SERVICE_GATT_STATUS ret2 = service_adapter_gatt_server_send_response(remote_addr, response);
    if (ret2 != GATT_SUCCESS) {
        BT_LOGE("fail, gatt server remove elements, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static const gatt_server_interface_t gatt_server_intance = {
    .size = sizeof(gatt_server_intance),
    .init = gatt_init,
    .clean_up = gatt_clean_up,

    .open_server = gatt_server_open,
    .close_server = gatt_server_close,
    .connect = gatt_server_connect,
    .disconnect = gatt_server_disconnect,
    .add_element = gatt_server_add_element,
    .remove_element = gatt_server_remove_element,
    .read_phy = gatt_server_read_phy,
    .update_phy = gatt_server_update_phy,
    .send_notify = gatt_server_send_notify,
    .send_indicate = gatt_server_send_indicate,
    .send_response = gatt_server_send_response,
};

const gatt_server_interface_t* get_gatt_server_instance()
{
    return &gatt_server_intance;
}

static void handle_event(void* data, size_t size)
{
    BT_LOGD("%s", __func__);
    gatt_server_msg_t* msg = (gatt_server_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }

    gatts_hdl_t* handle = (gatts_hdl_t*)(msg->handle);
    if (!handle) {
        BT_LOGE("%s fail, handle null", __func__);
        return;
    }
    switch (msg->event) {
    case ON_SERVER_OPENED: {
        BT_CBACK(handle->callbacks, _server_opened_cb, handle->btm_handle, handle->server_if);
        free(data);
        break;
    }
    case ON_SERVER_CLOSED: {
        BT_CBACK(handle->callbacks, _server_closed_cb, handle->btm_handle);
        remove_gatts_handle(handle);
        free(data);
        break;
    }
    case ON_SERVER_CONNECTION_CHANGED: {
        server_state_op_s* value = (server_state_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_connection_state_changed_cb, handle->btm_handle, value->addr, value->state);
        free(value);
        free(data);
        break;
    }
    case ON_SERVER_ELEMENTS_ADD: {
        server_element_op_s* value = (server_element_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_elements_added_cb, handle->btm_handle, value->status, value->elements, value->size);
        free(value);
        free(data);
        break;
    }
    case ON_SERVER_ELEMENTS_REMOVE: {
        server_element_op_s* value = (server_element_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_elements_removed_cb, handle->btm_handle, value->status, value->elements, value->size);
        free(value);
        free(data);
        break;
    }
    case ON_SERVER_PHY_READ: {
        server_phy_op_s* value = (server_phy_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_phy_read_cb, handle->btm_handle, value->addr, value->tx, value->rx);
        free(value);
        free(data);
        break;
    }
    case ON_SERVER_PHY_UPDATE: {
        server_phy_op_s* value = (server_phy_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_phy_update_cb, handle->btm_handle, value->addr, value->tx, value->rx);
        free(value);
        free(data);
        break;
    }
    case ON_SERVER_READ_REQUEST: {
        server_read_op_s* value = (server_read_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_read_request_cb, handle->btm_handle, value->addr, value->request_id, value->element);
        free(value);
        free(data);
        break;
    }
    case ON_SERVER_WRITE_REQUEST: {
        server_write_op_s* value = (server_write_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_write_request_cb, handle->btm_handle, value->addr, value->request_id, value->element, value->value, value->offset, value->size);
        free(value->value);
        free(value);
        free(data);
        break;
    }
    case ON_SRRVER_MTU_CHANGED: {
        server_mtu_op_s* value = (server_mtu_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_mtu_changed_cb, handle->btm_handle, value->addr, value->mtu);
        free(value);
        free(data);
        break;
    }
    case ON_SERVER_NOTIFICATION_SENT: {
        server_notify_op_s* value = (server_notify_op_s*)(msg->data);
        BT_CBACK(handle->callbacks, _server_notify_sent_cb, handle->btm_handle, value->addr, value->status);
        free(value);
        free(data);
        break;
    }
    default: {
        BT_LOGW("invalid event:%d", msg->event);
        break;
    }
    }
}

static void send_msg(gatt_server_msg_t* msg)
{
    excute_service_context_t* context = (excute_service_context_t*)malloc(sizeof(excute_service_context_t));
    context->loop_func = handle_event;
    context->data = (void*)msg;
    context->data_size = sizeof(gatt_server_msg_t);
    process_in_loop(context);
}
