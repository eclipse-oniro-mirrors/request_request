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

#ifndef REQUEST_GET_PROXY_H
#define REQUEST_GET_PROXY_H

#include <shared_mutex>
#include <string>

#include "c_string_wrapper.h"
#include "common_event_manager.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"
#include "log.h"
#include "matching_skills.h"
#include "want.h"

#ifdef __cplusplus
extern "C" {
#endif

// Snapshot of the current system proxy, all three fields read under one
// lock so they always describe the same proxy update.
struct HttpProxySnapshot {
    CStringWrapper host;
    CStringWrapper port;
    CStringWrapper exclusionList;
};

HttpProxySnapshot GetHttpProxySnapshot();

#ifdef __cplusplus
}
#endif

class SysNetProxySubscriber : public OHOS::EventFwk::CommonEventSubscriber {
public:
    SysNetProxySubscriber(OHOS::EventFwk::CommonEventSubscribeInfo &subscriberInfo)
        : CommonEventSubscriber(subscriberInfo)
    {
    }
    ~SysNetProxySubscriber() = default;
    void OnReceiveEvent(const OHOS::EventFwk::CommonEventData &data) override;
};

class SysNetProxyManager {
public:
    static SysNetProxyManager &GetInstance();
    void SubscriberEvent();

    void GetHttpProxy(const std::string proxyContent, std::string &host, std::string &port, std::string &exclusionList);
    void InitProxy(std::string &host, std::string &port, std::string &exclusion);
    void SetHttpProxy(std::string host, std::string port, std::string list)
    {
        REQUEST_HILOGD("SysNetProxyManager SetHttpProxy host is %{public}s", host.c_str());
        REQUEST_HILOGD("SysNetProxyManager SetHttpProxy port is %{public}s", port.c_str());
        REQUEST_HILOGD("SysNetProxyManager SetHttpProxy list is %{public}s", list.c_str());

        host_ = host;
        port_ = port;
        exclusionList_ = list;
    }

private:
    static std::shared_ptr<SysNetProxySubscriber> subscriber_;
    std::string host_;
    std::string port_;
    std::string exclusionList_;
    std::shared_mutex proxyMutex;
    // OnReceiveEvent writes proxy fields, GetHttpProxySnapshot builds a
    // consistent snapshot; both need direct access to the fields and lock.
    friend class SysNetProxySubscriber;
    friend HttpProxySnapshot GetHttpProxySnapshot();
};
std::shared_ptr<SysNetProxySubscriber> SysNetProxyManager::subscriber_ = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

void RegisterProxySubscriber();

#ifdef __cplusplus
}
#endif

#endif // REQUEST_GET_PROXY_H
