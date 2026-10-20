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

#ifndef UPLOAD_TASK_
#define UPLOAD_TASK_

#include <pthread.h>

#include <cstdio>
#include <thread>
#include <vector>

#include "ability_context.h"
#include "context.h"
#include "curl/curl.h"
#include "curl/easy.h"
#include "data_ability_helper.h"
#include "i_upload_task.h"
#include "upload/curl_adp.h"
#include "upload/obtain_file.h"
#include "upload/upload_common.h"
#include "upload/upload_hilog_wrapper.h"
#include "upload_config.h"

namespace OHOS::Request::Upload {
// Task lifecycle: INIT until the worker starts, RUNNING during the
// transfer, then SUCCESS or FAILURE (a removed task stays in its last
// state).
enum UploadTaskState {
    STATE_INIT,
    STATE_RUNNING,
    STATE_SUCCESS,
    STATE_FAILURE,
};
class UploadTaskNapiV5;
// One old-style in-process upload task: owns the parsed config, opens
// the source files (ObtainFile, by URI scheme) into FileData entries and
// runs the whole batch through a CUrlAdp on a detached thread. Remove()
// aborts the adapter and closes the files; the JS callbacks live in the
// config and are marshalled to the JS thread by the adapter/NAPI proxy.
class UploadTask
    : public IUploadTask
    , public std::enable_shared_from_this<UploadTask> {
public:
    UPLOAD_API UploadTask(std::shared_ptr<UploadConfig> &uploadConfig);
    /// Aborts the adapter and closes all source files if not already
    /// removed (the destructor also calls this).
    UPLOAD_API ~UploadTask();
    UPLOAD_API bool Remove();
    /// Spawns the detached worker thread (Run).
    UPLOAD_API void ExecuteTask();
    /// Worker body: runs the upload, fires the API3 complete callback,
    /// then releases the NAPI proxy.
    static void Run(std::shared_ptr<Upload::UploadTask> task);
    /// State-machine core of the worker: RUNNING, transfer, terminal
    /// state, file cleanup.
    void OnRun();

    UPLOAD_API void SetCallback(Type type, void *callback);
    /// The ability context used to resolve file URIs.
    UPLOAD_API void SetContext(std::shared_ptr<OHOS::AbilityRuntime::Context> context);
    /// The NAPI proxy; held only until Run finishes, then released.
    UPLOAD_API void SetUploadProxy(std::shared_ptr<UploadTaskNapiV5> proxy);

protected:
    /// Opens every config file into fileDatas_ (1-based fileIndex — the
    /// last file's index equals the count, which the adapter's success
    /// gate relies on) and sums totalSize_.
    uint32_t InitFileArray();
    /// Closes the opened source files and drops the array.
    void ClearFileArray();

private:
    void OnFail();
    void OnComplete();
    void ReportTaskFault(uint32_t ret) const;
    /// Removed-check, file array init, then the adapter's DoUpload.
    uint32_t StartUploadFile();

    std::shared_ptr<UploadConfig> uploadConfig_;
    std::unique_ptr<std::thread> thread_;
    std::shared_ptr<CUrlAdp> curlAdp_;
    std::shared_ptr<UploadTaskNapiV5> uploadProxy_;
    std::shared_ptr<OHOS::AbilityRuntime::Context> context_;
    int64_t uploadedSize_;
    // Total bytes across all files (summed by InitFileArray).
    int64_t totalSize_;
    std::vector<std::string> headerArray_;
    std::string header_;
    // Per-file runtime state handed to the adapter by reference.
    std::vector<FileData> fileDatas_;
    UploadTaskState state_;
    std::mutex mutex_;
    bool isRemoved_{ false };
    // Serializes Remove() against the worker's teardown.
    std::mutex removeMutex_;
    std::thread::native_handle_type thread_handle_;
    // Delay before the worker starts (50 ms), giving the JS caller time
    // to finish wiring callbacks.
    static constexpr int USLEEP_INTERVAL_BEFORE_RUN = 50 * 1000;
};
} // namespace OHOS::Request::Upload
#endif