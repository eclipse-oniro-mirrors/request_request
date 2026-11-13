/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "permission_verification.h"

#include <hilog/log.h>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "ipc_skeleton.h"

namespace OHOS::Request {
using namespace OHOS::Security::AccessToken;

namespace {
constexpr HiviewDFX::HiLogLabel LABEL = { LOG_CORE, 0xD001C50, "RequestCacheDownload" };
}

bool CheckInternetPermission()
{
    // Permission check is based on token (GetCallingFullTokenID + VerifyAccessToken).
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    TypeATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(static_cast<AccessTokenID>(tokenId));
    if (tokenType == TOKEN_INVALID) {
        HiviewDFX::HiLog::Warn(LABEL, "CheckInternetPermission denied: invalid token");
        return false;
    }
    int result = AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.INTERNET");
    if (result != PERMISSION_GRANTED) {
        HiviewDFX::HiLog::Warn(LABEL, "CheckInternetPermission denied: token type %{public}d",
            static_cast<int>(tokenType));
        return false;
    }
    return true;
}

bool CheckGetNetworkInfoPermission()
{
    uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
    TypeATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(static_cast<AccessTokenID>(tokenId));
    if (tokenType == TOKEN_INVALID) {
        HiviewDFX::HiLog::Warn(LABEL, "CheckGetNetworkInfoPermission denied: invalid token");
        return false;
    }
    int result = AccessTokenKit::VerifyAccessToken(tokenId, "ohos.permission.GET_NETWORK_INFO");
    if (result != PERMISSION_GRANTED) {
        HiviewDFX::HiLog::Warn(LABEL, "CheckGetNetworkInfoPermission denied: token type %{public}d",
            static_cast<int>(tokenType));
        return false;
    }
    return true;
}

} // namespace OHOS::Request