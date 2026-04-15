# Bluetooth Event Trace

轻量级蓝牙协议栈事件追踪系统，基于 NuttX System Trace（`sched_note_event` / noteram）后端。

## 1. 设计概念

### 1.1 三层架构：Core → Probe → Spec

```
                    ┌─────────────────────────────────┐
                    │         Trace Spec               │
                    │  (面向具体分析场景的完整方案)       │
                    │                                   │
                    │  定义 event_id、payload 格式、     │
                    │  过滤器、CLI 命令                  │
                    │                                   │
                    │  当前实现: spp/ (SPP Trace Spec)   │
                    │  未来可扩展: leaudio/, a2dp/ ...   │
                    └──────────┬────────────────────────┘
                               │ 调用
                    ┌──────────▼────────────────────────┐
                    │         Probe 接口层               │
                    │  (协议层打点声明，与 spec 解耦)     │
                    │                                   │
                    │  probe/bt_probe_hci.h              │
                    │  probe/bt_probe_rfcomm.h           │
                    │  probe/bt_probe_spp.h              │
                    │                                   │
                    │  由 Kconfig select 控制编译         │
                    └──────────┬────────────────────────┘
                               │ 调用
                    ┌──────────▼────────────────────────┐
                    │         Trace Core                 │
                    │  (与场景无关的基础设施)             │
                    │                                   │
                    │  bt_trace_write_var()              │
                    │    → sched_note_event()            │
                    │                                   │
                    │  采集控制由系统 trace 命令管理       │
                    └───────────────────────────────────┘
```

**Trace Core** 提供写入接口 `bt_trace_write_var(event_id, payload, len)`，将二进制 blob 写入 `sched_note_event()`。noteram driver 负责时间戳和 buffer 管理。采集的启停由系统 `trace start/stop` 命令控制，Core 不维护独立的开关状态。

**Probe 接口层** 在 `probe/` 目录下声明各协议层的打点函数（如 `bt_probe_hci_stack_tx_acl()`、`bt_probe_rfcomm_tx()`、`bt_probe_spp_tx_start()`）。这些函数由协议栈和服务代码调用，但 **实现由 Spec 提供**。Probe 头文件通过 `CONFIG_BT_PROBE_xxx` 控制：未启用时所有调用退化为空宏，零开销。

**Trace Spec** 是面向具体分析场景的完整方案。一个 Spec：
- 实现它所需的全部 probe 函数（可以只实现部分，不需要的留空）
- 定义 event_id 编码和 payload 格式
- 提供场景相关的过滤器
- 提供 CLI 子命令

当前可用的 Spec：
- [SPP Trace Spec](spp/README.md) — HCI → RFCOMM → SPP 跨层延迟和流控分析

### 1.2 Kconfig 依赖关系

```
CONFIG_BLUETOOTH_EVENT_TRACE          ← 总开关（Core）
  │  depends on SCHED_INSTRUMENTATION_DUMP
  │  depends on DRIVERS_NOTE
  │
  ├── CONFIG_BT_TRACE_SPP             ← SPP Trace Spec（可选）
  │     depends on BLUETOOTH_SPP
  │     select BT_PROBE_HCI           ← 自动拉入所需 probe 层
  │     select BT_PROBE_RFCOMM
  │     select BT_PROBE_SPP
  │
  ├── CONFIG_BT_TRACE_TOOL            ← 独立 bt_trace CLI（可选）
  │
  └── (未来 spec: BT_TRACE_LEAUDIO, BT_TRACE_A2DP, ...)
```

Spec 通过 `select` 声明它需要哪些 probe 层。多个 Spec 可以共享同一组 probe（例如未来的 A2DP Spec 也会 select `BT_PROBE_HCI`）。Probe 层本身不直接由用户配置。

### 1.3 扩展新 Spec

添加一个新的 Trace Spec（例如 LE Audio）需要：

1. 在 `trace/` 下新建目录（如 `trace/leaudio/`）
2. 实现所需的 probe 函数（可复用已有 probe 头文件，或新增 `probe/bt_probe_xxx.h`）
3. 定义 event_id 和 payload 格式
4. 在 Kconfig 中添加 `config BT_TRACE_LEAUDIO`，select 所需 probe
5. 在 CMakeLists.txt 中添加编译条件
6. （可选）添加 CLI 子命令和 format JSON

## 2. 配置

### 2.1 系统 trace 前置依赖

bt_trace 基于 NuttX note 框架，需要先开启系统侧配置：

```kconfig
# 必选：note 驱动和 DUMP 事件支持
CONFIG_SCHED_INSTRUMENTATION_DUMP=y   # bt_trace 使用 sched_note_event() 写入
CONFIG_DRIVERS_NOTERAM=y              # 内存环形缓冲区后端
CONFIG_DRIVERS_NOTERAM_BUFSIZE=8192   # buffer 大小（默认 2K 太小，建议 ≥8K）

# 推荐：配合系统 trace 命令使用
CONFIG_SYSTEM_TRACE=y                 # 启用 trace start/stop/dump 命令
CONFIG_DRIVERS_NOTECTL=y              # 启用 trace mode 运行时过滤控制
CONFIG_SCHED_INSTRUMENTATION_FILTER=y # 启用事件过滤

# 推荐：高精度时钟（ns 级）
CONFIG_ARCH_PERF_EVENTS=y             # 使用硬件 PMU 时钟源
CONFIG_PERF_OVERFLOW_CORRECTION=y     # 处理时钟溢出回滚
```

> **注意**：不开 `CONFIG_ARCH_PERF_EVENTS` 时，时间戳精度取决于系统滴答时钟（us 级，与 `CONFIG_USEC_PER_TICK` 一致），可能导致相邻事件时间戳相同。

### 2.2 蓝牙 trace 配置

```kconfig
CONFIG_BLUETOOTH_EVENT_TRACE=y      # bt_trace 总开关（Core）
```

然后根据需要启用具体的 Trace Spec：

```kconfig
CONFIG_BT_TRACE_SPP=y               # SPP Trace Spec（需要 BLUETOOTH_SPP=y）
```

### 2.3 可选配置

```kconfig
CONFIG_BT_TRACE_SYSTRACE_TAG=25       # noteram tag（默认 25）
CONFIG_BT_TRACE_TOOL=y                # 独立 bt_trace CLI（不依赖 bttool）
CONFIG_DRIVERS_NOTERAM_BUFSIZE=61440  # 同时开启 SWITCH 等系统 trace 时建议加大 buffer
```

### 2.4 CLI 入口

- 启用 `CONFIG_BLUETOOTH_TOOLS`（bttool）时：trace 命令集成到 `bttool trace` 子命令
- 启用 `CONFIG_BT_TRACE_TOOL` 时：提供独立的 `bt_trace` 命令（不依赖 bttool 和 framework 库）

两者共享同一套命令实现（`bt_trace_cmd.c`），通过编译时选择不同的 CLI 框架头文件。

## 3. CLI 命令

### 3.1 基础命令

```
bt_trace clock-test [delay_ms]  测试时钟精度（默认 5ms）
```

`clock-test` 用于验证 noteram 时间戳的准确性。它通过 `perf_gettime()` 测量一段已知延迟（默认 5ms），对比实际测量值与期望值的比率。比率接近 1.0 说明时钟准确，偏差过大（如 QEMU 时间膨胀）会导致 trace 事件的时间戳不可靠。

采集控制和状态查看使用系统 `trace` 命令：

```
trace start           开始记录 noteram buffer
trace stop            停止记录
trace dump <file>     导出 noteram buffer 到文件
```

各 Spec 会注册自己的子命令（如 `bt_trace spp ...`），详见各 Spec 的 README。

## 4. 使用流程

```bash
# 1. （可选）设置 Spec 过滤器缩小采集范围
#    具体命令见各 Spec 的 README

# 2. 启动系统 trace（开始记录 noteram buffer）
nsh> trace start

# 3. 执行蓝牙操作

# 4. 停止 trace 并导出
nsh> trace stop
nsh> trace dump /data/bt_trace.log

# 5. 将 dump 文件传到 PC 进行离线分析
```

> **说明**：bt_trace 事件通过 `sched_note_event()` 写入 noteram，采集的启停完全由系统 `trace start/stop` 控制。只需确保系统 trace 处于记录状态，蓝牙事件即会被捕获。

### 4.1 trace dump 输出格式

bt_trace 事件使用 `NOTE_DUMP_BINARY` 类型写入，在 `trace dump` 输出中格式为：

```
bt_service-8  [0]  1.234567000: tracing_mark_write: I|8|00 00 09 00 03 00 40 00 05 ef 00
```

其中 `I|{pid}|{hex bytes}` 的 hex bytes 就是 `[event_id_lo event_id_hi payload...]`。上例中 `00 00` 是 event_id 0x0000（HCI_STACK_TX_ACL），后续 9 字节是 payload。

这些二进制数据需要离线解析，事件格式定义在各 Spec 目录下（如 `spp/spp_trace_format.json`）。

### 4.2 使用 perfetto 查看

系统 trace 数据（任务切换、中断等）可以直接用 [perfetto](https://ui.perfetto.dev/) 查看。bt_trace 的 BINARY 事件在 perfetto 中显示为 Instant 标记（`I|` 前缀），不会形成时间区间，需要配合离线脚本解析。

## 5. 源码结构

```
debug/trace/
├── README.md                     本文档（架构和 Core 说明）
├── bt_event_trace.h              API: bt_trace_write_var()
├── bt_event_trace.c              Core 实现
├── bt_trace_cmd.c                基础 CLI 命令（clock-test）
├── bt_trace_tool.h               独立 CLI 框架
├── bt_trace_tool.c               独立 bt_trace 入口
└── spp/                          SPP Trace Spec → 详见 spp/README.md
    ├── bt_trace_spp.h
    ├── bt_trace_spp.c
    ├── bt_trace_spp_cmd.c
    └── spp_trace_format.json

debug/probe/                      Probe 接口声明（与 spec 解耦）
├── bt_probe_hci.h                HCI 层
├── bt_probe_rfcomm.h             RFCOMM 层
└── bt_probe_spp.h                SPP 层
```

Probe 调用位置：
- HCI H4 层：`nuttx/drivers/serial/uart_bth4.c`
- HCI Stack 层 / RFCOMM 层：`external/zblue/` 协议栈代码
- SPP 层：`service/profiles/spp/spp_service.c`
