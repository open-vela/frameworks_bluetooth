#
# Copyright (C) 2024 Xiaomi Corporation
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

if(CONFIG_BLUETOOTH)

  # Source Directories
  set(BLUETOOTH_DIR ${CMAKE_CURRENT_SOURCE_DIR})

  # Flags
  set(CFLAGS)
  set(CSRCS)
  set(INCDIR)

  # Sources
  file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/common/*.c)
  list(APPEND CSRCS ${APPEND_FILES})

  if(CONFIG_BLUETOOTH_FRAMEWORK)
    if(CONFIG_BLUETOOTH_FRAMEWORK_LOCAL)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/api/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
        file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/framework/api/bt_lea*)
        list(REMOVE_ITEM CSRCS ${EXLUDE_FILES})
      endif()

    elseif(CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/api/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
        file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/framework/api/bt_lea*)
        list(REMOVE_ITEM CSRCS ${EXLUDE_FILES})
      endif()

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/ipc/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/ipc/socket/src/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/socket/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      if (CONFIG_BLUETOOTH_FRAMEWORK_ASYNC)
        file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/socket/async/*.c)
        list(APPEND CSRCS ${APPEND_FILES})
      endif()

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/ipc/socket/include)

    else()
      # Other IPC source files

    endif()
  endif()

  list(
    APPEND
    CSRCS
    ${BLUETOOTH_DIR}/service/src/connection_manager.c
    ${BLUETOOTH_DIR}/service/src/manager_service.c
    ${BLUETOOTH_DIR}/service/common/index_allocator.c)

  if(CONFIG_BLUETOOTH_STORAGE_PROPERTY_SUPPORT)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/service/common/storage_property.c)
  else()
    list(APPEND CSRCS ${BLUETOOTH_DIR}/service/common/bt_storage.c)
  endif()

  if(CONFIG_BLUETOOTH_STORAGE_UPDATE)
    list(
      APPEND
      CSRCS
      ${BLUETOOTH_DIR}/tools/storage_update/storage_version_4.c
      ${BLUETOOTH_DIR}/tools/storage_update/storage_version_5.c)
  endif()

  if(CONFIG_BLUETOOTH_DEBUG_TRACE)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/debug/bt_trace.c)
  endif()

  list(APPEND CFLAGS -Wno-strict-prototypes)

  if(CONFIG_BLUETOOTH_SERVICE)
    list(
      APPEND
      CSRCS
      ${BLUETOOTH_DIR}/service/common/service_loop.c
      ${BLUETOOTH_DIR}/service/src/adapter_service.c
      ${BLUETOOTH_DIR}/service/src/adapter_state.c
      ${BLUETOOTH_DIR}/service/src/btservice.c
      ${BLUETOOTH_DIR}/service/src/device.c
      ${BLUETOOTH_DIR}/service/src/power_manager.c
      ${BLUETOOTH_DIR}/service/vendor/bt_vendor.c
      ${BLUETOOTH_DIR}/service/src/hci_parser.c)

    if(CONFIG_BLUETOOTH_BLE_ADV)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/advertising.c)
    endif()

    if(CONFIG_BLUETOOTH_BLE_SCAN)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/scan_manager.c
           ${BLUETOOTH_DIR}/service/src/scan_record.c
           ${BLUETOOTH_DIR}/service/src/scan_filter.c)
    endif()

    if(CONFIG_BLUETOOTH_L2CAP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/l2cap_service.c)
    endif()

    if(CONFIG_LE_DLF_SUPPORT)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/src/connection_manager_dlf.c)
    endif()

    if(CONFIG_BLUETOOTH_HCI_FILTER)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/vhal/bt_hci_filter.c)
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/stacks/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    if(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET
       OR CONFIG_BLUETOOTH_STACK_LE_BLUELET)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/stacks/bluelet/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE OR CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_debug_interface.c)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_zblue.c)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_adapter_interface.c)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_connection_manager.c)
    endif()

    if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE)
      if(CONFIG_BLUETOOTH_A2DP)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_a2dp_interface.c)
      endif()

      if(CONFIG_BLUETOOTH_AVRCP_CONTROL OR CONFIG_BLUETOOTH_AVRCP_TARGET)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_avrcp_interface.c)
      endif()
    endif()

    if(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_adapter_le_interface.c)

      if(CONFIG_BLUETOOTH_BLE_ADV)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_le_advertise_interface.c)
      endif()

      if(CONFIG_BLUETOOTH_BLE_SCAN)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_le_scan_interface.c)
      endif()

      if(CONFIG_BLUETOOTH_GATT)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_gatt_client_interface.c)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_gatt_server_interface.c)
      endif()
    endif()

    if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
      file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/service/stacks/bluelet/sal_lea_*)
      list(REMOVE_ITEM CSRCS ${EXLUDE_FILES})
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/system/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    if(NOT CONFIG_BLUETOOTH_A2DP)
      list(REMOVE_ITEM CSRCS
           ${BLUETOOTH_DIR}/service/profiles/system/bt_player.c)
    endif()

    if(NOT
       (CONFIG_BLUETOOTH_A2DP
        OR CONFIG_BLUETOOTH_HFP_AG
        OR CONFIG_BLUETOOTH_HFP_HF
        OR CONFIG_BLUETOOTH_BLE_AUDIO))
      list(REMOVE_ITEM CSRCS
           ${BLUETOOTH_DIR}/service/profiles/system/media_system.c)
    endif()

    list(APPEND CSRCS ${BLUETOOTH_DIR}/service/profiles/audio_interface/audio_control.c)
    if(CONFIG_AUDIOUTILS_TINYCOMPRESS)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/service/profiles/audio_interface/audio_transport.c)
        list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/audio_interface/include)
    endif()
    if(CONFIG_BLUETOOTH_GATT)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/gatt/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_A2DP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/codec/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/a2dp)

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/a2dp/codec)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SOURCE)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/source/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/sink/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_TARGET OR CONFIG_BLUETOOTH_AVRCP_CONTROL)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/avrcp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/avrcp)
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_TARGET)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/avrcp/target/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/avrcp/control/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HFP_HF)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_hf/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HFP_AG)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_ag/*.c)
      if(CONFIG_PHONE_SERVICE)
        list(REMOVE_ITEM APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/system/telephony_interface.c)
        list(REMOVE_ITEM APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_ag/hfp_ag_tele_service.c)
      else()
        list(REMOVE_ITEM APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_ag/hfp_ag_tele_phone_service.c)
      endif()
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_SPP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/spp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HID_DEVICE)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hid/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_PAN)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/pan/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT OR CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/audio_ipc/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/*.c)
      list(APPEND CSRCS ${APPEND_FILES})

      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/codec/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/server/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CCP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/ccp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/mcp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICS)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/vmics/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/client/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCS)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/mcs/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_TBS)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/tbs/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICP)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/vmicp/*.c)
      list(APPEND CSRCS ${APPEND_FILES})
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/utils/*.c)
    list(APPEND CSRCS ${APPEND_FILES})

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/vhal/bt_vhal.c)
    list(APPEND CSRCS ${APPEND_FILES})

    list(APPEND INCDIR ${BLUETOOTH_DIR}/service/vhal)
  endif()

  if(CONFIG_BLUETOOTH_DEBUG_MEMORY)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/debug/bt_memory.c)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE)
  if(CONFIG_APP_BT_SAMPLE_CODE_BASIC)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/sample_code/basic/*.c)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_ENABLE)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/sample_code/enable/*.c)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_DISCOVERY)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/sample_code/discovery/*.c)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_CREATEBOND)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/sample_code/createbond/*.c)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_ACCEPTBOND)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/sample_code/acceptbond/*.c)
  endif()
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/utils.c)
    list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/log.c)

    if(CONFIG_BLUETOOTH_FRAMEWORK_ASYNC)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/async/gap.c)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/async/log.c)
      if(CONFIG_BLUETOOTH_BLE_ADV)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/async/adv.c)
      endif()
      if(CONFIG_BLUETOOTH_BLE_SCAN)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/async/scan.c)
      endif()
      if(CONFIG_BLUETOOTH_GATT)
        list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/async/gatt_client.c)
      endif()
    endif()

    if(CONFIG_BLUETOOTH_BLE_ADV)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/adv.c)
    endif()

    if(CONFIG_BLUETOOTH_BLE_SCAN)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/scan.c)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/a2dp_sink.c)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SOURCE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/a2dp_source.c)
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/avrcp_control.c)
    endif()

    if(CONFIG_BLUETOOTH_GATT)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/gatt_client.c)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/gatt_server.c)
    endif()

    if(CONFIG_BLUETOOTH_HFP_HF)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/hfp_hf.c)
    endif()

    if(CONFIG_BLUETOOTH_HFP_AG)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/hfp_ag.c)
    endif()

    if(CONFIG_BLUETOOTH_SPP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/spp.c)
    endif()

    if(CONFIG_BLUETOOTH_HID_DEVICE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/hid_device.c)
    endif()

    if(CONFIG_BLUETOOTH_PAN)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/panu.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_server.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_mcp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CCP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_ccp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICS)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_vmics.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_client.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICP)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_vmicp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCS)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_mcs.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_TBS)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/lea_tbs.c)
    endif()

    if(CONFIG_BLUETOOTH_STORAGE_UPDATE)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/tools/storage_update/storage_tool.c)
    endif()
  endif()

  list(APPEND INCDIR ${BLUETOOTH_DIR}/framework/include)

  if(CONFIG_LIB_DBUS_RPMSG_SERVER_CPUNAME OR CONFIG_OFONO)
    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/dbus/dbus)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/glib/glib/glib)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/glib/glib)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/glib)

    list(APPEND INCDIR ${NUTTX_APPS_DIR}/frameworks/system/utils/gdbus)
  endif()

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/src)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/common)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/include)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/profiles/system)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/stacks)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/stacks/include)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/service/vendor)

  list(APPEND INCDIR ${BLUETOOTH_DIR}/dfx)

  if(CONFIG_BLUETOOTH_SERVICE)
    if(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET
       OR CONFIG_BLUETOOTH_STACK_LE_BLUELET)
      list(
        APPEND INCDIR
        ${NUTTX_APPS_DIR}/external/bluelet/bluelet/src/samples/stack_adapter/inc
      )

      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/stacks/bluelet/include)
      list(APPEND INCDIR ${NUTTX_APPS_DIR}/vendor/xiaomi/vela/bluelet/inc)
    endif()

    if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE OR CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND INCDIR ${BLUETOOTH_DIR}/service/stacks/zephyr/include)
      list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/zblue/zblue/port/include/)
      list(APPEND INCDIR
           ${NUTTX_APPS_DIR}/external/zblue/zblue/port/include/kernel/include)
      list(APPEND INCDIR ${NUTTX_APPS_DIR}/external/zblue/zblue/port/subsys/bluetooth/host)
    endif()

    list(APPEND INCDIR ${BLUETOOTH_DIR}/service/ipc)
  endif()

  if (CONFIG_APP_BT_SAMPLE_CODE)
  if(CONFIG_APP_BT_SAMPLE_CODE_BASIC)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/sample_code/basic)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_ENABLE)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/sample_code/enable)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_DISCOVERY)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/sample_code/discovery)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_CREATEBOND)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/sample_code/createbond)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE_ACCEPTBOND)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/sample_code/acceptbond)
  endif()
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/tools)
  endif()

  if(CONFIG_BLUETOOTH_STORAGE_UPDATE)
    list(APPEND INCDIR ${BLUETOOTH_DIR}/tools/storage_update)
  endif()


  if(CONFIG_ARCH_SIM)
    list(APPEND CFLAGS -O0)
  endif()

  list(APPEND CFLAGS -Wno-strict-prototypes)

  nuttx_add_library(libbluetooth STATIC)

  # Add Applications
  if(CONFIG_BLUETOOTH_SERVER)
    nuttx_add_application(
      NAME
      ${CONFIG_BLUETOOTH_SERVER_NAME}
      SRCS
      ${BLUETOOTH_DIR}/service/src/main.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      ${CONFIG_BLUETOOTH_TASK_STACK_SIZE}
      PRIORITY
      SCHED_PRIORITY_DEFAULT
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libbluetooth)
  endif()

  if(CONFIG_APP_BT_SAMPLE_CODE)
    if(CONFIG_APP_BT_SAMPLE_CODE_BASIC)
      nuttx_add_application(
        NAME
        bt_basic
        SRCS
        ${BLUETOOTH_DIR}/sample_code/basic/basic.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        STACKSIZE
        8192
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS
        ${CFLAGS}
        DEPENDS
        libbluetooth)
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_ENABLE)
      nuttx_add_application(
          NAME
          bt_enable
          SRCS
          ${BLUETOOTH_DIR}/sample_code/enable/enable.c
          INCLUDE_DIRECTORIES
          ${INCDIR}
          STACKSIZE
          8192
          PRIORITY
          ${SCHED_PRIORITY_DEFAULT}
          COMPILE_FLAGS
          ${CFLAGS}
          DEPENDS
          libbluetooth)
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_DISCOVERY)
      nuttx_add_application(
        NAME
        bt_discovery
        SRCS
        ${BLUETOOTH_DIR}/sample_code/discovery/discovery.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        STACKSIZE
        8192
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS
        ${CFLAGS}
        DEPENDS
        libbluetooth)
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_ACCEPTBOND)
      nuttx_add_application(
        NAME
        bt_acceptbond
        SRCS
        ${BLUETOOTH_DIR}/sample_code/acceptbond/acceptbond.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        STACKSIZE
        8192
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS
        ${CFLAGS}
        DEPENDS
        libbluetooth)
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_CREATEBOND)
      nuttx_add_application(
        NAME
        bt_createbond
        SRCS
        ${BLUETOOTH_DIR}/sample_code/createbond/createbond.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        STACKSIZE
        8192
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS
        ${CFLAGS}
        DEPENDS
        libbluetooth)
    endif()
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    nuttx_add_application(
      NAME
      bttool
      SRCS
      ${BLUETOOTH_DIR}/tools/bt_tools.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      8192
      PRIORITY
      ${SCHED_PRIORITY_DEFAULT}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libbluetooth)

    nuttx_add_application(
      NAME
      adapter_test
      SRCS
      ${BLUETOOTH_DIR}/tests/adapter_test.c
      INCLUDE_DIRECTORIES
      ${INCDIR}
      STACKSIZE
      8192
      PRIORITY
      ${SCHED_PRIORITY_DEFAULT}
      COMPILE_FLAGS
      ${CFLAGS}
      DEPENDS
      libbluetooth)
  endif()

  if(CONFIG_BLUETOOTH_STORAGE_UPDATE)
    nuttx_add_application(
        NAME
        bt_storage_update
        SRCS
        ${BLUETOOTH_DIR}/tools/storage_update/storage_update.c
        INCLUDE_DIRECTORIES
        ${INCDIR}
        STACKSIZE
        8192
        PRIORITY
        ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS
        ${CFLAGS}
        DEPENDS
        libbluetooth)
  endif()

  if(CONFIG_BLUETOOTH_FEATURE)
    set(FEATURE_SRCS ${BLUETOOTH_DIR}/feature/src/system_bluetooth_impl.c
                     ${BLUETOOTH_DIR}/feature/src/feature_bluetooth_util.c
                     ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_impl.c
                     ${BLUETOOTH_DIR}/feature/src/feature_bluetooth_callback.c)

    target_include_directories(libbluetooth PRIVATE ${CMAKE_CURRENT_LIST_DIR}/feature/include)

    set(JIDL_PATHS ${BLUETOOTH_DIR}/feature/jidl/bluetooth.jidl
                   ${BLUETOOTH_DIR}/feature/jidl/bluetooth_bt.jidl)
    set(JIDL_OUT_NAMES system_bluetooth system_bluetooth_bt)
    set(FEATURE_NAMES system_bluetooth system_bluetooth_bt)

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND FEATURE_SRCS ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_a2dpsink_impl.c)
      list(APPEND JIDL_PATHS ${BLUETOOTH_DIR}/feature/jidl/bluetooth_bt_a2dpsink.jidl)
      list(APPEND FEATURE_NAMES system_bluetooth_bt_a2dpsink)
      list(APPEND JIDL_OUT_NAMES system_bluetooth_bt_a2dpsink)
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      list(APPEND FEATURE_SRCS ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_avrcpcontrol_impl.c)
      list(APPEND JIDL_PATHS ${BLUETOOTH_DIR}/feature/jidl/bluetooth_bt_avrcpcontrol.jidl)
      list(APPEND FEATURE_NAMES system_bluetooth_bt_avrcpcontrol)
      list(APPEND JIDL_OUT_NAMES system_bluetooth_bt_avrcpcontrol)
    endif()

    nuttx_add_jidl(
      TARGET
      libbluetooth
      FEATURE_SRCS
      ${FEATURE_SRCS}
      JIDLS
      ${JIDL_PATHS}
      FEATURE_NAMES
      ${FEATURE_NAMES}
      JIDL_OUT_NAMES
      ${JIDL_OUT_NAMES}
      OUT_SRC_EXT
      c)
  elseif(CONFIG_BLUETOOTH_FEATURE_ASYNC)
    set(ASYNC_FEATURE_SRCS ${BLUETOOTH_DIR}/feature/feature_async/src/bluetooth_impl.c
                           ${BLUETOOTH_DIR}/feature/feature_async/src/feature_bluetooth_util.c
                           ${BLUETOOTH_DIR}/feature/feature_async/src/bluetooth_ble_impl.c)

    target_include_directories(libbluetooth PRIVATE ${CMAKE_CURRENT_LIST_DIR}/feature/feature_async/include)

    set(ASYNC_FEATURE_NAMES system_bluetooth system_bluetooth_ble)

    set(JIDL_PATHS ${BLUETOOTH_DIR}/feature/feature_async/jidl/bluetooth.jidl)
    list(APPEND JIDL_PATHS
        ${BLUETOOTH_DIR}/feature/feature_async/jidl/bluetooth_ble.jidl)

    if(CONFIG_BLUETOOTH_GATT)
      list(APPEND CSRCS ${BLUETOOTH_DIR}/framework/btwrap/async/bt_gatt_feature.c)
    endif()

    nuttx_add_jidl(
      TARGET
      libbluetooth
      FEATURE_SRCS
      ${ASYNC_FEATURE_SRCS}
      JIDLS
      ${JIDL_PATHS}
      FEATURE_NAMES
      ${ASYNC_FEATURE_NAMES}
      OUT_SRC_EXT
      c)
  else()
  endif()

  if(CONFIG_BLUETOOTH_DFX)
      set(XML_PATHS ${BLUETOOTH_DIR}/dfx/event_bt.xml)
      nuttx_add_dfx_event(
        TARGET
        dfx_bt
        XML_PATHS
        ${XML_PATHS})
  endif()

  # Add Dependson
  nuttx_add_dependencies(TARGET libbluetooth DEPENDS libuv)

  # Export Headers
  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/include
             ${BLUETOOTH_DIR}/framework/include)

  # Library Configuration
  if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE OR CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
    target_link_libraries(libbluetooth PRIVATE zblue_headers)
  endif()
  target_compile_options(libbluetooth PRIVATE ${CFLAGS})
  target_sources(libbluetooth PRIVATE ${CSRCS})
  target_include_directories(libbluetooth PRIVATE ${INCDIR})

endif()
