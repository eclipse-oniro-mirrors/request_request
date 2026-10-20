// Copyright (C) 2024 Huawei Device Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! Cache download task orchestration and state management.
//!
//! Aggregates the callback, network backend (netstack or ylong), common
//! utilities, error, and task submodules that together drive a cache download.

// Task state codes. The numeric values mirror the C++ `PreloadState` enum in
// `request_preload.h` one-to-one and cross the FFI boundary as plain
// integers; treat them as a cross-language contract: append only, never
// renumber or reorder.
/// Task created, not yet started.
pub(crate) const INIT: usize = 0;
/// Task is currently downloading.
pub(crate) const RUNNING: usize = 1;
/// Task finished successfully.
pub(crate) const SUCCESS: usize = 2;
/// Task failed (after exhausting retries).
pub(crate) const FAIL: usize = 3;
/// Task canceled by the caller.
pub(crate) const CANCEL: usize = 4;

mod callback;

cfg_netstack! {
    mod netstack;
}

cfg_ylong! {
    mod ylong;
}

pub(crate) mod common;
mod error;

/// Re-export of the cache download error type and its error kind enum.
pub use error::{CacheDownloadError, ErrorKind};
pub(crate) mod task;
