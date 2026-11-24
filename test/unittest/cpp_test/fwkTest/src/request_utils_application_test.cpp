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

#include <string>

#include <gtest/gtest.h>

#include "application_context.h"
#include "request_utils_application.h"

using namespace testing::ext;
using namespace OHOS::Request;

class RequestUtilsApplicationTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestUtilsApplicationTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RequestUtilsApplicationTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void RequestUtilsApplicationTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testCases
}

void RequestUtilsApplicationTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testCases
}

/**
 * @tc.name: GetCacheDirNullContextTest
 * @tc.desc: Test GetCacheDir in a process without an application context
 * @tc.precon: The unittest process has no application context attached, so
 *           Context::GetApplicationContext returns null
 * @tc.step: 1. Call GetCacheDir in the context-less process
 *           2. Compare the returned directory with the empty string
 *           3. Call GetCacheDir again and compare the two results
 * @tc.expect: GetCacheDir returns an empty string on the null context and
 *             repeats the same answer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestUtilsApplicationTest, GetCacheDirNullContextTest, TestSize.Level1)
{
    EXPECT_EQ(OHOS::AbilityRuntime::Context::GetApplicationContext(), nullptr);
    std::string cacheDir = static_cast<std::string>(GetCacheDir());
    EXPECT_TRUE(cacheDir.empty());

    std::string cacheDirAgain = static_cast<std::string>(GetCacheDir());
    EXPECT_EQ(cacheDirAgain, cacheDir);
}
