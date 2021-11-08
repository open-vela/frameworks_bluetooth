
#ifndef _SRV_INC_GATT_ADVERTISE_MANAGER_H
#define _SRV_INC_GATT_ADVERTISE_MANAGER_H

#include <nuttx/list.h>
#include <stddef.h>

#include "btm_manager.h"
#include "bts_common.h"


typedef void (*le_advertise_started_callback)(void* context, uint8_t adv_id);
typedef void (*le_advertise_stopped_callback)(void* context, uint8_t adv_id);
typedef void (*le_advertise_failed_callback)(uint8_t adv_id, int error);
typedef struct {
    le_advertise_started_callback _ble_advertise_started_cb;
    le_advertise_stopped_callback _ble_advertise_stopped_callback;
    le_advertise_failed_callback _ble_advertise_failed_callback;
} ble_advertiser_callbacks;

typedef struct {
    struct list_node node;

    uint8_t advertiser_id;
    advertise_param_t* param;
    ble_advertiser_callbacks* callbacks;

    void* btm_handle;
} advertise_hdl;

typedef void (*ble_advertise_started_callback)(uint8_t adv_id);
typedef void (*ble_advertise_stopped_callback)(uint8_t adv_id);
typedef struct {
    ble_advertise_started_callback ble_advtise_started_cb;
    ble_advertise_stopped_callback ble_advtise_stopped_cb;
} stack_le_advertise_callbacks;

typedef struct {
    size_t size;

    stack_le_advertise_callbacks* callbacks;
    bt_result_code (*start_adv)(advertise_hdl client);
    bt_result_code (*stop_adv)(uint8_t advertiser_id);
} gatt_advertise_interface_t;

const gatt_advertise_interface_t* get_ble_advertise_instance(void);
#endif