/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>

#include "adapter_internel.h"
#include "bluetooth_ipc.h"
#include "bt_adapter.h"
#include "btservice.h"
#include "service_loop.h"

#include "utils/log.h"

int main(int argc, char** argv)
{
    int ret;

    syslog(LOG_INFO, "bluetoothd main %d\n", __LINE__);

    ret = service_loop_init();
    if (ret != 0)
        goto out;

    ret = bt_service_init();
    if (ret != 0)
        goto out;

    bluetooth_ipc_add_services();

    /* add ipc fd to service loop or join main thread */
    /*
       blocked: libuv setup poll need change file to nonblock mode,
       but binder transact need block mode
    */
#ifdef CONFIG_BLUETOOTH_IPC_JOIN_LOOP
    bluetooth_ipc_join_service_loop();
    ret = service_loop_run(false, "bt_service");
#else
    ret = service_loop_run(true, "bt_service");
    if (ret != 0)
        goto out;
    bluetooth_ipc_join_thread_pool();
#endif

out:
    bt_service_cleanup();
    service_loop_exit();
    return ret;
}
