/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_REQUEST_UPLOAD_I_CALLBACKABLE_JUDGER
#define OHOS_REQUEST_UPLOAD_I_CALLBACKABLE_JUDGER
#include "i_notify_callback.h"
#include "i_progress_callback.h"
#include "i_header_receive_callback.h"

namespace OHOS::Request::Upload {
class ICallbackAbleJudger {
public:
    ICallbackAbleJudger() = default;
    virtual ~ICallbackAbleJudger() {}
    virtual bool JudgeNotify(const INotifyCallback *target) = 0;
    virtual bool JudgeProgress(const IProgressCallback *target) = 0;
    virtual bool JudgeHeaderReceive(const IHeaderReceiveCallback *target) = 0;
};
} // end of OHOS::Request::Upload
#endif