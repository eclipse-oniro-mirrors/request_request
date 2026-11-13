// Copyright (C) 2026 Huawei Device Co., Ltd.
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

//! Caller-identity helper (IdentityHelper): the single entry for caller identity
//! in the request service; business code must not call `ipc::Skeleton` directly.
//! `calling_app_uid()` returns the caller's app uid via the platform appid
//! interface `GetCallingAppUid` (@since 26.1.0).

use super::query_calling_bundle;

/// Full caller identity.
pub(crate) struct IdentityInfo {
    /// App uid (application uid).
    pub app_uid: u64,
    /// Bundle name (token → bundle).
    pub bundle: String,
    /// Full token id.
    pub token_id: u64,
}

/// Returns the caller's app uid (via platform GetCallingAppUid).
pub(crate) fn calling_app_uid() -> u64 {
    ipc::Skeleton::calling_app_uid()
}

/// Returns the caller's full token id.
pub(crate) fn calling_token_id() -> u64 {
    ipc::Skeleton::calling_full_token_id()
}

/// Collects the full caller identity (app_uid + bundle + token_id).
pub(crate) fn calling_app_identity() -> IdentityInfo {
    IdentityInfo {
        app_uid: calling_app_uid(),
        bundle: query_calling_bundle(),
        token_id: calling_token_id(),
    }
}

#[cfg(test)]
mod ut_identity {
    include!("../../tests/ut/ut_identity.rs");
}
