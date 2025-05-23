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

#ifndef OHOS_REQUEST_I_RESPONSE_LISTENER_H
#define OHOS_REQUEST_I_RESPONSE_LISTENER_H

#include "request_common.h"

namespace OHOS::Request {

class IResponseListener {
public:
    virtual ~IResponseListener() = default;
    virtual void OnResponseReceive(const std::shared_ptr<Response> &response) = 0;
};

} // namespace OHOS::Request

#endif // OHOS_REQUEST_I_RESPONSE_LISTENER_H