/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_gap_service.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "btdatatype.h"
#include "global.h"
#include "stack_adapter_common.h"
#include "stack_adapter_gap.h"

#include "btm_manager.h"
#include "bts_service.h"
#include "bts_gap.h"
#include "btm_gap.h"

#define LOG_TAG "bts_gap_service"
#include "log.h"



bt_result_code gap_if_init(bts_gap_callback_t* cb)
{
      gap_init(cb);

    return BT_RESULT_SUCCESS;
}

void gap_if_cleanup(void)
{
    gap_cleanup();
}

bt_result_code gap_if_enable(void)
{
    SERVICE_BT_STATUS ret = gap_enable();
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap enable fail,ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

bt_result_code gap_if_disable(bool normal_disable)
{
    SERVICE_BT_STATUS ret = gap_disable(normal_disable);
    if (ret != SERVICE_BT_STATUS_SUCCESS) {
        BT_LOGE("gap disable fail,ret:%d", ret);
        return BT_RESULT_FAILED;
    }
    return BT_RESULT_SUCCESS;
}

stack_state_t gap_if_get_stack_state(void)
{
    gap_get_stack_state();
    return BT_STATE_ON;
}


static gap_interface_t gap_interface = {
    .size = sizeof(gap_interface),
    .init = gap_if_init,
    .cleanup = gap_if_cleanup,
    .enable = gap_if_enable,
    .disable = gap_if_disable,
    .gap_get_stack_state = gap_if_get_stack_state,
};

gap_interface_t* get_gap_instance(void)
{
    return &gap_interface;
}