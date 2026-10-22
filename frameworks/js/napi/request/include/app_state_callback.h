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

#ifndef REQUEST_APP_STATE_CALLBACK_H
#define REQUEST_APP_STATE_CALLBACK_H

#include "application_context.h"

namespace OHOS {
namespace Request {
// App lifecycle callback used to recover from a missing download_server SA:
// when the app returns to the foreground it retries loading the SA if any
// foreground-mode task is waiting, and unregisters itself once no such task
// exists. Registered at most once per process by
// JsTask::RegisterForegroundResume (guarded by JsTask::register_); every
// lifecycle method except OnAbilityForeground is an intentional no-op.
class AppStateCallback
    : public OHOS::AbilityRuntime::AbilityLifecycleCallback, public std::enable_shared_from_this<AppStateCallback> {
public:
    void OnAbilityCreate(const AbilityRuntime::AbilityLifecycleCallbackArgs &ability) override{};

    void OnWindowStageCreate(
        const AbilityRuntime::AbilityLifecycleCallbackArgs &ability,
        const AbilityRuntime::AbilityLifecycleCallbackArgs &windowStage) override
    {
    }

    void OnWindowStageDestroy(
        const AbilityRuntime::AbilityLifecycleCallbackArgs &ability,
        const AbilityRuntime::AbilityLifecycleCallbackArgs &windowStage) override
    {
    }

    void OnWindowStageActive(
        const AbilityRuntime::AbilityLifecycleCallbackArgs &ability,
        const AbilityRuntime::AbilityLifecycleCallbackArgs &windowStage) override
    {
    }

    void OnWindowStageInactive(
        const AbilityRuntime::AbilityLifecycleCallbackArgs &ability,
        const AbilityRuntime::AbilityLifecycleCallbackArgs &windowStage) override
    {
    }

    void OnAbilityDestroy(const AbilityRuntime::AbilityLifecycleCallbackArgs &ability) override
    {
    }

    /// SA-recovery hook (flow in the .cpp): if the SA is still down,
    /// reloads it while a foreground-mode task is waiting, or unregisters
    /// this callback when none is.
    void OnAbilityForeground(const AbilityRuntime::AbilityLifecycleCallbackArgs &ability) override;

    void OnAbilityBackground(const AbilityRuntime::AbilityLifecycleCallbackArgs &ability) override
    {
    }

    void OnAbilityContinue(const AbilityRuntime::AbilityLifecycleCallbackArgs &ability) override
    {
    }
};
} // namespace Request
} // namespace OHOS
#endif