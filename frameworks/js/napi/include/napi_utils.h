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

#ifndef DOWNLOAD_NAPI_UTILS_H
#define DOWNLOAD_NAPI_UTILS_H

#include <string>
#include <vector>
#include <map>

#include "js_common.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "constant.h"

namespace OHOS::Request::NapiUtils {
static constexpr int32_t MAX_ARGC = 6;
static constexpr int32_t NO_ARG = 0;
static constexpr int32_t ONE_ARG = 1;
static constexpr int32_t TWO_ARG = 2;
static constexpr int32_t THE_ARG = 3;

static constexpr int32_t FIRST_ARGV = 0;
static constexpr int32_t SECOND_ARGV = 1;
static constexpr int32_t THIRD_ARGV = 2;

static constexpr int32_t MAX_NUMBER_BYTES = 8;
static constexpr int32_t MAX_LEN = 4096;

napi_status Convert2JSValue(napi_env env, bool in, napi_value &out);
napi_status Convert2JSValue(napi_env env, std::string &in, napi_value &out);
napi_status Convert2JSValue(napi_env env, const DownloadInfo &in, napi_value &out);
napi_value Convert2JSValue(napi_env env, int32_t code);
napi_value Convert2JSValue(napi_env env, uint32_t code);
napi_value Convert2JSValue(napi_env env, int64_t code);
napi_value Convert2JSValue(napi_env env, uint64_t code);
napi_value Convert2JSValue(napi_env env, const std::vector<int64_t> &code);
napi_value Convert2JSValue(napi_env env, const std::map<std::string, std::string> &code);
napi_value Convert2JSValue(napi_env env, const std::string &str);
napi_value Convert2JSValue(napi_env env, const std::vector<TaskState> &taskStates);
napi_value Convert2JSValue(napi_env env, const Progress &progress);
napi_value Convert2JSHeaders(napi_env env, const std::map<std::string, std::string> &header);

bool Convert2Boolean(napi_env env, napi_value object, const std::string &propertyName);
uint32_t Convert2Uint32(napi_env env, napi_value value);
uint32_t Convert2Uint32(napi_env env, napi_value object, const std::string &propertyName);
int64_t Convert2Int64(napi_env env, napi_value value);
int64_t Convert2Int64(napi_env env, napi_value object, const std::string &propertyName);
std::string Convert2String(napi_env env, napi_value value);
std::string Convert2String(napi_env env, napi_value object, const std::string &propertyName);

void ThrowError(napi_env env, ExceptionErrorCode code, const std::string &msg, bool withErrCode);
void ConvertError(int32_t errorCode, ExceptionError &err);
napi_value CreateBusinessError(napi_env env, ExceptionErrorCode errorCode,
    const std::string &errorMessage, bool withErrCode);

napi_valuetype GetValueType(napi_env env, napi_value value);
bool HasNamedProperty(napi_env env, napi_value object, const std::string &propertyName);
napi_value GetNamedProperty(napi_env env, napi_value object, const std::string &propertyName);
std::vector<std::string> GetPropertyNames(napi_env env, napi_value object);

void SetUint32Property(napi_env env, napi_value object, const std::string &name, uint32_t value);
void SetInt64Property(napi_env env, napi_value object, const std::string &name, int64_t value);
void SetStringPropertyUtf8(napi_env env, napi_value object, const std::string &name, const std::string &value);
napi_value CreateObject(napi_env env);
napi_value GetUndefined(napi_env env);
napi_value CallFunction(napi_env env, napi_value recv, napi_value func, size_t argc, const napi_value *argv);
std::string ToLower(const std::string &s);
Action GetRequestAction(napi_env env, napi_value configValue);
std::vector<FileSpec> Convert2FileVector(napi_env env, napi_value jsFiles, const std::string &version);
bool Convert2File(napi_env env, napi_value jsFile, FileSpec &file);
std::vector<FormItem> Convert2RequestDataVector(napi_env env, napi_value jsRequestDatas);
FormItem Convert2RequestData(napi_env env, napi_value jsRequestData);
bool IsPathValid(const std::string &filePath);
} // namespace OHOS::Request::NapiUtils
#endif /* DOWNLOAD_NAPI_UTILS_H */
