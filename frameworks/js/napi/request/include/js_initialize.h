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

#ifndef REQUEST_JS_INITIALIZE_H
#define REQUEST_JS_INITIALIZE_H

#include "ability.h"
#include "data_ability_helper.h"
#include "directory_ex.h"
#include "js_task.h"
#include "napi_base_context.h"

namespace OHOS::Request {
// Token length bounds (bytes) accepted by request.agent getTask/touch.
static constexpr uint32_t TOKEN_MAX_BYTES = 2048;
static constexpr uint32_t TOKEN_MIN_BYTES = 8;
// The app sandbox base directories of the three distribution areas — the
// same roots the PathUtils ACL grants operate on.
static const std::string AREA1 = "/data/storage/el1/base";
static const std::string AREA2 = "/data/storage/el2/base";
static const std::string AREA5 = "/data/storage/el5/base";

/// Extracts the host from a URL (lenient: scheme optional, backslashes
/// tolerated; stops at port/path/query).
std::string GetHostnameFromURL(const std::string &url);

// Native-constructor plumbing for the JS Task classes. Initialize is the
// constructor napi_new_instance invokes (through JsTask::Create /
// RequestFile / RequestFileV8 / GetTaskCreate): it validates the caller's
// arguments, parses the config and wraps a JsTask into the JS instance.
// The remaining statics are context/path helpers shared with the parsers
// in the sibling files (see the private section for the split).
class JsInitialize {
public:
    JsInitialize() = default;
    ~JsInitialize() = default;

    /// Native Task constructor: parses (context, config) — API8 passes the
    /// config alone — into a new JsTask and wraps it into the JS instance
    /// being constructed. firstInit=false (the getTask path) skips checks
    /// that must only run on first construction.
    static napi_value Initialize(napi_env env, napi_callback_info info, Version version, bool firstInit = true);
    /// Resolves the caller's ability context: a stage-model context value
    /// directly, otherwise the FA-model current ability.
    static napi_status GetContext(
        napi_env env, napi_value value, std::shared_ptr<OHOS::AbilityRuntime::Context> &context);
    /// Fills baseDir with the application-level base directory.
    static bool GetAppBaseDir(std::string &baseDir);
    /// Whether filepath starts with one of the app-storage area prefixes
    /// (see the .cpp: the per-app baseDir is filled in but never compared
    /// against the path).
    static bool CheckBelongAppBaseDir(const std::string &filepath, std::string &baseDir);
    /// Splits str on delim, skipping empty segments.
    static void StringSplit(const std::string &str, const char delim, std::vector<std::string> &elems);
    /// Trims spaces from both ends of str.
    static void StringTrim(std::string &str);
    /// Creates the directory chain given as single-segment elements.
    static bool CreateDirs(const std::vector<std::string> &pathDirs);
    /// Whether the directory exists.
    static bool FindDir(const std::string &pathDir);

private:
    // Private helpers, split across the sibling .cpp files:
    // - js_initialize_config.cpp: the Parse* field readers, the
    //   version-dispatching entry points (ParseConfig API10, ParseConfigV9
    //   API8/9) and the forms and files conversion (GetFormItems and Convert2 helpers);
    // - js_initialize_file.cpp: the per-task-type file checks
    //   (CheckUpload*/CheckDownload*/GetFd*/...) behind CheckFilePath;
    // - js_initialize_path.cpp: sandbox path standardization
    //   (StandardizePath/BaseToWhole/CacheToWhole/FileToWhole/...ToNormal).
    static ExceptionError InitParam(
        napi_env env, napi_value *argv, std::shared_ptr<OHOS::AbilityRuntime::Context> &context, Config &config);
    static void ParseConfigInner(napi_env env, napi_value jsConfig, Config &config);
    static bool ParseConfig(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseConfigV9(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static void SetParseConfig(napi_env env, napi_value jsConfig, Config &config);
    static bool ParseUploadConfig(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseDownloadConfig(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseAction(napi_env env, napi_value jsConfig, Action &action, std::string &errInfo);
    static bool ParseUrl(napi_env env, napi_value jsConfig, std::string &url, std::string &errInfo);
    static bool ParseNotification(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseNotificationWantAgent(napi_env env, napi_value notification, Config &config);
    static bool ParseMinSpeed(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseProxy(napi_env env, napi_value jsConfig, std::string &proxy, std::string &errInfo);
    static bool ParseCertsPath(
        napi_env env, napi_value jsConfig, std::vector<std::string> &certsPath, std::string &errInfo);
    static bool ParseData(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseIndex(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseName(napi_env env, napi_value jsVal, std::string &name);
    static bool ParseTitle(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static void ParseNetwork(napi_env env, napi_value jsConfig, Network &network);
    static void ParseCertificatePins(napi_env env, std::string &url, std::string &certificatePins);
    static void ParseMethod(napi_env env, napi_value jsConfig, Config &config);
    static void ParseRedirect(napi_env env, napi_value jsConfig, bool &redirect);
    static void ParseRoaming(napi_env env, napi_value jsConfig, Config &config);
    static void ParseRetry(napi_env env, napi_value jsConfig, bool &retry);
    static void ParseGauge(napi_env env, napi_value jsConfig, Config &config);
    static bool ParseSaveas(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseToken(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseTimeout(napi_env env, napi_value jsConfig, Config &config, std::string &errInfo);
    static bool ParseDescription(napi_env env, napi_value jsConfig, std::string &description, std::string &errInfo);
    static int64_t ParseEnds(napi_env env, napi_value jsConfig);
    static int64_t ParseBegins(napi_env env, napi_value jsConfig);
    static uint32_t ParsePriority(napi_env env, napi_value jsConfig);
    static std::map<std::string, std::string> ParseMap(
        napi_env env, napi_value jsConfig, const std::string &propertyName);

    static bool GetFormItems(
        napi_env env, napi_value jsVal, std::vector<FormItem> &forms, std::vector<FileSpec> &files);
    static bool Convert2FormItems(napi_env env, napi_value jsValue, std::vector<FormItem> &forms,
        std::vector<FileSpec> &files, std::string &errInfo);
    static bool Convert2FileSpecs(
        napi_env env, napi_value jsValue, const std::string &name, std::vector<FileSpec> &files);
    static bool Convert2FileSpec(napi_env env, napi_value jsValue, const std::string &name, FileSpec &file);
    static bool GetInternalPath(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const Config &config,
        std::string &path, std::string &errInfo);

    static bool CheckUploadBodyFiles(const std::string &filePath, Config &config, ExceptionError &error);
    static bool CheckPathIsFile(const std::string &path, ExceptionError &error);
    static bool CheckPathOverWrite(const std::string &path, const Config &config, ExceptionError &error);
    static bool GetFdUpload(const std::string &path, const Config &config, ExceptionError &error);
    static bool GetFdDownload(const std::string &path, const Config &config, ExceptionError &error);
    static void StandardizePathApi9(std::string &path);
    static bool InterceptData(const std::string &str, const std::string &in, std::string &out);
    static bool IsStageMode(napi_env env, napi_value value);
    static bool CheckDownloadFilePath(
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, Config &config, std::string &errInfo);
    static bool StandardizePath(
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const Config &config, std::string &path);
    static bool BaseToWhole(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, std::string &path);
    static bool CacheToWhole(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, std::string &path);
    static bool FileToWhole(
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const Config &config, std::string &path);
    static bool WholeToNormal(std::string &path, std::vector<std::string> &out);
    static bool PathVecToNormal(const std::vector<std::string> &in, std::vector<std::string> &out);
    static bool IsUserFile(const std::string &filePath);
    static void StandardizeFileSpec(FileSpec &file);
    static bool GetSandboxPath(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const Config &config,
        std::string &path, std::vector<std::string> &pathVec, std::string &errInfo);
    static bool CheckUserFileSpec(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, const Config &config,
        FileSpec &file, ExceptionError &error, bool isUpload);
    static bool CheckUploadFileSpec(const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, Config &config,
        FileSpec &file, ExceptionError &error);
    static bool CheckDownloadFile(
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, Config &config, ExceptionError &error);
    static bool CheckUploadFiles(
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, Config &config, ExceptionError &error);
    static bool CheckFilePath(
        const std::shared_ptr<OHOS::AbilityRuntime::Context> &context, Config &config, ExceptionError &error);
};
} // namespace OHOS::Request
#endif // REQUEST_JS_INITIALIZE_H
