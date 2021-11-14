
#include <stdio.h>
#include <pthread.h>
#include "btm_le_advertise.h"
#include "btm_manager.h"
#include "log.h"
#include "bts_spp.h"
#define LOG_TAG "bt_sample_gap"
#include "log.h"
#include "btm_gap.h"

 void *manager_handle = NULL;
void *gap_hanlde = NULL;
btm_gap_interface_t * gap_test_interface = NULL;
 void manager_init_status_changed_callback(bt_result_code status)
 {

 }

 void manager_state_changed_callback(bt_manager_bt_state state)
 {

    BT_LOGD("%s", __func__);
    char local_name[] = "BLUELET_NUTTX_GAP_TEST";
    gap_test_interface->set_name(gap_hanlde, local_name, sizeof(local_name));
 }

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
    .init_status_changed_callback_cb = manager_init_status_changed_callback,

};

void test_discovery_state_changed_callback(bt_discovery_state state)
{
    BT_LOGD("%s", __func__);

}
void test_adapter_state_changed_callback(stack_state_t state)
{
     BT_LOGD("%s", __func__);

}
void test_device_found_callback(bt_device_t* device)
{
     BT_LOGD("%s, device %02x%02x%02x%02x%02x%02x", __func__, device->addr[0], device->addr[1],device->addr[2],device->addr[3],device->addr[4],device->addr[5]);

}

const btm_gap_callbacks_t gap_test_callbacks = 
{
    .discovery_state_changed_callback_cb = test_discovery_state_changed_callback,
    .state_changed_cb = test_adapter_state_changed_callback,
    .discovery_state_changed_callback_cb = test_discovery_state_changed_callback,
    .device_found_callback_cb = test_device_found_callback,
};

int main(int argc, FAR char* argv[])
{
    btm_interface_t* manager = get_bt_manager_interface();
    manager->init(&manager_handle, &mgt_cb);
        
    gap_test_interface = get_gap_instance();
    gap_test_interface->register_callbacks(manager_handle, &gap_hanlde, &gap_test_callbacks);
    
    manager->enable(manager_handle);
    char input;
    bool exit = false;
    while (!exit)
    {
        printf("please input command:\n");
        input = getchar();
        switch (input)
        {
            case 'e':{
                printf("gap test exit:\n");
                exit = true;
                break;
            }
            case 'd':{
                printf("gap test start discovery :\n");
                gap_test_interface->start_discovery(gap_hanlde, 5000);
                break;
            }
            case 's':{
                printf("gap test stop discovery :\n");
                gap_test_interface->stop_discovery(gap_hanlde);
            }
            default:
            break;
        }
    }

    return 0;
}