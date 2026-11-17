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

#include <string>
#define private public
#define protected public

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include "download_server_ipc_interface_code.h"
#include "gmock/gmock.h"
#include "log.h"
#include "parcel_helper.h"
#include "request_common.h"
#include "request_running_task_count.h"
#include "runcount_notify_stub.h"

using namespace testing::ext;
using namespace OHOS::Request;

#undef private
#undef protected

class RuncountNotifyStubTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RuncountNotifyStubTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RuncountNotifyStubTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void RuncountNotifyStubTest::SetUp(void)
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

void RuncountNotifyStubTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
}

/**
 * @tc.name: OnCallBackTest001
 * @tc.desc: Test the OnCallBack function of RunCountNotifyStub
 * @tc.precon: NA
 * @tc.step: 1. Create a Notify object
 *           2. Call CallBack method with the Notify object
 *           3. Create a TaskInfo object
 *           4. Call Done method with the TaskInfo object
 *           5. Write an int64_t value to MessageParcel
 *           6. Call OnCallBack method with the MessageParcel
 *           7. Check if the count is updated correctly
 * @tc.expect: The count should be updated to the expected value
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RuncountNotifyStubTest, OnCallBackTest001, TestSize.Level1)
{
    Notify notify;
    RunCountNotifyStub::GetInstance()->CallBack(notify);
    TaskInfo taskInfo;
    RunCountNotifyStub::GetInstance()->Done(taskInfo);
    int64_t except = 10; // 10 is except value
    int old = FwkRunningTaskCountManager::GetInstance()->GetCount();
    OHOS::MessageParcel data;
    data.WriteInt64(except);
    RunCountNotifyStub::GetInstance()->OnCallBack(data);
    int count = FwkRunningTaskCountManager::GetInstance()->GetCount();
    EXPECT_EQ(count, except);
    FwkRunningTaskCountManager::GetInstance()->SetCount(old);
    count = FwkRunningTaskCountManager::GetInstance()->GetCount();
    EXPECT_EQ(count, old);
}

/**
 * @tc.name: OnRemoteRequestTest001
 * @tc.desc: Test the OnRemoteRequest function of RunCountNotifyStub
 * @tc.precon: NA
 * @tc.step: 1. Prepare MessageParcel data with interface token
 *           2. Call OnRemoteRequest with REQUEST_NOTIFY_RUNCOUNT code
 *           3. Prepare another MessageParcel with correct descriptor
 *           4. Call OnRemoteRequest with REQUEST_DONE_NOTIFY code
 *           5. Verify the return value is not zero
 * @tc.expect: OnRemoteRequest should handle different request codes correctly
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RuncountNotifyStubTest, OnRemoteRequestTest001, TestSize.Level1)
{
    uint32_t code = static_cast<uint32_t>(RequestNotifyInterfaceCode::REQUEST_NOTIFY_RUNCOUNT);
    OHOS::MessageParcel data;
    std::u16string token = u"token";
    data.WriteInterfaceToken(token);
    data.WriteInt64(0);
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;
    RunCountNotifyStub runCount = RunCountNotifyStub();
    runCount.OnRemoteRequest(code, data, reply, option);
    OHOS::MessageParcel data1;
    token = runCount.GetDescriptor();
    data1.WriteInterfaceToken(token);
    data1.WriteInt64(0);
    code = static_cast<uint32_t>(RequestNotifyInterfaceCode::REQUEST_DONE_NOTIFY);
    runCount.OnRemoteRequest(code, data1, reply, option);
    EXPECT_NE(runCount.OnRemoteRequest(code, data1, reply, option), 0);
    code = static_cast<uint32_t>(RequestNotifyInterfaceCode::REQUEST_NOTIFY_RUNCOUNT);
}

/**
 * @tc.name: OnRemoteRequestTest002
 * @tc.desc: OnRemoteRequest falls through to the default IPC stub handler when
 *           the caller sends a wrong interface token
 * @tc.precon: NA
 * @tc.step: 1. Prepare MessageParcel with a foreign interface token
 *           2. Call OnRemoteRequest with REQUEST_NOTIFY_RUNCOUNT code
 * @tc.expect: Return value is not OHOS::ERR_NONE
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RuncountNotifyStubTest, OnRemoteRequestTest002, TestSize.Level1)
{
    uint32_t code = static_cast<uint32_t>(RequestNotifyInterfaceCode::REQUEST_NOTIFY_RUNCOUNT);
    OHOS::MessageParcel data;
    std::u16string token = u"ohos.foreign.token";
    data.WriteInterfaceToken(token);
    data.WriteInt64(0);
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;
    RunCountNotifyStub runCount = RunCountNotifyStub();
    int32_t ret = runCount.OnRemoteRequest(code, data, reply, option);
    EXPECT_NE(ret, OHOS::ERR_NONE);
}

/**
 * @tc.name: OnRemoteRequestTest003
 * @tc.desc: OnRemoteRequest rejects REQUEST_NOTIFY_RUNCOUNT from a caller whose
 *           uid is not the system service uid 3815, and the run count stays
 *           unchanged
 * @tc.precon: NA
 * @tc.step: 1. Read the current run count
 *           2. Prepare MessageParcel with the correct descriptor and a count
 *              payload
 *           3. Call OnRemoteRequest with REQUEST_NOTIFY_RUNCOUNT code
 * @tc.expect: Return value is not OHOS::ERR_NONE and the stored count is unchanged
 *             (the test process uid differs from 3815)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RuncountNotifyStubTest, OnRemoteRequestTest003, TestSize.Level1)
{
    int old = FwkRunningTaskCountManager::GetInstance()->GetCount();
    uint32_t code = static_cast<uint32_t>(RequestNotifyInterfaceCode::REQUEST_NOTIFY_RUNCOUNT);
    OHOS::MessageParcel data;
    data.WriteInterfaceToken(RunCountNotifyStub().GetDescriptor());
    data.WriteInt64(old + 1000);
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;
    RunCountNotifyStub runCount = RunCountNotifyStub();
    int32_t ret = runCount.OnRemoteRequest(code, data, reply, option);
    EXPECT_NE(ret, OHOS::ERR_NONE);
    EXPECT_EQ(FwkRunningTaskCountManager::GetInstance()->GetCount(), old);
}

/**
 * @tc.name: OnRemoteRequestTest004
 * @tc.desc: OnRemoteRequest rejects an interface code other than
 *           REQUEST_NOTIFY_RUNCOUNT even with the correct descriptor
 * @tc.precon: NA
 * @tc.step: 1. Prepare MessageParcel with the correct descriptor
 *           2. Call OnRemoteRequest with an out-of-range code
 * @tc.expect: Return value is not OHOS::ERR_NONE
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RuncountNotifyStubTest, OnRemoteRequestTest004, TestSize.Level1)
{
    uint32_t code = 0xFFFF;
    OHOS::MessageParcel data;
    data.WriteInterfaceToken(RunCountNotifyStub().GetDescriptor());
    data.WriteInt64(0);
    OHOS::MessageParcel reply;
    OHOS::MessageOption option;
    RunCountNotifyStub runCount = RunCountNotifyStub();
    int32_t ret = runCount.OnRemoteRequest(code, data, reply, option);
    EXPECT_NE(ret, OHOS::ERR_NONE);
}

/**
 * @tc.name: CheckCallingPermissionTest001
 * @tc.desc: CheckCallingPermission is false for a wrong interface code and for
 *           a caller that is not the system service uid/token
 * @tc.precon: NA
 * @tc.step: 1. Call CheckCallingPermission with the valid code; the test
 *              process does not run as uid 3815/TOKEN_NATIVE
 *           2. Call CheckCallingPermission with an invalid code
 * @tc.expect: Both calls return false
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RuncountNotifyStubTest, CheckCallingPermissionTest001, TestSize.Level1)
{
    RunCountNotifyStub runCount = RunCountNotifyStub();
    uint32_t validCode = static_cast<uint32_t>(RequestNotifyInterfaceCode::REQUEST_NOTIFY_RUNCOUNT);
    EXPECT_FALSE(runCount.CheckCallingPermission(validCode));
    EXPECT_FALSE(runCount.CheckCallingPermission(0xFFFF));
}

/**
 * @tc.name: OnCallBackTest002
 * @tc.desc: OnCallBack propagates a negative run count payload into the
 *           framework running task count manager
 * @tc.precon: NA
 * @tc.step: 1. Read the current count and prepare a parcel with a negative
 *              payload
 *           2. Call OnCallBack
 *           3. Restore the previous count
 * @tc.expect: The stored count equals the negative payload
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RuncountNotifyStubTest, OnCallBackTest002, TestSize.Level1)
{
    int64_t payload = -5;
    int old = FwkRunningTaskCountManager::GetInstance()->GetCount();
    OHOS::MessageParcel data;
    data.WriteInt64(payload);
    RunCountNotifyStub::GetInstance()->OnCallBack(data);
    EXPECT_EQ(FwkRunningTaskCountManager::GetInstance()->GetCount(), payload);
    FwkRunningTaskCountManager::GetInstance()->SetCount(old);
    EXPECT_EQ(FwkRunningTaskCountManager::GetInstance()->GetCount(), old);
}