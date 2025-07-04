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

#include "legacy/download_task.h"

#include <pthread.h>

#include "constant.h"
#include "log.h"

namespace OHOS::Request::Legacy {
bool DownloadTask::isCurlGlobalInited_ = false;
const uint32_t DEFAULT_READ_TIMEOUT = 60;
const uint32_t DEFAULT_LOW_SPEED_LIMIT = 30;
constexpr uint32_t RETRY_TIME = 10;
DownloadTask::DownloadTask(const std::string &token, const DownloadOption &option, const DoneFunc &callback)
    : taskId_(token), option_(option), callback_(callback), totalSize_(0), hasFileSize_(false)
{
    REQUEST_HILOGI("constructor");
}

DownloadTask::~DownloadTask()
{
    REQUEST_HILOGI("destroy");
    if (filp_ != nullptr) {
        fclose(filp_);
    }
    delete[] errorBuffer_;
    delete thread_;
}

FILE *DownloadTask::OpenDownloadFile() const
{
    auto downloadFile = option_.fileDir_ + '/' + option_.filename_;
    FILE *filp = fopen(downloadFile.c_str(), "w+");
    if (filp == nullptr) {
        REQUEST_HILOGE("open download file failed");
    }
    return filp;
}

uint32_t DownloadTask::GetLocalFileSize()
{
    if (filp_ == nullptr) {
        filp_ = OpenDownloadFile();
        if (filp_ == nullptr) {
            return 0;
        }
    }

    int nRet = fseek(filp_, 0, SEEK_END);
    if (nRet != 0) {
        REQUEST_HILOGE("fseek error");
        return 0;
    }
    long lRet = ftell(filp_);
    if (lRet < 0) {
        REQUEST_HILOGE("ftell error");
        return 0;
    }
    return static_cast<uint32_t>(lRet);
}
void DownloadTask::NotifyDone(bool successful, const std::string &errMsg)
{
    if (filp_ != nullptr) {
        fclose(filp_);
        filp_ = nullptr;

        if (!successful) {
            REQUEST_HILOGE("remove download file");
            remove((option_.fileDir_ + '/' + option_.filename_).c_str());
        }
    }

    if (callback_) {
        callback_(taskId_, successful, errMsg);
    }
}

bool DownloadTask::GetFileSize(uint32_t &result)
{
    if (hasFileSize_) {
        REQUEST_HILOGD("Already get file size");
        return true;
    }
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle(curl_easy_init(), curl_easy_cleanup);

    if (!handle) {
        REQUEST_HILOGD("Failed to create download service task");
        return false;
    }

    curl_easy_setopt(handle.get(), CURLOPT_URL, option_.url_.c_str());
    curl_easy_setopt(handle.get(), CURLOPT_HEADER, 1L);
    curl_easy_setopt(handle.get(), CURLOPT_NOBODY, 1L);
    CURLcode code = curl_easy_perform(handle.get());
    double size = 0.0;
    curl_easy_getinfo(handle.get(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);

    if (code == CURLE_OK) {
        if (size > UINT_MAX) {
            REQUEST_HILOGD("file size overflow");
            return false;
        }
        result = static_cast<uint32_t>(size);
        if (result == static_cast<uint32_t>(-1)) {
            result = 0;
        }
        hasFileSize_ = true;
        REQUEST_HILOGD("Has got file size");
    }
    REQUEST_HILOGD("fetch file size %{public}d", result);
    return hasFileSize_;
}

bool DownloadTask::SetOption(CURL *handle, curl_slist *&headers)
{
    filp_ = OpenDownloadFile();
    if (filp_ == nullptr) {
        return false;
    }
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, filp_);

    errorBuffer_ = new (std::nothrow) char[CURL_ERROR_SIZE];
    if (errorBuffer_ == nullptr) {
        return false;
    }
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errorBuffer_);

    curl_easy_setopt(handle, CURLOPT_URL, option_.url_.c_str());
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(handle, CURLOPT_LOW_SPEED_TIME, DEFAULT_READ_TIMEOUT);
    curl_easy_setopt(handle, CURLOPT_LOW_SPEED_LIMIT, DEFAULT_LOW_SPEED_LIMIT);

    if (!option_.header_.empty()) {
        for (const auto &head : option_.header_) {
            headers = curl_slist_append(headers, head.c_str());
        }
        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
    }
    return true;
}

void DownloadTask::Start()
{
    if (!isCurlGlobalInited_) {
        curl_global_init(CURL_GLOBAL_ALL);
        isCurlGlobalInited_ = true;
    }

    thread_ = new (std::nothrow) std::thread(&DownloadTask::Run, this);
    if (thread_ == nullptr) {
        NotifyDone(false, "create download thread failed");
        return;
    }
    thread_->detach();
}

void DownloadTask::Run()
{
    REQUEST_HILOGD("start download task");
    pthread_setname_np(pthread_self(), "system_download");
    uint32_t retryTime = 0;
    bool result = false;
    do {
        if (GetFileSize(totalSize_)) {
            result = DoDownload();
        }
        retryTime++;
        REQUEST_HILOGD("download task retrytime: %{public}u, totalSize_: %{public}u", retryTime, totalSize_);
    } while (!result && retryTime < RETRY_TIME);

    if (retryTime >= RETRY_TIME) {
        NotifyDone(false, "Network failed");
    }
}

bool DownloadTask::DoDownload()
{
    REQUEST_HILOGD("download task DoDownload");
    curl_slist *headers{};
    std::shared_ptr<CURL> handle(curl_easy_init(), [headers](CURL *handle) {
        if (headers) {
            curl_slist_free_all(headers);
        }
        curl_easy_cleanup(handle);
    });

    if (handle == nullptr) {
        NotifyDone(false, "curl failed");
        REQUEST_HILOGD("curl failed");
        return false;
    }

    if (!SetOption(handle.get(), headers)) {
        REQUEST_HILOGD("curl set option failed");
        return false;
    }
    uint32_t localFileLength = GetLocalFileSize();
    if (localFileLength > 0) {
        if (localFileLength < totalSize_) {
            SetResumeFromLarge(handle.get(), localFileLength);
        } else {
            NotifyDone(true, "Download task has already completed");
            return true;
        }
    }

    auto code = curl_easy_perform(handle.get());
    REQUEST_HILOGI("code=%{public}d, %{public}s", code, errorBuffer_);
    if (code == CURLE_OK) {
        NotifyDone(code == CURLE_OK, errorBuffer_);
    }
    return code == CURLE_OK;
}

void DownloadTask::SetResumeFromLarge(CURL *curl, uint64_t pos)
{
    curl_easy_setopt(curl, CURLOPT_RESUME_FROM_LARGE, pos);
}
} // namespace OHOS::Request::Legacy