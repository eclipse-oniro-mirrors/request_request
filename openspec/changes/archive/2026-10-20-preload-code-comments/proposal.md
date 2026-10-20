# Proposal

## Why

预加载（cacheDownload）功能代码的注释现状两极分化，且错误注释具有误导性：

- **错误注释比缺失更危险**：耗时统计单位写成 seconds（实际源头 netstack_rs 全部为 milliseconds，错误从 `cache_download/src/info.rs` 传播到了公共头文件 `request_preload.h`）；全局重试默认值写成 3（实际为 1）；`create_file` 注释描述了一套并不存在的"临时文件+重命名"原子写机制，掩盖了真实的非原子落盘行为。
- **注释缺口集中在 JS 边界**：`js/napi/cache_download`（878 行的 `preload_module.cpp` 及其回调管理器）、`js/napi/preload_napi`（22 个工具函数）、`cache_download` 的 cxx 桥 C++ 侧（`preload_callback.cpp/.h`）几乎零注释，其中包含按 URL 注册回调、调用中延迟删除、文件操作串行队列等不读实现无法理解的机制。

本变更是注释整改的第一阶段（预加载）；第二阶段（上传下载主链路 `frameworks/js/napi/request`、`frameworks/native/request`、`services`）另行立项。

## What Changes

- 修正探索阶段确认的 10 处错误注释（完整清单见 tasks.md 第 1 组）：
  - 耗时单位 seconds -> milliseconds（`info.rs` 9 个方法 + `request_preload.h` 8 个 getter）
  - 全局重试默认值 3 -> 1（`services.rs` 2 处）
  - `build_cached_files_index` 虚构的返回值描述
  - `remove_file` 虚构的返回值描述
  - `create_file` 虚构的临时文件+重命名机制描述
  - `fetch` "cache or network" -> 仅缓存
  - ANI 模块文档 "animation resources" 的定位错误
  - ANI `download` 示例字段名笔误
- 按优先级补全缺失注释（P0 -> P1 -> P2，范围见 design.md）
- **仅注释变更**：不修改任何代码行为、公共 API 语义、IPC/FFI 协议、构建配置、测试
- 探索中顺带发现的 5 个代码缺陷（如 `task.rs` ylong 分支编译错误）不在本变更修复，仅记录于 tasks.md 附录备查

## Capabilities

纯注释/文档变更，无 spec 级行为变化，已在 `.openspec.yaml` 设置 `skip_specs: true`。

### New Capabilities

无。

### Modified Capabilities

无。

## Impact

- 受影响代码（预加载全部功能代码，45 个文件 / ~9,895 行，不含 tests/benches/构建脚本）：
  - `frameworks/native/cache_core/`（Rust + C++）
  - `frameworks/native/cache_download/`（Rust + C++）
  - `frameworks/js/napi/cache_download/`、`frameworks/js/napi/preload_napi/`
  - `frameworks/ets/ani/cache_download/`
  - `interfaces/inner_kits/cache_download/native/include/request_preload.h`（**公共头文件，仅改注释、不动 ABI，修改前需用户确认**）
- 验证方式：`cargo fmt --check` + 构建 `request` target（rk3568，`--no-indep`）；注释变更无可测行为，测试条款按 AGENTS.md Done 定义说明豁免
- 不影响：运行时行为、二进制接口、既有测试结果
