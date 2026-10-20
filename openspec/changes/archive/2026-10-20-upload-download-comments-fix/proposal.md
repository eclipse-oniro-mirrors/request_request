# Proposal

## Why

继已归档的 `preload-code-comments`（阶段一）之后，对上传/下载主链路开展注释工作的第一波。本轮勘察（4 个并行评估 agent + 人工抽查 6 处、全部证实）发现全链路存在约 43 处**错误、误导或劣质**注释，分布在 NAPI 树、C++ 客户端（request_native）、Rust 服务层（service/manage/task）与 ANI 绑定。错误注释比没有注释更危险：它们对 IPC 语义、单位、死逻辑等给出错误契约描述，会直接误导后续维护者。必须先修错，再进行从零补写（Wave 2/3），否则补写会建立在错误基线上。

## What Changes

- 修正约 43 处错误/误导/劣质注释（按树分组，逐处以 file:line 落入 tasks）：
  - NAPI（`frameworks/js/napi/request`，~14 处）：空/非空语义写反、9+ 处头文件守卫与 `#endif` 注释不匹配、常量缺单位、"Only use for DOWNLOAD" 误导范围
  - C++ 客户端（`frameworks/native/request`，~8 处）：API9 行为描述与代码相反、不可读的"Not sure"类注释、"0 means no duration check" 与服务端 `client.rs` 语义不符、语序破碎句
  - Rust 服务层（`services/src/service`，~9 处）：UDS 截断对象写错（headers→extras）、uid 检查描述与替换语义相反、CLEAR/SHOW_PROGRESS 接口注释与实际 no-op/保留码行为不符、TODO 注释与行为相反
  - Rust 管理层（`services/src/manage` + `services/src/task`，~12 处）：max_speed "achieved" vs "limit"、陈旧的"一个月未活跃"逻辑描述、SAFETY 注释缺真实不变量、重复裸常量互引
- 核实并修正 `frameworks/ets/ani/request` api10 `bridge.rs` 三处 "in seconds" 注释（先验证真实单位再改）
- 删除死代码注释块（`response_message_receiver.cpp` 的 INT8_SIZE）
- 在 tasks.md 附录记录勘察中发现的约 10 处代码 bug —— **仅记录，不修复**（用户指示）
- 不做任何行为变更：每一行改动都必须是注释

## Capabilities

### New Capabilities

（无）

### Modified Capabilities

（无 —— 纯注释修正，无 spec 级行为变更，`.openspec.yaml` 设置 `skip_specs: true`）

## Impact

- 代码：约 25 个文件，横跨 `frameworks/js/napi/request/`、`frameworks/native/request/`、`services/src/`、`frameworks/ets/ani/request/`
- 无 API / IPC / UDS / 行为 / 依赖变更；不改测试
- 验证方式：`cargo fmt --check` + git diff 非注释行过滤（逐组复核全部改动行为注释行）；构建豁免（用户已裁定注释类改动无需构建，本机亦无 OH 源码根）
- 后续波次不受影响：Wave 2（C++ 从零补写）、Wave 3（Rust 补漏 + 契约警示）另立 change
