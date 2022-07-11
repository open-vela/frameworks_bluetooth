#define LOG_TAG "bts_service_interface"
 #include <assert.h>
#include <nuttx/list.h>
#include <stdio.h>
#include <stdlib.h>

#include "bts_a2dp_source.h"
#include "bts_a2dp_sink.h"
#include "bts_avrcp_target.h"
#include "bts_gap.h"
#include "bts_gatt_service.h"
#include "bts_hf_client.h"
#include "bts_hid_service.h"
#include "bts_service.h"
#include "bts_service_interface.h"
#include "bts_spp.h"
#include "bts_panu.h"
#include "bts_avrcp_ctrl.h"
#include "log.h"

typedef struct {
    struct list_node handle_list;
    bt_service_state bt_state;
    ble_service_state ble_state;
} bt_service_t;

typedef struct {
    struct list_node node;
    void* handle;
    bt_service_if_callbacks* callbacks;
} bt_if_handle_t;

bt_service_t* service = NULL;

static void bts_service_if_state_changed_callback(bt_service_state state)
{
    struct list_node* list = &service->handle_list;
    bt_if_handle_t* if_handle;
    struct list_node* node;

    if (!service)
        return;
    service->bt_state = state;
    list_for_every(list, node)
    {
        if_handle = (bt_if_handle_t*)node;
        if (if_handle->callbacks->adapter_state_changed_cb) {
            if_handle->callbacks->adapter_state_changed_cb(if_handle->handle, state);
        }
    }
}

static void bts_service_if_ble_state_changed_callback(ble_service_state state)
{
    struct list_node* list = &service->handle_list;
    bt_if_handle_t* if_handle;
    struct list_node* node;

    list_for_every(list, node)
    {
        if_handle = (bt_if_handle_t*)node;
        if (if_handle->callbacks->adapter_state_ble_changed_cb) {
            if_handle->callbacks->adapter_state_ble_changed_cb(if_handle->handle, state);
        }
    }
}

static bt_service_callbacks service_callback = {
    .adapter_state_changed_cb = bts_service_if_state_changed_callback,
    .adapter_state_ble_changed_cb = bts_service_if_ble_state_changed_callback,
};

static bt_result_code bts_if_init(void* handle, bt_service_if_callbacks* callbacks)
{
    if (!service) {
        service = (bt_service_t*)malloc(sizeof(bt_service_t));
        assert(service);
        list_initialize(&service->handle_list);
        assert(bts_service_init(&service_callback) == BT_RESULT_SUCCESS);

        service->ble_state = STATE_BLE_OFF;
        service->bt_state = BTM_STATE_OFF;
#if defined(CONFIG_BLUETOOTH_LE_SCAN) || defined(CONFIG_BLUETOOTH_LE_ADVERTISE) || defined(CONFIG_BLUETOOTH_GATT_CLIENT) || defined(CONFIG_BLUETOOTH_GATT_SERVER)
        const gatt_interface_t* gatt_if = gatt_get_interface();
        if (gatt_if) {
            BT_LOGD("gatt init");
            gatt_if->init();
        }
#endif

#if defined(CONFIG_BLUETOOTH_HIDDEV)
        const hid_interface_t* hid_if = hid_get_interface();
        if (hid_if) {
            BT_LOGD("hid init");
            hid_if->init();
        }
#endif
    }
    bt_if_handle_t* service_if_handle = (bt_if_handle_t*)malloc(sizeof(bt_if_handle_t));
    assert(service_if_handle);
    service_if_handle->handle = handle;
    service_if_handle->callbacks = callbacks;
    list_add_tail(&service->handle_list, &service_if_handle->node);
    return BT_RESULT_SUCCESS;
}

static bt_if_handle_t* find_if_handle_by_handle(void* handle)
{
    struct list_node* list = &service->handle_list;
    bt_if_handle_t* if_handle;
    struct list_node* node;

    list_for_every(list, node)
    {
        if_handle = (bt_if_handle_t*)node;
        if (if_handle->handle == handle)
            return if_handle;
    }

    return NULL;
}

static bt_result_code bts_if_enable(void* handle)
{
    bt_if_handle_t* if_handle;
    if (!handle)
        return BT_RESULT_FAILED;
    if_handle = find_if_handle_by_handle(handle);
    if (!if_handle)
        return BT_RESULT_FAILED;

    gap_enable();
#ifdef CONFIG_BLUETOOTH_AVRCP_TG
    avrcp_target_service_start();
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CT
    avrcp_ctrl_service_start();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    a2dp_sink_service_start();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    a2dp_source_service_start();
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    hf_client_service_start();
#endif
#ifdef CONFIG_BLUETOOTH_SPP
    spp_service_start();
#endif
#ifdef CONFIG_BLUETOOTH_PAN
    pan_service_start();
#endif
    return BT_RESULT_SUCCESS;
}

static bt_result_code bts_if_disable(void* handle)
{
    bt_if_handle_t* if_handle;
    if (!handle)
        return BT_RESULT_FAILED;
    if_handle = find_if_handle_by_handle(handle);
    if (!if_handle)
        return BT_RESULT_FAILED;
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    a2dp_sink_service_stop();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    a2dp_source_service_stop();
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    hf_client_service_stop();
#endif
#ifdef CONFIG_BLUETOOTH_SPP
    spp_service_stop();
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_TG
    avrcp_target_service_stop();
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CT
    avrcp_ctrl_service_stop();
#endif
#ifdef CONFIG_BLUETOOTH_PAN
    pan_service_stop();
#endif
    gap_disable(true);
    return BT_RESULT_SUCCESS;
}

static void bts_if_cleanup(void* handle)
{
    bt_if_handle_t* if_handle;
    if (!handle)
        return;
    if_handle = find_if_handle_by_handle(handle);
    if (!if_handle)
        return;
    service->bt_state = BTM_STATE_TURNING_ON;
    bts_service_cleanup();
#if defined(CONFIG_BLUETOOTH_LE_SCAN) || defined(CONFIG_BLUETOOTH_LE_ADVERTISE) || defined(CONFIG_BLUETOOTH_GATT_CLIENT) || defined(CONFIG_BLUETOOTH_GATT_SERVER)
    const gatt_interface_t* gatt_if = gatt_get_interface();
    if (!gatt_if) {
        BT_LOGD("gatt cleanup");
        gatt_if->cleanup();
    }
#endif
#if defined(CONFIG_BLUETOOTH_HIDDEV)
    const hid_interface_t* hid_if = hid_get_interface();
    if (hid_if) {
        BT_LOGD("hidd cleanup");
        hid_if->cleanup();
    }
#endif
}

static bool is_profile(const char* p1, const char* p2)
{
    if (!p2) {
        BT_LOGE("fail, p2 nullptr");
        return false;
    }
    if (!p1) {
        BT_LOGE("fail, p1 nullptr");
        return false;
    }
    return strlen(p1) == strlen(p2) && strncmp(p1, p2, strlen(p2)) == 0;
}

static const void* if_get_profile_interface(const char* profile_id)
{
    /* sanity check */
#if defined(CONFIG_BLUETOOTH_LE_SCAN) || defined(CONFIG_BLUETOOTH_LE_ADVERTISE) || defined(CONFIG_BLUETOOTH_GATT_CLIENT) || defined(CONFIG_BLUETOOTH_GATT_SERVER)
    if (is_profile(profile_id, BT_PROFILE_GATT))
        return gatt_get_interface();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SINK
    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_SINK))
        return get_a2dp_sink_service_interface();
#endif
#ifdef CONFIG_BLUETOOTH_A2DP_SRC
    if (is_profile(profile_id, BT_PROFILE_ADVANCED_AUDIO_SOURCE))
        return get_a2dp_source_service_interface();
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_TG
    if (is_profile(profile_id, BT_PROFILE_AV_RC_TARGET))
        return get_avrcp_tg_service_interface();
#endif
#ifdef CONFIG_BLUETOOTH_AVRCP_CT
    if (is_profile(profile_id, BT_PROFILE_AV_RC_CTRL))
        return get_avrcp_ctrl_service_interface();
#endif
#ifdef CONFIG_BLUETOOTH_HFP_HF
    if (is_profile(profile_id, BT_PROFILE_HANDSFREE_HF))
        return (const void*)get_hf_client_service_interface();
#endif
#ifdef CONFIG_BLUETOOTH_SPP
    if (is_profile(profile_id, BT_PROFILE_SPP))
        return (const void*)get_spp_service_interface();
#endif
#ifdef CONFIG_BLUETOOTH_PAN
    if (is_profile(profile_id, BT_PROFILE_PAN))
        return get_pan_service_interface();
#endif
#if defined(CONFIG_BLUETOOTH_HIDDEV)
    if (is_profile(profile_id, BT_PROFILE_HIDDEV))
        return hid_get_interface();
#endif
    return NULL;
}

static btm_bt_state if_get_state(void* handle)
{
    if (!service)
        return BTM_STATE_OFF;
    return service->bt_state;
}

static btm_ble_state if_get_ble_state(void* handle)
{
    if (!service)
        return BTM_STATE_OFF;
    if (service->bt_state != BTM_STATE_OFF)
        return STATE_BLE_ON;
    else
        return service->ble_state;
}

static void bts_if_stack_state_change(bt_service_state state)
{
    stack_state_change(state);
}

static bluetooth_service_interface bluetooth_service = {
    .size = sizeof(bluetooth_service),
    .init = bts_if_init,
    .enable = bts_if_enable,
    .disable = bts_if_disable,
    .cleanup = bts_if_cleanup,
    .bt_get_state = if_get_state,
    .ble_get_state = if_get_ble_state,
    .get_profile_interface = if_get_profile_interface,
    .stack_state_change = bts_if_stack_state_change,
};

const bluetooth_service_interface* get_bluetooth_service_interface()
{
    return &bluetooth_service;
}
