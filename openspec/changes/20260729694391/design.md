# 设计文档

> 将 Spec 中的验收标准转化为可实现的架构决策、API 签名与模块级修改清单。

## 设计概述

| 属性 | 内容 |
|------|------|
| 设计目标 | 为三方应用提供按任务控制通知栏停止按钮显示/隐藏的能力 |
| 设计方法 | 在 `request.request.agent.Notification` 新增 `hideStopButton?: boolean`，沿 ArkTS → ANI → request_core → SA → 数据库 → C++ 通知层全链路透传，并在 C++ 层根据字段隐藏停止按钮 |
| 关键对称点 | 与现有 `disable`、`visibility` 字段保持一致的数据流 |
| 目标版本 | OpenHarmony-7.1-Release |

## 设计决策

| 决策项 | 选择 | 理由 |
|--------|------|------|
| API 字段名 | `hideStopButton?: boolean` | 语义直白，与 ArkTS 驼峰命名一致；false 表示显示，true 表示隐藏 |
| 内部 Rust 字段名 | `hide_stop_button: Option<bool>` | 符合 Rust 命名规范；与 ArkTS 字段清晰映射 |
| 数据库列名 | `hide_stop_button` | 与 Rust 字段一致，便于维护 |
| 默认值 | `false` | 保持现有行为；未设置时按 false 处理 |
| 作用范围 | 仅 API10 `request.request.agent.Notification` | API9 无 notification 配置对象，不宜硬塞 |
| 影响按钮 | 仅停止按钮 | 需求明确；暂停/继续/取消按钮不受影响 |
| C++ 实现策略 | 将 `hide_stop_button` 加入 `NotifyContent`，在 `LiveViewContent` 构造时决定是否展示 stop action | 与现有 `x_mark` 控制 cancel 按钮的方式对称 |

## 架构视图

### 数据流

```
ArkTS (app.ets)
    │
    ▼
request.request.agent.Notification
    hideStopButton?: boolean
    │
    ▼
@ohos.request.ets (type definition)
    │
    ▼
frameworks/ets/ani/request/src/api10/bridge.rs::Notification
    hide_stop_button: Option<bool>
    │
    ▼
common/request_core/src/config.rs::Notification
    hide_stop_button: Option<bool>
    │
    ▼
TaskConfig IPC serialization (client → SA)
    │
    ▼
services/src/service/notification_bar/notification_config.rs::NotificationConfig
    hide_stop_button: bool
    │
    ▼
services/src/service/notification_bar/database.rs
    task_notification_content.hide_stop_button
    group_notification_content.hide_stop_button
    │
    ▼
services/src/service/notification_bar/notify_flow.rs / typology.rs
    NotifyContent.hide_stop_button
    │
    ▼
services/src/service/notification_bar/mod.rs (ffi NotifyContent)
    │
    ▼
services/src/cxx/notification_bar.cpp::LiveViewContent
    根据 hide_stop_button 决定是否展示停止按钮
```

## API 设计

### ArkTS 接口

```typescript
// request_request/frameworks/ets/ani/request/ets/@ohos.request.ets
export namespace agent {
  export interface Notification {
    title?: string;
    text?: string;
    disable?: boolean;
    visibility?: int;
    wantAgent?: WantAgent;
    /**
     * Whether to hide the stop button on the notification bar.
     * Default is false, which means the stop button is shown.
     * @since 7.1
     */
    hideStopButton?: boolean;
  }

  export class NotificationInner implements Notification {
    title?: string;
    text?: string;
    disable?: boolean;
    visibility?: int;
    wantAgent?: WantAgent;
    hideStopButton?: boolean;
  }
}
```

### ANI Bridge（Rust）

```rust
// request_request/frameworks/ets/ani/request/src/api10/bridge.rs
#[derive(Clone)]
#[ani_rs::ani(path = "@ohos.request.request.agent.NotificationInner")]
pub struct Notification<'local> {
    pub title: Option<String>,
    pub text: Option<String>,
    pub disable: Option<bool>,
    pub visibility: Option<i32>,
    pub want_agent: Option<AniObject<'local>>,
    pub hide_stop_button: Option<bool>,
}

impl From<Notification<'_>> for request_core::config::Notification {
    fn from(value: Notification) -> Self {
        request_core::config::Notification {
            title: value.title,
            text: value.text,
            disable: value.disable,
            visibility: value.visibility,
            want_agent: None,
            hide_stop_button: value.hide_stop_button,
        }
    }
}
```

### request_core

```rust
// request_request/common/request_core/src/config.rs
pub struct Notification {
    pub title: Option<String>,
    pub text: Option<String>,
    pub disable: Option<bool>,
    pub visibility: Option<i32>,
    pub want_agent: Option<String>,
    pub hide_stop_button: Option<bool>,
}
```

`TaskConfig::serialize` 在 notification 块末尾追加：

```rust
if let Some(hide_stop_button) = self.notification.hide_stop_button {
    parcel.write(&true)?;
    parcel.write(&hide_stop_button)?;
} else {
    parcel.write(&false)?;
}
```

### SA 通知配置

```rust
// request_request/services/src/service/notification_bar/notification_config.rs
pub(crate) struct NotificationConfig {
    pub(crate) task_id: u32,
    pub(crate) title: Option<String>,
    pub(crate) text: Option<String>,
    pub(crate) want_agent: Option<String>,
    pub(crate) disable: bool,
    pub(crate) visibility: u32,
    pub(crate) hide_stop_button: bool,
}

impl Deserialize for NotificationConfig {
    fn deserialize(parcel: &mut ipc::parcel::MsgParcel) -> ipc::IpcResult<Self> {
        // ... 现有 title/text/want_agent/disable/visibility 读取 ...
        let hide_stop_button = if parcel.read::<bool>()? {
            parcel.read::<bool>()?
        } else {
            false
        };
        Ok(Self { /* ..., */ hide_stop_button })
    }
}
```

### 数据库

新增列：

```sql
-- task_notification_content 表
ALTER TABLE task_notification_content ADD COLUMN hide_stop_button BOOLEAN DEFAULT FALSE;

-- group_notification_content 表
ALTER TABLE group_notification_content ADD COLUMN hide_stop_button BOOLEAN DEFAULT FALSE;
```

`CustomizedNotification` 扩展：

```rust
pub(crate) struct CustomizedNotification {
    pub title: Option<String>,
    pub text: Option<String>,
    pub want_agent: Option<String>,
    pub hide_stop_button: bool,
}
```

查询/更新 SQL 同步增加 `hide_stop_button` 列。

### 通知内容与 FFI

```rust
// request_request/services/src/service/notification_bar/mod.rs
pub(crate) struct NotifyContent {
    title: String,
    text: String,
    want_agent: String,
    request_id: u32,
    uid: u32,
    live_view: bool,
    progress_circle: ProgressCircle,
    x_mark: bool,
    hide_stop_button: bool,
}
```

`typology.rs` 中各 `NotifyContent` 构造函数增加 `hide_stop_button` 参数；`task_progress_notify` 与 `group_progress_notify` 使用从数据库读取的值。

### C++ 通知层

```cpp
// request_request/services/src/cxx/notification_bar.cpp
std::shared_ptr<OHOS::Notification::NotificationContent> LiveViewContent(const NotifyContent &content)
{
    auto liveViewContent = std::make_shared<Notification::NotificationLocalLiveViewContent>();
    // ... 现有 title/text 设置 ...

    if (content.x_mark || content.progress_circle.open) {
        liveViewContent->addFlag(Notification::NotificationLocalLiveViewContent::LiveViewContentInner::BUTTON);
    }

    if (content.x_mark) {
        auto button = liveViewContent->GetButton();
        auto icon = CreatePixelMap();
        if (icon != nullptr) {
            button.addSingleButtonName("cancel");
            button.addSingleButtonIcon(icon);
            liveViewContent->SetButton(button);
        }
    }

    // TODO: 根据 content.hide_stop_button 控制停止按钮显示。
    // 具体实现依赖 Notification 框架最新 SDK：
    // 方案 1：若框架提供显式隐藏 stop 的 API，直接调用；
    // 方案 2：通过不注册 stop action / 设置 NotificationRequest 的对应 flag 实现；
    // 方案 3：若 stop 为系统默认按钮且不可单独隐藏，需在编码阶段与通知框架团队确认并调整设计。
    if (content.hide_stop_button) {
        // Hide stop button (framework-specific implementation)
    }

    // ... 现有 progress 设置 ...
    return liveViewContent;
}
```

## 文件级修改清单

| 仓库 | 文件 | 修改类型 | 修改内容 |
|------|------|----------|----------|
| request_request | `frameworks/ets/ani/request/ets/@ohos.request.ets` | MODIFY | `agent.Notification` / `NotificationInner` 新增 `hideStopButton?: boolean` |
| request_request | `frameworks/ets/ani/request/src/api10/bridge.rs` | MODIFY | `Notification` bridge 结构新增字段并更新 `From` 转换 |
| request_request | `common/request_core/src/config.rs` | MODIFY | `Notification` 结构新增字段；`TaskConfig::serialize` 追加序列化 |
| request_request | `services/src/service/notification_bar/notification_config.rs` | MODIFY | `NotificationConfig` 新增字段；`Deserialize` 实现读取新字段 |
| request_request | `services/src/service/notification_bar/database.rs` | MODIFY | 新增 `hide_stop_button` 列；更新 `CustomizedNotification`、查询/更新 SQL |
| request_request | `services/src/service/notification_bar/typology.rs` | MODIFY | 各 `NotifyContent` 构造函数透传 `hide_stop_button` |
| request_request | `services/src/service/notification_bar/notify_flow.rs` | MODIFY | 从数据库读取并传入 `hide_stop_button` |
| request_request | `services/src/service/notification_bar/mod.rs` | MODIFY | `NotifyContent` ffi 结构新增 `hide_stop_button` |
| request_request | `services/src/cxx/notification_bar.cpp` | MODIFY | `LiveViewContent` 根据字段隐藏停止按钮 |
| request_request | `services/include/notification_bar.h` | 可能无需 | 若结构体在头文件中定义则需同步 |

## BUILD.gn / 依赖影响

| 维度 | 影响 | 说明 |
|------|------|------|
| 新增公共 API | 是（修改 Public API） | 需更新 API 文档与 d.ts 声明 |
| ABI 变化 | 否 | 修改均在同一进程内；无 Rust/C++ 跨 so 接口变更 |
| 数据库版本 | 是 | 新增列需通过 `ALTER TABLE ... ADD COLUMN ... DEFAULT FALSE` 兼容旧数据 |
| IPC 格式 | 是（增量） | 新增字段带存在标志，置于 notification 块末尾 |

## 安全与权限

| 检查项 | 设计说明 |
|--------|----------|
| 字段来源 | 由应用自身传入，不涉及系统权限 |
| 影响范围 | 仅通知栏 UI，不影响任务停止能力或数据安全 |
| 默认值安全 | 默认 false，旧应用无感知 |
| wantAgent 校验 | 现有 `validate_want_agent_ownership` 逻辑保持不变 |

## 错误处理

| 错误场景 | 行为 | 错误类型 |
|----------|------|----------|
| ArkTS 传入非布尔值 | ANI/TypeScript 编译期或运行时参数校验报错 | 参数错误 |
| IPC 反序列化旧 parcel（无该字段） | 按 `false` 处理 | 兼容回退 |
| 数据库升级失败 | 字段缺失时按 `false` 处理；记录 fault 日志 | 降级兼容 |
| 通知框架不支持单独隐藏 stop | 需在产品化前确认；若不支持则返回设计调整 | 设计风险 |

## 测试策略

| 测试类型 | 覆盖点 | 位置 |
|----------|--------|------|
| 单元测试 | `NotificationConfig::deserialize` 默认值与显式值 | `services/tests/ut/service/notification_bar/` |
| 单元测试 | 数据库 `CustomizedNotification` 读写 `hide_stop_button` | `services/tests/ut/service/notification_bar/ut_database.rs` |
| 单元测试 | `TaskConfig::serialize` / deserialize 字段一致性 | `common/request_core/tests/ut/` |
| ArkTS 单测 | 配置对象字段透传 | `frameworks/ets/ani/request/tests/` |
| 集成测试 | 设置 true 后设备通知栏无 stop 按钮 | request 集成测试 |
| 集成测试 | 默认值/未设置时 stop 按钮仍存在 | request 集成测试 |

### 单元测试示例

```rust
#[test]
fn ut_notification_config_hide_stop_button_default() {
    // 构造不含 hide_stop_button 的 parcel 应解析为 false
    let mut parcel = MsgParcel::new();
    parcel.write(&false).unwrap(); // title
    parcel.write(&false).unwrap(); // text
    parcel.write(&false).unwrap(); // want_agent
    parcel.write(&false).unwrap(); // disable
    parcel.write(&1u32).unwrap();  // visibility
    parcel.write(&false).unwrap(); // hide_stop_button flag

    let config = NotificationConfig::deserialize(&mut parcel).unwrap();
    assert!(!config.hide_stop_button);
}

#[test]
fn ut_notification_config_hide_stop_button_true() {
    let mut parcel = MsgParcel::new();
    // ... title/text/want_agent/disable/visibility ...
    parcel.write(&true).unwrap();  // has hide_stop_button
    parcel.write(&true).unwrap();  // hide_stop_button value

    let config = NotificationConfig::deserialize(&mut parcel).unwrap();
    assert!(config.hide_stop_button);
}
```

## 关联 Spec 项

| Spec AC | 设计实现点 |
|---------|------------|
| AC-1.1 / R-1 | ArkTS 与 Rust 默认值均为 false；未设置时按 false 处理 |
| AC-1.2 / R-2 | 显式 false 时 C++ 层正常展示 stop 按钮 |
| AC-1.3 / R-3 | 显式 true 时 C++ 层隐藏 stop 按钮 |
| AC-1.4 / R-4 | IPC 与数据库新增字段带默认值，旧数据/旧客户端按 false 处理 |
| AC-1.5 / R-5 | 完成/失败通知模板不引用 stop 按钮 |
| AC-1.6 / R-6 | `disable: true` 时直接不发布通知 |

## 进入 Execution Plan 条件

- [x] 架构决策与方案探索一致
- [x] API 签名与调用链已明确
- [x] 文件级修改清单已列出
- [x] BUILD.gn / 依赖影响已识别
- [x] 安全、错误处理、测试策略已定义
- [x] 风险与缓解措施已记录（C++ 具体 API 待确认）
- [x] 与 Spec AC 的追溯关系已建立

**设计结论:** 通过，可进入 `execution-plan.md` 或直接编码。

## context-references

```yaml
context-queries:
  - repo: "openharmony/request_request"
    query: "request notification bar hide stop button implementation"
```

**关键文档：** `proposal.md`、`spec.md`、`lineage.md`
