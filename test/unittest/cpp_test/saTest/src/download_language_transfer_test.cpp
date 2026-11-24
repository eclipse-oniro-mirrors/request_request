/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>

#include <gtest/gtest.h>

// GetSystemLanguageByIntl has no public header; it is an extern "C" export of
// libdownload_language_transfer.z.so declared here to link the unit under test.
extern "C" void GetSystemLanguageByIntl(char *buffer, size_t bufferSize);

using namespace testing::ext;

namespace {
// Sentinel byte proving the callee did not write into the buffer.
constexpr char SENTINEL_BYTE = 'X';

// Buffer size used for the full-copy invocation.
constexpr size_t FULL_BUFFER_SIZE = 32;
} // namespace

class DownloadLanguageTransferTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void DownloadLanguageTransferTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void DownloadLanguageTransferTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void DownloadLanguageTransferTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testCases
}

void DownloadLanguageTransferTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testCases
}

/**
 * @tc.name: GetSystemLanguageGuardTest
 * @tc.desc: Test GetSystemLanguageByIntl guard arcs for a null buffer and a
 *           zero buffer size
 * @tc.precon: NA
 * @tc.step: 1. Call GetSystemLanguageByIntl with a null buffer
 *           2. Fill a buffer with the sentinel byte and call it with size 0
 * @tc.expect: The null-buffer call returns without crashing and the zero-size
 *             call leaves the sentinel byte untouched
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(DownloadLanguageTransferTest, GetSystemLanguageGuardTest, TestSize.Level1)
{
    GetSystemLanguageByIntl(nullptr, FULL_BUFFER_SIZE);
    char buffer[FULL_BUFFER_SIZE];
    std::fill_n(buffer, sizeof(buffer), SENTINEL_BYTE);
    GetSystemLanguageByIntl(buffer, 0);
    EXPECT_EQ(buffer[0], SENTINEL_BYTE);
}

/**
 * @tc.name: GetSystemLanguageTruncateTest
 * @tc.desc: Test GetSystemLanguageByIntl truncation arcs with size 2 and 1
 * @tc.precon: NA
 * @tc.step: 1. Call GetSystemLanguageByIntl with a size 2 buffer
 *           2. Check the result keeps exactly one character plus terminator
 *           3. Call GetSystemLanguageByIntl with a size 1 buffer
 *           4. Check the result is only the terminator
 * @tc.expect: The stored length is always bufferSize minus the terminator
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(DownloadLanguageTransferTest, GetSystemLanguageTruncateTest, TestSize.Level1)
{
    char buffer2[2];
    std::fill_n(buffer2, sizeof(buffer2), SENTINEL_BYTE);
    GetSystemLanguageByIntl(buffer2, sizeof(buffer2));
    EXPECT_EQ(strlen(buffer2), static_cast<size_t>(1));

    char buffer1[1];
    std::fill_n(buffer1, sizeof(buffer1), SENTINEL_BYTE);
    GetSystemLanguageByIntl(buffer1, sizeof(buffer1));
    EXPECT_EQ(strlen(buffer1), static_cast<size_t>(0));
}

/**
 * @tc.name: GetSystemLanguageNonEmptyTest
 * @tc.desc: Test GetSystemLanguageByIntl always yields a non-empty language
 * @tc.precon: The implementation falls back to zh-Hans when the system
 *           language is empty, so the result is never empty
 * @tc.step: 1. Call GetSystemLanguageByIntl with a full-size buffer
 * @tc.expect: The buffer holds a non-empty terminated language string
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(DownloadLanguageTransferTest, GetSystemLanguageNonEmptyTest, TestSize.Level1)
{
    char buffer[FULL_BUFFER_SIZE];
    std::fill_n(buffer, sizeof(buffer), SENTINEL_BYTE);
    GetSystemLanguageByIntl(buffer, sizeof(buffer));
    EXPECT_GT(strlen(buffer), static_cast<size_t>(0));
    EXPECT_NE(buffer[0], SENTINEL_BYTE);
}
