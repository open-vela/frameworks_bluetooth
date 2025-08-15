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

  # Variables
  set(COMPILE_OPTIONS)
  set(SOURCES)
  set(INCLUDE_DIRECTORIES)

  # Sources
  file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/common/*.c)
  list(APPEND SOURCES ${APPEND_FILES})

  if(CONFIG_BLUETOOTH_FRAMEWORK)
    if(CONFIG_BLUETOOTH_FRAMEWORK_LOCAL)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/api/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
        file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/framework/api/bt_lea*)
        list(REMOVE_ITEM SOURCES ${EXLUDE_FILES})
      endif()

    elseif(CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/api/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
        file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/framework/api/bt_lea*)
        list(REMOVE_ITEM SOURCES ${EXLUDE_FILES})
      endif()

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/ipc/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/ipc/socket/src/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/socket/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      if (CONFIG_BLUETOOTH_FRAMEWORK_ASYNC)
        file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/framework/socket/async/*.c)
        list(APPEND SOURCES ${APPEND_FILES})
      endif()

      list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/ipc/socket/include)

    else()
      # Other IPC source files

    endif()
  endif()

  list(
    APPEND
    SOURCES
    ${BLUETOOTH_DIR}/service/src/connection_manager.c
    ${BLUETOOTH_DIR}/service/src/manager_service.c
    ${BLUETOOTH_DIR}/service/common/index_allocator.c)

  if(CONFIG_BLUETOOTH_STORAGE_PROPERTY_SUPPORT)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/service/common/storage_property.c)
  else()
    list(APPEND SOURCES ${BLUETOOTH_DIR}/service/common/storage.c)
  endif()

  if(CONFIG_BLUETOOTH_DEBUG_TRACE)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/debug/bt_trace.c)
  endif()

  if(CONFIG_BLUETOOTH_SERVICE)
    list(
      APPEND
      SOURCES
      ${BLUETOOTH_DIR}/service/common/service_loop.c
      ${BLUETOOTH_DIR}/service/src/adapter_service.c
      ${BLUETOOTH_DIR}/service/src/adapter_state.c
      ${BLUETOOTH_DIR}/service/src/btservice.c
      ${BLUETOOTH_DIR}/service/src/device.c
      ${BLUETOOTH_DIR}/service/src/power_manager.c
      ${BLUETOOTH_DIR}/service/vendor/bt_vendor.c
      ${BLUETOOTH_DIR}/service/src/hci_parser.c)

    if(CONFIG_BLUETOOTH_BLE_ADV)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/src/advertising.c)
    endif()

    if(CONFIG_BLUETOOTH_BLE_SCAN)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/src/scan_manager.c
           ${BLUETOOTH_DIR}/service/src/scan_record.c
           ${BLUETOOTH_DIR}/service/src/scan_filter.c)
    endif()

    if(CONFIG_BLUETOOTH_L2CAP)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/src/l2cap_service.c)
    endif()

    if(CONFIG_LE_DLF_SUPPORT)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/src/connectino_manager_dlf.c)
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/stacks/*.c)
    list(APPEND SOURCES ${APPEND_FILES})

    if(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET
       OR CONFIG_BLUETOOTH_STACK_LE_BLUELET)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/stacks/bluelet/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE OR CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_debug_interface.c)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_zblue.c)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_adapter_interface.c)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_connection_manager.c)
    endif()

    if(CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_adapter_le_interface.c)

      if(CONFIG_BLUETOOTH_BLE_ADV)
        list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_le_advertise_interface.c)
      endif()

      if(CONFIG_BLUETOOTH_BLE_SCAN)
        list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_le_scan_interface.c)
      endif()

      if(CONFIG_BLUETOOTH_GATT)
        list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_gatt_client_interface.c)
        list(APPEND SOURCES ${BLUETOOTH_DIR}/service/stacks/zephyr/sal_gatt_server_interface.c)
      endif()
    endif()

    if(NOT CONFIG_BLUETOOTH_BLE_AUDIO)
      file(GLOB EXLUDE_FILES ${BLUETOOTH_DIR}/service/stacks/bluelet/sal_lea_*)
      list(REMOVE_ITEM SOURCES ${EXLUDE_FILES})
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/*.c)
    list(APPEND SOURCES ${APPEND_FILES})

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/system/*.c)
    list(APPEND SOURCES ${APPEND_FILES})

    if(NOT CONFIG_BLUETOOTH_A2DP)
      list(REMOVE_ITEM SOURCES
           ${BLUETOOTH_DIR}/service/profiles/system/bt_player.c)
    endif()

    if(NOT
       (CONFIG_BLUETOOTH_A2DP
        OR CONFIG_BLUETOOTH_HFP_AG
        OR CONFIG_BLUETOOTH_HFP_HF
        OR CONFIG_BLUETOOTH_BLE_AUDIO))
      list(REMOVE_ITEM SOURCES
           ${BLUETOOTH_DIR}/service/profiles/system/media_system.c)
    endif()

    file(GLOB APPEND_FILES
         ${BLUETOOTH_DIR}/service/profiles/audio_interface/*.c)
    list(APPEND SOURCES ${APPEND_FILES})

    if(CONFIG_BLUETOOTH_GATT)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/gatt/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_A2DP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/codec/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/profiles/a2dp)

      list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/profiles/a2dp/codec)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SOURCE)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/source/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/a2dp/sink/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_TARGET OR CONFIG_BLUETOOTH_AVRCP_CONTROL)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/avrcp/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/profiles/avrcp)
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_TARGET)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/avrcp/target/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/avrcp/control/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HFP_HF)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_hf/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HFP_AG)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hfp_ag/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_SPP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/spp/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_HID_DEVICE)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/hid/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_PAN)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/pan/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT OR CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/audio_ipc/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/*.c)
      list(APPEND SOURCES ${APPEND_FILES})

      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/codec/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/server/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CCP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/ccp/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCP)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/mcp/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICS)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/vmics/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/client/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCS)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/mcs/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_TBS)
      file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/profiles/leaudio/tbs/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICP)
      file(GLOB APPEND_FILES
           ${BLUETOOTH_DIR}/service/profiles/leaudio/vmicp/*.c)
      list(APPEND SOURCES ${APPEND_FILES})
    endif()

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/utils/*.c)
    list(APPEND SOURCES ${APPEND_FILES})

    file(GLOB APPEND_FILES ${BLUETOOTH_DIR}/service/vhal/*.c)
    list(APPEND SOURCES ${APPEND_FILES})

    list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/vhal)
  endif()

  if(CONFIG_BLUETOOTH_DEBUG_MEMORY)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/debug/bt_memory.c)
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/utils.c)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/log.c)

    if(CONFIG_BLUETOOTH_FRAMEWORK_ASYNC)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/async/gap.c)
      if(CONFIG_BLUETOOTH_BLE_ADV)
        list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/async/adv.c)
      endif()
    endif()

    if(CONFIG_BLUETOOTH_BLE_ADV)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/adv.c)
    endif()

    if(CONFIG_BLUETOOTH_BLE_SCAN)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/scan.c)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/a2dp_sink.c)
    endif()

    if(CONFIG_BLUETOOTH_A2DP_SOURCE)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/a2dp_source.c)
    endif()

    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/avrcp_control.c)
    endif()

    if(CONFIG_BLUETOOTH_GATT)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/gatt_client.c)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/gatt_server.c)
    endif()

    if(CONFIG_BLUETOOTH_HFP_HF)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/hfp_hf.c)
    endif()

    if(CONFIG_BLUETOOTH_HFP_AG)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/hfp_ag.c)
    endif()

    if(CONFIG_BLUETOOTH_SPP)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/spp.c)
    endif()

    if(CONFIG_BLUETOOTH_HID_DEVICE)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/hid_device.c)
    endif()

    if(CONFIG_BLUETOOTH_PAN)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/panu.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_SERVER)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_server.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCP)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_mcp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CCP)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_ccp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICS)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_vmics.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_CLIENT)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_client.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_VMICP)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_vmicp.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_MCS)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_mcs.c)
    endif()

    if(CONFIG_BLUETOOTH_LEAUDIO_TBS)
      list(APPEND SOURCES ${BLUETOOTH_DIR}/tools/lea_tbs.c)
    endif()
  endif()

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/framework/include)

  if(CONFIG_LIB_DBUS_RPMSG_SERVER_CPUNAME OR CONFIG_OFONO)
    list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/dbus/dbus)

    list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/glib/glib/glib)

    list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/glib/glib)

    list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/glib)

    list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/frameworks/system/utils/gdbus)
  endif()

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/src)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/common)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/profiles)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/profiles/include)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/profiles/system)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/stacks)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/stacks/include)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/vendor)

  list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/dfx)

  if(CONFIG_BLUETOOTH_SERVICE)
    if(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET
       OR CONFIG_BLUETOOTH_STACK_LE_BLUELET)
      list(
        APPEND INCLUDE_DIRECTORIES
        ${NUTTX_APPS_DIR}/external/bluelet/bluelet/src/samples/stack_adapter/inc
      )

      list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/stacks/bluelet/include)
      list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/vendor/xiaomi/vela/bluelet/inc)
    endif()

    if(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE OR CONFIG_BLUETOOTH_STACK_LE_ZBLUE)
      list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/zblue/zblue/port/include/)
      list(APPEND INCLUDE_DIRECTORIES
           ${NUTTX_APPS_DIR}/external/zblue/zblue/port/include/kernel/include)
      list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/zblue/zblue/port/include/host)
      list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/stacks/zephyr/include)
    endif()

    list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/service/ipc)
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    list(APPEND INCLUDE_DIRECTORIES ${BLUETOOTH_DIR}/tools)
  endif()

  if(CONFIG_ARCH_SIM)
    list(APPEND COMPILE_OPTIONS -O0)
  endif()

  list(APPEND COMPILE_OPTIONS -Wno-strict-prototypes -Wno-undef -Wno-incompatible-pointer-types -Wno-pointer-sign)

  if(CONFIG_BLUETOOTH_FEATURE)
    if(CONFIG_ARCH STREQUAL "arm")
      list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/libffi/libffi/src/arm)

    elseif(CONFIG_ARCH STREQUAL "arm64")
      list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/libffi/libffi/src/aarch64)

    elseif(CONFIG_ARCH STREQUAL "xtensa")
      list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/libffi/libffi/src/xtensa)

    else()
      list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/libffi/libffi/src/x86)
    endif()

    list(APPEND INCLUDE_DIRECTORIES ${NUTTX_APPS_DIR}/external/libffi)

    set(FEATURE_TOP ${NUTTX_APPS_DIR}/frameworks/runtimes/feature)
    list(APPEND INCLUDE_DIRECTORIES ${FEATURE_TOP}/include)
    list(APPEND INCLUDE_DIRECTORIES
         ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/feature/include)

    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/src/system_bluetooth.c)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/src/system_bluetooth_impl.c)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/src/feature_bluetooth_util.c)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt.c)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_impl.c)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/src/feature_bluetooth_callback.c)

    if(CONFIG_BLUETOOTH_A2DP_SINK)
      list(APPEND SOURCES
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_a2dpsink.c)
      list(APPEND SOURCES
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_a2dpsink_impl.c)
    endif()
    if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
      list(APPEND SOURCES
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_avrcpcontrol.c)
      list(APPEND SOURCES
           ${BLUETOOTH_DIR}/feature/src/system_bluetooth_bt_avrcpcontrol_impl.c)
    endif()
  elseif(CONFIG_BLUETOOTH_FEATURE_ASYNC)
    set(FEATURE_TOP ${NUTTX_APPS_DIR}/frameworks/runtimes/feature)
    list(APPEND INCLUDE_DIRECTORIES ${FEATURE_TOP}/include)
    list(APPEND INCLUDE_DIRECTORIES
         ${NUTTX_APPS_DIR}/frameworks/connectivity/bluetooth/feature/feature_async/include)

    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/feature_async/src/bluetooth_impl.c)
    list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/feature_async/src/feature_bluetooth_util.c)

    if(CONFIG_BLUETOOTH_BLE_ADV)
        list(APPEND SOURCES ${BLUETOOTH_DIR}/feature/feature_async/src/bluetooth_ble_impl.c)
    endif()
  else()
  endif()

  nuttx_add_library(libbluetooth STATIC)

  if(TARGET zblue::headers)
    target_link_libraries(libbluetooth PUBLIC zblue::headers)
  endif()

  target_include_directories(libbluetooth
    PUBLIC
      ${BLUETOOTH_DIR}/include
      ${BLUETOOTH_DIR}/framework/include
    PRIVATE
      ${INCLUDE_DIRECTORIES}
  )

  target_compile_options(libbluetooth PRIVATE
    ${COMPILE_OPTIONS}
    -Wno-strict-prototypes -Wno-undef -Wno-incompatible-pointer-types -Wno-pointer-sign
  )

  target_sources(libbluetooth PRIVATE ${SOURCES})

  nuttx_add_dependencies(TARGET libbluetooth DEPENDS libuv)

  if(CONFIG_BLUETOOTH_SERVER)
    nuttx_add_application(
      NAME        ${CONFIG_BLUETOOTH_SERVER_NAME}
      SRCS        ${BLUETOOTH_DIR}/service/src/main.c
      INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
      STACKSIZE   ${CONFIG_BLUETOOTH_TASK_STACK_SIZE}
      PRIORITY    SCHED_PRIORITY_DEFAULT
      COMPILE_FLAGS ${COMPILE_OPTIONS}
      DEPENDS     libbluetooth
    )

    if(TARGET zblue::headers)
      target_link_libraries(${CONFIG_BLUETOOTH_SERVER_NAME} PRIVATE zblue::headers)
    endif()
  endif()

  # Note: Dependencies are handled through nuttx_add_application DEPENDS parameter
  # No need to manually link to apps targets

  # Bluetooth sample code applications
  if(CONFIG_APP_BT_SAMPLE_CODE)
    if(CONFIG_APP_BT_SAMPLE_CODE_BASIC)
      nuttx_add_application(
        NAME          bt_sample_basic
        SRCS          ${BLUETOOTH_DIR}/samples/basic/main.c
        INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
        STACKSIZE     8192
        PRIORITY      ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS ${COMPILE_OPTIONS}
        DEPENDS       libbluetooth
      )
      if(TARGET zblue::headers)
        target_link_libraries(bt_sample_basic PRIVATE zblue::headers)
      endif()
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_ENABLE)
      nuttx_add_application(
        NAME          bt_sample_enable
        SRCS          ${BLUETOOTH_DIR}/samples/enable/main.c
        INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
        STACKSIZE     8192
        PRIORITY      ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS ${COMPILE_OPTIONS}
        DEPENDS       libbluetooth
      )
      if(TARGET zblue::headers)
        target_link_libraries(bt_sample_enable PRIVATE zblue::headers)
      endif()
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_DISCOVERY)
      nuttx_add_application(
        NAME          bt_sample_discovery
        SRCS          ${BLUETOOTH_DIR}/samples/discovery/main.c
        INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
        STACKSIZE     8192
        PRIORITY      ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS ${COMPILE_OPTIONS}
        DEPENDS       libbluetooth
      )
      if(TARGET zblue::headers)
        target_link_libraries(bt_sample_discovery PRIVATE zblue::headers)
      endif()
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_CREATEBOND)
      nuttx_add_application(
        NAME          bt_sample_createbond
        SRCS          ${BLUETOOTH_DIR}/samples/createbond/main.c
        INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
        STACKSIZE     8192
        PRIORITY      ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS ${COMPILE_OPTIONS}
        DEPENDS       libbluetooth
      )
      if(TARGET zblue::headers)
        target_link_libraries(bt_sample_createbond PRIVATE zblue::headers)
      endif()
    endif()

    if(CONFIG_APP_BT_SAMPLE_CODE_ACCEPTBOND)
      nuttx_add_application(
        NAME          bt_sample_acceptbond
        SRCS          ${BLUETOOTH_DIR}/samples/acceptbond/main.c
        INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
        STACKSIZE     8192
        PRIORITY      ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS ${COMPILE_OPTIONS}
        DEPENDS       libbluetooth
      )
      if(TARGET zblue::headers)
        target_link_libraries(bt_sample_acceptbond PRIVATE zblue::headers)
      endif()
    endif()
  endif()

  if(CONFIG_BLUETOOTH_TOOLS)
    foreach(_tool IN ITEMS bttool adapter_test)
      if(_tool STREQUAL bttool)
        set(_src ${BLUETOOTH_DIR}/tools/bt_tools.c)
      elseif(_tool STREQUAL adapter_test)
        set(_src ${BLUETOOTH_DIR}/tests/adapter_test.c)
      endif()
      nuttx_add_application(
        NAME          ${_tool}
        SRCS          ${_src}
        INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
        STACKSIZE     8192
        PRIORITY      ${SCHED_PRIORITY_DEFAULT}
        COMPILE_FLAGS ${COMPILE_OPTIONS}
        DEPENDS       libbluetooth
      )
      if(TARGET zblue::headers)
        target_link_libraries(${_tool} PRIVATE zblue::headers)
      endif()
    endforeach()
  endif()

  if(CONFIG_BLUETOOTH_UPGRADE)
    nuttx_add_application(
      NAME          bt_upgrade
      SRCS          ${BLUETOOTH_DIR}/tools/storage_transform.c
      INCLUDE_DIRECTORIES ${INCLUDE_DIRECTORIES}
      STACKSIZE     8192
      PRIORITY      ${SCHED_PRIORITY_DEFAULT}
      COMPILE_FLAGS ${COMPILE_OPTIONS}
      DEPENDS       libbluetooth
    )
    if(TARGET zblue::headers)
      target_link_libraries(bt_upgrade PRIVATE zblue::headers)
    endif()
  endif()

  # JIDL processing for feature APIs
  if(CONFIG_BLUETOOTH_FEATURE OR CONFIG_BLUETOOTH_FEATURE_ASYNC)
    include(nuttx_add_jidl)
    set(FEATURE_TOP ${NUTTX_APPS_DIR}/frameworks/runtimes/feature)
    set(PY_SCRIPT ${FEATURE_TOP}/tools/jidl/jsongensource.py)
    set(BINARY_EXT_MODULES_DIR ${CMAKE_BINARY_DIR}/feature/modules/)
    
    if(CONFIG_BLUETOOTH_FEATURE)
      set(JIDL_PATHS
        ${BLUETOOTH_DIR}/feature/jdil/bluetooth.jidl
        ${BLUETOOTH_DIR}/feature/jdil/bluetooth_bt.jidl
      )
      if(CONFIG_BLUETOOTH_A2DP_SINK)
        list(APPEND JIDL_PATHS
          ${BLUETOOTH_DIR}/feature/jdil/bluetooth_a2dpsink.jidl)
      endif()
      if(CONFIG_BLUETOOTH_AVRCP_CONTROL)
        list(APPEND JIDL_PATHS
          ${BLUETOOTH_DIR}/feature/jdil/bluetooth_avrcpcontrol.jidl)
      endif()
    elseif(CONFIG_BLUETOOTH_FEATURE_ASYNC)
      set(JIDL_PATHS ${BLUETOOTH_DIR}/feature/feature_async/jidl/bluetooth.jidl)
      if(CONFIG_BLUETOOTH_BLE_ADV)
        list(APPEND JIDL_PATHS
          ${BLUETOOTH_DIR}/feature/feature_async/jidl/bluetooth_ble.jidl)
      endif()
    endif()
    
    nuttx_add_jidl(
      TARGET         libbluetooth
      JIDL_SCRIPT    ${PY_SCRIPT}
      JIDL_OUT_DIR   ${BINARY_EXT_MODULES_DIR}
      JIDLS          ${JIDL_PATHS}
      OUT_SRC_EXT    c
    )
  endif()

  # Use nuttx_include_directories_for_all_apps to expose headers globally
  nuttx_include_directories_for_all_apps(
    ${BLUETOOTH_DIR}/include
    ${BLUETOOTH_DIR}/framework/include
  )
endif()
