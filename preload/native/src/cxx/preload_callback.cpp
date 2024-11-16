/*
* Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "preload_callback.h"

#include <memory>

#include "cxx.h"
#include "request_preload.h"
namespace OHOS::Request {

PreloadCallbackWrapper::PreloadCallbackWrapper(std::unique_ptr<PreloadCallback> callback)
{
    if (callback != nullptr) {
        this->_callback = std::move(callback);
    }
}

void PreloadCallbackWrapper::OnSuccess(const std::shared_ptr<Data> data, rust::str taskId) const
{
    if (this->_callback != nullptr && this->_callback->OnSuccess != nullptr) {
        this->_callback->OnSuccess(std::move(data), std::string(taskId));
    }
}

void PreloadCallbackWrapper::OnFail(rust::Box<DownloadError> error, rust::str taskId) const
{
    if (this->_callback != nullptr && this->_callback->OnFail != nullptr) {
        PreloadError preloadError(std::move(error));
        this->_callback->OnFail(preloadError, std::string(taskId));
    }
}

void PreloadCallbackWrapper::OnCancel() const
{
    if (this->_callback != nullptr && this->_callback->OnCancel != nullptr) {
        this->_callback->OnCancel();
    }
}

void PreloadCallbackWrapper::OnProgress(uint64_t current, uint64_t total) const
{
    if (this->_callback != nullptr && this->_callback->OnProgress != nullptr) {
        this->_callback->OnProgress(current, total);
    }
}

std::shared_ptr<Data> BuildSharedData(rust::Box<RustData> data)
{
    return std::make_shared<Data>(std::move(data));
}

} // namespace OHOS::Request