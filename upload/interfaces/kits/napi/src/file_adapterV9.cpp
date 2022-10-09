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

#include "file_adapterV9.h"

using namespace OHOS::AppExecFwk;
namespace OHOS::Request::Upload {
int32_t FileAdapterV9::DataAbilityOpenFile(std::string &fileUri,
    std::shared_ptr<OHOS::AbilityRuntime::Context> &context)
{
    std::shared_ptr<Uri> uri = std::make_shared<Uri>(fileUri);
    std::shared_ptr<DataAbilityHelper> dataAbilityHelper = DataAbilityHelper::Creator(context, uri);
    return dataAbilityHelper->OpenFile(*uri, "r");
}

std::string FileAdapterV9::InternalGetFilePath(std::shared_ptr<OHOS::AbilityRuntime::Context> &context)
{
    return context->GetCacheDir();
}
} // end of OHOS::Request::Upload