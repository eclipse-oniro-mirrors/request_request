// Copyright (C) 2025 Huawei Device Co., Ltd.
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

use super::wrapper::ffi::NetInfo;

#[allow(unused)]
pub trait Observer: Send + Sync {
    fn net_available(&self, net_id: i32) {}
    fn net_lost(&self, net_id: i32) {}
    fn net_capability_changed(&self, net_id: i32, net_info: &NetInfo) {}
}
