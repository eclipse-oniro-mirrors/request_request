/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#ifndef REQUEST_ASYNC_CALL_H
#define REQUEST_ASYNC_CALL_H

#include <functional>
#include <memory>

#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "upload/js_util.h"

namespace OHOS::Request::UploadNapi {
// Self-contained AsyncCall copy for the old in-process upload subtree,
// kept beside (not merged with) the root-tree AsyncCall that serves
// request.agent. Same three-stage skeleton (input on the JS thread, exec
// on a worker, output on the JS thread, each action runs once), but with
// upload-specific behavior: an added SyncCall that runs execute +
// complete inline, and a fixed error shape — any failure reports an
// E_SERVICE_ERROR "upload fail" business error, whereas the root tree
// records an inner code and shapes the error per version.
class AsyncCall final {
public:
    // Per-call state: input/output/exec actions are consumed on first
    // use, so a stage cannot run twice.
    class Context {
    public:
        using InputAction = std::function<napi_status(napi_env, size_t, napi_value *, napi_value)>;
        using OutputAction = std::function<napi_status(napi_env, napi_value *)>;
        using ExecAction = std::function<void(Context *)>;
        Context(InputAction input, OutputAction output) : input_(std::move(input)), output_(std::move(output)){};
        virtual ~Context(){};
        void SetAction(InputAction input, OutputAction output = nullptr)
        {
            input_ = input;
            output_ = output;
        }

        void SetAction(OutputAction output)
        {
            SetAction(nullptr, std::move(output));
        }

        /// Input stage (JS thread): runs and clears input_.
        virtual napi_status operator()(napi_env env, size_t argc, napi_value *argv, napi_value self)
        {
            if (input_ == nullptr) {
                return napi_ok;
            }
            napi_status ret = input_(env, argc, argv, self);
            input_ = nullptr;
            return ret;
        }

        /// Output stage (JS thread): runs and clears output_, producing
        /// the result value.
        virtual napi_status operator()(napi_env env, napi_value *result)
        {
            if (output_ == nullptr) {
                *result = nullptr;
                return napi_ok;
            }
            napi_status ret = output_(env, result);
            output_ = nullptr;
            return ret;
        }

        /// Exec stage (worker thread): runs and clears exec_.
        virtual void Exec()
        {
            if (exec_ == nullptr) {
                return;
            }
            exec_(this);
            exec_ = nullptr;
        };

    protected:
        friend class AsyncCall;
        InputAction input_ = nullptr;
        OutputAction output_ = nullptr;
        ExecAction exec_ = nullptr;
    };

    AsyncCall(napi_env env, napi_callback_info info, std::shared_ptr<Context> context);
    ~AsyncCall();
    /// Queues the async work: promise result when no trailing callback
    /// was pinned, undefined + callback invocation otherwise.
    napi_value Call(napi_env env, Context::ExecAction exec = nullptr);
    /// Runs exec and the completion handling inline on the calling
    /// thread, without creating async work.
    napi_value SyncCall(napi_env env, Context::ExecAction exec = nullptr);

private:
    // Callback layout of OnComplete: [error, data].
    enum { ARG_ERROR, ARG_DATA, ARG_BUTT };
    static void OnExecute(napi_env env, void *data);
    static void OnComplete(napi_env env, napi_status status, void *data);
    // One async call: the shared Context, the pinned trailing callback
    // and this-argument references, plus the promise/async-work handles.
    // Freed by DeleteContext after completion (or in ~AsyncCall when
    // Call/SyncCall never ran).
    struct AsyncContext {
        std::shared_ptr<Context> ctx = nullptr;
        napi_ref callback = nullptr;
        napi_ref self = nullptr;
        napi_deferred defer = nullptr;
        napi_async_work work = nullptr;
    };
    static void DeleteContext(napi_env env, AsyncContext *context);

    AsyncContext *context_ = nullptr;
    napi_env env_ = nullptr;
};
} // namespace OHOS::Request::UploadNapi

#endif // REQUEST_ASYNC_CALL_H
