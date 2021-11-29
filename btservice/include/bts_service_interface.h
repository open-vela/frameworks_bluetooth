#include "btm_manager.h"
#include "bts_common.h"

typedef void (*bts_service_if_adapter_state_changed_callback)(void* handle, bt_manager_bt_state state);
typedef void (*bts_service_if_adapter_ble_state_changed_callback)(void* handle, bt_manager_ble_state state);

typedef struct {
    size_t size;
    bts_service_if_adapter_state_changed_callback adapter_state_changed_cb;
    bts_service_if_adapter_ble_state_changed_callback adapter_state_ble_changed_cb;
} bt_service_if_callbacks;

typedef struct {
    size_t size;
    bt_result_code (*init)(void* handle, bt_service_if_callbacks* callbacks);
    bt_result_code (*enable)(void* handle);
    bt_result_code (*disable)(void* handle);
    bt_result_code (*enable_ble)(void* handle);
    void (*cleanup)(void* handle);
    bt_manager_bt_state (*bt_get_state)(void* handle);
    bt_manager_ble_state (*ble_get_state)(void* handle);
    const void* (*get_profile_interface)(const char* profile_id);
    void (*stack_state_change)(bt_service_state state);

} bluetooth_service_interface;

const bluetooth_service_interface* get_bluetooth_service_interface(void);
