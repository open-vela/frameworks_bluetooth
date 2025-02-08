/****************************************************************************
 *  Copyright (C) 2024 Xiaomi Corporation
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
#ifndef __SAL_ZBLUE_H_
#define __SAL_ZBLUE_H_

#include "bt_addr.h"
#include "bt_status.h"

#include <zephyr/bluetooth/conn.h>

#define AVDTP_RTP_HEADER_LEN 12
#define STREAM_DATA_RESERVED AVDTP_RTP_HEADER_LEN

bt_status_t bt_sal_get_remote_address(struct bt_conn* conn, bt_address_t* addr);

#endif
