/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cstring>
#include <sstream>
#define private public
#define protected public

#include <gtest/gtest.h>

#include "log.h"
#include "path_control.h"
#include "request_common.h"

using namespace testing::ext;
using namespace OHOS::Request;

#undef private
#undef protected

class PathControlTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void PathControlTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void PathControlTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void PathControlTest::SetUp(void)
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

void PathControlTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
}

/**
 * @tc.name: PathControlTest001
 * @tc.desc: Verify AddPathsToMap returns false for non-existent path
 * @tc.precon: NA
 * @tc.step: 1. Create filepath string "/A/B/C/test1"
 *           2. Call AddPathsToMap with filepath
 * @tc.expect: Should return false indicating failure
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest001, TestSize.Level1)
{
    std::string filepath = "/A/B/C/test1";
    bool res = PathControl::AddPathsToMap(filepath);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: PathControlTest002
 * @tc.desc: Verify SubPathsToMap returns false for non-existent path
 * @tc.precon: NA
 * @tc.step: 1. Create filepath string "/A/B/C/test1"
 *           2. Call SubPathsToMap with filepath
 * @tc.expect: Should return false indicating failure
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest002, TestSize.Level1)
{
    std::string filepath = "/A/B/C/test1";
    bool res = PathControl::SubPathsToMap(filepath);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: PathControlTest003
 * @tc.desc: Verify CheckBelongAppBaseDir returns false for non-app path
 * @tc.precon: NA
 * @tc.step: 1. Create filepath string "/A/B/C/test1"
 *           2. Call CheckBelongAppBaseDir with filepath
 * @tc.expect: Should return false indicating path is not in app base directory
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest003, TestSize.Level1)
{
    std::string filepath = "/A/B/C/test1";
    bool res = PathControl::CheckBelongAppBaseDir(filepath);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: PathControlTest004
 * @tc.desc: Verify ShieldPath correctly masks sensitive path segments
 * @tc.precon: NA
 * @tc.step: 1. Create path string "/ab/abcde"
 *           2. Call ShieldPath with path
 * @tc.expect: Should return masked path
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest004, TestSize.Level1)
{
    std::string path = "/ab/abcde";
    std::string res = PathControl::ShieldPath(path);
    EXPECT_EQ(res, "/*b/**cde");
}

/**
 * @tc.name: PathControlTest005
 * @tc.desc: CheckBelongAppBaseDir accepts the exact el1/el2/el5 base dirs and
 *           any deeper path inside them
 * @tc.precon: NA
 * @tc.step: 1. Call CheckBelongAppBaseDir with the three exact base dirs
 *           2. Call CheckBelongAppBaseDir with a deeper path inside each base
 * @tc.expect: All six calls return true
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest005, TestSize.Level1)
{
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el1/base"));
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el2/base"));
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el5/base"));
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el1/base/hips/file.txt"));
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el2/base/haps/entry/cache"));
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el5/base/rocket/a/b/c"));
}

/**
 * @tc.name: PathControlTest006
 * @tc.desc: CheckBelongAppBaseDir rejects paths outside the app base dirs
 * @tc.precon: NA
 * @tc.step: 1. Call CheckBelongAppBaseDir with empty, relative, non-canonical
 *              and sibling el3/el4 paths
 * @tc.expect: All calls return false
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest006, TestSize.Level1)
{
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir(""));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("relative/path"));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("data/storage/el1/base"));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("/data/storage/el3/base"));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("/data/storage/el4/base"));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("/data/storage/EL1/base"));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("/data/app/el1/base"));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("/data/storage"));
    EXPECT_FALSE(PathControl::CheckBelongAppBaseDir("/"));
}

/**
 * @tc.name: PathControlTest007
 * @tc.desc: CheckBelongAppBaseDir is a pure prefix check: trailing slash and
 *           suffix extensions still match; traversal-looking strings match too
 *           because the '..' segments are only resolved by the caller before
 *           reaching this gate
 * @tc.precon: NA
 * @tc.step: 1. Call CheckBelongAppBaseDir with "/data/storage/el1/base/"
 *           2. Call CheckBelongAppBaseDir with "/data/storage/el1/baseXYZ"
 *           3. Call CheckBelongAppBaseDir with a "../../" traversal string
 *              anchored at the el1 base
 * @tc.expect: All three return true (documents the current prefix-match
 *             semantics; '..' handling lives in the fd-based file checks)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest007, TestSize.Level1)
{
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el1/base/"));
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el1/baseXYZ"));
    EXPECT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el1/base/../../etc/passwd"));
}

/**
 * @tc.name: PathControlTest008
 * @tc.desc: ShieldPath of an empty path, root and a path without a leading
 *           slash
 * @tc.precon: NA
 * @tc.step: 1. Call ShieldPath with "", "/" and "ab"
 * @tc.expect: "" -> "", "/" -> "", "ab" -> "/" + "*b"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest008, TestSize.Level1)
{
    EXPECT_EQ(PathControl::ShieldPath(""), "");
    EXPECT_EQ(PathControl::ShieldPath("/"), "");
    EXPECT_EQ(PathControl::ShieldPath("ab"), "/*b");
}

/**
 * @tc.name: PathControlTest009
 * @tc.desc: ShieldPath masks the first half of every segment: short segments
 *           keep the single char, odd/even lengths round down to n/2 stars
 * @tc.precon: NA
 * @tc.step: 1. Call ShieldPath with single-char, 4-char and 5-char segments
 * @tc.expect: "/x" -> "/x"; "/abcd" -> "/" + "**cd"; "/abcde" -> "/" + "**cde"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest009, TestSize.Level1)
{
    EXPECT_EQ(PathControl::ShieldPath("/x"), "/x");
    EXPECT_EQ(PathControl::ShieldPath("/abcd"), "/**cd");
    EXPECT_EQ(PathControl::ShieldPath("/abcde"), "/**cde");
}

/**
 * @tc.name: PathControlTest010
 * @tc.desc: ShieldPath masks each segment independently across a multi-segment
 *           path, with 1..6 char tokens and empty paths covered
 * @tc.precon: NA
 * @tc.step: 1. Call ShieldPath with "/a/bb/ccc"
 *           2. Call ShieldPath with single tokens of length 1..6
 *           3. Call ShieldPath with "/" and ""
 * @tc.expect: "/a/bb/ccc" -> "/" + "a" + "/" + "*b" + "/" + "*cc"; every
 *             token keeps its second half and masks the first; "/" and ""
 *             both map to ""
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest010, TestSize.Level1)
{
    std::string res = PathControl::ShieldPath("/a/bb/ccc");
    EXPECT_EQ(res, "/a/*b/*cc");
    EXPECT_EQ(PathControl::ShieldPath("/a"), "/a");
    EXPECT_EQ(PathControl::ShieldPath("/ab"), "/*b");
    EXPECT_EQ(PathControl::ShieldPath("/abcde"), "/**cde");
    EXPECT_EQ(PathControl::ShieldPath("/abcdef"), "/***def");
    EXPECT_EQ(PathControl::ShieldPath("/"), "");
    EXPECT_EQ(PathControl::ShieldPath(""), "");
}

/**
 * @tc.name: PathControlTest011
 * @tc.desc: ShieldPath collapses consecutive slashes: empty tokens are dropped
 *           and every kept token is prefixed with exactly one '/', also at
 *           leading, trailing and all-slash positions
 * @tc.precon: NA
 * @tc.step: 1. Call ShieldPath with "//ab//c" and with a trailing-slash path
 *           2. Call ShieldPath with mixed empty tokens and an all-slash path
 * @tc.expect: "//ab//c" -> "/" + "*b" + "/" + "c", "/ab/" -> "/" + "*b",
 *             "//x///yz" -> "/" + "x" + "/" + "*z", "/a//b//c" -> "/" +
 *             "a" + "/" + "b" + "/" + "c", "///" -> ""
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest011, TestSize.Level1)
{
    EXPECT_EQ(PathControl::ShieldPath("//ab//c"), "/*b/c");
    EXPECT_EQ(PathControl::ShieldPath("/ab/"), "/*b");
    EXPECT_EQ(PathControl::ShieldPath("//x///yz"), "/x/*z");
    EXPECT_EQ(PathControl::ShieldPath("/a//b//c"), "/a/b/c");
    EXPECT_EQ(PathControl::ShieldPath("///"), "");
}

/**
 * @tc.name: PathControlTest012
 * @tc.desc: AddPathsToMap and SubPathsToMap reject empty and relative paths
 * @tc.precon: NA
 * @tc.step: 1. Call AddPathsToMap with "" and "relative/path"
 *           2. Call SubPathsToMap with "" and "relative/path"
 * @tc.expect: All four calls return false
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest012, TestSize.Level1)
{
    EXPECT_FALSE(PathControl::AddPathsToMap(""));
    EXPECT_FALSE(PathControl::AddPathsToMap("relative/path"));
    EXPECT_FALSE(PathControl::SubPathsToMap(""));
    EXPECT_FALSE(PathControl::SubPathsToMap("relative/path"));
}

/**
 * @tc.name: PathControlTest013
 * @tc.desc: AddPathsToMap and SubPathsToMap reject absolute well-formed paths
 *           that are outside the app base dirs: sibling areas el3/el4, bare
 *           prefix directories and system paths
 * @tc.precon: NA
 * @tc.step: 1. Call AddPathsToMap with "/data/app/el1/base/x" and
 *              "/data/storage/el4/base/y"
 *           2. Call AddPathsToMap with bare prefixes "/data" and
 *              "/data/storage"
 *           3. Call SubPathsToMap with "/etc" and "/data/storage/el3/base/x"
 * @tc.expect: All calls return false because no segment passes the app-base
 *             filter, and no ACL syscall is attempted
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest013, TestSize.Level1)
{
    EXPECT_FALSE(PathControl::AddPathsToMap("/data/app/el1/base/x"));
    EXPECT_FALSE(PathControl::AddPathsToMap("/data/storage/el4/base/y"));
    EXPECT_FALSE(PathControl::AddPathsToMap("/data"));
    EXPECT_FALSE(PathControl::AddPathsToMap("/data/storage"));
    EXPECT_FALSE(PathControl::SubPathsToMap("/data/storage/el3/base/x"));
    EXPECT_FALSE(PathControl::SubPathsToMap("/etc"));
}

/**
 * @tc.name: PathControlTest014
 * @tc.desc: InsureMapAcl is a safe no-op on an empty map; paths rejected by
 *           the app-base filter never enter the map, so repeated calls stay
 *           no-ops and the app-base contract still holds
 * @tc.precon: NA
 * @tc.step: 1. Call InsureMapAcl on the empty map
 *           2. Feed Add/SubPathsToMap paths that fail the app-base filter
 *           3. Call InsureMapAcl again and check the app-base contract
 * @tc.expect: Rejected calls return false; InsureMapAcl completes without
 *             crashing both times; CheckBelongAppBaseDir accepts the el1 base
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PathControlTest, PathControlTest014, TestSize.Level1)
{
    PathControl::InsureMapAcl();
    EXPECT_FALSE(PathControl::AddPathsToMap("/data/app/el1/base/x"));
    EXPECT_FALSE(PathControl::AddPathsToMap("relative/path"));
    EXPECT_FALSE(PathControl::SubPathsToMap("/data/storage/el3/base/x"));
    PathControl::InsureMapAcl();
    ASSERT_TRUE(PathControl::CheckBelongAppBaseDir("/data/storage/el1/base"));
}
