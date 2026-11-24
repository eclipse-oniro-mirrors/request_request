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

#include <string>
#define private public
#define protected public

#include <gtest/gtest.h>

#include <cstdint>
#include <unordered_map>

#include "gmock/gmock.h"
#include "log.h"
#include "sys_event.h"

using namespace testing::ext;
using namespace OHOS::Request;

#undef private
#undef protected

class SysEventTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void SysEventTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void SysEventTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void SysEventTest::SetUp(void)
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

void SysEventTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
}

/**
 * @tc.name: SysEventName001
 * @tc.desc: The hisysevent names and the routing table of SysEventLog are
 *           stable, hiview analysis depends on them
 * @tc.precon: NA
 * @tc.step: 1. Check STATISTIC_EVENT and FAULT_EVENT literal values
 *           2. Check that sysEventMap_ routes exactly these two names
 * @tc.expect: Event names are "EXEC_ERROR"/"EXEC_FAULT" and sysEventMap_
 *             contains exactly those two entries
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(SysEventTest, SysEventName001, TestSize.Level1)
{
    EXPECT_STREQ(STATISTIC_EVENT, "EXEC_ERROR");
    EXPECT_STREQ(FAULT_EVENT, "EXEC_FAULT");

    EXPECT_EQ(SysEventLog::sysEventMap_.size(), static_cast<size_t>(2));
    EXPECT_NE(SysEventLog::sysEventMap_.find(STATISTIC_EVENT), SysEventLog::sysEventMap_.end());
    EXPECT_NE(SysEventLog::sysEventMap_.find(FAULT_EVENT), SysEventLog::sysEventMap_.end());
}

/**
 * @tc.name: SysEventDfxCode001
 * @tc.desc: Spot-check the DfxErrorCode bitmask layout: the class nibble in
 *           the low byte, the subsystem nibble in bits 8-15, and full-value
 *           sentinels used by fuzzing/hiview
 * @tc.precon: NA
 * @tc.step: 1. Compare representative DfxErrorCode values against their
 *           documented constants
 * @tc.expect: Values match the documented constants
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(SysEventTest, SysEventDfxCode001, TestSize.Level1)
{
    EXPECT_EQ(static_cast<uint32_t>(APP_ERROR_00), 0x00000000u);
    EXPECT_EQ(static_cast<uint32_t>(APP_ERROR_01), 0x00000001u);
    EXPECT_EQ(static_cast<uint32_t>(INVALID_IPC_MESSAGE_A00), 0x001FFFFFu);
    EXPECT_EQ(static_cast<uint32_t>(INVALID_IPC_MESSAGE_00), 0x00100000u);
    EXPECT_EQ(static_cast<uint32_t>(TASK_FAULT_00), 0x002F00FFu);
    EXPECT_EQ(static_cast<uint32_t>(UDS_FAULT_00), 0x00300000u);
    EXPECT_EQ(static_cast<uint32_t>(SA_ERROR_00), 0x004F0000u);
    EXPECT_EQ(static_cast<uint32_t>(ACL_FAULT_00), 0xF0000000u);
    EXPECT_EQ(static_cast<uint32_t>(IPC_FAULT_00), 0xF0100000u);
    EXPECT_EQ(static_cast<uint32_t>(IPC_FAULT_01), 0xF0100001u);
    EXPECT_EQ(static_cast<uint32_t>(RDB_FAULT_13), 0xF06FFFFFu);
    EXPECT_EQ(static_cast<uint32_t>(STANDARD_FAULT_00), 0xF1000000u);
}

/**
 * @tc.name: SendSysEventLogUnknownEvent001
 * @tc.desc: SendSysEventLog is a silent no-op for an event name that is not in
 *           sysEventMap_
 * @tc.precon: NA
 * @tc.step: 1. Check that the unknown event name is absent from sysEventMap_
 *           2. Call the 3 overloads of SendSysEventLog with the unknown name
 * @tc.expect: The lookup precondition holds and the calls complete without
 *             crashing (the build disables exceptions, so any fault aborts the
 *             test process and fails the case)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(SysEventTest, SendSysEventLogUnknownEvent001, TestSize.Level1)
{
    const std::string unknown = "NOT_A_REQUEST_EVENT";
    EXPECT_EQ(SysEventLog::sysEventMap_.find(unknown), SysEventLog::sysEventMap_.end());

    SysEventLog::SendSysEventLog(unknown, IPC_FAULT_00, "extra");
    SysEventLog::SendSysEventLog(unknown, IPC_FAULT_00, "bundle", "module", "extra");
    SysEventLog::SendSysEventLog(unknown, IPC_FAULT_00, 1, 2);
}

/**
 * @tc.name: SendSysEventLogKnownEvent001
 * @tc.desc: The known-event overloads build a SysEventInfo and reach
 *           HiSysEventWrite; the write itself may fail under the test
 *           process's SELinux policy, the caller-visible contract is only
 *           "must not crash"
 * @tc.precon: NA
 * @tc.step: 1. Check that FAULT_EVENT and STATISTIC_EVENT are routed in
 *              sysEventMap_
 *           2. Call the 3 overloads of SendSysEventLog with both event names
 * @tc.expect: The routing precondition holds and the calls complete without
 *             crashing (any fault aborts the test process and fails the case)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(SysEventTest, SendSysEventLogKnownEvent001, TestSize.Level1)
{
    EXPECT_NE(SysEventLog::sysEventMap_.find(FAULT_EVENT), SysEventLog::sysEventMap_.end());
    EXPECT_NE(SysEventLog::sysEventMap_.find(STATISTIC_EVENT), SysEventLog::sysEventMap_.end());

    SysEventLog::SendSysEventLog(FAULT_EVENT, IPC_FAULT_00, std::to_string(-1));
    SysEventLog::SendSysEventLog(FAULT_EVENT, IPC_FAULT_01, "com.test.bundle", "entry", "extraInfo");
    SysEventLog::SendSysEventLog(FAULT_EVENT, TASK_FAULT_00, 60, 55);
    SysEventLog::SendSysEventLog(STATISTIC_EVENT, APP_ERROR_01, "extraInfo");
    SysEventLog::SendSysEventLog(STATISTIC_EVENT, APP_ERROR_02, "com.test.bundle", "entry", "extraInfo");
    SysEventLog::SendSysEventLog(STATISTIC_EVENT, APP_ERROR_00, 100, 90);
}

namespace {
constexpr uint32_t ONE_CALL_COUNT = 1;
constexpr uint32_t REPEAT_CALL_COUNT = 2;
constexpr uint32_t ZERO_CALL_COUNT = 0;
uint32_t g_recordCount = 0;
SysEventInfo g_lastInfo = { 0, "", "", "" };

void RecordCallback(const SysEventInfo &info)
{
    g_recordCount += 1;
    g_lastInfo = info;
}
} // namespace

/**
 * @tc.name: SendSysEventLogRepeatSameEvent001
 * @tc.desc: Repeated sends of the same event name reach the registered
 *           callback every time and deliver the info of the latest call; a
 *           recorded callback replaces the real hiview writer so the case
 *           makes no hisysevent writes
 * @tc.precon: NA
 * @tc.step: 1. Replace both sysEventMap_ callbacks with RecordCallback and
 *              save the originals
 *           2. Call each of the 3 SendSysEventLog overloads twice with the
 *              same event name
 *           3. Check the recorded call count and the last delivered info
 *           4. Restore the original callbacks and check the table size
 * @tc.expect: Each pair of calls triggers exactly REPEAT_CALL_COUNT
 *             deliveries, the last info carries the latest payload of its
 *             overload, and the routing table is restored to 2 entries
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(SysEventTest, SendSysEventLogRepeatSameEvent001, TestSize.Level1)
{
    auto savedStatistic = SysEventLog::sysEventMap_[STATISTIC_EVENT];
    auto savedFault = SysEventLog::sysEventMap_[FAULT_EVENT];
    SysEventLog::sysEventMap_[STATISTIC_EVENT] = RecordCallback;
    SysEventLog::sysEventMap_[FAULT_EVENT] = RecordCallback;

    g_recordCount = 0;
    SysEventLog::SendSysEventLog(STATISTIC_EVENT, APP_ERROR_01, "bundle", "module", "extra");
    SysEventLog::SendSysEventLog(STATISTIC_EVENT, APP_ERROR_02, "bundle2", "module2", "extra2");
    EXPECT_EQ(g_recordCount, REPEAT_CALL_COUNT);
    EXPECT_EQ(g_lastInfo.dCode, static_cast<uint32_t>(APP_ERROR_02));
    EXPECT_EQ(g_lastInfo.bundleName, "bundle2");
    EXPECT_EQ(g_lastInfo.extraInfo, "extra2");

    g_recordCount = 0;
    SysEventLog::SendSysEventLog(FAULT_EVENT, IPC_FAULT_00, "extraOne");
    SysEventLog::SendSysEventLog(FAULT_EVENT, IPC_FAULT_01, "extraTwo");
    EXPECT_EQ(g_recordCount, REPEAT_CALL_COUNT);
    EXPECT_EQ(g_lastInfo.dCode, static_cast<uint32_t>(IPC_FAULT_01));
    EXPECT_EQ(g_lastInfo.bundleName, "");
    EXPECT_EQ(g_lastInfo.extraInfo, "extraTwo");

    g_recordCount = 0;
    SysEventLog::SendSysEventLog(FAULT_EVENT, TASK_FAULT_00, 60, 55);
    SysEventLog::SendSysEventLog(FAULT_EVENT, TASK_FAULT_00, 61, 56);
    EXPECT_EQ(g_recordCount, REPEAT_CALL_COUNT);
    EXPECT_EQ(g_lastInfo.dCode, static_cast<uint32_t>(TASK_FAULT_00));
    EXPECT_EQ(g_lastInfo.extraInfo, "expect61=56");

    SysEventLog::sysEventMap_[STATISTIC_EVENT] = savedStatistic;
    SysEventLog::sysEventMap_[FAULT_EVENT] = savedFault;
    EXPECT_EQ(SysEventLog::sysEventMap_.size(), static_cast<size_t>(2));
}

/**
 * @tc.name: SendSysEventLogUnregisteredEvent001
 * @tc.desc: sends of an event name that is not in the routing table return
 *           early in all three overloads and reach no callback; a registered
 *           send first proves the callback path is live
 * @tc.precon: NA
 * @tc.step: 1. Replace both sysEventMap_ callbacks with RecordCallback and
 *              save the originals
 *           2. Send one registered FAULT_EVENT and check the delivery count
 *              and the recorded payload
 *           3. Send the 3 SendSysEventLog overloads with an unregistered
 *              event name
 *           4. Check the count did not increase and restore the callbacks
 * @tc.expect: The registered send records exactly ONE_CALL_COUNT deliveries
 *             with its payload, and the count stays at that level afterwards
 *             because the unregistered name misses the routing table
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(SysEventTest, SendSysEventLogUnregisteredEvent001, TestSize.Level1)
{
    auto savedStatistic = SysEventLog::sysEventMap_[STATISTIC_EVENT];
    auto savedFault = SysEventLog::sysEventMap_[FAULT_EVENT];
    SysEventLog::sysEventMap_[STATISTIC_EVENT] = RecordCallback;
    SysEventLog::sysEventMap_[FAULT_EVENT] = RecordCallback;

    const char *unregistered = "EXEC_NOT_REGISTERED";
    g_recordCount = ZERO_CALL_COUNT;
    SysEventLog::SendSysEventLog(FAULT_EVENT, IPC_FAULT_00, "extraAlive");
    EXPECT_EQ(g_recordCount, ONE_CALL_COUNT);
    EXPECT_EQ(g_lastInfo.dCode, static_cast<uint32_t>(IPC_FAULT_00));
    EXPECT_EQ(g_lastInfo.bundleName, "");
    EXPECT_EQ(g_lastInfo.extraInfo, "extraAlive");

    SysEventLog::SendSysEventLog(unregistered, APP_ERROR_01, "extra");
    SysEventLog::SendSysEventLog(unregistered, APP_ERROR_01, "bundle", "module", "extra");
    SysEventLog::SendSysEventLog(unregistered, TASK_FAULT_00, 60, 55);
    EXPECT_EQ(g_recordCount, ONE_CALL_COUNT);

    SysEventLog::sysEventMap_[STATISTIC_EVENT] = savedStatistic;
    SysEventLog::sysEventMap_[FAULT_EVENT] = savedFault;
    EXPECT_EQ(SysEventLog::sysEventMap_.size(), static_cast<size_t>(2));
}
