/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef REQUEST_LISTENER_LIST_H
#define REQUEST_LISTENER_LIST_H

#include <list>
#include <mutex>
#include <string>

#include "napi/native_api.h"
#include "napi_utils.h"

namespace OHOS::Request {
// Bookkeeping of the JS callbacks registered for one (task,
// SubscribeType) — the shared base of JSResponseListener and
// JSNotifyDataListener. Callbacks are held as napi_ref entries with a
// validity flag: removal only unflags an entry; its reference is deleted
// lazily on the next JS-thread dispatch (OnMessageReceive), where napi
// calls are legal. The recursive mutex makes add/remove/dispatch safe
// against a JS callback re-entering the listener during dispatch.
class ListenerList {
public:
    ListenerList(napi_env env, const std::string &taskId, const SubscribeType &type)
        : env_(env), taskId_(taskId), type_(type)
    {
    }
    /// Whether any callback is still live.
    bool HasListener();
    /// Drops every callback reference immediately (task teardown; JS
    /// thread only).
    void DeleteAllListenerRef();

protected:
    bool IsListenerAdded(napi_value cb);
    void OnMessageReceive(napi_value *value, uint32_t paramNumber);
    napi_status AddListenerInner(napi_value cb);
    napi_status RemoveListenerInner(napi_value cb = nullptr);

protected:
    const napi_env env_;
    const std::string taskId_;
    const SubscribeType type_;
    // (still valid, callback reference) pairs; invalid entries are erased
    // on the next dispatch.
    std::list<std::pair<bool, napi_ref>> allCb_;
    std::recursive_mutex allCbMutex_;
    // Live-callback count, read by the subscribe/unsubscribe decisions.
    std::atomic<uint32_t> validCbNum{ 0 };
};

} // namespace OHOS::Request

#endif // REQUEST_LISTENER_LIST_H
