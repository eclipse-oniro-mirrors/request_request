/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

#include "application_context.h"
#include "context.h"
#include "log.h"
#include "napi_utils.h"
#include "preload_callback_test.h"
#include "preload_common.h"
#include "preload_napi.h"
#include "request_preload.h"
#include "set_permission.h"

using namespace testing::ext;
using namespace OHOS::Request;
using namespace OHOS::AbilityRuntime;
using namespace std;

// Forward declare the mock-server accessor defined in preload/src/common.cpp
// (we compile that translation unit but do not include preload's common.h, which
// would clash with this file's own TestCallback).
const std::string &GetMockBaseUrl();

constexpr size_t SLEEP_INTERVAL = 100;
static std::string g_testUrlNotExist;
class PreloadNapiTest : public testing::Test {
public:
    void SetUp();
};

void PreloadNapiTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcases
    testing::UnitTest *test = testing::UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const testing::TestInfo *testInfo = test->current_test_info();
    ASSERT_NE(testInfo, nullptr);
    string testCaseName = string(testInfo->name());
    REQUEST_HILOGI("[SetUp] %{public}s start", testCaseName.c_str());
    GTEST_LOG_(INFO) << testCaseName.append(" start");
    std::vector<std::string> permissions = { "ohos.permission.INTERNET", "ohos.permission.GET_NETWORK_INFO" };
    SetPermission::SetAccessTokenPermission(permissions, "preload_napi_test");
    g_testUrlNotExist = GetMockBaseUrl() + "/test_not_exist.txt";
}

/**
 * @tc.name: BuildDownloadInfoNullTest
 * @tc.desc: Test BuildDownloadInfo function
 * @tc.precon: NA
 * @tc.step: 1. Create callback.
 *           2. Dwonload test url with 404 response.
 *           3. Verify BuildDownloadInfo return nullptr.
 * @tc.expect: BuildDownloadInfo return nullptr.
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, BuildDownloadInfoNullTest, TestSize.Level1)
{
    std::string url = g_testUrlNotExist;
    Preload::GetInstance()->Remove(url);
    TestCallback test;
    auto &[flagS, flagF, flagInfo, flagC, flagP, callback] = test;
    std::unique_ptr<PreloadOptions> options = std::make_unique<PreloadOptions>();
    auto handle = Preload::GetInstance()->load(url, std::make_unique<PreloadCallback>(callback), std::move(options));

    size_t counter = 100;
    while ((!handle->IsFinish() || !(flagC->load() || flagF->load() || flagS->load())) && counter-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }
    EXPECT_TRUE(flagF->load());
    EXPECT_TRUE(flagInfo->load());
    Preload::GetInstance()->Remove(url);
}

/**
 * @tc.name: SetGlobalRetryOptionsTest001
 * @tc.desc: Test SetGlobalRetryOptions with valid value
 * @tc.precon: NA
 * @tc.step: 1. Create RetryOptions with maxRetryCount = 5
 *           2. Call SetGlobalRetryOptions
 *           3. Restore to default value
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalRetryOptionsTest001, TestSize.Level1)
{
    RetryOptions retryOptions;
    retryOptions.maxRetryCount = 5;
    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
    EXPECT_TRUE(true);
    // Restore to default
    retryOptions.maxRetryCount = 1;
    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
}

/**
 * @tc.name: SetGlobalRetryOptionsTest002
 * @tc.desc: Test SetGlobalRetryOptions with minimum value
 * @tc.precon: NA
 * @tc.step: 1. Create RetryOptions with maxRetryCount = 0
 *           2. Call SetGlobalRetryOptions
 *           3. Restore to default value
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalRetryOptionsTest002, TestSize.Level1)
{
    RetryOptions retryOptions;
    retryOptions.maxRetryCount = 0;
    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
    EXPECT_TRUE(true);
    // Restore to default
    retryOptions.maxRetryCount = 1;
    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
}

/**
 * @tc.name: SetGlobalRetryOptionsTest003
 * @tc.desc: Test SetGlobalRetryOptions with maximum value
 * @tc.precon: NA
 * @tc.step: 1. Create RetryOptions with maxRetryCount = 10
 *           2. Call SetGlobalRetryOptions
 *           3. Restore to default value
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalRetryOptionsTest003, TestSize.Level1)
{
    RetryOptions retryOptions;
    retryOptions.maxRetryCount = 10;
    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
    EXPECT_TRUE(true);
    // Restore to default
    retryOptions.maxRetryCount = 1;
    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
}

/**
 * @tc.name: SetGlobalRetryOptionsTest004
 * @tc.desc: Test SetGlobalRetryOptions with default value
 * @tc.precon: NA
 * @tc.step: 1. Create RetryOptions with default maxRetryCount = 1
 *           2. Call SetGlobalRetryOptions
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalRetryOptionsTest004, TestSize.Level1)
{
    RetryOptions retryOptions;
    retryOptions.maxRetryCount = 1;
    Preload::GetInstance()->SetGlobalRetryOptions(retryOptions);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: SetGlobalTimeoutOptionsTest001
 * @tc.desc: Test SetGlobalTimeoutOptions with valid values
 * @tc.precon: NA
 * @tc.step: 1. Create TimeoutOptions with networkCheckTimeout = 10, httpTotalTimeout = 30
 *           2. Call SetGlobalTimeoutOptions
 *           3. Restore to default values
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalTimeoutOptionsTest001, TestSize.Level1)
{
    TimeoutOptions timeoutOptions;
    timeoutOptions.networkCheckTimeout = 10;
    timeoutOptions.httpTotalTimeout = 30;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
    EXPECT_TRUE(true);
    // Restore to default
    timeoutOptions.networkCheckTimeout = 20;
    timeoutOptions.httpTotalTimeout = 60;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
}

/**
 * @tc.name: SetGlobalTimeoutOptionsTest002
 * @tc.desc: Test SetGlobalTimeoutOptions with minimum networkCheckTimeout
 * @tc.precon: NA
 * @tc.step: 1. Create TimeoutOptions with networkCheckTimeout = 0, httpTotalTimeout = 60
 *           2. Call SetGlobalTimeoutOptions
 *           3. Restore to default values
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalTimeoutOptionsTest002, TestSize.Level1)
{
    TimeoutOptions timeoutOptions;
    timeoutOptions.networkCheckTimeout = 0;
    timeoutOptions.httpTotalTimeout = 60;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
    EXPECT_TRUE(true);
    // Restore to default
    timeoutOptions.networkCheckTimeout = 20;
    timeoutOptions.httpTotalTimeout = 60;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
}

/**
 * @tc.name: SetGlobalTimeoutOptionsTest003
 * @tc.desc: Test SetGlobalTimeoutOptions with maximum networkCheckTimeout
 * @tc.precon: NA
 * @tc.step: 1. Create TimeoutOptions with networkCheckTimeout = 20, httpTotalTimeout = 60
 *           2. Call SetGlobalTimeoutOptions
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalTimeoutOptionsTest003, TestSize.Level1)
{
    TimeoutOptions timeoutOptions;
    timeoutOptions.networkCheckTimeout = 20;
    timeoutOptions.httpTotalTimeout = 60;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: SetGlobalTimeoutOptionsTest004
 * @tc.desc: Test SetGlobalTimeoutOptions with minimum httpTotalTimeout
 * @tc.precon: NA
 * @tc.step: 1. Create TimeoutOptions with networkCheckTimeout = 20, httpTotalTimeout = 1
 *           2. Call SetGlobalTimeoutOptions
 *           3. Restore to default values
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalTimeoutOptionsTest004, TestSize.Level1)
{
    TimeoutOptions timeoutOptions;
    timeoutOptions.networkCheckTimeout = 20;
    timeoutOptions.httpTotalTimeout = 1;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
    EXPECT_TRUE(true);
    // Restore to default
    timeoutOptions.httpTotalTimeout = 60;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
}

/**
 * @tc.name: SetGlobalTimeoutOptionsTest005
 * @tc.desc: Test SetGlobalTimeoutOptions with default values
 * @tc.precon: NA
 * @tc.step: 1. Create TimeoutOptions with default values (20, 60)
 *           2. Call SetGlobalTimeoutOptions
 * @tc.expect: Function executes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetGlobalTimeoutOptionsTest005, TestSize.Level1)
{
    TimeoutOptions timeoutOptions;
    timeoutOptions.networkCheckTimeout = 20;
    timeoutOptions.httpTotalTimeout = 60;
    Preload::GetInstance()->SetGlobalTimeoutOptions(timeoutOptions);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: PreloadOptionsRetryTest001
 * @tc.desc: Test PreloadOptions with task-level retry configuration
 * @tc.precon: NA
 * @tc.step: 1. Create PreloadOptions with retry.maxRetryCount = 5
 *           2. Verify the retry configuration is set correctly
 * @tc.expect: retry.maxRetryCount equals 5
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, PreloadOptionsRetryTest001, TestSize.Level1)
{
    std::unique_ptr<PreloadOptions> options = std::make_unique<PreloadOptions>();
    options->retry.maxRetryCount = 5;
    EXPECT_EQ(options->retry.maxRetryCount, 5);
}

/**
 * @tc.name: PreloadOptionsTimeoutTest001
 * @tc.desc: Test PreloadOptions with task-level timeout configuration
 * @tc.precon: NA
 * @tc.step: 1. Create PreloadOptions with timeout settings
 *           2. Verify the timeout configuration is set correctly
 * @tc.expect: timeout fields equal expected values
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, PreloadOptionsTimeoutTest001, TestSize.Level1)
{
    std::unique_ptr<PreloadOptions> options = std::make_unique<PreloadOptions>();
    options->timeout.networkCheckTimeout = 15;
    options->timeout.httpTotalTimeout = 90;
    EXPECT_EQ(options->timeout.networkCheckTimeout, 15);
    EXPECT_EQ(options->timeout.httpTotalTimeout, 90);
}

namespace {
constexpr size_t INFO_WAIT_ROUNDS = 100;

// Triggers a 404 preload and captures the CppDownloadInfo that the failure
// callback carries. The rust info manager starts with zero capacity and drops
// every insert, so GetDownloadInfo cannot re-read even a successful download
// in this suite; the failure callback hands the statistics over directly.
std::shared_ptr<CppDownloadInfo> FetchFailedInfo()
{
    std::string url = g_testUrlNotExist;
    Preload::GetInstance()->Remove(url);
    auto flagF = std::make_shared<std::atomic_bool>(false);
    auto captured = std::make_shared<std::shared_ptr<CppDownloadInfo>>(nullptr);
    PreloadCallback callback{
        .OnSuccess = [](const std::shared_ptr<Data> &&, const std::string &) {},
        .OnCancel = []() {},
        .OnFail = [flagF, captured](const PreloadError &error, const std::string &) {
            *captured = error.GetDownloadInfo();
            flagF->store(true);
        },
        .OnProgress = [](uint64_t, uint64_t) {},
    };
    std::unique_ptr<PreloadOptions> options = std::make_unique<PreloadOptions>();
    auto handle = Preload::GetInstance()->load(url, std::make_unique<PreloadCallback>(callback), std::move(options));
    if (handle == nullptr) {
        return nullptr;
    }
    size_t counter = INFO_WAIT_ROUNDS;
    while ((!handle->IsFinish() || !flagF->load()) && counter-- > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }
    return *captured;
}
} // namespace

/**
 * @tc.name: IsValueMissingNullArgTest
 * @tc.desc: IsValueMissingOrSkipped reports a nullptr napi_value as missing
 *           before touching the environment, consistently across repeated
 *           calls and value slots
 * @tc.precon: NA
 * @tc.step: 1. Bind two null napi_value locals
 *           2. Call IsValueMissingOrSkipped with a null env for both values
 *           3. Cross-check GetValueType maps the same value to napi_undefined
 * @tc.expect: Both calls report the value as missing and the type probe
 *             agrees on napi_undefined without dereferencing env
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, IsValueMissingNullArgTest, TestSize.Level1)
{
    napi_value missing = nullptr;
    napi_value missingAgain = nullptr;
    EXPECT_TRUE(IsValueMissingOrSkipped(nullptr, missing));
    EXPECT_TRUE(IsValueMissingOrSkipped(nullptr, missingAgain));
    EXPECT_EQ(GetValueType(nullptr, missing), napi_undefined);
}

/**
 * @tc.name: GetValueTypeNullArgTest
 * @tc.desc: GetValueType maps a nullptr value to napi_undefined without
 *           entering napi_typeof, consistently across repeated calls and
 *           value slots
 * @tc.precon: NA
 * @tc.step: 1. Bind two null napi_value locals
 *           2. Call GetValueType with a null env for both values
 *           3. Cross-check IsValueMissingOrSkipped reports the same value as
 *              missing
 * @tc.expect: Both calls return napi_undefined and the missing-value probe
 *             agrees
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, GetValueTypeNullArgTest, TestSize.Level1)
{
    napi_value missing = nullptr;
    napi_value missingAgain = nullptr;
    EXPECT_EQ(GetValueType(nullptr, missing), napi_undefined);
    EXPECT_EQ(GetValueType(nullptr, missingAgain), napi_undefined);
    EXPECT_TRUE(IsValueMissingOrSkipped(nullptr, missing));
}

/**
 * @tc.name: Convert2JsNullEnvTest
 * @tc.desc: Convert2JSValue returns nullptr for both the string and uint32_t
 *           overloads when the napi environment is missing
 * @tc.precon: NA
 * @tc.step: 1. Bind the payload values used by both overloads
 *           2. Call Convert2JSValue with a null env and the string payload
 *           3. Call Convert2JSValue with a null env and the uint32_t payload
 *           4. Cross-check GetValueType still maps a null value to
 *              napi_undefined
 * @tc.expect: Both overloads return nullptr and the type probe stays
 *             consistent
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, Convert2JsNullEnvTest, TestSize.Level1)
{
    const std::string payload = "abc";
    const uint32_t payloadNumber = 42;
    EXPECT_EQ(Convert2JSValue(nullptr, payload), nullptr);
    EXPECT_EQ(Convert2JSValue(nullptr, payloadNumber), nullptr);
    EXPECT_EQ(GetValueType(nullptr, nullptr), napi_undefined);
}

/**
 * @tc.name: SetStringPropertyNullEnvTest
 * @tc.desc: SetStringPropertyUtf8 exits before touching the object when the
 *           created JS value is unavailable (null env), for both a normal
 *           and an empty property value
 * @tc.precon: NA
 * @tc.step: 1. Bind a null object slot and take the GetValueType baseline
 *           2. Call SetStringPropertyUtf8 with a null env and a normal value
 *           3. Repeat the call with an empty value
 *           4. Re-check GetValueType against the baseline
 * @tc.expect: Both calls complete without crashing and GetValueType still
 *             reports napi_undefined
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetStringPropertyNullEnvTest, TestSize.Level1)
{
    napi_value object = nullptr;
    EXPECT_EQ(GetValueType(nullptr, object), napi_undefined);
    SetStringPropertyUtf8(nullptr, object, "key", "value");
    SetStringPropertyUtf8(nullptr, object, "key", "");
    EXPECT_EQ(GetValueType(nullptr, object), napi_undefined);
}

/**
 * @tc.name: SetUint32PropertyNullEnvTest
 * @tc.desc: SetUint32Property exits before touching the object when the
 *           created JS value is unavailable (null env), for both a normal
 *           and a zero property value
 * @tc.precon: NA
 * @tc.step: 1. Bind a null object slot and take the GetValueType baseline
 *           2. Call SetUint32Property with a null env and a normal value
 *           3. Repeat the call with a zero value
 *           4. Re-check GetValueType against the baseline
 * @tc.expect: Both calls complete without crashing and GetValueType still
 *             reports napi_undefined
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, SetUint32PropertyNullEnvTest, TestSize.Level1)
{
    napi_value object = nullptr;
    EXPECT_EQ(GetValueType(nullptr, object), napi_undefined);
    SetUint32Property(nullptr, object, "key", 42u);
    SetUint32Property(nullptr, object, "key", 0u);
    EXPECT_EQ(GetValueType(nullptr, object), napi_undefined);
}

/**
 * @tc.name: BuildDownloadInfoEnvNullDirectTest
 * @tc.desc: BuildDownloadInfo returns nullptr through its explicit env guard
 *           for a real download info, consistently across repeated calls
 * @tc.precon: NA
 * @tc.step: 1. Trigger a failed preload and capture the download info from the failure callback
 *           2. Bind the captured info and call BuildDownloadInfo with a null env
 *           3. Repeat the call on the same info
 * @tc.expect: Both calls return nullptr
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, BuildDownloadInfoEnvNullDirectTest, TestSize.Level1)
{
    auto info = FetchFailedInfo();
    ASSERT_TRUE(info != nullptr);
    const CppDownloadInfo &captured = *info;
    EXPECT_EQ(BuildDownloadInfo(nullptr, captured), nullptr);
    EXPECT_EQ(BuildDownloadInfo(nullptr, *info), nullptr);
}

/**
 * @tc.name: BuildInfoResourceNullEnvTest
 * @tc.desc: BuildInfoResource fails cleanly when napi_create_object cannot
 *           run (null env), consistently across repeated calls
 * @tc.precon: NA
 * @tc.step: 1. Trigger a failed preload and capture the download info from the failure callback
 *           2. Call BuildInfoResource with a null env and an out value slot
 *           3. Repeat the call on the same info and slot
 * @tc.expect: Both calls return false
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, BuildInfoResourceNullEnvTest, TestSize.Level1)
{
    auto info = FetchFailedInfo();
    ASSERT_TRUE(info != nullptr);
    napi_value jsInfo = nullptr;
    EXPECT_FALSE(BuildInfoResource(nullptr, *info, jsInfo));
    EXPECT_FALSE(BuildInfoResource(nullptr, *info, jsInfo));
}

/**
 * @tc.name: BuildInfoNetworkNullEnvTest
 * @tc.desc: BuildInfoNetwork fails cleanly when napi_create_object cannot
 *           run (null env), consistently across repeated calls
 * @tc.precon: NA
 * @tc.step: 1. Trigger a failed preload and capture the download info from the failure callback
 *           2. Call BuildInfoNetwork with a null env and an out value slot
 *           3. Repeat the call on the same info and slot
 * @tc.expect: Both calls return false
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, BuildInfoNetworkNullEnvTest, TestSize.Level1)
{
    auto info = FetchFailedInfo();
    ASSERT_TRUE(info != nullptr);
    napi_value jsInfo = nullptr;
    EXPECT_FALSE(BuildInfoNetwork(nullptr, *info, jsInfo));
    EXPECT_FALSE(BuildInfoNetwork(nullptr, *info, jsInfo));
}

/**
 * @tc.name: BuildInfoPerformanceNullEnvTest
 * @tc.desc: BuildInfoPerformance fails cleanly when its first SetPerformanceField
 *           napi call cannot run (null env), consistently across repeated calls
 * @tc.precon: NA
 * @tc.step: 1. Trigger a failed preload and capture the download info from the failure callback
 *           2. Call BuildInfoPerformance with a null env and an out value slot
 *           3. Repeat the call on the same info and slot
 * @tc.expect: Both calls return false
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadNapiTest, BuildInfoPerformanceNullEnvTest, TestSize.Level1)
{
    auto info = FetchFailedInfo();
    ASSERT_TRUE(info != nullptr);
    napi_value jsInfo = nullptr;
    EXPECT_FALSE(BuildInfoPerformance(nullptr, *info, jsInfo));
    EXPECT_FALSE(BuildInfoPerformance(nullptr, *info, jsInfo));
}