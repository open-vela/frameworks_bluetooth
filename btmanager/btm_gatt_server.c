
#include "btm_gatt_server.h"
#include "btm_manager.h"
#include "bts_gatt.h"
#include "bts_gatts.h"

#include "log.h"

#define LOG_TAG "btm_gatts"

static btm_interface_t* bt_mgr_interface = NULL;

static gatt_server_interface_t* server_interface = NULL;

static void on_server_connection_state_changed(gatt_server_t* handle, bd_addr_t remote_addr, bt_state_t state)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_connection_state_changed_cb, handle, remote_addr, state);
}

static void on_server_opened_cb(gatt_server_t* handle, uint8_t server_if)
{
    CHECK_PTR(handle);
    handle->server_if = server_if;
    BT_CBACK(handle->callbacks, le_server_opened_cb, handle);
}

static void on_server_closed_cb(gatt_server_t* handle)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_closed_cb, handle);
}

static void on_server_element_added(gatt_server_t* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_service_added_cb, handle, status, element, size);
}

static void on_server_element_removed(gatt_server_t* handle, gatt_service_status_t status, gatt_element_t* element,
    size_t size)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_service_removed_cb, handle, status, element, size);
}

static void on_server_phy_read(gatt_server_t* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_phy_read_cb, handle, remote_addr, tx, rx);
}

static void on_server_phy_update(gatt_server_t* handle, bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx, gatt_service_status_t status)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_phy_update_cb, handle, remote_addr, tx, rx, status);
}

static void on_server_read_request(gatt_server_t* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_read_request_cb, handle, remote_addr, request_id, element);
}

static void on_server_write_request(gatt_server_t* handle, bd_addr_t remote_addr, uint32_t request_id,
    gatt_element_t* element, uint8_t* value, uint16_t offset,
    uint16_t size)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_write_request_cb, handle, remote_addr, request_id, element, value, offset, size);
}

static void on_server_mtu_changed(gatt_server_t* handle, bd_addr_t remote_addr, uint32_t mtu)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_mtu_changed_cb, handle, remote_addr, mtu);
}

static void on_server_notify_sent(gatt_server_t* handle, bd_addr_t remote_addr, gatt_service_status_t status)
{
    CHECK_PTR(handle);
    BT_CBACK(handle->callbacks, le_server_notify_sent_cb, handle, remote_addr, status);
}

static ble_gatt_server_callbacks server_callbacks = {
    ._server_connection_state_changed_cb = on_server_connection_state_changed,
    ._server_opened_cb = on_server_opened_cb,
    ._server_closed_cb = on_server_closed_cb,
    ._server_elements_added_cb = on_server_element_added,
    ._server_elements_removed_cb = on_server_element_removed,
    ._server_phy_read_cb = on_server_phy_read,
    ._server_phy_update_cb = on_server_phy_update,
    ._server_read_request_cb = on_server_read_request,
    ._server_write_request_cb = on_server_write_request,
    ._server_mtu_changed_cb = on_server_mtu_changed,
    ._server_notify_sent_cb = on_server_notify_sent,
};

static bt_result_code gatt_server_open(gatt_server_t** handle_ptr, gatt_server_callbacks* callbacks)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);

    *handle_ptr = (gatt_server_t*)malloc(sizeof(gatt_server_t));
    memset(*handle_ptr, 0, sizeof(gatt_server_t));
    (*handle_ptr)->callbacks = callbacks;

    gatts_hdl_t server = {
        .callbacks = &server_callbacks,
        .mgr_ctx = *handle_ptr,
    };
    bt_result_code ret = server_interface->open_server(server);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, open_server err:%d", ret);
        return ret;
    }

    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_close(gatt_server_t* handle)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->close_server(handle->server_if);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, close_server err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_connect(gatt_server_t* handle, bd_addr_t remote_addr, bool auto_connect)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->connect(handle->server_if, remote_addr, auto_connect);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, connect err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_disconnect(gatt_server_t* handle, bd_addr_t remote_addr)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->disconnect(handle->server_if, remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, disconnect err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_add_service(gatt_server_t* handle, gatt_element_t* element, uint16_t size)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->add_element(handle->server_if, element, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, add_service err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_remove_service(gatt_server_t* handle, uint32_t* ids, uint16_t size)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->remove_element(handle->server_if, ids, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, remove_service err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_read_phy(gatt_server_t* handle, bd_addr_t remote_addr)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->read_phy(handle->server_if, remote_addr);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, read_phy err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_update_phy(gatt_server_t* handle, bd_addr_t remote_addr, ble_phy_type_t tx_type, ble_phy_type_t rx_type)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->update_phy(handle->server_if, remote_addr, tx_type, rx_type);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, update_phy err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_notify(gatt_server_t* handle, bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->send_notify(handle->server_if, remote_addr, characteristic, value, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, send_notify err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_indicate(gatt_server_t* handle, bd_addr_t remote_addr, gatt_element_t* characteristic, uint8_t* value,
    size_t size)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->send_indicate(handle->server_if, remote_addr, characteristic, value, size);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, send_indicate err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_server_send_response(gatt_server_t* handle, bd_addr_t remote_addr, gatt_response_t* response)
{
    CHECK_PTR_RETURN(server_interface, BT_RESULT_STATE_NOT_ON);
    bt_result_code ret = server_interface->send_response(handle->server_if, remote_addr, response);
    if (ret != BT_RESULT_SUCCESS) {
        BT_LOGE("fail, send_response err:%d", ret);
        return ret;
    }
    return BT_RESULT_SUCCESS;
}

static btm_le_gatts_interface_t le_gatts_interface = {
    .size = sizeof(le_gatts_interface),

    .open = gatt_server_open,
    .close = gatt_server_close,
    .connect = gatt_server_connect,
    .disconnect = gatt_server_disconnect,
    .add_service = gatt_server_add_service,
    .remove_service = gatt_server_remove_service,
    .read_phy = gatt_server_read_phy,
    .update_phy = gatt_server_update_phy,
    .send_notify = gatt_server_send_notify,
    .send_indicate = gatt_server_send_indicate,
    .send_response = gatt_server_send_response,
};

btm_le_gatts_interface_t* get_le_gatts_interface(
    void* bt_mgr)
{
    if (!bt_mgr) {
        BT_LOGE("fail, bt_mgr NULL");
        return NULL;
    }
    bt_mgr_interface = (btm_interface_t*)bt_mgr;
    gatt_interface_t* interface = bt_mgr_interface->get_profile_interface(BT_PROFILE_GATT);
    if (!interface) {
        BT_LOGE("fail, get_profile_interface gatt");
        return NULL;
    }

    server_interface = interface->server;
    return &le_gatts_interface;
}