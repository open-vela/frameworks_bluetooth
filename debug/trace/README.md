# Bluetooth Event Trace

轻量级蓝牙协议栈事件追踪系统，基于 NuttX System Trace（`sched_note`）后端。
用于 SPP/HCI/RFCOMM 跨层延迟分析和吞吐量诊断。

## 架构

```
App / Profile
    │
    ▼
Probe 打点层 (bt_probe_hci.h, bt_probe_spp.h, ...)
    │  bt_trace_write_var(event_id, payload, payload_len)
    ▼
sched_note_event(BT_NOTE_TAG, LOG_INFO, NOTE_DUMP_BINARY, buf, len)
    │
    ▼
noteram driver (循环 buffer，覆盖最旧数据)
    │  trace dump
    ▼
ftrace 格式文本文件
    │  bt_trace.py extract / bt_trace_systrace.py extract
    ▼
BT_TRACE 文本 → bt_trace_decode.py → 人类可读输出 / Perfetto JSON
```

写入路径零拷贝：Probe 在栈上构造 `[event_id_lo, event_id_hi, payload...]`，
直接传给 `sched_note_event()`，由 noteram driver 添加时间戳和 note header。

## Kconfig 配置

```
CONFIG_BLUETOOTH_EVENT_TRACE=y      # 总开关（自动 select SCHED_INSTRUMENTATION_DUMP）
CONFIG_BT_TRACE_SYSTRACE_TAG=25     # noteram tag，用于过滤 BT 事件
CONFIG_BT_TRACE_SPP=y               # 启用 SPP trace spec（自动 select HCI/RFCOMM probe）
# CONFIG_BT_TRACE_SPP_LOG=y         # SPP probe 详细日志（调试用）
# CONFIG_BT_TRACE_TOOL=y            # 独立 bt_trace CLI 工具
```

noteram buffer 大小通过系统配置控制：
```
CONFIG_DRIVERS_NOTE_NOTERAM_BUFSIZE=8192   # noteram buffer 大小（字节）
```

## CLI 命令

通过 `bttool trace` 或独立 `bt_trace` 工具访问：

| 命令 | 说明 |
|------|------|
| `start` | 启用 BT event trace |
| `stop` | 停止 BT event trace |
| `status` | 显示 trace 状态（enabled/disabled），提示用 `trace status` 查看 buffer |
| `clock-test` | 测试 noteram 时钟源精度 |
| `spp` | SPP trace 过滤器管理 |

Buffer 管理和数据导出使用系统 `trace` 命令：
```
trace start       # 启动系统 trace（如需要）
trace status      # 查看 noteram buffer 使用情况
trace dump <file> # 导出 trace 数据到文件
trace stop        # 停止系统 trace
```

## 使用工作流

```bash
# 1. 设备端：启动 trace
nsh> bt_trace start

# 2. 执行蓝牙操作（SPP 传输等）

# 3. 设备端：停止 trace 并导出
nsh> bt_trace stop
nsh> trace dump /tmp/trace.dump

# 4. 主机端：提取 BT 事件并解码
python3 bt_trace.py extract trace.dump -o bt_trace.log
python3 bt_trace.py decode bt_trace.log -o decoded.txt
python3 bt_trace.py spp analyze bt_trace.log --timeline
python3 bt_trace.py spp perfetto bt_trace.log -o trace.json
```

## Buffer 溢出行为

noteram 使用循环覆盖策略：buffer 满时覆盖最旧数据，始终保留最新事件。
这与旧的 ring buffer 后端（buffer 满时停止 trace）不同。

建议：根据 trace 场景调整 `CONFIG_DRIVERS_NOTE_NOTERAM_BUFSIZE`。
SPP 吞吐量测试通常需要 8-16KB buffer。

## event_id 编码

```
bit [15]    : 方向    0 = TX,  1 = RX
bit [14:8]  : 协议层  0 = HCI, 2 = RFCOMM, 4 = SPP
bit [7:0]   : 序号    层内事件索引
```

## 事件表

| event_id | 名称 | 方向 | 层 |
|----------|------|------|-----|
| 0x0000 | HCI_STACK_TX_ACL | TX | HCI |
| 0x0003 | HCI_H4_TX_DONE | TX | HCI |
| 0x0004 | HCI_NOCP | TX | HCI |
| 0x8001 | HCI_H4_RX_ACL | RX | HCI |
| 0x8002 | HCI_STACK_RX_ACL | RX | HCI |
| 0x8003 | HCI_STACK_RX_EVT | RX | HCI |
| 0x8006 | HCI_H4_RX_EVT | RX | HCI |
| 0x0200 | RFCOMM_TX | TX | RFCOMM |
| 0x8200 | RFCOMM_RX | RX | RFCOMM |
| 0x0400 | SPP_TX_START | TX | SPP |
| 0x0401 | SPP_TX_SEND | TX | SPP |
| 0x0402 | SPP_TX_DONE | TX | SPP |
| 0x8400 | SPP_RX_START | RX | SPP |
| 0x8401 | SPP_RX_DONE | RX | SPP |

事件定义详见 `spp_trace_format.json`。

## 离线工具

| 工具 | 用途 |
|------|------|
| `bt_trace.py` | 统一 CLI 入口（extract/decode/spp 子命令） |
| `bt_trace_systrace.py` | 从 ftrace dump 提取 BT 事件 |
| `bt_trace_decode.py` | BT_TRACE 文本解码和格式化 |
| `spp_trace_analyze.py` | SPP 跨层延迟和吞吐量分析 |
| `spp_trace_to_perfetto.py` | 导出 Perfetto/Chrome JSON |
