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

#include "js_task.h"

#include <securec.h>
#include <sys/stat.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <new>

#include "app_state_callback.h"
#include "async_call.h"
#include "constant.h"
#include "js_initialize.h"
#include "js_native_api_types.h"
#include "legacy/request_manager.h"
#include "log.h"
#include "napi_base_context.h"
#include "napi_utils.h"
#include "path_utils.h"
#include "request_common.h"
#include "request_event.h"
#include "request_manager.h"
#include "storage_acl.h"
#include "sys_event.h"
#include "upload/upload_task_napiV5.h"

using namespace OHOS::StorageDaemon;
namespace fs = std::filesystem;
namespace OHOS::Request {
// Width of the default time window used by Search when the filter carries
// no time bounds (search then looks back one day).
constexpr int64_t MILLISECONDS_IN_ONE_DAY = 24 * 60 * 60 * 1000;
std::mutex JsTask::createMutex_;
thread_local napi_ref JsTask::createCtor = nullptr;
std::mutex JsTask::requestMutex_;
thread_local napi_ref JsTask::requestCtor = nullptr;
std::mutex JsTask::requestFileMutex_;
thread_local napi_ref JsTask::requestFileCtor = nullptr;
std::mutex JsTask::getTaskCreateMutex_;
thread_local napi_ref JsTask::getTaskCreateCtor = nullptr;
std::mutex JsTask::taskMutex_;
std::map<std::string, std::shared_ptr<JsTask::ContextInfo>> JsTask::taskContextMap_;
bool JsTask::register_ = false;

// Instance method table of the API10 Task class (request.agent): event
// subscription plus lifecycle control, all dispatched through RequestEvent.
napi_property_descriptor clzDes[] = {
    DECLARE_NAPI_FUNCTION(FUNCTION_ON, RequestEvent::On),
    DECLARE_NAPI_FUNCTION(FUNCTION_OFF, RequestEvent::Off),
    DECLARE_NAPI_FUNCTION(FUNCTION_START, RequestEvent::Start),
    DECLARE_NAPI_FUNCTION(FUNCTION_PAUSE, RequestEvent::Pause),
    DECLARE_NAPI_FUNCTION(FUNCTION_RESUME, RequestEvent::Resume),
    DECLARE_NAPI_FUNCTION(FUNCTION_STOP, RequestEvent::Stop),
    DECLARE_NAPI_FUNCTION(FUNCTION_SET_MAX_SPEED, RequestEvent::SetMaxSpeed),
};

// Instance method table of the API8/9 class (request.download/upload,
// downloadFile/uploadFile): same RequestEvent targets as the API10 class,
// plus suspend/delete/restore aliases and the getTaskInfo/getTaskMimeType/
// query/queryMimeType/remove legacy names.
napi_property_descriptor clzDesV9[] = {
    DECLARE_NAPI_FUNCTION(FUNCTION_ON, RequestEvent::On),
    DECLARE_NAPI_FUNCTION(FUNCTION_OFF, RequestEvent::Off),
    DECLARE_NAPI_FUNCTION(FUNCTION_SUSPEND, RequestEvent::Pause),
    DECLARE_NAPI_FUNCTION(FUNCTION_GET_TASK_INFO, RequestEvent::Query),
    DECLARE_NAPI_FUNCTION(FUNCTION_GET_TASK_MIME_TYPE, RequestEvent::QueryMimeType),
    DECLARE_NAPI_FUNCTION(FUNCTION_DELETE, RequestEvent::Remove),
    DECLARE_NAPI_FUNCTION(FUNCTION_RESTORE, RequestEvent::Resume),
    DECLARE_NAPI_FUNCTION(FUNCTION_PAUSE, RequestEvent::Pause),
    DECLARE_NAPI_FUNCTION(FUNCTION_QUERY, RequestEvent::Query),
    DECLARE_NAPI_FUNCTION(FUNCTION_QUERY_MIME_TYPE, RequestEvent::QueryMimeType),
    DECLARE_NAPI_FUNCTION(FUNCTION_REMOVE, RequestEvent::Remove),
    DECLARE_NAPI_FUNCTION(FUNCTION_RESUME, RequestEvent::Resume),
};

JsTask::~JsTask()
{
    REQUEST_HILOGD("~JsTask()");
}
// request.upload: API5-7 callback-style signatures are peeled off here and
// served entirely in-process by UploadTaskNapiV5 (the curl path); SetEnv
// detaches the proxy's env handle, then options-object calls continue
// through the server-backed flow as API8.
napi_value JsTask::JsUpload(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("JsUpload seq %{public}d", seq);
    std::shared_ptr<Upload::UploadTaskNapiV5> proxy = std::make_shared<Upload::UploadTaskNapiV5>(env);
    if (proxy->ParseCallback(env, info)) {
        return proxy->JsUpload(env, info);
    }
    proxy->SetEnv(nullptr);

    return JsMain(env, info, Version::API8, seq);
}

// request.download: the legacy signature is routed to
// Legacy::RequestManager; options-object calls go through the
// server-backed flow as API8.
napi_value JsTask::JsDownload(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("JsDownload seq %{public}d", seq);
    if (Legacy::RequestManager::IsLegacy(env, info)) {
        return Legacy::RequestManager::Download(env, info);
    }
    return JsMain(env, info, Version::API8, seq);
}

// request.downloadFile/uploadFile (API9): a single entry; the action is
// inferred from the options object (see NapiUtils::GetRequestAction).
napi_value JsTask::JsRequestFile(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("Js seq %{public}d", seq);
    return JsMain(env, info, Version::API9, seq);
}

// request.agent.create (API10).
napi_value JsTask::JsCreate(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("create seq %{public}d", seq);
    return JsMain(env, info, Version::API10, seq);
}

// Input stage shared by all versions: instantiates the version's Task JS
// class with the caller's argv (the native constructor parses the config),
// unwraps the JsTask pointer and pins the JS object with a persistent
// reference; API10 additionally exposes the raw config object as the
// instance's "config" property.
napi_status JsTask::CreateInput(std::shared_ptr<ContextInfo> context, const int32_t seq, size_t argc, napi_value *argv)
{
    napi_value ctor = GetCtor(context->env_, context->version_);
    napi_value jsTask = nullptr;
    napi_status status = napi_new_instance(context->env_, ctor, argc, argv, &jsTask);
    if (jsTask == nullptr || status != napi_ok) {
        REQUEST_HILOGE("End task create input, seq: %{public}d, failed:%{public}d", seq, status);
        return napi_generic_failure;
    }
    status = napi_unwrap(context->env_, jsTask, reinterpret_cast<void **>(&context->task));
    if (status != napi_ok) {
        return status;
    }
    status = napi_create_reference(context->env_, jsTask, NapiUtils::ONE_REF, &(context->taskRef));
    if (status != napi_ok) {
        return status;
    }
    if (context->version_ == Version::API10) {
        status = napi_set_named_property(context->env_, jsTask, "config", argv[1]);
        if (status != napi_ok) {
            return status;
        }
    }
    return napi_ok;
}

// Shared creation flow for API8-10: exec grants the sandbox ACLs and
// creates the task on the server (CreateExec); output resolves with the
// Task object, records it in taskContextMap_ and (API10) exposes the tid.
// Error-shape compatibility: API9 upload failures reject without an error
// code (withErrCode_ = false), while API8 permission failures include one.
napi_value JsTask::JsMain(napi_env env, napi_callback_info info, Version version, int32_t seq)
{
    auto context = std::make_shared<ContextInfo>();
    context->withErrCode_ = version != Version::API8;
    context->version_ = version;
    auto input = [context, seq](size_t argc, napi_value *argv, napi_value self) -> napi_status {
        return JsTask::CreateInput(context, seq, argc, argv);
    };
    auto exec = [context, seq]() {
        Config config = context->task->config_;
        context->innerCode_ = CreateExec(context, seq);
        if (context->innerCode_ == E_SERVICE_ERROR && config.version == Version::API9
            && config.action == Action::UPLOAD) {
            context->withErrCode_ = false;
        }
        if (config.version == Version::API8 && context->innerCode_ == E_PERMISSION) {
            context->withErrCode_ = true;
        }
    };
    auto output = [context, seq](napi_value *result) -> napi_status {
        if (result == nullptr || context->innerCode_ != E_OK) {
            REQUEST_HILOGE(
                "End task create in AsyncCall output, seq: %{public}d, failed:%{public}d", seq, context->innerCode_);
            return napi_generic_failure;
        }
        napi_status status = napi_get_reference_value(context->env_, context->taskRef, result);
        context->task->SetTid(context->tid);
        JsTask::AddTaskWhenCreate(context);
        if (context->version_ == Version::API10) {
            NapiUtils::SetStringPropertyUtf8(context->env_, *result, "tid", context->tid);
        }
        REQUEST_HILOGI("End create seq %{public}d, tid %{public}s", seq, context->tid.c_str());
        return status;
    };
    context->SetInput(input).SetOutput(output).SetExec(exec);
    AsyncCall asyncCall(env, info, context);
    asyncCall.SetQosLevel(napi_qos_utility);
    return asyncCall.Call(context, "create");
}

// Only used in create.
void JsTask::AddTaskWhenCreate(std::shared_ptr<ContextInfo> context)
{
    std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
    auto [it, inserted] = JsTask::taskContextMap_.try_emplace(context->tid, context);
    // The task id is freshly generated at creation, so it cannot already
    // exist in the map; this branch is not expected to run.
    if (!inserted) {
        REQUEST_HILOGE("CAddContext Exist %{public}s", context->tid.c_str());
    }
    if (!JsTask::taskContextMap_.empty()) {
        JsTask::SubscribeSA();
    }
}

// Worker-stage creation: a FOREGROUND task installs the SA-restart resume
// callback (see AppStateCallback); AuthorizePath grants the SA the ACLs it
// needs for the task files; RequestManager::Create mints the tid on the
// server. On success a REMOVE listener is registered so this side can
// react when the server removes the task.
int32_t JsTask::CreateExec(const std::shared_ptr<ContextInfo> &context, int32_t seq)
{
    REQUEST_HILOGD("JsTask CreateExec: Action %{public}d, Mode %{public}d, seq: %{public}d",
        context->task->config_.action, context->task->config_.mode, seq);

    if (context->task->config_.mode == Mode::FOREGROUND) {
        RegisterForegroundResume();
    }
    // Authorize the path
    int32_t err = JsTask::AuthorizePath(context->task->config_);
    if (err != E_OK) {
        return err;
    }

    int32_t ret = RequestManager::GetInstance()->Create(context->task->config_, seq, context->tid);
    if (ret != E_OK) {
        REQUEST_HILOGE("End create task in JsTask CreateExec, seq: %{public}d, failed: %{public}d", seq, ret);
        return ret;
    }
    JsTask::AddRemoveListener(context);
    return ret;
}

// Grants the download_server SA access to the task's sandbox files:
// download authorizes its single saveas target (user files are already
// accessible to it); upload authorizes every non-user source file (read)
// and each bodyFileName with read-write.
int32_t JsTask::AuthorizePath(const Config &config)
{
    if (config.action == Action::DOWNLOAD) {
        FileSpec fileSpec = config.files[0];
        if (fileSpec.isUserFile) {
            return E_OK;
        }
        if (!PathUtils::AddPathsToMap(fileSpec.uri, config.action)) {
            REQUEST_HILOGE("Add Path acl failed, %{public}s", PathUtils::ShieldPath(fileSpec.uri).c_str());
            return E_FILE_IO;
        }
    } else {
        for (auto &fileSpec : config.files) {
            if (fileSpec.isUserFile) {
                continue;
            }
            if (!PathUtils::AddPathsToMap(fileSpec.uri, config.action)) {
                return E_FILE_IO;
            }
        }

        for (auto &path : config.bodyFileNames) {
            // bodyFileNames need rw.
            if (!PathUtils::AddPathsToMap(path, Action::DOWNLOAD)) {
                return E_FILE_IO;
            }
        }
    }
    return E_OK;
}

// Registers the task's REMOVE listener with the server so this side can
// react when the server removes the task (see RemoveTaskContext).
void JsTask::AddRemoveListener(const std::shared_ptr<ContextInfo> &context)
{
    std::string tid = context->tid;
    context->task->listenerMutex_.lock();
    auto listener = std::make_shared<JSNotifyDataListener>(context->env_, tid, SubscribeType::REMOVE);
    context->task->notifyDataListenerMap_[SubscribeType::REMOVE] = listener;
    context->task->listenerMutex_.unlock();
    RequestManager::GetInstance()->AddListener(tid, SubscribeType::REMOVE, listener);
}

napi_value JsTask::GetCtor(napi_env env, Version version)
{
    switch (version) {
        case Version::API8:
            return GetCtorV8(env);
        case Version::API9:
            return GetCtorV9(env);
        case Version::API10:
            return GetCtorV10(env);
        default:
            break;
    }
    return nullptr;
}

// Returns this thread's cached API10 Task class; defined (once per thread)
// with the clzDes method table and the API10 native constructor.
napi_value JsTask::GetCtorV10(napi_env env)
{
    REQUEST_HILOGD("GetCtorV10 in");
    std::lock_guard<std::mutex> lock(createMutex_);
    napi_value cons;
    if (createCtor != nullptr) {
        REQUEST_NAPI_CALL(env, napi_get_reference_value(env, createCtor, &cons), "napi_get_reference_value failed");
        return cons;
    }
    size_t count = sizeof(clzDes) / sizeof(napi_property_descriptor);
    return DefineClass(env, clzDes, count, Create, &createCtor);
}

// Returns this thread's cached API9 Task class (clzDesV9 methods, API9
// native constructor).
napi_value JsTask::GetCtorV9(napi_env env)
{
    REQUEST_HILOGD("GetCtorV9 in");
    std::lock_guard<std::mutex> lock(requestFileMutex_);
    napi_value cons;
    if (requestFileCtor != nullptr) {
        REQUEST_NAPI_CALL(
            env, napi_get_reference_value(env, requestFileCtor, &cons), "napi_get_reference_value failed");
        return cons;
    }
    size_t count = sizeof(clzDesV9) / sizeof(napi_property_descriptor);
    return DefineClass(env, clzDesV9, count, RequestFile, &requestFileCtor);
}

// Returns this thread's cached API8 Task class — the same clzDesV9 JS
// surface as V9, but with the API8 native constructor.
napi_value JsTask::GetCtorV8(napi_env env)
{
    REQUEST_HILOGD("GetCtorV8 in");
    std::lock_guard<std::mutex> lock(requestMutex_);
    napi_value cons;
    if (requestCtor != nullptr) {
        REQUEST_NAPI_CALL(env, napi_get_reference_value(env, requestCtor, &cons), "napi_get_reference_value failed");
        return cons;
    }
    size_t count = sizeof(clzDesV9) / sizeof(napi_property_descriptor);
    return DefineClass(env, clzDesV9, count, RequestFileV8, &requestCtor);
}

// Defines the JS class "Request" with the given method table and native
// constructor, caching the constructor in *ctor.
napi_value JsTask::DefineClass(
    napi_env env, const napi_property_descriptor *desc, size_t count, napi_callback cb, napi_ref *ctor)
{
    napi_value cons = nullptr;
    napi_status status = napi_define_class(env, "Request", NAPI_AUTO_LENGTH, cb, nullptr, count, desc, &cons);
    if (status != napi_ok) {
        REQUEST_HILOGE("napi_define_class failed");
        return nullptr;
    }
    status = napi_create_reference(env, cons, 1, ctor);
    if (status != napi_ok) {
        REQUEST_HILOGE("napi_create_reference failed");
        return nullptr;
    }
    return cons;
}

// Native constructors of the three JS Task classes: invoked by
// napi_new_instance during the create input stage, they parse argv into
// the JsTask's config_ via JsInitialize::Initialize.
napi_value JsTask::Create(napi_env env, napi_callback_info info)
{
    REQUEST_HILOGD("Create API10");
    return JsInitialize::Initialize(env, info, Version::API10);
}

napi_value JsTask::RequestFile(napi_env env, napi_callback_info info)
{
    REQUEST_HILOGD("RequestFile API9");
    return JsInitialize::Initialize(env, info, Version::API9);
}

napi_value JsTask::RequestFileV8(napi_env env, napi_callback_info info)
{
    REQUEST_HILOGD("Request API8");
    return JsInitialize::Initialize(env, info, Version::API8);
}

// Returns this thread's cached Task class used to construct instances for
// tasks fetched from the server (clzDes methods, GetTaskCreate native
// constructor).
napi_value JsTask::GetTaskCtor(napi_env env)
{
    REQUEST_HILOGD("GetTaskCtor in");
    std::lock_guard<std::mutex> lock(getTaskCreateMutex_);
    napi_value cons;
    if (getTaskCreateCtor != nullptr) {
        REQUEST_NAPI_CALL(
            env, napi_get_reference_value(env, getTaskCreateCtor, &cons), "napi_get_reference_value failed");
        return cons;
    }
    size_t count = sizeof(clzDes) / sizeof(napi_property_descriptor);
    return DefineClass(env, clzDes, count, GetTaskCreate, &getTaskCreateCtor);
}

// Native constructor used when getTask instantiates the Task class.
napi_value JsTask::GetTaskCreate(napi_env env, napi_callback_info info)
{
    REQUEST_HILOGD("GetTask Create");
    return JsInitialize::Initialize(env, info, Version::API10, false);
}

// request.agent.getTask(context, tid, token?): AsyncCall entry — the exec
// stage serves tasks created in this process from the local map and fetches
// everything else from the server; the output stage hands back the live JS
// object or constructs a fresh one (GetTaskOutput).
napi_value JsTask::GetTask(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("GetTask seq %{public}d", seq);
    auto context = std::make_shared<ContextInfo>();
    context->withErrCode_ = true;
    context->version_ = Version::API10;
    auto input = [context, seq](size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ExceptionError err = ParseGetTask(context->env_, argc, argv, context);
        if (err.code != E_OK) {
            REQUEST_HILOGE("End get task in AsyncCall input, seq: %{public}d, failed: parse task failed", seq);
            NapiUtils::ThrowError(context->env_, err.code, err.errInfo, true);
            return napi_invalid_arg;
        }
        napi_status status = napi_create_reference(context->env_, argv[0], NapiUtils::ONE_REF, &(context->baseContext));
        return status;
    };
    auto output = [context, seq](napi_value *result) -> napi_status {
        if (context->innerCode_ != E_OK) {
            REQUEST_HILOGE(
                "End get task in AsyncCall output, seq: %{public}d, failed: %{public}d", seq, context->innerCode_);
            return napi_generic_failure;
        }
        if (GetTaskOutput(context, result, seq) != napi_ok) {
            REQUEST_HILOGE("End get task in AsyncCall output, seq: %{public}d, failed: get task output failed", seq);
            return napi_generic_failure;
        }
        REQUEST_HILOGI("End GetTask %{public}s", context->tid.c_str());
        return napi_ok;
    };
    auto exec = [context]() { GetTaskExecution(context); };
    context->SetInput(input).SetOutput(output).SetExec(exec);
    AsyncCall asyncCall(env, info, context);
    return asyncCall.Call(context, "getTask");
}

// Worker stage of getTask: fast path — the task was created in this
// process (taskContextMap_ hit), reuse its config after the token
// capability check; slow path — fetch the config from the server, set
// saveas for downloads, re-grant the sandbox ACLs, and require the task to
// be an API10 task whose token matches.
void JsTask::GetTaskExecution(std::shared_ptr<ContextInfo> context)
{
    // Set context->config.
    {
        std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
        std::string tid = context->tid;
        auto it = taskContextMap_.find(tid);
        if (it != taskContextMap_.end() && it->second->task != nullptr) {
            // The fast path must also enforce the token capability: the cached
            // task's token must match the caller-supplied token before the
            // existing Request object is handed back. Otherwise a known tid alone
            // is enough to retrieve another task's query/pause/resume/remove
            // capabilities without a valid token.
            if (it->second->task->config_.token != context->token) {
                context->innerCode_ = E_TASK_NOT_FOUND;
                return;
            }
            context->config = it->second->task->config_;
            return;
        }
    }
    std::string tid = context->tid;
    REQUEST_HILOGD("Process get task, tid: %{public}s", context->tid.c_str());
    Config &config = context->config;
    context->innerCode_ = RequestManager::GetInstance()->GetTask(tid, context->token, config);
    if (config.action == Action::DOWNLOAD && config.files.size() != 0) {
        config.saveas = config.files[0].uri;
    }
    if (context->innerCode_ == E_OK) {
        // Authorize the path
        int32_t err = JsTask::AuthorizePath(config);
        if (err != E_OK) {
            context->innerCode_ = err;
            return;
        }
    }

    if (context->config.version != Version::API10 || context->config.token != context->token) {
        context->innerCode_ = E_TASK_NOT_FOUND;
        return;
    }
}

// Constructs a fresh JS Task object for a server-fetched task: converts
// the config, instantiates the getTask class with (context, config),
// unwraps the JsTask, exposes config/tid (API10) and pins a reference.
napi_status JsTask::CtorJsTask(std::shared_ptr<ContextInfo> context, napi_value *result)
{
    napi_value jsConfig = NapiUtils::Convert2JSValueConfig(context->env_, context->config);
    napi_value ctor = GetTaskCtor(context->env_);
    napi_value baseCtx = nullptr;
    napi_status status = napi_generic_failure;
    status = napi_get_reference_value(context->env_, context->baseContext, &baseCtx);

    napi_value args[2] = { baseCtx, jsConfig };
    status = napi_new_instance(context->env_, ctor, NapiUtils::TWO_ARG, args, result);
    if (status != napi_ok || result == nullptr) {
        REQUEST_HILOGE("Get task failed, reason: %{public}d", status);
        return napi_generic_failure;
    }
    status = napi_unwrap(context->env_, *result, reinterpret_cast<void **>(&context->task));
    if (status != napi_ok) {
        return status;
    }
    context->task->SetTid(context->tid);
    if (context->version_ == Version::API10) {
        status = napi_set_named_property(context->env_, *result, "config", jsConfig);
        if (status != napi_ok) {
            return status;
        }
        NapiUtils::SetStringPropertyUtf8(context->env_, *result, "tid", context->tid);
    }
    status = napi_create_reference(context->env_, *result, NapiUtils::ONE_REF, &(context->taskRef));
    return status;
}

// Hands back the JS object of an already-live task; refused when the
// caller's env differs from the task's creating env (a JS object cannot
// cross envs).
napi_status JsTask::CheckTaskInMap(
    std::shared_ptr<ContextInfo> context, std::shared_ptr<ContextInfo> mapContext, napi_value *result)
{
    context->taskRef = nullptr;
    if (context->env_ != mapContext->env_) {
        REQUEST_HILOGE("getTask in different envs %{public}s", context->tid.c_str());
        context->innerCode_ = E_PARAMETER_CHECK;
        return napi_generic_failure;
    }
    context->task = mapContext->task;
    napi_status status = napi_get_reference_value(context->env_, mapContext->taskRef, result);
    if (status != napi_ok) {
        return status;
    }
    context->contextIf = false;
    return napi_ok;
}

// Output stage of getTask: live task in the map — reuse its object
// (CheckTaskInMap); new task — construct via CtorJsTask, record it in
// taskContextMap_, subscribe to SA death and register the REMOVE listener.
napi_status JsTask::GetTaskOutput(std::shared_ptr<ContextInfo> context, napi_value *result, int32_t seq)
{
    std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
    std::string tid = context->tid;
    auto it = taskContextMap_.find(tid);
    if (it != taskContextMap_.end() && it->second->task != nullptr) {
        return JsTask::CheckTaskInMap(context, it->second, result);
    }
    if (context->contextIf) {
        context->innerCode_ = E_PARAMETER_CHECK;
        REQUEST_HILOGE("End get task in AsyncCall output failed by error context");
        return napi_generic_failure;
    }
    napi_status status = JsTask::CtorJsTask(context, result);
    if (status != napi_ok) {
        JsTask::DeleteContextTaskRef(context);
        return status;
    }
    auto [itContext, insertedContext] = JsTask::taskContextMap_.try_emplace(context->tid, context);
    if (!insertedContext) {
        REQUEST_HILOGE("GAddContext Exist %{public}s", context->tid.c_str());
    }
    if (!JsTask::taskContextMap_.empty()) {
        JsTask::SubscribeSA();
    }
    JsTask::AddRemoveListener(context);
    return napi_ok;
}

// Validates (context, tid, token?): tid must be a non-empty string of at
// most 32 bytes; the optional token must be 8-2048 bytes.
ExceptionError JsTask::ParseGetTask(napi_env env, size_t argc, napi_value *argv, std::shared_ptr<ContextInfo> context)
{
    ExceptionError err = { .code = E_OK };
    // need at least 2 params.
    if (argc < 2) {
        REQUEST_HILOGE("Wrong number of arguments");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Missing mandatory parameters, need at least two params, context and id";
        return err;
    }
    std::shared_ptr<OHOS::AbilityRuntime::Context> runtimeContext = nullptr;
    napi_status getStatus = JsInitialize::GetContext(env, argv[0], runtimeContext);
    if (getStatus != napi_ok) {
        REQUEST_HILOGE("GetTask context fail");
        context->contextIf = true;
    }
    if (NapiUtils::GetValueType(env, argv[1]) != napi_string) {
        REQUEST_HILOGE("The parameter: tid is not of string type");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Incorrect parameter type, tid is not of string type";
        return err;
    }
    std::string tid = NapiUtils::Convert2String(env, argv[1]);
    if (tid.empty()) {
        REQUEST_HILOGE("tid is empty");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Parameter verification failed, tid is empty";
        return err;
    }
    // tid length <= 32
    if (tid.size() > 32) {
        REQUEST_HILOGE("tid invalid, %{public}s", tid.c_str());
        err.code = E_TASK_NOT_FOUND;
        err.errInfo = "task not found error";
        return err;
    }
    context->tid = tid;
    // handle 3rd param TOKEN
    if (argc == 3) {
        if (NapiUtils::GetValueType(env, argv[2]) != napi_string) { // argv[2] is the 3rd param
            REQUEST_HILOGE("The parameter: token is not of string type");
            err.code = E_PARAMETER_CHECK;
            err.errInfo = "Incorrect parameter type, token is not of string type";
            return err;
        }
        uint32_t bufferLen = TOKEN_MAX_BYTES + 2;
        std::unique_ptr<char[]> token = std::make_unique<char[]>(bufferLen);
        size_t len = 0;
        napi_status status = napi_get_value_string_utf8(env, argv[2], token.get(), bufferLen, &len);
        if (status != napi_ok) {
            REQUEST_HILOGE("napi get value string utf8 failed");
            memset_s(token.get(), bufferLen, 0, bufferLen);
            err.code = E_PARAMETER_CHECK;
            err.errInfo = "Parameter verification failed, get parameter token failed";
            return err;
        }
        if (len < TOKEN_MIN_BYTES || len > TOKEN_MAX_BYTES) {
            memset_s(token.get(), bufferLen, 0, bufferLen);
            err.code = E_PARAMETER_CHECK;
            err.errInfo = "Parameter verification failed, the length of token should between 8 and 2048 bytes";
            return err;
        }
        context->token = std::string(token.get(), len);
        memset_s(token.get(), bufferLen, 0, bufferLen);
    }
    return err;
}

// request.agent.remove(tid): removes the task on the server; resolves with
// true on success, rejects with the error code otherwise.
napi_value JsTask::Remove(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("Begin remove seq %{public}d", seq);
    struct RemoveContext : public AsyncCall::Context {
        std::string tid;
        bool res = false;
    };

    auto context = std::make_shared<RemoveContext>();
    context->withErrCode_ = true;
    context->version_ = Version::API10;
    auto input = [context, seq](size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ExceptionError err = ParseTid(context->env_, argc, argv, context->tid);
        if (err.code != E_OK) {
            REQUEST_HILOGE("End task remove in AsyncCall input, seq: %{public}d, failed: tid invalid", seq);
            NapiUtils::ThrowError(context->env_, err.code, err.errInfo, true);
            return napi_invalid_arg;
        }
        return napi_ok;
    };
    auto output = [context, seq](napi_value *result) -> napi_status {
        if (context->innerCode_ != E_OK) {
            context->res = false;
            REQUEST_HILOGE(
                "End task remove in AsyncCall output, seq: %{public}d, failed: %{public}d", seq, context->innerCode_);
            return napi_generic_failure;
        }
        REQUEST_HILOGI("End remove seq %{public}d", seq);
        return NapiUtils::Convert2JSValue(context->env_, context->res, *result);
    };
    auto exec = [context]() {
        context->innerCode_ = RequestManager::GetInstance()->Remove(context->tid, Version::API10);
    };
    context->SetInput(std::move(input)).SetOutput(std::move(output)).SetExec(std::move(exec));
    AsyncCall asyncCall(env, info, context);
    return asyncCall.Call(context, "remove");
}

// Validates the tid argument (non-empty string) shared by remove/query.
ExceptionError JsTask::ParseTid(napi_env env, size_t argc, napi_value *argv, std::string &tid)
{
    ExceptionError err = { .code = E_OK };
    if (argc < 1) {
        REQUEST_HILOGE("Wrong number of arguments");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Missing mandatory parameters, missing tid";
        return err;
    }
    if (NapiUtils::GetValueType(env, argv[0]) != napi_string) {
        REQUEST_HILOGE("The first parameter is not of string type");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Incorrect parameter type, tid is not of string type";
        return err;
    }
    tid = NapiUtils::Convert2String(env, argv[0]);
    if (tid.empty()) {
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Parameter verification failed, tid is empty";
        return err;
    }
    return err;
}

// request.agent.show(tid): task info without the token capability — the
// shared TouchInner path with the default "null" token.
napi_value JsTask::Show(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("Begin show seq %{public}d", seq);
    auto context = std::make_shared<TouchContext>();
    auto input = [context, seq](size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ExceptionError err = ParseTid(context->env_, argc, argv, context->tid);
        if (err.code != E_OK) {
            REQUEST_HILOGE("End task show in AsyncCall input, seq: %{public}d, failed: tid invalid", seq);
            NapiUtils::ThrowError(context->env_, err.code, err.errInfo, true);
            return napi_invalid_arg;
        }
        // tid length <= 32
        if (context->tid.size() > 32) {
            REQUEST_HILOGE("End task show in AsyncCall input, seq: %{public}d, failed: tid invalid", seq);
            NapiUtils::ThrowError(context->env_, E_TASK_NOT_FOUND, "task not found error", true);
            return napi_invalid_arg;
        }
        return napi_ok;
    };
    return TouchInner(env, info, std::move(input), std::move(context), seq);
}

// request.agent.touch(tid, token): task info guarded by the token
// capability.
napi_value JsTask::Touch(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("Begin touch seq %{public}d", seq);
    auto context = std::make_shared<TouchContext>();
    auto input = [context, seq](size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ExceptionError err = ParseTouch(context->env_, argc, argv, context);
        if (err.code != E_OK) {
            REQUEST_HILOGE("End task touch in AsyncCall input, seq: %{public}d, failed: arg invalid", seq);
            NapiUtils::ThrowError(context->env_, err.code, err.errInfo, true);
            return napi_invalid_arg;
        }
        return napi_ok;
    };
    return TouchInner(env, info, std::move(input), std::move(context), seq);
}

// Shared implementation of show/touch: exec calls the server Touch and
// fills taskInfo; output resolves with the serialized TaskInfo.
napi_value JsTask::TouchInner(napi_env env, napi_callback_info info, AsyncCall::Context::InputAction input,
    std::shared_ptr<TouchContext> context, int32_t seq)
{
    context->withErrCode_ = true;
    context->version_ = Version::API10;
    auto output = [context, seq](napi_value *result) -> napi_status {
        if (context->innerCode_ != E_OK) {
            REQUEST_HILOGE(
                "End task show in AsyncCall output, seq: %{public}d, failed: %{public}d", seq, context->innerCode_);
            return napi_generic_failure;
        }
        *result = NapiUtils::Convert2JSValue(context->env_, context->taskInfo);
        REQUEST_HILOGI("End show seq %{public}d", seq);
        return napi_ok;
    };
    auto exec = [context]() {
        context->innerCode_ = RequestManager::GetInstance()->Touch(context->tid, context->token, context->taskInfo);
    };
    context->SetInput(std::move(input)).SetOutput(std::move(output)).SetExec(std::move(exec));
    AsyncCall asyncCall(env, info, context);
    return asyncCall.Call(context, "touch");
}

bool JsTask::ParseTouchCheck(const napi_env env, const size_t argc, const napi_value *argv,
    const std::shared_ptr<TouchContext> context, ExceptionError &err)
{
    // 2 means least param num.
    if (argc < 2) {
        REQUEST_HILOGE("Wrong number of arguments");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Missing mandatory parameters, need at least two params, id and token";
        return false;
    }
    if (NapiUtils::GetValueType(env, argv[0]) != napi_string || NapiUtils::GetValueType(env, argv[1]) != napi_string) {
        REQUEST_HILOGE("The parameter: tid is not of string type");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Incorrect parameter type, tid is not of string type";
        return false;
    }
    context->tid = NapiUtils::Convert2String(env, argv[0]);
    if (context->tid.empty()) {
        REQUEST_HILOGE("tid is empty");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Parameter verification failed, tid is empty";
        return false;
    }
    // tid length <= 32
    if (context->tid.size() > 32) {
        REQUEST_HILOGE("tid invalid, %{public}s", context->tid.c_str());
        err.code = E_TASK_NOT_FOUND;
        err.errInfo = "task not found error";
        return false;
    }
    return true;
}

// Validates (tid, token) for touch: tid via the shared checks above, then
// the token must be 8-2048 bytes.
ExceptionError JsTask::ParseTouch(napi_env env, size_t argc, napi_value *argv, std::shared_ptr<TouchContext> context)
{
    ExceptionError err = { .code = E_OK };
    if (!JsTask::ParseTouchCheck(env, argc, argv, context, err)) {
        return err;
    }
    uint32_t bufferLen = TOKEN_MAX_BYTES + 2;
    char *token = new (std::nothrow) char[bufferLen];
    if (token == nullptr) {
        err.code = E_OTHER;
        err.errInfo = "cannot new token";
        return err;
    }
    size_t len = 0;
    napi_status status = napi_get_value_string_utf8(env, argv[1], token, bufferLen, &len);
    if (status != napi_ok) {
        REQUEST_HILOGE("napi get value string utf8 failed");
        memset_s(token, bufferLen, 0, bufferLen);
        delete[] token;
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Parameter verification failed, get token failed";
        return err;
    }
    if (len < TOKEN_MIN_BYTES || len > TOKEN_MAX_BYTES) {
        memset_s(token, bufferLen, 0, bufferLen);
        delete[] token;
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Parameter verification failed, the length of token should between 8 and 2048 bytes";
        return err;
    }
    context->token = std::string(token, len);
    memset_s(token, bufferLen, 0, bufferLen);
    delete[] token;
    return err;
}

// Parses the optional search filter. Defaults (also used per field when
// the value is missing or of the wrong type): bundle "*", state/action/
// mode ANY, and the time window [now - one day, now]. A window whose end
// (before) precedes its start (after) is rejected.
ExceptionError JsTask::ParseSearch(napi_env env, size_t argc, napi_value *argv, Filter &filter)
{
    ExceptionError err = { .code = E_OK };
    using namespace std::chrono;
    filter.bundle = "*";
    filter.before = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    filter.after = filter.before - MILLISECONDS_IN_ONE_DAY;
    if (argc < 1) {
        return err;
    }
    napi_valuetype valueType = NapiUtils::GetValueType(env, argv[0]);
    if (valueType == napi_null || valueType == napi_undefined) {
        return err;
    }
    if (valueType != napi_object) {
        REQUEST_HILOGE("The parameter: filter is not of object type");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Incorrect parameter type, filter is not of object type";
        return err;
    }
    filter.bundle = ParseBundle(env, argv[0]);
    filter.before = ParseBefore(env, argv[0]);
    filter.after = ParseAfter(env, argv[0], filter.before);
    if (filter.before < filter.after) {
        REQUEST_HILOGE("before is small than after");
        err.code = E_PARAMETER_CHECK;
        err.errInfo = "Parameter verification failed, filter before is small than after";
        return err;
    }
    filter.state = ParseState(env, argv[0]);
    filter.action = ParseAction(env, argv[0]);
    filter.mode = ParseMode(env, argv[0]);
    return err;
}

// Lenient per-field filter readers: a missing or wrong-typed value falls
// back to the field's default (wildcard for bundle/state/action/mode, now
// for before, now - one day for after).
std::string JsTask::ParseBundle(napi_env env, napi_value value)
{
    if (!NapiUtils::HasNamedProperty(env, value, "bundle")) {
        return "*";
    }
    napi_value value1 = NapiUtils::GetNamedProperty(env, value, "bundle");
    if (NapiUtils::GetValueType(env, value1) != napi_string) {
        return "*";
    }
    return NapiUtils::Convert2String(env, value1);
}

State JsTask::ParseState(napi_env env, napi_value value)
{
    if (!NapiUtils::HasNamedProperty(env, value, "state")) {
        return State::ANY;
    }
    napi_value value1 = NapiUtils::GetNamedProperty(env, value, "state");
    if (NapiUtils::GetValueType(env, value1) != napi_number) {
        return State::ANY;
    }
    return static_cast<State>(NapiUtils::Convert2Uint32(env, value1));
}

Action JsTask::ParseAction(napi_env env, napi_value value)
{
    if (!NapiUtils::HasNamedProperty(env, value, "action")) {
        return Action::ANY;
    }
    napi_value value1 = NapiUtils::GetNamedProperty(env, value, "action");
    if (NapiUtils::GetValueType(env, value1) != napi_number) {
        return Action::ANY;
    }
    return static_cast<Action>(NapiUtils::Convert2Uint32(env, value1));
}

Mode JsTask::ParseMode(napi_env env, napi_value value)
{
    if (!NapiUtils::HasNamedProperty(env, value, "mode")) {
        return Mode::ANY;
    }
    napi_value value1 = NapiUtils::GetNamedProperty(env, value, "mode");
    if (NapiUtils::GetValueType(env, value1) != napi_number) {
        return Mode::ANY;
    }
    return static_cast<Mode>(NapiUtils::Convert2Uint32(env, value1));
}

int64_t JsTask::ParseBefore(napi_env env, napi_value value)
{
    using namespace std::chrono;
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    if (!NapiUtils::HasNamedProperty(env, value, "before")) {
        return now;
    }
    napi_value value1 = NapiUtils::GetNamedProperty(env, value, "before");
    if (NapiUtils::GetValueType(env, value1) != napi_number) {
        return now;
    }
    int64_t ret = 0;
    REQUEST_NAPI_CALL_RETURN(env, napi_get_value_int64(env, value1, &ret), "napi_get_value_int64 failed", now);
    return ret;
}

int64_t JsTask::ParseAfter(napi_env env, napi_value value, int64_t before)
{
    int64_t defaultValue = before - MILLISECONDS_IN_ONE_DAY;
    if (!NapiUtils::HasNamedProperty(env, value, "after")) {
        return defaultValue;
    }
    napi_value value1 = NapiUtils::GetNamedProperty(env, value, "after");
    if (NapiUtils::GetValueType(env, value1) != napi_number) {
        return defaultValue;
    }
    int64_t ret = 0;
    REQUEST_NAPI_CALL_RETURN(env, napi_get_value_int64(env, value1, &ret), "napi_get_value_int64", defaultValue);
    return ret;
}

// request.agent.search(filter?): resolves with the tids matching the
// filter (defaults and leniency per ParseSearch).
napi_value JsTask::Search(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("Begin search seq %{public}d", seq);
    struct SearchContext : public AsyncCall::Context {
        Filter filter;
        std::vector<std::string> tids;
    };

    auto context = std::make_shared<SearchContext>();
    context->withErrCode_ = true;
    context->version_ = Version::API10;
    auto input = [context, seq](size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ExceptionError err = ParseSearch(context->env_, argc, argv, context->filter);
        if (err.code != E_OK) {
            REQUEST_HILOGE("End task search in AsyncCall input, seq: %{public}d, failed: arg invalid", seq);
            NapiUtils::ThrowError(context->env_, err.code, err.errInfo, true);
            return napi_invalid_arg;
        }
        return napi_ok;
    };
    auto output = [context, seq](napi_value *result) -> napi_status {
        if (context->innerCode_ != E_OK) {
            REQUEST_HILOGE(
                "End task search in AsyncCall output, seq: %{public}d, failed: %{public}d", seq, context->innerCode_);
            return napi_generic_failure;
        }
        *result = NapiUtils::Convert2JSValue(context->env_, context->tids);
        REQUEST_HILOGI("End search seq %{public}d", seq);
        return napi_ok;
    };
    auto exec = [context]() {
        context->innerCode_ = RequestManager::GetInstance()->Search(context->filter, context->tids);
    };
    context->SetInput(std::move(input)).SetOutput(std::move(output)).SetExec(std::move(exec));
    AsyncCall asyncCall(env, info, context);
    return asyncCall.Call(context, "search");
}

// request.agent.query(tid): resolves with the task's TaskInfo; the output
// stage always sets withSystem, which adds uid/bundle and clears url/data
// plus the upload file list in the serialized result (see Convert2JSValue).
napi_value JsTask::Query(napi_env env, napi_callback_info info)
{
    int32_t seq = RequestManager::GetInstance()->GetNextSeq();
    REQUEST_HILOGI("Begin query seq %{public}d", seq);
    struct QueryContext : public AsyncCall::Context {
        std::string tid;
        TaskInfo taskInfo;
    };

    auto context = std::make_shared<QueryContext>();
    context->withErrCode_ = true;
    context->version_ = Version::API10;
    auto input = [context, seq](size_t argc, napi_value *argv, napi_value self) -> napi_status {
        ExceptionError err = ParseTid(context->env_, argc, argv, context->tid);
        if (err.code != E_OK) {
            REQUEST_HILOGE("End task query in AsyncCall input, seq: %{public}d, failed: tid invalid", seq);
            NapiUtils::ThrowError(context->env_, err.code, err.errInfo, true);
            return napi_invalid_arg;
        }
        return napi_ok;
    };
    auto output = [context, seq](napi_value *result) -> napi_status {
        if (context->innerCode_ != E_OK) {
            REQUEST_HILOGE(
                "End task query in AsyncCall output, seq: %{public}d, failed: %{public}d", seq, context->innerCode_);
            return napi_generic_failure;
        }
        context->taskInfo.withSystem = true;
        *result = NapiUtils::Convert2JSValue(context->env_, context->taskInfo);
        REQUEST_HILOGI("End query seq %{public}d", seq);
        return napi_ok;
    };
    auto exec = [context]() {
        context->innerCode_ = RequestManager::GetInstance()->Query(context->tid, context->taskInfo);
    };
    context->SetInput(std::move(input)).SetOutput(std::move(output)).SetExec(std::move(exec));
    AsyncCall asyncCall(env, info, context);
    return asyncCall.Call(context, "query");
}

std::string JsTask::GetTid()
{
    return tid_;
}

void JsTask::SetTid(std::string &tid)
{
    tid_ = tid;
}

// Subscribes to server SA status notifications; guarded by taskContextMap_
// occupancy (see AddTaskWhenCreate/RemoveTaskContext).
void JsTask::SubscribeSA()
{
    REQUEST_HILOGD("SubscribeSA in");
    if (!RequestManager::GetInstance()->SubscribeSA()) {
        REQUEST_HILOGE("SubscribeSA Failed");
    }
}

void JsTask::UnsubscribeSA()
{
    REQUEST_HILOGD("UnsubscribeSA in");
    if (!RequestManager::GetInstance()->UnsubscribeSA()) {
        REQUEST_HILOGE("UnsubscribeSA Failed");
    }
}

// Re-subscribes every live tid with the (restarted) server SA so their
// event pushes continue to be delivered to this process.
void JsTask::ReloadListener()
{
    REQUEST_HILOGD("ReloadListener in");
    // collect all tids first to reduce lock holding time
    std::vector<std::string> tids;
    {
        std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
        for (const auto &it : taskContextMap_) {
            tids.push_back(it.first);
        }
    }
    for (const auto &it : tids) {
        REQUEST_HILOGD("ReloadListener tid: %{public}s", it.c_str());
        RequestManager::GetInstance()->Subscribe(it);
    }
}

// Stages certificate files for upload: copies every file found in the
// given dirs into the fixed sandbox path .ohos/.request/.certs and grants
// the SA read access to the staged copies; on success the caller's dirs
// list is replaced with the staging path.
bool JsTask::SetDirsPermission(std::vector<std::string> &dirs)
{
    if (dirs.empty()) {
        return true;
    }
    std::string newPath = "/data/storage/el2/base/.ohos/.request/.certs";
    std::vector<std::string> dirElems;
    JsInitialize::StringSplit(newPath, '/', dirElems);
    if (!JsInitialize::CreateDirs(dirElems)) {
        REQUEST_HILOGE("CreateDirs Error");
        return false;
    }

    for (const auto &folderPath : dirs) {
        fs::path folder = folderPath;
        if (!(fs::exists(folder) && fs::is_directory(folder))) {
            return false;
        }
        for (const auto &entry : fs::directory_iterator(folder)) {
            fs::path path = entry.path();
            std::string existfilePath = folder.string() + "/" + path.filename().string();
            std::string newfilePath = newPath + "/" + path.filename().string();
            if (!fs::exists(newfilePath)) {
                fs::copy(existfilePath, newfilePath);
            }
            // Certs only need read permission.
            if (!PathUtils::AddPathsToMap(newfilePath, Action::UPLOAD)) {
                REQUEST_HILOGE("Set path permission fail.");
                return false;
            }
        }
    }
    if (!dirs.empty()) {
        dirs.clear();
        dirs.push_back(newPath);
    }
    return true;
}

// Releases the read ACLs granted by SetDirsPermission for every file
// under the staged cert dirs.
void JsTask::RemoveDirsPermission(const std::vector<std::string> &dirs)
{
    for (const auto &folderPath : dirs) {
        fs::path folder = folderPath;
        for (const auto &entry : fs::directory_iterator(folder)) {
            fs::path path = entry.path();
            std::string filePath = folder.string() + "/" + path.filename().string();
            PathUtils::SubPathsToMap(filePath);
        }
    }
}

// Deletes the task's body temp files only — no ACL release; used for
// failure cleanup where the task entry itself stays alive.
void JsTask::ClearTaskTemp(const std::string &tid)
{
    std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
    auto it = taskContextMap_.find(tid);
    if (it == taskContextMap_.end()) {
        REQUEST_HILOGD("ClearTaskTemp files, not in ContextMap");
        return;
    }
    auto context = it->second;
    auto bodyFileNames = context->task->config_.bodyFileNames;
    for (auto &filePath : bodyFileNames) {
        std::error_code err;
        if (!std::filesystem::exists(filePath, err)) {
            continue;
        }
        err.clear();
        NapiUtils::RemoveFile(filePath);
    }
}

// Full per-task release when the task ends: revoke the bodyFileNames and
// file ACLs, delete the body temp files, release the cert permissions and
// mark the task as no longer holding grants.
void JsTask::ClearTaskThing(const std::string &tid)
{
    std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
    auto it = taskContextMap_.find(tid);
    if (it == taskContextMap_.end()) {
        REQUEST_HILOGD("ClearTaskThing files, not in ContextMap");
        return;
    }
    auto context = it->second;
    auto bodyFileNames = context->task->config_.bodyFileNames;
    for (auto &filePath : bodyFileNames) {
        PathUtils::SubPathsToMap(filePath);
        std::error_code err;
        if (!std::filesystem::exists(filePath, err)) {
            continue;
        }
        err.clear();
        NapiUtils::RemoveFile(filePath);
    }
    // Reset Acl permission
    for (auto &file : context->task->config_.files) {
        PathUtils::SubPathsToMap(file.uri);
    }
    context->task->isGetPermission = false;
    RemoveDirsPermission(context->task->config_.certsPath);
}

// Drops a task from the process table: deletes its notify-data listener
// references, erases the map entry, unsubscribes from SA status when the
// table empties, and schedules the JS reference cleanup.
void JsTask::RemoveTaskContext(const std::string &tid)
{
    std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
    auto it = taskContextMap_.find(tid);
    if (it == taskContextMap_.end()) {
        REQUEST_HILOGD("Clear task tmp files, not in ContextMap");
        return;
    }
    auto context = it->second;

    {
        std::lock_guard<std::mutex> listenerLockGuard(context->task->listenerMutex_);
        auto map = context->task->notifyDataListenerMap_;
        for (auto i = map.begin(); i != map.end(); i++) {
            i->second->DeleteAllListenerRef();
        }
        map.clear();
    }
    taskContextMap_.erase(it);
    if (taskContextMap_.empty()) {
        JsTask::UnsubscribeSA();
    }
    DeleteContextTaskRef(context);
}

// JS-thread cleanup helpers of DeleteContextTaskRef: each drops one
// persistent reference (the JS Task object / the ability context).
bool JsTask::DeleteTaskRef(ContextCallbackData *data)
{
    if (data->context->taskRef == nullptr) {
        return true;
    }
    napi_status status = napi_delete_reference(data->context->env_, data->context->taskRef);
    if (status != napi_ok) {
        REQUEST_HILOGE("UnrefTask delete taskRef failed");
        return false;
    }
    data->context->taskRef = nullptr;
    return true;
}

bool JsTask::DeleteBaseContextRef(ContextCallbackData *data)
{
    if (data->context->baseContext == nullptr) {
        return true;
    }
    napi_status status = napi_delete_reference(data->context->env_, data->context->baseContext);
    if (status != napi_ok) {
        REQUEST_HILOGE("UnrefTask delete baseContext failed");
        return false;
    }
    data->context->baseContext = nullptr;
    return true;
}

// Release the strong reference (taskRef) to the JS Task object on the JS thread.
// The lifetime of `data` is managed by the asynchronous callback and relies on the
// NAPI event loop dispatching it normally. In abnormal cases the callback may not run
// and `data` would leak; this is acceptable since the function is only called during
// task cleanup.
void JsTask::DeleteContextTaskRef(std::shared_ptr<ContextInfo> context)
{
    ContextCallbackData *data = new ContextCallbackData();
    if (data == nullptr) {
        return;
    }
    data->context = context;
    auto callback = [data]() {
        if (data == nullptr) {
            return;
        }
        if (data->context == nullptr || data->context->env_ == nullptr ||
            (data->context->taskRef == nullptr && data->context->baseContext == nullptr)) {
            delete data;
            return;
        }
        napi_handle_scope scope = nullptr;
        napi_status status = napi_open_handle_scope(data->context->env_, &scope);
        if (status != napi_ok || scope == nullptr) {
            REQUEST_HILOGE("UnrefTask napi_scope failed");
            delete data;
            return;
        }
        bool taskRefDeleted = DeleteTaskRef(data);
        bool baseContextDeleted = DeleteBaseContextRef(data);
        if (!taskRefDeleted || !baseContextDeleted) {
            REQUEST_HILOGE("UnrefTask delete reference failed");
        }
        status = napi_close_handle_scope(data->context->env_, scope);
        if (status != napi_ok) {
            REQUEST_HILOGE("UnrefTask napi_close_handle_scope failed");
        }
        delete data;
    };

    int32_t ret = napi_send_event(data->context->env_, callback, napi_eprio_high,
        "request:download|downloadfile|upload|uploadfile|agent.create");
    if (ret != napi_ok) {
        REQUEST_HILOGE("napi_send_event failed: %{public}d", ret);
        delete data;
    }
    return;
}

// Strict equality of a value against a referenced value; a null ref only
// matches a null value.
bool JsTask::Equals(napi_env env, napi_value value, napi_ref copy)
{
    if (copy == nullptr) {
        return (value == nullptr);
    }

    napi_value copyValue = nullptr;
    napi_get_reference_value(env, copy, &copyValue);

    bool isEquals = false;
    napi_strict_equals(env, value, copyValue, &isEquals);
    return isEquals;
}

// Installs the process-wide foreground-resume callback (AppStateCallback)
// on the first FOREGROUND task; the callback itself clears register_ when
// the process no longer holds any foreground task, so a later foreground
// task can re-register.
void JsTask::RegisterForegroundResume()
{
    if (register_) {
        return;
    }
    REQUEST_HILOGI("Process register foreground resume callback");
    register_ = true;
    auto context = AbilityRuntime::ApplicationContext::GetInstance();
    if (context == nullptr) {
        REQUEST_HILOGE("End register foreground resume callback, failed: Get ApplicationContext failed");
        SysEventLog::SendSysEventLog(FAULT_EVENT, ABMS_FAULT_00, "Register failed get AppContext");
        return;
    }
    context->RegisterAbilityLifecycleCallback(std::make_shared<AppStateCallback>());
    REQUEST_HILOGI("End register foreground resume callback successfully");
}
} // namespace OHOS::Request