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

#ifdef __BT_MESSAGE_CODE__
BT_LOG_MESSAGE_START,
    BT_LOG_ENABLE,
    BT_LOG_DISABLE,
    BT_LOG_SET_FILTER,
    BT_LOG_REMOVE_FILTER,
    BT_LOG_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_LOG_CALLBACK_START,
    BT_LOG_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_LOG_H__
#define _BT_MESSAGE_LOG_H__

    typedef union {
    struct {
        uint32_t filter_flag;
    } _bt_log_set_flag,
        _bt_log_remove_flag;
} bt_message_log_t;

#endif /* _BT_MESSAGE_LOG_H__ */