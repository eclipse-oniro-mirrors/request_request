// Copyright (C) 2023 Huawei Device Co., Ltd.
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

//! This create implement the request proxy and stub
#![cfg_attr(feature = "oh", feature(io_error_other))]
#![cfg_attr(test, feature(future_join))]
#![cfg_attr(test, allow(clippy::redundant_clone))]
#![allow(unreachable_pub, clippy::new_without_default)]
#![warn(
    missing_docs,
    clippy::redundant_static_lifetimes,
    clippy::enum_variant_names,
    clippy::clone_on_copy
)]
#[macro_use]
mod hilog;

pub mod ability;
mod error;
mod manage;
mod service;
mod sys_event;
mod task;

#[cfg(feature = "oh")]
mod trace;
mod utils;

pub use service::interface;
pub use task::config;
pub use utils::form_item::FileSpec;

const LOG_LABEL: hilog_rust::HiLogLabel = hilog_rust::HiLogLabel {
    log_type: hilog_rust::LogType::LogCore,
    domain: 0xD001C50,
    tag: "RequestService",
};

#[cfg(test)]
mod tests {
    /// test init
    pub(crate) fn test_init() {
        unsafe { SetAccessTokenPermission() };
    }

    extern "C" {
        fn SetAccessTokenPermission();
    }
}
