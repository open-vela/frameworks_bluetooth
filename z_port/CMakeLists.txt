#
# Copyright 2026 XiaoMi All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#

if(CONFIG_BLUETOOTH_Z_PORT)

  set(Z_PORT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

  set(CSRCS
    ${Z_PORT_DIR}/src/z_api_manager.c
    ${Z_PORT_DIR}/src/z_api_gap.c
    ${Z_PORT_DIR}/src/z_api_gatt.c
    ${Z_PORT_DIR}/src/z_api_ext_adv.c
    ${Z_PORT_DIR}/src/z_api_l2cap.c
    ${Z_PORT_DIR}/src/z_api_a2dp.c
    ${Z_PORT_DIR}/src/z_api_avrcp.c
    ${Z_PORT_DIR}/src/z_api_hfp.c
    ${Z_PORT_DIR}/src/z_api_hfp_ag.c
    ${Z_PORT_DIR}/src/z_api_pan.c
    ${Z_PORT_DIR}/src/z_api_spp.c
    ${Z_PORT_DIR}/src/z_api_hid.c)

  set(INCDIR
    ${Z_PORT_DIR}/include
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/framework/include
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/framework/common
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/src
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/common
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/profiles/include
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/stacks
    ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/service/stacks/include)

  nuttx_add_library(libz_port STATIC)

  target_sources(libz_port PRIVATE ${CSRCS})
  target_include_directories(libz_port PRIVATE ${INCDIR})

  # Link zblue headers for Zephyr BT type definitions
  if(TARGET zblue::headers)
    target_link_libraries(libz_port PRIVATE zblue::headers)
  endif()

endif()
