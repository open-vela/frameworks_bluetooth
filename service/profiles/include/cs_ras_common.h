/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
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

#ifndef _CS_RAS_COMMON_H_
#define _CS_RAS_COMMON_H_

#ifndef BIT
#define BIT(n) (1UL << n)
#endif /* BIT */

#define RAS_FEATURE_REALTIME_RANG_DATA BIT(0)
#define RAS_FEATURE_RETRIEVE_LOST_RANG_DATA_SEG BIT(1)
#define RAS_FEATURE_ABORT_OPERATION BIT(2)
#define RAS_FEATURE_FILTER_RANG_DATA BIT(3)

#endif /* _CS_RAS_COMMON_H_ */
