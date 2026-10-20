# Tasks

> **工作规范（对全部任务生效）**
> - 注释语言：英文（沿用仓库现状；Rust rustdoc / C++ Doxygen 均为英文）
> - Rust 用 rustdoc（`//!` 模块级、`///` 项级）；C++ 用 Doxygen（`/** @brief */`、`///`）+ 必要的内联注释
> - 粒度：文件/模块级概览 + 公有项必注 + 非显而易见的私有成员与机制（延迟删除、串行队列、两级缓存回退等）加解释；不复述代码字面意思
> - 跨语言契约处必须显式标注双端同步：任务状态常量 ↔ `PreloadState`、`ErrorKind` 枚举顺序、`FfiPredownloadOptions.headers` 扁平数组、`usize::MAX`/`u32::MAX`/`-1` 哨兵值
> - **只改注释**：不动代码行为、不做无关格式化、不动构建配置与测试
> - 每完成一个任务组用 `git diff` 复核：变更行应全部是注释行

## 1. 错误注释修正（探索阶段已确认的 10 处 + 1 处语义澄清）

- [x] 1.1 `frameworks/native/cache_download/src/info.rs`：9 个耗时 getter 文档 "in seconds" 改为 "in milliseconds"（源头 `netstack_rs` 全部为毫秒）。验证：`grep -n "seconds" info.rs` 无残留
- [x] 1.2 `interfaces/inner_kits/cache_download/native/include/request_preload.h:195-207`：`CppDownloadInfo` 各 getter "(seconds)" 改为 "(milliseconds)"。**⚠️ 公共头文件，仅注释不动 ABI，执行前需用户明确确认**（已获用户批准，2026-10-20）。验证：7 处 getter 已改；`:98`/`:106` `TimeoutOptions` 的 "in seconds" 为正确 options 域单位（netstack `*1000` 转换链证实），保留不动
- [x] 1.3 `frameworks/native/cache_download/src/services.rs:87` 与 `:530`：全局重试默认值 "default: 3" 改为 "default: 1"（实际 `Mutex::new(1)`，NAPI/头文件/ANI 三侧默认均为 1）。验证：`grep -n "default: 3" services.rs` 无残留
- [x] 1.4 `frameworks/native/cache_core/src/manage.rs:634-659`：删除 `build_cached_files_index` 虚构的 "Returns an iterator" 描述，改为描述实际行为（扫描缓存目录、过滤残缺文件、按容量回建索引）。验证：文档与无返回值签名一致
- [x] 1.5 `frameworks/native/cache_core/src/data/file.rs:416-434`：删除 `remove_file` 虚构的 "# Returns Amount of memory released"。验证：无 Returns 段
- [x] 1.6 `frameworks/native/cache_core/src/data/file.rs:436-464`：`create_file` 文档改为描述实际行为（直接打开最终 `_F` 路径写入，非原子；崩溃可能留下看似完整的文件）。验证：文档不再提及 temporary file / rename
- [x] 1.7 `frameworks/native/cache_download/src/cxx/request_preload.cpp:417`：`fetch` 文档 "from cache or network" 改为仅缓存（与 `request_preload.h:355` 一致）。验证：`grep -n "or network" request_preload.cpp` 无残留
- [x] 1.8 `frameworks/ets/ani/cache_download/src/cache_download.rs:14`：模块文档 "for animation resources" 改为实际定位（`@ohos.request.cacheDownload` 的 ANI 绑定）；`:80-98` 示例字段 `header` 改为 `headers`。验证：`grep -n "animation" cache_download.rs` 无残留
- [x] 1.9 `frameworks/native/cache_download/src/wrapper.rs:297-303`：`set_file_cache_path` 文档澄清实际语义（将给定路径注册为 inotify 监听目录，触发删除联动清理缓存；并不设置缓存写入路径）。验证：文档描述与 `observe_image_file_delete` 行为一致

## 2. P0 补注释：js/napi/cache_download（NAPI 绑定主入口）

- [x] 2.1 `include/preload_module.h`：补 `CallbackInfo`/`CallbackManager` 类文档与成员注释，重点解释 `allCb_` 的 `pair<bool, napi_ref>`（bool=活跃标志）、`isInvoked_`/`toDeleteCount_` 的调用中延迟删除机制、`validCbNum`。验证：每个成员有注释
- [x] 2.2 `src/preload_module.cpp:44-68`：补常量块注释（URL/容量上限、权限名、retry/timeout 边界值及与 `request_preload.h` 的对应）。验证：每个常量有注释
- [x] 2.3 `src/preload_module.cpp:87-245`：补 `CallbackInfo` 5 个方法注释（注册去重、移除的延迟删除分支、invoke 时的清理顺序）。验证：每个方法有文档
- [x] 2.4 `src/preload_module.cpp:247-348`：补 `CallbackManager` 4 个方法注释（按 URL 分表、`napi_send_event` 切回 JS 线程）。验证：每个方法有文档
- [x] 2.5 `src/preload_module.cpp:350-419`：补 `CheckInternetPermission`/`CheckNetworkInfoPermission`（static 缓存语义）、`CreatePreloadCallback`（`ErrorKind`→`ErrorCode` 映射）注释。验证：每个函数有文档
- [x] 2.6 `src/preload_module.cpp:421-878`：补全部 napi 导出函数（download/cancel/setMemoryCacheSize/setFileCacheSize/setDownloadInfoListSize/getDownloadInfo/clearMemoryCache/clearFileCache/on*/off*/setGlobal*）、`ParseNumberField`、`NapiCreateEnum*`、`registerFunc`、`RegisterModule` 文档，说明 JS 签名、参数校验与错误抛出行为。验证：每个导出函数有文档

## 3. P0 补注释：js/napi/preload_napi（参数解析与转换工具）

- [x] 3.1 `include/napi_utils.h` + `src/napi_utils.cpp`：补全部工具函数（`GetValueType`/`GetValueString`/`GetPropertyNames`/`Convert2JSValue` 等）、`PRELOAD_NAPI_CALL(_BASE)` 宏、`NO_ARG`~`THE_ARG` 常量注释。验证：头文件中每个声明有文档
- [x] 3.2 `include/preload_common.h` + `src/preload_common.cpp`：补全部选项解析/构对象函数注释，重点：`SENTINEL_NOT_SET` 哨兵语义、`SetOptionsSslType`/`GetCacheStrategy` 的默认回退行为、`BuildInfo*` 三件套产出的 JS 对象形状。验证：头文件中每个声明有文档

## 4. P0 补注释：cache_download cxx 桥 C++ 侧

- [x] 4.1 `frameworks/native/cache_download/include/preload_callback.h` + `src/cxx/preload_callback.cpp`：补 `PreloadCallbackWrapper`/`PreloadProgressCallbackWrapper` 类与 `OnSuccess`/`OnFail`/`OnCancel`/`OnProgress` 方法、`SharedData`/`UniqueData`/`UniqueInfo`/`ShareTaskHandle` 四个 Box→智能指针工厂函数注释（与 Rust 侧 `wrapper.rs` 的桥接关系、线程安全前提）。验证：每个类与方法有文档

## 5. P1 补注释：cache_download Rust 零散缺失

- [x] 5.1 `src/wrapper.rs:270-282`：补 `ffi_fetch`/`ffi_get_download_info` 文档（缓存未命中返回 null 的语义）。验证：无缺文档的 FFI 方法
- [x] 5.2 `src/download/mod.rs:19-23`：补 `INIT`/`RUNNING`/`SUCCESS`/`FAIL`/`CANCEL` 逐项注释，并标注数值与 C++ `PreloadState` 枚举一一对应、为跨语言契约只增不改。验证：每个常量有注释且含契约警示
- [x] 5.3 `src/services.rs:611-673`：补 `callback_from_local_file`（索引恢复期的 RAM/文件两级回退）、`read_local_file_with_callback`、`complete_callback`、`CallbackResult` 文档。验证：无缺文档项
- [x] 5.4 `src/download/error.rs:129-144`：`ErrorKind` 枚举补顺序契约注释（`ffi_kind() as i32` 与 C++ `ErrorKind` 枚举值一一对应，双端不得独立重排）。验证：含契约警示

## 6. P1 补注释：cache_core 零散缺失

- [x] 6.1 `src/data/mod.rs:35`：补 `MAX_CACHE_SIZE` 注释（20MB 单条目上限，作用于 RAM 与文件两级），并说明与 `DEFAULT_RAM_CACHE_SIZE`（20MB RAM 总容量默认值）数值相同但含义不同。验证：常量有注释
- [x] 6.2 `include/inotify_event_listener.h` + `src/cxx/native_ffi.cpp` + `include/native_ffi.h`：补 `DirectoryMonitor` 成员、`REQUEST_FDSAN_TAG`、`NewDirectoryMonitor`/`StartObserve` 注释。验证：每个成员与函数有注释
- [x] 6.3 `src/manage.rs:673-675`：`fetch` 文档误用 `# Safety` 段（函数是安全的），改为普通说明 `'static` 生命周期要求的原因。验证：无 `# Safety` 段

## 7. P2 收尾：零散小项

- [x] 7.1 `frameworks/native/cache_core/src/update.rs:29-34`：删除注释掉的旧版 `Updater` 结构体（死代码注释）；修正 `Updater` 文档示例中不可编译的 `'static` 引用用法（改为说明性文字或可编译示例）。验证：无注释掉的代码块
- [x] 7.2 `frameworks/ets/ani/cache_download/src/callback.rs:115-124`：补 `CallbackWrapper` 两个 trait impl 方法的行为说明（转发到按 URL 管理的回调注册表）。验证：方法有注释
- [x] 7.3 `frameworks/native/cache_download/src/download/ylong/mod.rs` 与 `ylong/client.rs`：清理注释行尾随空格，校准 "unlimited redirects"（实为 `limited(usize::MAX)`）等措辞。验证：`grep -n " $" ` 无命中
- [x] 7.4 全量复核：按上述规范通读全部 45 个功能文件，捕获实施期间新发现的错误/缺失注释并按同规范处理（预期为少量）。验证：抽查 5 个文件的注释完整性与正确性。复核结论：修正 `inotify_event_listener.cpp` `SetupEpoll` 的 `@return`（实为 epoll_ctl 结果 0/-1，非文件描述符）；options 域超时单位确认为秒（`request_preload.h:106` + netstack `*1000` 转换链），仓内现存 `in seconds` 注释均正确；`cache_core/src/wrapper.rs` 3 处存量行尾空格（cxx 宏体内，rustfmt 不可见）记录不修

## 8. 验证（集成检查）

- [x] 8.1 `cargo fmt --check` 通过（对照仓库 rustfmt.toml）。结果：本变更触碰的全部 Rust 文件零 diff、零解析错误；仓库另有 19 个与本变更无关文件的存量 fmt diff（services/、common/database、ani/request、request_next），未触碰。注意：本机仅 stable 工具链，rustfmt.toml 的 nightly-only 选项（wrap_comments 等）无法本地启用；已用"新增行无超 100 字符"作 wrap_comments 代理检查通过
- [x] 8.2 构建 `request` target：`./build.sh --product-name rk3568 --build-target out/rk3568/build_configs/request/request:request --no-indep` 成功。**用户裁定豁免（2026-10-20）**：注释-only 变更无需构建验证——零代码行变更已由 diff 非注释过滤机器证明，rustfmt 全量解析通过，注释不可能改变编译产物；本机亦无 OH 源码根。如后续需要可在有构建环境的机器上运行上述命令复核
- [x] 8.3 `git diff` 全量复核：所有变更行均为注释行，无行为/格式化/配置变更。结果：23 文件、451+/82-，非注释行过滤零命中
- [x] 8.4 注释-only 变更无可测行为：在最终交付说明中按 AGENTS.md Done 定义声明测试条款豁免理由，并列出 `request_preload.h` 修改（若已获批准）作为公共头文件触碰记录。**公共头文件触碰记录**：`request_preload.h` 已按用户批准（2026-10-20）执行任务 1.2，仅 7 处 getter 注释 `(seconds)`→`(milliseconds)`，不动任何声明与 ABI

## Workflow follow-up

- 实施完成后归档本 change（`openspec archive`），归档前确认 8.x 全部通过
- 第二阶段（上传下载主链路注释整改）另行立项，不并入本 change

