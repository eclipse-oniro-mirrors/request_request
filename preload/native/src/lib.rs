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

//! # Pre-download native

#![allow(missing_docs)]
#![allow(stable_features)]
#![deny(unused_must_use)]
#![feature(lazy_cell)]
#[macro_use]
extern crate request_utils;

mod agent;
pub use agent::{CustomCallback, DownloadAgent, DownloadRequest};

mod cache;
mod download;
pub mod error;
mod utils;
pub use error::DownloadError;

cfg_ohos! {
    mod wrapper;
    const TAG: &str = "PreloadNative\0";
    const DOMAIN: u32 = 0xD001C50;
    #[cfg(not(test))]
    use ffrt_rs::ffrt_spawn as spawn;
    #[cfg(test)]
    use std::thread::spawn as spawn;
}

cfg_not_ohos! {
    use std::thread::spawn as spawn;
}

cfg_test! {
    mod test;
    pub use test::init;
    pub(crate) use test::TEST_URL;
    pub use test::test_server;
}