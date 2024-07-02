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

use crate::utils::c_wrapper::CStringWrapper;

pub(crate) fn check_url_domain(app_id: &str, domain_type: &str, url: &str) -> Option<bool> {
    match unsafe { PolicyCheckUrlDomain(app_id.into(), domain_type.into(), url.into()) } {
        0 => Some(true),
        1 => Some(false),
        _ => None,
    }
}

#[link(name = "download_server_cxx", kind = "static")]
extern "C" {
    pub(crate) fn PolicyCheckUrlDomain(
        app_id: CStringWrapper,
        domain_type: CStringWrapper,
        url: CStringWrapper,
    ) -> i32;
}