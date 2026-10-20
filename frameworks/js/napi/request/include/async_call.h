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
#ifndef REQUEST_ASYNC_CALL_H
#define REQUEST_ASYNC_CALL_H

#include <functional>
#include <memory>
#include <new>
#include <string>

#include "constant.h"
#include "log.h"
#include "napi/native_node_api.h"
#include "napi_utils.h"
#include "request_common.h"

namespace OHOS::Request {
/// Three-stage skeleton for async NAPI entry functions.
///
/// A call site derives a Context, sets three actions on it and hands it to
/// AsyncCall:
/// 1. InputAction  — parse/validate argv on the JS thread, inside the
///    AsyncCall constructor, before anything is queued;
/// 2. ExecAction   — the business work, on a worker thread (OnExecute);
/// 3. OutputAction — build the JS result value, back on the JS thread
///    (OnComplete).
/// The result is delivered as a promise when the JS call had no trailing
/// callback, or as callback(err, data) when it did. JsTask::ContextInfo,
/// JsTask::TouchContext and RequestEvent::ExecContext are typical
/// subclasses of Context.
class AsyncCall final {
public:
    /// Per-call state shared by all three stages; owns the napi handles and
    /// the business error code. Kept alive by shared_ptr until OnComplete
    /// (or a Call() failure) drops it — possibly on a worker thread, hence
    /// the JS-thread cleanup scheduled in the destructor.
    class Context {
    public:
        // The three stage actions. InputAction receives the argc/argv left
        // after an optional trailing callback was peeled off; OutputAction
        // fills the data slot of the result.
        using InputAction = std::function<napi_status(size_t, napi_value *, napi_value)>;
        using OutputAction = std::function<napi_status(napi_value *)>;
        using ExecAction = std::function<void()>;
        Context() = default;
        /// Schedules the napi handle cleanup (async work, self and callback
        /// references) on the JS thread via napi_send_event — the last
        /// shared_ptr may drop on a worker thread, where napi_delete_*
        /// must not run.
        virtual ~Context()
        {
            ContextNapiHolder *holder = new (std::nothrow)
                ContextNapiHolder{ .env = env_, .callbackRef = callbackRef_, .self = self_, .work = work_ };
            if (holder == nullptr) {
                REQUEST_HILOGE("new ContextNapiHolder null");
                return;
            }
            // Deletes reference of callback in JS main thread.
            auto afterCallback = [holder]() {
                napi_handle_scope scope = nullptr;
                napi_status status = napi_open_handle_scope(holder->env, &scope);
                if (status != napi_ok || scope == nullptr) {
                    delete holder;
                    return;
                } else if (holder->env == nullptr || holder->work == nullptr || holder->self == nullptr) {
                    napi_close_handle_scope(holder->env, scope);
                    delete holder;
                    return;
                }
                napi_delete_async_work(holder->env, holder->work);
                napi_delete_reference(holder->env, holder->self);
                if (holder->callbackRef != nullptr) {
                    napi_delete_reference(holder->env, holder->callbackRef);
                }
                napi_close_handle_scope(holder->env, scope);
                delete holder;
            };
            int32_t ret = napi_send_event(env_, afterCallback, napi_eprio_high,
                "request:download|upload|agent.create|agent.getTask");
            if (ret != napi_ok) {
                REQUEST_HILOGE("napi_send_event failed: %{public}d", ret);
                delete holder;
            }
        };
        inline Context &SetInput(InputAction action)
        {
            input_ = std::move(action);
            return *this;
        }
        inline Context &SetOutput(OutputAction action)
        {
            output_ = std::move(action);
            return *this;
        }
        inline Context &SetExec(ExecAction action)
        {
            exec_ = std::move(action);
            return *this;
        }
        /// Builds the JS business error from innerCode_ (produced by exec).
        inline napi_value CreateErr()
        {
            ExceptionError error;
            NapiUtils::ConvertError(innerCode_, error);
            return NapiUtils::CreateBusinessError(env_, error.code, error.errInfo, withErrCode_);
        }

        InputAction input_ = nullptr;
        OutputAction output_ = nullptr;
        ExecAction exec_ = nullptr;

        // Handles owned for the duration of the call: callbackRef_ is null
        // in promise mode, defer_ is null in callback mode; innerCode_ is
        // the business error code exec produced; withErrCode_ controls
        // whether the business error carries an errCode field; version_
        // selects the API9/API10 error shape in OnComplete.
        napi_env env_;
        napi_ref callbackRef_ = nullptr;
        napi_ref self_ = nullptr;
        napi_deferred defer_ = nullptr;
        napi_async_work work_ = nullptr;

        int32_t innerCode_;
        bool withErrCode_;
        Version version_;
    };

    /// Extracts the call arguments and runs the input stage (see the .cpp);
    /// may leave the context carrying only an error for Call().
    AsyncCall(napi_env env, napi_callback_info info, const std::shared_ptr<Context> &context);
    ~AsyncCall();
    /// Queues the work and returns the JS-side value: a promise when the
    /// call had no callback, undefined otherwise. resourceName only feeds
    /// the async work name ("REQUEST_" + name).
    napi_value Call(const std::shared_ptr<Context> &context, const std::string &resourceName = "AsyncCall");

    /// QoS level for queueing the async work (default napi_qos_default).
    inline void SetQosLevel(napi_qos_t napiQosLevel)
    {
        napiQosLevel_ = napiQosLevel;
    }

private:
    // Result layout shared by promise and callback delivery: (error, data).
    enum { ARG_ERROR, ARG_DATA, ARG_BUTT };

    /// Heap carrier for the napi_async_work userdata; freed in OnComplete
    /// or on the Call() failure paths.
    struct WorkData {
        std::shared_ptr<Context> ctx = nullptr;
        ~WorkData()
        {
            ctx = nullptr;
        }
    };

    /// Snapshot of the handles the destructor's JS-thread cleanup needs;
    /// copied out because the Context itself may already be gone by then.
    struct ContextNapiHolder {
        napi_env env;
        napi_ref callbackRef;
        napi_ref self;
        napi_async_work work;
    };
    /// Worker-thread stage: runs exec_ once and clears it.
    static void OnExecute(napi_env env, void *data);
    /// JS-thread completion stage: output_ fills the data slot; the error
    /// slot starts as null (API10) or undefined (API9) and is replaced by
    /// CreateErr() when the work or the output failed.
    static void OnComplete(napi_env env, napi_status status, void *data);
    napi_qos_t napiQosLevel_ = napi_qos_default;
};
} // namespace OHOS::Request
#endif // REQUEST_ASYNC_CALL_H
