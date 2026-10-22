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

#ifndef LEGACY_DOWNLOAD_TASK_H
#define LEGACY_DOWNLOAD_TASK_H

#include <cstdio>
#include <functional>
#include <thread>
#include <vector>

#include "curl/curl.h"

namespace OHOS::Request::Legacy {
// In-process curl download behind the legacy signature of
// request.download (Legacy::RequestManager::Download) — no server SA
// involved: one task runs on a detached thread driving a libcurl easy
// handle. Resumes from the already-downloaded prefix when the local file
// is shorter than the remote size, retries up to 10 times, then reports
// through the DoneFunc callback.
class DownloadTask {
public:
    /// One download request: url, target file name and directory, extra
    /// header lines.
    struct DownloadOption {
        std::string url_;
        std::string filename_;
        std::string fileDir_;
        std::vector<std::string> header_;
    };

    /// Completion callback: (taskId, successful, error message).
    using DoneFunc = std::function<void(const std::string &, bool, const std::string &)>;
    DownloadTask(const std::string &token, const DownloadOption &option, const DoneFunc &callback);

    ~DownloadTask();

    /// Spawns the detached worker thread (after a one-time
    /// curl_global_init).
    void Start();

    /// Worker body: retry loop around size-probe + download.
    void Run();

    /// One download attempt: resume from the local prefix (or
    /// short-circuit when already complete), then perform.
    bool DoDownload();

    /// Sets the byte offset the transfer resumes from.
    void SetResumeFromLarge(CURL *curl, uint64_t pos);

private:
    /// Opens/creates the target file (O_NOFOLLOW, truncated).
    FILE *OpenDownloadFile() const;

    /// Size of the already-downloaded prefix (0 when the file is new).
    uint32_t GetLocalFileSize();

    /// Wires the write file, error buffer, url, TLS settings, low-speed
    /// abort thresholds and the extra headers.
    bool SetOption(CURL *handle, curl_slist *&headers);

    /// Closes the file (removing it on failure) and reports via the
    /// callback.
    void NotifyDone(bool successful, const std::string &errMsg = "");

    /// One-shot HEAD request for the remote size; an unknown size is
    /// treated as 0.
    bool GetFileSize(uint32_t &result);

    std::string taskId_;
    DownloadOption option_;
    DoneFunc callback_;
    std::thread *thread_{};
    FILE *filp_{};
    char *errorBuffer_{};
    /// curl_global_init ran once in this process.
    static bool isCurlGlobalInited_;

    uint32_t totalSize_;
    bool hasFileSize_;
};
} // namespace OHOS::Request::Legacy
#endif // LEGACY_DOWNLOAD_TASK_H