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

#ifndef LEGACY_DOWNLOAD_MANAGER_H
#define LEGACY_DOWNLOAD_MANAGER_H

#include <atomic>
#include <functional>
#include <map>
#include <mutex>

#include "legacy/download_task.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "request_common.h"
#include "visibility.h"

namespace OHOS::Request::Legacy {
// Legacy in-process download manager behind the old callback-style
// request.download signature (an options object carrying success/fail/
// complete functions — IsLegacy). One DownloadDescriptor is kept per
// in-flight DownloadTask; the completion callbacks attached via
// OnDownloadComplete fire on the JS thread — OnTaskDone itself runs on
// the download worker thread and only marshals the call through
// CallFunctionAsync (napi_send_event), never calling napi directly.
class RequestManager {
public:
    /// Whether the download call uses the legacy callback-style options
    /// object.
    static bool IsLegacy(napi_env env, napi_callback_info info);

    /// Legacy download entry: parse the options, validate the target
    /// path and filename uniqueness, mint a token, report it synchronously
    /// through the JS success callback, then start the detached curl
    /// task.
    static napi_value Download(napi_env env, napi_callback_info info);

    REQUEST_API static napi_value OnDownloadComplete(napi_env env, napi_callback_info info);

    /// DownloadTask completion hook (worker thread): pops the descriptor
    /// and invokes the success callback with "internal://cache/<file>"
    /// or the fail callback with (message, 400) on the JS thread, then
    /// deletes the task.
    static void OnTaskDone(const std::string &token, bool successful, const std::string &errMsg);

private:
    /// Builds the JS-thread call frame (receiver, argc, argv) of a
    /// deferred callback.
    using ArgsGenerator = std::function<void(napi_env env, napi_ref *recv, int &argc, napi_value *argv)>;

    /// Per-task state: the owning task, the target filename, the env and
    /// the pinned JS references (options object, success/fail callbacks).
    struct DownloadDescriptor {
        DownloadTask *task_{};
        std::string filename_;
        napi_env env_{};
        napi_ref this_{};
        napi_ref successCb_{};
        napi_ref failCb_{};
    };

    /// Heap payload of one deferred JS call, owned by the
    /// napi_send_event lambda.
    struct CallFunctionData {
        napi_env env_{};
        napi_ref func_{};
        ArgsGenerator generator_;
    };

    static std::string GetTaskToken();

    static std::string GetCacheDir(napi_env env);

    static std::string GetFilenameFromUrl(std::string &url);

    static bool IsPathValid(const std::string &dir, const std::string &filename);

    static bool HasSameFilename(const std::string &filename);

    static std::vector<std::string> ParseHeader(napi_env env, napi_value option);

    static DownloadTask::DownloadOption ParseOption(napi_env env, napi_value option);

    static void CallFailCallback(napi_env env, napi_value object, const std::string &msg);

    static void CallSuccessCallback(napi_env env, napi_value object, const std::string &token);

    static void CallFunctionAsync(napi_env env, napi_ref func, const ArgsGenerator &generator);

    static std::atomic<uint32_t> taskId_;
    static std::mutex lock_;
    // In-flight tasks by token (guarded by lock_).
    static std::map<std::string, RequestManager::DownloadDescriptor> downloadDescriptors_;

    // Callback argument counts and the fail-callback error codes: 400
    // download error, 401 unknown token.
    static constexpr int DOWNLOAD_ARGC = 1;
    static constexpr int SUCCESS_CB_ARGC = 1;
    static constexpr int FAIL_CB_ARGC = 2;
    static constexpr int FAIL_CB_DOWNLOAD_ERROR = 400;
    static constexpr int FAIL_CB_TASK_NOT_EXIST = 401;
    static constexpr int MAX_CB_ARGS = 2;

    // Path prefix reported back to JS for the downloaded file.
    static inline const std::string URI_PREFIX = "internal://cache/";
};
} // namespace OHOS::Request::Legacy
#endif // LEGACY_DOWNLOAD_MANAGER_H