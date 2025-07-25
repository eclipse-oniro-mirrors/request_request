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
class CUrlAdp : public std::enable_shared_from_this<CUrlAdp> {
public:
    CUrlAdp(std::vector<FileData> &fileArray, std::shared_ptr<UploadConfig> &config);
    virtual ~CUrlAdp();
    uint32_t DoUpload(std::shared_ptr<IUploadTask> task);
    bool Remove();
    bool IsReadAbort()
    {
        return isReadAbort_;
    }

protected:
    bool ClearCurlResource();
    void SplitHttpMessage(const std::string &stmp, FileData *&fData);
    static int ProgressCallback(
        void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata);
    static size_t ReadCallback(char *buffer, size_t size, size_t nitems, void *arg);
    static void NotifyAPI5(FileData *fData, std::string &headers);
    static bool CheckCUrlAdp(FileData *fData);

private:
    int CheckUploadStatus(CURLM *curlMulti);
    bool MultiAddHandle(CURLM *curlMulti, std::vector<CURL *> &curlArray);
    int32_t UploadOneFile();
    bool IsSuccess(const uint32_t count, const uint32_t size);
    void SetCurlOpt(CURL *curl);
    void SetHeadData(CURL *curl);
    void SetHttpPut(CURL *curl);
    void SetMimePost(CURL *curl);
    void SetSslOpt(CURL *curl);
    void SetConnectionOpt(CURL *curl);
    void SetNetworkOpt(CURL *curl);
    void SetCallbackOpt(CURL *curl);
    void SetBehaviorOpt(CURL *curl);
    std::string ReadCertification();
    void CurlGlobalInit();
    void CurlGlobalCleanup();
    void StartTimer();
    void StopTimer();

private:
    uint32_t timerId_;
    std::shared_ptr<IUploadTask> uploadTask_;
    std::vector<FileData> &fileDatas_;
    FileData mfileData_;
    std::shared_ptr<UploadConfig> config_;
    static constexpr int32_t HTTP_SUCCESS = 200;
    std::mutex mutex_;
    std::mutex curlMutex_;
    std::mutex globalMutex_;
    bool isCurlGlobalInit_;
    CURLM *curlMulti_;
    std::vector<CURL *> curlArray_;
    bool isReadAbort_;
    Utils::Timer timer_;

    static constexpr int TRANS_TIMEOUT_MS = 300 * 1000;
    static constexpr int READFILE_TIMEOUT_MS = 30 * 1000;
    static constexpr int TIMEOUTTYPE = 1;
    static constexpr int FILE_UPLOAD_INTERVAL = 1000;
    static constexpr int COLLECT_DO_FLAG = 1;
    static constexpr int COLLECT_END_FLAG = 2;
};
} // namespace OHOS::Request::Upload
#endif