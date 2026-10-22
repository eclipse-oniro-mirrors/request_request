# Tasks

## 工作规范（全组适用）

- **只新增/改注释**：`+` 行全部为注释或空行；不改代码、不改日志字符串/DFX、不动配置与测试、不格式化无关代码。
- **注释语言与风格**：英文。C++ 声明用 `///`（Doxygen 风格，范例：`frameworks/native/request/include/multi_instance_runcount_manager.h`——类级讲 what/why/who，方法级写行为契约），实现内部说明用普通 `//`。
- **密度基准**（按范例）：文件头一句职责；类/结构体级文档；公开方法级文档（行为、单位、返回语义）；关键成员说明；非显然逻辑的行内注释。**不写**逐行流水账，不给一目了然的 getter/setter 单独注释。
- **先读懂再落笔**：每条注释写前必须沿调用链核实实际行为；锚点：`docs/agent/feature-client.md`（§3.1 NAPI 线程模型、§3.2 IPC 契约、§3.3 UDS 协议、V9/V10 双事件表）+ Wave 1 事实基线（speed 单位 bytes/s、时间戳 ms、timeout seconds、API9 吞全部错误码等）。不写推测性注释。
- **Wave 1 已修正的注释是已核实基线，不要动**：9 个头文件的 `#endif` 守卫注释、`napi_utils.cpp:761`、`js_notify_data_listener.cpp:96`、`request_event.cpp:26`（16 KB/s）与 `:497`（错误码清单）、`js_task.cpp:197/:251/:575`、`js_initialize.cpp:31/:47`、`js_initialize_config.cpp`（ParseSaveas）、`js_initialize_path.cpp:82`（CheckBelongAppBaseDir）、`js_initialize_file.cpp:148/:273`、`curl_adp.cpp`（GlobalInit no-op NOTE）与 `curl_adp.h:90`（µs）。新写的注释不得与这些已核实语义矛盾。
- **代码 bug 只记录不修**：阅读中发现的问题一律进附录 B（沿用 Wave 1 附录 A 先例），不在本 change 修复；Wave 1 附录 A（A1-A12）已有条目不重复登记，只补新发现。
- **验证方法**：每组完成后 `git diff -- <该组路径>` 肉眼复核；收尾跑全量 `git diff -U0` 非注释行过滤（过滤正则沿用 Wave 1：`^[+-][[:space:]]*(//|/\*|\*|//!|///)` 及空行；纯新增场景下残留必须全为空 `+` 行或注释改写的 `−` 行）。构建豁免（用户已裁定注释类改动无需构建）。本树纯 C++，无 cargo fmt 项。
- **行数为勘察时快照**，apply 时以文件内容定位为准。

## 1. 模块入口与公共设施

- [x] 1.1 `src/request_module.cpp`（282 行）— NAPI 模块注册与 JS API 分发表：`nm_modname="request"`、static import、Init() 里 JS 属性→C++ 函数的映射；注明模块名/安装目录与 JS import 路径的强绑定（feature-client §3.4）
- [x] 1.2 `include/async_call.h` + `src/async_call.cpp`（141+155）— AsyncCall 异步调用骨架：Context 生命周期（构造校验→execute→complete 的三段式）、WorkData 与 ContextNapiHolder 各自角色、与 napi_create_async_work 的关系
- [x] 1.3 `include/napi_utils.h` + `src/napi_utils.cpp`（161+868）— napi_value↔C++ 类型转换与 JS 对象构建工具集：字符串/数值/枚举转换、TaskConfig→JS 对象的组装职责
- [x] 1.4 `include/path_utils.h` + `src/path_utils.cpp`（36+233）— 路径工具：AREA1/2/5 常量（Wave 1 已核实于 :36-38）、路径归属/合法性校验
- [x] 1.5 `include/app_state_callback.h` + `src/app_state_callback.cpp`（67+63）— 应用前后台状态回调；具体用途（旧版上传后台暂停？）沿注册点调用链核实后再写
- [x] 1.6 `include/notification_bar.h` + `src/notification_bar.cpp`（29+329）— NAPI 侧通知栏；具体职责（旧版上传进度通知？）实施时核实

## 2. 任务对象与初始化

- [x] 2.1 `include/js_task.h` + `src/js_task.cpp`（140+1265）— JsTask JS 任务对象：JS 侧生命周期、ContextInfo/ContextCallbackData/TouchContext 等上下文结构、任务方法（start/stop/pause/touch 等）的 JS 语义与 API9/10 行为分支
- [x] 2.2 `include/js_initialize.h` + `src/js_initialize.cpp`（131+162）— request.agent 初始化入口：JS 配置对象解析的入口分发（含 Wave 1 已改写的两处 todo 结论）
- [x] 2.3 `src/js_initialize_config.cpp`（869）— 配置字段解析：action/mode/网络/超时/限速/headers 等字段的解析规则与默认值（单位按事实基线：speed bytes/s、timeout seconds）
- [x] 2.4 `src/js_initialize_file.cpp`（428）— files 数组解析：普通 file 与 user file 的解析差异（user file 仅 API10+，Wave 1 已核实）、内部存储路径校验
- [x] 2.5 `src/js_initialize_path.cpp`（218）— 路径相关解析：saveas 等（Wave 1 已核实 CheckBelongAppBaseDir 真实校验范围，新注释保持一致）

## 3. 事件与监听（UDS→JS 回调链）

- [x] 3.1 `include/request_event.h` + `src/request_event.cpp`（99+625）— 事件订阅与分发：supportEventsV9_/supportEventsV10_ 双表语义（勿混用，feature-client P2）、JsParam/ExecContext 结构、`on('progress' 等)` 的注册与触发路径
- [x] 3.2 `include/js_response_listener.h` + `src/js_response_listener.cpp`（41+76）— UDS 消息→JS 回调的线程切换：§3.1 三步链（napi_send_event→handle scope→OnMessageReceive）；注释必须警示"非 JS 线程不得直接调 napi 函数"
- [x] 3.3 `include/js_notify_data_listener.h` + `src/js_notify_data_listener.cpp`（62+320）— NotifyData 事件缓存生命周期（NotifyDataPtr/ReasonDataPtr 结构；以 Wave 1 已改写的事件缓存注释为基准展开）
- [x] 3.4 `include/listener_list.h` + `src/listener_list.cpp`（53+124）— 监听器列表管理：注册/注销/分发遍历的线程安全前提

## 4. Legacy（API9 旧路径）

- [x] 4.1 `include/legacy/download_task.h` + `src/legacy/download_task.cpp`（71+246）— 旧版 download 任务对象：DownloadOption 结构、与新版 JsTask 的委托关系
- [x] 4.2 `include/legacy/request_manager.h` + `src/legacy/request_manager.cpp`（92+306）— 旧版请求管理：DownloadDescriptor/CallFunctionData、JS 回调分发的异步模型

## 5. Upload 子树（进程内 curl 上传）

- [x] 5.1 `include/upload/upload_common.h`（56）+ `include/upload/upload_hilog_wrapper.h`（73）— 公共类型（Type/UploadErrorCode/TaskState）与日志域/子模块封装（文件级与枚举级文档即可，宏不逐条注释）
- [x] 5.2 `include/upload/upload_config.h`（71）— File/RequestData/UploadResponse/UploadConfig/FileData 结构文档（字段单位、curl_slist 职责归属）
- [x] 5.3 `include/upload/async_call.h` + `src/upload/async_call.cpp`（107+151）— upload 子树独立的 AsyncCall 副本：与根树 1.2 的实现差异需注明（为何存在两份）
- [x] 5.4 `include/upload/js_util.h` + `src/upload/js_util.cpp`（68+364）— upload 专用 JS 工具（与根树 napi_utils 的分工）
- [x] 5.5 `include/upload/i_file_adapter.h` + `include/upload/file_adapter.h` + `src/upload/file_adapter.cpp`（33+29+38）— 文件源抽象与实现：接口职责、流式读取契约
- [x] 5.6 `include/upload/obtain_file.h` + `src/upload/obtain_file.cpp`（50+145）— 文件获取：内部存储路径与外部 URI/fd 的来源分支
- [x] 5.7 `include/upload/curl_adp.h` + `src/upload/curl_adp.cpp`（95+476）— curl 适配器：多文件顺序/并发上传控制、libcurl 回调集（write/header/progress）、上传状态机与通知触发；Wave 1 已核实的 no-op GlobalInit 与 FILE_UPLOAD_INTERVAL（µs）为基线
- [x] 5.8 `include/upload/i_upload_task.h` + `include/upload/upload_task.h` + `src/upload/upload_task.cpp`（25+87+192）— 上传任务接口与实现：UploadTaskState 枚举、观察者注册、remove 流程
- [x] 5.9 `include/upload/upload_task_napiV5.h` + `src/upload/upload_task_napiV5.cpp`（85+254）— API5-9 旧上传 NAPI 封装：SystemFail/Success/CompleteCallback 三个回调结构与 RecycleRef 的内存回收语义（重点核实）

## 6. 全量复核与收尾

- [x] 6.1 全量 `git diff -U0` 非注释行过滤：`+` 行全为注释或空行；`−` 行仅限注释改写且代码前缀逐字节不变（纯新增为主的场景，残留应接近全空 `+` 行）— **结果：全树 CLEAN，且 0 个 `−` 行，本变更为纯新增 diff**
- [x] 6.2 附录 B 新发现问题清单终稿：逐条 file:line + 一句话描述，标注"仅记录未修复"；与 Wave 1 附录 A 去重 — **终稿 B1-B14；B1 已确认（11 处调用点全部先 SetInput）；B3/B7/B13/B14 全仓 grep 已复核**
- [x] 6.3 语义一致性抽查：新注释与 Wave 1 已修正注释、proposal 事实基线（单位/生命周期/线程模型）无矛盾 — **抽查项：16 KB/s（:26 基线 vs :156 新注释一致）、timeout seconds、token 8-2048 bytes、MAX_STRING_LENGTH=64KB（napi_utils.h:61 实证）、READFILE 30 s、USLEEP 50 ms；纯新增 diff 保证所有基线注释字节不变**

## Workflow follow-up

- 全部任务完成后 `/opsx:archive napi-comments-add`（design 有意跳过、specs 以 skip_specs 豁免，按 phase-1/Wave-1 先例带警告归档）。
- 归档后 `git commit -s -m`，message 末行 `Co-Authored-By: Agent`。
- Batch 2（`frameworks/native/request/` 29 文件 + `services/src/cxx/` 16 文件）与相邻树（cj / request_action / js/ani 遗留）另行立项。

---

## 附录 B：实施中发现的代码问题（仅记录，未修复）

| # | 位置 | 问题 | 状态 |
|---|---|---|---|
| B1 | `include/async_call.h` Context | `env_`/`innerCode_`/`withErrCode_`/`version_` 无默认成员初始化；`input_ == nullptr` 时构造函数早退不设 `innerCode_`，`Call()` 读取未初始化值。**已确认**：全树 11 处 AsyncCall 调用点均在构造前 `SetInput(...)` 链式注入非空 input（js_task 6、notification_bar 3、request_event 2），`input_ == nullptr` 在现存代码中不可达，风险仅为理论性 | 仅记录，已确认 |
| B2 | `src/napi_utils.cpp:703` `Convert2FileVector` | `version` 形参从未使用（调用方仍传 "API8"/protocolVersion） | 仅记录 |
| B3 | `include/napi_utils.h` | `MAX_NUMBER_BYTES=8`、`MAX_LEN=4096` 两常量在 NAPI 树内无引用（死常量；删除属代码变更）。全仓 grep 已复核：`NapiUtils::MAX_NUMBER_BYTES`/`NapiUtils::MAX_LEN` 全仓零引用 | 仅记录，已复核 |
| B4 | `src/js_initialize_config.cpp` ParseProxy | 错误信息写 "http(s)://..." 但正则 `^http:\/\/.+\d{1,5}$` 实际只接受 http:// | 仅记录 |
| B5 | `src/js_initialize_file.cpp:35` | `FILE_PERMISSION = 0644` 文件内静态常量，全文件无引用（死常量） | 仅记录 |
| B6 | `src/legacy/download_task.cpp` DoDownload | `shared_ptr` 删除器按值捕获构造时的 `headers`（此刻为 nullptr），SetOption 后构建的 slist 不会再被 `curl_slist_free_all` 释放（内存泄漏） | 仅记录 |
| B7 | `include/upload/upload_common.h` + `include/upload/upload_hilog_wrapper.h` | 死声明：`Type` 枚举唯一消费者 `UploadTask::SetCallback` 全仓无定义无调用；`TaskState`、`ONE_ARG`、`TWO_ARG`、`FORMATTED` 宏全仓无引用。全仓 grep 已复核（含 services/test/interfaces，upload 头文件无树外包含者） | 仅记录，已复核 |
| B8 | `src/upload/curl_adp.cpp` SetSslOpts | `CURLOPT_SSLVERSION` 传入字符串（"CURL_SSLVERSION_TLSv1_2" 或 X-TLS-Version 头的值），该选项需要 int 枚举——传参即未定义行为，TLS 版本设置实际无效 | 仅记录 |
| B9 | `src/upload/curl_adp.cpp` UploadOneFile | `curl_multi_wait` 失败时直接 `return res`（CURLMcode），与 UploadErrorCode 数值空间冲突（CURLM 1-7 恰好撞上 UNSUPPORT_URI..TASK_REMOVED） | 仅记录 |
| B10 | `src/upload/curl_adp.cpp` SetHttpPut | int64_t `totalsize` 传给 `CURLOPT_INFILESIZE`（long 形参）；>2GB 文件在 32 位目标（RK3568）上高位丢失（应使用 CURLOPT_INFILESIZE_LARGE） | 仅记录 |
| B11 | `src/upload/async_call.cpp` 构造函数 | `argv[argc - 1]` 读取无 `argc > 0` 保护（根树版本有保护），零参调用时数组越界读 | 仅记录 |
| B12 | `src/upload/obtain_file.cpp` | GetDataAbilityFile 中 `fdopen` 失败时 fd 未 close（泄漏）；GetInternalFile 中 `ftell` 返回值未检查（-1 会被当作巨大 size） | 仅记录 |
| B13 | `include/upload/js_util.h` + `include/upload/obtain_file.h` + `src/upload/js_util.cpp` | 死声明第二批：`JSUtil::MAX_NUMBER_BYTES`/`SEPARATOR` 无引用；`SPLIT_ZERO`~`SPLIT_THREE` 宏无引用；`Convert2FileVector` 的 `version` 形参未使用（与 B2 同类）。全仓 grep 已复核零引用 | 仅记录，已复核 |
| B14 | `include/upload/upload_task.h` + `src/upload/upload_task.cpp` InitFileArray | 死声明第三批：`OnFail`/`OnComplete`/`ReportTaskFault` 全仓无定义无调用；`headerArray_`/`header_` 成员无引用；`uploadedSize_` 仅在构造函数清零；`thread_handle_` 赋值后从未使用。另 InitFileArray 出错时 `initResult = data.result`（恒为 UPLOAD_ERRORCODE_UPLOAD_FAIL）覆盖了具体错误码 ret，应取 ret | 仅记录 |
| （Wave 1 附录 A 已有条目不重复登记） | | | |
