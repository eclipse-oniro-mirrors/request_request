/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "preload_module.h"

#include <dlfcn.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "base/request/request/common/include/constant.h"
#include "base/request/request/common/include/log.h"
#include "ipc_skeleton.h"
#include "js_native_api.h"
#include "js_native_api_types.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "napi_utils.h"
#include "preload_common.h"
#include "preload_napi.h"
#include "request_preload.h"

namespace OHOS::Request {
using namespace Security::AccessToken;

// Upper bound (in bytes) for the URL argument accepted by every URL-taking
// export below. (The name is a historical typo of MAX_URL_LENGTH.)
constexpr const size_t MAX_UTL_LENGTH = 8192;

// Upper bounds for the size arguments of setMemoryCacheSize,
// setFileCacheSize and setDownloadInfoListSize.
constexpr int64_t MAX_MEM_SIZE = 1073741824;
constexpr int64_t MAX_FILE_SIZE = 4294967296;
constexpr int64_t MAX_INFO_LIST_SIZE = 8192;
// Permissions verified before download (INTERNET) and getDownloadInfo
// (GET_NETWORK_INFO) run; the other exports perform no permission check.
const std::string INTERNET_PERMISSION = "ohos.permission.INTERNET";
const std::string GET_NETWORK_INFO_PERMISSION = "ohos.permission.GET_NETWORK_INFO";

// Retry count boundaries for maxRetryCount (download /
// setGlobalRetryOptions); DEFAULT matches the Rust-side global default of 1.
constexpr int64_t DEFAULT_MAX_RETRY_COUNT = 1;
constexpr int64_t MIN_RETRY_COUNT = 0;
constexpr int64_t MAX_RETRY_COUNT = 10;

// Timeout boundaries in seconds for networkCheckTimeout and httpTotalTimeout;
// inputs are validated against these before reaching the Preload singleton.
constexpr int64_t DEFAULT_NETWORK_CHECK_TIMEOUT = 20;
constexpr int64_t MIN_NETWORK_CHECK_TIMEOUT = 0;
constexpr int64_t MAX_NETWORK_CHECK_TIMEOUT = 20;
constexpr int64_t DEFAULT_HTTP_TOTAL_TIMEOUT = 60;
constexpr int64_t MIN_HTTP_TOTAL_TIMEOUT = 1;
constexpr int64_t MAX_HTTP_TOTAL_TIMEOUT = static_cast<int64_t>(std::numeric_limits<uint32_t>::max() / 1000);

// Inclusive [minVal, maxVal] range accepted by ParseNumberField.
struct Range {
    int64_t minVal;
    int64_t maxVal;
};

// Reads the optional numeric property fieldName of obj into outValue.
// Returns true when the field is missing (outValue keeps its value) or is a
// number within range; returns false on a type mismatch or an out-of-range
// value, leaving outValue untouched.
bool ParseNumberField(napi_env env, napi_value obj, const char *fieldName, Range range, int32_t &outValue)
{
    napi_value field = GetNamedProperty(env, obj, fieldName);
    if (IsValueMissingOrSkipped(env, field)) {
        return true;
    }
    if (GetValueType(env, field) != napi_number) {
        return false;
    }
    int64_t value = GetValueNum(env, field);
    if (value < range.minVal || value > range.maxVal) {
        return false;
    }
    outValue = static_cast<int32_t>(value);
    return true;
}

// Registers cb for this URL. A function already in the list (strict
// equality) is reactivated instead of duplicated, undoing a deferred removal.
// New entries hold a strong reference (refcount 1) that keeps the JS function
// alive until it is removed.
napi_status CallbackInfo::RegisterCallback(napi_value cb)
{
    std::lock_guard<std::recursive_mutex> lock(allCbMutex_);

    for (auto &[isActive, ref] : allCb_) {
        napi_value existingCallback = nullptr;
        napi_get_reference_value(env_, ref, &existingCallback);

        bool areCallbacksEqual = false;
        napi_strict_equals(env_, cb, existingCallback, &areCallbacksEqual);

        if (!areCallbacksEqual) {
            continue;
        }

        if (!isActive) {
            isActive = true;
            --toDeleteCount_;
            ++validCbNum;
        }
        return napi_ok;
    }

    napi_ref ref;
    napi_status status = napi_create_reference(env_, cb, 1, &ref);
    if (status != napi_ok) {
        REQUEST_HILOGE("RegisterCallback status not ok, reason: %{public}d", status);
        return status;
    }
    allCb_.push_back(std::make_pair(true, ref));
    ++validCbNum;

    return napi_ok;
}

// Removes callbacks for this URL: with cb == nullptr all of them, otherwise
// only the strictly-equal one. While an invocation is in flight, entries are
// merely marked inactive and their physical removal is deferred to
// CleanupCallbacks(), because the invocation loop is still walking the list.
napi_status CallbackInfo::RemoveCallback(napi_value cb)
{
    std::lock_guard<std::recursive_mutex> lock(allCbMutex_);

    if (validCbNum == 0) {
        return napi_ok;
    }

    if (cb == nullptr) {
        if (!isInvoked_) {
            for (const auto &callback : allCb_) {
                napi_delete_reference(env_, callback.second);
            }
            allCb_.clear();
        } else {
            toDeleteCount_ = allCb_.size();
            for (auto &callback : allCb_) {
                callback.first = false;
            }
            validCbNum = 0;
        }
        return napi_ok;
    }

    auto it = std::find_if(allCb_.begin(), allCb_.end(), [this, cb](const auto &callback) {
        napi_value referenceValue = nullptr;
        napi_get_reference_value(env_, callback.second, &referenceValue);

        bool isEqual = false;
        napi_strict_equals(env_, cb, referenceValue, &isEqual);
        return isEqual;
    });
    if (it != allCb_.end()) {
        if (!isInvoked_) {
            napi_delete_reference(env_, it->second);
            allCb_.erase(it);
        } else if (it->first) {
            it->first = false;
            ++toDeleteCount_;
            --validCbNum;
        }
    }
    return napi_ok;
}

// Physically erases every inactive entry and deletes its napi_ref. Runs after
// an invocation completes; does nothing when no removal is pending.
void CallbackInfo::CleanupCallbacks()
{
    if (toDeleteCount_ == 0) {
        return;
    }
    // STL erase-remove_if
    auto newEnd = std::remove_if(allCb_.begin(), allCb_.end(), [this](const std::pair<bool, napi_ref> &cbPair) -> bool {
        if (!cbPair.first) {
            napi_delete_reference(env_, cbPair.second);
            return true; // will be erased
        }
        return false;
    });
    size_t deletedCount = static_cast<size_t>(std::distance(newEnd, allCb_.end()));
    allCb_.erase(newEnd, allCb_.end());
    toDeleteCount_ = (toDeleteCount_ > deletedCount) ? (toDeleteCount_ - deletedCount) : 0;
}

// Invokes every active success callback with a single argument (values[0])
// inside a fresh handle scope. Inactive entries are deleted first; callbacks
// removed from inside a callback are cleaned up afterwards. Must be called on
// the JS thread of env_.
void CallbackInfo::InvokeSuccessCallbacks(napi_value values[])
{
    std::lock_guard<std::recursive_mutex> lock(allCbMutex_);
    isInvoked_ = true;
    napi_handle_scope scope = nullptr;
    napi_status status = napi_open_handle_scope(env_, &scope);
    if (status != napi_ok || scope == nullptr) {
        REQUEST_HILOGE("InvokeSuccessCallbacks napi_scope failed, reason: %{public}d", status);
        return;
    }
    uint32_t paramNumber = 1;
    // todo: consider an unreachable url, clear no use cb
    for (auto it = allCb_.begin(); it != allCb_.end();) {
        if (it->first == false) {
            napi_delete_reference(env_, it->second);
            it = allCb_.erase(it);
            --toDeleteCount_;
            continue;
        }
        napi_value callbackFunc = nullptr;
        napi_get_reference_value(env_, it->second, &callbackFunc);
        napi_value callbackResult = nullptr;
        napi_call_function(env_, nullptr, callbackFunc, paramNumber, values, &callbackResult);
        it++;
    }
    CleanupCallbacks();
    isInvoked_ = false;
    REQUEST_HILOGD("Successfully invoked success callback");
    napi_close_handle_scope(env_, scope);
}

// Invokes every active error callback with a single argument (values[0], the
// { errorCode, message } object) inside a fresh handle scope. Inactive entries
// are deleted first; callbacks removed from inside a callback are cleaned up
// afterwards. Must be called on the JS thread of env_.
void CallbackInfo::InvokeErrorCallbacks(napi_value values[])
{
    std::lock_guard<std::recursive_mutex> lock(allCbMutex_);
    isInvoked_ = true;
    napi_handle_scope scope = nullptr;
    napi_status status = napi_open_handle_scope(env_, &scope);
    if (status != napi_ok || scope == nullptr) {
        REQUEST_HILOGE("InvokeErrorCallbacks napi_scope failed, reason: %{public}d", status);
        return;
    }
    uint32_t paramNumber = 1;
    // todo: consider an unreachable url, clear no use cb
    for (auto it = allCb_.begin(); it != allCb_.end();) {
        if (it->first == false) {
            napi_delete_reference(env_, it->second);
            it = allCb_.erase(it);
            --toDeleteCount_;
            continue;
        }
        napi_value callbackFunc = nullptr;
        napi_get_reference_value(env_, it->second, &callbackFunc);
        napi_value callbackResult = nullptr;
        napi_call_function(env_, nullptr, callbackFunc, paramNumber, values, &callbackResult);
        it++;
    }
    CleanupCallbacks();
    isInvoked_ = false;
    REQUEST_HILOGD("Successfully invoked error callbacks");
    napi_close_handle_scope(env_, scope);
}

// Returns the process-wide CallbackManager singleton.
CallbackManager &CallbackManager::GetInstance()
{
    static CallbackManager instance;
    return instance;
}

// Registers cb as a success/error callback for url, creating the per-URL
// CallbackInfo on first use. A null cb is logged and ignored.
napi_status CallbackManager::RegisterCallback(const std::string &url, CallbackType type, napi_env env, napi_value cb)
{
    REQUEST_HILOGD("RegisterCallback start. Type: %s", (type == CallbackType::SUCCESS) ? "success" : "error");
    // check same callback
    if (cb == nullptr) {
        REQUEST_HILOGE("RegisterCallback no cb");
        return napi_ok;
    }

    mgrMutex_.lock();
    auto &targetMap = (type == CallbackType::SUCCESS) ? successCallbacks_ : errorCallbacks_;
    auto it = targetMap.find(url);
    if (it == targetMap.end()) {
        // if URL is not exist, create a new CallbackInfo
        REQUEST_HILOGD("RegisterCallback create new CallbackInfo");
        targetMap[url] = std::make_shared<CallbackInfo>(env, url);
    }
    std::shared_ptr<CallbackInfo> info = targetMap[url];
    mgrMutex_.unlock();

    return info->RegisterCallback(cb);
}

// Removes one callback (all of them when cb is null) for url. A per-URL entry
// is created when none exists so the removal is a no-op rather than an error.
napi_status CallbackManager::RemoveCallback(const std::string &url, CallbackType type, napi_env env, napi_value cb)
{
    mgrMutex_.lock();
    auto &targetMap = (type == CallbackType::SUCCESS) ? successCallbacks_ : errorCallbacks_;
    REQUEST_HILOGD("RemoveCallback start. Type: %s", (type == CallbackType::SUCCESS) ? "success" : "error");
    auto it = targetMap.find(url);
    if (it == targetMap.end()) {
        // if URL is not exist, create a new CallbackInfo
        targetMap[url] = std::make_shared<CallbackInfo>(env, url);
    }
    std::shared_ptr<CallbackInfo> info = targetMap[url];
    mgrMutex_.unlock();

    return info->RemoveCallback(cb);
}

// Schedules the success callbacks of url on the JS thread of env via
// napi_send_event (high priority), since native completion arrives on a
// download thread. The data and taskId arguments are currently not passed on
// to JS.
void CallbackManager::InvokeSuccessCallbacks(
    const std::string &url, std::shared_ptr<Data> data, napi_env env, const std::string &taskId)
{
    REQUEST_HILOGD("Invoking success callbacks for URL");
    mgrMutex_.lock();
    auto it = successCallbacks_.find(url);
    if (it == successCallbacks_.end()) {
        // if URL is not exist, create a new CallbackInfo
        successCallbacks_[url] = std::make_shared<CallbackInfo>(env, url);
    }
    std::shared_ptr<CallbackInfo> info = successCallbacks_[url];
    mgrMutex_.unlock();

    int32_t ret = napi_send_event(
        info->env_,
        [url, info]() {
            napi_value values[2] = { nullptr };
            info->InvokeSuccessCallbacks(values);
        },
        napi_eprio_high, "request:cachedownload.download");
    if (ret != napi_ok) {
        REQUEST_HILOGE("napi_send_event failed: %{public}d", ret);
    }
}

// Schedules the error callbacks of url on the JS thread of env via
// napi_send_event (high priority). On the JS thread an error object of shape
// { errorCode: number, message: string } is built and passed as the single
// argument to each error callback.
void CallbackManager::InvokeErrorCallbacks(
    const std::string &url, std::shared_ptr<DownloadError> error, napi_env env, const std::string &taskId)
{
    REQUEST_HILOGD("Invoking error callbacks for URL");
    mgrMutex_.lock();
    auto it = errorCallbacks_.find(url);
    if (it == errorCallbacks_.end()) {
        // if URL is not exist, create a new CallbackInfo
        errorCallbacks_[url] = std::make_shared<CallbackInfo>(env, url);
    }
    std::shared_ptr<CallbackInfo> info = errorCallbacks_[url];
    mgrMutex_.unlock();

    int32_t ret = napi_send_event(
        info->env_,
        [url, info, error]() {
            napi_value values[2] = { nullptr };

            napi_value value = nullptr;
            napi_create_object(info->env_, &value);
            napi_set_named_property(
                info->env_, value, "errorCode", Convert2JSValue(info->env_, static_cast<uint32_t>(error->errorCode)));
            napi_set_named_property(info->env_, value, "message", Convert2JSValue(info->env_, error->message));
            values[0] = value;

            info->InvokeErrorCallbacks(values);
        },
        napi_eprio_high, "request:cachedownload.download");
    if (ret != napi_ok) {
        REQUEST_HILOGE("napi_send_event failed: %{public}d", ret);
    }
}

// Returns whether the calling process holds ohos.permission.INTERNET. The
// result is computed once and cached for the process lifetime, so a later
// permission revocation is not re-checked.
bool CheckInternetPermission()
{
    static bool hasPermission = []() {
        uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
        TypeATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(static_cast<AccessTokenID>(tokenId));
        if (tokenType == TOKEN_INVALID) {
            return false;
        }
        int result = AccessTokenKit::VerifyAccessToken(tokenId, INTERNET_PERMISSION);
        return result == PERMISSION_GRANTED;
    }();
    return hasPermission;
}

// Returns whether the calling process holds
// ohos.permission.GET_NETWORK_INFO; computed once and cached like
// CheckInternetPermission().
bool CheckNetworkInfoPermission()
{
    static bool hasPermission = []() {
        uint64_t tokenId = IPCSkeleton::GetCallingFullTokenID();
        TypeATokenTypeEnum tokenType = AccessTokenKit::GetTokenTypeFlag(static_cast<AccessTokenID>(tokenId));
        if (tokenType == TOKEN_INVALID) {
            return false;
        }
        int result = AccessTokenKit::VerifyAccessToken(tokenId, GET_NETWORK_INFO_PERMISSION);
        return result == PERMISSION_GRANTED;
    }();
    return hasPermission;
}

// Builds the PreloadCallback handed to Preload::load(). Both handlers forward
// to the CallbackManager, which hops to the JS thread before touching any
// napi value. OnFail maps the native ErrorKind to the public ErrorCode (IO and
// unmapped kinds fall back to OTHERS) and wraps it with the error message.
PreloadCallback CreatePreloadCallback(napi_env env, const std::string url)
{
    auto &cbManager = CallbackManager::GetInstance();
    return PreloadCallback{
        .OnSuccess =
            [env, url, &cbManager](const std::shared_ptr<Data> &&data, const std::string &taskId) {
                REQUEST_HILOGD("OnSuccess called with url");
                cbManager.InvokeSuccessCallbacks(url, data, env, taskId);
            },
        .OnFail =
            [env, url, &cbManager](const PreloadError &error, const std::string &taskId) {
                REQUEST_HILOGD("OnFail called with url");
                ErrorCode errorCode = ErrorCode::OTHERS; // Default to OTHERS
                ErrorKind kind = error.GetErrorKind();
                switch (kind) {
                    case ErrorKind::DNS:
                        errorCode = ErrorCode::DNS;
                        break;
                    case ErrorKind::TCP:
                        errorCode = ErrorCode::TCP;
                        break;
                    case ErrorKind::SSL:
                        errorCode = ErrorCode::SSL;
                        break;
                    case ErrorKind::HTTP:
                        errorCode = ErrorCode::HTTP;
                        break;
                    case ErrorKind::IO:
                    case ErrorKind::OTHERS:
                    default:
                        errorCode = ErrorCode::OTHERS;
                        break;
                }

                // Create a new DownloadError object
                auto downloadError = std::make_shared<DownloadError>(errorCode, error.GetMessage());

                // Pass the DownloadError to the callback manager
                cbManager.InvokeErrorCallbacks(url, downloadError, env, taskId);
            },
    };
}

/**
 * @brief JS API download(url: string, options?: object): void.
 *
 * Requires the INTERNET permission. Starts preloading url into the two-level
 * cache. options may carry headers, sslType, caPath, cacheStrategy,
 * maxRetryCount, networkCheckTimeout and httpTotalTimeout; a wrong-typed or
 * out-of-range numeric field throws a parameter error. Completion is reported
 * through the onDownloadSuccess/onDownloadError callbacks.
 */
napi_value download(napi_env env, napi_callback_info info)
{
    if (!CheckInternetPermission()) {
        ThrowError(env, E_PERMISSION, "internet permission denied");
        REQUEST_HILOGI("internet permission denied");
        return nullptr;
    }
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");
    if (GetValueType(env, args[0]) != napi_string || GetValueType(env, args[1]) != napi_object) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter error");
        return nullptr;
    }
    size_t urlLength = GetStringLength(env, args[0]);
    if (urlLength > MAX_UTL_LENGTH) {
        ThrowError(env, E_PARAMETER_CHECK, "url exceeds the maximum length");
        return nullptr;
    }
    std::string url = GetValueString(env, args[0], urlLength);
    std::unique_ptr<PreloadOptions> options = std::make_unique<PreloadOptions>();
    SetOptionsHeaders(env, args[1], options);
    SetOptionsSslType(env, args[1], options);
    napi_value napiCaPath = GetNamedProperty(env, args[1], "caPath");
    if (napiCaPath != nullptr) {
        std::string caPath = GetStringValueWithDefault(env, napiCaPath);
        options->caPath = caPath;
    }
    bool isUpdate = true;
    GetCacheStrategy(env, args[1], isUpdate);
    // Parameter validation: throw an error when out of range
    if (!SetOptionsRetry(env, args[1], options)) {
        ThrowError(env, E_PARAMETER_CHECK, "maxRetryCount out of range");
        return nullptr;
    }
    if (!SetOptionsTimeout(env, args[1], options)) {
        ThrowError(env, E_PARAMETER_CHECK, "timeout parameter out of range");
        return nullptr;
    }
    auto jsCallback = CreatePreloadCallback(env, url);
    Preload::GetInstance()->load(url, std::make_unique<PreloadCallback>(jsCallback), std::move(options), isUpdate);
    return nullptr;
}

/**
 * @brief JS API cancel(url: string): void.
 *
 * Cancels the in-flight preload task for url; a wrong-typed or oversized url
 * throws a parameter error.
 */
napi_value cancel(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");
    if (GetValueType(env, args[0]) != napi_string) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter error");
        return nullptr;
    }
    size_t urlLength = GetStringLength(env, args[0]);
    if (urlLength > MAX_UTL_LENGTH) {
        ThrowError(env, E_PARAMETER_CHECK, "url exceeds the maximum length");
        return nullptr;
    }
    std::string url = GetValueString(env, args[0], urlLength);
    Preload::GetInstance()->Cancel(url);
    return nullptr;
}

/**
 * @brief JS API setMemoryCacheSize(size: number): void.
 *
 * Sets the RAM cache capacity in bytes; a non-number or a value above 1 GiB
 * throws a parameter error.
 */
napi_value setMemoryCacheSize(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");

    if (GetValueType(env, args[0]) != napi_number) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter error");
        return nullptr;
    }
    int64_t size = GetValueNum(env, args[0]);
    if (size > MAX_MEM_SIZE) {
        ThrowError(env, E_PARAMETER_CHECK, "memory cache size exceeds the maximum value");
        return nullptr;
    }
    Preload::GetInstance()->SetRamCacheSize(size);
    return nullptr;
}

/**
 * @brief JS API setFileCacheSize(size: number): void.
 *
 * Sets the file cache capacity in bytes; a non-number or a value above 4 GiB
 * throws a parameter error.
 */
napi_value setFileCacheSize(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");

    if (GetValueType(env, args[0]) != napi_number) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter error");
        return nullptr;
    }
    int64_t size = GetValueNum(env, args[0]);
    if (size > MAX_FILE_SIZE) {
        ThrowError(env, E_PARAMETER_CHECK, "file cache size exceeds the maximum value");
        return nullptr;
    }
    Preload::GetInstance()->SetFileCacheSize(size);
    return nullptr;
}

/**
 * @brief JS API setDownloadInfoListSize(size: number): void.
 *
 * Sets how many download-info records are retained; size must be a number in
 * [0, 8192] or a parameter error is thrown.
 */
napi_value setDownloadInfoListSize(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");

    if (GetValueType(env, args[0]) != napi_number) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter error");
        return nullptr;
    }
    int64_t size = GetValueNum(env, args[0]);
    if (size > MAX_INFO_LIST_SIZE) {
        ThrowError(env, E_PARAMETER_CHECK, "info list size exceeds the maximum value");
        return nullptr;
    }
    if (size < 0) {
        ThrowError(env, E_PARAMETER_CHECK, "info list size is negative");
        return nullptr;
    }
    Preload::GetInstance()->SetDownloadInfoListSize(size);
    return nullptr;
}

/**
 * @brief JS API getDownloadInfo(url: string): object | undefined.
 *
 * Requires the GET_NETWORK_INFO permission. Returns the timing and size
 * statistics collected for url, or undefined when no statistics exist.
 */
napi_value getDownloadInfo(napi_env env, napi_callback_info info)
{
    if (!CheckNetworkInfoPermission()) {
        ThrowError(env, E_PERMISSION, "GET_NETWORK_INFO permission denied");
        REQUEST_HILOGI("GET_NETWORK_INFO permission denied");
        return nullptr;
    }
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");
    if (GetValueType(env, args[0]) != napi_string) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter error");
        return nullptr;
    }
    size_t urlLength = GetStringLength(env, args[0]);
    if (urlLength > MAX_UTL_LENGTH) {
        ThrowError(env, E_PARAMETER_CHECK, "url exceeds the maximum length");
        return nullptr;
    }
    std::string url = GetValueString(env, args[0], urlLength);
    std::optional<CppDownloadInfo> result = Preload::GetInstance()->GetDownloadInfo(url);
    if (!result) {
        napi_value undefined;
        napi_get_undefined(env, &undefined);
        return undefined;
    }
    return BuildDownloadInfo(env, result.value());
}

/**
 * @brief JS API clearMemoryCache(): void.
 *
 * Drops every RAM cache entry; takes no arguments and never throws.
 */
napi_value clearMemoryCache(napi_env env, napi_callback_info info)
{
    Preload::GetInstance()->ClearMemoryCache();
    return nullptr;
}

/**
 * @brief JS API clearFileCache(): void.
 *
 * Drops every file cache entry (files of running tasks are kept); takes no
 * arguments and never throws.
 */
napi_value clearFileCache(napi_env env, napi_callback_info info)
{
    Preload::GetInstance()->ClearFileCache();
    return nullptr;
}

/**
 * @brief JS API onDownloadSuccess(url: string, callback: function): void.
 *
 * Registers callback for url; it is invoked with a single argument when the
 * preload completes. Registering an equal function again is deduplicated.
 */
napi_value onDownloadSuccess(napi_env env, napi_callback_info info)
{
    size_t argc = TWO_ARG;
    napi_value args[TWO_ARG] = { nullptr };
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter obtain error");
        return nullptr;
    }

    // parameter check
    if (argc < TWO_ARG) {
        ThrowError(env, E_PARAMETER_CHECK, "missing mandatory parameters, wrong number of arguments");
        return nullptr;
    }
    if (GetValueType(env, args[0]) != napi_string) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter url type error");
        return nullptr;
    }
    size_t urlLength = GetStringLength(env, args[0]);
    if (urlLength == 0 || urlLength > MAX_UTL_LENGTH) {
        ThrowError(env, E_PARAMETER_CHECK, urlLength == 0 ? "url is empty" : "url exceeds the maximum length");
        return nullptr;
    }
    std::string url = GetValueString(env, args[0], urlLength);

    if (GetValueType(env, args[1]) != napi_function) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter callback type error");
        return nullptr;
    }

    // register success callback
    CallbackManager::GetInstance().RegisterCallback(url, CallbackType::SUCCESS, env, args[1]);
    REQUEST_HILOGD("Success callback registered for URL");

    return nullptr;
}

/**
 * @brief JS API onDownloadError(url: string, callback: function): void.
 *
 * Registers callback for url; it is invoked with a single { errorCode,
 * message } argument when the preload fails. Registering an equal function
 * again is deduplicated.
 */
napi_value onDownloadError(napi_env env, napi_callback_info info)
{
    size_t argc = TWO_ARG;
    napi_value args[TWO_ARG] = { nullptr };
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter obtain error");
        return nullptr;
    }
    // parameter check
    if (argc < TWO_ARG) {
        ThrowError(env, E_PARAMETER_CHECK, "missing mandatory parameters, wrong number of arguments");
        return nullptr;
    }
    if (GetValueType(env, args[0]) != napi_string) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter url type error");
        return nullptr;
    }
    size_t urlLength = GetStringLength(env, args[0]);
    if (urlLength == 0 || urlLength > MAX_UTL_LENGTH) {
        ThrowError(env, E_PARAMETER_CHECK, urlLength == 0 ? "url is empty" : "url exceeds the maximum length");
        return nullptr;
    }
    std::string url = GetValueString(env, args[0], urlLength);
    if (GetValueType(env, args[1]) != napi_function) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter callback type error");
        return nullptr;
    }

    // register error callback
    CallbackManager::GetInstance().RegisterCallback(url, CallbackType::ERROR, env, args[1]);
    REQUEST_HILOGD("Error callback registered for URL");

    return nullptr;
}

/**
 * @brief JS API offDownloadSuccess(url: string, callback?: function): void.
 *
 * Removes the given success callback for url, or all of them when callback is
 * omitted.
 */
napi_value offDownloadSuccess(napi_env env, napi_callback_info info)
{
    size_t argc = TWO_ARG;
    napi_value args[TWO_ARG] = { nullptr };
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter obtain error");
        return nullptr;
    }

    // parameter check
    if (GetValueType(env, args[0]) != napi_string) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter url type error");
        return nullptr;
    }

    size_t urlLength = GetStringLength(env, args[0]);
    if (urlLength == 0 || urlLength > MAX_UTL_LENGTH) {
        ThrowError(env, E_PARAMETER_CHECK, urlLength == 0 ? "url is empty" : "url exceeds the maximum length");
        return nullptr;
    }

    std::string url = GetValueString(env, args[0], urlLength);

    if (argc == ONE_ARG) {
        CallbackManager::GetInstance().RemoveCallback(url, CallbackType::SUCCESS, env, nullptr);
        REQUEST_HILOGD("Success callback all removed for URL");
        return nullptr;
    }

    if (GetValueType(env, args[1]) != napi_function) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter callback type error");
        return nullptr;
    }

    // remove success callback
    CallbackManager::GetInstance().RemoveCallback(url, CallbackType::SUCCESS, env, args[1]);
    REQUEST_HILOGD("Success callback removed for URL");

    return nullptr;
}

/**
 * @brief JS API offDownloadError(url: string, callback?: function): void.
 *
 * Removes the given error callback for url, or all of them when callback is
 * omitted.
 */
napi_value offDownloadError(napi_env env, napi_callback_info info)
{
    size_t argc = TWO_ARG;
    napi_value args[TWO_ARG] = { nullptr };
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter obtain error");
        return nullptr;
    }

    // parameter check
    if (GetValueType(env, args[0]) != napi_string) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter url type error");
        return nullptr;
    }

    size_t urlLength = GetStringLength(env, args[0]);
    if (urlLength == 0 || urlLength > MAX_UTL_LENGTH) {
        ThrowError(env, E_PARAMETER_CHECK, urlLength == 0 ? "url is empty" : "url exceeds the maximum length");
        return nullptr;
    }

    std::string url = GetValueString(env, args[0], urlLength);

    if (argc == ONE_ARG) {
        CallbackManager::GetInstance().RemoveCallback(url, CallbackType::ERROR, env, nullptr);
        REQUEST_HILOGD("Error callback all removed for URL");
        return nullptr;
    }

    if (GetValueType(env, args[1]) != napi_function) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter callback type error");
        return nullptr;
    }

    // remove error callback
    CallbackManager::GetInstance().RemoveCallback(url, CallbackType::ERROR, env, args[1]);
    REQUEST_HILOGD("Error callback removed for URL");

    return nullptr;
}

/**
 * @brief JS API setGlobalRetryOptions(options?: object): void.
 *
 * Sets the process-wide retry count for tasks without their own override.
 * Calling without options (or with a missing field) applies the default of 1.
 * maxRetryCount must be a number in [0, 10] or a parameter error is thrown.
 */
napi_value setGlobalRetryOptions(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");

    RetryOptions retryOptions = { .maxRetryCount = static_cast<int32_t>(DEFAULT_MAX_RETRY_COUNT) };

    if (argc == 0 || IsValueMissingOrSkipped(env, args[0])) {
        Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
        return nullptr;
    }

    if (GetValueType(env, args[0]) != napi_object) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter type error");
        return nullptr;
    }

    if (!ParseNumberField(env, args[0], "maxRetryCount", {MIN_RETRY_COUNT, MAX_RETRY_COUNT},
        retryOptions.maxRetryCount)) {
        ThrowError(env, E_PARAMETER_CHECK, "maxRetryCount type error or out of range");
        return nullptr;
    }

    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
    return nullptr;
}

/**
 * @brief JS API setGlobalTimeoutOptions(options?: object): void.
 *
 * Sets the process-wide timeouts in seconds for tasks without their own
 * override. Missing options or fields apply the defaults (20s network check,
 * 60s HTTP total). networkCheckTimeout must be in [0, 20] and httpTotalTimeout
 * in [1, uint32 max / 1000], else a parameter error is thrown.
 */
napi_value setGlobalTimeoutOptions(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    PRELOAD_NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr), "napi_get_cb_info failed");

    TimeoutOptions timeoutOptions = {
        .networkCheckTimeout = static_cast<int32_t>(DEFAULT_NETWORK_CHECK_TIMEOUT),
        .httpTotalTimeout = static_cast<int32_t>(DEFAULT_HTTP_TOTAL_TIMEOUT)
    };

    if (argc == 0 || IsValueMissingOrSkipped(env, args[0])) {
        Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
        return nullptr;
    }

    if (GetValueType(env, args[0]) != napi_object) {
        ThrowError(env, E_PARAMETER_CHECK, "parameter type error");
        return nullptr;
    }

    if (!ParseNumberField(env, args[0], "networkCheckTimeout", {MIN_NETWORK_CHECK_TIMEOUT, MAX_NETWORK_CHECK_TIMEOUT},
        timeoutOptions.networkCheckTimeout)) {
        ThrowError(env, E_PARAMETER_CHECK, "networkCheckTimeout type error or out of range");
        return nullptr;
    }

    if (!ParseNumberField(env, args[0], "httpTotalTimeout", {MIN_HTTP_TOTAL_TIMEOUT, MAX_HTTP_TOTAL_TIMEOUT},
        timeoutOptions.httpTotalTimeout)) {
        ThrowError(env, E_PARAMETER_CHECK, "httpTotalTimeout type error or out of range");
        return nullptr;
    }

    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
    return nullptr;
}

// Builds the SslType constant object { TLS, TLCP } exposed on the module.
static void NapiCreateEnumSslType(napi_env env, napi_value &sslType)
{
    napi_create_object(env, &sslType);
    SetStringPropertyUtf8(env, sslType, "TLS", "TLS");
    SetStringPropertyUtf8(env, sslType, "TLCP", "TLCP");
}

// Builds the CacheStrategy constant object { FORCE, LAZY } exposed on the
// module.
static void NapiCreateEnumCacheStrategy(napi_env env, napi_value &cacheStrategy)
{
    napi_create_object(env, &cacheStrategy);
    SetUint32Property(env, cacheStrategy, "FORCE", static_cast<uint32_t>(CacheStrategy::FORCE));
    SetUint32Property(env, cacheStrategy, "LAZY", static_cast<uint32_t>(CacheStrategy::LAZY));
}

// Builds the ErrorCode constant object { OTHERS, DNS, TCP, SSL, HTTP } exposed
// on the module; the values match the ErrorCode enum in preload_module.h.
static void NapiCreateEnumErrorCode(napi_env env, napi_value &errorCode)
{
    napi_create_object(env, &errorCode);
    SetUint32Property(env, errorCode, "OTHERS", static_cast<uint32_t>(ErrorCode::OTHERS));
    SetUint32Property(env, errorCode, "DNS", static_cast<uint32_t>(ErrorCode::DNS));
    SetUint32Property(env, errorCode, "TCP", static_cast<uint32_t>(ErrorCode::TCP));
    SetUint32Property(env, errorCode, "SSL", static_cast<uint32_t>(ErrorCode::SSL));
    SetUint32Property(env, errorCode, "HTTP", static_cast<uint32_t>(ErrorCode::HTTP));
}

// Module registration callback: exposes the enum constant objects and the JS
// functions above on the module's exports.
static napi_value registerFunc(napi_env env, napi_value exports)
{
    napi_value sslType = nullptr;
    napi_value cacheStrategy = nullptr;
    napi_value errorCode = nullptr;
    NapiCreateEnumSslType(env, sslType);
    NapiCreateEnumCacheStrategy(env, cacheStrategy);
    NapiCreateEnumErrorCode(env, errorCode);
    napi_property_descriptor desc[]{
        DECLARE_NAPI_PROPERTY("SslType", sslType),
        DECLARE_NAPI_PROPERTY("CacheStrategy", cacheStrategy),
        DECLARE_NAPI_PROPERTY("ErrorCode", errorCode),
        DECLARE_NAPI_FUNCTION("download", download),
        DECLARE_NAPI_FUNCTION("cancel", cancel),
        DECLARE_NAPI_FUNCTION("setMemoryCacheSize", setMemoryCacheSize),
        DECLARE_NAPI_FUNCTION("setFileCacheSize", setFileCacheSize),
        DECLARE_NAPI_FUNCTION("setDownloadInfoListSize", setDownloadInfoListSize),
        DECLARE_NAPI_FUNCTION("getDownloadInfo", getDownloadInfo),
        DECLARE_NAPI_FUNCTION("clearMemoryCache", clearMemoryCache),
        DECLARE_NAPI_FUNCTION("clearFileCache", clearFileCache),
        DECLARE_NAPI_FUNCTION("onDownloadSuccess", onDownloadSuccess),
        DECLARE_NAPI_FUNCTION("onDownloadError", onDownloadError),
        DECLARE_NAPI_FUNCTION("offDownloadSuccess", offDownloadSuccess),
        DECLARE_NAPI_FUNCTION("offDownloadError", offDownloadError),
        DECLARE_NAPI_FUNCTION("setGlobalRetryOptions", setGlobalRetryOptions),
        DECLARE_NAPI_FUNCTION("setGlobalTimeoutOptions", setGlobalTimeoutOptions),

    };
    PRELOAD_NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(napi_property_descriptor), desc),
        "napi_define_properties failed");
    return exports;
}

} // namespace OHOS::Request

// Library constructor: registers the "request.cacheDownload" module with the
// NAPI framework when the library is loaded.
static __attribute__((constructor)) void RegisterModule()
{
    static napi_module module = { .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = nullptr,
        .nm_register_func = OHOS::Request::registerFunc,
        .nm_modname = "request.cacheDownload",
        .nm_priv = ((void *)0),
        .reserved = { 0 } };
    napi_module_register(&module);
}
