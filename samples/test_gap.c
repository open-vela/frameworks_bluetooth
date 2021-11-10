
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

 }

static bt_mgr_callback_t mgt_cb = {
    .bt_manager_state_changed_callback_cb = manager_state_changed_callback,
    .init_status_changed_callback_cb = manager_init_status_changed_callback,

};

void test_discovery_state_changed_callback(bt_discovery_state state)
{

}
void test_adapter_state_changed_callback(stack_state_t state)
{
  
    BT_LOGD("%s", __func__);
    char local_name[] = "BLUELET_NUTTX_GAP_TEST";
    gap_test_interface->set_name(manager_handle, local_name, sizeof(local_name));
    
}

const btm_gap_callbacks_t gap_test_callbacks = 
{
    .discovery_state_changed_callback_cb = test_discovery_state_changed_callback,
    .state_changed_cb = test_adapter_state_changed_callback,
};

int main(int argc, FAR char* argv[])
{
    btm_interface_t* manager = get_bt_manager_interface();
    void *gap_hanlde;
    manager->init(&manager_handle, &mgt_cb);
        
    gap_test_interface = get_gap_instance();
    gap_test_interface->register_callbacks(manager_handle, &gap_hanlde, &gap_test_callbacks);
    
    //gap_test_interface->start_discovery(gap_hanlde, 500);

    manager->enable(manager_handle);




    while (1){
        usleep(1000);
    }

    return 0;
}