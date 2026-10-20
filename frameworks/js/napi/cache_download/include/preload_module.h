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

#ifndef REQUEST_PRE_DOWNLOAD_MODULE_H
#define REQUEST_PRE_DOWNLOAD_MODULE_H

#include <list>
#include <map>
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"

namespace OHOS::Request {
// Error code enumeration for cache download operations
enum class ErrorCode: uint32_t {
    // Other unspecified errors
    OTHERS = 0xFF,
    
    // DNS-related errors
    DNS = 0x00,
    
    // TCP connection errors
    TCP = 0x10,
    
    // SSL security errors
    SSL = 0x20,
    
    // HTTP protocol errors
    HTTP = 0x30,
};

// Struct representing a download error with error code and message
struct DownloadError {
    // Error code identifying the specific type of cache download failure
    const ErrorCode errorCode;
    
    // Descriptive message explaining the failure reason
    const std::string message;
    
    // Constructor initializing error code and message
    DownloadError(ErrorCode code, const std::string& msg) : errorCode(code), message(msg) {}
};

// Callback type enumeration
enum class CallbackType {
    SUCCESS,
    ERROR
};

/**
 * @brief Registry of JS callbacks registered for one URL and one callback
 * type (success or error).
 *
 * Holds strong references (napi_ref) to every registered JS function. Removals
 * requested while an invocation is in flight are deferred: the entry is marked
 * inactive and physically removed once the invocation finishes.
 */
class CallbackInfo {
    friend class CallbackManager;

public:
    CallbackInfo(napi_env env, const std::string &url)
        : env_(env), url_(url)
    {
    }
    
    // Removes one callback (or all when cb is null). Removals requested while
    // an invocation is in flight are deferred by marking entries inactive.
    napi_status RemoveCallback(napi_value cb);
    // Registers a JS function; registering a function already in the list
    // (strict equality) reactivates it instead of adding a duplicate.
    napi_status RegisterCallback(napi_value cb);
    // Invokes every active success callback with a single argument on the JS
    // thread, then physically removes deferred entries.
    void InvokeSuccessCallbacks(napi_value values[]);
    // Invokes every active error callback with a single argument on the JS
    // thread, then physically removes deferred entries.
    void InvokeErrorCallbacks(napi_value values[]);

protected:
    // napi environment all callbacks of this URL were registered on.
    const napi_env env_;
    // URL this registry is keyed by.
    const std::string url_;
    // All registered callbacks as (isActive, napi_ref) pairs; isActive == false
    // marks an entry removed while an invocation is in flight.
    std::list<std::pair<bool, napi_ref>> allCb_;
    // Guards allCb_ and the deletion bookkeeping below.
    std::recursive_mutex allCbMutex_;
    // Number of active callbacks; 0 means there is nothing left to invoke.
    std::atomic<uint32_t> validCbNum{ 0 };
    // True while Invoke*Callbacks is iterating; forces removals to be deferred.
    bool isInvoked_{ false };
    // Number of inactive entries waiting to be physically removed by
    // CleanupCallbacks().
    size_t toDeleteCount_{ 0 };
    
    // Physically removes all inactive entries and deletes their references.
    void CleanupCallbacks();
};

class Data;

/**
 * @brief Process-wide registry of cache download callbacks, keyed by URL.
 *
 * Keeps two maps (success and error) of per-URL CallbackInfo. Native download
 * completion arrives on non-JS threads, so invocation is scheduled onto the JS
 * thread with napi_send_event before any JS function is called.
 */
class CallbackManager {
public:
    // Returns the process-wide singleton.
    static CallbackManager &GetInstance();
    
    // Registers cb as a success/error callback for url, creating the per-URL
    // CallbackInfo on first use. A null cb is ignored.
    napi_status RegisterCallback(const std::string& url, CallbackType type, napi_env env, napi_value callback);
    
    // Removes one callback (all when callback is null) for url; creates an
    // empty per-URL entry when none exists yet.
    napi_status RemoveCallback(const std::string& url, CallbackType type, napi_env env, napi_value callback);
    
    // Schedules invocation of url's success callbacks on the JS thread.
    void InvokeSuccessCallbacks(const std::string& url,
        std::shared_ptr<Data> data, napi_env env, const std::string& taskId);
    
    // Schedules invocation of url's error callbacks on the JS thread with an
    // error object of shape { errorCode, message }.
    void InvokeErrorCallbacks(const std::string& url,
        std::shared_ptr<DownloadError> error, napi_env env, const std::string& taskId);
  
private:
    CallbackManager() = default;
    
    CallbackManager(const CallbackManager&) = delete;
    CallbackManager& operator=(const CallbackManager&) = delete;
    
    // Guards the two maps below (map lookups only; per-URL work locks the
    // CallbackInfo itself).
    std::recursive_mutex mgrMutex_;
    
    // Success callbacks keyed by URL.
    std::map<std::string, std::shared_ptr<CallbackInfo>> successCallbacks_;
    
    // Error callbacks keyed by URL.
    std::map<std::string, std::shared_ptr<CallbackInfo>> errorCallbacks_;
};

}

#endif