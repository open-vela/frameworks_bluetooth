#pragma once

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "btm_manager.h"
#include "uv.h"
/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/*process command data from manager, this is in the other thread,
 *need send message to service loop
 */ 
typedef void (*bts_process_command_func_in_service) (void *data, size_t data_size);

typedef void (*bts_process_loop_data) (void *data, size_t data_size);

//process block command in work loop
typedef void (*bts_process_int_work_func) (int command_id, char *data, size_t data_size);

typedef void (*process_in_io) (char * data);
typedef void (*process_in_timer) (char * data);

typedef struct excute_manager_context
{
  void * handle;
  size_t data_size;
  char * data;
  int profile_id;
}excute_manager_context_t;

typedef struct excute_service_context
{
  void * handle;
  size_t data_size;
  char * data;
  int command_id;
  bts_process_loop_data loop_func;
}excute_service_context_t;

bt_result_code  register_process_func_to_service(bt_profile_id profile_id, 
                                        bts_process_command_func_in_service func);


void process_in_work_thread( process_in_io func_in_io,  void * data);

uv_timer_t *start_timer(int timeout, int repeat, process_in_timer timer_callback, void * data);
void stop_timer(uv_timer_t * timer);

void process_in_loop(excute_service_context_t *context) ;

char* package_profile_buffer_to_manager(int command_id, void* command_buffer, int command_size, int * profile_size);

bt_result_code send_pb_command_buffer_to_manager(bt_profile_id profile_id, void* profile_buff, size_t profile_size);
#ifdef CONFIG_BLUETOOTH_LOCAL_THREAD
void process_data_from_manager(int profile_id, char * buff, size_t size) ;
#endif
int bts_service_get_interface(void* handle);

int bts_service_init();

