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

#include "cj_app_state_callback.h"

#include "cj_application_context.h"
#include "cj_request_task.h"
#include "ffrt.h"
#include "log.h"
#include "request_common.h"
#include "request_manager.h"

namespace OHOS::CJSystemapi::Request {
using OHOS::Request::Mode;
using OHOS::Request::RequestManager;

void CJAppStateCallback::OnAbilityForeground(const int64_t &ability)
{
    if (RequestManager::GetInstance()->IsSaReady()) {
        return;
    }
    {
        std::lock_guard<std::mutex> lockGuard(CJRequestTask::taskMutex_);
        for (auto &item : CJRequestTask::taskMap_) {
            if (item.second->config_.mode == Mode::FOREGROUND) {
                ffrt::submit([]() mutable { RequestManager::GetInstance()->LoadRequestServer(); });
                return;
            }
        }
    }
    // CAS clears the guard exactly once; a concurrent foreground-event
    // handler or creator thread cannot also run the unregistration logic.
    bool expected = true;
    if (!CJRequestTask::register_.compare_exchange_strong(expected, false)) {
        return;
    }
    // Re-scan inside taskMutex_ before unregistering: a FOREGROUND task may
    // have been created between the first scan above and this point (it is
    // still registering or not yet visible in taskMap_). If one exists it
    // still depends on this callback to resume the SA, so restore the guard
    // and keep the callback registered.
    {
        std::lock_guard<std::mutex> lockGuard(CJRequestTask::taskMutex_);
        for (auto &item : CJRequestTask::taskMap_) {
            if (item.second->config_.mode == Mode::FOREGROUND) {
                CJRequestTask::register_.store(true);
                return;
            }
        }
    }
    auto context = ApplicationContextCJ::CJApplicationContext::GetInstance();
    if (context == nullptr) {
        REQUEST_HILOGE("Get CjApplicationContext failed");
        return;
    }
    context->UnregisterAbilityLifecycleCallback(shared_from_this());
    REQUEST_HILOGD("Unregister foreground resume callback success");
}
} // namespace OHOS::CJSystemapi::Request