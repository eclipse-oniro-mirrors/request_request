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

#ifndef REQUEST_PRE_DOWNLOAD_NAPI_UTILS_H
#define REQUEST_PRE_DOWNLOAD_NAPI_UTILS_H

#include <vector>

#include "js_native_api.h"
#include "js_native_api_types.h"
#include "napi/native_common.h"
#include "request_preload.h"
namespace OHOS::Request {
// Argument-count shorthands used when reading napi callback info.
static constexpr int32_t NO_ARG = 0;
static constexpr int32_t ONE_ARG = 1;
static constexpr int32_t TWO_ARG = 2;
static constexpr int32_t THE_ARG = 3;

// Runs a napi call and, when it fails, throws a BusinessError with the given
// code and message and returns retVal from the enclosing function.
#define PRELOAD_NAPI_CALL_BASE(env, theCall, code, message, retVal) \
    do {                                                            \
        if ((theCall) != napi_ok) {                                 \
            ThrowError(env, code, message);                         \
            return retVal;                                          \
        }                                                           \
    } while (0)

// PRELOAD_NAPI_CALL_BASE with the E_OTHER error code and a nullptr return
// value, for use in functions returning napi_value.
#define PRELOAD_NAPI_CALL(env, theCall, message) \
    PRELOAD_NAPI_CALL_BASE(env, theCall, E_OTHER, message, nullptr)

// Returns the type of value; a nullptr value is reported as napi_undefined.
napi_valuetype GetValueType(napi_env env, napi_value value);
// Returns the named property of object, or nullptr when it is missing.
napi_value GetNamedProperty(napi_env env, napi_value object, const std::string &propertyName);
// Returns value as a string, or "" when it is not a string.
std::string GetStringValueWithDefault(napi_env env, napi_value value);

// Returns the UTF-8 byte length of the string value (excluding terminator).
size_t GetStringLength(napi_env env, napi_value value);
// Reads the string value into a std::string; length must be its UTF-8 byte
// length as returned by GetStringLength.
std::string GetValueString(napi_env env, napi_value value, size_t length);
// Returns the own property names of object; non-string entries are skipped.
std::vector<std::string> GetPropertyNames(napi_env env, napi_value object);
// Returns the string value of object's named property, or "" when the
// property is missing or not a string.
std::string GetPropertyValue(napi_env env, napi_value object, const std::string &propertyName);
// Returns the value as an int64 number (0 when the call fails).
int64_t GetValueNum(napi_env env, napi_value value);
// Throws a BusinessError carrying code and msg to JS.
void ThrowError(napi_env env, int32_t code, const std::string &msg);
// Sets a string property; silently skipped when the conversion fails, throws
// a plain error when the assignment fails.
void SetStringPropertyUtf8(napi_env env, napi_value object, const std::string &name, const std::string &value);
// Sets a uint32 property; same failure behavior as SetStringPropertyUtf8.
void SetUint32Property(napi_env env, napi_value object, const std::string &name, uint32_t value);
// Converts a std::string to a JS string value, or nullptr on failure.
napi_value Convert2JSValue(napi_env env, const std::string &str);
// Converts a uint32 to a JS number value, or nullptr on failure.
napi_value Convert2JSValue(napi_env env, uint32_t code);
} // namespace OHOS::Request
#endif
