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

#include <cstddef>
#include <string>

#include <gtest/gtest.h>

#include "request_utils_wrapper.h"

using namespace testing::ext;
using namespace OHOS::Request;

namespace {
// SHA256 digest length in characters: 32 bytes -> 64 lowercase hex digits.
constexpr size_t SHA256_HEX_LENGTH = 64;

// Standard SHA256 test vectors (NIST / FIPS 180 style corpus).
constexpr const char *SHA256_EMPTY = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
constexpr const char *SHA256_ABC = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
constexpr const char *SHA256_FOX = "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592";
constexpr const char *SHA256_FOX_INPUT = "The quick brown fox jumps over the lazy dog";
constexpr const char *SHA256_NIST_INPUT = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
constexpr const char *SHA256_NIST = "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1";

// Number of binary bytes fed to the format probe below.
constexpr size_t FORMAT_PROBE_BYTES = 256;

// Letter-cycle step keeping the probe valid UTF-8 (rust::str guarantees it).
constexpr size_t FORMAT_PROBE_MODULUS = 26;

// First letter of the probe alphabet.
constexpr char FORMAT_PROBE_FIRST_LETTER = 'a';

bool IsLowercaseHex64(const std::string &digest)
{
    if (digest.size() != SHA256_HEX_LENGTH) {
        return false;
    }
    return digest.find_first_not_of("0123456789abcdef") == std::string::npos;
}
} // namespace

class RequestUtilsWrapperTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestUtilsWrapperTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RequestUtilsWrapperTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void RequestUtilsWrapperTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testCases
}

void RequestUtilsWrapperTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testCases
}

/**
 * @tc.name: Sha256GoldenVectorsTest
 * @tc.desc: Test SHA256 against the standard empty/abc/quick-fox vectors and
 *           the 56-byte NIST vector
 * @tc.precon: NA
 * @tc.step: 1. Hash the empty string and compare with the standard vector
 *           2. Hash "abc" and compare with the standard vector
 *           3. Hash the quick-brown-fox sample and compare with the vector
 *           4. Hash the 56-byte NIST message and compare with the vector
 * @tc.expect: All four digests match the standard SHA256 vectors
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestUtilsWrapperTest, Sha256GoldenVectorsTest, TestSize.Level1)
{
    EXPECT_EQ(static_cast<std::string>(SHA256(rust::str(""))), std::string(SHA256_EMPTY));
    EXPECT_EQ(static_cast<std::string>(SHA256(rust::str("abc"))), std::string(SHA256_ABC));
    EXPECT_EQ(static_cast<std::string>(SHA256(rust::str(std::string(SHA256_FOX_INPUT)))),
        std::string(SHA256_FOX));
    const std::string nistInput = SHA256_NIST_INPUT;
    EXPECT_EQ(static_cast<std::string>(SHA256(rust::str(nistInput))), std::string(SHA256_NIST));
}

/**
 * @tc.name: Sha256DigestFormatTest
 * @tc.desc: Test the SHA256 output format over a multi-letter ASCII input
 * @tc.precon: NA
 * @tc.step: 1. Build a 256-byte ASCII input cycling through 'a'..'z'
 *           2. Hash it and check the digest is 64 lowercase hex characters
 * @tc.expect: The digest has exactly 64 lowercase hexadecimal characters
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestUtilsWrapperTest, Sha256DigestFormatTest, TestSize.Level1)
{
    std::string probe;
    probe.reserve(FORMAT_PROBE_BYTES);
    for (size_t i = 0; i < FORMAT_PROBE_BYTES; ++i) {
        probe.push_back(static_cast<char>(FORMAT_PROBE_FIRST_LETTER + (i % FORMAT_PROBE_MODULUS)));
    }
    std::string digest = static_cast<std::string>(SHA256(rust::str(probe)));
    EXPECT_TRUE(IsLowercaseHex64(digest));
}

/**
 * @tc.name: IsStageContextNullEnvTest
 * @tc.desc: Test IsStageContext with a null ani env and null object slots at
 *           two different slot addresses
 * @tc.precon: The SDK ani_base_context stub reports stage mode without
 *           touching the env or the object
 * @tc.step: 1. Build two AniObject wrappers around distinct null slots
 *           2. Call IsStageContext with a null env and the first slot
 *           3. Call IsStageContext with a null env and the second slot
 * @tc.expect: Both calls return true because the stub always reports ANI_OK
 *             with stageMode set regardless of the slot address
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestUtilsWrapperTest, IsStageContextNullEnvTest, TestSize.Level1)
{
    void *slotFirst = nullptr;
    void *slotSecond = nullptr;
    AniObject *objFirst = reinterpret_cast<AniObject *>(&slotFirst);
    AniObject *objSecond = reinterpret_cast<AniObject *>(&slotSecond);
    EXPECT_TRUE(IsStageContext(nullptr, objFirst));
    EXPECT_TRUE(IsStageContext(nullptr, objSecond));
}

/**
 * @tc.name: GetStageModeContextNullEnvTest
 * @tc.desc: Test GetStageModeContext with a null ani env
 * @tc.precon: ani_base_context guards a null env before touching the object
 * @tc.step: 1. Build an AniEnv** slot holding a null env pointer
 *           2. Build an AniObject wrapper around a null pointer slot
 *           3. Call GetStageModeContext with the null env slot
 * @tc.expect: GetStageModeContext returns a null context
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestUtilsWrapperTest, GetStageModeContextNullEnvTest, TestSize.Level1)
{
    AniEnv *envPtr = nullptr;
    AniEnv **envSlot = &envPtr;
    void *slot = nullptr;
    AniObject *obj = reinterpret_cast<AniObject *>(&slot);
    auto context = GetStageModeContext(envSlot, obj);
    EXPECT_EQ(context, nullptr);
}

/**
 * @tc.name: StringfyWantAgentNullEnvTest
 * @tc.desc: Test StringfyWantAgent with a null ani env
 * @tc.precon: UnwrapWantAgent bails out on a null env and leaves the agent
 *           pointer null
 * @tc.step: 1. Build an AniEnv** slot holding a null env pointer
 *           2. Build an AniObject wrapper around a null pointer slot
 *           3. Call StringfyWantAgent with the null env slot
 * @tc.expect: StringfyWantAgent returns the empty string on the null agent
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestUtilsWrapperTest, StringfyWantAgentNullEnvTest, TestSize.Level1)
{
    AniEnv *envPtr = nullptr;
    AniEnv **envSlot = &envPtr;
    void *slot = nullptr;
    AniObject *obj = reinterpret_cast<AniObject *>(&slot);
    std::string text = static_cast<std::string>(StringfyWantAgent(envSlot, obj));
    EXPECT_TRUE(text.empty());
}
