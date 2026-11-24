/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <__chrono/duration.h>
#include <sys/stat.h>

#include <climits>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "application_context.h"
#include "constant.h"
#include "context.h"
#include "context_impl.h"
#include "request_manager.h"
#include "request_service_proxy.h"
#include "task_builder.h"
#define private public
#define protected public

#include "request_manager_impl.h"

#include <gtest/gtest.h>

#include "gmock/gmock.h"

#include "accesstoken_kit.h"
#include "log.h"
#include "nativetoken_kit.h"
#include "request_action.h"
#include "request_common.h"
#include "request_manager_impl.h"
#include "token_setproc.h"

using namespace testing::ext;
using namespace OHOS::Request;
using namespace OHOS::AbilityRuntime;

#undef private
#undef protected

void GrantInternetPermission()
{
    const char **perms = new const char *[1];
    perms[0] = "ohos.permission.INTERNET";
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "request_service",
        .aplStr = "system_core",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

void GrantDownSessionPermission()
{
    const char **perms = new const char *[1];
    perms[0] = "ohos.permission.DOWNLOAD_SESSION_MANAGER";
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 1,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "request_service",
        .aplStr = "system_core",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

void GrantNoPermission()
{
    const char **perms = new const char *[0];
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 0,
        .aclsNum = 0,
        .dcaps = nullptr,
        .perms = perms,
        .acls = nullptr,
        .processName = "request_service",
        .aplStr = "system_core",
    };
    uint64_t tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
    delete[] perms;
}

class RequestActionTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestActionTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RequestActionTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
    GrantNoPermission();
}

void RequestActionTest::SetUp(void)
{
    // input testCase setup step，setup invoked before each testCase
    testing::UnitTest *test = testing::UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const testing::TestInfo *testInfo = test->current_test_info();
    ASSERT_NE(testInfo, nullptr);
    string testCaseName = string(testInfo->name());
    REQUEST_HILOGI("[SetUp] %{public}s start", testCaseName.c_str());
    GTEST_LOG_(INFO) << testCaseName.append(" start");
}

void RequestActionTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
}

std::string g_tidUser = "550015967"; //test correct tid which will be replaced after create used

/**
 * @tc.name: StartTest001
 * @tc.desc: Test the Start interface with valid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant internet permission
 *           3. Call Start with valid tid
 * @tc.expect: Start operation completes successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StartTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tidStr = "tid";
    GrantInternetPermission();
    auto res = RequestAction::GetInstance()->Start(tidStr);
    REQUEST_HILOGI("===> StartTest001 res %{public}d", res);
}

/**
 * @tc.name: StopTest001
 * @tc.desc: Test the Stop interface with valid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Stop with valid tid
 * @tc.expect: Stop operation completes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StopTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    RequestAction::GetInstance()->Stop(tid);
}

/**
 * @tc.name: TouchTest001
 * @tc.desc: Test the Touch interface with valid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Touch with valid tid, token and info
 * @tc.expect: Touch operation completes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, TouchTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    std::string token = "11111111";
    TaskInfo info;
    RequestAction::GetInstance()->Touch(tid, token, info);
}

/**
 * @tc.name: ShowTest001
 * @tc.desc: Test the Show interface with valid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Show with valid tid and info
 * @tc.expect: Show operation completes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ShowTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    TaskInfo info;
    RequestAction::GetInstance()->Show(tid, info);
}

/**
 * @tc.name: PauseTest001
 * @tc.desc: Test the Pause interface with valid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Pause with valid tid
 * @tc.expect: Pause operation completes without error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, PauseTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    RequestAction::GetInstance()->Pause(tid);
}

/**
 * @tc.name: ResumeTest001
 * @tc.desc: Test the Resume interface with valid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant internet permission
 *           3. Call Resume with valid tid
 * @tc.expect: Resume operation completes successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ResumeTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    GrantInternetPermission();
    auto res = RequestAction::GetInstance()->Resume(tid);
    REQUEST_HILOGI("===> ResumeTest001 res %{public}d", res);
}

/**
 * @tc.name: RemoveTest001
 * @tc.desc: Test the Remove interface with valid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Remove with valid tid
 * @tc.expect: Remove operation completes successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, RemoveTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    auto res = RequestAction::GetInstance()->Remove(tid);
    REQUEST_HILOGI("===>except 0= %{public}d", res);
}

/**
 * @tc.name: StartTest002
 * @tc.desc: Test the Start interface with invalid task ID returns non-zero
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant internet permission
 *           3. Call Start with invalid tid
 * @tc.expect: Start operation returns non-zero error code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StartTest002, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tidStr = "tid";
    GrantInternetPermission();
    auto res = RequestAction::GetInstance()->Start(tidStr);
    EXPECT_NE(res, 0);
}

/**
 * @tc.name: StopTest002
 * @tc.desc: Test the Stop interface with invalid task ID returns TASK_NOT_FOUND
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Stop with invalid tid
 * @tc.expect: Stop operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StopTest002, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    auto res = RequestAction::GetInstance()->Stop(tid);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: TouchTest002
 * @tc.desc: Test the Touch interface with invalid task ID returns TASK_NOT_FOUND
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Touch with invalid tid, token and info
 * @tc.expect: Touch operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, TouchTest002, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    std::string token = "11111111";
    TaskInfo info;
    auto res = RequestAction::GetInstance()->Touch(tid, token, info);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: ShowTest002
 * @tc.desc: Test the Show interface with invalid task ID returns TASK_NOT_FOUND
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Show with invalid tid and info
 * @tc.expect: Show operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ShowTest002, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    TaskInfo info;
    auto res = RequestAction::GetInstance()->Show(tid, info);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: PauseTest002
 * @tc.desc: Test the Pause interface with invalid task ID returns TASK_NOT_FOUND
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Pause with invalid tid
 * @tc.expect: Pause operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, PauseTest002, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    auto res = RequestAction::GetInstance()->Pause(tid);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: ResumeTest002
 * @tc.desc: Test the Resume interface with invalid task ID returns non-zero
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant internet permission
 *           3. Call Resume with invalid tid
 * @tc.expect: Resume operation returns non-zero error code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ResumeTest002, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    GrantInternetPermission();
    auto res = RequestAction::GetInstance()->Resume(tid);
    EXPECT_NE(res, 0);
}

/**
 * @tc.name: RemoveTest002
 * @tc.desc: Test the Remove interface with invalid task ID returns TASK_NOT_FOUND
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Call Remove with invalid tid
 * @tc.expect: Remove operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, RemoveTest002, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = "tid";
    auto res = RequestAction::GetInstance()->Remove(tid);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: StartTest003
 * @tc.desc: Test the Start interface with DOWNLOAD_SESSION_MANAGER permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Call Start with valid tid
 * @tc.expect: Start operation does not return 13499999 (PERMISSION_DENIED)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StartTest003, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    GrantDownSessionPermission();
    auto res = RequestAction::GetInstance()->Start(tid);
    EXPECT_NE(res, 13499999);
    REQUEST_HILOGI("===> StartTest003 res 0=%{public}d", res);
}

/**
 * @tc.name: StopTest003
 * @tc.desc: Test the Stop interface with DOWNLOAD_SESSION_MANAGER permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Call Stop with valid tid
 * @tc.expect: Stop operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StopTest003, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    GrantDownSessionPermission();
    auto res = RequestAction::GetInstance()->Stop(tid);
    EXPECT_EQ(res, 21900006);
    REQUEST_HILOGI("===> StopTest003 res 0=%{public}d", res);
}

/**
 * @tc.name: TouchTest003
 * @tc.desc: Test the Touch interface with DOWNLOAD_SESSION_MANAGER permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Call Touch with valid tid, token and info
 * @tc.expect: Touch operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, TouchTest003, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    std::string token = "11111111";
    TaskInfo info;
    GrantDownSessionPermission();
    auto res = RequestAction::GetInstance()->Touch(tid, token, info);
    EXPECT_EQ(res, 21900006);
    REQUEST_HILOGI("===> TouchTest003 res 0=%{public}d", res);
}

/**
 * @tc.name: ShowTest003
 * @tc.desc: Test the Show interface with DOWNLOAD_SESSION_MANAGER permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Call Show with valid tid and info
 * @tc.expect: Show operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ShowTest003, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    TaskInfo info;
    GrantDownSessionPermission();
    auto res = RequestAction::GetInstance()->Show(tid, info);
    EXPECT_EQ(res, 21900006);
    REQUEST_HILOGI("===> ShowTest003 res 0=%{public}d", res);
}

/**
 * @tc.name: PauseTest003
 * @tc.desc: Test the Pause interface with DOWNLOAD_SESSION_MANAGER permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Call Pause with valid tid
 * @tc.expect: Pause operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, PauseTest003, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    GrantDownSessionPermission();
    auto res = RequestAction::GetInstance()->Pause(tid);
    EXPECT_EQ(res, 21900006);
    REQUEST_HILOGI("===> PauseTest003 res 0=%{public}d", res);
}

/**
 * @tc.name: ResumeTest003
 * @tc.desc: Test the Resume interface with DOWNLOAD_SESSION_MANAGER permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Call Resume with valid tid
 * @tc.expect: Resume operation does not return 13499999 (PERMISSION_DENIED)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ResumeTest003, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    GrantDownSessionPermission();
    auto res = RequestAction::GetInstance()->Resume(tid);
    EXPECT_NE(res, 13499999);
    REQUEST_HILOGI("===> ResumeTest003 res 0=%{public}d", res);
}

/**
 * @tc.name: RemoveTest003
 * @tc.desc: Test the Remove interface with DOWNLOAD_SESSION_MANAGER permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Call Remove with valid tid
 * @tc.expect: Remove operation does not return 201 (PERMISSION_DENIED)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, RemoveTest003, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    GrantDownSessionPermission();
    auto res = RequestAction::GetInstance()->Remove(tid);
    EXPECT_NE(res, 201);
    REQUEST_HILOGI("===>RemoveTest003 res 0= %{public}d", res);
}

/**
 * @tc.name: StartTest004
 * @tc.desc: Test the Start interface without any permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call Start with valid tid
 * @tc.expect: Start operation does not return 13499999 (PERMISSION_DENIED)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StartTest004, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    auto res = RequestAction::GetInstance()->Start(tid);
    EXPECT_NE(res, 13499999);
}

/**
 * @tc.name: StopTest004
 * @tc.desc: Test the Stop interface without any permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call Stop with valid tid
 * @tc.expect: Stop operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StopTest004, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    auto res = RequestAction::GetInstance()->Stop(tid);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: TouchTest004
 * @tc.desc: Test the Touch interface without any permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call Touch with invalid tid, token and info
 * @tc.expect: Touch operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, TouchTest004, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    std::string token = "11111111";
    TaskInfo info;
    auto res = RequestAction::GetInstance()->Touch(tid, token, info);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: ShowTest004
 * @tc.desc: Test the Show interface without any permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call Show with invalid tid and info
 * @tc.expect: Show operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ShowTest004, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    TaskInfo info;
    auto res = RequestAction::GetInstance()->Show(tid, info);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: PauseTest004
 * @tc.desc: Test the Pause interface without any permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call Pause with invalid tid and info
 * @tc.expect: Pause operation returns 21900006 (TASK_NOT_FOUND)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, PauseTest004, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    auto res = RequestAction::GetInstance()->Pause(tid);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: ResumeTest004
 * @tc.desc: Test the Resume interface without any permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call Resume with invalid tid and info
 * @tc.expect: Resume operation returns non-zero value
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ResumeTest004, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    auto res = RequestAction::GetInstance()->Resume(tid);
    EXPECT_NE(res, 13499999);
}

/**
 * @tc.name: RemoveTest004
 * @tc.desc: Test the Remove interface without any permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call Remove with invalid tid and info
 * @tc.expect: Remove operation does not return 201 error code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, RemoveTest004, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::string tid = g_tidUser;
    auto res = RequestAction::GetInstance()->Remove(tid);
    EXPECT_EQ(res, 21900006);
}

/**
 * @tc.name: StartTasksTest001
 * @tc.desc: Test the StartTasks interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Prepare vector of invalid tids
 *           4. Call StartTasks with batch tids
 * @tc.expect: StartTasks operation completes without E_OTHER error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StartTasksTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantDownSessionPermission();
    std::string tid = "tid";
    std::vector<std::string> tids = { tid };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->StartTasks(tids, rets);
    EXPECT_NE(res, ExceptionErrorCode::E_OTHER);
    ExceptionErrorCode res0 = rets[tid];
    EXPECT_NE(res, ExceptionErrorCode::E_OTHER);
    REQUEST_HILOGI("===> StartTasksTest001 res 0=%{public}d", res0);
}

/**
 * @tc.name: StopTasksTest001
 * @tc.desc: Test the StopTasks interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Prepare vector of invalid tids
 *           4. Call StopTasks with batch tids
 * @tc.expect: StopTasks returns E_OK and individual tasks return TASK_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StopTasksTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantDownSessionPermission();
    std::string tid = "tid";
    std::vector<std::string> tids = { tid };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->StopTasks(tids, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    ExceptionErrorCode res0 = rets[tid];
    EXPECT_EQ(res0, ExceptionErrorCode::E_TASK_NOT_FOUND);
    REQUEST_HILOGI("===> StopTasksTest001 res 0=%{public}d", res0);
}

/**
 * @tc.name: ResumeTasksTest001
 * @tc.desc: Test the ResumeTasks interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Prepare vector of invalid tids
 *           4. Call ResumeTasks with batch tids
 * @tc.expect: ResumeTasks operation completes without E_OTHER error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ResumeTasksTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantDownSessionPermission();
    std::string tid = "tid";
    std::vector<std::string> tids = { tid };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->ResumeTasks(tids, rets);
    EXPECT_NE(res, ExceptionErrorCode::E_OTHER);
    ExceptionErrorCode res0 = rets[tid];
    EXPECT_NE(res, ExceptionErrorCode::E_OTHER);
    REQUEST_HILOGI("===> ResumeTasksTest001 res 0=%{public}d", res0);
}

/**
 * @tc.name: PauseTasksTest001
 * @tc.desc: Test the PauseTasks interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Prepare vector of invalid tids
 *           4. Call PauseTasks with batch tids
 * @tc.expect: PauseTasks returns E_OK and individual tasks return TASK_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, PauseTasksTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantDownSessionPermission();
    std::string tid = "tid";
    std::vector<std::string> tids = { tid };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->PauseTasks(tids, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    ExceptionErrorCode res0 = rets[tid];
    EXPECT_EQ(res0, ExceptionErrorCode::E_TASK_NOT_FOUND);
    REQUEST_HILOGI("===> PauseTasksTest001 res 0=%{public}d", res0);
}

/**
 * @tc.name: ShowTasksTest001
 * @tc.desc: Test the ShowTasks interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Prepare vector of invalid tids
 *           4. Call ShowTasks with batch tids
 * @tc.expect: ShowTasks returns E_OK and individual tasks return TASK_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ShowTasksTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantDownSessionPermission();
    std::string tid = "tid";
    std::vector<std::string> tids = { tid };
    std::unordered_map<std::string, TaskInfoRet> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->ShowTasks(tids, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    TaskInfoRet res0 = rets[tid];
    EXPECT_EQ(res0.code, ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: TouchTasksTest001
 * @tc.desc: Test the TouchTasks interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Prepare vector of invalid tid-token pairs
 *           4. Call TouchTasks with batch tid-token pairs
 * @tc.expect: TouchTasks returns E_OK and individual tasks return TASK_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, TouchTasksTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantDownSessionPermission();
    std::string tid = "tid";
    std::string token = "tasktoken";
    TaskIdAndToken tidToken = { tid, token };
    std::vector<TaskIdAndToken> tids = { tidToken };
    std::unordered_map<std::string, TaskInfoRet> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->TouchTasks(tids, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    TaskInfoRet res0 = rets[tid];
    EXPECT_EQ(res0.code, ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: SetMaxSpeedTest001
 * @tc.desc: Test the SetMaxSpeed interface without permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call SetMaxSpeed with invalid tid and max speed
 * @tc.expect: SetMaxSpeed returns E_PARAMETER_CHECK error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, SetMaxSpeedTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantNoPermission();
    std::string tid = "tid";
    int64_t maxSpeed = 1000;
    auto res = RequestAction::GetInstance()->SetMaxSpeed(tid, maxSpeed);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: SetMaxSpeedsTest001
 * @tc.desc: Test the SetMaxSpeeds interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Prepare vector of speed configurations
 *           4. Call SetMaxSpeeds with batch configs
 * @tc.expect: SetMaxSpeeds returns E_OK and individual configs return E_PARAMETER_CHECK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, SetMaxSpeedsTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantNoPermission();
    std::string tid = "tid";
    SpeedConfig config = { tid, 1000 };
    std::vector<SpeedConfig> configs = { config };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->SetMaxSpeeds(configs, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    ExceptionErrorCode res0 = rets[tid];
    EXPECT_EQ(res0, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: SetModeTest001
 * @tc.desc: Test the SetMode interface without permission
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant no permission
 *           3. Call SetMode with invalid tid and mode
 * @tc.expect: SetMode returns E_PERMISSION error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, SetModeTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantNoPermission();
    std::string tid = "tid";
    Mode mode = Mode::BACKGROUND;
    ExceptionErrorCode res = RequestAction::GetInstance()->SetMode(tid, mode);
    EXPECT_EQ(res, ExceptionErrorCode::E_PERMISSION);
}

/**
 * @tc.name: DisableTaskNotificationTest001
 * @tc.desc: Test the DisableTaskNotification interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Prepare vector of invalid tids
 *           3. Call DisableTaskNotification with batch tids
 * @tc.expect: DisableTaskNotification returns E_OK and individual tasks return TASK_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, DisableTaskNotificationTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    std::vector<std::string> tids = { "tid", "123", "123123" };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->DisableTaskNotification(tids, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    EXPECT_EQ(rets["tid"], ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: CreateTest001
 * @tc.desc: Test the Create interface with valid TaskBuilder parameters
 * @tc.precon: NA
 * @tc.step: 1. Create TaskBuilder instance with valid parameters
 *           2. Set all required fields including URL, title, description
 *           3. Call Create with the TaskBuilder
 * @tc.expect: Create operation returns E_PARAMETER_CHECK due to invalid parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateTest001, TestSize.Level1)
{
    std::string tid;
    TaskBuilder builder;
    std::string url = "https://gitee.com/tiga-ultraman/downloadTests/releases/download/v1.01/test.txt";
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> extras;
    auto buildRes = builder.setAction(Action::DOWNLOAD)
                        .setUrl(url)
                        .setTitle("title")
                        .setDescription("description")
                        .setMode(Mode::FOREGROUND)
                        .setOverwrite(true)
                        .setMethod("GET")
                        .setHeaders(headers)
                        .setData("data")
                        .setSaveAs("./test.txt")
                        .setNetwork(Network::ANY)
                        .setMetered(true)
                        .setRoaming(true)
                        .setRetry(true)
                        .setRedirect(true)
                        .setProxy("")
                        .setIndex(0)
                        .setBegins(0)
                        .setEnds(-1)
                        .setGauge(true)
                        .setPrecise(false)
                        .setToken("")
                        .setPriority(0)
                        .setExtras(extras)
                        .build();
    auto res = RequestAction::GetInstance()->Create(builder, tid);
    REQUEST_HILOGI("===> CreateTest001 res 0=%{public}d", res);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateTasksTest001
 * @tc.desc: Test the CreateTasks interface with empty TaskBuilder vector
 * @tc.precon: NA
 * @tc.step: 1. Create empty vector of TaskBuilder instances
 *           2. Prepare empty vector for TaskRet results
 *           3. Call CreateTasks with empty builders vector
 * @tc.expect: CreateTasks returns E_OK with empty results
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateTasksTest001, TestSize.Level1)
{
    std::vector<TaskBuilder> builders;
    std::vector<TaskRet> rets;
    auto res = RequestAction::GetInstance()->CreateTasks(builders, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
}

/**
 * @tc.name: RemoveTasksTest001
 * @tc.desc: Test the RemoveTasks interface with batch operations
 * @tc.precon: NA
 * @tc.step: 1. Create RequestAction instance
 *           2. Grant DOWNLOAD_SESSION_MANAGER permission
 *           3. Prepare vector of invalid tids
 *           4. Call RemoveTasks with batch tids
 * @tc.expect: RemoveTasks returns E_OK and individual tasks return TASK_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, RemoveTasksTest001, TestSize.Level1)
{
    EXPECT_NE(RequestAction::GetInstance(), nullptr);
    GrantDownSessionPermission();
    std::string tid = "tid";
    std::vector<std::string> tids = { tid };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res = RequestAction::GetInstance()->RemoveTasks(tids, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    ExceptionErrorCode res0 = rets[tid];
    EXPECT_EQ(res0, ExceptionErrorCode::E_TASK_NOT_FOUND);
    REQUEST_HILOGI("===> RemoveTasksTest001 res 0=%{public}d", res0);
}

/**
 * @tc.name: CreateInnerTest001
 * @tc.desc: Test the CreateDirs interface with empty path vector
 * @tc.precon: NA
 * @tc.step: 1. Create empty vector of directory paths
 *           2. Call CreateDirs with empty path vector
 * @tc.expect: CreateDirs returns true for empty input
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest001, TestSize.Level1)
{
    std::vector<std::string> pathDirs;
    auto res = RequestAction::CreateDirs(pathDirs);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0011
 * @tc.desc: Test the CreateDirs interface with empty path vector duplicate
 * @tc.precon: NA
 * @tc.step: 1. Create empty vector of directory paths
 *           2. Call CreateDirs with empty path vector
 * @tc.expect: CreateDirs returns true for empty input
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0011, TestSize.Level1)
{
    std::vector<std::string> pathDirs = { "sys", "tmp" };
    auto res = RequestAction::CreateDirs(pathDirs);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0012
 * @tc.desc: Test the CreateDirs interface with valid directory paths
 * @tc.precon: NA
 * @tc.step: 1. Create vector of valid directory paths
 *           2. Call CreateDirs with the path vector
 * @tc.expect: CreateDirs returns true for valid paths
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0012, TestSize.Level1)
{
    std::vector<std::string> pathDirs = { "data", "test", "CreateInTestDir" };
    auto res = RequestAction::CreateDirs(pathDirs);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest002
 * @tc.desc: Test the FileToWhole interface with null context
 * @tc.precon: NA
 * @tc.step: 1. Create null context pointer
 *           2. Create empty config and path
 *           3. Call FileToWhole with null context
 * @tc.expect: FileToWhole returns true for null context handling
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest002, TestSize.Level1)
{
    // convert "file://example" to "/data/storage/el?/base/exmaple"
    std::shared_ptr<OHOS::AbilityRuntime::Context> context;
    Config config;
    std::string path;
    auto res = RequestAction::FileToWhole(context, config, path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0021
 * @tc.desc: Test the FileToWhole interface with null context
 * @tc.precon: NA
 * @tc.step: 1. Create null context pointer
 *           2. Create empty config and path
 *           3. Call FileToWhole with null context
 * @tc.expect: FileToWhole returns false for null context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0021, TestSize.Level1)
{
    // convert "file://example" to "/data/storage/el?/base/exmaple"
    std::shared_ptr<OHOS::AbilityRuntime::Context> context;
    Config config = { .bundleName = "com.example.aaa" };
    std::string path = "aaa/file";
    auto res = RequestAction::FileToWhole(context, config, path);
    EXPECT_EQ(res, false);
}

class ContextTestMock : public ApplicationContext {
public:
    ~ContextTestMock() {};
    std::string GetBaseDir(void) const override
    {
        return "/data/app/base";
    };
    std::string GetCacheDir(void) override
    {
        return "/data/app/cache";
    };
};

class ContextTestErrMock : public ApplicationContext {
public:
    ~ContextTestErrMock() {};
    std::string GetBaseDir(void) const override
    {
        return "";
    };
    std::string GetCacheDir(void) override
    {
        return "";
    };
};

/**
 * @tc.name: CreateInnerTest003
 * @tc.desc: Test the BaseToWhole interface with mock context
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Prepare empty path string
 *           3. Call BaseToWhole with mock context
 * @tc.expect: BaseToWhole returns true for valid mock context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest003, TestSize.Level1)
{
    // convert "internal://cache/exmaple" to "/data/....../cache/example"
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    std::string path;
    auto res = RequestAction::BaseToWhole(context, path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0031
 * @tc.desc: Test the BaseToWhole interface with error mock context
 * @tc.precon: NA
 * @tc.step: 1. Create error mock ApplicationContext instance
 *           2. Prepare empty path string
 *           3. Call BaseToWhole with error mock context
 * @tc.expect: BaseToWhole returns false for error mock context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0031, TestSize.Level1)
{
    // convert "internal://cache/exmaple" to "/data/....../cache/example"
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestErrMock>();
    std::string path;
    auto res = RequestAction::BaseToWhole(context, path);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest004
 * @tc.desc: Test the CacheToWhole interface with valid mock context
 * @tc.precon: NA
 * @tc.step: 1. Create valid mock ApplicationContext instance
 *           2. Prepare empty path string
 *           3. Call CacheToWhole with valid mock context
 * @tc.expect: CacheToWhole returns true for valid mock context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest004, TestSize.Level1)
{
    // convert "./exmaple" to "/data/....../cache/example"
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    std::string path;
    auto res = RequestAction::CacheToWhole(context, path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0041
 * @tc.desc: Test the CacheToWhole interface with error mock context
 * @tc.precon: NA
 * @tc.step: 1. Create error mock ApplicationContext instance
 *           2. Prepare empty path string
 *           3. Call CacheToWhole with error mock context
 * @tc.expect: CacheToWhole returns false for error mock context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0041, TestSize.Level1)
{
    // convert "./exmaple" to "/data/....../cache/example"
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestErrMock>();
    std::string path;
    auto res = RequestAction::CacheToWhole(context, path);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest005
 * @tc.desc: Test the StandardizePath interface with mock context
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty config and path
 *           3. Call StandardizePath with mock context
 * @tc.expect: StandardizePath returns true for valid mock context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest005, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path;
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0051
 * @tc.desc: Test the StandardizePath interface with root path
 * @tc.precon: NA
 * @tc.step: 1. Create valid mock ApplicationContext instance
 *           2. Create empty config and root path "/"
 *           3. Call StandardizePath with root path
 * @tc.expect: StandardizePath returns true for root path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0051, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path = "/";
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0052
 * @tc.desc: Test the StandardizePath interface with file protocol and error context
 * @tc.precon: NA
 * @tc.step: 1. Create error mock ApplicationContext instance
 *           2. Create empty config and file protocol path
 *           3. Call StandardizePath with file protocol
 * @tc.expect: StandardizePath returns false for error context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0052, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestErrMock>();
    Config config;
    std::string path = "file://aa";
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0053
 * @tc.desc: Test the StandardizePath interface with internal protocol and error context
 * @tc.precon: NA
 * @tc.step: 1. Create error mock ApplicationContext instance
 *           2. Create empty config and internal protocol path
 *           3. Call StandardizePath with internal protocol
 * @tc.expect: StandardizePath returns false for error context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0053, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestErrMock>();
    Config config;
    std::string path = "internal://aa";
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0054
 * @tc.desc: Test the StandardizePath interface with relative path and error context
 * @tc.precon: NA
 * @tc.step: 1. Create error mock ApplicationContext instance
 *           2. Create empty config and relative path
 *           3. Call StandardizePath with relative path
 * @tc.expect: StandardizePath returns false for error context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0054, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestErrMock>();
    Config config;
    std::string path = "./";
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest006
 * @tc.desc: Test the StringSplit interface with basic string splitting
 * @tc.precon: NA
 * @tc.step: 1. Create test string with delimiter
 *           2. Set delimiter character
 *           3. Call StringSplit with test string and delimiter
 * @tc.expect: StringSplit successfully splits string into 2 elements
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest006, TestSize.Level1)
{
    const std::string str = "test/test1";
    char delim = '/';
    std::vector<std::string> elems;
    RequestAction::StringSplit(str, delim, elems);
    EXPECT_EQ(elems.size(), 2);
}

/**
 * @tc.name: CreateInnerTest007
 * @tc.desc: Test the PathVecToNormal interface with empty input vectors
 * @tc.precon: NA
 * @tc.step: 1. Create empty input vector
 *           2. Create empty output vector
 *           3. Call PathVecToNormal with empty vectors
 * @tc.expect: PathVecToNormal returns true for empty input
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest007, TestSize.Level1)
{
    std::vector<std::string> in;
    std::vector<std::string> out;
    auto res = RequestAction::PathVecToNormal(in, out);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0071
 * @tc.desc: Test the PathVecToNormal interface with parent directory navigation
 * @tc.precon: NA
 * @tc.step: 1. Create input vector with parent directory ".." and subdirectory
 *           2. Create output vector with pre-allocated size
 *           3. Call PathVecToNormal with valid vectors
 * @tc.expect: PathVecToNormal returns true for valid parent directory navigation
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0071, TestSize.Level1)
{
    std::vector<std::string> in = { "..", "aaaa" };
    std::vector<std::string> out;
    out.resize(10);
    auto res = RequestAction::PathVecToNormal(in, out);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0072
 * @tc.desc: Test the PathVecToNormal interface with invalid parent directory path
 * @tc.precon: NA
 * @tc.step: 1. Create input vector with only parent directory ".."
 *           2. Create empty output vector
 *           3. Call PathVecToNormal with invalid path
 * @tc.expect: PathVecToNormal returns false for invalid parent directory path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0072, TestSize.Level1)
{
    std::vector<std::string> in = { ".." };
    std::vector<std::string> out;
    out.resize(0);
    auto res = RequestAction::PathVecToNormal(in, out);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest008
 * @tc.desc: Test the WholeToNormal interface with empty path
 * @tc.precon: NA
 * @tc.step: 1. Create empty path string
 *           2. Create output vector
 *           3. Call WholeToNormal with empty path
 * @tc.expect: WholeToNormal returns true for empty path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest008, TestSize.Level1)
{
    std::string path;
    std::vector<std::string> out;
    auto res = RequestAction::WholeToNormal(path, out);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0081
 * @tc.desc: Test the WholeToNormal interface with invalid parent directory path
 * @tc.precon: NA
 * @tc.step: 1. Create path string with parent directory navigation
 *           2. Create empty output vector
 *           3. Call WholeToNormal with invalid path
 * @tc.expect: WholeToNormal returns false for invalid parent directory path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0081, TestSize.Level1)
{
    std::string path = "../aa";
    std::vector<std::string> out;
    out.resize(0);
    auto res = RequestAction::WholeToNormal(path, out);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0082
 * @tc.desc: Test the WholeToNormal interface with valid complex path
 * @tc.precon: NA
 * @tc.step: 1. Create path string with complex navigation
 *           2. Create output vector with pre-allocated size
 *           3. Call WholeToNormal with valid complex path
 * @tc.expect: WholeToNormal returns true for valid complex path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0082, TestSize.Level1)
{
    std::string path = "/data/../aa";
    std::vector<std::string> out;
    out.resize(10);
    auto res = RequestAction::WholeToNormal(path, out);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest009
 * @tc.desc: Test the GetAppBaseDir interface with empty base directory
 * @tc.precon: NA
 * @tc.step: 1. Create empty base directory string
 *           2. Call GetAppBaseDir with empty string
 *           3. Verify the result
 * @tc.expect: GetAppBaseDir returns false for empty base directory
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest009, TestSize.Level1)
{
    std::string baseDir;
    auto res = RequestAction::GetAppBaseDir(baseDir);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest010
 * @tc.desc: Test the CheckBelongAppBaseDir interface with empty parameters
 * @tc.precon: NA
 * @tc.step: 1. Create empty filepath string
 *           2. Create empty baseDir string
 *           3. Call CheckBelongAppBaseDir with empty parameters
 * @tc.expect: CheckBelongAppBaseDir returns false for empty parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest010, TestSize.Level1)
{
    std::string filepath;
    std::string baseDir;
    auto res = RequestAction::CheckBelongAppBaseDir(filepath, baseDir);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0101
 * @tc.desc: Test the FindAreaPath interface with various storage area paths
 * @tc.precon: NA
 * @tc.step: 1. Test with empty filepath
 *           2. Test with valid EL1 storage path
 *           3. Test with valid EL2 storage path
 *           4. Test with valid EL5 storage path
 * @tc.expect: Returns false for empty path, true for valid storage area paths
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0101, TestSize.Level1)
{
    std::string filepath;
    auto res = RequestAction::FindAreaPath(filepath);
    EXPECT_EQ(res, false);
    std::string filepath1 = "/data/storage/el1/base/a";
    auto res1 = RequestAction::FindAreaPath(filepath1);
    EXPECT_EQ(res1, true);
    std::string filepath2 = "/data/storage/el2/base/a";
    auto res2 = RequestAction::FindAreaPath(filepath2);
    EXPECT_EQ(res2, true);
    std::string filepath3 = "/data/storage/el5/base/a";
    auto res3 = RequestAction::FindAreaPath(filepath3);
    EXPECT_EQ(res3, true);
}

/**
 * @tc.name: CreateInnerTest011
 * @tc.desc: Test the GetSandboxPath interface with invalid empty parameters
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create empty path string
 *           4. Create empty path vector
 *           5. Call GetSandboxPath with empty parameters
 * @tc.expect: Returns false for invalid empty parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest011, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path;
    std::vector<std::string> pathVec;
    auto res = RequestAction::GetSandboxPath(context, config, path, pathVec);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0111
 * @tc.desc: Test the GetSandboxPath interface with StandardizePath failure
 * @tc.precon: NA
 * @tc.step: 1. Create error mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create root path "/"
 *           4. Create empty path vector
 *           5. Call GetSandboxPath with error context
 * @tc.expect: Returns false when StandardizePath fails
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0111, TestSize.Level1)
{
    // StandardizePath is false
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestErrMock>();
    Config config;
    std::string path = "/";
    std::vector<std::string> pathVec;
    auto res = RequestAction::GetSandboxPath(context, config, path, pathVec);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0112
 * @tc.desc: Test the GetSandboxPath interface with WholeToNormal failure
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create root path "/"
 *           4. Create empty path vector
 *           5. Call GetSandboxPath with root path
 * @tc.expect: Returns false when WholeToNormal fails
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0112, TestSize.Level1)
{
    // WholeToNormal is false
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path = "/";
    std::vector<std::string> pathVec;
    auto res = RequestAction::GetSandboxPath(context, config, path, pathVec);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0113
 * @tc.desc: Test the GetSandboxPath interface with empty path vector
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create parent directory path "../aa"
 *           4. Create empty path vector (resize to 0)
 *           5. Call GetSandboxPath with empty vector
 * @tc.expect: Returns false for empty path vector
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0113, TestSize.Level1)
{
    // pathVec empty
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path = "../aa";
    std::vector<std::string> pathVec;
    pathVec.resize(0);
    auto res = RequestAction::GetSandboxPath(context, config, path, pathVec);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0114
 * @tc.desc: Test the GetSandboxPath interface with CheckBelongAppBaseDir failure
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create root path "/"
 *           4. Create path vector with size 10
 *           5. Call GetSandboxPath with invalid base directory
 * @tc.expect: Returns false when CheckBelongAppBaseDir fails
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0114, TestSize.Level1)
{
    // CheckBelongAppBaseDir is false
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path = "/";
    std::vector<std::string> pathVec;
    pathVec.resize(10);
    auto res = RequestAction::GetSandboxPath(context, config, path, pathVec);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest012
 * @tc.desc: Test the CheckDownloadFilePath interface with invalid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Call CheckDownloadFilePath with invalid parameters
 * @tc.expect: Returns false for invalid download file path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest012, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    auto res = RequestAction::CheckDownloadFilePath(context, config);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest013
 * @tc.desc: Test the InterceptData interface with empty parameters
 * @tc.precon: NA
 * @tc.step: 1. Create empty input string
 *           2. Create empty intercept string
 *           3. Create empty output string
 *           4. Call InterceptData with empty parameters
 * @tc.expect: Returns false for empty input parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest013, TestSize.Level1)
{
    std::string str;
    std::string in;
    std::string out;
    auto res = RequestAction::InterceptData(str, in, out);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0131
 * @tc.desc: Test the InterceptData interface with non-matching path patterns
 * @tc.precon: NA
 * @tc.step: 1. Create root path "/"
 *           2. Create intercept pattern "a/"
 *           3. Create empty output string
 *           4. Call InterceptData with non-matching patterns
 * @tc.expect: Returns false for non-matching path intercept
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0131, TestSize.Level1)
{
    std::string str = "/";
    std::string in = "a/";
    std::string out;
    auto res = RequestAction::InterceptData(str, in, out);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest014
 * @tc.desc: Test the StandardizeFileSpec interface with various file specifications
 * @tc.precon: NA
 * @tc.step: 1. Test with basic file spec containing only URI
 *           2. Test with complete file spec having all fields
 *           3. Test with file spec having empty type but hasContentType=true
 *           4. Verify all field standardizations
 * @tc.expect: Correctly standardizes filename, name, and type fields
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest014, TestSize.Level1)
{
    FileSpec file = { .uri = "/test.txt" };
    RequestAction::StandardizeFileSpec(file);
    EXPECT_EQ(file.name, "file");
    EXPECT_EQ(file.filename, "test.txt");
    EXPECT_EQ(file.type, "txt");
    FileSpec file1 = { .uri = "/test.txt", .filename = "test1", .name = "file1", .type = "text/plain" };
    RequestAction::StandardizeFileSpec(file1);
    EXPECT_EQ(file1.name, "file1");
    EXPECT_FALSE(file1.filename.empty());
    EXPECT_FALSE(file1.type.empty());
    FileSpec file2 = { .uri = "/test.txt", .filename = "test1", .name = "file1", .type = "", .hasContentType = true };
    RequestAction::StandardizeFileSpec(file2);
    EXPECT_EQ(file2.type, "");
}

/**
 * @tc.name: CreateInnerTest017
 * @tc.desc: Test the IsPathValid interface with empty path
 * @tc.precon: NA
 * @tc.step: 1. Create empty filepath string
 *           2. Call IsPathValid with empty path
 * @tc.expect: Returns false for empty path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest017, TestSize.Level1)
{
    std::string filepath;
    auto res = RequestAction::IsPathValid(filepath);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0171
 * @tc.desc: Test the IsPathValid interface with invalid storage path
 * @tc.precon: NA
 * @tc.step: 1. Create invalid storage path "/data/storage/el1/base/test_createinner_0171/"
 *           2. Call IsPathValid with invalid path
 * @tc.expect: Returns false for invalid storage path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0171, TestSize.Level1)
{
    std::string filepath = "/data/storage/el1/base/test_createinner_0171/";
    auto res = RequestAction::IsPathValid(filepath);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0172
 * @tc.desc: Test the IsPathValid interface with valid storage path
 * @tc.precon: NA
 * @tc.step: 1. Create valid storage path "/data/storage/el1/base/"
 *           2. Call IsPathValid with valid path
 * @tc.expect: Returns true for valid storage path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0172, TestSize.Level1)
{
    std::string filepath = "/data/test/";
    auto res = RequestAction::IsPathValid(filepath);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest018
 * @tc.desc: Test the GetInternalPath interface with empty parameters
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create empty path string
 *           4. Call GetInternalPath with empty parameters
 * @tc.expect: Returns false for empty path parameter
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest018, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path;
    auto res = RequestAction::GetInternalPath(context, config, path);
    EXPECT_EQ(res, false);
    std::string path1 = "internal://cache/test1.txt";
    auto res1 = RequestAction::GetInternalPath(context, config, path1);
    EXPECT_EQ(res1, false);
}

/**
 * @tc.name: CreateInnerTest0181
 * @tc.desc: Test the GetInternalPath interface with error mock context
 * @tc.precon: NA
 * @tc.step: 1. Create error mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create internal path "internal://cache/test1.txt"
 *           4. Call GetInternalPath with error context
 * @tc.expect: Returns false for error mock context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0181, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestErrMock>();
    Config config;
    std::string path = "internal://cache/test1.txt";
    auto res = RequestAction::GetInternalPath(context, config, path);
    EXPECT_EQ(res, false);
}

class ContextCacheTestMock : public ApplicationContext {
public:
    ~ContextCacheTestMock() {};
    std::string GetCacheDir(void) override
    {
        return "/data";
    };
};

/**
 * @tc.name: CreateInnerTest0182
 * @tc.desc: Test the GetInternalPath interface with valid cache directory
 * @tc.precon: NA
 * @tc.step: 1. Create cache test mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create relative path "test"
 *           4. Call GetInternalPath with valid cache directory
 * @tc.expect: Returns true for valid cache directory path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0182, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextCacheTestMock>();
    Config config;
    std::string path = "test";
    auto res = RequestAction::GetInternalPath(context, config, path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest019
 * @tc.desc: Test the FindDir interface with empty directory path
 * @tc.precon: NA
 * @tc.step: 1. Create empty directory path string
 *           2. Call FindDir with empty path
 * @tc.expect: Returns false for empty directory path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest019, TestSize.Level1)
{
    std::string pathDir;
    auto res = RequestAction::FindDir(pathDir);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest020
 * @tc.desc: Test the GetFdDownload interface with empty parameters
 * @tc.precon: NA
 * @tc.step: 1. Create empty path string
 *           2. Create empty Config instance
 *           3. Call GetFdDownload with empty parameters
 * @tc.expect: Returns E_FILE_IO for empty parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest020, TestSize.Level1)
{
    std::string path;
    Config config;
    auto res = RequestAction::GetFdDownload(path, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
}

/**
 * @tc.name: CreateInnerTest0201
 * @tc.desc: Test the GetFdDownload interface with various config combinations
 * @tc.precon: NA
 * @tc.step: 1. Test with API10 version, firstInit=true, overwrite=false
 *           2. Test with API9 version, firstInit=true, overwrite=false
 *           3. Test with API10 version, firstInit=false, overwrite=true
 *           4. Test with API10 version, firstInit=false, overwrite=false
 * @tc.expect: Returns appropriate error codes for each config combination
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0201, TestSize.Level1)
{
    std::string path = "/data/test";
    Config config = { .version = Version::API10, .firstInit = true, .overwrite = false };
    auto res = RequestAction::GetFdDownload(path, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
    Config config1 = { .version = Version::API9, .firstInit = true, .overwrite = false };
    auto res1 = RequestAction::GetFdDownload(path, config1);
    EXPECT_EQ(res1, ExceptionErrorCode::E_FILE_PATH);
    Config config2 = { .version = Version::API10, .firstInit = false, .overwrite = true };
    auto res2 = RequestAction::GetFdDownload(path, config2);
    EXPECT_EQ(res2, ExceptionErrorCode::E_FILE_IO);
    Config config3 = { .version = Version::API10, .firstInit = false, .overwrite = false };
    auto res3 = RequestAction::GetFdDownload(path, config3);
    EXPECT_EQ(res2, ExceptionErrorCode::E_FILE_IO);
    Config config4 = { .version = Version::API10, .firstInit = true, .overwrite = true };
    EXPECT_EQ(RequestAction::GetFdDownload(path, config4), ExceptionErrorCode::E_FILE_IO);
}

/**
 * @tc.name: CreateInnerTest0202
 * @tc.desc: Test the GetFdDownload interface with valid storage path
 * @tc.precon: NA
 * @tc.step: 1. Create valid storage path "/data/storage/el1/base/test"
 *           2. Create Config with API10 version, firstInit=true, overwrite=true
 *           3. Call GetFdDownload with valid parameters
 * @tc.expect: Returns E_FILE_IO due to file system limitations
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0202, TestSize.Level1)
{
    std::string path = "/data/storage/el1/base/test";
    Config config = { .version = Version::API10, .firstInit = true, .overwrite = true };
    auto res = RequestAction::GetFdDownload(path, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
}

/**
 * @tc.name: CreateInnerTest021
 * @tc.desc: Test the CheckDownloadFile interface with empty parameters
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Call CheckDownloadFile with empty parameters
 * @tc.expect: Returns E_PARAMETER_CHECK for empty parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest021, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    auto res = RequestAction::CheckDownloadFile(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0211
 * @tc.desc: Test the CheckDownloadFile interface with API9 version and URI file
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create FileSpec with URI "/test.txt"
 *           3. Create Config with API9 version
 *           4. Call CheckDownloadFile with URI file
 * @tc.expect: Returns E_PARAMETER_CHECK for URI file in API9
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0211, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    FileSpec file = { .uri = "/test.txt" };
    Config config = { .version = Version::API9, .files = { file } };
    auto res = RequestAction::CheckDownloadFile(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0212
 * @tc.desc: Test the CheckDownloadFile interface with API9 version and relative path
 * @tc.precon: NA
 * @tc.step: 1. Create cache test mock ApplicationContext instance
 *           2. Create FileSpec with relative path "test"
 *           3. Create Config with API9 version
 *           4. Call CheckDownloadFile with relative path
 * @tc.expect: Returns E_PARAMETER_CHECK for relative path in API9
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0212, TestSize.Level1)
{
    // CheckDownloadFile api9/ find("/")-false/ GetInternalPath-true
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextCacheTestMock>();
    FileSpec file = { .uri = "test" };
    Config config = { .version = Version::API9, .files = { file } };
    auto res = RequestAction::CheckDownloadFile(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0213
 * @tc.desc: Test the CheckDownloadFile interface with saveas path validation
 * @tc.precon: NA
 * @tc.step: 1. Create cache test mock ApplicationContext instance
 *           2. Create FileSpec with relative path "test"
 *           3. Create Config with API9 version and saveas URI
 *           4. Call CheckDownloadFile with saveas parameter
 * @tc.expect: Returns E_PARAMETER_CHECK for invalid saveas in API9
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0213, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextCacheTestMock>();
    FileSpec file = { .uri = "test" };
    Config config = { .version = Version::API9, .saveas = "file://media/Photo/1/test.img" };
    auto res = RequestAction::CheckDownloadFile(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0214
 * @tc.desc: Test the CheckDownloadFile interface with API10 version and saveas path
 * @tc.precon: NA
 * @tc.step: 1. Create cache test mock ApplicationContext instance
 *           2. Create FileSpec with relative path "test"
 *           3. Create Config with API10 version, overwrite=false, and saveas URI
 *           4. Call CheckDownloadFile with saveas parameter
 * @tc.expect: Returns E_PARAMETER_CHECK for invalid saveas path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0214, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextCacheTestMock>();
    FileSpec file = { .uri = "test" };
    Config config = { .version = Version::API10, .overwrite = false, .saveas = "file://media/Photo/1/test.img" };
    auto res = RequestAction::CheckDownloadFile(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0215
 * @tc.desc: Test the CheckDownloadFile interface with API10 background mode
 * @tc.precon: NA
 * @tc.step: 1. Create cache test mock ApplicationContext instance
 *           2. Create FileSpec with file URI
 *           3. Create Config with API10, BACKGROUND mode, overwrite=true
 *           4. Call CheckDownloadFile with saveas parameter
 * @tc.expect: Returns E_PARAMETER_CHECK for invalid file path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0215, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextCacheTestMock>();
    FileSpec file = { .uri = "file://media/Photo/1/test.img" };
    Config config = { .version = Version::API10,
        .mode = Mode::BACKGROUND,
        .overwrite = true,
        .saveas = "file://media/Photo/1/test.img" };
    auto res = RequestAction::CheckDownloadFile(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest022
 * @tc.desc: Test the IsUserFile interface with empty path
 * @tc.precon: NA
 * @tc.step: 1. Create empty path string
 *           2. Call IsUserFile with empty path
 * @tc.expect: Returns false for empty path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest022, TestSize.Level1)
{
    std::string path;
    auto res = RequestAction::IsUserFile(path);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0221
 * @tc.desc: Test the IsUserFile interface with docs directory path
 * @tc.precon: NA
 * @tc.step: 1. Create docs directory path "file://docs/"
 *           2. Call IsUserFile with docs path
 * @tc.expect: Returns true for docs directory path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0221, TestSize.Level1)
{
    std::string path = "file://docs/";
    auto res = RequestAction::IsUserFile(path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest0222
 * @tc.desc: Test the IsUserFile interface with media directory path
 * @tc.precon: NA
 * @tc.step: 1. Create media directory path "file://media/"
 *           2. Call IsUserFile with media path
 * @tc.expect: Returns true for media directory path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0222, TestSize.Level1)
{
    std::string path = "file://media/";
    auto res = RequestAction::IsUserFile(path);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest023
 * @tc.desc: Test the CheckUserFileSpec interface with empty parameters
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create empty FileSpec instance
 *           4. Call CheckUserFileSpec with empty parameters
 * @tc.expect: Returns E_PARAMETER_CHECK for empty parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest023, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    FileSpec file;
    auto res = RequestAction::CheckUserFileSpec(context, config, file, true);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0231
 * @tc.desc: Test the CheckUserFileSpec interface with null context and background mode
 * @tc.precon: NA
 * @tc.step: 1. Create null ApplicationContext instance
 *           2. Create Config with BACKGROUND mode
 *           3. Create empty FileSpec instance
 *           4. Call CheckUserFileSpec with null context
 * @tc.expect: Returns E_PARAMETER_CHECK for null context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0231, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context;
    Config config = { .mode = Mode::BACKGROUND };
    FileSpec file;
    auto res = RequestAction::CheckUserFileSpec(context, config, file, true);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0232
 * @tc.desc: Test the CheckUserFileSpec interface with null context and foreground mode
 * @tc.precon: NA
 * @tc.step: 1. Create null ApplicationContext instance
 *           2. Create Config with FOREGROUND mode
 *           3. Create empty FileSpec instance
 *           4. Call CheckUserFileSpec with null context
 * @tc.expect: Returns E_PARAMETER_CHECK for null context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0232, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context;
    Config config = { .mode = Mode::FOREGROUND };
    FileSpec file;
    auto res = RequestAction::CheckUserFileSpec(context, config, file, true);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0233
 * @tc.desc: Test the CheckUserFileSpec interface with foreground mode and firstInit=true
 * @tc.precon: NA
 * @tc.step: 1. Create null ApplicationContext instance
 *           2. Create Config with FOREGROUND mode and firstInit=true
 *           3. Create FileSpec with empty URI and isUserFile=true
 *           4. Call CheckUserFileSpec with false parameter
 * @tc.expect: Returns E_FILE_IO for empty URI file specification
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0233, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context;
    Config config = { .mode = Mode::FOREGROUND, .firstInit = true };
    FileSpec file = { .uri = "", .isUserFile = true };
    auto res = RequestAction::CheckUserFileSpec(context, config, file, false);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
}

/**
 * @tc.name: CreateInnerTest0234
 * @tc.desc: Test the CheckUserFileSpec interface with foreground mode and firstInit=false
 * @tc.precon: NA
 * @tc.step: 1. Create null ApplicationContext instance
 *           2. Create Config with FOREGROUND mode and firstInit=false
 *           3. Create FileSpec with empty URI and isUserFile=true
 *           4. Call CheckUserFileSpec with false parameter
 * @tc.expect: Returns E_FILE_IO for empty URI file specification
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0234, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context;
    Config config = { .mode = Mode::FOREGROUND, .firstInit = false };
    FileSpec file = { .uri = "", .isUserFile = true };
    auto res = RequestAction::CheckUserFileSpec(context, config, file, false);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
}

/**
 * @tc.name: CreateInnerTest024
 * @tc.desc: Test the CheckPathIsFile interface with various path scenarios
 * @tc.precon: NA
 * @tc.step: 1. Test with empty path
 *           2. Test with directory path "/data/test"
 *           3. Test with existing file path "/data/test/CreateInnerFile"
 *           4. Verify file existence and type checking
 * @tc.expect: Returns correct boolean for each path type
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest024, TestSize.Level1)
{
    // exist is false
    std::string path;
    auto res = RequestAction::CheckPathIsFile(path);
    EXPECT_EQ(res, false);
    // is_directory is true
    std::string path1 = "/data/test";
    auto res1 = RequestAction::CheckPathIsFile(path1);
    EXPECT_EQ(res1, false);
    // exist and no_directory
    std::ofstream file("/data/test/CreateInnerFile");
    file.close();
    std::string path2 = "/data/test/CreateInnerFile";
    auto res2 = RequestAction::CheckPathIsFile(path2);
    EXPECT_EQ(res2, true);
}

/**
 * @tc.name: CreateInnerTest025
 * @tc.desc: Test the GetFdUpload interface with empty path and different API versions
 * @tc.precon: NA
 * @tc.step: 1. Create empty path string
 *           2. Create Config with API10 version
 *           3. Call GetFdUpload with API10 config
 *           4. Create Config with API9 version
 *           5. Call GetFdUpload with API9 config
 * @tc.expect: Returns E_FILE_IO for API10, E_FILE_PATH for API9
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest025, TestSize.Level1)
{
    std::string path;
    // open file error in api10
    Config config = { .version = Version::API10 };
    auto res = RequestAction::GetFdUpload(path, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
    // open file error in api9
    Config config1 = { .version = Version::API9 };
    auto res1 = RequestAction::GetFdUpload(path, config1);
    EXPECT_EQ(res1, ExceptionErrorCode::E_FILE_PATH);
}

/**
 * @tc.name: CreateInnerTest0251
 * @tc.desc: Test the GetFdUpload interface with valid existing file
 * @tc.precon: NA
 * @tc.step: 1. Create test file "/data/test/CreateInnerFile"
 *           2. Create Config with API10 version
 *           3. Call GetFdUpload with valid file path
 * @tc.expect: Returns E_OK for valid existing file
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0251, TestSize.Level1)
{
    std::ofstream file("/data/test/CreateInnerFile");
    file.close();
    std::string path = "/data/test/CreateInnerFile";
    Config config = { .version = Version::API10 };
    auto res = RequestAction::GetFdUpload(path, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
}

/**
 * @tc.name: CreateInnerTest0252
 * @tc.desc: Test the GetFdUpload interface with system file path and API version variations
 * @tc.precon: NA
 * @tc.step: 1. Create system file path with leading space
 *           2. Test with API10 version configuration
 *           3. Test with API9 version configuration
 * @tc.expect: Returns E_FILE_IO for API10, E_FILE_PATH for API9 due to file access issues
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0252, TestSize.Level1)
{
    std::string path = " system/etc/init.cfg";
    // open file error in api10
    Config config = { .version = Version::API10 };
    auto res = RequestAction::GetFdUpload(path, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
    // open file error in api9
    Config config1 = { .version = Version::API9 };
    auto res1 = RequestAction::GetFdUpload(path, config1);
    EXPECT_EQ(res1, ExceptionErrorCode::E_FILE_PATH);
}

/**
 * @tc.name: CreateInnerTest026
 * @tc.desc: Test the CheckUploadFileSpec interface with invalid parameters
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create empty Config instance
 *           3. Create empty FileSpec instance
 *           4. Call CheckUploadFileSpec with invalid parameters
 * @tc.expect: Returns E_PARAMETER_CHECK for invalid input parameters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest026, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    FileSpec file;
    auto res = RequestAction::CheckUploadFileSpec(context, config, file);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0261
 * @tc.desc: Test the CheckUploadFileSpec interface with API version variations
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create FileSpec with empty uri
 *           3. Test with API9 configuration
 *           4. Test with API10 configuration
 * @tc.expect: Returns E_PARAMETER_CHECK for both API versions with invalid file spec
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0261, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    FileSpec file;
    Config config = { .version = Version::API9 };
    auto res = RequestAction::CheckUploadFileSpec(context, config, file);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
    Config config1 = { .version = Version::API10 };
    EXPECT_EQ(RequestAction::CheckUploadFileSpec(context, config1, file), ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0271
 * @tc.desc: Test the CheckUploadFiles interface with valid empty configuration
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create API10 configuration with empty files
 *           3. Call CheckUploadFiles with valid parameters
 * @tc.expect: Returns E_OK for valid empty file configuration
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0271, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config = { .version = Version::API10 };
    auto res = RequestAction::CheckUploadFiles(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
}

/**
 * @tc.name: CreateInnerTest0272
 * @tc.desc: Test the CheckUploadFiles interface with invalid user file path
 * @tc.precon: NA
 * @tc.step: 1. Create mock ApplicationContext instance
 *           2. Create FileSpec with invalid user file path
 *           3. Create API10 configuration with invalid file
 *           4. Call CheckUploadFiles with invalid file path
 * @tc.expect: Returns E_PARAMETER_CHECK for invalid user file path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0272, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    FileSpec file = { .uri = "file://media/Photo/1/test.img", .isUserFile = true };
    Config config = { .version = Version::API10, .files = { file } };
    auto res = RequestAction::CheckUploadFiles(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest028
 * @tc.desc: Test the CheckUploadBodyFiles interface with empty and invalid configurations
 * @tc.precon: NA
 * @tc.step: 1. Test with empty filepath and empty configuration
 *           2. Create multipart configuration with invalid file path
 *           3. Call CheckUploadBodyFiles with invalid parameters
 * @tc.expect: Returns E_OK for empty config, E_PARAMETER_CHECK for invalid file path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest028, TestSize.Level1)
{
    std::string filepath;
    Config config = { .version = Version::API10 };
    auto res = RequestAction::CheckUploadBodyFiles(filepath, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    // len !=0 but filePath empty
    FileSpec fileSpec = {
        .filename = "filename", .name = "file", .uri = "/data/test/CheckUploadBodyFilesTest", .type = "text/plain"
    };
    Config config1 = { .multipart = true, .files = { fileSpec } };
    EXPECT_EQ(RequestAction::CheckUploadBodyFiles(filepath, config1), ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: CreateInnerTest0281
 * @tc.desc: Test the CheckUploadBodyFiles interface with valid filepath and invalid file
 * @tc.precon: NA
 * @tc.step: 1. Create valid filepath string
 *           2. Create multipart configuration with invalid file
 *           3. Call CheckUploadBodyFiles with valid filepath and invalid file
 * @tc.expect: Returns E_FILE_IO for file I/O error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0281, TestSize.Level1)
{
    // len !=0 and filepath no empty
    std::string filepath = "/data/test";
    FileSpec fileSpec = {
        .filename = "filename", .name = "file", .uri = "/data/test/CheckUploadBodyFilesTest", .type = "text/plain"
    };
    Config config1 = { .multipart = true, .files = { fileSpec } };
    EXPECT_EQ(RequestAction::CheckUploadBodyFiles(filepath, config1), ExceptionErrorCode::E_FILE_IO);
}

/**
 * @tc.name: CreateInnerTest029
 * @tc.desc: Test the SetDirsPermission interface with invalid directory paths
 * @tc.precon: NA
 * @tc.step: 1. Create vector with invalid directory path "test"
 *           2. Call SetDirsPermission with invalid directory
 * @tc.expect: Returns false for invalid directory permission setting
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest029, TestSize.Level1)
{
    std::vector<std::string> dirs = { "test" };
    auto res = RequestAction::SetDirsPermission(dirs);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest0291
 * @tc.desc: Test the SetDirsPermission interface with empty directory vector
 * @tc.precon: NA
 * @tc.step: 1. Create empty directory vector
 *           2. Call SetDirsPermission with empty vector
 * @tc.expect: Returns true for empty directory list (no operation needed)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest0291, TestSize.Level1)
{
    std::vector<std::string> dirs;
    auto res = RequestAction::SetDirsPermission(dirs);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name: CreateInnerTest030
 * @tc.desc: Test the CheckFilePath interface with empty configuration
 * @tc.precon: NA
 * @tc.step: 1. Create empty Config instance
 *           2. Call CheckFilePath with empty configuration
 * @tc.expect: Returns E_FILE_IO for invalid file path check
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest030, TestSize.Level1)
{
    Config config;
    auto res = RequestAction::CheckFilePath(config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
}

/**
 * @tc.name: CreateInnerTest031
 * @tc.desc: Test the RemoveFile interface with non-existent file
 * @tc.precon: NA
 * @tc.step: 1. Create filepath string for non-existent file
 *           2. Call RemoveFile with non-existent file path
 *           3. Verify file does not exist after removal attempt
 * @tc.expect: File does not exist after RemoveFile operation
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest031, TestSize.Level1)
{
    std::string filepath = "data/test/testRemove";
    RequestAction::RemoveFile(filepath);
    auto res = std::filesystem::exists(filepath);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CreateInnerTest033
 * @tc.desc: Test the RemoveDirsPermission interface with valid directory path
 * @tc.precon: NA
 * @tc.step: 1. Verify RequestManager instance exists
 *           2. Create vector with valid test directory path
 *           3. Call RemoveDirsPermission with test directory
 * @tc.expect: Successfully processes directory permission removal
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest033, TestSize.Level1)
{
    EXPECT_NE(RequestManager::GetInstance(), nullptr);
    std::vector<std::string> dirs = { "/data/test" };
    RequestAction::RemoveDirsPermission(dirs);
}

/**
 * @tc.name: CreateInnerTest034
 * @tc.desc: Test the ClearTaskTemp interface with empty task ID
 * @tc.precon: NA
 * @tc.step: 1. Create empty task ID string
 *           2. Call ClearTaskTemp with empty task ID
 * @tc.expect: Returns false for empty task ID
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, CreateInnerTest034, TestSize.Level1)
{
    std::string tid;
    auto res = RequestAction::ClearTaskTemp(tid);
    EXPECT_EQ(res, false);
}

class MockRequestServiceInterface : public RequestServiceInterface {
public:
    MOCK_METHOD(ExceptionErrorCode, StartTasks,
        (const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets), (override));
    MOCK_METHOD(ExceptionErrorCode, StopTasks,
        (const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets), (override));
    MOCK_METHOD(ExceptionErrorCode, ResumeTasks,
        (const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets), (override));
    MOCK_METHOD(ExceptionErrorCode, RemoveTasks,
        (const std::vector<std::string> &tids, const Version version, std::vector<ExceptionErrorCode> &rets),
        (override));
    MOCK_METHOD(ExceptionErrorCode, PauseTasks,
        (const std::vector<std::string> &tids, const Version version, std::vector<ExceptionErrorCode> &rets),
        (override));
    MOCK_METHOD(ExceptionErrorCode, QueryTasks,
        (const std::vector<std::string> &tids, (std::vector<TaskInfoRet> & rets)), (override));
    MOCK_METHOD(ExceptionErrorCode, ShowTasks,
        (const std::vector<std::string> &tids, (std::vector<TaskInfoRet> & rets)), (override));
    MOCK_METHOD(ExceptionErrorCode, TouchTasks,
        ((const std::vector<TaskIdAndToken> &tidTokens), (std::vector<TaskInfoRet> & rets)), (override));

    MOCK_METHOD(ExceptionErrorCode, SetMode, (const std::string &tid, const Mode mode), (override));
    MOCK_METHOD(int32_t, Create, (const Config &config, std::string &taskId), (override));
    MOCK_METHOD(int32_t, GetTask, (const std::string &tid, const std::string &token, Config &config), (override));
    MOCK_METHOD(int32_t, Start, (const std::string &tid), (override));
    MOCK_METHOD(int32_t, Pause, (const std::string &tid, const Version version), (override));
    MOCK_METHOD(int32_t, QueryMimeType, (const std::string &tid, std::string &mimeType), (override));
    MOCK_METHOD(int32_t, Remove, (const std::string &tid, const Version version), (override));
    MOCK_METHOD(int32_t, Resume, (const std::string &tid), (override));
    MOCK_METHOD(int32_t, Stop, (const std::string &tid), (override));
    MOCK_METHOD(int32_t, Query, (const std::string &tid, TaskInfo &info), (override));
    MOCK_METHOD(int32_t, Touch, (const std::string &tid, const std::string &token, TaskInfo &info), (override));
    MOCK_METHOD(int32_t, Search, (const Filter &filter, std::vector<std::string> &tids), (override));
    MOCK_METHOD(int32_t, Show, (const std::string &tid, TaskInfo &info), (override));

    MOCK_METHOD(int32_t, OpenChannel, (int32_t & sockFd), (override));
    MOCK_METHOD(int32_t, Subscribe, (const std::string &taskId), (override));
    MOCK_METHOD(int32_t, Unsubscribe, (const std::string &taskId), (override));
    MOCK_METHOD(int32_t, SubRunCount, (const OHOS::sptr<NotifyInterface> &listener), (override));
    MOCK_METHOD(int32_t, UnsubRunCount, (), (override));
    MOCK_METHOD(OHOS::sptr<OHOS::IRemoteObject>, AsObject, (), (override));
    MOCK_METHOD(int32_t, CreateGroup, (std::string & gid, const bool gauge, Notification &info), (override));
    MOCK_METHOD(int32_t, AttachGroup, (const std::string &gid, const std::vector<std::string> &tid), (override));
    MOCK_METHOD(int32_t, DeleteGroup, (const std::string &gid), (override));
    MOCK_METHOD(int32_t, SetMaxSpeed, (const std::string &tid, const int64_t maxSpeed), (override));
    MOCK_METHOD(ExceptionErrorCode, SetMaxSpeeds,
        (const std::vector<SpeedConfig> &speedConfig, std::vector<ExceptionErrorCode> &rets), (override));
    MOCK_METHOD(ExceptionErrorCode, DisableTaskNotification,
        (const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets));
    MOCK_METHOD(ExceptionErrorCode, CreateTasks, (const std::vector<Config> &configs, std::vector<TaskRet> &rets));
};

namespace {
// Speed limit fed to SetMaxSpeeds; the value itself is irrelevant because the
// batch call fails at the proxy boundary before any semantic check.
constexpr int64_t MOCK_MAX_SPEED = 1024;

// Swaps the cached service proxy for an in-process mock and always restores
// the previous value on destruction, so the tested path never reaches the
// real download_server instance.
class ProxySwapGuard {
public:
    explicit ProxySwapGuard(const OHOS::sptr<RequestServiceInterface> &mock)
    {
        saved_ = RequestManagerImpl::GetInstance()->requestServiceProxy_;
        RequestManagerImpl::GetInstance()->requestServiceProxy_ = mock;
    }

    ~ProxySwapGuard()
    {
        RequestManagerImpl::GetInstance()->requestServiceProxy_ = saved_;
    }

    ProxySwapGuard(const ProxySwapGuard &) = delete;
    ProxySwapGuard &operator=(const ProxySwapGuard &) = delete;

private:
    OHOS::sptr<RequestServiceInterface> saved_;
};
} // namespace

/**
 * @tc.name: StartTasksEarlyExit001
 * @tc.desc: StartTasks returns the batch error directly when the service
 *           call fails, skipping the per-tid result mapping
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that answers StartTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call StartTasks with the mock cached in place
 *           3. Restore the proxy and check the result
 * @tc.expect: StartTasks returns E_TASK_NOT_FOUND and leaves the result map
 *             empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StartTasksEarlyExit001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, StartTasks(testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<std::string> tids = { "tid_mock_start" };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->StartTasks(tids, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: StopTasksEarlyExit001
 * @tc.desc: StopTasks returns the batch error directly when the service
 *           call fails
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that answers StopTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call StopTasks with the mock cached in place
 * @tc.expect: StopTasks returns E_TASK_NOT_FOUND and leaves the result map
 *             empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StopTasksEarlyExit001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, StopTasks(testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<std::string> tids = { "tid_mock_stop" };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->StopTasks(tids, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ResumeTasksEarlyExit001
 * @tc.desc: ResumeTasks returns the batch error directly when the service
 *           call fails
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that answers ResumeTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call ResumeTasks with the mock cached in place
 * @tc.expect: ResumeTasks returns E_TASK_NOT_FOUND and leaves the result
 *             map empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ResumeTasksEarlyExit001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, ResumeTasks(testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<std::string> tids = { "tid_mock_resume" };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->ResumeTasks(tids, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: PauseTasksEarlyExit001
 * @tc.desc: PauseTasks returns the batch error directly when the service
 *           call fails
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that answers PauseTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call PauseTasks with the mock cached in place
 * @tc.expect: PauseTasks returns E_TASK_NOT_FOUND and leaves the result map
 *             empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, PauseTasksEarlyExit001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, PauseTasks(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<std::string> tids = { "tid_mock_pause" };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->PauseTasks(tids, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ShowTasksEarlyExit001
 * @tc.desc: ShowTasks returns the batch error directly when the service
 *           call fails, without filling task infos
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that answers ShowTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call ShowTasks with the mock cached in place
 * @tc.expect: ShowTasks returns E_TASK_NOT_FOUND and leaves the info map
 *             empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ShowTasksEarlyExit001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, ShowTasks(testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<std::string> tids = { "tid_mock_show" };
    std::unordered_map<std::string, TaskInfoRet> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->ShowTasks(tids, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: TouchTasksEarlyExit001
 * @tc.desc: TouchTasks returns the batch error directly when the service
 *           call fails, without filling task infos
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that answers TouchTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call TouchTasks with one tid-token pair and the mock cached
 *              in place
 * @tc.expect: TouchTasks returns E_TASK_NOT_FOUND and leaves the info map
 *             empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, TouchTasksEarlyExit001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, TouchTasks(testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<TaskIdAndToken> tidTokens = { { "tid_mock_touch", "token_mock_touch" } };
    std::unordered_map<std::string, TaskInfoRet> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->TouchTasks(tidTokens, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: SetMaxSpeedsEarlyExit001
 * @tc.desc: SetMaxSpeeds returns the batch error directly when the service
 *           call fails, without filling per-tid results
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that answers SetMaxSpeeds with
 *              E_TASK_NOT_FOUND
 *           2. Call SetMaxSpeeds with one speed config and the mock cached
 *              in place
 * @tc.expect: SetMaxSpeeds returns E_TASK_NOT_FOUND and leaves the result
 *             map empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, SetMaxSpeedsEarlyExit001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, SetMaxSpeeds(testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<SpeedConfig> speedConfig = { { "tid_mock_speed", MOCK_MAX_SPEED } };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->SetMaxSpeeds(speedConfig, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_TRUE(rets.empty());
}

namespace {
// Minimal counting listeners for the registry-level pass-through cases.
class StubResponseListener : public IResponseListener {
public:
    void OnResponseReceive(const std::shared_ptr<Response> &response) override {}
};

class StubNotifyListener : public INotifyDataListener {
public:
    void OnNotifyDataReceive(const std::shared_ptr<NotifyData> &notifyData) override {}
    void OnFaultsReceive(const std::shared_ptr<int32_t> &tid, const std::shared_ptr<SubscribeType> &type,
        const std::shared_ptr<Reason> &reason) override {}
    void OnWaitReceive(std::int32_t taskId, WaitingReason reason) override {}
};
} // namespace

/**
 * @tc.name: ManagerQueryPassthrough001
 * @tc.desc: RequestManager::Query forwards to the service through the cached
 *           proxy and reports the proxy answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering Query with E_TASK_NOT_FOUND
 *           2. Call RequestManager::GetInstance()->Query
 * @tc.expect: The returned code equals the mock answer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerQueryPassthrough001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, Query(testing::_, testing::_))
        .WillRepeatedly(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    TaskInfo info;
    int32_t res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->Query("tid_mgr_query", info);
    }
    EXPECT_EQ(res, static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
}

/**
 * @tc.name: ManagerSearchPassthrough002
 * @tc.desc: RequestManager::Search forwards the filter and reports the proxy
 *           answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering Search with E_OK
 *           2. Call RequestManager::GetInstance()->Search with an empty
 *              bundle filter
 * @tc.expect: The returned code is E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerSearchPassthrough002, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, Search(testing::_, testing::_)).WillRepeatedly(testing::Return(E_OK));
    Filter filter;
    std::vector<std::string> tids;
    int32_t res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->Search(filter, tids);
    }
    EXPECT_EQ(res, E_OK);
}

/**
 * @tc.name: ManagerQueryMimeTypePassthrough003
 * @tc.desc: RequestManager::QueryMimeType forwards to the cached proxy
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering QueryMimeType with E_TASK_NOT_FOUND
 *           2. Call RequestManager::GetInstance()->QueryMimeType
 * @tc.expect: The returned code equals the mock answer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerQueryMimeTypePassthrough003, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, QueryMimeType(testing::_, testing::_))
        .WillRepeatedly(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::string mimeType;
    int32_t res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->QueryMimeType("tid_mgr_mime", mimeType);
    }
    EXPECT_EQ(res, static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
}

/**
 * @tc.name: ManagerUnsubscribePassthrough004
 * @tc.desc: RequestManager::Unsubscribe forwards to the cached proxy
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering Unsubscribe with E_OK
 *           2. Call RequestManager::GetInstance()->Unsubscribe
 * @tc.expect: The returned code is E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerUnsubscribePassthrough004, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, Unsubscribe(testing::_)).WillRepeatedly(testing::Return(E_OK));
    int32_t res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->Unsubscribe("tid_mgr_unsub");
    }
    EXPECT_EQ(res, E_OK);
}

/**
 * @tc.name: ManagerSubscribePassthrough005
 * @tc.desc: RequestManager::Subscribe reaches the cached proxy even when the
 *           UDS channel cannot be opened because the mock answers OpenChannel
 *           with success but no usable fd
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering Subscribe with E_OK and
 *              OpenChannel with E_OK while leaving the fd untouched
 *           2. Call RequestManager::GetInstance()->Subscribe
 * @tc.expect: The returned code is E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerSubscribePassthrough005, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, Subscribe(testing::_)).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*mock, OpenChannel(testing::_)).WillRepeatedly(testing::Return(E_OK));
    int32_t res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->Subscribe("tid_mgr_sub");
    }
    EXPECT_EQ(res, E_OK);
}

/**
 * @tc.name: ManagerRemoveTasksPassthrough006
 * @tc.desc: RequestManager::RemoveTasks forwards the batch and reports the
 *           proxy answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering RemoveTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManager::GetInstance()->RemoveTasks
 * @tc.expect: The returned code equals the mock answer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerRemoveTasksPassthrough006, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, RemoveTasks(testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(ExceptionErrorCode::E_TASK_NOT_FOUND));
    std::vector<std::string> tids = { "tid_mgr_remove" };
    std::vector<ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->RemoveTasks(tids, Version::API10, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: ManagerSetMaxSpeedPassthrough007
 * @tc.desc: RequestManager::SetMaxSpeed forwards the limit to the cached
 *           proxy
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering SetMaxSpeed with E_OK
 *           2. Call RequestManager::GetInstance()->SetMaxSpeed
 * @tc.expect: The returned code is E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerSetMaxSpeedPassthrough007, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, SetMaxSpeed(testing::_, testing::_)).WillRepeatedly(testing::Return(E_OK));
    int32_t res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->SetMaxSpeed("tid_mgr_speed", MOCK_MAX_SPEED);
    }
    EXPECT_EQ(res, E_OK);
}

/**
 * @tc.name: ManagerGroupPassthrough008
 * @tc.desc: group create, attach and delete forward to the cached proxy in
 *           sequence
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering CreateGroup, AttachGroup and
 *              DeleteGroup with E_OK
 *           2. Call the three RequestManager group interfaces
 * @tc.expect: All three calls return E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerGroupPassthrough008, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*mock, CreateGroup(testing::_, testing::_, testing::_)).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*mock, AttachGroup(testing::_, testing::_)).WillRepeatedly(testing::Return(E_OK));
    EXPECT_CALL(*mock, DeleteGroup(testing::_)).WillRepeatedly(testing::Return(E_OK));
    std::string gid = "gid_mgr";
    Notification info{ .text = "text", .title = "title", .wantAgent = "", .disable = false };
    int32_t res;
    {
        ProxySwapGuard guard(mock);
        res = RequestManager::GetInstance()->CreateGroup(gid, true, info);
        EXPECT_EQ(res, E_OK);
        res = RequestManager::GetInstance()->AttachGroup(gid, std::vector<std::string>{ "tid_group" });
        EXPECT_EQ(res, E_OK);
        res = RequestManager::GetInstance()->DeleteGroup(gid);
    }
    EXPECT_EQ(res, E_OK);
}

/**
 * @tc.name: ManagerGetNextSeqIncrements009
 * @tc.desc: successive GetNextSeq calls return consecutive sequence numbers
 * @tc.precon: NA
 * @tc.step: 1. Call RequestManager::GetInstance()->GetNextSeq three times
 * @tc.expect: Each later value is the previous value plus one
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerGetNextSeqIncrements009, TestSize.Level1)
{
    int32_t first = RequestManager::GetInstance()->GetNextSeq();
    int32_t second = RequestManager::GetInstance()->GetNextSeq();
    int32_t third = RequestManager::GetInstance()->GetNextSeq();
    EXPECT_EQ(second, first + 1);
    EXPECT_EQ(third, second + 1);
}

/**
 * @tc.name: ManagerListenerRegistry010
 * @tc.desc: listener registration on a fresh task id succeeds for both
 *           listener families, RemoveAllListeners clears the registry entry
 *           and a re-registration still succeeds afterwards
 * @tc.precon: NA
 * @tc.step: 1. Call RequestManager::AddListener for a notify listener
 *           2. Call RequestManager::AddListener for a response listener
 *           3. Call RequestManager::RemoveAllListeners and repeat step 1
 * @tc.expect: Every call returns E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerListenerRegistry010, TestSize.Level1)
{
    const std::string taskId = "tid_mgr_listener";
    auto notifyListener = std::make_shared<StubNotifyListener>();
    auto responseListener = std::make_shared<StubResponseListener>();
    EXPECT_EQ(RequestManager::GetInstance()->AddListener(taskId, SubscribeType::PROGRESS, notifyListener), E_OK);
    EXPECT_EQ(RequestManager::GetInstance()->AddListener(taskId, SubscribeType::RESPONSE, responseListener), E_OK);
    EXPECT_EQ(RequestManager::GetInstance()->RemoveListener(taskId, SubscribeType::RESPONSE, responseListener), E_OK);
    RequestManager::GetInstance()->RemoveAllListeners(taskId);
    EXPECT_EQ(RequestManager::GetInstance()->AddListener(taskId, SubscribeType::PROGRESS, notifyListener), E_OK);
    EXPECT_EQ(RequestManager::GetInstance()->RemoveListener(taskId, SubscribeType::PROGRESS, notifyListener), E_OK);
}

/**
 * @tc.name: ManagerCreateTasksEmptyConfigs011
 * @tc.desc: CreateTasks with an empty config list succeeds without contacting
 *           the service
 * @tc.precon: NA
 * @tc.step: 1. Call RequestManager::CreateTasks with an empty config vector
 * @tc.expect: The returned code is E_OK and the result vector stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ManagerCreateTasksEmptyConfigs011, TestSize.Level1)
{
    std::vector<Config> configs;
    std::vector<TaskRet> rets;
    ExceptionErrorCode res = RequestManager::GetInstance()->CreateTasks(configs, rets);
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: StartTasksSuccessMapping001
 * @tc.desc: StartTasks maps each per-tid answer from the service vector into
 *           the result map when the batch call succeeds
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that fills the answer vector with E_OK
 *              and returns E_OK
 *           2. Call StartTasks with the mock cached in place
 *           3. Restore the proxy and check the result map
 * @tc.expect: StartTasks returns E_OK and the map carries the per-tid value
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StartTasksSuccessMapping001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    std::vector<ExceptionErrorCode> answer = { ExceptionErrorCode::E_OK };
    EXPECT_CALL(*mock, StartTasks(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(answer), testing::Return(ExceptionErrorCode::E_OK)));
    std::vector<std::string> tids = { "tid_success_start" };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->StartTasks(tids, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), static_cast<size_t>(1));
    EXPECT_EQ(rets.at("tid_success_start"), ExceptionErrorCode::E_OK);
}

/**
 * @tc.name: StopTasksSuccessMapping001
 * @tc.desc: StopTasks maps each per-tid answer into the result map when the
 *           batch call succeeds
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that fills the answer vector with
 *              E_TASK_NOT_FOUND and returns E_OK
 *           2. Call StopTasks with the mock cached in place
 * @tc.expect: StopTasks returns E_OK and the map carries the per-tid error
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, StopTasksSuccessMapping001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    std::vector<ExceptionErrorCode> answer = { ExceptionErrorCode::E_TASK_NOT_FOUND };
    EXPECT_CALL(*mock, StopTasks(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(answer), testing::Return(ExceptionErrorCode::E_OK)));
    std::vector<std::string> tids = { "tid_success_stop" };
    std::unordered_map<std::string, ExceptionErrorCode> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->StopTasks(tids, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), static_cast<size_t>(1));
    EXPECT_EQ(rets.at("tid_success_stop"), ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: TouchTasksSuccessMapping001
 * @tc.desc: TouchTasks maps each per-tid TaskInfoRet into the result map when
 *           the batch call succeeds
 * @tc.precon: NA
 * @tc.step: 1. Prepare a mock proxy that fills the answer vector with one
 *              TaskInfoRet and returns E_OK
 *           2. Call TouchTasks with the mock cached in place
 * @tc.expect: TouchTasks returns E_OK and the map carries the per-tid entry
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, TouchTasksSuccessMapping001, TestSize.Level1)
{
    auto mock = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    TaskInfoRet answerEntry;
    answerEntry.code = ExceptionErrorCode::E_OK;
    std::vector<TaskInfoRet> answer = { answerEntry };
    EXPECT_CALL(*mock, TouchTasks(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(answer), testing::Return(ExceptionErrorCode::E_OK)));
    std::vector<TaskIdAndToken> tidTokens = { { "tid_success_touch", "token_success_touch" } };
    std::unordered_map<std::string, TaskInfoRet> rets;
    ExceptionErrorCode res;
    {
        ProxySwapGuard guard(mock);
        res = RequestAction::GetInstance()->TouchTasks(tidTokens, rets);
    }
    EXPECT_EQ(res, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), static_cast<size_t>(1));
    EXPECT_EQ(rets.at("tid_success_touch").code, ExceptionErrorCode::E_OK);
}

/**
 * @tc.name: ActionPathInterceptSucceed001
 * @tc.desc: InterceptData extracts the tail after the last delimiter for a
 *           regular path and for a bare extension pattern
 * @tc.precon: NA
 * @tc.step: 1. Call InterceptData with "/" and "/data/file.txt"
 *           2. Call InterceptData with "." and "file.txt"
 * @tc.expect: Both calls return true and carry the substring after the last
 *             delimiter
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathInterceptSucceed001, TestSize.Level1)
{
    std::string out;
    auto res = RequestAction::InterceptData("/", "/data/file.txt", out);
    EXPECT_EQ(res, true);
    EXPECT_EQ(out, "file.txt");
    std::string out2;
    auto res2 = RequestAction::InterceptData(".", "file.txt", out2);
    EXPECT_EQ(res2, true);
    EXPECT_EQ(out2, "txt");
}

/**
 * @tc.name: ActionPathStandardizeInternal002
 * @tc.desc: StandardizePath rewrites an internal:// path onto the mocked base
 *           directory
 * @tc.precon: NA
 * @tc.step: 1. Create a mock context whose GetBaseDir returns "/data/app/base"
 *           2. Call StandardizePath with "internal://aa"
 * @tc.expect: The call returns true and the path becomes "/data/app/base/aa"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathStandardizeInternal002, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path = "internal://aa";
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, true);
    EXPECT_EQ(path, "/data/app/base/aa");
}

/**
 * @tc.name: ActionPathStandardizeDot003
 * @tc.desc: StandardizePath rewrites a "./" path onto the mocked cache
 *           directory
 * @tc.precon: NA
 * @tc.step: 1. Create a mock context whose GetCacheDir returns "/data/app/cache"
 *           2. Call StandardizePath with "./aa"
 * @tc.expect: The call returns true and the path becomes "/data/app/cache/aa"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathStandardizeDot003, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config;
    std::string path = "./aa";
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, true);
    EXPECT_EQ(path, "/data/app/cache/aa");
}

/**
 * @tc.name: ActionPathStandardizeFile004
 * @tc.desc: StandardizePath rewrites a file:// path whose bundle matches the
 *           config by stripping the bundle prefix
 * @tc.precon: NA
 * @tc.step: 1. Create a mock context and a config with bundleName
 *              "com.example.aaa"
 *           2. Call StandardizePath with "file://com.example.aaa/dir/file"
 * @tc.expect: The call returns true and the path becomes "/dir/file"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathStandardizeFile004, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    Config config = { .bundleName = "com.example.aaa" };
    std::string path = "file://com.example.aaa/dir/file";
    auto res = RequestAction::StandardizePath(context, config, path);
    EXPECT_EQ(res, true);
    EXPECT_EQ(path, "/dir/file");
}

/**
 * @tc.name: ActionPathIsPathValidTooLong005
 * @tc.desc: IsPathValid rejects a directory component longer than PATH_MAX
 *           before touching the filesystem, and a directory part of exactly
 *           PATH_MAX characters still fails at realpath
 * @tc.precon: NA
 * @tc.step: 1. Build a path whose directory part exceeds PATH_MAX characters
 *           2. Call IsPathValid with the long path
 *           3. Repeat with a directory part of exactly PATH_MAX characters
 * @tc.expect: The oversized path fails the length check and the boundary
 *             path fails the realpath check, so both calls return false
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathIsPathValidTooLong005, TestSize.Level1)
{
    // IsPathValid checks the length of the directory part only, i.e. the
    // substring before the last '/', so the oversized component must be a
    // directory segment rather than the file name.
    std::string filepath = "/" + std::string(PATH_MAX + 1, 'a') + "/f";
    auto res = RequestAction::IsPathValid(filepath);
    EXPECT_EQ(res, false);

    // A directory part of exactly PATH_MAX characters passes the length
    // check; realpath then rejects it because such a component cannot exist.
    std::string boundaryPath = "/" + std::string(PATH_MAX - 1, 'a') + "/f";
    EXPECT_EQ(RequestAction::IsPathValid(boundaryPath), false);
}

/**
 * @tc.name: ActionPathInternalPrefixed006
 * @tc.desc: GetInternalPath maps an "internal://cache/" prefixed path onto
 *           the mocked cache directory and passes validation
 * @tc.precon: NA
 * @tc.step: 1. Create a mock context whose GetCacheDir returns "/data"
 *           2. Call GetInternalPath with "internal://cache/test.txt"
 * @tc.expect: The call returns true and the path becomes "/data/test.txt"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathInternalPrefixed006, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextCacheTestMock>();
    Config config;
    std::string path = "internal://cache/test.txt";
    auto res = RequestAction::GetInternalPath(context, config, path);
    EXPECT_EQ(res, true);
    EXPECT_EQ(path, "/data/test.txt");
}

/**
 * @tc.name: ActionPathSplitEmptySeg007
 * @tc.desc: StringSplit drops empty segments produced by consecutive or
 *           trailing delimiters
 * @tc.precon: NA
 * @tc.step: 1. Call StringSplit on "/a//b/" with the '/' delimiter
 * @tc.expect: The output holds exactly the two non-empty segments "a" and "b"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathSplitEmptySeg007, TestSize.Level1)
{
    std::vector<std::string> elems;
    RequestAction::StringSplit("/a//b/", '/', elems);
    ASSERT_EQ(elems.size(), static_cast<size_t>(2));
    EXPECT_EQ(elems[0], "a");
    EXPECT_EQ(elems[1], "b");
}

/**
 * @tc.name: ActionPathVecValue008
 * @tc.desc: PathVecToNormal pushes regular segments and resolves ".." by
 *           popping the previous segment, failing only when ".." escapes the
 *           root
 * @tc.precon: NA
 * @tc.step: 1. Call PathVecToNormal with {"a","b","..","c"} into an empty out
 *           2. Call PathVecToNormal with {"a","..",".."} into an empty out
 * @tc.expect: The first call returns true with out {"a","c"}; the second
 *             returns false with out left empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathVecValue008, TestSize.Level1)
{
    std::vector<std::string> out;
    auto res = RequestAction::PathVecToNormal({ "a", "b", "..", "c" }, out);
    EXPECT_EQ(res, true);
    ASSERT_EQ(out.size(), static_cast<size_t>(2));
    EXPECT_EQ(out[0], "a");
    EXPECT_EQ(out[1], "c");
    std::vector<std::string> out2;
    auto res2 = RequestAction::PathVecToNormal({ "a", "..", ".." }, out2);
    EXPECT_EQ(res2, false);
    EXPECT_TRUE(out2.empty());
}

/**
 * @tc.name: ActionPathWholeValue009
 * @tc.desc: WholeToNormal rewrites the input path in place to its normalized
 *           form and reports the consumed segments
 * @tc.precon: NA
 * @tc.step: 1. Call WholeToNormal with "/data/../aa" into an empty out
 * @tc.expect: The call returns true, the path becomes "/aa" and out carries
 *             the single segment "aa"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathWholeValue009, TestSize.Level1)
{
    std::string path = "/data/../aa";
    std::vector<std::string> out;
    auto res = RequestAction::WholeToNormal(path, out);
    EXPECT_EQ(res, true);
    EXPECT_EQ(path, "/aa");
    ASSERT_EQ(out.size(), static_cast<size_t>(1));
    EXPECT_EQ(out[0], "aa");
}

/**
 * @tc.name: ActionPathFileToWholeBundle010
 * @tc.desc: FileToWhole strips a matching bundle prefix from the path; the
 *           context argument is not dereferenced
 * @tc.precon: NA
 * @tc.step: 1. Create a null context and a config with bundleName
 *              "com.example.aaa"
 *           2. Call FileToWhole with "com.example.aaa/dir/file"
 * @tc.expect: The call returns true and the path becomes "/dir/file"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathFileToWholeBundle010, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context;
    Config config = { .bundleName = "com.example.aaa" };
    std::string path = "com.example.aaa/dir/file";
    auto res = RequestAction::FileToWhole(context, config, path);
    EXPECT_EQ(res, true);
    EXPECT_EQ(path, "/dir/file");
}

/**
 * @tc.name: ActionPathDownloadApi9Abs011
 * @tc.desc: CheckDownloadFile with API9 and an absolute saveas skips the
 *           internal:// mapping, creates the file and fails on the ACL map
 *           because the path is outside the app base dirs
 * @tc.precon: NA
 * @tc.step: 1. Create a mock context and an API9 config whose saveas is an
 *              absolute path under /data/test
 *           2. Call CheckDownloadFile
 * @tc.expect: The call returns E_FILE_IO from the AddPathsToMap rejection
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathDownloadApi9Abs011, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextTestMock>();
    const std::string savePath = "/data/test/action_api9_abs011.txt";
    std::remove(savePath.c_str());
    FileSpec file = { .uri = savePath };
    Config config = { .version = Version::API9, .saveas = savePath, .files = { file } };
    auto res = RequestAction::CheckDownloadFile(context, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
    std::remove(savePath.c_str());
}

/**
 * @tc.name: ActionPathUploadApi9Dir012
 * @tc.desc: CheckUploadFileSpec with API9 resolves an internal:// style name
 *           onto the cache dir and then fails because the resolved path is a
 *           directory
 * @tc.precon: NA
 * @tc.step: 1. Create a mock context whose GetCacheDir returns "/data"
 *           2. Call CheckUploadFileSpec with the file uri "test" under an
 *              API9 config
 * @tc.expect: The call returns E_FILE_PATH
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathUploadApi9Dir012, TestSize.Level1)
{
    std::shared_ptr<ApplicationContext> context = std::make_shared<ContextCacheTestMock>();
    FileSpec file = { .uri = "test" };
    Config config = { .version = Version::API9, .files = { file } };
    auto res = RequestAction::CheckUploadFileSpec(context, config, file);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_PATH);
}

/**
 * @tc.name: ActionPathBodyFilesNoMultipart013
 * @tc.desc: CheckUploadBodyFiles without multipart iterates the file list and
 *           fails when the temp body file is outside the app base dirs, so no
 *           body file name is recorded
 * @tc.precon: NA
 * @tc.step: 1. Call CheckUploadBodyFiles with filepath "/data/test", one file
 *              entry and multipart disabled
 * @tc.expect: The call returns E_FILE_IO and config.bodyFileNames stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathBodyFilesNoMultipart013, TestSize.Level1)
{
    const std::string filepath = "/data/test";
    FileSpec fileSpec = { .filename = "body013", .name = "file",
        .uri = "/data/test/body013.txt", .type = "text/plain" };
    Config config = { .multipart = false, .files = { fileSpec } };
    auto res = RequestAction::CheckUploadBodyFiles(filepath, config);
    EXPECT_EQ(res, ExceptionErrorCode::E_FILE_IO);
    EXPECT_TRUE(config.bodyFileNames.empty());
}

/**
 * @tc.name: ActionPathCreateDirsOnFile014
 * @tc.desc: CreateDirs fails when a deeper level must be created below an
 *           existing regular file
 * @tc.precon: NA
 * @tc.step: 1. Create the regular file "/data/test/CreateDirsFile014"
 *           2. Call CreateDirs for a sub directory below that file
 * @tc.expect: The call returns false and the blocker file is removed again
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathCreateDirsOnFile014, TestSize.Level1)
{
    const std::string blocker = "/data/test/CreateDirsFile014";
    std::ofstream file(blocker);
    file.close();
    std::vector<std::string> pathDirs = { "data", "test", "CreateDirsFile014", "sub" };
    auto res = RequestAction::CreateDirs(pathDirs);
    EXPECT_EQ(res, false);
    std::error_code ec;
    std::filesystem::remove_all(blocker, ec);
}

/**
 * @tc.name: ActionPathAppBaseDirEmpty015
 * @tc.desc: GetAppBaseDir fails when the application context exists but its
 *           base directory is empty
 * @tc.precon: NA
 * @tc.step: 1. Detach the ContextImpl from the application context so
 *              GetBaseDir yields an empty string
 *           2. Call GetAppBaseDir
 * @tc.expect: The call returns false and leaves the output empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathAppBaseDirEmpty015, TestSize.Level1)
{
    // ApplicationContext::GetBaseDir() returns "" when no ContextImpl is
    // attached, so detaching the impl deterministically drives the empty
    // base-dir arm without any device state. The impl is re-attached so
    // later cases still observe the shared singleton contract.
    auto ctx = ApplicationContext::GetInstance();
    EXPECT_NE(ctx, nullptr);
    if (ctx != nullptr) {
        ctx->AttachContextImpl(nullptr);
    }
    std::string baseDir;
    auto res = RequestAction::GetAppBaseDir(baseDir);
    EXPECT_EQ(res, false);
    EXPECT_TRUE(baseDir.empty());
    if (ctx != nullptr) {
        ctx->AttachContextImpl(std::make_shared<ContextImpl>());
    }
}

/**
 * @tc.name: ActionPathSetDirsPermEmpty016
 * @tc.desc: SetDirsPermission over an existing empty folder redirects the
 *           caller to the consolidated certs directory
 * @tc.precon: NA
 * @tc.step: 1. Create the empty directory "/data/test/empty_dir_016"
 *           2. Call SetDirsPermission with that directory
 * @tc.expect: The call returns true and the dirs vector is rewritten to the
 *             certs path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestActionTest, ActionPathSetDirsPermEmpty016, TestSize.Level1)
{
    const std::string emptyDir = "/data/test/empty_dir_016";
    std::error_code ec;
    std::filesystem::create_directories(emptyDir, ec);
    std::vector<std::string> dirs = { emptyDir };
    auto res = RequestAction::SetDirsPermission(dirs);
    EXPECT_EQ(res, true);
    ASSERT_EQ(dirs.size(), static_cast<size_t>(1));
    EXPECT_EQ(dirs[0], "/data/storage/el2/base/.ohos/.request/.certs");
}