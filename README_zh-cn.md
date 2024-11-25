# 蓝牙概述

\[ [English](README.md) | 简体中文 \]

## 一 openvela 蓝牙能力介绍

openvela 蓝牙已经通过 Bluetooth 5.4 认证。目前支持的蓝牙能力包括：

- Core

  - BR/EDR/BLE
  - GAP
  - L2CAP
  - GATT Client/Server
- A2DP SRC/SNK
- AVRCP CT/TG
- HFP AG/HF
- PAN
- SPP
- HID
- HOGP
- LEA

  - TMAP
  - CAP
  - BAP/ASCS/PACS/BASS
  - CSIP/CSIS
  - MCP/MCS
  - CCP/TBS
  - VCP/VCS
- Mesh

openvela 蓝牙目前还能够支持多种开源、闭源协议栈，如Zephyr、Bluez、Bluedroid、Barrot等。

## 二 openvela 蓝牙应用开发

对于第三方应用开发者，可以使用 openvela  快应用 QuickApp Feature ，它是基于 QuickJS 引擎使用 C++ 实现的一系列 API 接口，为三方应用提供系统访问能力。

蓝牙接口请参考：[https://doc.quickapp.cn/features/system/bluetooth.html](https://doc.quickapp.cn/features/system/bluetooth.html)

目前支持三方应用调用的蓝牙能力包括：

- 打开、关闭蓝牙
- 查询、监听蓝牙状态
- 开始、停止蓝牙扫描
- 查询、监听蓝牙扫描结果
- 连接、断开蓝牙
- 查询已经连接的蓝牙设备
- 查询 BLE 蓝牙设备的 Services
- 读写 BLE 蓝牙设备的 Characteristics

将来会提供更多蓝牙能力来供三方应用来调用。当蓝牙协议栈、蓝牙服务开源后，还会提供更为强大的 NDK 接口来直接调用所有蓝牙系统的能力。

## 三 openvela 蓝牙驱动开发

openvela 蓝牙支持多种驱动架构。以目前常用的 BTH4 驱动架构为例，芯片厂商可以实现一个 **struct bt_driver_s** 结构体类型的变量，并为其初始化以下成员函数：

- CODE int (*open)(FAR struct bt_driver_s *btdev);
- CODE int (*send)(FAR struct bt_driver_s *btdev, enum bt_buf_type_e type, FAR void *data, size_t len);
- CODE int (*ioctl)(FAR struct bt_driver_s *btdev, int cmd, unsigned long arg);
- CODE void (*close)(FAR struct bt_driver_s *btdev);

上面成员函数的实现依赖于 HCI 的实际工作方式，也就是 Host 和 Controller 之间的物理总线。

然后，将上述结构体类型的变量通过 API **bt_driver_register**()注册该驱动实例。

- int **bt_driver_register**(FAR struct bt_driver_s *drv);

类型定义可参考头文件 nuttx/include/nuttx/wireless/bluetooth/bt_driver.h。调用关系如下图所示：

![](img/bt_driver.png)

备注：对于 receive() 成员函数，芯片厂商无需定义，BTH4 驱动会为其初始化。

- CODE int (*receive)(FAR struct bt_driver_s *btdev, enum bt_buf_type_e type, FAR void *data, size_t len);

当收到来自芯片的 HCI 数据时，只要调用 **bt_netdev_receive**()即可，它会调用这个 receive()函数来保存收到 HCI 数据。

