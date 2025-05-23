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

/// request task config
pub mod config;
/// request task info
pub mod info;

pub(crate) mod download;
pub(crate) mod files;
pub(crate) mod notify;
mod operator;
pub(crate) mod reason;
pub(crate) mod request_task;
pub(crate) const ATOMIC_SERVICE: u32 = 1;
pub(crate) mod bundle;
pub(crate) mod client;
pub(crate) mod ffi;
pub(crate) mod speed_limiter;
pub(crate) mod task_control;
pub(crate) mod upload;
