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

#ifndef REQUEST_JS_RESPONSE_LISTENER_H
#define REQUEST_JS_RESPONSE_LISTENER_H

#include "i_response_listener.h"
#include "listener_list.h"

namespace OHOS::Request {
// Listener for the request.agent "response" event. OnResponseReceive is
// invoked on the UDS receive thread — no napi call happens there; the
// response is marshalled to the JS thread via napi_send_event and only
// converted/dispatched inside that event (handle scope → convert →
// callback). The server subscription follows the JS listener count: the
// first added callback subscribes via RequestManager, removing the last
// one unsubscribes.
class JSResponseListener
    : public IResponseListener
    , public ListenerList
    , public std::enable_shared_from_this<JSResponseListener> {
public:
    JSResponseListener(napi_env env, const std::string &taskId) : ListenerList(env, taskId, SubscribeType::RESPONSE)
    {
    }
    napi_status AddListener(napi_value cb);
    napi_status RemoveListener(napi_value cb = nullptr);
    void OnResponseReceive(const std::shared_ptr<Response> &response) override;

private:
    // Guards the stashed response so the JS-thread event can read what the
    // UDS thread wrote.
    std::mutex responseMutex_;
    std::shared_ptr<Response> response_;
};

} // namespace OHOS::Request

#endif // REQUEST_JS_RESPONSE_LISTENER_H