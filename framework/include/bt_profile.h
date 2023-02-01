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
#ifndef _BT_PROFILE_H__
#define _BT_PROFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PROFILE_A2DP_NAME      "A2DP-Src"
#define PROFILE_A2DP_SINK_NAME "A2DP-Sink"
#define PROFILE_HFP_HF_NAME    "HFP-HF"
#define PROFILE_HFP_AG_NAME    "HFP-AG"
#define PROFILE_SPP_NAME       "SPP"

enum profile_id {
    PROFILE_A2DP,
    PROFILE_A2DP_SINK,
    PROFILE_HFP_HF,
    PROFILE_HFP_AG,
    PROFILE_SPP,
    PROFILE_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* _BT_PROFILE_H__ */