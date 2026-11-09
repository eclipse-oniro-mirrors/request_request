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

use super::*;

// @tc.name: ut_identity_helper
// @tc.desc: Identity helper acquisition functions are callable and structurally
// consistent
// @tc.precon: NA
// @tc.step: 1. Acquire app uid / token / bundle via the identity helper
//           2. Verify the aggregate identity matches the individual accessors
// @tc.expect: aggregate identity equals the individual accessors; bundle is
// empty in test env (no IPC caller, same as ut_utils_oh)
// @tc.type: FUNC
#[cfg(feature = "oh")]
#[test]
fn ut_identity_helper() {
    let identity = calling_app_identity();
    let uid = calling_app_uid();
    let token = calling_token_id();
    assert_eq!(identity.app_uid, uid);
    assert_eq!(identity.token_id, token);
    assert!(identity.bundle.is_empty());
}
