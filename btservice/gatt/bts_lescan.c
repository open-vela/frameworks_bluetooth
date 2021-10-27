#include "bts_lescan.h"

#include <errno.h>
#include <string.h>

#include "bts_service.h"
#include "stack_adapter_gatt.h"

typedef struct
{
    enum {
        ON_SCAN_RESULT = 0,
    } event;

    scan_hdl_t* handle;
    size_t size;
    void* data;
} gatt_lescan_msg_t;

static void send_msg(gatt_lescan_msg_t* msg);
static void handle_event(void* data, size_t size);

static struct list_node scanner_list = LIST_INITIAL_VALUE(scanner_list);

static scan_hdl_t* find_scan_handle(uint8_t scanner_id)
{
    scan_hdl_t* client;
    list_for_every_entry(&scanner_list, client, scan_hdl_t, node)
    {
        if (client->scanner_id == scanner_id) {
            return client;
        }
    }
    return NULL;
}

static void add_scan_handle(scan_hdl_t scanner)
{
    scan_hdl_t* client = (scan_hdl_t*)malloc(sizeof(scan_hdl_t));
    if (!client) {
        BT_LOGE("malloc client fail");
        return;
    }

    client->scanner_id = scanner.scanner_id;
    client->filter = scanner.filter;
    client->settings = scanner.settings;
    client->callbacks = scanner.callbacks;
    list_add_tail(&scanner_list, &client->node);
}

static bool remove_scan_handle(scan_hdl_t* scanner)
{
    list_delete(&scanner->node);
    free(scanner);
    return true;
}

static uint8_t generate_scanner_id()
{
    uint8_t found = 0;
    scan_hdl_t* client;
    for (uint8_t i = 1; i < 256; i++) {
        list_for_every_entry(&scanner_list, client, scan_hdl_t, node)
        {
            if (client->scanner_id == i) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return i;
        }
    }
    BT_LOGE("client id overflow");
    return 0;
}

static bt_result_code start_scan(scan_hdl_t client)
{
    SERVICE_BT_STATUS ret = service_adapter_gap_set_ble_scan_filter(&client.filter);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble scan filter fail, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    ret = service_adapter_gap_set_ble_scan_parameters(&client.settings);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble scan parameters fail, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    ret = service_adapter_gap_start_ble_scan();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble start scan, err:%d", ret);
        return BT_RESULT_FAILED;
    }

    client.scanner_id = generate_scanner_id();
    add_scan_handle(client);

    BT_CBACK(client.callbacks, _ble_scan_started_cb, client.mgr_ctx, client.scanner_id);
    return BT_RESULT_SUCCESS;
}

static bt_result_code stop_scan(uint8_t scanner_id)
{
    scan_hdl_t* client = find_scan_handle(scanner_id);
    if (!client) {
        BT_LOGE("fail, scanner_id invalid ");
        return BT_RESULT_FAILED;
    }
    remove_scan_handle(client);

    SERVICE_BT_STATUS ret = service_adapter_gap_stop_ble_scan();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("set ble stop scan, err:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

void on_ble_scan_result(const scan_result_t* scan_result_data)
{
    scan_hdl_t* client;
    list_for_every_entry(&scanner_list, client, scan_hdl_t, node)
    {
        gatt_lescan_msg_t* msg = (gatt_lescan_msg_t*)malloc(sizeof(gatt_lescan_msg_t));
        memset(msg, 0, sizeof(gatt_lescan_msg_t));
        msg->event = ON_SCAN_RESULT;
        msg->handle = client;
        // scan_result_t* data = (scan_result_t*)malloc(sizeof(scan_result_t));
        // uint8_t* adv_data = (uint8_t*)malloc(scan_result_data->length);
        // data->adv_data = adv_data;
        // memcpy(data, scan_result_data, sizeof(scan_result_t));
        // memcpy(data->adv_data, scan_result_data->adv_data, sizeof(scan_result_data->length));

        send_msg(msg);
    }
}

static const stack_le_scan_callbacks le_scanner_cbs = {
    .ble_scan_result = on_ble_scan_result,
};

static const gatt_scan_interface_t ble_scan_intance = {
    .size = sizeof(gatt_scan_interface_t),

    .callbacks = &le_scanner_cbs,
    .start_scan = start_scan,
    .stop_scan = stop_scan,
};

// static stack_bluetooth_adapter* bluetooth_adapter = NULL;

// static stack_ble_scanner_interface* stack_scanner_interface = NULL;

const gatt_scan_interface_t* get_ble_scan_instance()
{
    // stack_bluetooth_adapter* bluetooth_adapter = get_stack_bluetooth_adapter();
    // if (!bluetooth_adapter) {
    //     BT_LOGE("fail, get_stack_bluetooth_adapter NULL");
    //     return NULL;
    // }

    // stack_bt_gatt_interface* bt_gatt = (stack_bt_gatt_interface*)bluetooth_adapter->get_profile_interface(BT_PROFILE_GATT_ID);
    // if (!bt_gatt) {
    //     BT_LOGE("fail, get_profile_interface NULL");
    //     return NULL;
    // }

    // stack_scanner_interface = bt_gatt->scanner;
    return &ble_scan_intance;
}

static void handle_event(void* data, size_t size)
{
    BT_LOGD("%s", __func__);
    gatt_lescan_msg_t* msg = (gatt_lescan_msg_t*)(data);
    if (!msg) {
        BT_LOGE("%s fail, msg null", __func__);
        return;
    }

    switch (msg->event) {
    case ON_SCAN_RESULT: {
        scan_hdl_t* handle = (scan_hdl_t*)(msg->handle);
        scan_result_t* scan_result_data = (scan_result_t*)(msg->data);
        BT_CBACK(handle->callbacks, _ble_scan_result_cb, handle->mgr_ctx, scan_result_data);
        free(scan_result_data->adv_data);
        free(data);
        break;
    }

    default:
        break;
    }
}

static void send_msg(gatt_lescan_msg_t* msg)
{
    excute_service_context_t* context = (excute_service_context_t*)malloc(sizeof(excute_service_context_t));
    context->loop_func = handle_event;
    context->data = (void*)msg;
    context->data_size = sizeof(gatt_lescan_msg_t);
    process_in_loop(context);
}