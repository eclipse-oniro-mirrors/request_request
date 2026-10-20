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

#ifndef REQUEST_PRE_DOWNLOAD_CALLBACK_H
#define REQUEST_PRE_DOWNLOAD_CALLBACK_H
#include <memory>

#include "cxx.h"
#include "request_preload.h"

namespace OHOS::Request {

/**
 * @brief cxx-bridge wrapper that lets Rust call a C++ PreloadCallback.
 *
 * Copies the std::function members out of the caller's PreloadCallback, so
 * the callbacks can be invoked from Rust through plain wrapper methods.
 * Rust holds this type in a UniquePtr inside FfiCallback (wrapper.rs) and
 * marks it Send; the wrapped callbacks must therefore be safe to invoke from
 * any thread.
 */
class PreloadCallbackWrapper {
public:
    // Copies the OnSuccess/OnFail/OnCancel handlers out of callback; leaves
    // them unset when callback is null, making every On* a no-op.
    PreloadCallbackWrapper(std::unique_ptr<PreloadCallback> &callback);
    ~PreloadCallbackWrapper() = default;
    PreloadCallbackWrapper(const PreloadCallbackWrapper &) = delete;
    PreloadCallbackWrapper &operator=(const PreloadCallbackWrapper &) = delete;

    // Invokes the wrapped success handler with the downloaded data.
    void OnSuccess(const std::shared_ptr<Data> data, rust::str TaskId) const;
    // Invokes the wrapped failure handler with the error and its download info.
    void OnFail(rust::Box<CacheDownloadError> error, rust::Box<RustDownloadInfo> info, rust::str TaskId) const;
    // Invokes the wrapped cancellation handler.
    void OnCancel() const;

private:
    // Handlers copied from the caller's PreloadCallback; an unset one makes
    // the corresponding On* method a no-op.
    std::function<void(const std::shared_ptr<Data> &&, const std::string &TaskId)> onSuccess_;
    std::function<void(const PreloadError &, const std::string &TaskId)> onFail_;
    std::function<void()> onCancel_;
};

/**
 * @brief cxx-bridge wrapper for the progress part of a C++ PreloadCallback.
 *
 * Same bridging scheme as PreloadCallbackWrapper, but only the OnProgress
 * handler is copied and exposed.
 */
class PreloadProgressCallbackWrapper {
public:
    // Copies the OnProgress handler out of callback; unset when null.
    PreloadProgressCallbackWrapper(std::unique_ptr<PreloadCallback> &callback);
    ~PreloadProgressCallbackWrapper() = default;
    PreloadProgressCallbackWrapper(const PreloadProgressCallbackWrapper &) = delete;
    PreloadProgressCallbackWrapper &operator=(const PreloadProgressCallbackWrapper &) = delete;

    // Invokes the wrapped progress handler (bytes downloaded so far / total).
    void OnProgress(uint64_t current, uint64_t total) const;

private:
    // Progress handler copied from the caller's PreloadCallback.
    std::function<void(uint64_t, uint64_t)> onProgress_;
};

// cxx cannot hand Rust native types (Box<T>, Arc<T>) across the bridge
// directly, so every Rust type crossing to C++ goes through one of these
// factory functions, which move the Box into the matching C++ wrapper type.
// They are declared in the cxx bridge module of cache_download's wrapper.rs
// and must stay in sync with it.

// Wraps a Rust download result for shared C++ ownership.
std::shared_ptr<Data> SharedData(rust::Box<RustData> data);
// Wraps a Rust download result for exclusive C++ ownership.
std::unique_ptr<Data> UniqueData(rust::Box<RustData> data);
// Wraps Rust download statistics into a C++ CppDownloadInfo.
std::unique_ptr<CppDownloadInfo> UniqueInfo(rust::Box<RustDownloadInfo> info);
// Wraps a Rust task handle for shared C++ ownership.
std::shared_ptr<PreloadHandle> ShareTaskHandle(rust::Box<TaskHandle> handle);
} // namespace OHOS::Request

#endif // REQUEST_PRE_DOWNLOAD_CALLBACK_H
