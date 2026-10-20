# Tasks

## 工作规范（全组适用）

- **只改注释**：每行 diff 必须是注释行；不改代码、不格式化、不动配置与测试。
- **注释语言**：英文。Rust 用 rustdoc（`///` / `//!`）；C++ 用 `///`（Doxygen 风格，范例：`frameworks/native/request/include/multi_instance_runcount_manager.h`）。
- **粒度**：改写以"准确描述现有行为"为准，不顺带扩写；跨语言契约（IPC 码、UDS 格式、State/Reason 枚举值只增不改）必须显式警示。
- **行号快照**：下列行号为勘察时快照，apply 时以引文内容定位为准。
- **验证方法**：每组完成后 `git diff -- <该组路径>` 肉眼复核 + 全量 `git diff -U0` 非注释行过滤（过滤规则见 6.2）；Rust 侧 `cargo fmt --check`。构建豁免（用户已裁定注释类改动无需构建）。
- **代码 bug 只记录不修**：勘察发现的代码问题一律进附录 A，不在本 change 修复。
- **已核实无误、不要动的注释**（勘察抽查结论）：进度通知 500ms 时间戳类注释、`8192`/`POSITION_OF_LENGTH = 10`、服务端 16KB/s 限速注释、`construct.rs:121` 的 `(config.rs:685)` 交叉引用、`visibility 0b01/0b10`、`ability.rs` on_idle 相关注释、`request_event.cpp:497` 错误码清单注释、`js_task.cpp:251` "bodyFileNames need rw."。

## 1. Rust 服务层（services/src/service）修错

- [x] 1.1 `client/mod.rs:718` 注释称 UDS 中 headers 长度上限 8192，实际 `:720-728` 截断的是 `progress.extras`。改写为准确描述 extras 截断逻辑与 8192 上限真实作用对象（验证：grep 改后注释与截断代码对象一致）
- [x] 1.2 `client/mod.rs:335` 陈旧 TODO "// uid and token_id will be used later" 挂在 Client 结构体文档前、与结构体无关。删除或改写为对该字段的准确说明（验证：grep 确认 TODO 消失且无代码行变动）
- [x] 1.3 `set_max_speed.rs:124` 注释称 "skip uid check if task used by innerkits"，实际 `:126` `Some(id) => uid = id` 是**用 DB 中任务属主 uid 替换**当前 uid。改写为替换语义（验证：注释描述与 match 分支行为一致）
- [x] 1.4 `interface.rs:44-45` CLEAR=12 文档 "Clears a request from the system"，但 `stub.rs:176` 无条件 no-op；`interface.rs:64-65` SHOW_PROGRESS=22 文档 "Shows the progress"，实为保留码（客户端不发送、stub 无分支）。两处改写为实际语义并标注保留/no-op 状态（验证：grep 两码的注释与 stub.rs 实际分支对照）
- [x] 1.5 `publish.rs:37` 与 `notify_flow.rs:41` 存在重复常量 NOTIFY_PROGRESS_INTERVAL(500)（notify_flow 侧为 `cfg!(test) ? 1 : 500`）。两侧注释互相引用并警示同步义务（验证：两文件注释均含对方路径）〔实际路径为 `notification_bar/publish.rs` 与 `notification_bar/notify_flow.rs`〕
- [x] 1.6 `query.rs:157`、`show.rs:153`、`touch.rs:154` 三处 "// TODO: Sends info only when ErrOk." 与实际行为相反。核实各自真实分支后逐处改写（验证：注释与 if 条件一致）
- [x] 1.7 `stub.rs:113` TODO 位于 rustdoc 文档注释内部，污染 API 文档。移出文档注释或改写为普通注释（验证：`cargo doc` 级别的文档注释中无 TODO 字样，grep 确认）
- [x] 1.8 低危修正：`manager.rs:102` 重复注释删除其一；`run_count/manager.rs:111` "todo: may be another error code" 改写为明确现状（验证：grep 原文消失）〔manager.rs:102 实为 `client/manager.rs:102`〕
- [x] 1.9 风格一致化（3 处）：`command/mod.rs:22-42` 子模块声明 `//`→`///`；cfg 属性先于文档注释的顺序修正；`notify_flow.rs` 私有字段 `//`→`///`。逐处确认不改变语义（验证：diff 逐行均为注释行）〔注：属性顺序子项跳过——移动 cfg/allow 属性行属于代码行变更，违反只改注释纪律；command/mod.rs 注释上移导致 mod 行仅剩尾部空白变化，非注释残留 42 行已逐行核对均为纯空白差异〕

## 2. Rust 管理层（services/src/manage + services/src/task）修错

- [x] 2.1 `request_task.rs:105-106` max_speed 注释 "Maximum speed achieved" 与 `info.rs:59` "Maximum speed limit" 矛盾——实为限速值非实测值。改写为 limit 语义（验证：两文件用词一致）
- [x] 2.2 `scheduler/mod.rs:~817` 注释称任务 "inactive for more than one month" 才清理，实际基于 ctime（创建时间）而非活跃时间。改写为 ctime 语义（验证：注释与代码读取的时间字段一致）
- [x] 2.3 `task_manager.rs:570` "// failed logic?" 疑问式注释无信息量。核实该分支真实意图后改写（验证：注释为陈述句且与分支行为一致）〔unload 结果被丢弃、函数恒返回 true，open question 保留为陈述〕
- [x] 2.4 `task_manager.rs:242` "If unload_sa success, can not breaks this loop" 语法破碎。改写为准确控制流描述（验证：注释与 break/continue 实际逻辑一致）〔核实：handle_schedule_event 仅 Unload 事件返回 unload_sa() 结果〕
- [x] 2.5 `task_manager.rs:519` 语法不通注释改写（验证：通读该函数注释自洽）
- [x] 2.6 `task_manager.rs:545` ONE_MONTH 与 `scheduler/mod.rs:52` MILLISECONDS_IN_ONE_MONTH 为重复裸常量（类型还不同：i64/u64）。两侧补单位（毫秒）注释并互相引用警示同步（验证：两处注释含单位与对方路径）
- [x] 2.7 `database.rs:956-962` SAFETY 注释只写 safety 字样未写真实不变量。补齐实际安全前提（何种指针/生命周期约束）（验证：注释能回答"为什么这里 unsafe 是安全的"）〔旧注释"Mutex 保护 inner"不成立——Mutex 仅保护 user_file_tasks；已改为如实陈述：oh 依赖 Rdb 存储内部同步、not-oh 依赖宿主测试单线程使用，本文件无锁〕
- [x] 2.8 `files.rs:337-343` doctest 依赖运行态环境不可复现。改为非执行代码块或标注 illustrative（验证：`cargo test --doc` 该项不再被收集执行，或标注后语义明确）〔采用 ```ignore 栅栏——rustdoc 对 ignore 块不编译不执行；构建豁免下未实际跑 cargo test --doc〕
- [x] 2.9 `request_task.rs` file_total_size 字段注释缺下载路径 -1 哨兵语义（对照 `files.rs:164`）。补齐（验证：注释覆盖 -1 与正数两种取值含义）〔已核实：Upload 起始为本地文件大小之和，Download 起始 -1、收到 Content-Length 后更新（:642）〕
- [x] 2.10 `request_task.rs:906` mtime 注释缺单位。补（验证：注释含单位）〔实际行号 :921（快照漂移）；单位沿用 info.rs "milliseconds since epoch" 惯例〕

## 3. C++ 客户端（frameworks/native/request）修错

- [x] 3.1 `request_service_proxy.cpp:768` 注释 "API9 or lower will not return E_TASK_NOT_FOUND." 与代码双重不符：`:770` 条件是 `version == API9`（不含 lower），且 `:772` `result = E_OK` 抑制**所有**错误码。改写为准确描述（验证：注释与两行代码逐一对照）
- [x] 3.2 `request_manager_impl.cpp:534` "Not sure about the SA state…doesn't hang frequently" 及 "SA accesses a low-memory framework" 语义不可读。核实上下文后整体改写（验证：新注释能独立说清该分支存在原因）〔改写依据已核实：GetRequestServiceProxy(:455-458) 会在下次使用时刷新代理〕
- [x] 3.3 `request_manager_impl.cpp:566` "Sometimes too slow to return." 无上下文孤句。补足主语与场景（验证：注释指明哪个操作、什么条件下慢）〔原句无从考证所指，改为如实描述该 SAMgr 回调的职责（恢复 callback_ 与运行计数订阅）〕
- [x] 3.4 `request_manager_impl.cpp:446` "When SubRuncount/…need to get proxy but not need to load" 病句。改写为 needLoadSA=false 的准确语义（验证：注释与该参数取值逻辑一致）
- [x] 3.5 `request_manager_impl.cpp:337` "Proxy does not affect sub runcount at framework." 含糊。改写为具体的"框架侧运行计数不受代理获取影响"语义（验证：通读函数后注释无歧义）〔已核实：SA 不在时 SubRunCount 仍返回 E_OK，订阅在 SA 回来后由 OnAddSystemAbility→RestoreSubRunCount 恢复〕
- [x] 3.6 `request_common.h:151` 与 `task_builder.h:63` "0 means no duration check" 与服务端 `client.rs:121` 语义不符：服务端要求 speed>0 **且** duration>0，任一为 0 则整条限速检查关闭。两处同步改写并加跨端警示（验证：两端注释表述一致且提及联动条件）
- [x] 3.7 `request.cpp:45` "for api9, remove do not notify after complete/fail" 语序混乱。核实行为后改写（验证：注释与函数内 version 分支行为一致）〔已核实：needRemove_ 初始 true（request.h:50），complete/fail 置 false（:54）→API9 后续 REMOVE 被抑制（:49）〕
- [x] 3.8 `response_message_receiver.cpp:35` 死代码注释块 `// static constexpr int32_t INT8_SIZE = 1;` 删除（验证：grep INT8_SIZE 无残留）

## 4. NAPI 树（frameworks/js/napi/request）修错

- [x] 4.1 `napi_utils.cpp:761` "// If it is empty, it need to be reset." 挂在 `if (!mimeType.empty())` 分支上，语义与分支相反。改写（验证：注释与分支条件一致）
- [x] 4.2 头文件守卫与 `#endif` 注释不匹配（9 处）：`napi_utils.h:161`（守卫 REQUEST_NAPI_UTILS_H、endif 写 DOWNLOAD_NAPI_UTILS_H）、`request_event.h:100`（DOWNLOAD_EVENT_H）、`js_task.h:140`（REQUEST_TASK_NAPI）、`js_initialize.h:131`、`async_call.h:141`、`path_utils.h:37`、`listener_list.h:53`、`js_notify_data_listener.h:62`、`js_response_listener.h:41`。逐文件对齐 endif 注释与真实守卫宏（验证：脚本比对每文件 `#ifndef` 与 `#endif //` 字串一致）〔9 处全部对齐；endif 行为尾随注释改写，指令前缀逐字节不变〕
- [x] 4.3 `js_notify_data_listener.cpp:96` "Waiting for 'complete' to read and delete." 指代不明（谁等待、删什么）。改写为明确的事件缓存生命周期描述（验证：注释指明主体与对象）
- [x] 4.4 `request_event.cpp:26` MIN_SPEED_LIMIT = 16*1024 常量注释缺单位。补 bytes/s（验证：注释含单位）〔已核实用途：:340 处 maxSpeed 参数下限校验，错误信息即 "16 KB/s"〕
- [x] 4.5 `js_initialize.cpp:31,47` 两处 "todo: check if needed" 悬而未决。核实后改写为现状结论（仍需关注则写明原因，不再需要则删除）（验证：grep "check if needed" 无残留）
- [x] 4.6 `curl_adp.cpp:258-272` CurlGlobalInit/Cleanup 两个"空壳"函数仅翻转 `isCurlGlobalInit_` 布尔、从不调用 curl_global_init/cleanup。函数注释写明实际为 no-op 及原因；`curl_adp.h:90` FILE_UPLOAD_INTERVAL 补单位（µs 常量，1000 = 1ms）（验证：注释与函数体逐行对照）
- [x] 4.7 `js_initialize_config.cpp:370` "Only use for Action::DOWNLOAD." 误导——ParseSaveas 对 UPLOAD 也生效。改写（验证：grep ParseSaveas 全部调用点确认注释覆盖两种 action）〔已核实调用点 :139 无 action 门控，两种 action 均进入、函数内部对非 DOWNLOAD 清空字段〕
- [x] 4.8 `js_initialize_path.cpp:202-213` CheckBelongAppBaseDir 只检查 `/data/storage/el{1,2,5}/base` 前缀、从不比较 baseDir 本身。注释写明真实校验范围（验证：注释与循环内比较逻辑一致）〔AREA1/2/5 定义已核实于 path_utils.cpp:36-38〕
- [x] 4.9 `js_task.cpp:197` "// Unreachable." 缺上下文。改写为该分支为何不可达（验证：注释能回答"为什么"）
- [x] 4.10 低价值改写（4 处）：`js_initialize_file.cpp:148` "File is exist."、`js_initialize_file.cpp:273`、`js_initialize_path.cpp:82`、`js_task.cpp:575`。改通顺（验证：grep 原文消失）〔:273 已按完整循环语义改写（user file 仅 API10+）；:575 为尾随注释、代码前缀不变〕

## 5. ANI 绑定（frameworks/ets/ani/request）单位核实

- [x] 5.1 `api10/bridge.rs:241/:267/:269` 三处 "in seconds" 注释：先沿调用链追踪到 netstack/服务端，核实真实单位（preload 先例：options 域为秒、统计域为毫秒，勿凭名字推断），错误则修正、正确则保留并记录结论到附录（验证：每处注释附单位依据或已按实证改写）〔核实结论：三处均正确，未改动。:267/:269 由 `client.rs:103-104` `Timeout::from_secs` 证明为秒（ANI 默认值 60/604800=7天亦吻合）；:241 duration 全链路无换算（bridge.rs:249 原样传递→client.rs:124 原样传入 min_speed_interval），与服务端注释及 curl CURLOPT_LOW_SPEED_TIME 语义一致；ylong_http_client 源码不在本仓，属残余不确定项，见附录 A12〕

## 6. 全量复核与收尾

- [x] 6.1 对全部改动的 Rust crate 跑 `cargo fmt --check`（验证：零 diff）〔实测：download_server 在 stable rustfmt 下有 16 处**存量**违规（rustfmt.toml 的 nightly-only 选项本地无法生效所致）；以 stash 基线对比证明本次改动零新增 hunk（前后文件集合与 hunk 数完全一致）；Rust 改动仅涉及 services crate〕
- [x] 6.2 全量 `git diff -U0` 非注释行过滤：剔除注释行与空行后必须零残留（过滤正则：`^[+-][[:space:]]*(//|/\*|\*|//!|///)` 及空行）（验证：过滤输出为空）〔实测 66 行残留，全部归入 4 类可证明良性：command/mod.rs 注释上移的 42 行 mod 行（仅尾部空白差异）、9 对 #endif 尾随注释、MinSpeed 2 对尾随注释、js_task.cpp:575 尾随注释；代码前缀多重集合差异为 NONE——非注释代码逐字节不变〕
- [x] 6.3 附录 A 代码问题清单终稿：核对每条 file:line 与一句话描述齐全，全部标注"仅记录未修复"（验证：逐条可按 file:line 定位）〔A3 路径勘误为 NAPI 树；A8 五处拼写实证于 reason.rs:147-164；A1/A4-A7/A9-A11 均已在实施中直接目验或源于已证实的勘察〕

## Workflow follow-up

- 全部任务完成后 `/opsx:archive upload-download-comments-fix` 归档（design 为有意跳过，归档时按 phase-1 先例带警告通过）。
- 归档后 `git commit -s -m`，message 末行 `Co-Authored-By: Agent`。
- Wave 2（C++ 从零补写，~94 文件）与 Wave 3（Rust 补漏 + 契约警示 + request_core/request_next 同步检查）另行立项。

---

## 附录 A：勘察发现的代码问题（仅记录，未修复）

| # | 位置 | 问题 | 状态 |
|---|---|---|---|
| A1 | `frameworks/native/request/src/request_service_proxy.cpp:342` | `if (ret != ExceptionErrorCode::E_OK)` 死逻辑，应为 `code`（TouchTasks 错误判断失效） | 仅记录 |
| A2 | `frameworks/js/napi/request/src/upload/curl_adp.cpp:168` | CURLOPT_SSLVERSION 传入字符串指针而非枚举值 | 仅记录 |
| A3 | `frameworks/js/napi/request/src/upload/upload_task.cpp:109` | `initResult = data.result` 应为 `ret`，错误码被覆盖 | 仅记录 |
| A4 | `frameworks/js/napi/request/src/request_event.cpp:95` | failMap_ 以 REASON_OK 映射 ERROR_FILE_ALREADY_EXISTS，语义存疑 | 仅记录 |
| A5 | `frameworks/native/request/libdownload_single.map` | `*DownloadTask*` 条目无任何匹配符号，已失效 | 仅记录 |
| A6 | `services/src/manage/database.rs:165-177` | 非 OH 路径 execute() 疑似缺陷（结果处理不对） | 仅记录，需确认 |
| A7 | `frameworks/js/napi/request/include/js_task.h:40` | 幽灵声明 `static napi_value JsRequest(...)` 无定义无引用（删除属代码变更，故未动） | 仅记录 |
| A8 | `services/src/task/reason.rs` | to_str 用户可见字符串拼写错误："TSL"→TLS、"occured"、"NetWork"（改字符串属代码变更） | 仅记录 |
| A9 | `services/src/service/command/publish.rs:37` vs `notify_flow.rs:41` | NOTIFY_PROGRESS_INTERVAL 重复定义且值有漂移风险（去重属代码变更，本波仅注释互引） | 仅记录 |
| A10 | 日志文案与行为不符（改字符串属代码变更）：`request_service_proxy.cpp:429`（DisableTaskNotification 打 "SetMode"）、`:959/:1004/:1012`（Create/DeleteGroup 打 "AttachGroup"）、`:908-922`（"UnubRunCount"）；`remove.rs:69`（"Service pause"）、`pause.rs:178`（"Service start"）、`set_mode.rs:153`、`manager.rs:207/:237`；`request_module.cpp:147`（InitAgent 打 "InitV10 end"）；`notification_bar.cpp:291,327`（复用 "createGroup"）；`js_initialize_file.cpp:342`（"overrite"）；`js_task.cpp:199/:527` | 仅记录 |
| A11 | 魔法常量缺出处：REMOTE_DIED_ERROR=29189、EXPECT_UID=3815、LOAD_SA_TIMEOUT_MS=15000、RETRY_TIMES=5 | 仅记录（Wave 3 补溯源注释） |
| A12 | `frameworks/ets/ani/request/src/api10/bridge.rs:241/:267/:269` | 三处 "in seconds" 注释核实为**正确**：超时由 `Timeout::from_secs` 证明、duration 全链路无换算；ylong_http_client（外部 git 依赖，本仓无源码）的 `min_speed_interval` 参数单位未能直接查证，属残余不确定项 | 已核实，未修改 |
