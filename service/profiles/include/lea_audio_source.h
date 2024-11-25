/****************************************************************************
 * frameworks/bluetooth/btservice/leaudio/audio_sink.c
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
#ifndef __LEA_AUDIO_SOURCE_H__
#define __LEA_AUDIO_SOURCE_H__

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "bt_status.h"
#include "lea_audio_common.h"
#include "lea_codec.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

typedef struct {
    lea_audio_suspend_callback lea_audio_suspend_cb;
    lea_audio_resume_callback lea_audio_resume_cb;
    lea_audio_meatadata_updated_callback lea_audio_meatadata_updated_cb;
    lea_audio_send_callback lea_audio_send_cb;
} lea_source_callabcks_t;

/****************************************************************************
 * Public Fucntion
 ****************************************************************************/

void lea_audio_source_set_callback(lea_source_callabcks_t* callback);

bt_status_t lea_audio_source_init(bool offloading);

bt_status_t lea_audio_source_start(void);

bt_status_t lea_audio_source_stop(bool update_codec);

bt_status_t lea_audio_source_suspend(void);

bt_status_t lea_audio_source_resume(void);

bt_status_t lea_audio_source_update_codec(lea_audio_config_t* codec, uint16_t sdu_size);

bool lea_audio_source_is_started(void);

bool lea_audio_source_ctrl_is_connected(void);

void lea_audio_source_cleanup(void);

#endif