/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "log.h"
#include "request.h"
#include "request_common.h"

using namespace testing::ext;
using namespace OHOS::Request;

namespace {
// Pinned payloads the fake listeners record on delivery.
constexpr int32_t RESPONSE_STATUS_VALUE = 206;
constexpr int32_t WAIT_TASK_ID_VALUE = 5;
constexpr uint32_t NOTIFY_TASK_ID_VALUE = 9;

// Counts deliveries so each case asserts forwarding happened exactly when
// the listener registry predicts it.
class CountResponseListener : public IResponseListener {
public:
    int calls = 0;
    int32_t lastStatus = 0;

    void OnResponseReceive(const std::shared_ptr<Response> &response) override
    {
        calls++;
        if (response != nullptr) {
            lastStatus = response->statusCode;
        }
    }
};

class CountNotifyListener : public INotifyDataListener {
public:
    int calls = 0;
    int faultCalls = 0;
    int waitCalls = 0;
    Reason lastFaultReason = Reason::REASON_OK;
    WaitingReason lastWaitReason = WaitingReason::TaskQueueFull;

    void OnNotifyDataReceive(const std::shared_ptr<NotifyData> &notifyData) override
    {
        calls++;
    }

    void OnFaultsReceive(const std::shared_ptr<int32_t> &tid, const std::shared_ptr<SubscribeType> &type,
        const std::shared_ptr<Reason> &reason) override
    {
        faultCalls++;
        if (reason != nullptr) {
            lastFaultReason = *reason;
        }
    }

    void OnWaitReceive(std::int32_t taskId, WaitingReason reason) override
    {
        waitCalls++;
        lastWaitReason = reason;
    }
};

std::shared_ptr<NotifyData> MakeNotifyData(SubscribeType type, Version version)
{
    auto notifyData = std::make_shared<NotifyData>();
    notifyData->type = type;
    notifyData->taskId = NOTIFY_TASK_ID_VALUE;
    notifyData->version = version;
    notifyData->action = Action::DOWNLOAD;
    notifyData->mode = Mode::BACKGROUND;
    return notifyData;
}
} // namespace

class RequestListenerTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestListenerTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void RequestListenerTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void RequestListenerTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcase
}

void RequestListenerTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testcase
}

/**
 * @tc.name: RequestGetId001
 * @tc.desc: the task id given at construction is returned by getId
 *           consistently across repeated calls and independent instances
 * @tc.precon: NA
 * @tc.step: 1. Create a Request with a known task id
 *           2. Call getId twice on the same instance
 *           3. Create a second Request with a different task id and call
 *              getId on it
 * @tc.expect: Repeated calls return the construction id and the second
 *             instance reports its own id
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, RequestGetId001, TestSize.Level1)
{
    const std::string taskId = "tid-listener-001";
    Request request(taskId);
    EXPECT_EQ(request.getId(), taskId);
    EXPECT_EQ(request.getId(), taskId);

    const std::string otherTaskId = "tid-listener-002";
    Request other(otherTaskId);
    EXPECT_EQ(other.getId(), otherTaskId);
}

/**
 * @tc.name: ResponseListenerForward001
 * @tc.desc: response events reach the registered response listener and stop
 *           after it is removed
 * @tc.precon: NA
 * @tc.step: 1. Register a counting response listener
 *           2. Deliver a response and check the listener recorded it
 *           3. Remove the listener and deliver again
 * @tc.expect: The first delivery forwards with the pinned status code, the
 *             second delivery is dropped and HasListener flips accordingly
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, ResponseListenerForward001, TestSize.Level1)
{
    Request request("tid-response-001");
    auto listener = std::make_shared<CountResponseListener>();

    EXPECT_FALSE(request.HasListener());
    request.AddListener(SubscribeType::RESPONSE, listener);
    EXPECT_TRUE(request.HasListener());

    auto response = std::make_shared<Response>();
    response->statusCode = RESPONSE_STATUS_VALUE;
    request.OnResponseReceive(response);
    ASSERT_EQ(listener->calls, 1);
    EXPECT_EQ(listener->lastStatus, RESPONSE_STATUS_VALUE);

    request.RemoveListener(SubscribeType::RESPONSE, listener);
    EXPECT_FALSE(request.HasListener());
    request.OnResponseReceive(response);
    EXPECT_EQ(listener->calls, 1);
}

/**
 * @tc.name: NotifyListenerForward002
 * @tc.desc: a notify listener registered for one type receives only events
 *           of that type and stops after removal
 * @tc.precon: NA
 * @tc.step: 1. Register a counting listener for PROGRESS
 *           2. Deliver a PROGRESS event of API10 and a COMPLETED event
 *           3. Remove the listener and deliver PROGRESS again
 * @tc.expect: Only the PROGRESS delivery is forwarded; HasListener follows
 *             the registry state
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, NotifyListenerForward002, TestSize.Level1)
{
    Request request("tid-notify-001");
    auto listener = std::make_shared<CountNotifyListener>();

    request.AddListener(SubscribeType::PROGRESS, listener);
    EXPECT_TRUE(request.HasListener());
    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::PROGRESS, Version::API10));
    ASSERT_EQ(listener->calls, 1);
    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::COMPLETED, Version::API10));
    EXPECT_EQ(listener->calls, 1);

    request.RemoveListener(SubscribeType::PROGRESS, listener);
    EXPECT_FALSE(request.HasListener());
    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::PROGRESS, Version::API10));
    EXPECT_EQ(listener->calls, 1);
}

/**
 * @tc.name: NotifyListenerInvalidType003
 * @tc.desc: RESPONSE and BUTT are rejected as notify listener types
 * @tc.precon: NA
 * @tc.step: 1. Register a notify listener with type RESPONSE
 *           2. Register a notify listener with type BUTT
 *           3. Check the registry and deliver a PROGRESS event of API10
 * @tc.expect: Neither registration takes effect and the API10 event is not
 *             cached for later replay
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, NotifyListenerInvalidType003, TestSize.Level1)
{
    Request request("tid-notify-002");
    auto listener = std::make_shared<CountNotifyListener>();

    request.AddListener(SubscribeType::RESPONSE, listener);
    request.AddListener(SubscribeType::BUTT, listener);
    EXPECT_FALSE(request.HasListener());

    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::PROGRESS, Version::API10));
    request.AddListener(SubscribeType::PROGRESS, listener);
    EXPECT_EQ(listener->calls, 0);
}

/**
 * @tc.name: UnusedNotifyReplay004
 * @tc.desc: an API9 event arriving before its listener is cached and
 *           replayed on registration; a completed API9 replay also clears
 *           the pending-remove flag
 * @tc.precon: NA
 * @tc.step: 1. Deliver a COMPLETED API9 event with no listener registered
 *           2. Register a listener for COMPLETED
 *           3. Deliver a REMOVE API9 event with no REMOVE listener
 *           4. Register the same listener for REMOVE and deliver REMOVE
 *              API9, then REMOVE API10
 * @tc.expect: The completed event replays once; the first REMOVE API9
 *             delivery is forwarded (pending-remove still set), the replayed
 *             REMOVE API9 is suppressed and REMOVE API10 is forwarded
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, UnusedNotifyReplay004, TestSize.Level1)
{
    Request request("tid-notify-003");
    auto listener = std::make_shared<CountNotifyListener>();

    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::COMPLETED, Version::API9));
    EXPECT_EQ(listener->calls, 0);
    request.AddListener(SubscribeType::COMPLETED, listener);
    ASSERT_EQ(listener->calls, 1);

    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::REMOVE, Version::API9));
    EXPECT_EQ(listener->calls, 1);

    request.AddListener(SubscribeType::REMOVE, listener);
    EXPECT_EQ(listener->calls, 1);
    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::REMOVE, Version::API9));
    EXPECT_EQ(listener->calls, 1);
    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::REMOVE, Version::API10));
    ASSERT_EQ(listener->calls, 2);
}

/**
 * @tc.name: PendingRemoveDefaultTrue005
 * @tc.desc: with the pending-remove flag still at its default, a REMOVE
 *           event of API9 is forwarded to the registered listener
 * @tc.precon: NA
 * @tc.step: 1. Register a listener for REMOVE on a fresh Request
 *           2. Deliver a REMOVE API9 event
 * @tc.expect: The event is forwarded exactly once
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, PendingRemoveDefaultTrue005, TestSize.Level1)
{
    Request request("tid-notify-004");
    auto listener = std::make_shared<CountNotifyListener>();
    request.AddListener(SubscribeType::REMOVE, listener);

    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::REMOVE, Version::API9));
    EXPECT_EQ(listener->calls, 1);
}

/**
 * @tc.name: Api10EventNotCached006
 * @tc.desc: an API10 event without a listener is dropped instead of cached
 * @tc.precon: NA
 * @tc.step: 1. Deliver a COMPLETED API10 event with no listener
 *           2. Register a listener for COMPLETED
 * @tc.expect: The listener receives no replay
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, Api10EventNotCached006, TestSize.Level1)
{
    Request request("tid-notify-005");
    auto listener = std::make_shared<CountNotifyListener>();

    request.OnNotifyDataReceive(MakeNotifyData(SubscribeType::COMPLETED, Version::API10));
    request.AddListener(SubscribeType::COMPLETED, listener);
    EXPECT_EQ(listener->calls, 0);
}

/**
 * @tc.name: FaultsForward007
 * @tc.desc: fault events reach the listener registered for the fault type
 *           and are dropped for unregistered types
 * @tc.precon: NA
 * @tc.step: 1. Register a listener for FAULT_OCCUR
 *           2. Deliver a FAULT_OCCUR fault with a pinned reason
 *           3. Deliver a PAUSE fault
 * @tc.expect: Only the FAULT_OCCUR delivery forwards and carries the pinned
 *             reason
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, FaultsForward007, TestSize.Level1)
{
    Request request("tid-fault-001");
    auto listener = std::make_shared<CountNotifyListener>();
    request.AddListener(SubscribeType::FAULT_OCCUR, listener);

    auto tid = std::make_shared<int32_t>(NOTIFY_TASK_ID_VALUE);
    auto type = std::make_shared<SubscribeType>(SubscribeType::FAULT_OCCUR);
    auto reason = std::make_shared<Reason>(Reason::NETWORK_OFFLINE);
    request.OnFaultsReceive(tid, type, reason);
    ASSERT_EQ(listener->faultCalls, 1);
    EXPECT_EQ(listener->lastFaultReason, Reason::NETWORK_OFFLINE);

    auto pauseType = std::make_shared<SubscribeType>(SubscribeType::PAUSE);
    request.OnFaultsReceive(tid, pauseType, reason);
    EXPECT_EQ(listener->faultCalls, 1);
}

/**
 * @tc.name: WaitForward008
 * @tc.desc: wait events reach the WAIT listener once it is registered and
 *           are dropped before
 * @tc.precon: NA
 * @tc.step: 1. Deliver a wait event with no listener
 *           2. Register a listener for WAIT
 *           3. Deliver the wait event again
 * @tc.expect: The first delivery is dropped, the second forwards with the
 *             pinned waiting reason
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, WaitForward008, TestSize.Level1)
{
    Request request("tid-wait-001");
    auto listener = std::make_shared<CountNotifyListener>();

    request.OnWaitReceive(WAIT_TASK_ID_VALUE, WaitingReason::TaskQueueFull);
    EXPECT_EQ(listener->waitCalls, 0);

    request.AddListener(SubscribeType::WAIT, listener);
    request.OnWaitReceive(WAIT_TASK_ID_VALUE, WaitingReason::TaskQueueFull);
    ASSERT_EQ(listener->waitCalls, 1);
    EXPECT_EQ(listener->lastWaitReason, WaitingReason::TaskQueueFull);
}

/**
 * @tc.name: HasListenerMix009
 * @tc.desc: HasListener reports true while either a response listener or any
 *           notify listener is registered
 * @tc.precon: NA
 * @tc.step: 1. Register a notify listener for PROGRESS and check
 *           2. Remove it, register a response listener and check
 * @tc.expect: HasListener is true in both registered states and false after
 *             the final removal
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestListenerTest, HasListenerMix009, TestSize.Level1)
{
    Request request("tid-has-001");
    auto notifyListener = std::make_shared<CountNotifyListener>();
    auto responseListener = std::make_shared<CountResponseListener>();

    request.AddListener(SubscribeType::PROGRESS, notifyListener);
    EXPECT_TRUE(request.HasListener());
    request.RemoveListener(SubscribeType::PROGRESS, notifyListener);
    EXPECT_FALSE(request.HasListener());

    request.AddListener(SubscribeType::RESPONSE, responseListener);
    EXPECT_TRUE(request.HasListener());
    request.RemoveListener(SubscribeType::RESPONSE, responseListener);
    EXPECT_FALSE(request.HasListener());
}
