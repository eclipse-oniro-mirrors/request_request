/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef REQUEST_TASK_NAPI_H
#define REQUEST_TASK_NAPI_H

#include "async_call.h"
#include "js_notify_data_listener.h"
#include "js_response_listener.h"
#include "request_common.h"

namespace OHOS::Request {
// JS-side Task object and the module-level task entries of @ohos.request.
//
// Each JS Task instance wraps one task id and carries the parsed Config,
// the response/notify-data listeners and the tid; all task behavior
// (scheduling, transfer, persistence) stays in the server SA — the
// entries below only marshal JS calls into IPC via RequestManager.
//
// Two families of entries:
// - Creation: JsCreate (request.agent.create, API10), JsRequestFile
//   (downloadFile/uploadFile, API9), JsUpload/JsDownload (request.upload/
//   download, API8; their API5-7 legacy signatures are peeled off inside
//   and routed to the in-process upload task / legacy request manager).
// - Operations on a tid: GetTask/Remove/Show/Touch/Search/Query, all
//   AsyncCall-based (promise or callback delivery).
// The statics manage the per-process task table taskContextMap_: SA
// subscription while any task exists, ACL grant/release for sandbox
// paths, cert staging, temp-file and reference cleanup.
class JsTask {
public:
    ~JsTask();
    /// Per-task state shared by the create/getTask flows; lives in
    /// taskContextMap_ (keyed by tid) while the task is addressable from
    /// this process. token defaults to "null" when the JS call passed
    /// none; baseContext holds the caller's ability context reference.
    struct ContextInfo : public AsyncCall::Context {
        JsTask *task = nullptr;
        napi_ref taskRef = nullptr;
        napi_ref baseContext = nullptr;
        Config config{};
        std::string tid{};
        std::string token = "null";
        bool contextIf = false;
    };
    /// request.agent.create (API10).
    static napi_value JsCreate(napi_env env, napi_callback_info info);
    /// request.upload: API5-7 callback-style calls go to the in-process
    /// UploadTaskNapiV5; options-object calls continue as API8.
    static napi_value JsUpload(napi_env env, napi_callback_info info);
    /// request.download: the legacy signature goes to
    /// Legacy::RequestManager, otherwise continues as API8.
    static napi_value JsDownload(napi_env env, napi_callback_info info);
    static napi_value JsRequest(napi_env env, napi_callback_info info);
    /// request.downloadFile/uploadFile (API9): one native entry, the
    /// action is inferred from the options object.
    static napi_value JsRequestFile(napi_env env, napi_callback_info info);

    /// request.agent.getTask(context, tid, token?): returns the live JS
    /// Task object when this process created the task (token must match),
    /// otherwise constructs one from the server's task config.
    static napi_value GetTask(napi_env env, napi_callback_info info);
    static napi_value Remove(napi_env env, napi_callback_info info);
    /// show/touch both call the server Touch and return the TaskInfo;
    /// show takes only the tid, touch additionally requires the token.
    static napi_value Show(napi_env env, napi_callback_info info);
    static napi_value Touch(napi_env env, napi_callback_info info);
    /// request.agent.search(filter?): tids matching bundle/state/action/
    /// mode and the [after, before] time window (default last day).
    static napi_value Search(napi_env env, napi_callback_info info);
    static napi_value Query(napi_env env, napi_callback_info info);

    std::string GetTid();
    void SetTid(std::string &tid);

    // Task-table management: SubscribeSA/UnsubscribeSA keep the SA-death
    // listener in sync with taskContextMap_ occupancy; ReloadListener
    // re-subscribes every live tid after the SA restarted; the
    // permission/cleanup helpers release ACLs, certs and temp files when a
    // task ends (ClearTaskTemp deletes body files only, ClearTaskThing is
    // the full release, RemoveTaskContext also drops the map entry and JS
    // references).
    static void SubscribeSA();
    static void UnsubscribeSA();
    static void ReloadListener();
    static bool SetDirsPermission(std::vector<std::string> &dirs);
    static void ClearTaskThing(const std::string &tid);
    static void ClearTaskTemp(const std::string &tid);
    static void RemoveDirsPermission(const std::vector<std::string> &dirs);
    static void RemoveTaskContext(const std::string &tid);

    // Per-task state: config_ is the parsed creation config; isGetPermission
    // tracks whether the sandbox ACLs were granted for this task;
    // register_ guards the one-per-process foreground-resume callback
    // (see AppStateCallback); taskContextMap_ (taskMutex_) holds every task
    // created in this process; the listeners receive the server's UDS
    // pushes (responseListener_ for responses, notifyDataListenerMap_ per
    // SubscribeType for events, listenerMutex_ guards the map).
    Config config_;
    bool isGetPermission;
    static bool register_;
    static std::mutex taskMutex_;
    static std::map<std::string, std::shared_ptr<ContextInfo>> taskContextMap_;

    std::mutex listenerMutex_;
    std::shared_ptr<JSResponseListener> responseListener_;
    std::map<SubscribeType, std::shared_ptr<JSNotifyDataListener>> notifyDataListenerMap_;

private:
    /// Carrier for a ContextInfo across the JS-thread cleanup callback
    /// (see DeleteContextTaskRef in the .cpp).
    struct ContextCallbackData {
        std::shared_ptr<ContextInfo> context = nullptr;
    };

    /// show/touch call state: tid + token in, TaskInfo out.
    struct TouchContext : public AsyncCall::Context {
        std::string tid;
        std::string token = "null";
        TaskInfo taskInfo;
    };

    static napi_value DefineClass(
        napi_env env, const napi_property_descriptor *desc, size_t count, napi_callback cb, napi_ref *ctor);
    static napi_value JsMain(napi_env env, napi_callback_info info, Version version, int32_t seq);
    static napi_value Create(napi_env env, napi_callback_info info);
    static napi_value GetCtor(napi_env env, Version version);
    static napi_value GetCtorV8(napi_env env);
    static napi_value GetCtorV9(napi_env env);
    static napi_value GetCtorV10(napi_env env);
    static napi_value RequestFile(napi_env env, napi_callback_info info);
    static napi_value RequestFileV8(napi_env env, napi_callback_info info);
    static int32_t CreateExec(const std::shared_ptr<ContextInfo> &context, int32_t seq);
    static napi_status CreateInput(
        std::shared_ptr<ContextInfo> context, const int32_t seq, size_t argc, napi_value *argv);
    static napi_value GetTaskCtor(napi_env env);
    static napi_value GetTaskCreate(napi_env env, napi_callback_info info);
    static void GetTaskExecution(std::shared_ptr<ContextInfo> context);
    static napi_status GetTaskOutput(std::shared_ptr<ContextInfo> context, napi_value *result, int32_t seq);
    static napi_status CtorJsTask(std::shared_ptr<ContextInfo> context, napi_value *result);
    static napi_status CheckTaskInMap(
        std::shared_ptr<ContextInfo> context, std::shared_ptr<ContextInfo> mapContext, napi_value *result);
    static ExceptionError ParseGetTask(
        napi_env env, size_t argc, napi_value *argv, std::shared_ptr<ContextInfo> context);
    static ExceptionError ParseTid(napi_env env, size_t argc, napi_value *argv, std::string &tid);
    static napi_value TouchInner(napi_env env, napi_callback_info info, AsyncCall::Context::InputAction action,
        std::shared_ptr<TouchContext> context, int32_t req);
    static ExceptionError ParseSearch(napi_env env, size_t argc, napi_value *argv, Filter &filter);
    static std::string ParseBundle(napi_env env, napi_value value);
    static State ParseState(napi_env env, napi_value value);
    static Action ParseAction(napi_env env, napi_value value);
    static Mode ParseMode(napi_env env, napi_value value);
    static ExceptionError ParseTouch(
        napi_env env, size_t argc, napi_value *argv, std::shared_ptr<TouchContext> context);
    static int64_t ParseBefore(napi_env env, napi_value value);
    static int64_t ParseAfter(napi_env env, napi_value value, int64_t before);
    static void DeleteContextTaskRef(std::shared_ptr<ContextInfo> context);
    static bool DeleteTaskRef(ContextCallbackData *data);
    static bool DeleteBaseContextRef(ContextCallbackData *data);
    static void RegisterForegroundResume();
    static void AddTaskWhenCreate(std::shared_ptr<ContextInfo> context);
    static void AddRemoveListener(const std::shared_ptr<ContextInfo> &context);
    static bool ParseTouchCheck(const napi_env env, const size_t argc, const napi_value *argv,
        const std::shared_ptr<TouchContext> context, ExceptionError &err);
    static int32_t AuthorizePath(const Config &config);
    bool Equals(napi_env env, napi_value value, napi_ref copy);

    // JS class constructor caches: one napi_ref per (thread, class) — each
    // version's Task class is defined exactly once per thread, on first use,
    // under the matching mutex.
    static std::mutex createMutex_;
    static thread_local napi_ref requestCtor;
    static std::mutex requestMutex_;
    static thread_local napi_ref requestFileCtor;
    static std::mutex requestFileMutex_;
    static thread_local napi_ref createCtor;
    static std::mutex getTaskCreateMutex_;
    static thread_local napi_ref getTaskCreateCtor;
    // Task id assigned by the server at create time.
    std::string tid_;
};
} // namespace OHOS::Request

#endif // REQUEST_TASK_NAPI_H
