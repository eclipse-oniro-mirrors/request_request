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

#include "request_preload.h"

#include <cstdint>
#include <memory>

#include "cxx.h"
#include "wrapper.rs.h"

namespace OHOS::Request {
Data::Data(rust::Box<RustData> &&data)
{
    data_ = data.into_raw();
}

Data::~Data()
{
    rust::Box<RustData>::from_raw(data_);
}

Data::Data(Data &&other) noexcept : data_(other.data_)
{
    other.data_ = nullptr;
}

Data &Data::operator=(Data &&other) &noexcept
{
    if (data_) {
        rust::Box<RustData>::from_raw(data_);
    }
    data_ = other.data_;
    other.data_ = nullptr;
    return *this;
}

template<typename T> Slice<T>::Slice(std::unique_ptr<rust::Slice<T>> &&slice) : slice_(std::move(slice))
{
}

template<typename T> Slice<T>::~Slice()
{
}

template<typename T> T *Slice<T>::data() const noexcept
{
    return slice_->data();
}

template<typename T> std::size_t Slice<T>::size() const noexcept
{
    return slice_->size();
}

template<typename T> std::size_t Slice<T>::length() const noexcept
{
    return slice_->length();
}

template<typename T> bool Slice<T>::empty() const noexcept
{
    return slice_->empty();
}

template<typename T> T &Slice<T>::operator[](std::size_t n) const noexcept
{
    return (*slice_)[n];
}

Slice<const uint8_t> Data::bytes() const
{
    auto bytes = std::make_unique<rust::Slice<const uint8_t>>(data_->bytes());
    return Slice<const uint8_t>(std::move(bytes));
}

rust::Slice<const uint8_t> Data::rustSlice() const
{
    return data_->bytes();
}

PreloadError::PreloadError(rust::Box<CacheDownloadError> &&error)
{
    error_ = error.into_raw();
}

PreloadError::PreloadError(PreloadError &&other) noexcept : error_(other.error_)
{
    other.error_ = nullptr;
}

PreloadError &PreloadError::operator=(PreloadError &&other) &noexcept
{
    if (error_) {
        rust::Box<CacheDownloadError>::from_raw(error_);
    }
    error_ = other.error_;
    other.error_ = nullptr;
    return *this;
}

PreloadError::~PreloadError()
{
    rust::Box<CacheDownloadError>::from_raw(error_);
}

int32_t PreloadError::GetCode() const
{
    return error_->code();
}

std::string PreloadError::GetMessage() const
{
    return std::string(error_->message());
}

ErrorKind PreloadError::GetErrorKind() const
{
    return static_cast<ErrorKind>(error_->ffi_kind());
}

Preload::Preload()
{
    agent_ = cache_download_service();
}

PreloadHandle::PreloadHandle(PreloadHandle &&other) noexcept : handle_(other.handle_)
{
    other.handle_ = nullptr;
}

PreloadHandle &PreloadHandle::operator=(PreloadHandle &&other) &noexcept
{
    if (handle_) {
        rust::Box<TaskHandle>::from_raw(handle_);
    }
    handle_ = other.handle_;
    other.handle_ = nullptr;
    return *this;
}

std::shared_ptr<PreloadHandle> Preload::load(std::string const &url, std::unique_ptr<PreloadCallback> callback,
    std::unique_ptr<PreloadOptions> options, bool update)
{
    auto callback_wrapper = std::make_unique<PreloadCallbackWrapper>(callback);

    std::shared_ptr<PreloadProgressCallbackWrapper> progress_callback_wrapper = nullptr;
    if (callback != nullptr && callback->OnProgress != nullptr) {
        progress_callback_wrapper = std::make_shared<PreloadProgressCallbackWrapper>(callback);
    }

    FfiPredownloadOptions ffiOptions = { .headers = rust::Vec<rust::str>() };
    if (options != nullptr) {
        for (auto header : options->headers) {
            ffiOptions.headers.push_back(std::get<0>(header));
            ffiOptions.headers.push_back(std::get<1>(header));
        }
    }
    auto taskHandle = agent_->ffi_preload(
        rust::str(url), std::move(callback_wrapper), std::move(progress_callback_wrapper), update, ffiOptions);
    return std::make_shared<PreloadHandle>(std::move(taskHandle));
}

std::optional<Data> Preload::fetch(std::string const &url)
{
    std::unique_ptr<Data> data = agent_->ffi_fetch(rust::str(url));
    if (data == nullptr) {
        return std::nullopt;
    }
    return std::move(*data);
}

void Preload::SetRamCacheSize(uint64_t size)
{
    agent_->set_ram_cache_size(size);
}
void Preload::SetFileCacheSize(uint64_t size)
{
    agent_->set_file_cache_size(size);
}

void Preload::Cancel(std::string const &url)
{
    agent_->cancel(rust::str(url));
}

void Preload::Remove(std::string const &url)
{
    agent_->remove(rust::str(url));
}

bool Preload::Contains(const std::string &url)
{
    return agent_->contains(rust::str(url));
}

Preload *Preload::GetInstance()
{
    static Preload agent;
    return &agent;
}

PreloadHandle::PreloadHandle(rust::Box<TaskHandle> handle)
{
    handle_ = handle.into_raw();
}

PreloadHandle::~PreloadHandle()
{
    rust::Box<TaskHandle>::from_raw(handle_);
}

void PreloadHandle::Cancel()
{
    handle_->cancel();
}

std::string PreloadHandle::GetTaskId()
{
    return std::string(handle_->task_id());
}

bool PreloadHandle::IsFinish()
{
    return handle_->is_finish();
}

PreloadState PreloadHandle::GetState()
{
    return static_cast<PreloadState>(handle_->state());
}
template class Slice<const uint8_t>;

} // namespace OHOS::Request