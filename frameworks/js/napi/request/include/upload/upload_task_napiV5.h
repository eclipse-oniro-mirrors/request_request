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

#ifndef UPLOAD_TASK_NAPIV5_H
#define UPLOAD_TASK_NAPIV5_H

#include <string>
#include <vector>

#include "context.h"
#include "data_ability_helper.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "upload/upload_task.h"
#include "upload_config.h"

namespace OHOS::Request::Upload {
// NAPI face of the old request.upload (API5-9, option-object style):
// pins the JS success/fail/complete callbacks as napi_refs, injects them
// into the UploadConfig as std::function targets, and delivers every
// invocation on the JS thread via napi_send_event — the upload itself
// runs on the worker thread and must not call napi there.
class UploadTaskNapiV5 : public std::enable_shared_from_this<UploadTaskNapiV5> {
public:
    // Heap payload of one fail-callback hop, owned by the event lambda.
    struct SystemFailCallback {
        std::string data;
        int32_t code;
        napi_env env;
        napi_ref ref;
    };

    // Heap payload of one success-callback hop, owned by the event
    // lambda.
    struct SystemSuccessCallback {
        napi_env env;
        napi_ref ref;
        Upload::UploadResponse response;
    };

    // Heap payload of one complete-callback hop; the embedded proxy
    // shared_ptr keeps this object alive until the call has run.
    struct SystemCompleteCallback {
        std::shared_ptr<Upload::UploadTaskNapiV5> proxy;
    };

    // Bundle of the three pinned refs, deleted on the JS thread by the
    // destructor's napi_send_event hop (the destructor may run on any
    // thread).
    struct RecycleRef {
        napi_env env;
        napi_ref successRef;
        napi_ref failRef;
        napi_ref completeRef;
    };

    UploadTaskNapiV5(napi_env env) : env_(env){};
    ~UploadTaskNapiV5();
    /// Bound into UploadConfig::fsuccess; hops to the JS thread and
    /// invokes the pinned success ref with the response object.
    static void OnSystemSuccess(napi_env env, napi_ref ref, Upload::UploadResponse &response);
    /// Bound into UploadConfig::ffail; hops to the JS thread and
    /// invokes the pinned fail ref with (data, code).
    static void OnSystemFail(napi_env env, napi_ref ref, std::string &response, int32_t &code);
    /// Bound into UploadConfig::fcomplete; hops to the JS thread and
    /// invokes the pinned complete ref (API3 only).
    static void OnSystemComplete(std::shared_ptr<Upload::UploadTaskNapiV5> proxy);
    /// JS entry: context + config parse, callback wiring, task creation
    /// and execution. Returns null — results arrive through callbacks.
    napi_value JsUpload(napi_env env, napi_callback_info info);
    /// Pins the success/fail/complete functions of the option object;
    /// true when at least one was found.
    bool ParseCallback(napi_env env, napi_callback_info info);
    /// Binds the static OnSystem* targets into the config.
    void AddCallbackToConfig(napi_env env, std::shared_ptr<UploadConfig> &config);
    inline void SetEnv(napi_env env)
    {
        env_ = env;
    }

private:
    std::shared_ptr<Upload::UploadTask> uploadTask_ = nullptr;
    // Pinned JS callbacks (see ParseCallback).
    napi_ref success_ = nullptr;
    napi_ref fail_ = nullptr;
    napi_ref complete_ = nullptr;
    napi_env env_ = nullptr;
    static bool CreateNapiScope(napi_env env, napi_handle_scope &scope);
    static void CloseNapiScope(napi_env env, napi_handle_scope &scope);
    static bool CallNoParamCallback(napi_env env, napi_ref ref);
    static bool CallSingleParamCallback(napi_env env, napi_ref ref, napi_value param);
    static bool CallDoubleParamCallback(napi_env env, napi_ref ref, napi_value param1, napi_value param2);
    static bool CallCallbackWithParam(napi_env env, napi_ref ref, size_t paramCount, napi_value *params);
    static bool CreateFailJsParams(
        napi_env env, const std::string &data, int32_t code, napi_value &jsData, napi_value &jsCode);
};
} // namespace OHOS::Request::Upload

#endif // UPLOAD_TASK_NAPIV5_H