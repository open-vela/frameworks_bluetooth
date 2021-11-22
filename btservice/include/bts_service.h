#ifndef __BTS_SERVICE_H__
#define __BTS_SERVICE_H__

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "bts_common.h"
#include "btm_manager.h"
#include "uv.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*process command data from manager, this is in the other thread,
 *need send message to service loop
 */ 

typedef void (*bts_process_loop_data) (void *data, size_t data_size);

//process block command in work loop
typedef void (*bts_process_int_work_func) (int command_id, char *data, size_t data_size);

typedef void (*process_in_io) (char * data);
typedef void (*process_in_timer) (char * data);

typedef struct excute_manager_context{
  void * handle;
  size_t data_size;
  char * data;
  int profile_id;
}excute_manager_context_t;

typedef struct excute_service_context{
  void * handle;
  size_t data_size;
  char * data;
  int command_id;
  bts_process_loop_data loop_func;
  int profile_id;
}excute_service_context_t;

typedef struct {
    bt_profile_id profile;
    uint16_t event;
    void* data;
    size_t size;
} bts_uv_msg_t;
typedef void (*bts_profile_callbacks)(bt_profile_id id, void* data, size_t size);

typedef bt_manager_bt_state bt_service_state;
typedef bt_manager_ble_state ble_service_state;

typedef void (*bts_service_adapter_state_changed_callback)(bt_service_state state);
typedef void (*bts_service_adapter_ble_state_changed_callback)(ble_service_state state);

typedef struct {
    size_t size;
    bts_service_adapter_state_changed_callback adapter_state_changed_cb;
    bts_service_adapter_ble_state_changed_callback adapter_state_ble_changed_cb;
} bt_service_callbacks;

uv_poll_t* bts_uv_poll_start(int fd, int pevents, uv_poll_cb cb);
void bts_uv_poll_stop(uv_poll_t* handle);
void process_in_work_thread( process_in_io func_in_io,  void * data);
uv_timer_t *start_timer(int timeout, int repeat, process_in_timer timer_callback, void * data);
void stop_timer(uv_timer_t * timer);

int bts_service_get_interface(void* handle);
uv_loop_t *get_service_loop(void);

bt_result_code bts_service_init(bt_service_callbacks* callbacks);
bt_result_code bts_service_bt_enable(void);
bt_result_code bts_service_bt_disable(void);
bt_result_code bts_service_ble_enable(void);
bt_result_code bts_service_ble_disable(void);
bt_service_state bts_service_bt_get_state(void);
ble_service_state bts_service_ble_get_state(void);
void bts_service_cleanup(void);
void stack_state_change(stack_state_t state);
bool bts_send_uv_msg(bt_profile_id id,  void* data, size_t size);
bool bts_register_profile_process(bt_profile_id id, bts_profile_callbacks cb);
bool bts_unregister_profile_process(bt_profile_id id);
#endif
