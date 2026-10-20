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

#ifndef REQUEST_PRE_DOWNLOAD_NATIVE_FFI_H
#define REQUEST_PRE_DOWNLOAD_NATIVE_FFI_H

#include "cxx.h"
#include "inotify_event_listener.h"

namespace OHOS::Request {
// cxx bridge entry points used by cache_core's Rust side to drive the C++
// inotify monitor. Rust cannot construct or call C++ classes directly, so it
// goes through these two functions.

// Creates a DirectoryMonitor watching target with the given Rust rebuild
// callback; the result is handed back to Rust as a UniquePtr.
std::unique_ptr<DirectoryMonitor> NewDirectoryMonitor(const std::string &target, rust::Box<DirRebuilder> callback);
// Starts monitor's watch thread.
void StartObserve(DirectoryMonitor &monitor);
} // namespace OHOS::Request

#endif