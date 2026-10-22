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

#ifndef REQUEST_NAPI_UTILS_H
#define REQUEST_NAPI_UTILS_H

#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "constant.h"
#include "ffrt.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "openssl/sha.h"
#include "parameter.h"
#include "request_common.h"
#include "utf8_utils.h"

// Placeholder retVal for the REQUEST_NAPI_* macros in void functions:
// "return RETVAL_NOTHING;" expands to a plain return.
#define RETVAL_NOTHING

// JS <-> C++ conversion layer of the NAPI module: value converters, object
// builders for the shared IPC types (TaskInfo/Progress/Config/FileSpec...),
// the inner-to-JS error mapping, and the check-throw-return macros used by
// the entry functions.
namespace OHOS::Request::NapiUtils {
// Argument counts expected by the simple entry functions (js_initialize,
// request_event) and indexes into the fetched argument vector. MAX_ARGC
// caps how many slots are fetched; extra arguments are ignored.
static constexpr int32_t MAX_ARGC = 6;
static constexpr int32_t NO_ARG = 0;
static constexpr int32_t ONE_ARG = 1;
static constexpr int32_t TWO_ARG = 2;
static constexpr int32_t THE_ARG = 3;
static constexpr int32_t ONE_REF = 1;

static constexpr int32_t FIRST_ARGV = 0;
static constexpr int32_t SECOND_ARGV = 1;
static constexpr int32_t THIRD_ARGV = 2;

static constexpr int32_t MAX_NUMBER_BYTES = 8;
static constexpr int32_t MAX_LEN = 4096;
// Cap for converting a JS string to std::string; longer strings are
// truncated to 64 KB (see Convert2String).
static constexpr int32_t MAX_STRING_LENGTH = 65536;

// Check-throw-return helpers for NAPI entry code. REQUEST_NAPI_ASSERT
// evaluates an assertion, REQUEST_NAPI_CALL* checks a napi_status result;
// on failure both throw the given code/message to JS and return retVal.
// REQUEST_GET_AND_THROW_LAST_ERROR rethrows a pending napi exception when
// no JS exception is already waiting.
#define REQUEST_NAPI_ASSERT_BASE(env, assertion, code, message, retVal) \
    do {                                                         \
        if (!(assertion)) {                                      \
            NapiUtils::ThrowError(env, code, message, true);     \
            return retVal;                                       \
        }                                                        \
    } while (0)

#define REQUEST_NAPI_ASSERT(env, message, assertion) \
    REQUEST_NAPI_ASSERT_BASE(env, assertion, E_OTHER, message, nullptr)


#define REQUEST_NAPI_CALL_BASE(env, theCall, code, message, retVal)  \
    do {                                                             \
        if ((theCall) != napi_ok) {                                  \
            NapiUtils::ThrowError(env, code, message, true);         \
            return retVal;                                           \
        }                                                            \
    } while (0)

#define REQUEST_NAPI_CALL(env, theCall, message) \
    REQUEST_NAPI_CALL_BASE(env, theCall, E_OTHER, message, nullptr)

#define REQUEST_NAPI_CALL_RETURN(env, theCall, message, retVal) \
    REQUEST_NAPI_CALL_BASE(env, theCall, E_OTHER, message, retVal)

#define REQUEST_NAPI_CALL_RETURN_VOID(env, theCall, message) \
    REQUEST_NAPI_CALL_BASE(env, theCall, E_OTHER, message, RETVAL_NOTHING)


#define REQUEST_GET_AND_THROW_LAST_ERROR_BASE(env, code, message)                                       \
    do {                                                                                                \
        const napi_extended_error_info* errorInfo = nullptr;                                            \
        napi_get_last_error_info((env), &errorInfo);                                                    \
        bool isPending = false;                                                                         \
        napi_is_exception_pending((env), &isPending);                                                   \
        if (!isPending && errorInfo != nullptr) {                                                       \
            const char* errorMessage =                                                                  \
                errorInfo->error_message != nullptr ? errorInfo->error_message : "empty error message"; \
            NapiUtils::ThrowError(env, code, message, true);                                            \
        }                                                                                               \
    } while (0)

#define REQUEST_GET_AND_THROW_LAST_ERROR(env, message) \
    REQUEST_GET_AND_THROW_LAST_ERROR_BASE(env, E_OTHER, message)

// C++ -> JS conversions. Two shapes coexist: the older
// napi_status(env, in, &out) overloads and the napi_value(env, in)
// overloads, which return nullptr on failure. The shared-type overloads
// build JS objects with the exact property names of the public d.ts —
// those names are part of the JS API contract.
napi_status Convert2JSValue(napi_env env, bool in, napi_value &out);
napi_status Convert2JSValue(napi_env env, std::string &in, napi_value &out);
/// Legacy API9 download info; note the legacy property names
/// (downloadId, failedReason, pausedReason, ...).
napi_status Convert2JSValue(napi_env env, const DownloadInfo &in, napi_value &out);
napi_value Convert2JSValue(napi_env env, bool code);
napi_value Convert2JSValue(napi_env env, int32_t code);
napi_value Convert2JSValue(napi_env env, uint32_t code);
napi_value Convert2JSValue(napi_env env, int64_t code);
napi_value Convert2JSValue(napi_env env, uint64_t code);
napi_value Convert2JSValue(napi_env env, const std::vector<int64_t> &code);
napi_value Convert2JSValue(napi_env env, const std::vector<int32_t> &code);
napi_value Convert2JSValue(napi_env env, const std::vector<std::string> &ids);
napi_value Convert2JSValue(napi_env env, const std::map<std::string, std::string> &code);
napi_value Convert2JSValue(napi_env env, const std::string &str);
napi_value Convert2JSValue(napi_env env, const std::vector<TaskState> &taskStates);
napi_value Convert2JSValue(napi_env env, const Progress &progress);
napi_value Convert2JSValue(napi_env env, TaskInfo &taskInfo);
napi_value Convert2JSValue(napi_env env, const Reason reason);
/// WaitingReason is exposed as its raw numeric value.
napi_value Convert2JSValue(napi_env env, WaitingReason reason);
/// Serializes a task-creation Config back to a JS object.
napi_value Convert2JSValueConfig(napi_env env, Config &config);
napi_value Convert2JSValue(napi_env env, const std::shared_ptr<Response> &response);
napi_value Convert2JSValue(napi_env env, const std::vector<FileSpec> &files, const std::vector<FormItem> &forms);
napi_value Convert2JSValue(napi_env env, const MinSpeed &minSpeed);
/// Headers with multi-values: builds a real JS Map (name -> value array)
/// through the global Map constructor.
napi_value Convert2JSHeaders(napi_env env, const std::map<std::string, std::vector<std::string>> &header);
/// Headers object plus body. The body becomes a string when it is valid
/// UTF-8, an ArrayBuffer otherwise; isSeparate wraps both in one
/// {headers, body} object, otherwise the body is attached inside the
/// headers object (progress.extras shape).
napi_value Convert2JSHeadersAndBody(napi_env env, const std::map<std::string, std::string> &header,
    const std::vector<uint8_t> &bodyBytes, bool isSeparate);

// JS -> C++ conversions. Lenient by design: a missing property or a
// wrong-typed value yields the zero value of the target type instead of
// failing the call.
bool Convert2Boolean(napi_env env, napi_value object, const std::string &propertyName);
uint32_t Convert2Uint32(napi_env env, napi_value value);
uint32_t Convert2Uint32(napi_env env, napi_value object, const std::string &propertyName);
int32_t Convert2Int32(napi_env env, napi_value value);
int64_t Convert2Int64(napi_env env, napi_value value);
int64_t Convert2Int64(napi_env env, napi_value object, const std::string &propertyName);
std::string Convert2String(napi_env env, napi_value value);
std::string Convert2String(napi_env env, napi_value object, const std::string &propertyName);

// Error mapping. ConvertError rewrites inner service codes into the
// JS-visible ExceptionErrorCode set; CreateBusinessError builds the error
// object (preset message + custom text, optionally with a numeric code
// property); ThrowError additionally raises it on the JS side.
void ThrowError(napi_env env, ExceptionErrorCode code, const std::string &msg, bool withErrCode);
void ConvertError(int32_t errorCode, ExceptionError &err);
napi_value CreateBusinessError(
    napi_env env, ExceptionErrorCode errorCode, const std::string &errorMessage, bool withErrCode);

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
/// Legacy entry dispatch: an options object carrying method/files/data
/// means UPLOAD, otherwise DOWNLOAD.
Action GetRequestAction(napi_env env, napi_value configValue);
/// Converts the JS files array; elements that fail validation are skipped,
/// not fatal.
std::vector<FileSpec> Convert2FileVector(napi_env env, napi_value jsFiles, const std::string &version);
/// Requires filename/name/uri/type to be present; an empty "type" leaves
/// hasContentType false.
bool Convert2File(napi_env env, napi_value jsFile, FileSpec &file);
std::vector<FormItem> Convert2RequestDataVector(napi_env env, napi_value jsRequestDatas);
FormItem Convert2RequestData(napi_env env, napi_value jsRequestData);
/// Returns the save path of a download task (first file's uri); empty for
/// uploads or when no file exists.
std::string GetSaveas(const std::vector<FileSpec> &files, Action action);
/// Checks the directory part of the path is canonical (realpath must
/// resolve to itself), rejecting symlinked or missing parents; the trailing
/// file name itself is not validated.
bool IsPathValid(const std::string &filePath);
/// SHA-256 digest as a lowercase hex string (openssl).
std::string SHA256(const char *str, size_t len);
/// Reads the whole file into fileData; leaves it empty (with a warning
/// log) when the file cannot be opened.
void ReadBytesFromFile(const std::string &filePath, std::vector<uint8_t> &fileData);
/// Deletes the file asynchronously on an FFRT task.
void RemoveFile(const std::string &filePath);
} // namespace OHOS::Request::NapiUtils
#endif /* REQUEST_NAPI_UTILS_H */
