# L2CAP Zephyr SAL Porting 记录

## 概述

将 L2CAP LE CoC 从 bluelet SAL 适配到 zephyr SAL，使 framework 层通过 zblue 原生 `bt_l2cap_*` API 完成 L2CAP 通道操作。

## 架构

```
App → bt_l2cap API → l2cap_service.c → sal_l2cap_interface.h (公共接口)
                                              │
                              ┌────────────────┼────────────────┐
                              │ bluelet/       │ zephyr/        │
                              │ (已有)          │ (本次新增)      │
                              │ service_adapter │ bt_l2cap_*     │
                              └────────────────┴────────────────┘
```

## 变更文件清单

| 操作 | 文件 | 说明 |
|------|------|------|
| 新增 | `frameworks/.../stacks/zephyr/sal_l2cap_interface.c` | zephyr SAL L2CAP 实现 |
| 拷贝 | `frameworks/.../stacks/include/sal_l2cap_interface.h` | 从 bluelet/include 拷贝到公共 include |
| 修改 | `frameworks/.../CMakeLists.txt` | 添加 sal_l2cap_interface.c 编译 |
| 修改 | `frameworks/.../Makefile` | 添加 sal_l2cap_interface.c 编译（Makefile 构建路径） |
| 新增 | `vendor/.../bluetooth/app/app_l2cap.h` | app 层 L2CAP stub 声明 |
| 新增 | `vendor/.../bluetooth/app/app_l2cap.c` | app 层 L2CAP stub 实现 |
| 修改 | `vendor/.../miwear_api_port.c` | 添加 `#include "app_l2cap.h"` |

### 编译修复记录

1. **CMakeLists.txt 遗漏** — 项目实际用 cmake 构建，仅改 Makefile 不够，需同步修改 CMakeLists.txt
2. **miwear_api_port.c 隐式声明** — `app_l2cap_ready`/`app_l2cap_send` 从未实现，之前因 L2CAP 未开启而隐藏。新增 `app_l2cap.c/h` stub 解决
3. **cmake glob 缓存** — 新增 .c 文件后需 touch CMakeLists.txt 触发 cmake 重新扫描
4. **log 头文件差异** — miwear app 层用 `bt_log.h`，不能用 framework 的 `utils/log.h`

## SAL 接口映射

| SAL 接口 | zblue 原生调用 | 说明 |
|----------|---------------|------|
| `bt_sal_l2cap_listen_channel` | `bt_l2cap_server_register()` | 注册 server + accept 回调 |
| `bt_sal_l2cap_stop_listen_channel` | 标记 server 不可用 | zblue 无 unregister API |
| `bt_sal_l2cap_connect_channel` | `get_le_conn_from_addr()` → `bt_l2cap_chan_connect()` | 需先获取 bt_conn |
| `bt_sal_l2cap_disconnect_channel` | `bt_l2cap_chan_disconnect()` | 通过 cid 查找 channel |
| `bt_sal_l2cap_send_packet` | `net_buf_alloc()` → `bt_l2cap_chan_send()` | 需要 SDU headroom |
| `bt_sal_l2cap_give_incoming_credits` | `bt_l2cap_chan_give_credits()` | 仅 SEG_RECV 模式有效 |

## 回调映射

| zblue 回调 | service 回调 | 触发时机 |
|-----------|-------------|---------|
| `ops.connected` | `l2cap_on_channel_connected()` | 通道建立完成 |
| `ops.disconnected` | `l2cap_on_channel_disconnected()` | 通道断开 |
| `ops.recv` | `l2cap_on_packet_received()` | 收到 SDU 数据 |
| `ops.sent` | `l2cap_on_packet_sent()` | 数据发送完成 |
| `server.accept` | `l2cap_on_cid_allocated()` | 收到连接请求，分配 CID |

## 关键数据结构

```c
sal_l2cap_server_t   // 封装 struct bt_l2cap_server + config
sal_l2cap_channel_t  // 封装 struct bt_l2cap_le_chan + addr/psm/状态
sal_l2cap_manager_t  // servers[8] + channels[20] + mutex
```

## Porting 要点

### 1. bt_conn 获取
zblue 的 `bt_l2cap_chan_connect()` 需要 `struct bt_conn*`，通过 `get_le_conn_from_addr()` 从地址获取（定义在 `sal_zephyr_interface.h`）。

### 2. net_buf 内存管理
- 定义专用 `NET_BUF_POOL_FIXED_DEFINE(l2cap_tx_pool, ...)` 用于发送
- 发送前需 `net_buf_reserve(buf, BT_L2CAP_SDU_CHAN_SEND_RESERVE)` 预留 headroom
- `alloc_buf` 回调用于 SDU 分段接收的 buffer 分配

### 3. CID 映射
- zblue 内部管理 CID，通过 `le_chan.rx.cid` 获取 local CID
- SAL 层维护 `cid → sal_l2cap_channel_t` 的查找表

### 4. Credits 流控
- 默认模式：zblue 自动管理 credits（recv 回调返回 0 即释放 credit）
- SEG_RECV 模式：需手动调用 `bt_l2cap_chan_give_credits()`
- `bt_sal_l2cap_give_incoming_credits()` 在非 SEG_RECV 模式下直接返回 SUCCESS

### 5. Server 生命周期
- zblue 没有 `bt_l2cap_server_unregister()` API
- `stop_listen` 暂时只标记 server 为不可用，阻止新的 accept
- 后续需要关注 zblue 是否新增 unregister 接口

### 6. accept 回调中的 channel 分配
- accept 时分配 `sal_l2cap_channel_t`，设置 `le_chan.chan.ops` 和 `rx.mtu/mps`
- 调用 `l2cap_on_cid_allocated()` 通知 service 层
- connected 回调中再通知 `l2cap_on_channel_connected()` 携带完整参数

## 编译验证

- 配置：`lunch 19` ([bes]-[o62lte]-[cp])
- 前提：`CONFIG_BLUETOOTH_L2CAP=y` + `CONFIG_BLUETOOTH_STACK_LE_ZBLUE=y`
- 结果：`sal_l2cap_interface.c` 编译通过，0 error，0 warning
- 注意：整体编译有 `miwear_api_port.c` 的已有错误（与本次改动无关）

## 后续计划

1. **BR L2CAP CoC**：在 `sal_l2cap_interface.c` 中增加 `BT_TRANSPORT_BREDR` 分支，使用 `bt_l2cap_br_chan` + `bt_l2cap_br_server_register()`
2. **Service 层扩展**：`l2cap_service.c` 去掉 BLE-only 限制，增加 BR PSM 管理
3. **BR L2CAP API**：新增 `bt_br_l2cap.h` / `bt_br_l2cap.c` 全栈接口
