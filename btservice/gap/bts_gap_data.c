/****************************************************************************
 * frameworks/bluetooth/src/btservice/profile/bts_gap_data.c
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

#define LOG_TAG "bts_gap_data"
#include "log.h"

#ifndef MAX_BONDED_DEVICES_SUPPORTED
#define MAX_BONDED_DEVICES_SUPPORTED 5
#endif

typedef struct {
    /* Basic information to save */
    uint32_t size;
    uint8_t spk_volume;
    uint8_t mic_volume;
    uint16_t bonded_number;
    SERVICE_REMOTE_DEVICE_S bonded_devices[MAX_BONDED_DEVICES_SUPPORTED];
    uint32_t check_sum;
    /* More items to be added from here for future extension */
} bt_storage_t;

static char s_db_path[256];

static void init_storage(void)
{
    strcat(s_db_path, "/tmp/storage.db");
}


/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
    This function saves recoverable information, e.g. bonded devices, to
    local storage.
------------------------------------------------------------------------*/
void write_storage_file(void *data_in, uint32_t data_size)
{
    FILE *db_file = NULL;
    BT_LOGD("%s", __func__);

    db_file = fopen(s_db_path, "awb+");
    if (db_file != NULL) {
          BT_LOGD("%s", __func__);

        fwrite(data_in, sizeof(uint8_t), data_size, db_file);
        fflush(db_file);
        fclose(db_file);
    }
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
    This function reads recoverable information, e.g. bonded devices, from
    local storage.
------------------------------------------------------------------------*/
uint32_t read_storage_file(void *data_out, uint32_t data_size)
{
    FILE *db_file = NULL;
    uint32_t size_read = 0;

    db_file = fopen(s_db_path, "rb");
    if (db_file != NULL) {
        size_read = fread(data_out, sizeof(uint8_t), data_size, db_file);
        fclose(db_file);
    }
    return size_read;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
    This function recovers data from local storage.
-------------------------------------------------------------------------------*/
void gap_read_data_storage(void)
{
    uint32_t size_read;
    bt_storage_t *pdata = malloc(sizeof(bt_storage_t));

    memset(pdata, 0, sizeof(bt_storage_t));
    size_read = read_storage_file(pdata, sizeof(bt_storage_t));

    if ((size_read >= sizeof(bt_storage_t))
        && (pdata->size >= sizeof(bt_storage_t))
        && (pdata->bonded_number < MAX_BONDED_DEVICES_SUPPORTED)
        && ((pdata->size + pdata->bonded_number) == pdata->check_sum)) {
        /* Data format validated */
        uint32_t i;
        for (i = 0; i < pdata->bonded_number; i++) {
            service_adapter_gap_set_bonded_device(pdata->bonded_devices + i);
        }
        //APPI_SPK_VOLUME = pdata->spk_volume;
        //APPI_MIC_VOLUME = pdata->mic_volume;
    } else {
        /* Set default value */
        //APPI_SPK_VOLUME = 0x3F;/* 0 - 0x7E */
        //APPI_MIC_VOLUME = 0x07;/* 0 - 15 */
    }
    free(pdata);
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
Description:
    This function saves latest data to local storage.
-------------------------------------------------------------------------------*/
void gap_update_data_storage(void)
{
      bt_storage_t *pdata = malloc(sizeof(bt_storage_t));

      memset(pdata, 0, sizeof(bt_storage_t));
      pdata->size = sizeof(bt_storage_t);
      //pdata->spk_volume = APPI_SPK_VOLUME;
      //pdata->mic_volume = APPI_MIC_VOLUME;
      pdata->bonded_number = (uint16_t)service_adapter_gap_get_bonded_devices(pdata->bonded_devices,
                              MAX_BONDED_DEVICES_SUPPORTED);
      pdata->check_sum = pdata->size + pdata->bonded_number;
      BT_LOGD("%s, %s, %s", __func__, pdata->bonded_devices[0].bd_addr, pdata->bonded_devices[0].link_key);

      write_storage_file(pdata, sizeof(bt_storage_t));
      free(pdata);
}

