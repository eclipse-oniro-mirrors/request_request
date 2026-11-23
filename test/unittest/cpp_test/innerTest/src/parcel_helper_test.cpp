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
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "log.h"
#include "message_parcel.h"
#include "parcel_helper.h"
#include "request_common.h"
#include "request_common_utils.h"

using namespace testing::ext;
using namespace OHOS::Request;
using OHOS::MessageParcel;

namespace {
// Poison count used to trip the size-vs-readable-bytes guards of every
// unmarshal stage; it exceeds any parcel tail the tests write.
constexpr uint32_t OVERSIZE_COUNT = 0xFFFFFFF0;
// Entry counts for the roundtrip parcels.
constexpr uint32_t PAIR_COUNT = 2;
constexpr uint32_t STATE_COUNT = 2;
// Scalar fixtures pinned by the roundtrip assertions.
constexpr uint32_t TRIES_VALUE = 3;
constexpr uint32_t PRIORITY_VALUE = 5;
constexpr uint32_t INDEX_VALUE = 7;
constexpr uint32_t RESPONSE_CODE_VALUE = 206;
constexpr uint64_t CTIME_VALUE = 1000;
constexpr uint64_t MTIME_VALUE = 2000;
constexpr uint64_t PROCESSED_VALUE = 4096;
constexpr uint64_t TOTAL_PROCESSED_VALUE = 8192;
constexpr int64_t MIN_SPEED_VALUE = 1024;
constexpr int64_t MIN_SPEED_DURATION_VALUE = 30;
const std::string TEST_UID = "2000";
const std::string TEST_BUNDLE = "com.example.itest";
const std::string TEST_URL = "https://127.0.0.1/data.txt";
const std::string TEST_TID = "tid-001";
const std::string TEST_TITLE = "task title";
const std::string TEST_MIME = "application/json";
const std::string TEST_DATA = "payload";
const std::string TEST_DESCRIPTION = "desc";
const std::string TEST_KEY = "key";
const std::string TEST_VALUE = "value";
const std::string TEST_FORM_NAME = "formName";
const std::string TEST_FORM_VALUE = "formValue";
const std::string TEST_FILE_NAME = "file.bin";
const std::string TEST_FILE_URI = "file://itest/local.bin";
const std::string TEST_FILE_FILENAME = "local.bin";
const std::string TEST_FILE_TYPE = "application/octet-stream";
const std::string TEST_STATE_PATH = "/data/storage/el2/base/heap/file";
const std::string TEST_STATE_MESSAGE = "state message";

// Writes the scalar head of the TaskInfo layout exactly as the service side
// serializes it; code selects the fault-reason branch of UnMarshalBase.
void WriteTaskInfoBase(MessageParcel &parcel, Reason code)
{
    parcel.WriteBool(false);
    parcel.WriteBool(true);
    parcel.WriteUint32(static_cast<uint32_t>(Action::DOWNLOAD));
    parcel.WriteUint32(static_cast<uint32_t>(Mode::BACKGROUND));
    parcel.WriteUint32(static_cast<uint32_t>(code));
    parcel.WriteUint32(TRIES_VALUE);
    parcel.WriteString(TEST_UID);
    parcel.WriteString(TEST_BUNDLE);
    parcel.WriteString(TEST_URL);
    parcel.WriteString(TEST_TID);
    parcel.WriteString(TEST_TITLE);
    parcel.WriteString(TEST_MIME);
    parcel.WriteUint64(CTIME_VALUE);
    parcel.WriteUint64(MTIME_VALUE);
    parcel.WriteString(TEST_DATA);
    parcel.WriteString(TEST_DESCRIPTION);
    parcel.WriteUint32(PRIORITY_VALUE);
}

// Writes a valid forms stage with PAIR_COUNT entries.
void WriteFormStage(MessageParcel &parcel)
{
    parcel.WriteUint32(PAIR_COUNT);
    for (uint32_t i = 0; i < PAIR_COUNT; i++) {
        parcel.WriteString(TEST_FORM_NAME);
        parcel.WriteString(TEST_FORM_VALUE);
    }
}

// Writes a valid files stage with PAIR_COUNT entries.
void WriteFileStage(MessageParcel &parcel)
{
    parcel.WriteUint32(PAIR_COUNT);
    for (uint32_t i = 0; i < PAIR_COUNT; i++) {
        parcel.WriteString(TEST_FILE_NAME);
        parcel.WriteString(TEST_FILE_URI);
        parcel.WriteString(TEST_FILE_FILENAME);
        parcel.WriteString(TEST_FILE_TYPE);
    }
}

// Writes the progress stage (scalar + sizes vector, no size guard).
void WriteProgressStage(MessageParcel &parcel)
{
    parcel.WriteUint32(static_cast<uint32_t>(State::RUNNING));
    parcel.WriteUint32(INDEX_VALUE);
    parcel.WriteUint64(PROCESSED_VALUE);
    parcel.WriteUint64(TOTAL_PROCESSED_VALUE);
    std::vector<int64_t> sizes = { MIN_SPEED_VALUE };
    parcel.WriteInt64Vector(sizes);
}

// Writes a valid key-value map stage with PAIR_COUNT entries. The first key
// stays TEST_KEY (value asserts use it) and later keys get a numeric suffix,
// because a map stage is read into std::map and duplicate keys would
// collapse, breaking size assertions.
void WriteMapStage(MessageParcel &parcel)
{
    parcel.WriteUint32(PAIR_COUNT);
    for (uint32_t i = 0; i < PAIR_COUNT; i++) {
        parcel.WriteString(i == 0 ? TEST_KEY : TEST_KEY + std::to_string(i));
        parcel.WriteString(TEST_VALUE);
    }
}

// Writes a valid task-state stage with STATE_COUNT entries.
void WriteTaskStateStage(MessageParcel &parcel)
{
    parcel.WriteUint32(STATE_COUNT);
    for (uint32_t i = 0; i < STATE_COUNT; i++) {
        parcel.WriteString(TEST_STATE_PATH);
        parcel.WriteUint32(RESPONSE_CODE_VALUE);
        parcel.WriteString(TEST_STATE_MESSAGE);
    }
}

// Writes the scalar head of the Config layout (through method).
void WriteConfigScalars(MessageParcel &parcel)
{
    parcel.WriteUint32(static_cast<uint32_t>(Action::UPLOAD));
    parcel.WriteUint32(static_cast<uint32_t>(Mode::FOREGROUND));
    parcel.WriteUint32(0);
    parcel.WriteBool(true);
    parcel.WriteUint32(static_cast<uint32_t>(Network::WIFI));
    parcel.WriteBool(false);
    parcel.WriteBool(false);
    parcel.WriteBool(true);
    parcel.WriteBool(true);
    parcel.WriteUint32(INDEX_VALUE);
    parcel.WriteInt64(MIN_SPEED_VALUE);
    parcel.WriteInt64(MIN_SPEED_DURATION_VALUE);
    parcel.WriteBool(true);
    parcel.WriteBool(false);
    parcel.WriteUint32(PRIORITY_VALUE);
    parcel.WriteBool(false);
    parcel.WriteBool(true);
    parcel.WriteString(TEST_BUNDLE);
    parcel.WriteString(TEST_URL);
    parcel.WriteString(TEST_TITLE);
    parcel.WriteString(TEST_DESCRIPTION);
    parcel.WriteString("POST");
}
// Restores-check for the Roundtrip001 TaskInfo. The size guards live inline in
// the test body so a wrong-sized vector fails with the marshalling context.
void ExpectTaskInfoRestored(const TaskInfo &info)
{
    EXPECT_FALSE(info.gauge);
    EXPECT_TRUE(info.retry);
    EXPECT_EQ(info.action, Action::DOWNLOAD);
    EXPECT_EQ(info.mode, Mode::BACKGROUND);
    EXPECT_EQ(info.code, Reason::REASON_OK);
    EXPECT_EQ(info.tries, TRIES_VALUE);
    EXPECT_EQ(info.uid, TEST_UID);
    EXPECT_EQ(info.bundle, TEST_BUNDLE);
    EXPECT_EQ(info.url, TEST_URL);
    EXPECT_EQ(info.tid, TEST_TID);
    EXPECT_EQ(info.title, TEST_TITLE);
    EXPECT_EQ(info.mimeType, TEST_MIME);
    EXPECT_EQ(info.ctime, CTIME_VALUE);
    EXPECT_EQ(info.mtime, MTIME_VALUE);
    EXPECT_EQ(info.data, TEST_DATA);
    EXPECT_EQ(info.description, TEST_DESCRIPTION);
    EXPECT_EQ(info.priority, PRIORITY_VALUE);
    EXPECT_EQ(info.forms[0].name, TEST_FORM_NAME);
    EXPECT_EQ(info.forms[1].value, TEST_FORM_VALUE);
    EXPECT_EQ(info.files[1].uri, TEST_FILE_URI);
    EXPECT_EQ(info.files[1].filename, TEST_FILE_FILENAME);
    EXPECT_EQ(info.files[1].type, TEST_FILE_TYPE);
    EXPECT_EQ(info.progress.state, State::RUNNING);
    EXPECT_EQ(info.progress.index, INDEX_VALUE);
    EXPECT_EQ(info.progress.processed, PROCESSED_VALUE);
    EXPECT_EQ(info.progress.totalProcessed, TOTAL_PROCESSED_VALUE);
    EXPECT_EQ(info.progress.sizes[0], MIN_SPEED_VALUE);
    EXPECT_EQ(info.progress.extras.at(TEST_KEY), TEST_VALUE);
    EXPECT_EQ(info.extras.at(TEST_KEY), TEST_VALUE);
    EXPECT_EQ(info.version, Version::API10);
    EXPECT_EQ(info.taskStates[1].path, TEST_STATE_PATH);
    EXPECT_EQ(info.taskStates[1].responseCode, RESPONSE_CODE_VALUE);
    EXPECT_EQ(info.taskStates[1].message, TEST_STATE_MESSAGE);
}

// Restores-check for the ConfigRoundtrip008 Config, mirroring the write order.
// The size guards live inline in the test body so a wrong-sized vector fails
// with the marshalling context.
void ExpectConfigRestored(const Config &config)
{
    EXPECT_EQ(config.action, Action::UPLOAD);
    EXPECT_EQ(config.mode, Mode::FOREGROUND);
    EXPECT_TRUE(config.overwrite);
    EXPECT_EQ(config.network, Network::WIFI);
    EXPECT_TRUE(config.retry);
    EXPECT_TRUE(config.redirect);
    EXPECT_EQ(config.index, INDEX_VALUE);
    EXPECT_EQ(config.begins, MIN_SPEED_VALUE);
    EXPECT_EQ(config.ends, MIN_SPEED_DURATION_VALUE);
    EXPECT_TRUE(config.gauge);
    EXPECT_FALSE(config.precise);
    EXPECT_EQ(config.priority, PRIORITY_VALUE);
    EXPECT_FALSE(config.background);
    EXPECT_TRUE(config.multipart);
    EXPECT_EQ(config.bundleName, TEST_BUNDLE);
    EXPECT_EQ(config.url, TEST_URL);
    EXPECT_EQ(config.title, TEST_TITLE);
    EXPECT_EQ(config.description, TEST_DESCRIPTION);
    EXPECT_EQ(config.method, "POST");
    EXPECT_EQ(config.headers.at(TEST_KEY), TEST_VALUE);
    EXPECT_EQ(config.data, TEST_DATA);
    EXPECT_EQ(config.token, TEST_TID);
    EXPECT_EQ(config.extras.at(TEST_KEY), TEST_VALUE);
    EXPECT_EQ(config.version, Version::API10);
    EXPECT_EQ(config.forms[0].name, TEST_FORM_NAME);
    EXPECT_EQ(config.files[1].uri, TEST_FILE_URI);
    EXPECT_EQ(config.bodyFileNames[1], TEST_STATE_PATH);
    EXPECT_EQ(config.minSpeed.speed, MIN_SPEED_VALUE);
    EXPECT_EQ(config.minSpeed.duration, MIN_SPEED_DURATION_VALUE);
}
} // namespace

class ParcelHelperTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void ParcelHelperTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void ParcelHelperTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void ParcelHelperTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcase
}

void ParcelHelperTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testcase
}

/**
 * @tc.name: UnMarshalRoundtrip001
 * @tc.desc: UnMarshal rebuilds a fully populated TaskInfo from a parcel that
 *           follows the service serialization order byte for byte
 * @tc.precon: NA
 * @tc.step: 1. Write every TaskInfo stage with PAIR_COUNT entries
 *           2. Call ParcelHelper::UnMarshal
 *           3. Check every scalar, vector and map field
 * @tc.expect: All fields are restored to the written values
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalRoundtrip001, TestSize.Level1)
{
    MessageParcel parcel;
    WriteTaskInfoBase(parcel, Reason::REASON_OK);
    WriteFormStage(parcel);
    WriteFileStage(parcel);
    WriteProgressStage(parcel);
    WriteMapStage(parcel);
    WriteMapStage(parcel);
    parcel.WriteUint32(static_cast<uint32_t>(Version::API10));
    WriteTaskStateStage(parcel);

    TaskInfo info;
    ParcelHelper::UnMarshal(parcel, info);
    ASSERT_EQ(info.forms.size(), PAIR_COUNT);
    ASSERT_EQ(info.files.size(), PAIR_COUNT);
    ASSERT_EQ(info.progress.sizes.size(), static_cast<size_t>(1));
    ASSERT_EQ(info.progress.extras.size(), PAIR_COUNT);
    ASSERT_EQ(info.extras.size(), PAIR_COUNT);
    ASSERT_EQ(info.taskStates.size(), STATE_COUNT);
    ExpectTaskInfoRestored(info);
}

/**
 * @tc.name: UnMarshalFaultReason002
 * @tc.desc: a non-OK reason in the base stage derives faults and reason text
 *           through CommonUtils on unmarshal
 * @tc.precon: NA
 * @tc.step: 1. Write the base stage with code NETWORK_OFFLINE and no other
 *              stage
 *           2. Call ParcelHelper::UnMarshal
 * @tc.expect: faults becomes Faults::DISCONNECTED and reason carries the
 *             offline text
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalFaultReason002, TestSize.Level1)
{
    MessageParcel parcel;
    WriteTaskInfoBase(parcel, Reason::NETWORK_OFFLINE);

    TaskInfo info;
    ParcelHelper::UnMarshal(parcel, info);
    EXPECT_EQ(info.code, Reason::NETWORK_OFFLINE);
    EXPECT_EQ(info.faults, Faults::DISCONNECTED);
    EXPECT_EQ(info.reason, CommonUtils::GetMsgByReason(Reason::NETWORK_OFFLINE));
}

/**
 * @tc.name: UnMarshalFormSizeFail003
 * @tc.desc: an oversized form count aborts UnMarshal before any form is read
 * @tc.precon: NA
 * @tc.step: 1. Write the base stage followed by an oversized form count
 *           2. Call ParcelHelper::UnMarshal
 * @tc.expect: forms stays empty because the form stage rejected the count
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalFormSizeFail003, TestSize.Level1)
{
    MessageParcel parcel;
    WriteTaskInfoBase(parcel, Reason::REASON_OK);
    parcel.WriteUint32(OVERSIZE_COUNT);

    TaskInfo info;
    ParcelHelper::UnMarshal(parcel, info);
    EXPECT_TRUE(info.forms.empty());
}

/**
 * @tc.name: UnMarshalFileSizeFail004
 * @tc.desc: an oversized file count aborts UnMarshal after the forms stage
 * @tc.precon: NA
 * @tc.step: 1. Write the base and forms stages, then an oversized file count
 *           2. Call ParcelHelper::UnMarshal
 * @tc.expect: forms keeps its entries while files stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalFileSizeFail004, TestSize.Level1)
{
    MessageParcel parcel;
    WriteTaskInfoBase(parcel, Reason::REASON_OK);
    WriteFormStage(parcel);
    parcel.WriteUint32(OVERSIZE_COUNT);

    TaskInfo info;
    ParcelHelper::UnMarshal(parcel, info);
    ASSERT_EQ(info.forms.size(), PAIR_COUNT);
    EXPECT_TRUE(info.files.empty());
}

/**
 * @tc.name: UnMarshalProgressExtrasSizeFail005
 * @tc.desc: an oversized progress-extras count aborts UnMarshal after the
 *           progress stage
 * @tc.precon: NA
 * @tc.step: 1. Write base, forms, files and progress stages, then an
 *              oversized progress-extras count
 *           2. Call ParcelHelper::UnMarshal
 * @tc.expect: files keeps its entries while progress.extras stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalProgressExtrasSizeFail005, TestSize.Level1)
{
    MessageParcel parcel;
    WriteTaskInfoBase(parcel, Reason::REASON_OK);
    WriteFormStage(parcel);
    WriteFileStage(parcel);
    WriteProgressStage(parcel);
    parcel.WriteUint32(OVERSIZE_COUNT);

    TaskInfo info;
    ParcelHelper::UnMarshal(parcel, info);
    ASSERT_EQ(info.files.size(), PAIR_COUNT);
    EXPECT_TRUE(info.progress.extras.empty());
}

/**
 * @tc.name: UnMarshalExtrasSizeFail006
 * @tc.desc: an oversized extras count aborts UnMarshal after the
 *           progress-extras stage
 * @tc.precon: NA
 * @tc.step: 1. Write every stage through progress-extras, then an oversized
 *              extras count
 *           2. Call ParcelHelper::UnMarshal
 * @tc.expect: progress.extras keeps its entries while extras stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalExtrasSizeFail006, TestSize.Level1)
{
    MessageParcel parcel;
    WriteTaskInfoBase(parcel, Reason::REASON_OK);
    WriteFormStage(parcel);
    WriteFileStage(parcel);
    WriteProgressStage(parcel);
    WriteMapStage(parcel);
    parcel.WriteUint32(OVERSIZE_COUNT);

    TaskInfo info;
    ParcelHelper::UnMarshal(parcel, info);
    ASSERT_EQ(info.progress.extras.size(), PAIR_COUNT);
    EXPECT_TRUE(info.extras.empty());
}

/**
 * @tc.name: UnMarshalTaskStateSizeFail007
 * @tc.desc: an oversized task-state count aborts UnMarshal before the states
 *           are read while earlier stages stay complete
 * @tc.precon: NA
 * @tc.step: 1. Write every stage through version, then an oversized
 *              task-state count
 *           2. Call ParcelHelper::UnMarshal
 * @tc.expect: extras keeps its entries, version is restored and taskStates
 *             stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalTaskStateSizeFail007, TestSize.Level1)
{
    MessageParcel parcel;
    WriteTaskInfoBase(parcel, Reason::REASON_OK);
    WriteFormStage(parcel);
    WriteFileStage(parcel);
    WriteProgressStage(parcel);
    WriteMapStage(parcel);
    WriteMapStage(parcel);
    parcel.WriteUint32(static_cast<uint32_t>(Version::API10));
    parcel.WriteUint32(OVERSIZE_COUNT);

    TaskInfo info;
    ParcelHelper::UnMarshal(parcel, info);
    ASSERT_EQ(info.extras.size(), PAIR_COUNT);
    EXPECT_EQ(info.version, Version::API10);
    EXPECT_TRUE(info.taskStates.empty());
}

/**
 * @tc.name: UnMarshalConfigRoundtrip008
 * @tc.desc: UnMarshalConfig rebuilds a fully populated Config including
 *           headers, extras, forms, files, body file names and minSpeed
 * @tc.precon: NA
 * @tc.step: 1. Write every Config stage with PAIR_COUNT entries
 *           2. Call ParcelHelper::UnMarshalConfig
 *           3. Check every restored field
 * @tc.expect: All fields are restored to the written values
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalConfigRoundtrip008, TestSize.Level1)
{
    MessageParcel parcel;
    WriteConfigScalars(parcel);
    WriteMapStage(parcel);
    parcel.WriteString(TEST_DATA);
    parcel.WriteString(TEST_TID);
    WriteMapStage(parcel);
    parcel.WriteUint32(static_cast<uint32_t>(Version::API10));
    WriteFormStage(parcel);
    WriteFileStage(parcel);
    parcel.WriteUint32(STATE_COUNT);
    for (uint32_t i = 0; i < STATE_COUNT; i++) {
        parcel.WriteString(TEST_STATE_PATH);
    }
    parcel.WriteInt64(MIN_SPEED_VALUE);
    parcel.WriteInt64(MIN_SPEED_DURATION_VALUE);

    Config config;
    ParcelHelper::UnMarshalConfig(parcel, config);
    ASSERT_EQ(config.headers.size(), PAIR_COUNT);
    ASSERT_EQ(config.extras.size(), PAIR_COUNT);
    ASSERT_EQ(config.forms.size(), PAIR_COUNT);
    ASSERT_EQ(config.files.size(), PAIR_COUNT);
    ASSERT_EQ(config.bodyFileNames.size(), STATE_COUNT);
    ExpectConfigRestored(config);
}

/**
 * @tc.name: UnMarshalConfigHeaderSizeFail009
 * @tc.desc: an oversized header count fails UnMarshalConfig before the
 *           trailing fields are read
 * @tc.precon: NA
 * @tc.step: 1. Write the scalar Config head followed by an oversized header
 *              count
 *           2. Call ParcelHelper::UnMarshalConfig
 * @tc.expect: headers stays empty and the fields behind the header stage
 *             keep their defaults
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalConfigHeaderSizeFail009, TestSize.Level1)
{
    MessageParcel parcel;
    WriteConfigScalars(parcel);
    parcel.WriteUint32(OVERSIZE_COUNT);

    Config config;
    ParcelHelper::UnMarshalConfig(parcel, config);
    EXPECT_TRUE(config.headers.empty());
    // UnMarshalConfig returns on the header-stage failure without reading the
    // fields behind it, so token keeps its "null" default instead of empty.
    EXPECT_EQ(config.token, "null");
    EXPECT_TRUE(config.bodyFileNames.empty());
}

/**
 * @tc.name: UnMarshalConfigExtrasSizeFail010
 * @tc.desc: an oversized extras count fails UnMarshalConfig after the
 *           header stage succeeded
 * @tc.precon: NA
 * @tc.step: 1. Write the Config head with a valid header stage, then an
 *              oversized extras count
 *           2. Call ParcelHelper::UnMarshalConfig
 * @tc.expect: headers keeps its entries while extras and bodyFileNames stay
 *             empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalConfigExtrasSizeFail010, TestSize.Level1)
{
    MessageParcel parcel;
    WriteConfigScalars(parcel);
    WriteMapStage(parcel);
    parcel.WriteString(TEST_DATA);
    parcel.WriteString(TEST_TID);
    parcel.WriteUint32(OVERSIZE_COUNT);

    Config config;
    ParcelHelper::UnMarshalConfig(parcel, config);
    ASSERT_EQ(config.headers.size(), PAIR_COUNT);
    EXPECT_TRUE(config.extras.empty());
    EXPECT_TRUE(config.bodyFileNames.empty());
}

/**
 * @tc.name: UnMarshalConfigFormSizeFail011
 * @tc.desc: an oversized form count fails UnMarshalConfig after the extras
 *           stage succeeded
 * @tc.precon: NA
 * @tc.step: 1. Write the Config head with valid header and extras stages,
 *              then an oversized form count
 *           2. Call ParcelHelper::UnMarshalConfig
 * @tc.expect: extras keeps its entries while forms stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalConfigFormSizeFail011, TestSize.Level1)
{
    MessageParcel parcel;
    WriteConfigScalars(parcel);
    WriteMapStage(parcel);
    parcel.WriteString(TEST_DATA);
    parcel.WriteString(TEST_TID);
    WriteMapStage(parcel);
    parcel.WriteUint32(static_cast<uint32_t>(Version::API10));
    parcel.WriteUint32(OVERSIZE_COUNT);

    Config config;
    ParcelHelper::UnMarshalConfig(parcel, config);
    ASSERT_EQ(config.extras.size(), PAIR_COUNT);
    EXPECT_TRUE(config.forms.empty());
}

/**
 * @tc.name: UnMarshalConfigFileSpecSizeFail012
 * @tc.desc: an oversized file count fails UnMarshalConfig after the form
 *           stage succeeded
 * @tc.precon: NA
 * @tc.step: 1. Write the Config head with valid header, extras and form
 *              stages, then an oversized file count
 *           2. Call ParcelHelper::UnMarshalConfig
 * @tc.expect: forms keeps its entries while files stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalConfigFileSpecSizeFail012, TestSize.Level1)
{
    MessageParcel parcel;
    WriteConfigScalars(parcel);
    WriteMapStage(parcel);
    parcel.WriteString(TEST_DATA);
    parcel.WriteString(TEST_TID);
    WriteMapStage(parcel);
    parcel.WriteUint32(static_cast<uint32_t>(Version::API10));
    WriteFormStage(parcel);
    parcel.WriteUint32(OVERSIZE_COUNT);

    Config config;
    ParcelHelper::UnMarshalConfig(parcel, config);
    ASSERT_EQ(config.forms.size(), PAIR_COUNT);
    EXPECT_TRUE(config.files.empty());
}

/**
 * @tc.name: UnMarshalConfigBodyFileNameSizeFail013
 * @tc.desc: an oversized body-file-name count fails UnMarshalConfig after
 *           the file stage succeeded
 * @tc.precon: NA
 * @tc.step: 1. Write the Config head with valid header, extras, form and
 *              file stages, then an oversized body-file-name count
 *           2. Call ParcelHelper::UnMarshalConfig
 * @tc.expect: files keeps its entries while bodyFileNames stays empty
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, UnMarshalConfigBodyFileNameSizeFail013, TestSize.Level1)
{
    MessageParcel parcel;
    WriteConfigScalars(parcel);
    WriteMapStage(parcel);
    parcel.WriteString(TEST_DATA);
    parcel.WriteString(TEST_TID);
    WriteMapStage(parcel);
    parcel.WriteUint32(static_cast<uint32_t>(Version::API10));
    WriteFormStage(parcel);
    WriteFileStage(parcel);
    parcel.WriteUint32(OVERSIZE_COUNT);

    Config config;
    ParcelHelper::UnMarshalConfig(parcel, config);
    ASSERT_EQ(config.files.size(), PAIR_COUNT);
    EXPECT_TRUE(config.bodyFileNames.empty());
}

/**
 * @tc.name: GetFaultByReasonUnknown001
 * @tc.desc: reasons outside the mapping table degrade to Faults::OTHERS
 *           regardless of the value
 * @tc.precon: NA
 * @tc.step: 1. Call CommonUtils::GetFaultByReason with three out-of-table
 *              reason values near the uint32 boundaries
 * @tc.expect: All three results are Faults::OTHERS because no value is in the
 *             mapping table
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, GetFaultByReasonUnknown001, TestSize.Level1)
{
    auto unknownFirst = static_cast<Reason>(0xFFFFFFFE);
    auto unknownSecond = static_cast<Reason>(0xFFFFFFFD);
    auto unknownThird = static_cast<Reason>(0x7FFFFFFF);
    EXPECT_EQ(CommonUtils::GetFaultByReason(unknownFirst), Faults::OTHERS);
    EXPECT_EQ(CommonUtils::GetFaultByReason(unknownSecond), Faults::OTHERS);
    EXPECT_EQ(CommonUtils::GetFaultByReason(unknownThird), Faults::OTHERS);
}

/**
 * @tc.name: GetMsgByReasonUnknown002
 * @tc.desc: reasons outside the message table degrade to the unknown text
 *           regardless of the value
 * @tc.precon: NA
 * @tc.step: 1. Call CommonUtils::GetMsgByReason with three out-of-table
 *              reason values near the uint32 boundaries
 * @tc.expect: All three results are the literal unknown marker
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, GetMsgByReasonUnknown002, TestSize.Level1)
{
    auto unknownFirst = static_cast<Reason>(0xFFFFFFFE);
    auto unknownSecond = static_cast<Reason>(0xFFFFFFFD);
    auto unknownThird = static_cast<Reason>(0x7FFFFFFF);
    EXPECT_EQ(CommonUtils::GetMsgByReason(unknownFirst), "unknown");
    EXPECT_EQ(CommonUtils::GetMsgByReason(unknownSecond), "unknown");
    EXPECT_EQ(CommonUtils::GetMsgByReason(unknownThird), "unknown");
}

/**
 * @tc.name: GetFaultByReasonMappings003
 * @tc.desc: representative reason codes map to their pinned fault classes
 * @tc.precon: NA
 * @tc.step: 1. Call CommonUtils::GetFaultByReason for DNS, LOW_SPEED and
 *              CONNECT_ERROR
 *           2. Call CommonUtils::GetFaultByReason for NETWORK_OFFLINE,
 *              IO_ERROR and CONTINUOUS_TASK_TIMEOUT
 * @tc.expect: The first group results are Faults::DNS, Faults::LOW_SPEED and
 *             Faults::TCP; the second group results are Faults::DISCONNECTED,
 *             Faults::FSIO and Faults::TIMEOUT
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(ParcelHelperTest, GetFaultByReasonMappings003, TestSize.Level1)
{
    EXPECT_EQ(CommonUtils::GetFaultByReason(Reason::DNS), Faults::DNS);
    EXPECT_EQ(CommonUtils::GetFaultByReason(Reason::LOW_SPEED), Faults::LOW_SPEED);
    EXPECT_EQ(CommonUtils::GetFaultByReason(Reason::CONNECT_ERROR), Faults::TCP);
    EXPECT_EQ(CommonUtils::GetFaultByReason(Reason::NETWORK_OFFLINE), Faults::DISCONNECTED);
    EXPECT_EQ(CommonUtils::GetFaultByReason(Reason::IO_ERROR), Faults::FSIO);
    EXPECT_EQ(CommonUtils::GetFaultByReason(Reason::CONTINUOUS_TASK_TIMEOUT), Faults::TIMEOUT);
}
