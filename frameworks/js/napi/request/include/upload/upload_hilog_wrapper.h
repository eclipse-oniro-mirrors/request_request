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

#ifndef UPLOAD_HILOG_WRAPPER_H
#define UPLOAD_HILOG_WRAPPER_H

#include "hilog/log.h"

namespace OHOS::Request::Upload {
// param of log interface, such as UPLOAD_HILOGF.
enum UploadSubModule {
    UPLOAD_MODULE_INNERKIT = 0,
    UPLOAD_MODULE_FRAMEWORK,
    UPLOAD_MODULE_COMMON,
    UPLOAD_MODULE_JS_NAPI,
    UPLOAD_MODULE_TEST,
    UPLOAD_MODULE_BUTT,
};

// 0xD001C50: subsystem:miscservices module:upload_native, 8 bits reserved.
static constexpr unsigned int BASE_UPLOAD_DOMAIN_ID = 0xD001C50;

enum UploadDomainId {
    UPLOAD_INNERKIT_DOMAIN = BASE_UPLOAD_DOMAIN_ID + UPLOAD_MODULE_INNERKIT,
    UPLOAD_FRAMEWORK_DOMAIN,
    UPLOAD_COMMON_DOMAIN,
    UPLOAD_JS_NAPI,
    UPLOAD_TEST,
    UPLOAD_BUTT,
};

static constexpr OHOS::HiviewDFX::HiLogLabel UPLOAD_MODULE_LABEL[UPLOAD_MODULE_BUTT] = {
    { LOG_CORE, UPLOAD_INNERKIT_DOMAIN, "UploadInnerKit" },
    { LOG_CORE, UPLOAD_FRAMEWORK_DOMAIN, "UploadFramework" },
    { LOG_CORE, UPLOAD_COMMON_DOMAIN, "UploadCommon" },
    { LOG_CORE, UPLOAD_JS_NAPI, "UploadJSNAPI" },
    { LOG_CORE, UPLOAD_TEST, "UploadTest" },
};

#define FILENAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define FORMATTED(fmt, ...) "[%{public}s] %{public}s# " fmt, FILENAME, __FUNCTION__, ##__VA_ARGS__

// In order to improve performance, do not check the module range.
// Besides, make sure module is less than UPLOAD_MODULE_BUTT.
#define UPLOAD_HILOGF(module, fmt, ...)                                                                        \
    (void)HILOG_IMPL(LOG_CORE, LOG_FATAL, UPLOAD_MODULE_LABEL[module].domain, UPLOAD_MODULE_LABEL[module].tag, \
        "[%{public}s] %{public}s# " fmt, FILENAME, __FUNCTION__, ##__VA_ARGS__)
#define UPLOAD_HILOGE(module, fmt, ...)                                                                        \
    (void)HILOG_IMPL(LOG_CORE, LOG_ERROR, UPLOAD_MODULE_LABEL[module].domain, UPLOAD_MODULE_LABEL[module].tag, \
        "[%{public}s] %{public}s# " fmt, FILENAME, __FUNCTION__, ##__VA_ARGS__)
#define UPLOAD_HILOGW(module, fmt, ...)                                                                       \
    (void)HILOG_IMPL(LOG_CORE, LOG_WARN, UPLOAD_MODULE_LABEL[module].domain, UPLOAD_MODULE_LABEL[module].tag, \
        "[%{public}s] %{public}s# " fmt, FILENAME, __FUNCTION__, ##__VA_ARGS__)
#define UPLOAD_HILOGI(module, fmt, ...)                                                                       \
    (void)HILOG_IMPL(LOG_CORE, LOG_INFO, UPLOAD_MODULE_LABEL[module].domain, UPLOAD_MODULE_LABEL[module].tag, \
        "[%{public}s] %{public}s# " fmt, FILENAME, __FUNCTION__, ##__VA_ARGS__)
#define UPLOAD_HILOGD(module, fmt, ...)                                                                        \
    (void)HILOG_IMPL(LOG_CORE, LOG_DEBUG, UPLOAD_MODULE_LABEL[module].domain, UPLOAD_MODULE_LABEL[module].tag, \
        "[%{public}s] %{public}s# " fmt, FILENAME, __FUNCTION__, ##__VA_ARGS__)
} // namespace OHOS::Request::Upload
#endif // UPLOAD_HILOG_WRAPPER_H
