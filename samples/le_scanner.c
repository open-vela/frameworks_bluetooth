
#include <stdio.h>

#include "mgr/bt_manager.h"
#include "btmanager/btm_le_scan.h"
#include "log.h"

static void on_scan_result(const scan_result_t* result)
{
    log_d("type:%d", result->);
}

static void on_scan_failed(int errorCode) { log_d("err:%d", errorCode); }

static void adapter_state_changed_callback(bt_state_t state)
{
    log_d("state:%d", state);
}

int main()
{
    btm_interface* manager = get_bt_manager_interface();
    const bt_callbacks manager_cb = {
        .size = sizeof(manager_cb),
        .adapter_state_changed_cb = adapter_state_changed_callback,
    };

    manager->init(&manager_cb);
    btm_le_scan_interface* scanner = get_le_scan_interface(manager);

    scan_filter filter;
    scan_settings setting;
    gatt_scan_callbacks callbacks = {
        .on_scan_failed = on_scan_failed,
        .on_scan_result = on_scan_result,
    };
    scanner->start_scan(filter, setting, &callbacks);

    getchar();
    return 0;
}