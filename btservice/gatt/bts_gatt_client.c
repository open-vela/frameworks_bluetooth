#include "bts_gatt_client.h"

#include <errno.h>
#include <string.h>

#include "bts_service.h"
#include "log.h"
#include "stack_adapter_gatt.h"

#define LOG_TAG "bts_gattc"

typedef struct
{
    enum {
        ON_CLIENT_CONNECT_STATE = 0,
        ON_CLIENT_SERVICE_DISCOVERED,
        ON_CLIENT_READ_RESULT,
        ON_CLIENT_WRITE_RESULT,
        ON_CLIENT_NOTIFY_REQUEST,
        ON_CLIENT_RSSI_READ,
        ON_CLIENT_PHY_READ,
        ON_CLIENT_PHY_UPDATE,
        ON_CLIENT_MTU_CHANGED,
    } event;

    bts_gattc_hdl_t* handle;
    size_t size;
    void* data;
} bts_gattc_msg_t;

typedef struct {
    gatt_element_t* element;
    uint16_t size
} bts_gattc_service_discover_s;

typedef struct bts_gatt_client {
    gatt_element_t* element;
    gatt_service_status_t status;
} bts_gattc_write_result_s;

typedef struct {
    gatt_element_t* element;
    uint8_t* value;
    uint16_t size;
} bts_gattc_notify_request_s;

typedef struct {
    int32_t rssi;
    gatt_service_status_t status
} bts_gattc_read_rssi_s;

typedef struct {
    ble_phy_type_t tx;
    ble_phy_type_t rx;
} bts_gattc_phy_type_s;

typedef struct
{
    gatt_element_t* element;
    uint8_t* value;
    uint16_t size;
    gatt_service_status_t status;
} bts_gattc_read_result_s;

static void send_msg(bts_gattc_msg_t* msg);
static void handle_event(void* data, size_t size);

static struct list_node gattc_list = LIST_INITIAL_VALUE(gattc_list);

static bts_gattc_hdl_t* find_gattc_handle(bd_addr_t addr)
{
    bts_gattc_hdl_t* gattc;
    list_for_every_entry(&gattc_list, gattc, bts_gattc_hdl_t, node)
    {
        if (!memcmp(gattc->remote_addr, addr, sizeof(bd_addr_t))) {
            return gattc;
        }
    }
    return NULL;
}

static bool add_gattc_handle(bts_gattc_hdl_t handle)
{
    bts_gattc_hdl_t* gattc = (bts_gattc_hdl_t*)malloc(sizeof(bts_gattc_hdl_t));
    CHECK_PTR_RETURN(gattc, false);
    memset(gattc, 0, sizeof(bts_gattc_hdl_t));

    gattc->callbacks = handle.callbacks;
    gattc->btm_handle = handle.btm_handle;
    memcpy(gattc->remote_addr, handle.remote_addr, sizeof(bd_addr_t));
    list_add_tail(&gattc_list, &gattc->node);
    return true;
}

static bool remove_gatt_client(bts_gattc_hdl_t* gattc)
{
    list_delete(&gattc->node);
    free(gattc);
    return true;
}

static bts_gattc_msg_t* create_adp_msg(uint8_t event, bts_gattc_hdl_t* handle, void* data, size_t size)
{
    bts_gattc_msg_t* msg = (bts_gattc_msg_t*)malloc(sizeof(bts_gattc_msg_t));
    CHECK_PTR_RETURN(msg, NULL);

    if (size < 0) {
        BT_LOGE("fail, invlaid size:%d", size);
        return NULL;
    }
    msg->event = event;
    msg->handle = handle;
    msg->size = size;
    if (size == 0) {
        return msg;
    }

    void* value = malloc(size);
    if (!value) {
        BT_LOGE("fail, malloc data");
        free(msg);
        return NULL;
    }
    memcpy(value, data, size);
    msg->data = value;

    return msg;
}

static void on_client_connection_state_changed(bd_addr_t remote_addr, bt_state_t state)
{
    BT_LOGD("%s, remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x] state:%d", __func__, remote_addr[0],
        remote_addr[1], remote_addr[2], remote_addr[3], remote_addr[4], remote_addr[5], state);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_CONNECT_STATE, handle, &state, sizeof(bt_state_t));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_service_discovered(bd_addr_t remote_addr, gatt_element_t* element,
    uint16_t size)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_service_discover_s data;
    gatt_element_t* items = (gatt_element_t*)malloc(sizeof(gatt_element_t) * size);
    for (size_t index = 0; index < size; index++, items++, element++) {
        items->id = element->id;
        memcpy(items->uuid, element->uuid, sizeof(BT_UUID_T));
        items->type = element->type;
        items->properties = element->properties;
        items->permissions = element->permissions;
    }
    data.element = items - size;
    data.size = size;
    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_SERVICE_DISCOVERED, handle, &data, sizeof(bts_gattc_service_discover_s));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_read_result(bd_addr_t remote_addr, gatt_element_t* element, uint8_t* value,
    uint16_t size, gatt_service_status_t status)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_read_result_s data;
    data.element = (gatt_element_t*)malloc(sizeof(gatt_element_t));
    memcpy(data.element, element, sizeof(gatt_element_t));
    data.value = (uint8_t*)malloc(sizeof(uint8_t) * size);
    memcpy(data.value, value, size);
    data.size = size;
    data.status = status;
    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_READ_RESULT, handle, &data, sizeof(bts_gattc_read_result_s));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_write_result(bd_addr_t remote_addr, gatt_element_t* element,
    gatt_service_status_t status)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_write_result_s data;
    data.element = (gatt_element_t*)malloc(sizeof(gatt_element_t));
    memcpy(data.element, element, sizeof(gatt_element_t));
    data.status = status;
    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_WRITE_RESULT, handle, &data, sizeof(bts_gattc_write_result_s));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_nofity_request(bd_addr_t remote_addr, gatt_element_t* element,
    uint8_t* value, uint16_t size)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_notify_request_s data;
    data.element = (gatt_element_t*)malloc(sizeof(gatt_element_t));
    memcpy(data.element, element, sizeof(gatt_element_t));
    data.value = (uint8_t*)malloc(sizeof(uint8_t) * size);
    memcpy(data.value, value, size);
    data.size = size;
    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_NOTIFY_REQUEST, handle, &data, sizeof(bts_gattc_notify_request_s));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_rssi_read(bd_addr_t remote_addr, int32_t rssi,
    gatt_service_status_t status)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_read_rssi_s data;
    data.rssi = rssi;
    data.status = status;
    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_RSSI_READ, handle, &data, sizeof(bts_gattc_read_rssi_s));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_phy_read(bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_phy_type_s data;
    data.tx = tx;
    data.rx = rx;
    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_PHY_READ, handle, &data, sizeof(bts_gattc_phy_type_s));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_phy_update(bd_addr_t remote_addr, ble_phy_type_t tx, ble_phy_type_t rx)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_phy_type_s data;
    data.tx = tx;
    data.rx = rx;
    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_PHY_UPDATE, handle, &data, sizeof(bts_gattc_phy_type_s));
    CHECK_PTR(msg);
    send_msg(msg);
}

static void on_client_mtu_changed(bd_addr_t remote_addr, uint32_t mtu)
{
    BT_LOGD("%s", __func__);
    bts_gattc_hdl_t* handle = find_gattc_handle(remote_addr);
    CHECK_PTR(handle);

    bts_gattc_msg_t* msg = create_adp_msg(ON_CLIENT_MTU_CHANGED, handle, &mtu, sizeof(uint32_t));
    CHECK_PTR(msg);
    send_msg(msg);
}

static const stack_gatt_client_callbacks gatt_client_cbs = {
    .size = sizeof(gatt_client_cbs),

    .gatt_client_connection_state_changed_cb = on_client_connection_state_changed,
    .gatt_client_service_discovered_cb = on_client_service_discovered,
    .gatt_client_element_read_cb = on_client_read_result,
    .gatt_client_element_written_cb = on_client_write_result,
    .gatt_client_element_changed_cb = on_client_nofity_request,
    .gatt_client_remote_rssi_read_cb = on_client_rssi_read,
    .gatt_client_phy_read_cb = on_client_phy_read,
    .gatt_client_phy_update_cb = on_client_phy_update,
    .gatt_client_mtu_changed_cb = on_client_mtu_changed,
};

static bt_result_code gatt_client_connect(bts_gattc_hdl_t handle)
{
    BT_LOGD("%s, remote_addr:[%02x:%02x:%02x:%02x:%02x:%02x]", __func__, handle.remote_addr[0],
        handle.remote_addr[1], handle.remote_addr[2], handle.remote_addr[3], handle.remote_addr[4], handle.remote_addr[5]);
    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_connect(handle.remote_addr, &gatt_client_cbs);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle connect, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    bool ret2 = add_gattc_handle(handle);
    if (!ret2) {
        BT_LOGE("fail, add_gattc_handle, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_disconnect(bd_addr_t addr)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_disconnect(handle->remote_addr);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle connect, err:%d", ret);
        remove_gatt_client(handle);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_discover_services(bd_addr_t addr, bt_uuid_t uuid)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret;
    bt_uuid_t null_uuid;
    memset(&null_uuid, 0, sizeof(bt_uuid_t));
    if (!memcmp(null_uuid, uuid, sizeof(bt_uuid_t))) {
        BT_LOGD("discover all service");
        ret = service_adapter_gatt_client_discover_services(handle->remote_addr);
    } else {
        ret = service_adapter_gatt_client_discover_service(handle->remote_addr, uuid);
    }

    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt discover service, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_read_request(bd_addr_t addr, gatt_element_t* element)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_read_element(handle->remote_addr, element);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle read, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_write_request(bd_addr_t addr, gatt_element_t* element, uint8_t* value,
    uint16_t length)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_write_element(handle->remote_addr, element, value, length);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle write, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_register_notification(bd_addr_t addr, gatt_element_t* element, bool enable)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_register_notifications(handle->remote_addr, element, enable);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle register notify, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_read_rssi(bd_addr_t addr)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_read_remote_rssi(handle->remote_addr);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle read rssi, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_read_phy(bd_addr_t addr)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_read_phy(handle->remote_addr);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle read phy, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_update_phy(bd_addr_t addr, ble_phy_type_t tx_phy, ble_phy_type_t rx_phy)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_set_phy(handle->remote_addr, tx_phy, rx_phy);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle update phy, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_update_mtu(bd_addr_t addr, uint32_t mtu)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_set_mtu(handle->remote_addr, mtu);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt handle update mtu, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static bt_result_code gatt_client_update_connection_parameter(bd_addr_t addr, uint32_t min_interval, uint32_t max_interval,
    uint32_t latency, uint32_t timeout, uint32_t min_connection_event_length, uint32_t max_connection_event_length)
{
    bts_gattc_hdl_t* handle = find_gattc_handle(addr);
    CHECK_PTR_RETURN(handle, BT_RESULT_FAILED);

    SERVICE_GATT_STATUS ret = service_adapter_gatt_client_update_connection_parameter(handle->remote_addr, min_interval, max_interval,
        latency, timeout, min_connection_event_length, max_connection_event_length);
    if (ret != GATT_SUCCESS) {
        BT_LOGE("fail, gatt update conn para, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

static const bts_gattc_interface_t gatt_client_intance = {
    .size = sizeof(gatt_client_intance),

    .connect = gatt_client_connect,
    .disconnect = gatt_client_disconnect,
    .discover_services = gatt_client_discover_services,
    .read_request = gatt_client_read_request,
    .write_request = gatt_client_write_request,
    .register_notification = gatt_client_register_notification,
    .read_rssi = gatt_client_read_rssi,
    .read_phy = gatt_client_read_phy,
    .update_phy = gatt_client_update_phy,
    .update_mtu = gatt_client_update_mtu,
    .update_connection_parameter = gatt_client_update_connection_parameter,
};

const bts_gattc_interface_t* get_bts_gattc_instance()
{
    return &gatt_client_intance;
}

static void handle_event(void* data, size_t size)
{
    BT_LOGD("%s", __func__);
    bts_gattc_msg_t* msg = (bts_gattc_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }
    bts_gattc_hdl_t* handle = (bts_gattc_hdl_t*)(msg->handle);
    if (!handle) {
        BT_LOGE("%s fail, handle null", __func__);
        return;
    }

    switch (msg->event) {
    case ON_CLIENT_CONNECT_STATE: {
        bt_state_t* state = (bt_state_t*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_connection_state_changed_cb, handle->btm_handle, *state);
        if (state == SERVICE_PROFILE_DISCONNECTED) {
            BT_LOGD("remove_gatt_client handle");
            remove_gatt_client(handle);
        }
        break;
    }
    case ON_CLIENT_SERVICE_DISCOVERED: {
        bts_gattc_service_discover_s* data = (bts_gattc_service_discover_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_service_discovered_cb, handle->btm_handle, data->element, data->size);
        free(data->element);
        break;
    }
    case ON_CLIENT_READ_RESULT: {
        bts_gattc_read_result_s* data = (bts_gattc_read_result_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_read_result_cb, handle->btm_handle, data->element, data->value, data->size, data->status);
        free(data->element);
        free(data->value);
        break;
    }
    case ON_CLIENT_WRITE_RESULT: {
        bts_gattc_write_result_s* data = (bts_gattc_write_result_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_write_result_cb, handle->btm_handle, data->element, data->status);
        free(data->element);
        break;
    }
    case ON_CLIENT_NOTIFY_REQUEST: {
        bts_gattc_notify_request_s* data = (bts_gattc_notify_request_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_nofity_request_cb, handle->btm_handle, data->element, data->value, data->size);
        free(data->element);
        free(data->value);
        break;
    }
    case ON_CLIENT_RSSI_READ: {
        bts_gattc_read_rssi_s* data = (bts_gattc_read_rssi_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_rssi_read_cb, handle->btm_handle, data->rssi, data->status);
        break;
    }
    case ON_CLIENT_PHY_READ: {
        bts_gattc_phy_type_s* data = (bts_gattc_phy_type_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_phy_read_cb, handle->btm_handle, data->tx, data->rx);
        break;
    }
    case ON_CLIENT_PHY_UPDATE: {
        bts_gattc_phy_type_s* data = (bts_gattc_phy_type_s*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_phy_update_cb, handle->btm_handle, data->tx, data->rx);
        break;
    }
    case ON_CLIENT_MTU_CHANGED: {
        uint32_t* mtu = (uint32_t*)(msg->data);
        BT_CBACK(handle->callbacks, bts_gattc_mtu_changed_cb, handle->btm_handle, *mtu);
        break;
    }
    default: {
        BT_LOGW("invalid event:%d", msg->event);
        break;
    }
    }
    if (msg->size > 0)
        free(msg->data);
    free(msg);
}

static void send_msg(bts_gattc_msg_t* msg)
{

    excute_service_context_t* context = (excute_service_context_t*)malloc(sizeof(excute_service_context_t));
    context->loop_func = handle_event;
    context->data = (void*)msg;
    context->data_size = sizeof(bts_gattc_msg_t);
    process_in_loop(context);
}
