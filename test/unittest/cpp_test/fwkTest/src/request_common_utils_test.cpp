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
#include <vector>

#include "gmock/gmock.h"
#include "log.h"
#include "request_common.h"
#include "request_common_utils.h"

using namespace testing::ext;
using namespace OHOS::Request;

#undef private
#undef protected

class RequestCommonUtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestCommonUtilsTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RequestCommonUtilsTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void RequestCommonUtilsTest::SetUp(void)
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

void RequestCommonUtilsTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
}

namespace {
// Full {Reason -> Faults, message} mapping of CommonUtils, mirrors the static
// tables in request_common_utils.cpp so that any drift between the tables or
// the enum is caught by the compiler-visible enum values, not silently.
struct ReasonExpect {
    Reason reason;
    Faults fault;
    const char *msg;
};

const ReasonExpect G_REASON_EXPECTS[] = {
    { REASON_OK, Faults::OTHERS, "Task successful" },
    { TASK_SURVIVAL_ONE_MONTH, Faults::OTHERS, "The task has not been completed for a month yet" },
    { WAITTING_NETWORK_ONE_DAY, Faults::OTHERS,
        "The task waiting for network recovery has not been completed for a day yet" },
    { STOPPED_NEW_FRONT_TASK, Faults::OTHERS, "Stopped by a new front task" },
    { RUNNING_TASK_MEET_LIMITS, Faults::OTHERS, "Too many task in running state" },
    { USER_OPERATION, Faults::OTHERS, "User operation" },
    { APP_BACKGROUND_OR_TERMINATE, Faults::OTHERS, "The app is background or terminate" },
    { NETWORK_OFFLINE, Faults::DISCONNECTED, "NetWork is offline" },
    { UNSUPPORTED_NETWORK_TYPE, Faults::OTHERS, "NetWork type not meet the task config" },
    { BUILD_CLIENT_FAILED, Faults::PARAM, "Build client error" },
    { BUILD_REQUEST_FAILED, Faults::PARAM, "Build request error" },
    { GET_FILESIZE_FAILED, Faults::FSIO,
        "Failed because cannot get the file size from the server and the precise is setted true by user" },
    { CONTINUOUS_TASK_TIMEOUT, Faults::TIMEOUT, "Continuous processing task time out" },
    { CONNECT_ERROR, Faults::TCP, "Connect error" },
    { REQUEST_ERROR, Faults::PROTOCOL, "Request error" },
    { UPLOAD_FILE_ERROR, Faults::OTHERS, "There are some files upload failed" },
    { REDIRECT_ERROR, Faults::REDIRECT, "Redirect error" },
    { PROTOCOL_ERROR, Faults::PROTOCOL, "Http protocol error" },
    { IO_ERROR, Faults::FSIO, "Io Error" },
    { UNSUPPORT_RANGE_REQUEST, Faults::PROTOCOL, "Range request not supported" },
    { OTHERS_ERROR, Faults::OTHERS, "Some other error occured" },
    { ACCOUNT_STOPPED, Faults::OTHERS, "Account stopped" },
    { NETWORK_CHANGED, Faults::OTHERS, "Network changed" },
    { DNS, Faults::DNS, "DNS error" },
    { TCP, Faults::TCP, "TCP error" },
    { SSL, Faults::SSL, "TSL/SSL error" },
    { INSUFFICIENT_SPACE, Faults::OTHERS, "Insufficient space error" },
    { NETWORK_APP, Faults::DISCONNECTED, "NetWork is offline and the app is background or terminate" },
    { NETWORK_ACCOUNT, Faults::DISCONNECTED, "NetWork is offline and the account is stopped" },
    { APP_ACCOUNT, Faults::OTHERS, "The account is stopped and the app is background or terminate" },
    { NETWORK_APP_ACCOUNT, Faults::DISCONNECTED,
        "NetWork is offline and the account is stopped and the app isbackground or terminate" },
    { LOW_SPEED, Faults::LOW_SPEED, "Below low speed limit" },
};
} // namespace

/**
 * @tc.name: GetFaultByReasonAllReasons001
 * @tc.desc: GetFaultByReason maps every Reason in the static table to the
 *           documented Faults category
 * @tc.precon: NA
 * @tc.step: 1. Iterate over the full Reason/Faults expectation table
 *           2. Call CommonUtils::GetFaultByReason for each Reason
 * @tc.expect: Each Reason maps to its documented Faults value (device SDK
 *             version is >= 12, so no downgrade applies)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestCommonUtilsTest, GetFaultByReasonAllReasons001, TestSize.Level1)
{
    for (const auto &expect : G_REASON_EXPECTS) {
        Faults fault = CommonUtils::GetFaultByReason(expect.reason);
        EXPECT_EQ(fault, expect.fault) << "fault mismatch for Reason " << static_cast<uint32_t>(expect.reason);
    }
}

/**
 * @tc.name: GetFaultByReasonUnknown001
 * @tc.desc: GetFaultByReason falls back to Faults::OTHERS for a Reason that is
 *           not in the table
 * @tc.precon: NA
 * @tc.step: 1. Call GetFaultByReason with an out-of-table Reason value
 * @tc.expect: Returns Faults::OTHERS
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestCommonUtilsTest, GetFaultByReasonUnknown001, TestSize.Level1)
{
    Reason unknown = static_cast<Reason>(0x7FFFFFFF);
    Faults fault = CommonUtils::GetFaultByReason(unknown);
    EXPECT_EQ(fault, Faults::OTHERS);
}

/**
 * @tc.name: GetMsgByReasonAllReasons001
 * @tc.desc: GetMsgByReason returns the documented message for every Reason in
 *           the static table
 * @tc.precon: NA
 * @tc.step: 1. Iterate over the full Reason/message expectation table
 *           2. Call CommonUtils::GetMsgByReason for each Reason
 * @tc.expect: Each Reason returns its documented non-empty message
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestCommonUtilsTest, GetMsgByReasonAllReasons001, TestSize.Level1)
{
    for (const auto &expect : G_REASON_EXPECTS) {
        std::string msg = CommonUtils::GetMsgByReason(expect.reason);
        EXPECT_EQ(msg, expect.msg) << "message mismatch for Reason " << static_cast<uint32_t>(expect.reason);
        EXPECT_FALSE(msg.empty());
    }
}

/**
 * @tc.name: GetMsgByReasonUnknown001
 * @tc.desc: GetMsgByReason falls back to "unknown" for a Reason that is not in
 *           the table
 * @tc.precon: NA
 * @tc.step: 1. Call GetMsgByReason with an out-of-table Reason value
 * @tc.expect: Returns the literal string "unknown"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestCommonUtilsTest, GetMsgByReasonUnknown001, TestSize.Level1)
{
    Reason unknown = static_cast<Reason>(0x7FFFFFFE);
    std::string msg = CommonUtils::GetMsgByReason(unknown);
    EXPECT_EQ(msg, "unknown");
}

/**
 * @tc.name: ReasonTableCoversEnum001
 * @tc.desc: The expectation table covers the whole Reason enum range
 *           consecutively, so adding a new Reason without updating the table
 *           fails this test
 * @tc.precon: NA
 * @tc.step: 1. Check that REASON_OK is the first entry and LOW_SPEED the last
 *           2. Check that the table size matches LOW_SPEED - REASON_OK + 1
 * @tc.expect: Table size equals the full Reason enum span
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestCommonUtilsTest, ReasonTableCoversEnum001, TestSize.Level1)
{
    constexpr size_t tableSize = sizeof(G_REASON_EXPECTS) / sizeof(G_REASON_EXPECTS[0]);
    constexpr size_t enumSpan = static_cast<size_t>(LOW_SPEED) - static_cast<size_t>(REASON_OK) + 1;
    EXPECT_EQ(G_REASON_EXPECTS[0].reason, REASON_OK);
    EXPECT_EQ(G_REASON_EXPECTS[tableSize - 1].reason, LOW_SPEED);
    EXPECT_EQ(tableSize, enumSpan);
}
