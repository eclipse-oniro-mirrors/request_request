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

#include "gtest/hwext/gtest-ext.h"
#define private public
#define protected public
#include <gtest/gtest.h>

#include <memory>

#include "gmock/gmock.h"
#include "log.h"
#include "request.h"
#include "request_common.h"

using namespace testing::ext;
using namespace OHOS::Request;

#undef private
#undef protected

int g_requestTest = 0;

class RequestTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RequestTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void RequestTest::SetUp(void)
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

void RequestTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
}

HWTEST_F(RequestTest, GetIdTest001, TestSize.Level1)
{
    string tid = "testTid";
    Request request = Request(tid);
    EXPECT_EQ(request.getId(), tid);
}

class RTResponseListenerImpl : public IResponseListener {
public:
    ~RTResponseListenerImpl(){};
    void OnResponseReceive(const std::shared_ptr<Response> &response) override
    {
        (void)response;
        g_requestTest = 2; // 2 is except result
        return;
    }
};

/**
 * @tc.name: AddAndRemoveListenerTest001
 * @tc.desc: Test AddAndRemoveListenerTest001 interface base function - AddListener/RemoveListener
 * @tc.type: FUNC
 * @tc.require: Issue Number
 */
HWTEST_F(RequestTest, AddAndRemoveListenerTest001, TestSize.Level1)
{
    string tid = "testTid";
    SubscribeType type = SubscribeType::RESPONSE;
    Request request = Request(tid);
    std::shared_ptr<RTResponseListenerImpl> listenerPtr = std::make_shared<RTResponseListenerImpl>();
    request.AddListener(type, listenerPtr);
    EXPECT_EQ(request.HasListener(), true);
    request.RemoveListener(type, listenerPtr);
    EXPECT_EQ(request.HasListener(), false);
    type = SubscribeType::FAILED;
    request.AddListener(type, listenerPtr);
    EXPECT_EQ(request.HasListener(), false);
    request.RemoveListener(type, listenerPtr);
}

class RTNotifyDataListenerImpl : public INotifyDataListener {
public:
    ~RTNotifyDataListenerImpl(){};
    void OnNotifyDataReceive(const std::shared_ptr<NotifyData> &notifyData) override
    {
        (void)notifyData;
        g_requestTest = 1;
        return;
    }
    void OnFaultsReceive(const std::shared_ptr<int32_t> &tid, const std::shared_ptr<SubscribeType> &type,
        const std::shared_ptr<Reason> &reason) override
    {
    }
    void OnWaitReceive(std::int32_t taskId, WaitingReason reason) override
    {
    }
};

/**
 * @tc.name: OnNotifyDataReceiveTest001
 * @tc.desc: Test OnNotifyDataReceiveTest001 interface base function - OnNotifyDataReceive
 * @tc.type: FUNC
 * @tc.require: Issue Number
 */
HWTEST_F(RequestTest, OnNotifyDataReceiveTest001, TestSize.Level1)
{
    g_requestTest = 0;
    string tid = "testTid";
    SubscribeType type = SubscribeType::COMPLETED;
    Request request = Request(tid);
    std::shared_ptr<NotifyData> notifyData = std::make_shared<NotifyData>();
    notifyData->type = type;
    notifyData->version = Version::API9;
    request.OnNotifyDataReceive(notifyData);
    EXPECT_EQ(g_requestTest, 0);
    std::shared_ptr<RTNotifyDataListenerImpl> listenerPtr = std::make_shared<RTNotifyDataListenerImpl>();
    request.AddListener(type, listenerPtr);
    EXPECT_EQ(request.HasListener(), true);
    request.OnNotifyDataReceive(notifyData);
    notifyData->type = SubscribeType::RESPONSE;
    notifyData->version = Version::API10;
    request.OnNotifyDataReceive(notifyData);
    EXPECT_EQ(g_requestTest, 1);
    g_requestTest = 0;
    notifyData->type = SubscribeType::REMOVE;
    notifyData->version = Version::API9;
    request.needRemove_ = false;
    request.AddListener(SubscribeType::REMOVE, listenerPtr);
    request.OnNotifyDataReceive(notifyData);
    EXPECT_EQ(g_requestTest, 0);
}

/**
 * @tc.name: NeedNotifyTest001
 * @tc.desc: Test NeedNotifyTest001 interface base function - NeedNotify
 * @tc.type: FUNC
 * @tc.require: Issue Number
 */
HWTEST_F(RequestTest, NeedNotifyTest001, TestSize.Level1)
{
    string tid = "testTid";
    Request request = Request(tid);
    std::shared_ptr<NotifyData> notifyData = std::make_shared<NotifyData>();
    notifyData->type = SubscribeType::COMPLETED;
    notifyData->version = Version::API10;
    request.needRemove_ = true;
    EXPECT_EQ(request.NeedNotify(notifyData), true);
    notifyData->type = SubscribeType::REMOVE;
    notifyData->version = Version::API9;
    request.needRemove_ = true;
    EXPECT_EQ(request.NeedNotify(notifyData), true);
    notifyData->type = SubscribeType::COMPLETED;
    notifyData->version = Version::API10;
    EXPECT_EQ(request.NeedNotify(notifyData), true);
    notifyData->type = SubscribeType::FAILED;
    notifyData->version = Version::API10;
    EXPECT_EQ(request.NeedNotify(notifyData), true);
    notifyData->type = SubscribeType::HEADER_RECEIVE;
    notifyData->version = Version::API9;
    EXPECT_EQ(request.NeedNotify(notifyData), true);
    notifyData->type = SubscribeType::REMOVE;
    notifyData->version = Version::API9;
    request.needRemove_ = false;
    EXPECT_EQ(request.NeedNotify(notifyData), false);
}

/**
 * @tc.name: AddAndRemoveListenerTest002
 * @tc.desc: Test AddAndRemoveListenerTest002 interface base function - AddListener/RemoveListener
 * @tc.type: FUNC
 * @tc.require: Issue Number
 */
HWTEST_F(RequestTest, AddAndRemoveListenerTest002, TestSize.Level1)
{
    g_requestTest = 0;
    string tid = "testTid";
    SubscribeType type = SubscribeType::COMPLETED;
    Request request = Request(tid);
    std::shared_ptr<NotifyData> notifyData = std::make_shared<NotifyData>();
    notifyData->type = type;
    notifyData->version = Version::API9;
    request.OnNotifyDataReceive(notifyData);
    std::shared_ptr<RTNotifyDataListenerImpl> listenerPtr = std::make_shared<RTNotifyDataListenerImpl>();
    request.AddListener(SubscribeType::BUTT, listenerPtr);
    request.AddListener(type, listenerPtr);
    EXPECT_EQ(request.HasListener(), true);
    EXPECT_EQ(g_requestTest, 1);
    request.RemoveListener(SubscribeType::RESPONSE, listenerPtr);
    request.RemoveListener(SubscribeType::BUTT, listenerPtr);
    request.RemoveListener(type, listenerPtr);
    EXPECT_EQ(request.HasListener(), false);
}

/**
 * @tc.name: OnResponseReceiveTest001
 * @tc.desc: Test OnResponseReceiveTest001 interface base function - OnResponseReceive
 * @tc.type: FUNC
 * @tc.require: Issue Number
 */
HWTEST_F(RequestTest, OnResponseReceiveTest001, TestSize.Level1)
{
    g_requestTest = 0;
    string tid = "testTid";
    SubscribeType type = SubscribeType::RESPONSE;
    std::shared_ptr<Response> response = std::make_shared<Response>();
    Request request = Request(tid);
    request.OnResponseReceive(response);
    EXPECT_EQ(g_requestTest, 0);
    std::shared_ptr<RTResponseListenerImpl> listenerPtr = std::make_shared<RTResponseListenerImpl>();
    request.AddListener(type, listenerPtr);
    EXPECT_EQ(request.HasListener(), true);
    request.OnResponseReceive(response);
    EXPECT_EQ(g_requestTest, 2); // 2 is except result
}

/**
 * @tc.name: AddListenerTest001
 * @tc.desc: Test AddListenerTest001 interface base function - AddListener
 * @tc.type: FUNC
 * @tc.require: Issue Number
 */
HWTEST_F(RequestTest, AddListenerTest002, TestSize.Level1)
{
    g_requestTest = 0;
    string tid = "testTid";
    Request request = Request(tid);
    std::shared_ptr<RTNotifyDataListenerImpl> listenerPtr = std::make_shared<RTNotifyDataListenerImpl>();
    request.AddListener(SubscribeType::RESPONSE, listenerPtr);
    request.AddListener(SubscribeType::BUTT, listenerPtr);
    std::shared_ptr<NotifyData> notifyData = std::make_shared<NotifyData>();
    notifyData->type = SubscribeType::HEADER_RECEIVE;
    notifyData->version = Version::API9;
    request.needRemove_ = true;
    request.unusedNotifyData_[SubscribeType::HEADER_RECEIVE] = notifyData;
    request.AddListener(SubscribeType::HEADER_RECEIVE, listenerPtr);
    request.needRemove_ = false;
    request.unusedNotifyData_[SubscribeType::HEADER_RECEIVE] = notifyData;
    request.AddListener(SubscribeType::HEADER_RECEIVE, listenerPtr);
    EXPECT_EQ(g_requestTest, 1);
}