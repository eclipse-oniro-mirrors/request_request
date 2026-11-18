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

#include "request_blob_serialize.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "log.h"
using namespace testing::ext;

namespace {
// Helper to build a CStringWrapper over caller-owned memory. Memory must stay
// valid until the wrapped string vector is serialized.
CStringWrapper MakeCStringWrapper(const std::string &str)
{
    CStringWrapper wrapper;
    wrapper.cStr = const_cast<char *>(str.data());
    wrapper.len = static_cast<uint32_t>(str.length());
    return wrapper;
}

// Deep-copy a decoded CFormItem/CFileSpec vector: the *BlobToC* helpers
// new[] the cStr buffers, so tests must free them to avoid leaks under ASAN.
void FreeCFormItemVec(std::vector<CFormItem> &vec)
{
    for (CFormItem &obj : vec) {
        delete[] obj.name.cStr;
        delete[] obj.value.cStr;
    }
}

void FreeCFileSpecVec(std::vector<CFileSpec> &vec)
{
    for (CFileSpec &obj : vec) {
        delete[] obj.name.cStr;
        delete[] obj.path.cStr;
        delete[] obj.fileName.cStr;
        delete[] obj.mimeType.cStr;
    }
}
} // namespace

class RequestBlobSerializeTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestBlobSerializeTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void RequestBlobSerializeTest::TearDownTestCase(void)
{
    // input testsuite teardown step，teardown after all testcases
}

void RequestBlobSerializeTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcases
    testing::UnitTest *test = testing::UnitTest::GetInstance();
    ASSERT_NE(test, nullptr);
    const testing::TestInfo *testInfo = test->current_test_info();
    ASSERT_NE(testInfo, nullptr);
    string testCaseName = string(testInfo->name());
    REQUEST_HILOGI("[SetUp] %{public}s start", testCaseName.c_str());
    GTEST_LOG_(INFO) << testCaseName.append(" start");
}

void RequestBlobSerializeTest::TearDown(void)
{
    // input testcase teardown step，teardown after each testcases
}

/**
 * @tc.name: BlobRoundTripTest_001
 * @tc.desc: Normal strings (len <= 255) round-trip through CStringToBlob/BlobToStringVec
 * @tc.precon: NA
 * @tc.step: 1. Build two strings ("abc" and a 255-byte string containing 0xF4 bytes)
 *           2. Serialize them with CStringToBlob
 *           3. Deserialize the blob with BlobToStringVec
 *           4. Compare the result with the original strings
 * @tc.expect: Round trip returns exactly the original strings
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, BlobRoundTripTest_001, TestSize.Level1)
{
    std::string shortStr = "abc";
    std::string maxStr(255, 'x');
    maxStr[0] = static_cast<char>(0xF4); // highest possible length header value in content
    maxStr[1] = static_cast<char>(0x8F);
    maxStr[2] = static_cast<char>(0xBF);

    std::vector<CStringWrapper> wrappers;
    wrappers.push_back(MakeCStringWrapper(shortStr));
    wrappers.push_back(MakeCStringWrapper(maxStr));

    std::vector<uint8_t> blob = CStringToBlob(wrappers.data(), wrappers.size());
    ASSERT_EQ(blob.size(), static_cast<size_t>(1 + shortStr.length() + 1 + maxStr.length()));

    std::vector<std::string> result = BlobToStringVec(blob);
    ASSERT_EQ(result.size(), wrappers.size());
    EXPECT_EQ(result[0], shortStr);
    EXPECT_EQ(result[1], maxStr);
}

/**
 * @tc.name: CStringToBlobRejectOverlongTest_001
 * @tc.desc: CStringToBlob rejects strings longer than UINT8_MAX instead of truncating the length header
 * @tc.precon: NA
 * @tc.step: 1. Build a 365-byte string (length header would truncate to 109 without the fix)
 *           2. Serialize it with CStringToBlob
 *           3. Verify an empty blob is returned (write side rejection)
 * @tc.expect: CStringToBlob returns an empty blob for any string longer than 255 bytes
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, CStringToBlobRejectOverlongTest_001, TestSize.Level1)
{
    // 365 = 0x16D: the 1-byte header would silently record 0x6D (109) and
    // desynchronize every following length header — the reported bug.
    std::string overlong(365, 'a');
    std::vector<CStringWrapper> wrappers;
    wrappers.push_back(MakeCStringWrapper(overlong));

    std::vector<uint8_t> blob = CStringToBlob(wrappers.data(), wrappers.size());
    EXPECT_TRUE(blob.empty());

    // Boundary: exactly 256 is also rejected (UINT8_MAX + 1).
    std::string boundary(UINT8_MAX + 1, 'a');
    wrappers.clear();
    wrappers.push_back(MakeCStringWrapper(boundary));
    blob = CStringToBlob(wrappers.data(), wrappers.size());
    EXPECT_TRUE(blob.empty());
}

/**
 * @tc.name: CStringToBlobRejectMixedTest_001
 * @tc.desc: CStringToBlob rejects the whole vector when any entry exceeds the length header capacity
 * @tc.precon: NA
 * @tc.step: 1. Build a vector with a valid short string followed by a 4096-byte string
 *           2. Serialize with CStringToBlob
 *           3. Verify an empty blob is returned
 * @tc.expect: No partially-written (desynchronized) blob is ever produced
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, CStringToBlobRejectMixedTest_001, TestSize.Level1)
{
    // Strings must outlive the CStringWrapper (wrappers only hold pointers).
    std::string valid = "ok";
    std::string overlong(4096, 'b');
    std::vector<CStringWrapper> wrappers;
    wrappers.push_back(MakeCStringWrapper(valid));
    wrappers.push_back(MakeCStringWrapper(overlong));

    std::vector<uint8_t> blob = CStringToBlob(wrappers.data(), wrappers.size());
    EXPECT_TRUE(blob.empty());
}

/**
 * @tc.name: BlobToStringVecTruncatedTest_001
 * @tc.desc: BlobToStringVec stops at a length header that exceeds the remaining blob instead of reading out of bounds
 * @tc.precon: NA
 * @tc.step: 1. Hand-craft a blob {0xFF} + 10 content bytes (header claims 255, only 10 remain)
 *           2. Deserialize with BlobToStringVec
 *           3. Verify an empty vector is returned without crash or exception
 * @tc.expect: Corrupted blob yields an empty result, never an out-of-bounds read
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, BlobToStringVecTruncatedTest_001, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    blob.push_back(0xFF); // claims 255 bytes of content
    for (int i = 0; i < 10; ++i) { // only 10 bytes remain
        blob.push_back(static_cast<uint8_t>('c'));
    }

    std::vector<std::string> result = BlobToStringVec(blob);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: BlobToStringVecKeepValidPrefixTest_001
 * @tc.desc: BlobToStringVec keeps already-decoded strings when a later header is corrupted
 * @tc.precon: NA
 * @tc.step: 1. Hand-craft a blob encoding "abc" followed by a header claiming 200 bytes with 3 remaining
 *           2. Deserialize with BlobToStringVec
 *           3. Verify the first string is preserved and the corrupted tail is dropped
 * @tc.expect: Partial (prefix) result is returned for partially corrupted blobs
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, BlobToStringVecKeepValidPrefixTest_001, TestSize.Level1)
{
    std::vector<uint8_t> blob;
    blob.push_back(0x03);
    blob.push_back('a');
    blob.push_back('b');
    blob.push_back('c');
    blob.push_back(200); // claims 200 bytes, but nothing valid remains
    blob.push_back('x');
    blob.push_back('y');
    blob.push_back('z');

    std::vector<std::string> result = BlobToStringVec(blob);
    ASSERT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_EQ(result[0], "abc");
}

/**
 * @tc.name: BlobToStringVecMalformedInputTest_001
 * @tc.desc: BlobToStringVec is safe for degenerate and malformed blobs
 * @tc.precon: NA
 * @tc.step: 1. Feed the empty blob, {0x05} (header without content) and {0x00, 0xFF}
 *           2. Verify each input yields a safe result without crashing
 * @tc.expect: Empty blob gives empty vector; {0x05} and {0x00, 0xFF} never read out of bounds
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, BlobToStringVecMalformedInputTest_001, TestSize.Level1)
{
    std::vector<std::string> result = BlobToStringVec({});
    EXPECT_TRUE(result.empty());

    result = BlobToStringVec({0x05}); // header claims 5 bytes, blob has none
    EXPECT_TRUE(result.empty());

    result = BlobToStringVec({0x00, 0xFF}); // valid empty string, then truncated header
    ASSERT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_TRUE(result[0].empty());
}

/**
 * @tc.name: FormItemRoundTripTest_001
 * @tc.desc: CFormItem blobs round-trip through CFormItemToBlob/BlobToCFormItem
 * @tc.precon: NA
 * @tc.step: 1. Build a CFormItem with name/value strings
 *           2. Serialize with CFormItemToBlob and deserialize with BlobToCFormItem
 *           3. Compare decoded strings with the originals, then free decoded buffers
 * @tc.expect: Round trip preserves name and value
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FormItemRoundTripTest_001, TestSize.Level1)
{
    std::string name = "field1";
    std::string value = "value1";
    CFormItem item;
    item.name = MakeCStringWrapper(name);
    item.value = MakeCStringWrapper(value);

    std::vector<uint8_t> blob = CFormItemToBlob(&item, 1);
    ASSERT_EQ(blob.size(), sizeof(CFormItem) + name.length() + value.length());

    std::vector<CFormItem> result = BlobToCFormItem(blob);
    ASSERT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(result[0].name.cStr, result[0].name.len), name);
    EXPECT_EQ(std::string(result[0].value.cStr, result[0].value.len), value);
    FreeCFormItemVec(result);
}

/**
 * @tc.name: FormItemTruncatedBlobTest_001
 * @tc.desc: BlobToCFormItem stops on truncated CFormItem blobs instead of reading out of bounds
 * @tc.precon: NA
 * @tc.step: 1. Serialize a CFormItem, then truncate the blob to a few bytes (shorter than sizeof(CFormItem))
 *           2. Feed the truncated blob to BlobToCFormItem
 *           3. Verify an empty vector is returned without crash
 * @tc.expect: Truncated blobs never read past the end of the blob buffer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FormItemTruncatedBlobTest_001, TestSize.Level1)
{
    std::string name = "n";
    std::string value = "v";
    CFormItem item;
    item.name = MakeCStringWrapper(name);
    item.value = MakeCStringWrapper(value);

    std::vector<uint8_t> blob = CFormItemToBlob(&item, 1);
    ASSERT_GT(blob.size(), static_cast<size_t>(4));
    blob.resize(4); // shorter than sizeof(CFormItem): struct itself is truncated

    std::vector<CFormItem> result = BlobToCFormItem(blob);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: FormItemHostileLengthTest_001
 * @tc.desc: BlobToCFormItem survives a hostile in-blob length header claiming huge sizes
 * @tc.precon: NA
 * @tc.step: 1. Build a blob containing one valid CFormItem struct whose name.len = 0xFFFFFF00
 *           2. Feed it to BlobToCFormItem
 *           3. Verify no out-of-bounds read happens and the result is empty
 * @tc.expect: The boundary check rejects the oversized length instead of new[]/memcpy out of bounds
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FormItemHostileLengthTest_001, TestSize.Level1)
{
    CFormItem hostile;
    hostile.name.len = 0xFFFFFF00; // claims ~4GB, blob has almost nothing
    hostile.value.len = 0;
    std::vector<uint8_t> blob(reinterpret_cast<const uint8_t *>(&hostile),
        reinterpret_cast<const uint8_t *>(&hostile) + sizeof(CFormItem));

    std::vector<CFormItem> result = BlobToCFormItem(blob);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: FormItemCumulativeOverflowTest_001
 * @tc.desc: BlobToCFormItem rejects lengths that are each plausible but together exceed the blob
 * @tc.precon: NA
 * @tc.step: 1. Build a blob with one CFormItem struct whose name.len = 15, value.len = 15
 *           2. Append only 20 content bytes (less than the claimed 30 total)
 *           3. Feed it to BlobToCFormItem and verify an empty result
 * @tc.expect: The cumulative bound check catches the sum overflow; value is never read at
 *             position + name.len beyond the blob end
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FormItemCumulativeOverflowTest_001, TestSize.Level1)
{
    CFormItem trick;
    trick.name.len = 15;
    trick.value.len = 15; // each is small, but 15 + 15 > 20 remaining bytes
    std::vector<uint8_t> blob(reinterpret_cast<const uint8_t *>(&trick),
        reinterpret_cast<const uint8_t *>(&trick) + sizeof(CFormItem));
    blob.insert(blob.end(), 20, 'x');

    std::vector<CFormItem> result = BlobToCFormItem(blob);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: FileSpecCumulativeOverflowTest_001
 * @tc.desc: BlobToCFileSpec rejects per-field-valid lengths whose sum exceeds the blob
 * @tc.precon: NA
 * @tc.step: 1. Build a blob with one CFileSpec struct whose four lengths are 8/8/8/8
 *           2. Append 20 content bytes (sum of lengths + flag byte needs 33) without the flag byte
 *           3. Feed it to BlobToCFileSpec and verify an empty result
 * @tc.expect: The cumulative check (including the trailing flag byte) never reads out of bounds
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FileSpecCumulativeOverflowTest_001, TestSize.Level1)
{
    CFileSpec trick;
    trick.name.len = 8;
    trick.path.len = 8;
    trick.fileName.len = 8;
    trick.mimeType.len = 8; // 32 bytes of strings + 1 flag byte, but only 20 remain
    std::vector<uint8_t> blob(reinterpret_cast<const uint8_t *>(&trick),
        reinterpret_cast<const uint8_t *>(&trick) + sizeof(CFileSpec));
    blob.insert(blob.end(), 20, 'x');

    std::vector<CFileSpec> result = BlobToCFileSpec(blob);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: FileSpecMissingFlagByteTest_001
 * @tc.desc: BlobToCFileSpec rejects a blob whose strings fit but leave no room for the flag byte
 * @tc.precon: NA
 * @tc.step: 1. Build a blob with one CFileSpec struct whose four lengths are all 0
 *           2. Append no content at all, so the mandatory trailing flag byte is missing
 *           3. Feed it to BlobToCFileSpec and verify an empty result
 * @tc.expect: The flag-byte bound check prevents a 1-byte out-of-bounds read at the blob end
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FileSpecMissingFlagByteTest_001, TestSize.Level1)
{
    CFileSpec exact;
    exact.name.len = 0;
    exact.path.len = 0;
    exact.fileName.len = 0;
    exact.mimeType.len = 0; // zero-length strings, but the flag byte is still required
    std::vector<uint8_t> blob(reinterpret_cast<const uint8_t *>(&exact),
        reinterpret_cast<const uint8_t *>(&exact) + sizeof(CFileSpec));

    std::vector<CFileSpec> result = BlobToCFileSpec(blob);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: FileSpecRoundTripTest_001
 * @tc.desc: CFileSpec blobs round-trip through CFileSpecToBlob/BlobToCFileSpec
 * @tc.precon: NA
 * @tc.step: 1. Build a CFileSpec with name/path/fileName/mimeType and a flag
 *           2. Serialize and deserialize it
 *           3. Compare every decoded field with the original, then free decoded buffers
 * @tc.expect: Round trip preserves all fields including the trailing flag byte
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FileSpecRoundTripTest_001, TestSize.Level1)
{
    std::string name = "spec";
    std::string path = "/data/storage/el2/base/files/dl.txt";
    std::string fileName = "dl.txt";
    std::string mimeType = "text/plain";
    CFileSpec spec;
    spec.name = MakeCStringWrapper(name);
    spec.path = MakeCStringWrapper(path);
    spec.fileName = MakeCStringWrapper(fileName);
    spec.mimeType = MakeCStringWrapper(mimeType);
    spec.is_user_file = true;

    std::vector<uint8_t> blob = CFileSpecToBlob(&spec, 1);
    ASSERT_EQ(blob.size(),
        sizeof(CFileSpec) + name.length() + path.length() + fileName.length() + mimeType.length() + 1);

    std::vector<CFileSpec> result = BlobToCFileSpec(blob);
    ASSERT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(result[0].name.cStr, result[0].name.len), name);
    EXPECT_EQ(std::string(result[0].path.cStr, result[0].path.len), path);
    EXPECT_EQ(std::string(result[0].fileName.cStr, result[0].fileName.len), fileName);
    EXPECT_EQ(std::string(result[0].mimeType.cStr, result[0].mimeType.len), mimeType);
    EXPECT_TRUE(result[0].is_user_file);
    FreeCFileSpecVec(result);
}

/**
 * @tc.name: FileSpecTruncatedBlobTest_001
 * @tc.desc: BlobToCFileSpec stops on truncated blobs instead of reading out of bounds
 * @tc.precon: NA
 * @tc.step: 1. Serialize a CFileSpec, then truncate the blob to a few bytes
 *           2. Feed the truncated blob to BlobToCFileSpec
 *           3. Verify an empty vector is returned without crash
 * @tc.expect: Truncated blobs never read past the end of the blob buffer
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, FileSpecTruncatedBlobTest_001, TestSize.Level1)
{
    std::string name = "n";
    std::string empty;
    CFileSpec spec;
    spec.name = MakeCStringWrapper(name);
    spec.path = MakeCStringWrapper(empty);
    spec.fileName = MakeCStringWrapper(empty);
    spec.mimeType = MakeCStringWrapper(empty);
    spec.is_user_file = false;

    std::vector<uint8_t> blob = CFileSpecToBlob(&spec, 1);
    ASSERT_GT(blob.size(), static_cast<size_t>(4));
    blob.resize(4); // shorter than sizeof(CFileSpec)

    std::vector<CFileSpec> result = BlobToCFileSpec(blob);
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: CStringToBlobEmptyVecTest_001
 * @tc.desc: CStringToBlob accepts empty and null inputs
 * @tc.precon: NA
 * @tc.step: 1. Serialize a zero-length vector and a null pointer
 *           2. Verify both produce an empty blob
 * @tc.expect: Empty inputs produce empty blobs; empty string lists are unchanged by the fix
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestBlobSerializeTest, CStringToBlobEmptyVecTest_001, TestSize.Level1)
{
    std::vector<uint8_t> blob = CStringToBlob(nullptr, 0);
    EXPECT_TRUE(blob.empty());

    std::vector<CStringWrapper> emptyVec;
    blob = CStringToBlob(emptyVec.data(), 0);
    EXPECT_TRUE(blob.empty());

    // A single empty string is still representable: header 0x00, no content.
    std::string empty;
    std::vector<CStringWrapper> wrappers;
    wrappers.push_back(MakeCStringWrapper(empty));
    blob = CStringToBlob(wrappers.data(), wrappers.size());
    ASSERT_EQ(blob.size(), static_cast<size_t>(1));
    std::vector<std::string> result = BlobToStringVec(blob);
    ASSERT_EQ(result.size(), static_cast<size_t>(1));
    EXPECT_TRUE(result[0].empty());
}
