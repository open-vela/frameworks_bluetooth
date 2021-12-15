#include "btm_manager.h"

typedef void (*bts_service_if_adapter_state_changed_callback)(void* handle, BTM_BT_STATE state);
typedef void (*bts_service_if_adapter_ble_state_changed_callback)(void* handle, BTM_BLE_STATE state);

typedef struct {
    size_t size;
    bts_service_if_adapter_state_changed_callback adapter_state_changed_cb;
    bts_service_if_adapter_ble_state_changed_callback adapter_state_ble_changed_cb;
} bt_service_if_callbacks;

typedef struct {
    size_t size;
    BT_RESULT_CODE (*init)(void* handle, bt_service_if_callbacks* callbacks);
    BT_RESULT_CODE (*enable)(void* handle);
    BT_RESULT_CODE (*disable)(void* handle);
    BT_RESULT_CODE (*enable_ble)(void* handle);
    void (*cleanup)(void* handle);
    BTM_BT_STATE (*bt_get_state)(void* handle);
    BTM_BLE_STATE (*ble_get_state)(void* handle);
    const void* (*get_profile_interface)(const char* profile_id);
    void (*stack_state_change)(bt_service_state state);

} bluetooth_service_interface;

const bluetooth_service_interface* get_bluetooth_service_interface(void);
