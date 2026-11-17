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

#include "utf8_utils.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "log.h"

using namespace testing::ext;
using namespace OHOS::Request::Utf8Utils;

class Utf8UtilsTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void Utf8UtilsTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void Utf8UtilsTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void Utf8UtilsTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testCases
    testing::UnitTest *test = testing::UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const testing::TestInfo *testInfo = test->current_test_info();
    ASSERT_NE(testInfo, nullptr);
    string testCaseName = string(testInfo->name());
    REQUEST_HILOGI("[SetUp] %{public}s start", testCaseName.c_str());
    GTEST_LOG_(INFO) << testCaseName.append(" start");
}

void Utf8UtilsTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testCases
}

namespace {
std::vector<uint8_t> Bytes(const std::initializer_list<uint8_t> &list)
{
    return std::vector<uint8_t>(list);
}
} // namespace

/**
 * @tc.name: RunUtf8ValidationAscii001
 * @tc.desc: ASCII-only and empty inputs are always valid UTF-8
 * @tc.precon: NA
 * @tc.step: 1. Call RunUtf8Validation with an empty vector
 *           2. Call RunUtf8Validation with ASCII text
 * @tc.expect: Both return true
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationAscii001, TestSize.Level1)
{
    EXPECT_TRUE(RunUtf8Validation({}));
    std::string ascii("c_string_wrapper_for_test 0123 !@#");
    std::vector<uint8_t> v(ascii.begin(), ascii.end());
    EXPECT_TRUE(RunUtf8Validation(v));
}

/**
 * @tc.name: RunUtf8ValidationTwoOctet001
 * @tc.desc: Valid and invalid 2-octet UTF-8 sequences (C2..DF lead, 80..BF
 *           tail); C0/C1 are invalid lead bytes
 * @tc.precon: NA
 * @tc.step: 1. Validate well-formed C2 80 and DF BF
 *           2. Validate invalid leads C0 80 / C1 80, bad tail C2 7F and
 *              truncated C2
 * @tc.expect: Well-formed sequences pass, the others are rejected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationTwoOctet001, TestSize.Level1)
{
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xC2, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xDF, 0xBF })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xC0, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xC1, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xC2, 0x7F })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xC2 })));
}

/**
 * @tc.name: RunUtf8ValidationThreeOctet001
 * @tc.desc: 3-octet sequences: E0 requires an A0..BF second byte, ED requires
 *           80..9F (excludes surrogate range), E1..EC/EE/EF require 80..BF
 * @tc.precon: NA
 * @tc.step: 1. Validate well-formed E1 80 80 / E0 A0 80 / ED 80 80 / EF BF BF
 *           2. Validate overlong E0 9F 80 and surrogate ED A0 80
 *           3. Validate truncated E1 80
 * @tc.expect: Well-formed sequences pass, the others are rejected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationThreeOctet001, TestSize.Level1)
{
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xE1, 0x80, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xE0, 0xA0, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xED, 0x80, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xEE, 0x80, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xEF, 0xBF, 0xBF })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xE0, 0x9F, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xED, 0xA0, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xE1, 0x7F, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xE1, 0x80 })));
}

/**
 * @tc.name: RunUtf8ValidationFourOctet001
 * @tc.desc: 4-octet sequences: F0 requires 90..BF second byte, F4 requires
 *           80..8F (U+10FFFF cap), F5..FF are invalid leads
 * @tc.precon: NA
 * @tc.step: 1. Validate well-formed F0 90 80 80 / F1 80 80 80 / F3 BF BF BF /
 *              F4 8F BF BF
 *           2. Validate out-of-range F0 8F 80 80 / F4 90 80 80 and invalid
 *              leads F5 80 80 80 / F8 80 80 80 80
 *           3. Validate truncated F1 80 80
 * @tc.expect: Well-formed sequences pass, the others are rejected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationFourOctet001, TestSize.Level1)
{
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xF0, 0x90, 0x80, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xF1, 0x80, 0x80, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xF3, 0xBF, 0xBF, 0xBF })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xF4, 0x8F, 0xBF, 0xBF })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF0, 0x8F, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF4, 0x90, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF5, 0x80, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF8, 0x80, 0x80, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF1, 0x80, 0x80 })));
}

/**
 * @tc.name: RunUtf8ValidationContinuation001
 * @tc.desc: A continuation byte (80..BF) or an unexpected lead class as the
 *           first byte is invalid
 * @tc.precon: NA
 * @tc.step: 1. Validate single continuation bytes 0x80 and 0xBF
 * @tc.expect: Both are rejected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationContinuation001, TestSize.Level1)
{
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xBF })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 'a', 0x80, 'b' })));
}

/**
 * @tc.name: RunUtf8ValidationMixed001
 * @tc.desc: Realistic mixed content ("abc" + U+4E2D + U+1F600) validates, and
 *           one broken sequence anywhere invalidates the whole buffer
 * @tc.precon: NA
 * @tc.step: 1. Validate "abc" + E4 B8 AD + F0 9F 98 80
 *           2. Corrupt the last octet of the 4-octet char and validate again
 * @tc.expect: Mixed content passes; corrupted content fails
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationMixed001, TestSize.Level1)
{
    std::vector<uint8_t> mixed = { 'a', 'b', 'c', 0xE4 };
    const uint8_t mid[] = { 0xB8, 0xAD };
    const uint8_t emoji[] = { 0xF0, 0x9F, 0x98, 0x80 };
    mixed.insert(mixed.end(), mid, mid + sizeof(mid));
    mixed.insert(mixed.end(), emoji, emoji + sizeof(emoji));
    ASSERT_EQ(mixed.size(), 10U);
    EXPECT_TRUE(RunUtf8Validation(mixed));
    mixed[9] = 0x7F; // break the 4-octet tail
    EXPECT_FALSE(RunUtf8Validation(mixed));
}

/**
 * @tc.name: RunUtf8ValidationTruncatedTail001
 * @tc.desc: A truncated sequence at the very end of the buffer is rejected
 *           without reading past the buffer
 * @tc.precon: NA
 * @tc.step: 1. Validate a 2-octet lead at the end
 *           2. Validate a 4-octet sequence missing its last octet
 * @tc.expect: Both are rejected
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationTruncatedTail001, TestSize.Level1)
{
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 'x', 0xC2 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF1, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xE0 })));
}

/**
 * @tc.name: RunUtf8ValidationThreeOctetBoundary001
 * @tc.desc: 3-octet boundary tails: E0 and E1..EC reject tails above BF, ED
 *           rejects tails below 80, EE/EF reject tails outside 80..BF
 * @tc.precon: NA
 * @tc.step: 1. Validate E0/E1/EC sequences whose second byte exceeds BF
 *           2. Validate ED/EE sequences whose second byte is below 80
 *           3. Validate an EF sequence whose second byte exceeds BF
 * @tc.expect: All malformed sequences are rejected, a valid boundary passes
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationThreeOctetBoundary001, TestSize.Level1)
{
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xE0, 0xC0, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xE1, 0xC0, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xEC, 0xC0, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xED, 0x7F, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xEE, 0x7F, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xEF, 0xC0, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xE1, 0xBF, 0x80 })));
}

/**
 * @tc.name: RunUtf8ValidationFourOctetBoundary001
 * @tc.desc: 4-octet boundary cases: truncated leads, tails outside the range
 *           allowed for each lead, and a broken second continuation pair
 * @tc.precon: NA
 * @tc.step: 1. Validate truncated F0 and F4 leads
 *           2. Validate F0/F3 tails above BF and F1/F4 tails outside range
 *           3. Validate F0 90 followed by a non-continuation byte
 * @tc.expect: All malformed sequences are rejected, a valid sequence passes
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(Utf8UtilsTest, RunUtf8ValidationFourOctetBoundary001, TestSize.Level1)
{
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF0 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF4 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF0, 0xC0, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF1, 0x7F, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF3, 0xC0, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF4, 0x7F, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF4, 0x9F, 0x80, 0x80 })));
    EXPECT_FALSE(RunUtf8Validation(Bytes({ 0xF0, 0x90, 0x00, 0x80 })));
    EXPECT_TRUE(RunUtf8Validation(Bytes({ 0xF0, 0x90, 0x80, 0x80 })));
}
