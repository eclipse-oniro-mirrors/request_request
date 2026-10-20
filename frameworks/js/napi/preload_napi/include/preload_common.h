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

#ifndef REQUEST_PRE_DOWNLOAD_PRELOAD_COMMON_H
#define REQUEST_PRE_DOWNLOAD_PRELOAD_COMMON_H

#include <vector>

#include "js_native_api.h"
#include "js_native_api_types.h"
#include "napi/native_common.h"
#include "napi_utils.h"
#include "request_preload.h"
namespace OHOS::Request {
// Returns true when value is nullptr, undefined or null, i.e. the caller did
// not provide the field; used to fall back to default option values.
inline bool IsValueMissingOrSkipped(napi_env env, napi_value value)
{
    if (value == nullptr) {
        return true;
    }
    napi_valuetype type = GetValueType(env, value);
    return type == napi_undefined || type == napi_null;
}

// Builds the "performance" sub-object of the JS download info: dnsTime,
// connectTime, tlsTime, firstSendTime, firstReceiveTime, totalTime and
// redirectTime (all in milliseconds). Returns false on a napi failure.
bool BuildInfoPerformance(napi_env env, const CppDownloadInfo &result, napi_value &jsInfo);
// Builds the "network" sub-object of the JS download info:
// { ip, dnsServers[] } (ip is omitted when the server address is empty).
// Returns false on a napi failure.
bool BuildInfoNetwork(napi_env env, const CppDownloadInfo &result, napi_value &jsInfo);
// Builds the "resource" sub-object of the JS download info: { size }.
// Returns false on a napi failure.
bool BuildInfoResource(napi_env env, const CppDownloadInfo &result, napi_value &jsInfo);
// Reads the "headers" object of arg into options->headers as (name, value)
// pairs; a missing or non-object "headers" leaves the list empty.
void SetOptionsHeaders(napi_env env, napi_value arg, std::unique_ptr<PreloadOptions> &options);
// Reads the "sslType" string of arg into options->sslType. A missing field or
// a non-string value falls back to SslType::DEFAULT; an unrecognized string
// falls back to TLS.
void SetOptionsSslType(napi_env env, napi_value arg, std::unique_ptr<PreloadOptions> &options);
// Reads the "cacheStrategy" number of arg into isUpdate: only
// CacheStrategy::LAZY sets it to false; missing, wrong-typed or other values
// leave it true.
void GetCacheStrategy(napi_env env, napi_value arg, bool &isUpdate);
// Reads retry.maxRetryCount (range [0, 10]) from arg. A missing field keeps
// the SENTINEL_NOT_SET sentinel (use the global default); returns false on a
// wrong type or an out-of-range value so the caller can throw.
bool SetOptionsRetry(napi_env env, napi_value arg, std::unique_ptr<PreloadOptions> &options);
// Reads timeout.networkCheckTimeout (range [0, 20]) and
// timeout.httpTotalTimeout (range [1, uint32 max / 1000]), both in seconds,
// from arg. Missing fields keep the SENTINEL_NOT_SET sentinel; returns false
// on a wrong type or an out-of-range value so the caller can throw.
bool SetOptionsTimeout(napi_env env, napi_value arg, std::unique_ptr<PreloadOptions> &options);
// Sets a double field on the performance object; returns the napi status.
inline napi_status SetPerformanceField(napi_env env, napi_value performance, double field_value, const char *js_name);
} // namespace OHOS::Request
#endif