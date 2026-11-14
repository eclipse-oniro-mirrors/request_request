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

#include "request_utils_dataability.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>

using namespace testing::ext;
using namespace OHOS::Request;

namespace {
// Loopback-style constant path. It is never touched on device because the
// null-context guard short-circuits before any filesystem access.
const std::string LOCAL_PATH = "/data/local/tmp/request_ut.dat";
} // namespace

class RequestUtilsDataAbilityTest : public testing::Test {};

/**
 * @tc.name: DataAbilityOpenFileNullContext001
 * @tc.desc: DataAbilityOpenFile reports failure for a null context before any
 *           DataAbilityHelper is created, regardless of the requested path
 * @tc.precon: NA
 * @tc.step: 1. Call DataAbilityOpenFile with a null context and a constant path
 *           2. Call DataAbilityOpenFile with a null context and an empty path
 *           3. Call DataAbilityOpenFile with a second null context instance
 * @tc.expect: All three calls return -1 because the helper creator rejects a
 *             null context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestUtilsDataAbilityTest, DataAbilityOpenFileNullContext001, TestSize.Level1)
{
    std::shared_ptr<Context> nullContext;
    EXPECT_EQ(DataAbilityOpenFile(nullContext, LOCAL_PATH), -1);
    EXPECT_EQ(DataAbilityOpenFile(nullContext, ""), -1);
    std::shared_ptr<Context> otherNullContext;
    EXPECT_EQ(DataAbilityOpenFile(otherNullContext, LOCAL_PATH), -1);
}
