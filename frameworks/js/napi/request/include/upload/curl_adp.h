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

#ifndef CURLADP_H
#define CURLADP_H

#include <mutex>
#include <vector>

#include "curl/curl.h"
#include "curl/easy.h"
#include "i_upload_task.h"
#include "timer.h"
#include "upload/upload_common.h"
#include "upload_config.h"

namespace OHOS::Request::Upload {
// libcurl adapter of the old in-process upload: uploads the files of one
// task one after another, each through its own easy handle driven by a
// multi handle, and wires the header/progress/read callbacks onto the
// per-file FileData. The task (IUploadTask) is held only to keep the
// adapter's lifetime tied to it. Remove() flips an abort flag that every
// callback checks.
class CUrlAdp : public std::enable_shared_from_this<CUrlAdp> {
public:
    /// fileDatas is the task's per-file array (kept by reference);
    /// config is the shared parsed UploadConfig.
    CUrlAdp(std::vector<FileData> &fileArray, std::shared_ptr<UploadConfig> &config);
    virtual ~CUrlAdp();
    /// Sequential upload of every file; per-file results are stored back
    /// into fileDatas. Returns UPLOAD_OK only when all files succeeded.
    uint32_t DoUpload(std::shared_ptr<IUploadTask> task);
    /// Flips the abort flag: the in-flight read aborts
    /// (CURL_READFUNC_ABORT) and the remaining files are marked
    /// UPLOAD_TASK_REMOVED.
    bool Remove();
    bool IsReadAbort()
    {
        return isReadAbort_;
    }

protected:
    /// Frees this file's easy handle(s) and the multi handle.
    bool ClearCurlResource();
    /// Parses one raw response header line: the status line sets
    /// httpCode and starts collection, the terminating empty line ends
    /// it.
    void SplitHttpMessage(const std::string &stmp, FileData *&fData);
    /// XFERINFO callback: maintains upsize (writing it back into the
    /// matching fileDatas_ entry) and aborts the transfer on Remove.
    static int ProgressCallback(
        void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    /// Collects response headers into responseHead and, at the end of a
    /// header block, notifies the API5 callbacks.
    static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata);
    /// Feeds file bytes to the transfer; the 30 s read watchdog runs
    /// around each fread and aborts on timeout or Remove.
    static size_t ReadCallback(char *buffer, size_t size, size_t nitems, void *arg);
    /// End of one header block: a 200 on the last file fires the success
    /// callback, any other code fires the fail callback.
    static void NotifyAPI5(FileData *fData, std::string &headers);
    /// Shared precondition of every curl callback: the FileData and its
    /// adapter must be alive and not removed.
    static bool CheckCUrlAdp(FileData *fData);

private:
    /// Reads the transfer result: only HTTP 200 counts as success; curl
    /// errors map to the upload error codes.
    int CheckUploadStatus(CURLM *curlMulti);
    bool MultiAddHandle(CURLM *curlMulti, std::vector<CURL *> &curlArray);
    /// One file's transfer: creates the multi handle, adds the easy
    /// handle and pumps curl_multi_wait until the transfer finishes.
    int32_t UploadOneFile();
    bool IsSuccess(const uint32_t count, const uint32_t size);
    // Option wiring, applied to every easy handle in this order.
    void SetCurlOpt(CURL *curl);
    /// Config headers as a curl_slist (stored in mfileData_.list); adds
    /// a default Content-Type when the config has none —
    /// application/octet-stream for PUT, multipart/form-data otherwise.
    void SetHeadData(CURL *curl);
    void SetHttpPut(CURL *curl);
    /// multipart/form-data body: one part per config data field plus a
    /// single "file" part fed by ReadCallback.
    void SetMimePost(CURL *curl);
    /// https: CA bundle from /etc/ssl/certs/cacert.pem and the TLS
    /// version taken from the X-TLS-Version header; non-https URLs get
    /// certificate verification disabled.
    void SetSslOpt(CURL *curl);
    void SetConnectionOpt(CURL *curl);
    void SetNetworkOpt(CURL *curl);
    void SetCallbackOpt(CURL *curl);
    void SetBehaviorOpt(CURL *curl);
    std::string ReadCertification();
    void CurlGlobalInit();
    void CurlGlobalCleanup();
    /// Registers the one-shot read watchdog (READFILE_TIMEOUT_MS).
    void StartTimer();
    void StopTimer();

private:
    uint32_t timerId_;
    std::shared_ptr<IUploadTask> uploadTask_;
    std::vector<FileData> &fileDatas_;
    // Working copy of the file currently being uploaded; the curl
    // callbacks receive its address and its adp member holds a
    // shared_ptr back to this adapter.
    FileData mfileData_;
    std::shared_ptr<UploadConfig> config_;
    static constexpr int32_t HTTP_SUCCESS = 200;
    std::mutex mutex_;
    // Held by every curl callback while touching adapter state.
    std::mutex curlMutex_;
    std::mutex globalMutex_;
    bool isCurlGlobalInit_;
    CURLM *curlMulti_;
    std::vector<CURL *> curlArray_;
    // Set by Remove() and the read watchdog; checked by every callback.
    bool isReadAbort_;
    Utils::Timer timer_;

    static constexpr int TRANS_TIMEOUT_MS = 300 * 1000;
    static constexpr int READFILE_TIMEOUT_MS = 30 * 1000;
    static constexpr int TIMEOUTTYPE = 1;
    // Polling interval for usleep during upload, in microseconds (1000 = 1 ms).
    static constexpr int FILE_UPLOAD_INTERVAL = 1000;
    static constexpr int COLLECT_DO_FLAG = 1;
    // Header-collection state: DO while lines accumulate, END once the
    // terminating empty line has been seen.
    static constexpr int COLLECT_END_FLAG = 2;
};
} // namespace OHOS::Request::Upload
#endif