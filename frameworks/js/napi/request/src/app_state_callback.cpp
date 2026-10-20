/*
* Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "app_state_callback.h"

#include "ffrt.h"
#include "js_task.h"
#include "log.h"
#include "request_manager.h"
#include "sys_event.h"

namespace OHOS {
namespace Request {
// Runs when the app returns to the foreground. Flow:
// 1. SA already up — nothing to recover.
// 2. SA down and some live FOREGROUND-mode task exists — retry
//    LoadRequestServer (foreground tasks cannot run without the SA).
// 3. SA down but no foreground task left — this callback has no further
//    purpose; unregister itself from the ApplicationContext.
void AppStateCallback::OnAbilityForeground(const AbilityRuntime::AbilityLifecycleCallbackArgs &ability)
{
    if (RequestManager::GetInstance()->IsSaReady()) {
        return;
    }
    SysEventLog::SendSysEventLog(FAULT_EVENT, SAMGR_FAULT_02, "Check SA failed");

    bool hasForeground = false;
    {
        std::lock_guard<std::mutex> lockGuard(JsTask::taskMutex_);
        for (auto it = JsTask::taskContextMap_.begin(); it != JsTask::taskContextMap_.end(); ++it) {
            if (it->second->task == nullptr) {
                continue;
            }
            if (it->second->config.mode == Mode::FOREGROUND) {
                hasForeground = true;
                break;
            }
        }
    }
    if (hasForeground) {
        // Retry loading the SA off-thread; foreground tasks cannot run
        // without it.
        ffrt::submit([]() mutable { RequestManager::GetInstance()->LoadRequestServer(); });
        return;
    }
    if (!JsTask::register_) {
        return;
    }
    // One-shot: reset the guard so a later RegisterForegroundResume can
    // install a fresh callback, then drop this one.
    JsTask::register_ = false;
    auto context = AbilityRuntime::ApplicationContext::GetInstance();
    if (context == nullptr) {
        REQUEST_HILOGE("Get ApplicationContext failed");
        SysEventLog::SendSysEventLog(FAULT_EVENT, ABMS_FAULT_00, "Get AppContext failed");
        return;
    }
    context->UnregisterAbilityLifecycleCallback(shared_from_this());
    REQUEST_HILOGD("Unregister foreground resume callback success");
}
} // namespace Request
} // namespace OHOS