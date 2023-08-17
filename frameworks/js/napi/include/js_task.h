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

#ifndef REQUEST_TASK_NAPI
#define REQUEST_TASK_NAPI

#include "async_call.h"
#include "js_common.h"
#include "request_notify.h"

namespace OHOS::Request {
class JsTask {
public:
    ~JsTask();
    static napi_value JsCreate(napi_env env, napi_callback_info info);
    static napi_value JsUpload(napi_env env, napi_callback_info info);
    static napi_value JsDownload(napi_env env, napi_callback_info info);
    static napi_value JsRequest(napi_env env, napi_callback_info info);
    static napi_value JsRequestFile(napi_env env, napi_callback_info info);

    static napi_value Remove(napi_env env, napi_callback_info info);
    static napi_value Show(napi_env env, napi_callback_info info);
    static napi_value Touch(napi_env env, napi_callback_info info);
    static napi_value Search(napi_env env, napi_callback_info info);
    static napi_value Query(napi_env env, napi_callback_info info);

    std::string GetTid();
    void SetTid(int32_t tid);
    void AddListener(const std::string &key, const sptr<RequestNotify> &listener);
    void RemoveListener(const std::string &type, const std::string &tid, napi_value callback);
    void RemoveListener(const std::string &type, const std::string &tid);
    size_t GetListenerSize(const std::string &key);
    void ClearListener();

    static void ClearTaskMap(const std::string &key);
    static void AddTaskMap(const std::string &key, JsTask* task);

    Config config_;
    static std::mutex taskMutex_;
    static std::map<std::string, JsTask*> taskMap_;
    std::mutex listenerMutex_;
    std::map<std::string, std::vector<sptr<RequestNotify>>> listenerMap_;
private:
    struct ContextInfo : public AsyncCall::Context {
        JsTask *task = nullptr;
        napi_ref taskRef = nullptr;
        napi_ref jsConfig = nullptr;
        Config config{};
        int32_t tid{};
    };

    struct TouchContext : public AsyncCall::Context {
        std::string tid;
        std::string token = "null";
        TaskInfo taskInfo;
    };

    static napi_value DefineClass(napi_env env, const napi_property_descriptor* desc, size_t count,
        napi_callback cb, napi_ref *ctor);
    static napi_value JsMain(napi_env env, napi_callback_info info, Version version);
    static napi_value Create(napi_env env, napi_callback_info info);
    static napi_value GetCtor(napi_env env, Version version);
    static napi_value GetCtorV8(napi_env env);
    static napi_value GetCtorV9(napi_env env);
    static napi_value GetCtorV10(napi_env env);
    static napi_value RequestFile(napi_env env, napi_callback_info info);
    static napi_value RequestFileV8(napi_env env, napi_callback_info info);
    static int32_t CreateExec(const std::shared_ptr<ContextInfo> &context);
    static std::string ParseTid(napi_env env, size_t argc, napi_value *argv);
    static napi_value TouchInner(napi_env env, napi_callback_info info, AsyncCall::Context::InputAction action,
        std::shared_ptr<TouchContext> context);
    static bool ParseSearch(napi_env env, size_t argc, napi_value *argv, Filter &filter);
    static std::string ParseBundle(napi_env env, napi_value value);
    static State ParseState(napi_env env, napi_value value);
    static Action ParseAction(napi_env env, napi_value value);
    static Mode ParseMode(napi_env env, napi_value value);
    static bool ParseTouch(napi_env env, size_t argc, napi_value *argv, std::shared_ptr<TouchContext> context);
    static int64_t ParseBefore(napi_env env, napi_value value);
    static int64_t ParseAfter(napi_env env, napi_value value, int64_t before);
    bool Equals(napi_env env, napi_value value, napi_ref copy);

    static std::mutex createMutex_;
    static thread_local napi_ref requestCtor;
    static std::mutex requestMutex_;
    static thread_local napi_ref requestFileCtor;
    static std::mutex requestFileMutex_;
    static thread_local napi_ref createCtor;
    std::string tid_;
};
} // namespace OHOS::Request

#endif // REQUEST_TASK_NAPI