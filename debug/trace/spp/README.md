# SPP Trace Spec

覆盖 HCI → RFCOMM → SPP 三层的事件追踪，用于 SPP 数据传输的跨层延迟和流控分析。

## 1. 配置

```kconfig
# 前置依赖（见 ../README.md 第 2.1 节）
CONFIG_BLUETOOTH_EVENT_TRACE=y

# 启用 SPP Trace Spec（需要 BLUETOOTH_SPP=y）
CONFIG_BT_TRACE_SPP=y
```

`BT_TRACE_SPP` 会自动 select `BT_PROBE_HCI`、`BT_PROBE_RFCOMM`、`BT_PROBE_SPP` 三个 probe 层。

## 2. 打点位置

**TX 方向（发送）：**

```
App 写入 pipe
    │
SPP_TX_START (0x0400) ── SPP Service 从 pipe 读到数据
    │
SPP_TX_SEND  (0x0401) ── 调用 bt_sal_spp_write()，发送一个 RFCOMM 分片
    │
RFCOMM_TX    (0x0200) ── RFCOMM 构造 UIH 帧，发往 L2CAP
    │
HCI_STACK_TX_ACL (0x0000) ── 协议栈将 ACL 包交给 Controller
    │
HCI_H4_TX_DONE   (0x0003) ── H4 write() 返回
    │
HCI_NOCP         (0x0004) ── Controller 返回 Number Of Completed Packets
    │
SPP_TX_DONE  (0x0402) ── SPP 收到发送完成回调
```

**RX 方向（接收）：**

```
Controller 收到空口数据
    │
HCI_H4_RX_ACL    (0x8001) ── H4 收到 ACL 包
    │
HCI_STACK_RX_ACL (0x8002) ── 协议栈收到 ACL 包
    │
RFCOMM_RX        (0x8200) ── RFCOMM 解析完成
    │
SPP_RX_START     (0x8400) ── SPP 收到数据，准备写入 pipe
    │
SPP_RX_DONE      (0x8401) ── pipe 写入完成
```

## 3. event_id 编码

```
bit [15]    : 方向    0 = TX,  1 = RX
bit [14:8]  : 协议层  0 = HCI, 2 = RFCOMM, 4 = SPP
bit [7:0]   : 序号    层内事件索引
```

完整事件表：

| event_id | 名称 | 方向 | 层 | payload 大小 |
|----------|------|------|-----|-------------|
| 0x0000 | HCI_STACK_TX_ACL | TX | HCI | 9 字节 |
| 0x0003 | HCI_H4_TX_DONE | TX | HCI | 9 字节 |
| 0x0004 | HCI_NOCP | TX | HCI | 6 字节 |
| 0x8000 | HCI_IRQ_RX_ACL | RX | HCI | 9 字节 |
| 0x8001 | HCI_H4_RX_ACL | RX | HCI | 9 字节 |
| 0x8002 | HCI_STACK_RX_ACL | RX | HCI | 9 字节 |
| 0x8003 | HCI_STACK_RX_EVT | RX | HCI | 9 字节 |
| 0x8005 | HCI_IRQ_RX_EVT | RX | HCI | 9 字节 |
| 0x8006 | HCI_H4_RX_EVT | RX | HCI | 9 字节 |
| 0x0200 | RFCOMM_TX | TX | RFCOMM | 5 字节 |
| 0x8200 | RFCOMM_RX | RX | RFCOMM | 5 字节 |
| 0x0400 | SPP_TX_START | TX | SPP | 4 字节 |
| 0x0401 | SPP_TX_SEND | TX | SPP | 5 字节 |
| 0x0402 | SPP_TX_DONE | TX | SPP | 3 字节 |
| 0x8400 | SPP_RX_START | RX | SPP | 4 字节 |
| 0x8401 | SPP_RX_DONE | RX | SPP | 2 字节 |

各事件的 payload 字段定义见 `spp_trace_format.json`。

## 4. HCI ACL 过滤逻辑

SPP Trace Spec 在 HCI 层只记录与 RFCOMM 数据传输相关的 ACL 包，自动跳过：
- 续传分片（PB flag = continuation）
- 信令通道（L2CAP CID < 0x0040）
- 复用控制帧（DLCI = 0）
- 非 UIH 帧（control & 0xEF ≠ 0xEF）

HCI EVT 只记录 NOCP（evt_code = 0x13），其他 HCI Event 不记录。

这些过滤逻辑是 SPP Spec 特有的。其他 Spec 可以实现不同的 HCI probe 过滤策略。

## 5. 运行时过滤器

5 个运行时过滤器，在采集时生效（不匹配的事件不写入 noteram）：

| 过滤器 | 作用范围 | 说明 |
|--------|---------|------|
| direction | 全部层 | TX / RX / ALL |
| layer | 全部层 | HCI / RFCOMM / SPP 的任意组合 |
| acl_handle | HCI 层 | 按 ACL 连接句柄过滤 |
| dlci | HCI + RFCOMM 层 | 按 RFCOMM DLCI 过滤（DLCI = SCN << 1 \| dir_bit） |
| port | SPP 层 | 按 SPP conn_id 过滤 |

## 6. CLI 命令

```
bt_trace spp dir <tx|rx|all>              方向过滤
bt_trace spp layer <hci|rfcomm|spp|all>   层过滤（可多选）
bt_trace spp port <port>                  SPP port 过滤
bt_trace spp dlci <dlci>                  RFCOMM DLCI 过滤（2-61）
bt_trace spp acl <acl_handle>             ACL handle 过滤
bt_trace spp reset                        重置所有过滤器
```

## 7. 使用示例

```bash
# 先配置过滤器
nsh> bt_trace spp dir tx
nsh> bt_trace spp dlci 10

# 启动系统 trace 开始采集
nsh> trace start

# 执行 SPP 数据传输...

# 停止并导出
nsh> trace stop
nsh> trace dump /data/spp_trace.log
```

## 8. 问题分析思路

### 8.1 TX 延迟定位

通过对比各层事件的时间戳差值，定位延迟瓶颈所在层：

| 区间 | 含义 |
|------|------|
| SPP_TX_START → SPP_TX_SEND | pipe 读取到发送调用 |
| SPP_TX_SEND → RFCOMM_TX | SPP → RFCOMM 封包 |
| RFCOMM_TX → HCI_STACK_TX_ACL | RFCOMM → HCI 封包 |
| HCI_STACK_TX_ACL → HCI_H4_TX_DONE | 协议栈到 H4 UART 写入 |
| HCI_H4_TX_DONE → HCI_NOCP | Controller 处理 + 空口传输 |
| SPP_TX_SEND → SPP_TX_DONE | 单个分片完整发送延迟 |

### 8.2 流控状态观察

| 字段 | 观察点 |
|------|--------|
| HCI ACL 记录的 `acl_quota` | 值持续为 1 表示 HCI TX buffer 即将耗尽 |
| HCI_NOCP 的 `acl_quota` | 反映 Controller 归还 buffer 后的剩余量 |
| RFCOMM_TX 的 `peer_credits` | 值为 0 或 1 表示 RFCOMM 发送配额即将耗尽 |
| RFCOMM_RX 的 `local_credits` | 值过低表示本端接收配额不足 |
| SPP_TX_SEND 的 `quota` | SPP 层发送配额 |

## 9. 源码

| 文件 | 说明 |
|------|------|
| `bt_trace_spp.h` | 过滤器 API、方向/层常量、event_id 编码规则 |
| `bt_trace_spp.c` | HCI / RFCOMM / SPP 三层 probe 实现和过滤逻辑 |
| `bt_trace_spp_cmd.c` | CLI 过滤器命令（dir / layer / port / dlci / acl / reset） |
| `spp_trace_format.json` | 事件格式定义（event_id → payload 字段映射） |
