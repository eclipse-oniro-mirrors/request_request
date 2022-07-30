/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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

#include "download_config.h"
#include "constant.h"
#include <algorithm>
#include <string>
#include <utility>
#include "log.h"

namespace OHOS::Request::Download {
DownloadConfig::DownloadConfig()
    : url_(""), enableMetered_(false), enableRoaming_(false), description_(""), networkType_(0),
      filePath_(""), title_(""), fd_(-1), fdError_(0) {
}

void DownloadConfig::SetUrl(const std::string &url)
{
    url_ = url;
}

void DownloadConfig::SetHeader(const std::string &key, const std::string &val)
{
    header_[key] = val;
}

void DownloadConfig::SetMetered(bool enableMetered)
{
    enableMetered_ = enableMetered;
}

void DownloadConfig::SetRoaming(bool enableRoaming)
{
    enableRoaming_ = enableRoaming;
}

void DownloadConfig::SetDescription(const std::string &description)
{
    description_ = description;
}

void DownloadConfig::SetNetworkType(uint32_t type)
{
    networkType_ = type;
}

void DownloadConfig::SetFilePath(const std::string &filePath)
{
    filePath_ = filePath;
}

void DownloadConfig::SetTitle(const std::string &title)
{
    title_ = title;
}

void DownloadConfig::SetFD(int32_t fd)
{
    fd_ = fd;
}

void DownloadConfig::SetFDError(int32_t fdError)
{
    fdError_ = fdError;
}

const std::string &DownloadConfig::GetUrl() const
{
    return url_;
}

const std::map<std::string, std::string> &DownloadConfig::GetHeader() const
{
    return header_;
}

bool DownloadConfig::GetMetered() const
{
    return enableMetered_;
}

bool DownloadConfig::GetRoaming() const
{
    return enableRoaming_;
}

const std::string &DownloadConfig::GetDescription() const
{
    return description_;
}

uint32_t DownloadConfig::GetNetworkType() const
{
    return networkType_;
}

const std::string &DownloadConfig::GetFilePath() const
{
    return filePath_;
}

const std::string &DownloadConfig::GetTitle() const
{
    return title_;
}

int32_t DownloadConfig::GetFD() const
{
    return fd_;
}

int32_t DownloadConfig::GetFDError() const
{
    return fdError_;
}

void DownloadConfig::Dump(bool isFull) const
{
    DOWNLOAD_HILOGD("fd: %{public}d", fd_);
    DOWNLOAD_HILOGD("fd errno: %{public}d", fdError_);
    DOWNLOAD_HILOGD("URL: %{public}s", url_.c_str());
    DOWNLOAD_HILOGD("enableMetered: %{public}s", enableMetered_ ? "true" : "false");
    DOWNLOAD_HILOGD("enableRoaming: %{public}s", enableRoaming_ ? "true" : "false");
    DOWNLOAD_HILOGD("description: %{public}s", description_.c_str());
    std::string networkDesc = "WLAN and Mobile";
    if ((networkType_ & NETWORK_MASK) == NETWORK_MOBILE) {
        networkDesc = "Mobile";
    } else if ((networkType_ & NETWORK_MASK) == NETWORK_WIFI) {
        networkDesc = "WLAN";
    }
    DOWNLOAD_HILOGD("networkType: %{public}s", networkDesc.c_str());
    DOWNLOAD_HILOGD("filePath: %{public}s", filePath_.c_str());
    DOWNLOAD_HILOGD("title: %{public}s", title_.c_str());
    if (isFull) {
        DOWNLOAD_HILOGD("Header Information:");
        std::for_each(header_.begin(), header_.end(), [](std::pair<std::string, std::string> p) {
            DOWNLOAD_HILOGD("%{public}s : %{public}s", p.first.c_str(), p.second.c_str());
        });
        DOWNLOAD_HILOGD("Header Information -------------- End");
    }
}
} // namespace OHOS::Request::Download
