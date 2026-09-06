/****************************************************************************
 *  Copyright (C) 2026 Xiaomi Corporation
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
#ifndef __HID_DEFS_H__
#define __HID_DEFS_H__

#ifndef BIT
#define BIT(n) (1UL << (n))
#endif /* BIT */

/* USB HID spec 1.11 §6.2.2 - Short item header parsing */
#define HID_ITEM_SIZE(hdr) ((hdr)&0x03)
#define HID_ITEM_TYPE(hdr) (((hdr) >> 2) & 0x03)
#define HID_ITEM_TAG(hdr) (((hdr) >> 4) & 0x0F)
#define HID_ITEM_BSIZE_LONG 3
#define HID_ITEM_BSIZE_LONG_ACTUAL 4

/* Item types */
#define HID_ITEM_TYPE_MAIN 0x0
#define HID_ITEM_TYPE_GLOBAL 0x1

/* Main item tags */
#define HID_ITEM_TAG_INPUT 0x8
#define HID_ITEM_TAG_OUTPUT 0x9
#define HID_ITEM_TAG_FEATURE 0xB

/* Global item tags */
#define HID_ITEM_TAG_REPORT_SIZE 0x7
#define HID_ITEM_TAG_REPORT_ID 0x8
#define HID_ITEM_TAG_REPORT_COUNT 0x9

#endif /* __HID_DEFS_H__ */
