# Zephyr SAL LE 地址回调适配

## 背景

蓝牙服务层上层业务获取 LE 地址的链路：
1. LE 地址变更（RPA 刷新 / 业务主动设置）→ 底层协议栈触发回调
2. `adapter_on_le_addr_update` → 更新 `adapter_service.c` 的 `le_properties`
3. 上层业务调 `adapter_get_le_address` → 直接从 `le_properties` 读取

Bluelet 老协议栈通过 `ble_address_callback` → `adapter_on_le_addr_update` 完成通知。Zephyr SAL 层缺少这个回调，导致上层拿不到更新后的 LE 地址。

## 参考实现（Bluelet）

### 回调注册链路
```
Adpt_Gap_LELocalAddrCbk()
  → ADPT_GAP_CBKS.gap_ble_address_cb(bd, type)
    → ble_address_callback()  // bluelet/sal_adapter_interface.c
      → adapter_on_le_addr_update(&addr, ble_addr_type)
```

### 触发场景
| 场景 | 函数 |
|------|------|
| 设置随机地址成功（含 RPA 刷新） | `Adpt_HCI_LESetRandomAddrCfm` |
| 主动获取 LE 地址 | `Adpt_Gap_FsmBLEGetAddress` |

## 适配方案

在 `frameworks/connectivity/bluetooth/service/stacks/zephyr/sal_adapter_le_interface.c` 中，将 3 个函数改为 `STACK_CALL` worker 模式，通过 `sal_send_req` 投递到 service loop 异步执行，操作成功后回调 `adapter_on_le_addr_update`。

### 改动点

| 函数 | worker | 行为 |
|------|--------|------|
| `bt_sal_le_set_static_identity` | `zblue_le_set_static_identity` | `bt_id_set_default_addr_mc` 成功后回调，type=RANDOM |
| `bt_sal_le_set_public_identity` | `zblue_le_set_public_identity` | `bt_id_set_default_addr_mc` 成功后回调，type=PUBLIC |
| `bt_sal_le_get_address` | `zblue_le_get_address` | `bt_id_get` 获取当前地址后回调，type 根据实际判断 |

`bt_sal_le_set_address` 内部走 `bt_sal_le_set_static_identity`，自动覆盖。

### 代码示例

```c
static void STACK_CALL(le_set_static_identity)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t le_addr = { .type = BT_ADDR_LE_RANDOM };

    memcpy(&le_addr.a, &req->addr, sizeof(le_addr.a));

    if (bt_id_set_default_addr_mc(req->id, &le_addr) < 0) {
        BT_LOGE("%s, set static identity fail", __func__);
        return;
    }

    adapter_on_le_addr_update(&req->addr, BT_LE_ADDR_TYPE_RANDOM);
}

static void STACK_CALL(le_get_address)(void* args)
{
    sal_adapter_req_t* req = args;
    bt_addr_le_t got = { 0 };
    bt_address_t addr;
    ble_addr_type_t addr_type;
    size_t count = 1;

    UNUSED(req);

    bt_id_get(&got, &count);
    bt_addr_set(&addr, (uint8_t*)&got.a);
    addr_type = (got.type == BT_ADDR_LE_RANDOM) ? BT_LE_ADDR_TYPE_RANDOM : BT_LE_ADDR_TYPE_PUBLIC;

    adapter_on_le_addr_update(&addr, addr_type);
}
```

## 涉及文件

| 文件 | 说明 |
|------|------|
| `frameworks/connectivity/bluetooth/service/stacks/zephyr/sal_adapter_le_interface.c` | **修改** - 新增 worker 回调 |
| `frameworks/connectivity/bluetooth/service/stacks/bluelet/sal_adapter_interface.c` | 参考 - bluelet 的 `ble_address_callback` |
| `external/bluelet/bluelet/src/samples/stack_adapter/src/stack_adapter_gap.c` | 参考 - `Adpt_Gap_LELocalAddrCbk` 触发场景 |
| `frameworks/connectivity/bluetooth/service/src/adapter_service.c` | 上层 - `adapter_on_le_addr_update` 定义 |

## 遗留项

- Zephyr RPA 15 分钟自动刷新（`rpa_timeout` → `le_update_private_addr` → `set_random_address`）路径目前无 hook 点，如需支持需在 zephyr 栈层 `id.c` 的 `set_random_address` 成功后加通知
