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

#include <optional>

#include "constant.h"
#include "refbase.h"
#include "runcount_notify_stub.h"
#include "running_task_count.h"
#define private public
#define protected public

#include <gtest/gtest.h>
#include <sys/socket.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "gmock/gmock.h"
#include "log.h"
#include "request_common.h"
#include "request_manager.h"
#include "request_manager_impl.h"
#include "request_running_task_count.h"
#include "request_service_proxy.h"
#include "system_ability_definition.h"
#include "want_agent.h"
#include "want_agent_helper.h"

using namespace testing::ext;
using namespace OHOS::Request;

#undef private
#undef protected

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

class RequestManagerImplTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    OHOS::sptr<RequestServiceInterface> testProxy;
    OHOS::sptr<MockRequestServiceInterface> exceptProxy;
};

void RequestManagerImplTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RequestManagerImplTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void RequestManagerImplTest::SetUp(void)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    testProxy = RequestManagerImpl::GetInstance()->GetRequestServiceProxy(true);
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    auto proxy = RequestManagerImpl::GetInstance()->GetRequestServiceProxy(true);
    EXPECT_TRUE(proxy == (static_cast<OHOS::sptr<RequestServiceInterface>>(exceptProxy)));
    // input testCase setup step，setup invoked before each testCase
    testing::UnitTest *test = testing::UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const testing::TestInfo *testInfo = test->current_test_info();
    ASSERT_NE(testInfo, nullptr);
    string testCaseName = string(testInfo->name());
    REQUEST_HILOGI("[SetUp] %{public}s start", testCaseName.c_str());
    GTEST_LOG_(INFO) << testCaseName.append(" start");
}

void RequestManagerImplTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = testProxy;
    testProxy = nullptr;
    exceptProxy = nullptr;
}

/**
 * @tc.name: CreateTest001
 * @tc.desc: Test Create interface with channel recovery and task creation scenarios
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Simulate channel broken state
 *           2. Expect OpenChannel to return E_TASK_STATE first, then E_OK
 *           3. Create config with API9 version
 *           4. Expect Create to return E_CHANNEL_NOT_OPEN first, then E_OK
 *           5. Expect Subscribe to return E_OK
 *           6. Expect Start to return E_OK first, then E_CHANNEL_NOT_OPEN
 * @tc.expect: First Create call succeeds (E_OK), second returns E_CHANNEL_NOT_OPEN
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateTest001, TestSize.Level1)
{
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_))
        .WillOnce(testing::Return(E_TASK_STATE))
        .WillOnce(testing::Return(E_OK));
    Config config;
    config.version = Version::API9;
    int32_t seq = 1;
    std::string tid = "1";
    EXPECT_CALL(*exceptProxy, Create(testing::_, tid))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN))
        .WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, Subscribe(testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, Start(testing::_))
        .WillOnce(testing::Return(E_OK))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_OK);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: GetTaskTest001
 * @tc.desc: Test GetTask interface with channel broken and error handling
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create config, token and tid
 *           2. Simulate channel broken state
 *           3. Expect OpenChannel to return E_CHANNEL_NOT_OPEN repeatedly
 *           4. Expect Subscribe to return E_OK
 *           5. Expect GetTask to return E_CHANNEL_NOT_OPEN first, then E_OTHER
 * @tc.expect: First GetTask call returns E_OK (channel recovery), second returns E_OTHER
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, GetTaskTest001, TestSize.Level1)
{
    Config config;
    string token = "token";
    string tid = "tid";
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_)).WillRepeatedly(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_CALL(*exceptProxy, Subscribe(testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, GetTask(tid, token, testing::_))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN))
        .WillOnce(testing::Return(E_OTHER));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->GetTask(tid, token, config), E_OK);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->GetTask(tid, token, config), E_OTHER);
}

/**
 * @tc.name: StartTest001
 * @tc.desc: Test Start interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Expect Start to return E_CHANNEL_NOT_OPEN
 *           3. Call Start with the test task ID
 * @tc.expect: Start returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, StartTest001, TestSize.Level1)
{
    string tid = "tid";
    EXPECT_CALL(*exceptProxy, Start(tid)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Start(tid), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: StopTest001
 * @tc.desc: Test Stop interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Expect Stop to return E_CHANNEL_NOT_OPEN
 *           3. Call Stop with the test task ID
 * @tc.expect: Stop returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, StopTest001, TestSize.Level1)
{
    string tid = "tid";
    EXPECT_CALL(*exceptProxy, Stop(tid)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Stop(tid), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: QueryTest001
 * @tc.desc: Test Query interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task info and task ID
 *           2. Expect Query to return E_CHANNEL_NOT_OPEN
 *           3. Call Query with the test task ID and info
 * @tc.expect: Query returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, QueryTest001, TestSize.Level1)
{
    TaskInfo info;
    string tid = "tid";
    EXPECT_CALL(*exceptProxy, Query(tid, testing::_)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Query(tid, info), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: TouchTest001
 * @tc.desc: Test Touch interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task info, task ID, and token
 *           2. Expect Touch to return E_CHANNEL_NOT_OPEN
 *           3. Call Touch with the test parameters
 * @tc.expect: Touch returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, TouchTest001, TestSize.Level1)
{
    TaskInfo info;
    string tid = "tid";
    string token = "token";
    EXPECT_CALL(*exceptProxy, Touch(tid, token, testing::_)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Touch(tid, token, info), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: SearchTest001
 * @tc.desc: Test Search interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test filter and task ID vector
 *           2. Expect Search to return E_CHANNEL_NOT_OPEN
 *           3. Call Search with the test parameters
 * @tc.expect: Search returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, SearchTest001, TestSize.Level1)
{
    Filter filter;
    std::vector<std::string> tids;
    EXPECT_CALL(*exceptProxy, Search(testing::_, tids)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Search(filter, tids), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: ShowTest001
 * @tc.desc: Test Show interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task info and task ID
 *           2. Expect Show to return E_CHANNEL_NOT_OPEN
 *           3. Call Show with the test parameters
 * @tc.expect: Show returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ShowTest001, TestSize.Level1)
{
    TaskInfo info;
    string tid = "tid";
    EXPECT_CALL(*exceptProxy, Show(tid, testing::_)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Show(tid, info), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: PauseTest001
 * @tc.desc: Test Pause interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Expect Pause with API10 version to return E_CHANNEL_NOT_OPEN
 *           3. Call Pause with the test task ID and version
 * @tc.expect: Pause returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, PauseTest001, TestSize.Level1)
{
    string tid = "tid";
    EXPECT_CALL(*exceptProxy, Pause(tid, Version::API10)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Pause(tid, Version::API10), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: QueryMimeTypeTest001
 * @tc.desc: Test QueryMimeType interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID and mime type
 *           2. Expect QueryMimeType to return E_CHANNEL_NOT_OPEN
 *           3. Call QueryMimeType with the test parameters
 * @tc.expect: QueryMimeType returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, QueryMimeTypeTest001, TestSize.Level1)
{
    string tid = "tid";
    std::string mimeType = "mimeType";
    EXPECT_CALL(*exceptProxy, QueryMimeType(tid, mimeType)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->QueryMimeType(tid, mimeType), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: RemoveTest001
 * @tc.desc: Test Remove interface with task not found scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Call Remove with the test task ID and API10 version
 * @tc.expect: Remove returns E_TASK_NOT_FOUND as expected for non-existent task
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, RemoveTest001, TestSize.Level1)
{
    string tid = "tid";
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Remove(tid, Version::API10), E_TASK_NOT_FOUND);
}

/**
 * @tc.name: ResumeTest001
 * @tc.desc: Test Resume interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Expect Resume to return E_CHANNEL_NOT_OPEN
 *           3. Call Resume with the test task ID
 * @tc.expect: Resume returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ResumeTest001, TestSize.Level1)
{
    string tid = "tid";
    EXPECT_CALL(*exceptProxy, Resume(tid)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Resume(tid), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: SubscribeTest001
 * @tc.desc: Test Subscribe interface with channel broken and recovery scenarios
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Simulate channel broken state
 *           3. Expect OpenChannel to return E_CHANNEL_NOT_OPEN
 *           4. Expect Subscribe to return E_CHANNEL_NOT_OPEN first, then E_OK
 *           5. Call Subscribe with the test task ID
 * @tc.expect: Subscribe returns E_OK after channel recovery
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, SubscribeTest001, TestSize.Level1)
{
    string taskId = "taskId";
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_CALL(*exceptProxy, Subscribe(taskId))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN))
        .WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Subscribe(taskId), E_OK);
}

/**
 * @tc.name: UnsubscribeTest001
 * @tc.desc: Test Unsubscribe interface with channel not open scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Expect Unsubscribe to return E_CHANNEL_NOT_OPEN
 *           3. Call Unsubscribe with the test task ID
 * @tc.expect: Unsubscribe returns E_CHANNEL_NOT_OPEN as expected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, UnsubscribeTest001, TestSize.Level1)
{
    string taskId = "taskId";
    EXPECT_CALL(*exceptProxy, Unsubscribe(taskId)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Unsubscribe(taskId), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: SubRunCountTest001
 * @tc.desc: Test SubRunCount and UnsubRunCount interfaces for run count subscription
 * @tc.precon: RequestManagerImpl instance is initialized, RunCountNotifyStub is available
 * @tc.step: 1. Get RunCountNotifyStub instance
 *           2. Call SubRunCount with the listener
 *           3. Verify SubRunCount returns E_OK
 *           4. Call UnsubRunCount
 *           5. Verify UnsubRunCount returns E_OK
 * @tc.expect: Both SubRunCount and UnsubRunCount return E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, SubRunCountTest001, TestSize.Level1)
{
    auto listener = RunCountNotifyStub::GetInstance();
    EXPECT_EQ(RequestManagerImpl::GetInstance()->SubRunCount(listener), E_OK);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->UnsubRunCount(), E_OK);
}

/**
 * @tc.name: EnsureChannelOpenTest001
 * @tc.desc: Test EnsureChannelOpen interface with channel broken and recovery scenarios
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Simulate channel broken state
 *           2. Expect OpenChannel to return E_CHANNEL_NOT_OPEN first, then E_OK
 *           3. Call EnsureChannelOpen twice
 * @tc.expect: First call returns E_CHANNEL_NOT_OPEN, second returns E_OK after recovery
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, EnsureChannelOpenTest001, TestSize.Level1)
{
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN))
        .WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->EnsureChannelOpen(), E_CHANNEL_NOT_OPEN);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->EnsureChannelOpen(), E_OK);
}

/**
 * @tc.name: OnResponseReceiveTest001
 * @tc.desc: Test OnResponseReceive interface with valid response handling
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Create a Response instance
 *           3. Call OnResponseReceive with the response
 * @tc.expect: OnResponseReceive processes the response without errors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, OnResponseReceiveTest001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    std::shared_ptr<Response> response = std::make_shared<Response>();
    RequestManagerImpl::GetInstance()->OnResponseReceive(response);
}

/**
 * @tc.name: OnNotifyDataReceiveTest001
 * @tc.desc: Test OnNotifyDataReceive interface with valid notify data
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Create a NotifyData instance
 *           3. Call OnNotifyDataReceive with the notify data
 * @tc.expect: OnNotifyDataReceive processes the data without errors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, OnNotifyDataReceiveTest001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    std::shared_ptr<NotifyData> notifyData = std::make_shared<NotifyData>();
    RequestManagerImpl::GetInstance()->OnNotifyDataReceive(notifyData);
}

/**
 * @tc.name: UnsubscribeSA001
 * @tc.desc: Test UnsubscribeSA interface for system ability unsubscription
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Call UnsubscribeSA
 * @tc.expect: UnsubscribeSA executes without errors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, UnsubscribeSA001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    RequestManagerImpl::GetInstance()->UnsubscribeSA();
}

void RMITestCallback()
{
}

/**
 * @tc.name: OnAddSystemAbility001
 * @tc.desc: Test OnAddSystemAbility interface with system ability addition handling
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test device ID
 *           2. Create SystemAbilityStatusChangeListener
 *           3. Set up test callback
 *           4. Test with DOWNLOAD_SERVICE_ID
 *           5. Restore null listener
 *           6. Test with PRINT_SERVICE_ID
 * @tc.expect: Listener handles both service IDs appropriately
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, OnAddSystemAbility001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    string deviceId = "deviceId";
    RequestManagerImpl::SystemAbilityStatusChangeListener listener =
        RequestManagerImpl::SystemAbilityStatusChangeListener();
    RequestManagerImpl::GetInstance()->RestoreListener(RMITestCallback);
    listener.OnAddSystemAbility(OHOS::DOWNLOAD_SERVICE_ID, deviceId);
    RequestManagerImpl::GetInstance()->RestoreListener(nullptr);
    listener.OnAddSystemAbility(OHOS::PRINT_SERVICE_ID, deviceId);
}

/**
 * @tc.name: OnRemoveSystemAbility001
 * @tc.desc: Test OnRemoveSystemAbility interface with system ability removal handling
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test device ID
 *           2. Create SystemAbilityStatusChangeListener
 *           3. Test with DOWNLOAD_SERVICE_ID
 *           4. Test with PRINT_SERVICE_ID
 * @tc.expect: Listener handles both service ID removals appropriately
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, OnRemoveSystemAbility001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    string deviceId = "deviceId";
    RequestManagerImpl::SystemAbilityStatusChangeListener listener =
        RequestManagerImpl::SystemAbilityStatusChangeListener();
    listener.OnRemoveSystemAbility(OHOS::DOWNLOAD_SERVICE_ID, deviceId);
    listener.OnRemoveSystemAbility(OHOS::PRINT_SERVICE_ID, deviceId);
}

/**
 * @tc.name: CreateTest002
 * @tc.desc: Test Create interface with multiple error scenarios and channel states
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Simulate channel broken state
 *           2. Expect OpenChannel to return E_TASK_STATE repeatedly
 *           3. Create config with API10 version
 *           4. Expect Create to return E_FILE_PATH, then E_OK twice
 *           5. Verify proxy equality after operations
 * @tc.expect: Create returns appropriate error codes, proxy remains consistent
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateTest002, TestSize.Level1)
{
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_)).WillRepeatedly(testing::Return(E_TASK_STATE));
    Config config;
    config.version = Version::API10;
    int32_t seq = 1;
    std::string tid = "1";
    EXPECT_CALL(*exceptProxy, Create(testing::_, tid))
        .WillOnce(testing::Return(E_FILE_PATH))
        .WillOnce(testing::Return(E_OK))
        .WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_FILE_PATH);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_OK);
    RequestManagerImpl::GetInstance()->Create(config, seq, tid);
    auto proxy = RequestManagerImpl::GetInstance()->GetRequestServiceProxy(true);
    EXPECT_TRUE(proxy == (static_cast<OHOS::sptr<RequestServiceInterface>>(exceptProxy)));
}

/**
 * @tc.name: SubscribeTest002
 * @tc.desc: Test Subscribe interface with channel recovery scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test task ID
 *           2. Simulate channel broken state
 *           3. Expect OpenChannel to return E_CHANNEL_NOT_OPEN
 *           4. Expect Subscribe to return E_OK
 *           5. Call Subscribe with the test task ID
 * @tc.expect: Subscribe succeeds with E_OK despite initial channel issues
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, SubscribeTest002, TestSize.Level1)
{
    string taskId = "taskId";
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_CALL(*exceptProxy, Subscribe(taskId)).WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Subscribe(taskId), E_OK);
}

/**
 * @tc.name: EnsureChannelOpenTest002
 * @tc.desc: Test EnsureChannelOpen interface with existing message receiver
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create ResponseMessageReceiver instance
 *           2. Set msgReceiver_ to the new instance
 *           3. Call EnsureChannelOpen
 *           4. Simulate channel broken state
 * @tc.expect: EnsureChannelOpen returns E_OK with existing receiver
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, EnsureChannelOpenTest002, TestSize.Level1)
{
    RequestManagerImpl::GetInstance()->msgReceiver_ =
        std::make_shared<ResponseMessageReceiver>(RequestManagerImpl::GetInstance(), -1);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->EnsureChannelOpen(), E_OK);
    RequestManagerImpl::GetInstance()->OnChannelBroken();
}

/**
 * @tc.name: GetTaskTest002
 * @tc.desc: Test GetTask interface with task cache management
 * @tc.precon: RequestManagerImpl instance is initialized, task map is accessible
 * @tc.step: 1. Create test task ID
 *           2. Access tasks map and clear the test task ID
 *           3. Call GetTask with the test task ID
 *           4. Verify task is not null
 *           5. Call GetTask again
 *           6. Clear task from map again
 * @tc.expect: GetTask returns non-null task instances consistently
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, GetTaskTest002, TestSize.Level1)
{
    string taskId = "taskId";
    std::map<std::string, std::shared_ptr<Request>> tasks = RequestManagerImpl::GetInstance()->tasks_;
    tasks.erase(taskId);
    std::shared_ptr<Request> task = RequestManagerImpl::GetInstance()->GetTask(taskId);
    EXPECT_NE(task.get(), nullptr);
    task = RequestManagerImpl::GetInstance()->GetTask(taskId);
    EXPECT_NE(task.get(), nullptr);
    tasks.erase(taskId);
}

/**
 * @tc.name: SubscribeSATest001
 * @tc.desc: Test SubscribeSA interface with listener initialization
 * @tc.precon: RequestManagerImpl instance is initialized, saChangeListener_ is initially null
 * @tc.step: 1. Verify saChangeListener_ is initially null
 *           2. Call SubscribeSA
 *           3. Verify saChangeListener_ is no longer null
 *           4. Call SubscribeSA again
 * @tc.expect: SubscribeSA creates listener on first call, handles duplicate calls gracefully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, SubscribeSATest001, TestSize.Level1)
{
    RequestManagerImpl::GetInstance()->saChangeListener_ = nullptr;
    RequestManagerImpl::GetInstance()->SubscribeSA();
    EXPECT_NE(RequestManagerImpl::GetInstance()->saChangeListener_, nullptr);
    RequestManagerImpl::GetInstance()->SubscribeSA();
}

/**
 * @tc.name: RestoreSubRunCountTest001
 * @tc.desc: Test RestoreSubRunCount interface with null listener handling
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Create null NotifyInterface listener
 *           3. Set requestServiceProxy_ to mock
 *           4. Call RestoreSubRunCount
 * @tc.expect: RestoreSubRunCount handles null listener gracefully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, RestoreSubRunCountTest001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    OHOS::sptr<NotifyInterface> listener(nullptr);
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    RequestManagerImpl::GetInstance()->RestoreSubRunCount();
}

class TestRunCountDemo : public IRunningTaskObserver {
public:
    ~TestRunCountDemo() = default;
    void OnRunningTaskCountUpdate(int count) override
    {
        return;
    }
};

/**
 * @tc.name: OnAddSystemAbility002
 * @tc.desc: Test OnAddSystemAbility interface with observer management
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test device ID
 *           2. Restore null listener
 *           3. Verify no observers initially
 *           4. Create test observer
 *           5. Add observer to manager
 *           6. Verify observer exists
 *           7. Test with PRINT_SERVICE_ID
 *           8. Clear observers
 * @tc.expect: Listener manages observers correctly for non-download services
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, OnAddSystemAbility002, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    string deviceId = "deviceId";
    RequestManagerImpl::GetInstance()->RestoreListener(nullptr);
    EXPECT_FALSE(FwkRunningTaskCountManager::GetInstance()->HasObserver());
    std::shared_ptr<IRunningTaskObserver> ob = std::make_shared<TestRunCountDemo>();
    auto pNewFwkOb = std::make_shared<FwkIRunningTaskObserver>(ob);
    FwkRunningTaskCountManager::GetInstance()->observers_.push_back(pNewFwkOb);
    EXPECT_TRUE(FwkRunningTaskCountManager::GetInstance()->HasObserver());
    RequestManagerImpl::SystemAbilityStatusChangeListener listener =
        RequestManagerImpl::SystemAbilityStatusChangeListener();
    listener.OnAddSystemAbility(OHOS::PRINT_SERVICE_ID, deviceId);
    FwkRunningTaskCountManager::GetInstance()->observers_.clear();
}

/**
 * @tc.name: ReopenChannel001
 * @tc.desc: Test ReopenChannel interface with channel reopening scenario
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Create ResponseMessageReceiver instance
 *           3. Set msgReceiver_ to new instance
 *           4. Expect OpenChannel to return E_CHANNEL_NOT_OPEN
 *           5. Call ReopenChannel
 * @tc.expect: ReopenChannel handles channel reopening appropriately
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ReopenChannel001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    RequestManagerImpl::GetInstance()->msgReceiver_ =
        std::make_shared<ResponseMessageReceiver>(RequestManagerImpl::GetInstance(), -1);
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_)).WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    RequestManagerImpl::GetInstance()->ReopenChannel();
}

/**
 * @tc.name: CreateGroup001
 * @tc.desc: Test CreateGroup interface with successful group creation
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Create test group ID
 *           3. Set gauge to true
 *           4. Create notification info with text, title, and wantAgent
 *           5. Expect CreateGroup to return E_OK
 *           6. Call CreateGroup with test parameters
 * @tc.expect: CreateGroup returns E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateGroup001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    std::string gid = "gid";
    bool gauge = true;
    OHOS::AbilityRuntime::WantAgent::WantAgentInfo paramsInfo;
    std::shared_ptr<OHOS::AbilityRuntime::WantAgent::WantAgent> wantAgent =
        OHOS::AbilityRuntime::WantAgent::WantAgentHelper::GetWantAgent(paramsInfo);
    Notification info{
        .text = "text",
        .title = "title",
        .wantAgent = OHOS::AbilityRuntime::WantAgent::WantAgentHelper::ToString(wantAgent),
        .disable = false,

    };
    EXPECT_CALL(*exceptProxy, CreateGroup(gid, testing::_, testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->CreateGroup(gid, gauge, info), E_OK);
}

/**
 * @tc.name: CreateGroup002
 * @tc.desc: Test CreateGroup interface with successful group creation
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Create test group ID
 *           3. Set gauge to true
 *           4. Create notification info with text, title
 *           5. Expect CreateGroup to return E_OK
 *           6. Call CreateGroup with test parameters
 * @tc.expect: CreateGroup returns E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateGroup002, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    std::string gid = "gid";
    bool gauge = true;
    Notification info{
        .text = "text",
        .title = "title",
        .disable = false,
    };
    EXPECT_CALL(*exceptProxy, CreateGroup(gid, testing::_, testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->CreateGroup(gid, gauge, info), E_OK);
}

/**
 * @tc.name: AttachGroup001
 * @tc.desc: Test AttachGroup interface with task attachment to group
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test group ID
 *           2. Create test task ID vector with multiple tasks
 *           3. Expect AttachGroup to return E_OK
 *           4. Call AttachGroup with test parameters
 *           5. Verify result equals E_OK
 * @tc.expect: AttachGroup returns E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, AttachGroup001, TestSize.Level1)
{
    std::string gid = "gid";
    std::vector<std::string> tids = { "tid", "1231" };
    EXPECT_CALL(*exceptProxy, AttachGroup(gid, testing::_)).WillOnce(testing::Return(E_OK));
    auto res = RequestManagerImpl::GetInstance()->AttachGroup(gid, tids);
    EXPECT_EQ(res, E_OK);
}

/**
 * @tc.name: DeleteGroup001
 * @tc.desc: Test DeleteGroup interface with successful group deletion
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create test group ID
 *           2. Expect DeleteGroup to return E_OK
 *           3. Call DeleteGroup with test group ID
 *           4. Verify result equals E_OK
 * @tc.expect: DeleteGroup returns E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, DeleteGroup001, TestSize.Level1)
{
    std::string gid = "gid";
    EXPECT_CALL(*exceptProxy, DeleteGroup(gid)).WillOnce(testing::Return(E_OK));
    auto res = RequestManagerImpl::GetInstance()->DeleteGroup(gid);
    EXPECT_EQ(res, E_OK);
}

/**
 * @tc.name: QueryTasks001
 * @tc.desc: Test QueryTasks interface with multiple task query
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Verify mock proxy is not null
 *           2. Create test task ID vector with multiple tasks
 *           3. Create result vector for TaskInfoRet
 *           4. Expect QueryTasks to return E_OK
 *           5. Call QueryTasks with test parameters
 * @tc.expect: QueryTasks returns E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, QueryTasks001, TestSize.Level1)
{
    EXPECT_NE(exceptProxy, nullptr);
    std::vector<std::string> tids = { "tid", "123" };
    std::vector<TaskInfoRet> rets;
    EXPECT_CALL(*exceptProxy, QueryTasks(tids, testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->QueryTasks(tids, rets), E_OK);
}

/**
 * @tc.name: CreateWithNotificationTest001
 * @tc.desc: Test Create interface with notification configuration
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create config with API9 version and notification settings
 *           2. Set notification properties: text, title, wantAgent, disable, visibility
 *           3. Expect Create to return E_OK
 *           4. Call Create method with the configured parameters
 * @tc.expect: Create returns E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateWithNotificationTest001, TestSize.Level1)
{
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    OHOS::AbilityRuntime::WantAgent::WantAgentInfo paramsInfo;
    std::shared_ptr<OHOS::AbilityRuntime::WantAgent::WantAgent> wantAgent =
        OHOS::AbilityRuntime::WantAgent::WantAgentHelper::GetWantAgent(paramsInfo);
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_))
        .WillOnce(testing::Return(E_TASK_STATE))
        .WillOnce(testing::Return(E_OK));
    Config config;
    config.version = Version::API9;

    config.notification.text = "text";
    config.notification.title = "title";
    config.notification.wantAgent = OHOS::AbilityRuntime::WantAgent::WantAgentHelper::ToString(wantAgent);
    config.notification.disable = false;
    config.notification.visibility = VISIBILITY_COMPLETION;

    int32_t seq = 1;
    std::string tid = "1";
    EXPECT_CALL(*exceptProxy, Create(testing::_, tid))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN))
        .WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, Subscribe(testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, Start(testing::_))
        .WillOnce(testing::Return(E_OK))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_OK);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_CHANNEL_NOT_OPEN);
}

/**
 * @tc.name: CreateWithNotificationTest002
 * @tc.desc: Test Create interface with notification configuration
 * @tc.precon: RequestManagerImpl instance is initialized, mock proxy is set up
 * @tc.step: 1. Create config with API9 version and notification settings
 *           2. Set notification properties: text, title, disable, visibility
 *           3. Expect Create to return E_OK
 *           4. Call Create method with the configured parameters
 * @tc.expect: Create returns E_OK successfully
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateWithNotificationTest002, TestSize.Level1)
{
    RequestManagerImpl::GetInstance()->OnChannelBroken();
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_))
        .WillOnce(testing::Return(E_TASK_STATE))
        .WillOnce(testing::Return(E_OK));
    Config config;
    config.version = Version::API9;

    config.notification.text = "text";
    config.notification.title = "title";
    config.notification.disable = false;
    config.notification.visibility = VISIBILITY_COMPLETION;

    int32_t seq = 1;
    std::string tid = "1";
    EXPECT_CALL(*exceptProxy, Create(testing::_, tid))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN))
        .WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, Subscribe(testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, Start(testing::_))
        .WillOnce(testing::Return(E_OK))
        .WillOnce(testing::Return(E_CHANNEL_NOT_OPEN));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_OK);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_CHANNEL_NOT_OPEN);
}

class RMITestResponseListener : public IResponseListener {
public:
    void OnResponseReceive(const std::shared_ptr<Response> &response) override
    {
        responseCount_++;
    }
    int responseCount_ = 0;
};

class RMITestNotifyListener : public INotifyDataListener {
public:
    void OnNotifyDataReceive(const std::shared_ptr<NotifyData> &notifyData) override
    {
        notifyCount_++;
    }
    void OnFaultsReceive(const std::shared_ptr<int32_t> &tid, const std::shared_ptr<SubscribeType> &type,
        const std::shared_ptr<Reason> &reason) override
    {
        faultsCount_++;
    }
    void OnWaitReceive(std::int32_t taskId, WaitingReason reason) override
    {
        waitCount_++;
    }
    int notifyCount_ = 0;
    int faultsCount_ = 0;
    int waitCount_ = 0;
};

/**
 * @tc.name: CreateTasksEmptyTest001
 * @tc.desc: CreateTasks returns E_OK immediately for an empty config list without
 *           touching the proxy or the output vector
 * @tc.precon: NA
 * @tc.step: 1. Prepare an empty configs vector and an empty rets vector
 *           2. Call CreateTasks with the empty list
 * @tc.expect: Returns E_OK and rets stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateTasksEmptyTest001, TestSize.Level1)
{
    std::vector<Config> configs;
    std::vector<TaskRet> outRets;
    configs.reserve(1);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->CreateTasks(configs, outRets), ExceptionErrorCode::E_OK);
    EXPECT_TRUE(outRets.empty());
    EXPECT_EQ(outRets.size(), 0u);
}

/**
 * @tc.name: CreateTasksChannelReopenTest001
 * @tc.desc: CreateTasks drives the E_CHANNEL_NOT_OPEN recovery loop: the first
 *           failed task triggers ReopenChannel, every failed task triggers
 *           Subscribe, and the subscribe result is written to a per-task copy
 * @tc.precon: NA
 * @tc.step: 1. Mock CreateTasks to return three TaskRets: two channel errors and one OK
 *           2. Mock OpenChannel and Subscribe to return E_OK
 *           3. Call CreateTasks and inspect the return value and rets
 * @tc.expect: Returns E_OK, rets round-trips intact with original codes, Subscribe runs twice
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateTasksChannelReopenTest001, TestSize.Level1)
{
    std::vector<Config> configs;
    configs.push_back(Config());
    std::vector<TaskRet> rets;
    rets.push_back(TaskRet{ ExceptionErrorCode::E_CHANNEL_NOT_OPEN, "t1" });
    rets.push_back(TaskRet{ ExceptionErrorCode::E_CHANNEL_NOT_OPEN, "t2" });
    rets.push_back(TaskRet{ ExceptionErrorCode::E_OK, "t3" });
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, CreateTasks(testing::_, testing::_))
        .WillOnce(testing::DoAll(testing::SetArgReferee<1>(rets), testing::Return(ExceptionErrorCode::E_OK)));
    EXPECT_CALL(*exceptProxy, Subscribe(testing::_)).Times(2).WillRepeatedly(testing::Return(E_OK));
    std::vector<TaskRet> outRets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->CreateTasks(configs, outRets), ExceptionErrorCode::E_OK);
    ASSERT_EQ(outRets.size(), 3u);
    EXPECT_EQ(outRets[0].tid, "t1");
    EXPECT_EQ(outRets[0].code, ExceptionErrorCode::E_CHANNEL_NOT_OPEN);
    EXPECT_EQ(outRets[1].tid, "t2");
    EXPECT_EQ(outRets[2].tid, "t3");
    EXPECT_EQ(outRets[2].code, ExceptionErrorCode::E_OK);
}

/**
 * @tc.name: CreateTasksFailTest001
 * @tc.desc: CreateTasks forwards a non-E_OK proxy failure code to the caller and
 *           leaves the output vector untouched; E_SERVICE_ERROR is reserved by
 *           CallProxyMethod as the remote-died retry code, so E_PERMISSION is used
 * @tc.precon: NA
 * @tc.step: 1. Mock CreateTasks to return E_PERMISSION
 *           2. Call CreateTasks with one config
 * @tc.expect: Returns E_PERMISSION and rets stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateTasksFailTest001, TestSize.Level1)
{
    std::vector<Config> configs;
    configs.push_back(Config());
    EXPECT_CALL(*exceptProxy, OpenChannel(testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_CALL(*exceptProxy, CreateTasks(testing::_, testing::_))
        .WillOnce(testing::Return(ExceptionErrorCode::E_PERMISSION));
    std::vector<TaskRet> outRets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->CreateTasks(configs, outRets), ExceptionErrorCode::E_PERMISSION);
    EXPECT_TRUE(outRets.empty());
    EXPECT_EQ(outRets.size(), 0u);
}

/**
 * @tc.name: CreateFileCloseTest001
 * @tc.desc: Create closes the fd of user files after a successful create, skips
 *           non-user files and keeps fds <= 0 untouched
 * @tc.precon: NA
 * @tc.step: 1. Prepare a config with one user file holding a live socketpair fd,
 *              one non-user file and one user file with fd -1
 *           2. Mock Create to return E_OK and call Create with API10
 *           3. Probe the socketpair fd after the call
 * @tc.expect: Returns E_OK; the live fd was closed so writes fail, other fds ignored
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, CreateFileCloseTest001, TestSize.Level1)
{
    int fds[2];
    ASSERT_GE(socketpair(AF_UNIX, SOCK_STREAM, 0, fds), 0);
    FileSpec userFile;
    userFile.isUserFile = true;
    userFile.fd = fds[0];
    FileSpec notUserFile;
    notUserFile.isUserFile = false;
    notUserFile.fd = 99;
    FileSpec invalidFdFile;
    invalidFdFile.isUserFile = true;
    invalidFdFile.fd = -1;
    Config config;
    config.version = Version::API10;
    config.files.push_back(userFile);
    config.files.push_back(notUserFile);
    config.files.push_back(invalidFdFile);
    int32_t seq = 1;
    std::string tid;
    EXPECT_CALL(*exceptProxy, Create(testing::_, testing::_)).WillOnce(testing::Return(E_OK));
    EXPECT_EQ(RequestManagerImpl::GetInstance()->Create(config, seq, tid), E_OK);
    char probe = 0;
    EXPECT_EQ(write(fds[0], &probe, 1), -1);
    EXPECT_EQ(close(fds[1]), 0);
}

/**
 * @tc.name: AddRemoveListenerAllTest001
 * @tc.desc: AddListener and RemoveListener find or create the task and succeed
 *           for both the response and notify-data listener overloads
 * @tc.precon: NA
 * @tc.step: 1. Add a response listener and a notify listener on the same task id
 *           2. Verify the task exists and has listeners
 *           3. Remove both listeners and verify no listener remains
 * @tc.expect: All four calls return E_OK; the task has no listener after removal
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, AddRemoveListenerAllTest001, TestSize.Level1)
{
    string tid = "arl";
    auto respListener = std::make_shared<RMITestResponseListener>();
    auto notifyListener = std::make_shared<RMITestNotifyListener>();
    auto mgr = RequestManagerImpl::GetInstance();
    EXPECT_EQ(mgr->AddListener(tid, SubscribeType::RESPONSE, respListener), E_OK);
    EXPECT_EQ(mgr->AddListener(tid, SubscribeType::COMPLETED, notifyListener), E_OK);
    EXPECT_NE(mgr->GetTask(tid).get(), nullptr);
    EXPECT_EQ(mgr->GetTask(tid)->HasListener(), true);
    EXPECT_EQ(mgr->RemoveListener(tid, SubscribeType::RESPONSE, respListener), E_OK);
    EXPECT_EQ(mgr->RemoveListener(tid, SubscribeType::COMPLETED, notifyListener), E_OK);
    EXPECT_EQ(mgr->GetTask(tid)->HasListener(), false);
}

/**
 * @tc.name: OnFaultsWaitReceiveForwardTest001
 * @tc.desc: OnFaultsReceive and OnWaitReceive forward to the listener registered
 *           on the matching task, and skip unknown subscribe types silently
 * @tc.precon: NA
 * @tc.step: 1. Register a notify listener under COMPLETED and WAIT on task "5"
 *           2. Call OnFaultsReceive with the COMPLETED type
 *           3. Call OnFaultsReceive with an unregistered type
 *           4. Call OnWaitReceive after registering the WAIT listener
 * @tc.expect: The registered callbacks fire exactly once each; the unknown type does not fire
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, OnFaultsWaitReceiveForwardTest001, TestSize.Level1)
{
    string tid = "5";
    auto notifyListener = std::make_shared<RMITestNotifyListener>();
    auto mgr = RequestManagerImpl::GetInstance();
    EXPECT_EQ(mgr->AddListener(tid, SubscribeType::COMPLETED, notifyListener), E_OK);
    EXPECT_EQ(mgr->AddListener(tid, SubscribeType::WAIT, notifyListener), E_OK);
    auto tidPtr = std::make_shared<int32_t>(5);
    auto typePtr = std::make_shared<SubscribeType>(SubscribeType::COMPLETED);
    auto reasonPtr = std::make_shared<Reason>();
    mgr->OnFaultsReceive(tidPtr, typePtr, reasonPtr);
    EXPECT_EQ(notifyListener->faultsCount_, 1);
    auto otherType = std::make_shared<SubscribeType>(SubscribeType::REMOVE);
    mgr->OnFaultsReceive(tidPtr, otherType, reasonPtr);
    EXPECT_EQ(notifyListener->faultsCount_, 1);
    mgr->OnWaitReceive(5, WaitingReason::TaskQueueFull);
    EXPECT_EQ(notifyListener->waitCount_, 1);
    EXPECT_EQ(notifyListener->notifyCount_, 0);
}

/**
 * @tc.name: OnRemoveReceiverShutdownTest001
 * @tc.desc: OnRemoveSystemAbility resets the proxy, run count and SA status, and
 *           shutting the receiver down cascades OnChannelBroken which releases it
 * @tc.precon: NA
 * @tc.step: 1. Attach a message receiver with fd -1 to the manager
 *           2. Dispatch OnRemoveSystemAbility with DOWNLOAD_SERVICE_ID
 * @tc.expect: Proxy is null, count is zero, SA offline, the receiver is reset to null
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, OnRemoveReceiverShutdownTest001, TestSize.Level1)
{
    auto mgr = RequestManagerImpl::GetInstance();
    mgr->msgReceiver_ = std::make_shared<ResponseMessageReceiver>(mgr, -1);
    RequestManagerImpl::SystemAbilityStatusChangeListener listener;
    listener.OnRemoveSystemAbility(OHOS::DOWNLOAD_SERVICE_ID, "deviceId");
    EXPECT_EQ(mgr->requestServiceProxy_, nullptr);
    EXPECT_EQ(FwkRunningTaskCountManager::GetInstance()->GetCount(), 0);
    EXPECT_FALSE(FwkRunningTaskCountManager::GetInstance()->SaIsOnline());
    EXPECT_EQ(mgr->msgReceiver_, nullptr);
}

/**
 * @tc.name: SubscribeUnsubscribeSATest001
 * @tc.desc: SubscribeSA early-returns true when a listener is already registered
 *           and UnsubscribeSA early-returns true when none is, both without any
 *           samgr interaction
 * @tc.precon: NA
 * @tc.step: 1. Register a fresh listener object and call SubscribeSA
 *           2. Verify the early-return keeps the listener pointer set
 *           3. Clear the listener pointer and call UnsubscribeSA
 * @tc.expect: Both calls return true without touching samgr; the pointer state is preserved
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, SubscribeUnsubscribeSATest001, TestSize.Level1)
{
    auto mgr = RequestManagerImpl::GetInstance();
    mgr->saChangeListener_ = new (std::nothrow) RequestManagerImpl::SystemAbilityStatusChangeListener();
    EXPECT_NE(mgr->saChangeListener_, nullptr);
    EXPECT_TRUE(mgr->SubscribeSA());
    EXPECT_NE(mgr->saChangeListener_, nullptr);
    mgr->saChangeListener_ = nullptr;
    EXPECT_TRUE(mgr->UnsubscribeSA());
    EXPECT_EQ(mgr->saChangeListener_, nullptr);
}

namespace {
// Batch error fed to every mock; it differs from E_SERVICE_ERROR so the
// proxy loop breaks on the first answer and never reaches the retry arm,
// which would reload the real service proxy from samgr.
constexpr ExceptionErrorCode BATCH_ERR = ExceptionErrorCode::E_TASK_NOT_FOUND;
constexpr int64_t MOCK_SPEED_LIMIT = 1024;
constexpr Mode MOCK_MODE = Mode::FOREGROUND;
} // namespace

/**
 * @tc.name: ImplSetModeBatchErr001
 * @tc.desc: SetMode forwards the mode through the cached mock proxy and
 *           returns the proxy answer unchanged; the mock bookkeeping resets
 *           cleanly afterwards
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering SetMode with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::SetMode with the mock in place
 *           3. Verify and clear the mock expectations before teardown
 * @tc.expect: The returned code equals the mock answer and the mock reports
 *             no unmet expectations
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplSetModeBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, SetMode(testing::_, testing::_)).WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->SetMode("tid_mode", MOCK_MODE), BATCH_ERR);
    EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(exceptProxy.GetRefPtr()));
}

/**
 * @tc.name: ImplDisableTaskNotificationBatchErr001
 * @tc.desc: DisableTaskNotification returns the batch error directly when the
 *           service call fails, leaving the result vector untouched
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering DisableTaskNotification with
 *              E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::DisableTaskNotification
 * @tc.expect: The returned code equals the mock answer and the result vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplDisableTaskNotificationBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, DisableTaskNotification(testing::_, testing::_))
        .WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_disable" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->DisableTaskNotification(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplStartTasksBatchErr001
 * @tc.desc: StartTasks returns the batch error directly when the service call
 *           fails, skipping the per-tid result mapping
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering StartTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::StartTasks
 * @tc.expect: The returned code equals the mock answer and the result vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplStartTasksBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, StartTasks(testing::_, testing::_)).WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_start" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->StartTasks(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplStopTasksBatchErr001
 * @tc.desc: StopTasks returns the batch error directly when the service call
 *           fails
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering StopTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::StopTasks
 * @tc.expect: The returned code equals the mock answer and the result vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplStopTasksBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, StopTasks(testing::_, testing::_)).WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_stop" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->StopTasks(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplResumeTasksBatchErr001
 * @tc.desc: ResumeTasks returns the batch error directly when the service
 *           call fails
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering ResumeTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::ResumeTasks
 * @tc.expect: The returned code equals the mock answer and the result vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplResumeTasksBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, ResumeTasks(testing::_, testing::_)).WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_resume" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->ResumeTasks(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplRemoveTasksBatchErr001
 * @tc.desc: RemoveTasks returns the batch error directly when the service
 *           call fails, for both API9 and API10 versions
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering RemoveTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::RemoveTasks with Version::API9 and
 *              again with Version::API10
 * @tc.expect: Both calls return the mock answer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplRemoveTasksBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, RemoveTasks(testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_remove" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->RemoveTasks(tids, Version::API9, rets), BATCH_ERR);
    EXPECT_EQ(RequestManagerImpl::GetInstance()->RemoveTasks(tids, Version::API10, rets), BATCH_ERR);
}

/**
 * @tc.name: ImplPauseTasksBatchErr001
 * @tc.desc: PauseTasks returns the batch error directly when the service call
 *           fails
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering PauseTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::PauseTasks
 * @tc.expect: The returned code equals the mock answer and the result vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplPauseTasksBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, PauseTasks(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_pause" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->PauseTasks(tids, Version::API10, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplShowTasksBatchErr001
 * @tc.desc: ShowTasks returns the batch error directly when the service call
 *           fails, without filling task infos
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering ShowTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::ShowTasks
 * @tc.expect: The returned code equals the mock answer and the info vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplShowTasksBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, ShowTasks(testing::_, testing::_)).WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_show" };
    std::vector<TaskInfoRet> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->ShowTasks(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplTouchTasksBatchErr001
 * @tc.desc: TouchTasks returns the batch error directly when the service call
 *           fails, without filling task infos
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering TouchTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::TouchTasks with one tid-token pair
 * @tc.expect: The returned code equals the mock answer and the info vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplTouchTasksBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, TouchTasks(testing::_, testing::_)).WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<TaskIdAndToken> tidTokens = { { "tid_touch", "token_touch" } };
    std::vector<TaskInfoRet> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->TouchTasks(tidTokens, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplSetMaxSpeedsBatchErr001
 * @tc.desc: SetMaxSpeeds returns the batch error directly when the service
 *           call fails, without filling per-tid results
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering SetMaxSpeeds with
 *              E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::SetMaxSpeeds with one speed config
 * @tc.expect: The returned code equals the mock answer and the result vector
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplSetMaxSpeedsBatchErr001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, SetMaxSpeeds(testing::_, testing::_)).WillOnce(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<SpeedConfig> speedConfig = { { "tid_speed", MOCK_SPEED_LIMIT } };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->SetMaxSpeeds(speedConfig, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ImplSetMaxSpeedPassthrough001
 * @tc.desc: SetMaxSpeed forwards the limit through the cached mock proxy
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering SetMaxSpeed with E_TASK_NOT_FOUND
 *           2. Call RequestManagerImpl::SetMaxSpeed
 * @tc.expect: The returned code equals the mock answer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ImplSetMaxSpeedPassthrough001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, SetMaxSpeed(testing::_, testing::_))
        .WillOnce(testing::Return(static_cast<int32_t>(BATCH_ERR)));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    EXPECT_EQ(RequestManagerImpl::GetInstance()->SetMaxSpeed("tid_speed_one", MOCK_SPEED_LIMIT),
        static_cast<int32_t>(BATCH_ERR));
}

/**
 * @tc.name: ProxyConstructNullRemote001
 * @tc.desc: constructing a RequestServiceProxy over a null remote object
 *           succeeds and AsObject reports the same null object back; a
 *           second instance behaves the same way
 * @tc.precon: NA
 * @tc.step: 1. Construct RequestServiceProxy with a null IRemoteObject sptr
 *           2. Call AsObject on the proxy
 *           3. Construct a second proxy over the same null remote and call
 *              AsObject on it
 * @tc.expect: Both constructions complete and both AsObject calls return
 *             null
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ProxyConstructNullRemote001, TestSize.Level1)
{
    OHOS::sptr<OHOS::IRemoteObject> nullRemote;
    RequestServiceProxy proxy(nullRemote);
    EXPECT_EQ(proxy.AsObject(), nullptr);

    RequestServiceProxy secondProxy(nullRemote);
    EXPECT_EQ(secondProxy.AsObject(), nullptr);
}

/**
 * @tc.name: ManagerBatchStartStop001
 * @tc.desc: the RequestManager thin wrappers for StartTasks and StopTasks
 *           forward to the impl and report the proxy answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering StartTasks and StopTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call RequestManager::StartTasks and RequestManager::StopTasks
 * @tc.expect: Both wrappers return the mock answer with empty result vectors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ManagerBatchStartStop001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, StartTasks(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    EXPECT_CALL(*exceptProxy, StopTasks(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_mgr_ss" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManager::GetInstance()->StartTasks(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
    EXPECT_EQ(RequestManager::GetInstance()->StopTasks(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ManagerBatchResumePause001
 * @tc.desc: the RequestManager thin wrappers for ResumeTasks and PauseTasks
 *           forward to the impl and report the proxy answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering ResumeTasks and PauseTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call RequestManager::ResumeTasks and RequestManager::PauseTasks
 * @tc.expect: Both wrappers return the mock answer with empty result vectors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ManagerBatchResumePause001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, ResumeTasks(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    EXPECT_CALL(*exceptProxy, PauseTasks(testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_mgr_rp" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManager::GetInstance()->ResumeTasks(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
    EXPECT_EQ(RequestManager::GetInstance()->PauseTasks(tids, Version::API10, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
}

/**
 * @tc.name: ManagerBatchShowTouch001
 * @tc.desc: the RequestManager thin wrappers for ShowTasks and TouchTasks
 *           forward to the impl and report the proxy answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering ShowTasks and TouchTasks with
 *              E_TASK_NOT_FOUND
 *           2. Call RequestManager::ShowTasks and RequestManager::TouchTasks
 * @tc.expect: Both wrappers return the mock answer with empty info vectors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ManagerBatchShowTouch001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, ShowTasks(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    EXPECT_CALL(*exceptProxy, TouchTasks(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_mgr_st" };
    std::vector<TaskIdAndToken> tidTokens = { { "tid_mgr_touch", "token_mgr" } };
    std::vector<TaskInfoRet> infos;
    EXPECT_EQ(RequestManager::GetInstance()->ShowTasks(tids, infos), BATCH_ERR);
    EXPECT_TRUE(infos.empty());
    EXPECT_EQ(RequestManager::GetInstance()->TouchTasks(tidTokens, infos), BATCH_ERR);
    EXPECT_TRUE(infos.empty());
}

/**
 * @tc.name: ManagerRemoveSetMode001
 * @tc.desc: the RequestManager thin wrappers for RemoveTasks and SetMode
 *           forward to the impl and report the proxy answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering RemoveTasks and SetMode with
 *              E_TASK_NOT_FOUND
 *           2. Call RequestManager::RemoveTasks for both versions and
 *              RequestManager::SetMode
 * @tc.expect: Every wrapper returns the mock answer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ManagerRemoveSetMode001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, RemoveTasks(testing::_, testing::_, testing::_))
        .WillRepeatedly(testing::Return(BATCH_ERR));
    EXPECT_CALL(*exceptProxy, SetMode(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_mgr_rm" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManager::GetInstance()->RemoveTasks(tids, Version::API9, rets), BATCH_ERR);
    EXPECT_EQ(RequestManager::GetInstance()->RemoveTasks(tids, Version::API10, rets), BATCH_ERR);
    EXPECT_EQ(RequestManager::GetInstance()->SetMode("tid_mgr_mode", MOCK_MODE), BATCH_ERR);
}

/**
 * @tc.name: ManagerSpeedFamily001
 * @tc.desc: the RequestManager thin wrappers for SetMaxSpeeds and SetMaxSpeed
 *           forward to the impl and report the proxy answer unchanged
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering SetMaxSpeeds and SetMaxSpeed with
 *              the batch error
 *           2. Call RequestManager::SetMaxSpeeds and RequestManager::SetMaxSpeed
 * @tc.expect: Both wrappers return the mock answer with empty result vectors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ManagerSpeedFamily001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, SetMaxSpeeds(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    EXPECT_CALL(*exceptProxy, SetMaxSpeed(testing::_, testing::_))
        .WillRepeatedly(testing::Return(static_cast<int32_t>(BATCH_ERR)));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<SpeedConfig> speedConfig = { { "tid_mgr_speed", MOCK_SPEED_LIMIT } };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManager::GetInstance()->SetMaxSpeeds(speedConfig, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
    EXPECT_EQ(RequestManager::GetInstance()->SetMaxSpeed("tid_mgr_speed_one", MOCK_SPEED_LIMIT),
        static_cast<int32_t>(BATCH_ERR));
}

/**
 * @tc.name: ManagerDisableCreateTasks001
 * @tc.desc: the RequestManager thin wrappers for DisableTaskNotification and
 *           CreateTasks forward to the impl and return the batch error
 *           without contacting the service a second time
 * @tc.precon: NA
 * @tc.step: 1. Cache a mock proxy answering DisableTaskNotification and
 *              CreateTasks with E_TASK_NOT_FOUND
 *           2. Call RequestManager::DisableTaskNotification and
 *              RequestManager::CreateTasks
 * @tc.expect: Both wrappers return the mock answer with empty result vectors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestManagerImplTest, ManagerDisableCreateTasks001, TestSize.Level1)
{
    exceptProxy = OHOS::sptr<MockRequestServiceInterface>(new MockRequestServiceInterface());
    EXPECT_CALL(*exceptProxy, DisableTaskNotification(testing::_, testing::_))
        .WillRepeatedly(testing::Return(BATCH_ERR));
    EXPECT_CALL(*exceptProxy, CreateTasks(testing::_, testing::_)).WillRepeatedly(testing::Return(BATCH_ERR));
    RequestManagerImpl::GetInstance()->requestServiceProxy_ = exceptProxy;
    std::vector<std::string> tids = { "tid_mgr_dis" };
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(RequestManager::GetInstance()->DisableTaskNotification(tids, rets), BATCH_ERR);
    EXPECT_TRUE(rets.empty());
    std::vector<Config> configs(1);
    std::vector<TaskRet> taskRets;
    EXPECT_EQ(RequestManager::GetInstance()->CreateTasks(configs, taskRets), BATCH_ERR);
    EXPECT_TRUE(taskRets.empty());
}