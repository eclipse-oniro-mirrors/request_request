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

#ifndef I_PROGRESS_CALLBACK
#define I_PROGRESS_CALLBACK

#include "napi/native_api.h"

namespace OHOS::Request::Upload {
class IProgressCallback {
public:
    IProgressCallback() = default;
    virtual ~IProgressCallback() {};
    virtual void Progress(const int64_t uploadedSize, const int64_t totalSize) = 0;
    virtual napi_ref GetCallback() = 0;
};
} // end of OHOS::Request::Upload
#endif