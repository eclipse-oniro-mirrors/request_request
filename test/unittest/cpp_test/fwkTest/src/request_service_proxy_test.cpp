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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "download_server_ipc_interface_code.h"
#include "gmock/gmock.h"
#include "log.h"
#include "parcel_helper.h"
#include "request_common.h"
#include "request_service_proxy.h"
#include "runcount_notify_stub.h"

using namespace testing::ext;
using namespace OHOS::Request;

#undef private
#undef protected

// Interface descriptor declared by RequestServiceInterface via
// DECLARE_INTERFACE_DESCRIPTOR("OHOS.Download.RequestServiceInterface").
static const std::u16string REQUEST_SERVICE_DESCRIPTOR = u"OHOS.Download.RequestServiceInterface";

namespace {
// Scriptable OHOS::IRemoteObject: captures the request parcel bytes of every
// SendRequest call and writes a scripted reply, so proxy methods can be
// asserted without a real SAMGR connection.
class MockRemoteObject : public OHOS::IRemoteObject {
public:
    explicit MockRemoteObject(std::u16string descriptor) : OHOS::IRemoteObject(std::move(descriptor))
    {
    }

    int SendRequest(
        uint32_t code, OHOS::MessageParcel &data, OHOS::MessageParcel &reply, OHOS::MessageOption &option) override
    {
        lastCode_ = code;
        // Parcel::GetData returns the buffer address as uintptr_t, so cast to
        // a real pointer first; otherwise assign(pointer-as-integer, ...) would
        // resolve to the (count, value) overload and treat the address as a
        // byte count.
        const uint8_t *base = reinterpret_cast<const uint8_t *>(data.GetData());
        requestBytes_.assign(base, base + data.GetDataSize());
        if (replyWriter_) {
            replyWriter_(reply);
        }
        return sendRet_;
    }

    int Dump(int fd, const std::vector<std::u16string> &args) override
    {
        return 0;
    }

    int32_t GetObjectRefCount() override
    {
        return 0;
    }

    bool AddDeathRecipient(const OHOS::sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    bool RemoveDeathRecipient(const OHOS::sptr<DeathRecipient> &recipient) override
    {
        return true;
    }

    void ScriptReply(int32_t ret, const std::function<void(OHOS::MessageParcel &)> &writer)
    {
        sendRet_ = ret;
        replyWriter_ = writer;
    }

    uint32_t LastCode() const
    {
        return lastCode_;
    }

    // Rebuilds the captured request bytes into a readable parcel by copying
    // them through the parcel's own allocator. Do NOT use ParseFrom here: the
    // Parcel destructor runs allocator_->Dealloc(data_) unconditionally, so a
    // buffer set via ParseFrom must not outlive its owner.
    bool FillRequestParcel(OHOS::MessageParcel &parcel)
    {
        if (requestBytes_.empty()) {
            return false;
        }
        return parcel.WriteBuffer(requestBytes_.data(), requestBytes_.size());
    }

private:
    std::function<void(OHOS::MessageParcel &)> replyWriter_;
    std::vector<uint8_t> requestBytes_;
    int sendRet_ = OHOS::ERR_NONE;
    uint32_t lastCode_ = 0;
};

// Distinct values of the full Config fixture: every serialized field carries
// its own value so field-order mistakes surface as mismatched assertions.
constexpr uint32_t FULL_BUNDLE_TYPE = 3;
constexpr uint32_t FULL_INDEX = 7;
constexpr int64_t FULL_BEGINS = 100;
constexpr int64_t FULL_ENDS = 999;
constexpr uint32_t FULL_PRIORITY = 42;
constexpr int64_t FULL_SPEED = 1234;
constexpr int64_t FULL_SPEED_DURATION = 56;
constexpr uint64_t FULL_CONN_TIMEOUT = 30;
constexpr uint64_t FULL_TOTAL_TIMEOUT = 600;

// Distinct scalar values of the TaskInfo stream written by WriteTaskInfoParcel.
constexpr uint32_t INFO_TRIES = 3;
constexpr uint64_t INFO_CTIME = 1111;
constexpr uint64_t INFO_MTIME = 2222;
constexpr uint32_t INFO_PRIORITY = 9;
constexpr uint32_t INFO_PROGRESS_INDEX = 2;
constexpr uint64_t INFO_PROCESSED = 300;
constexpr uint64_t INFO_TOTAL_PROCESSED = 600;
constexpr uint32_t INFO_RESPONSE_CODE = 200;
const std::vector<int64_t> INFO_SIZES = { 100, 200, 300 };

// Distinct scalar values decoded from the scripted GetTask reply.
constexpr uint32_t GET_BUNDLE_TYPE = 5;
constexpr uint32_t GET_INDEX = 4;
constexpr int64_t GET_BEGINS = 11;
constexpr int64_t GET_ENDS = 22;
constexpr uint32_t GET_PRIORITY = 33;
constexpr int64_t GET_SPEED = 77;
constexpr int64_t GET_SPEED_DURATION = 88;

// Distinct scalar values written into the demo parcel for UnMarshalConfig.
constexpr uint32_t DEMO_BUNDLE_TYPE = 2;
constexpr uint32_t DEMO_INDEX = 1;
constexpr int64_t DEMO_BEGINS = 10;
constexpr int64_t DEMO_ENDS = 20;
constexpr uint32_t DEMO_PRIORITY = 5;
constexpr int64_t DEMO_SPEED = 7;
constexpr int64_t DEMO_SPEED_DURATION = 8;

// Timestamp bounds of the Search filter.
constexpr int64_t QUERY_BEFORE = 1234;
constexpr int64_t QUERY_AFTER = 5678;

// Scalar half of the full Config fixture, up to the plain string fields.
Config MakeBaseConfig()
{
    Config config;
    config.action = Action::UPLOAD;
    config.version = Version::API10;
    config.mode = Mode::FOREGROUND;
    config.bundleType = FULL_BUNDLE_TYPE;
    config.overwrite = true;
    config.network = Network::WIFI;
    config.metered = true;
    config.roaming = false;
    config.retry = false;
    config.redirect = false;
    config.background = true;
    config.multipart = true;
    config.index = FULL_INDEX;
    config.begins = FULL_BEGINS;
    config.ends = FULL_ENDS;
    config.gauge = true;
    config.precise = true;
    config.priority = FULL_PRIORITY;
    config.minSpeed.speed = FULL_SPEED;
    config.minSpeed.duration = FULL_SPEED_DURATION;
    config.timeout.connectionTimeout = FULL_CONN_TIMEOUT;
    config.timeout.totalTimeout = FULL_TOTAL_TIMEOUT;
    config.url = "https://example.com/file";
    config.title = "test-title";
    config.method = "GET";
    config.token = "task-token";
    config.description = "test-description";
    config.data = "test-data";
    config.proxy = "http://proxy:8080";
    return config;
}

// A Config with a distinct value in every field serialized by WriteConfigData;
// vector/certificate/notification fields are appended onto MakeBaseConfig().
Config MakeFullConfig()
{
    Config config = MakeBaseConfig();
    config.certificatePins = "pin-sha256";
    config.certsPath = { "/etc/certs/a.pem", "/etc/certs/b.pem" };
    config.forms.push_back(FormItem{ .name = "formName", .value = "formValue" });
    FileSpec file;
    file.name = "fileName";
    file.uri = "file://uri";
    file.filename = "file.txt";
    file.type = "text/plain";
    file.isUserFile = false;
    config.files.push_back(file);
    config.bodyFileNames = { "body1", "body2" };
    config.headers = { { "h1", "v1" }, { "h2", "v2" } };
    config.extras = { { "extraKey", "extraValue" } };
    config.notification.title = "notify-title";
    config.notification.text = std::nullopt;
    config.notification.wantAgent = "wantAgentUri";
    config.notification.disable = true;
    config.notification.visibility = 0b10;
    config.notification.hideStopButton = true;
    return config;
}

// Writes a TaskInfo in the field order consumed by ParcelHelper::UnMarshal.
void WriteTaskInfoParcel(OHOS::MessageParcel &reply)
{
    reply.WriteBool(true);  // gauge
    reply.WriteBool(false); // retry
    reply.WriteUint32(static_cast<uint32_t>(Action::DOWNLOAD));
    reply.WriteUint32(static_cast<uint32_t>(Mode::BACKGROUND));
    reply.WriteUint32(static_cast<uint32_t>(Reason::REASON_OK));
    reply.WriteUint32(INFO_TRIES);                 // tries
    reply.WriteString("1000");                     // uid
    reply.WriteString("bundle");                   // bundle
    reply.WriteString("https://example.com/file"); // url
    reply.WriteString("task-1");                   // tid
    reply.WriteString("title");                    // title
    reply.WriteString("text/plain");               // mimeType
    reply.WriteUint64(INFO_CTIME);                 // ctime
    reply.WriteUint64(INFO_MTIME);                 // mtime
    reply.WriteString("data");                     // data
    reply.WriteString("description");              // description
    reply.WriteUint32(INFO_PRIORITY);              // priority
    reply.WriteUint32(1);                          // forms size
    reply.WriteString("formName");
    reply.WriteString("formValue");
    reply.WriteUint32(1); // files size
    reply.WriteString("fileName");
    reply.WriteString("file://uri");
    reply.WriteString("file.txt");
    reply.WriteString("text/plain");
    reply.WriteUint32(static_cast<uint32_t>(State::RUNNING)); // progress.state
    reply.WriteUint32(INFO_PROGRESS_INDEX);                   // progress.index
    reply.WriteUint64(INFO_PROCESSED);                        // progress.processed
    reply.WriteUint64(INFO_TOTAL_PROCESSED);                  // progress.totalProcessed
    reply.WriteInt64Vector(INFO_SIZES);                       // sizes
    reply.WriteUint32(1);                                     // progress extras size
    reply.WriteString("pkey");
    reply.WriteString("pvalue");
    reply.WriteUint32(1); // extras size
    reply.WriteString("ekey");
    reply.WriteString("evalue");
    reply.WriteUint32(static_cast<uint32_t>(Version::API10));
    reply.WriteUint32(1); // taskStates size
    reply.WriteString("/path/file");
    reply.WriteUint32(INFO_RESPONSE_CODE);
    reply.WriteString("ok message");
}

// Reads back the scalar section of the parcel produced from MakeFullConfig,
// in WriteConfigData wire order, up to certificatePins.
void ExpectFullConfigScalars(OHOS::MessageParcel &data)
{
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Action::UPLOAD));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Version::API10));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Mode::FOREGROUND));
    EXPECT_EQ(data.ReadUint32(), FULL_BUNDLE_TYPE);
    EXPECT_TRUE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Network::WIFI));
    EXPECT_TRUE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());
    EXPECT_TRUE(data.ReadBool());
    EXPECT_TRUE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), FULL_INDEX);
    EXPECT_EQ(data.ReadInt64(), FULL_BEGINS);
    EXPECT_EQ(data.ReadInt64(), FULL_ENDS);
    EXPECT_TRUE(data.ReadBool());
    EXPECT_TRUE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), FULL_PRIORITY);
    EXPECT_EQ(data.ReadInt64(), FULL_SPEED);
    EXPECT_EQ(data.ReadInt64(), FULL_SPEED_DURATION);
    EXPECT_EQ(data.ReadUint64(), FULL_CONN_TIMEOUT);
    EXPECT_EQ(data.ReadUint64(), FULL_TOTAL_TIMEOUT);
    EXPECT_EQ(data.ReadString(), "https://example.com/file");
    EXPECT_EQ(data.ReadString(), "test-title");
    EXPECT_EQ(data.ReadString(), "GET");
    EXPECT_EQ(data.ReadString(), "task-token");
    EXPECT_EQ(data.ReadString(), "test-description");
    EXPECT_EQ(data.ReadString(), "test-data");
    EXPECT_EQ(data.ReadString(), "http://proxy:8080");
    EXPECT_EQ(data.ReadString(), "pin-sha256");
}

// Reads back the certificates/vector/notification section of the same parcel,
// and checks nothing remains.
void ExpectFullConfigVectors(OHOS::MessageParcel &data)
{
    EXPECT_EQ(data.ReadUint32(), 2u); // certs size
    EXPECT_EQ(data.ReadString(), "/etc/certs/a.pem");
    EXPECT_EQ(data.ReadString(), "/etc/certs/b.pem");

    EXPECT_EQ(data.ReadUint32(), 1u); // forms size
    EXPECT_EQ(data.ReadString(), "formName");
    EXPECT_EQ(data.ReadString(), "formValue");

    EXPECT_EQ(data.ReadUint32(), 1u); // files size
    EXPECT_EQ(data.ReadString(), "fileName");
    EXPECT_EQ(data.ReadString(), "file://uri");
    EXPECT_EQ(data.ReadString(), "file.txt");
    EXPECT_EQ(data.ReadString(), "text/plain");
    EXPECT_FALSE(data.ReadBool());

    EXPECT_EQ(data.ReadUint32(), 2u); // bodyFileNames size
    EXPECT_EQ(data.ReadString(), "body1");
    EXPECT_EQ(data.ReadString(), "body2");

    EXPECT_EQ(data.ReadUint32(), 2u); // headers size
    EXPECT_EQ(data.ReadString(), "h1");
    EXPECT_EQ(data.ReadString(), "v1");
    EXPECT_EQ(data.ReadString(), "h2");
    EXPECT_EQ(data.ReadString(), "v2");

    EXPECT_EQ(data.ReadUint32(), 1u); // extras size
    EXPECT_EQ(data.ReadString(), "extraKey");
    EXPECT_EQ(data.ReadString(), "extraValue");

    EXPECT_TRUE(data.ReadBool());
    EXPECT_EQ(data.ReadString(), "notify-title");
    EXPECT_FALSE(data.ReadBool());
    EXPECT_TRUE(data.ReadBool());
    EXPECT_EQ(data.ReadString(), "wantAgentUri");
    EXPECT_TRUE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), 0b10);
    EXPECT_TRUE(data.ReadBool());
    EXPECT_TRUE(data.ReadBool());

    EXPECT_EQ(data.GetReadableBytes(), 0u);
}

// Reads back the parcel produced from a default Config: every field carries
// its default, vectors are zero-length and notification optionals are absent.
void ExpectDefaultConfigParcel(OHOS::MessageParcel &data)
{
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Action::DOWNLOAD));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Version::API8));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Mode::BACKGROUND));
    EXPECT_EQ(data.ReadUint32(), 0u);
    EXPECT_FALSE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Network::ANY));
    EXPECT_FALSE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());
    EXPECT_TRUE(data.ReadBool());
    EXPECT_TRUE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), 0u);
    EXPECT_EQ(data.ReadInt64(), 0);
    EXPECT_EQ(data.ReadInt64(), -1);
    EXPECT_FALSE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), 0u);
    EXPECT_EQ(data.ReadInt64(), 0);
    EXPECT_EQ(data.ReadInt64(), 0);
    EXPECT_EQ(data.ReadUint64(), 0u);
    EXPECT_EQ(data.ReadUint64(), 0u);
    EXPECT_EQ(data.ReadString(), "");
    EXPECT_EQ(data.ReadString(), "");
    EXPECT_EQ(data.ReadString(), "");
    EXPECT_EQ(data.ReadString(), "null");
    EXPECT_EQ(data.ReadString(), "");
    EXPECT_EQ(data.ReadString(), "");
    EXPECT_EQ(data.ReadString(), "");
    EXPECT_EQ(data.ReadString(), "");

    EXPECT_EQ(data.ReadUint32(), 0u); // certs
    EXPECT_EQ(data.ReadUint32(), 0u); // forms
    EXPECT_EQ(data.ReadUint32(), 0u); // files
    EXPECT_EQ(data.ReadUint32(), 0u); // bodyFileNames
    EXPECT_EQ(data.ReadUint32(), 0u); // headers
    EXPECT_EQ(data.ReadUint32(), 0u); // extras

    EXPECT_FALSE(data.ReadBool()); // notification.title absent
    EXPECT_FALSE(data.ReadBool()); // notification.text absent
    EXPECT_FALSE(data.ReadBool()); // notification.wantAgent absent
    EXPECT_FALSE(data.ReadBool()); // disable
    EXPECT_EQ(data.ReadUint32(), 0b01);
    EXPECT_TRUE(data.ReadBool());
    EXPECT_FALSE(data.ReadBool());

    EXPECT_EQ(data.GetReadableBytes(), 0u);
}

// Writes the scripted GetTask reply: per-task code then a Config stream whose
// scalar values are distinct from MakeFullConfig.
void WriteGetTaskConfigReply(OHOS::MessageParcel &reply)
{
    reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
    reply.WriteUint32(static_cast<uint32_t>(Action::UPLOAD));
    reply.WriteUint32(static_cast<uint32_t>(Mode::FOREGROUND));
    reply.WriteUint32(GET_BUNDLE_TYPE);
    reply.WriteBool(true);
    reply.WriteUint32(static_cast<uint32_t>(Network::CELLULAR));
    reply.WriteBool(true);
    reply.WriteBool(true);
    reply.WriteBool(false);
    reply.WriteBool(false);
    reply.WriteUint32(GET_INDEX);
    reply.WriteInt64(GET_BEGINS);
    reply.WriteInt64(GET_ENDS);
    reply.WriteBool(true);
    reply.WriteBool(true);
    reply.WriteUint32(GET_PRIORITY);
    reply.WriteBool(true);
    reply.WriteBool(false);
    reply.WriteString("com.get.bundle");
    reply.WriteString("https://example.com/get");
    reply.WriteString("get-title");
    reply.WriteString("get-description");
    reply.WriteString("POST");
    reply.WriteUint32(1); // headers
    reply.WriteString("gh");
    reply.WriteString("gv");
    reply.WriteString("get-data");
    reply.WriteString("get-token");
    reply.WriteUint32(1); // extras
    reply.WriteString("gx");
    reply.WriteString("gy");
    reply.WriteUint32(static_cast<uint32_t>(Version::API10));
    reply.WriteUint32(1); // forms
    reply.WriteString("gform");
    reply.WriteString("gvalue");
    reply.WriteUint32(1); // files
    reply.WriteString("gfile");
    reply.WriteString("file://guri");
    reply.WriteString("gfile.txt");
    reply.WriteString("text/plain");
    reply.WriteUint32(1); // bodyFileNames
    reply.WriteString("gbody");
    reply.WriteInt64(GET_SPEED);
    reply.WriteInt64(GET_SPEED_DURATION);
}

// Asserts every field of the Config decoded from WriteGetTaskConfigReply.
void ExpectGetTaskConfig(const Config &config)
{
    EXPECT_EQ(config.action, Action::UPLOAD);
    EXPECT_EQ(config.mode, Mode::FOREGROUND);
    EXPECT_EQ(config.bundleType, GET_BUNDLE_TYPE);
    EXPECT_TRUE(config.overwrite);
    EXPECT_EQ(config.network, Network::CELLULAR);
    EXPECT_TRUE(config.metered);
    EXPECT_TRUE(config.roaming);
    EXPECT_FALSE(config.retry);
    EXPECT_FALSE(config.redirect);
    EXPECT_EQ(config.index, GET_INDEX);
    EXPECT_EQ(config.begins, GET_BEGINS);
    EXPECT_EQ(config.ends, GET_ENDS);
    EXPECT_TRUE(config.gauge);
    EXPECT_TRUE(config.precise);
    EXPECT_EQ(config.priority, GET_PRIORITY);
    EXPECT_TRUE(config.background);
    EXPECT_FALSE(config.multipart);
    EXPECT_EQ(config.bundleName, "com.get.bundle");
    EXPECT_EQ(config.url, "https://example.com/get");
    EXPECT_EQ(config.title, "get-title");
    EXPECT_EQ(config.description, "get-description");
    EXPECT_EQ(config.method, "POST");
    EXPECT_EQ(config.headers.at("gh"), "gv");
    EXPECT_EQ(config.data, "get-data");
    EXPECT_EQ(config.token, "get-token");
    EXPECT_EQ(config.extras.at("gx"), "gy");
    EXPECT_EQ(config.version, Version::API10);
    ASSERT_EQ(config.forms.size(), 1u);
    EXPECT_EQ(config.forms[0].name, "gform");
    ASSERT_EQ(config.files.size(), 1u);
    EXPECT_EQ(config.files[0].filename, "gfile.txt");
    ASSERT_EQ(config.bodyFileNames.size(), 1u);
    EXPECT_EQ(config.bodyFileNames[0], "gbody");
    EXPECT_EQ(config.minSpeed.speed, GET_SPEED);
    EXPECT_EQ(config.minSpeed.duration, GET_SPEED_DURATION);
}

// Writes a Config stream whose scalar values are distinct from both
// MakeFullConfig and WriteGetTaskConfigReply, in the wire order consumed by
// ParcelHelper::UnMarshalConfig.
void WriteDemoConfigParcel(OHOS::MessageParcel &data)
{
    data.WriteUint32(static_cast<uint32_t>(Action::DOWNLOAD));
    data.WriteUint32(static_cast<uint32_t>(Mode::BACKGROUND));
    data.WriteUint32(DEMO_BUNDLE_TYPE);
    data.WriteBool(true);
    data.WriteUint32(static_cast<uint32_t>(Network::WIFI));
    data.WriteBool(true);
    data.WriteBool(false);
    data.WriteBool(true);
    data.WriteBool(false);
    data.WriteUint32(DEMO_INDEX);
    data.WriteInt64(DEMO_BEGINS);
    data.WriteInt64(DEMO_ENDS);
    data.WriteBool(false);
    data.WriteBool(true);
    data.WriteUint32(DEMO_PRIORITY);
    data.WriteBool(false);
    data.WriteBool(false);
    data.WriteString("com.demo");
    data.WriteString("https://example.com/demo");
    data.WriteString("demo-title");
    data.WriteString("demo-description");
    data.WriteString("GET");
    data.WriteUint32(1);
    data.WriteString("hk");
    data.WriteString("hv");
    data.WriteString("demo-data");
    data.WriteString("demo-token");
    data.WriteUint32(1);
    data.WriteString("dk");
    data.WriteString("dv");
    data.WriteUint32(static_cast<uint32_t>(Version::API9));
    data.WriteUint32(1);
    data.WriteString("dform");
    data.WriteString("dvalue");
    data.WriteUint32(1);
    data.WriteString("dfile");
    data.WriteString("file://duri");
    data.WriteString("dfile.txt");
    data.WriteString("application/json");
    data.WriteUint32(1);
    data.WriteString("dbody");
    data.WriteInt64(DEMO_SPEED);
    data.WriteInt64(DEMO_SPEED_DURATION);
}

// Asserts every field of the Config decoded from WriteDemoConfigParcel.
void ExpectDemoConfig(const Config &config)
{
    EXPECT_EQ(config.action, Action::DOWNLOAD);
    EXPECT_EQ(config.mode, Mode::BACKGROUND);
    EXPECT_EQ(config.bundleType, DEMO_BUNDLE_TYPE);
    EXPECT_TRUE(config.overwrite);
    EXPECT_EQ(config.network, Network::WIFI);
    EXPECT_TRUE(config.metered);
    EXPECT_FALSE(config.roaming);
    EXPECT_TRUE(config.retry);
    EXPECT_FALSE(config.redirect);
    EXPECT_EQ(config.index, DEMO_INDEX);
    EXPECT_EQ(config.begins, DEMO_BEGINS);
    EXPECT_EQ(config.ends, DEMO_ENDS);
    EXPECT_FALSE(config.gauge);
    EXPECT_TRUE(config.precise);
    EXPECT_EQ(config.priority, DEMO_PRIORITY);
    EXPECT_FALSE(config.background);
    EXPECT_FALSE(config.multipart);
    EXPECT_EQ(config.bundleName, "com.demo");
    EXPECT_EQ(config.url, "https://example.com/demo");
    EXPECT_EQ(config.title, "demo-title");
    EXPECT_EQ(config.description, "demo-description");
    EXPECT_EQ(config.method, "GET");
    EXPECT_EQ(config.headers.at("hk"), "hv");
    EXPECT_EQ(config.data, "demo-data");
    EXPECT_EQ(config.token, "demo-token");
    EXPECT_EQ(config.extras.at("dk"), "dv");
    EXPECT_EQ(config.version, Version::API9);
    ASSERT_EQ(config.forms.size(), 1u);
    EXPECT_EQ(config.forms[0].value, "dvalue");
    ASSERT_EQ(config.files.size(), 1u);
    EXPECT_EQ(config.files[0].uri, "file://duri");
    ASSERT_EQ(config.bodyFileNames.size(), 1u);
    EXPECT_EQ(config.bodyFileNames[0], "dbody");
    EXPECT_EQ(config.minSpeed.speed, DEMO_SPEED);
    EXPECT_EQ(config.minSpeed.duration, DEMO_SPEED_DURATION);
}
} // namespace

class RequestServiceProxyTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void RequestServiceProxyTest::SetUpTestCase(void)
{
    // input testSuit setup step，setup invoked before all testCases
}

void RequestServiceProxyTest::TearDownTestCase(void)
{
    // input testSuit teardown step，teardown invoked after all testCases
}

void RequestServiceProxyTest::SetUp(void)
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

void RequestServiceProxyTest::TearDown(void)
{
    // input testCase teardown step，teardown invoked after each testCase
}

/**
 * @tc.name: WriteConfigDataTest001
 * @tc.desc: WriteConfigData serializes a fully populated Config field by field
 *           in the create-path wire order shared with the server side
 * @tc.precon: NA
 * @tc.step: 1. Call RequestServiceProxy::WriteConfigData with a full Config
 *           2. Read the parcel back in the exact wire order
 * @tc.expect: Every field round-trips to its written value and the parcel has
 *             no leftover bytes
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, WriteConfigDataTest001, TestSize.Level1)
{
    Config config = MakeFullConfig();
    OHOS::MessageParcel data;
    RequestServiceProxy::WriteConfigData(config, data);

    ASSERT_GT(data.GetDataSize(), 0u);
    ExpectFullConfigScalars(data);
    ExpectFullConfigVectors(data);
    ASSERT_TRUE(data.RewindRead(0));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Action::UPLOAD));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Version::API10));
}

/**
 * @tc.name: WriteConfigDataTest002
 * @tc.desc: A default Config serializes with empty vectors and a notification
 *           with all optional fields absent
 * @tc.precon: NA
 * @tc.step: 1. Call WriteConfigData with a default-constructed Config
 *           2. Read the parcel back in the exact wire order
 * @tc.expect: Defaults are written and vectors are written as zero-length
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, WriteConfigDataTest002, TestSize.Level1)
{
    Config config;
    config.action = Action::DOWNLOAD;
    config.version = Version::API8;
    OHOS::MessageParcel data;
    RequestServiceProxy::WriteConfigData(config, data);

    ASSERT_GT(data.GetDataSize(), 0u);
    ExpectDefaultConfigParcel(data);
    ASSERT_TRUE(data.RewindRead(0));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Action::DOWNLOAD));
    EXPECT_EQ(data.ReadUint32(), static_cast<uint32_t>(Version::API8));
}

/**
 * @tc.name: GetVectorDataTest001
 * @tc.desc: GetVectorData writes certs/forms/files/bodyFileNames/headers/
 *           extras as sized vector blocks in that order
 * @tc.precon: NA
 * @tc.step: 1. Call GetVectorData on an empty parcel with a populated Config
 *           2. Read the blocks back
 * @tc.expect: Vector blocks are written with correct sizes and contents
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, GetVectorDataTest001, TestSize.Level1)
{
    Config config = MakeFullConfig();
    OHOS::MessageParcel data;
    RequestServiceProxy::GetVectorData(config, data);

    EXPECT_EQ(data.ReadUint32(), 2u);
    EXPECT_EQ(data.ReadString(), "/etc/certs/a.pem");
    EXPECT_EQ(data.ReadString(), "/etc/certs/b.pem");
    EXPECT_EQ(data.ReadUint32(), 1u);
    EXPECT_EQ(data.ReadString(), "formName");
    EXPECT_EQ(data.ReadString(), "formValue");
    EXPECT_EQ(data.ReadUint32(), 1u);
    EXPECT_EQ(data.ReadString(), "fileName");
    EXPECT_EQ(data.ReadString(), "file://uri");
    EXPECT_EQ(data.ReadString(), "file.txt");
    EXPECT_EQ(data.ReadString(), "text/plain");
    EXPECT_FALSE(data.ReadBool());
    EXPECT_EQ(data.ReadUint32(), 2u);
    EXPECT_EQ(data.ReadString(), "body1");
    EXPECT_EQ(data.ReadString(), "body2");
    EXPECT_EQ(data.ReadUint32(), 2u);
    EXPECT_EQ(data.ReadString(), "h1");
    EXPECT_EQ(data.ReadString(), "v1");
    EXPECT_EQ(data.ReadString(), "h2");
    EXPECT_EQ(data.ReadString(), "v2");
    EXPECT_EQ(data.ReadUint32(), 1u);
    EXPECT_EQ(data.ReadString(), "extraKey");
    EXPECT_EQ(data.ReadString(), "extraValue");
    EXPECT_EQ(data.GetReadableBytes(), 0u);
}

/**
 * @tc.name: CreateTasksTest001
 * @tc.desc: CreateTasks sends CMD_REQUEST with the interface token and task
 *           count, then parses per-task codes and stringified tids
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and two per-task results
 *           2. Call CreateTasks with two configs
 * @tc.expect: Returns E_OK; rets contain the scripted codes and tids; the
 *             request parcel starts with the interface token and count 2
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, CreateTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(11);
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_CHANNEL_NOT_OPEN));
        reply.WriteInt32(12);
    });
    RequestServiceProxy proxy(object);

    std::vector<Config> configs = { MakeFullConfig(), MakeFullConfig() };
    std::vector<TaskRet> rets;
    ExceptionErrorCode ret = proxy.CreateTasks(configs, rets);

    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 2u);
    EXPECT_EQ(rets[0].code, ExceptionErrorCode::E_OK);
    EXPECT_EQ(rets[0].tid, "11");
    EXPECT_EQ(rets[1].code, ExceptionErrorCode::E_CHANNEL_NOT_OPEN);
    EXPECT_EQ(rets[1].tid, "12");

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_REQUEST));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 2u);
}

/**
 * @tc.name: CreateTasksTest002
 * @tc.desc: CreateTasks returns E_SERVICE_ERROR when SendRequest fails
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 with an empty reply
 *           2. Call CreateTasks
 * @tc.expect: Returns E_SERVICE_ERROR
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, CreateTasksTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<Config> configs = { MakeFullConfig() };
    std::vector<TaskRet> rets;
    ExceptionErrorCode ret = proxy.CreateTasks(configs, rets);
    EXPECT_EQ(ret, ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(rets.size(), 1u);
}

/**
 * @tc.name: CreateTasksTest003
 * @tc.desc: CreateTasks propagates the top-level error code from the reply
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with top-level E_PARAMETER_CHECK
 *           2. Call CreateTasks
 * @tc.expect: Returns E_PARAMETER_CHECK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, CreateTasksTest003, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_PARAMETER_CHECK));
    });
    RequestServiceProxy proxy(object);

    std::vector<Config> configs = { MakeFullConfig() };
    std::vector<TaskRet> rets;
    ExceptionErrorCode ret = proxy.CreateTasks(configs, rets);
    EXPECT_EQ(ret, ExceptionErrorCode::E_PARAMETER_CHECK);
}

/**
 * @tc.name: StartTasksTest001
 * @tc.desc: StartTasks writes the tids in order and parses per-task results
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and two per-task codes
 *           2. Call StartTasks with two tids
 * @tc.expect: Returns E_OK; rets match the script; request parcel carries the
 *             token, count and both tids
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, StartTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OTHER));
    });
    RequestServiceProxy proxy(object);

    std::vector<std::string> tids = { "1", "2" };
    std::vector<ExceptionErrorCode> rets;
    ExceptionErrorCode ret = proxy.StartTasks(tids, rets);

    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 2u);
    EXPECT_EQ(rets[0], ExceptionErrorCode::E_OK);
    EXPECT_EQ(rets[1], ExceptionErrorCode::E_OTHER);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_START));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 2u);
    EXPECT_EQ(request.ReadString(), "1");
    EXPECT_EQ(request.ReadString(), "2");
}

/**
 * @tc.name: StartTasksTest002
 * @tc.desc: StartTasks returns E_SERVICE_ERROR when the remote died
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return REMOTE_DIED_ERROR
 *           2. Call StartTasks
 * @tc.expect: Returns E_SERVICE_ERROR
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, StartTasksTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(29189, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<std::string> tids = { "1" };
    std::vector<ExceptionErrorCode> rets;
    ExceptionErrorCode ret = proxy.StartTasks(tids, rets);
    EXPECT_EQ(ret, ExceptionErrorCode::E_SERVICE_ERROR);
}

/**
 * @tc.name: StopResumeTasksTest001
 * @tc.desc: StopTasks and ResumeTasks send their own command codes and parse
 *           per-task results
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and one per-task code for StopTasks
 *           2. Call StopTasks, check code and results
 *           3. Repeat for ResumeTasks
 * @tc.expect: CMD_STOP/CMD_RESUME are used and results are parsed
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, StopResumeTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    RequestServiceProxy proxy(object);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
    });
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.StopTasks({ "7" }, rets), ExceptionErrorCode::E_OK);
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_STOP));
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0], ExceptionErrorCode::E_OK);

    rets.clear();
    EXPECT_EQ(proxy.ResumeTasks({ "7" }, rets), ExceptionErrorCode::E_OK);
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_RESUME));
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0], ExceptionErrorCode::E_OK);
}

/**
 * @tc.name: PauseTasksTest001
 * @tc.desc: PauseTasks writes the version before the count and parses per-task
 *           results
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and one per-task code
 *           2. Call PauseTasks with version API10
 * @tc.expect: The request parcel carries version then count then tid; rets are
 *             parsed
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, PauseTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
    });
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    ExceptionErrorCode ret = proxy.PauseTasks({ "3" }, Version::API10, rets);

    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0], ExceptionErrorCode::E_OK);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_PAUSE));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), static_cast<uint32_t>(Version::API10));
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "3");
}

/**
 * @tc.name: RemoveTasksTest001
 * @tc.desc: RemoveTasks sends CMD_REMOVE with version and parses per-task
 *           results; error replies are propagated
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with top-level E_TASK_NOT_FOUND
 *           2. Call RemoveTasks with version API10
 *           3. Repeat with a successful reply
 * @tc.expect: The error code propagates; the success path parses per-task
 *             codes
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, RemoveTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    RequestServiceProxy proxy(object);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.RemoveTasks({ "8" }, Version::API10, rets), ExceptionErrorCode::E_TASK_NOT_FOUND);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
    });
    rets.clear();
    EXPECT_EQ(proxy.RemoveTasks({ "8" }, Version::API10, rets), ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0], ExceptionErrorCode::E_OK);
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_REMOVE));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), static_cast<uint32_t>(Version::API10));
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "8");
}

/**
 * @tc.name: QueryTasksTest001
 * @tc.desc: QueryTasks parses TaskInfoRet entries through ParcelHelper::UnMarshal
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK, one per-task code and a full
 *              TaskInfo stream
 *           2. Call QueryTasks with one tid
 * @tc.expect: rets[0].info carries every scripted field
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, QueryTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        WriteTaskInfoParcel(reply);
    });
    RequestServiceProxy proxy(object);

    std::vector<TaskInfoRet> rets;
    ExceptionErrorCode ret = proxy.QueryTasks({ "task-1" }, rets);

    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0].code, ExceptionErrorCode::E_OK);
    EXPECT_EQ(rets[0].info.tid, "task-1");
    EXPECT_EQ(rets[0].info.bundle, "bundle");
    EXPECT_EQ(rets[0].info.url, "https://example.com/file");
    EXPECT_EQ(rets[0].info.tries, INFO_TRIES);
    EXPECT_EQ(rets[0].info.ctime, INFO_CTIME);
    EXPECT_EQ(rets[0].info.mtime, INFO_MTIME);
    EXPECT_EQ(rets[0].info.priority, INFO_PRIORITY);
    ASSERT_EQ(rets[0].info.forms.size(), 1u);
    EXPECT_EQ(rets[0].info.forms[0].name, "formName");
    ASSERT_EQ(rets[0].info.files.size(), 1u);
    EXPECT_EQ(rets[0].info.files[0].filename, "file.txt");
    EXPECT_EQ(rets[0].info.progress.state, State::RUNNING);
    EXPECT_EQ(rets[0].info.progress.processed, INFO_PROCESSED);
    EXPECT_EQ(rets[0].info.progress.totalProcessed, INFO_TOTAL_PROCESSED);
    ASSERT_EQ(rets[0].info.progress.sizes.size(), 3u);
    EXPECT_EQ(rets[0].info.progress.sizes[2], INFO_SIZES[2]);
    EXPECT_EQ(rets[0].info.extras.at("ekey"), "evalue");
    EXPECT_EQ(rets[0].info.version, Version::API10);
    ASSERT_EQ(rets[0].info.taskStates.size(), 1u);
    EXPECT_EQ(rets[0].info.taskStates[0].path, "/path/file");
    EXPECT_EQ(rets[0].info.taskStates[0].responseCode, INFO_RESPONSE_CODE);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_QUERY));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "task-1");
}

/**
 * @tc.name: TouchTasksTest001
 * @tc.desc: TouchTasks writes tid and token per entry and parses TaskInfoRet
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and one TaskInfo stream
 *           2. Call TouchTasks with tid and token
 * @tc.expect: The request parcel carries tid then token; the TaskInfo is
 *             parsed
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, TouchTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        WriteTaskInfoParcel(reply);
    });
    RequestServiceProxy proxy(object);

    std::vector<TaskInfoRet> rets;
    ExceptionErrorCode ret = proxy.TouchTasks({ TaskIdAndToken{ .tid = "t1", .token = "tk1" } }, rets);

    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0].info.tid, "task-1");

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_TOUCH));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "t1");
    EXPECT_EQ(request.ReadString(), "tk1");
}

/**
 * @tc.name: ShowTasksTest001
 * @tc.desc: ShowTasks sends CMD_SHOW and parses TaskInfoRet entries
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and one TaskInfo stream
 *           2. Call ShowTasks with one tid
 * @tc.expect: Returns E_OK and parses the TaskInfo
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, ShowTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        WriteTaskInfoParcel(reply);
    });
    RequestServiceProxy proxy(object);

    std::vector<TaskInfoRet> rets;
    ExceptionErrorCode ret = proxy.ShowTasks({ "task-1" }, rets);
    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0].info.tid, "task-1");
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_SHOW));
}

/**
 * @tc.name: SetMaxSpeedsTest001
 * @tc.desc: SetMaxSpeeds writes tid and maxSpeed pairs and parses per-task
 *           results
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and one per-task code
 *           2. Call SetMaxSpeeds with one SpeedConfig
 * @tc.expect: Request parcel carries tid then speed; rets parsed
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, SetMaxSpeedsTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
    });
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    ExceptionErrorCode ret =
        proxy.SetMaxSpeeds(std::vector<SpeedConfig>{ SpeedConfig{ .tid = "s1", .maxSpeed = 4096 } }, rets);

    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 1u);
    EXPECT_EQ(rets[0], ExceptionErrorCode::E_OK);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_SET_MAX_SPEED));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "s1");
    EXPECT_EQ(request.ReadInt64(), 4096);
}

/**
 * @tc.name: SetModeTest001
 * @tc.desc: SetMode writes tid and mode and returns the reply code
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK
 *           2. Call SetMode with Mode::FOREGROUND
 *           3. Repeat with an error reply
 * @tc.expect: Success returns E_OK; the error reply propagates; request parcel
 *             carries tid then mode
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, SetModeTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    RequestServiceProxy proxy(object);

    object->ScriptReply(OHOS::ERR_NONE,
        [](OHOS::MessageParcel &reply) { reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK)); });
    EXPECT_EQ(proxy.SetMode("m1", Mode::FOREGROUND), ExceptionErrorCode::E_OK);

    object->ScriptReply(OHOS::ERR_NONE,
        [](OHOS::MessageParcel &reply) { reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_MODE)); });
    EXPECT_EQ(proxy.SetMode("m1", Mode::FOREGROUND), ExceptionErrorCode::E_TASK_MODE);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_SET_MODE));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadString(), "m1");
    EXPECT_EQ(request.ReadUint32(), static_cast<uint32_t>(Mode::FOREGROUND));
}

/**
 * @tc.name: DisableTaskNotificationTest001
 * @tc.desc: DisableTaskNotification writes the tid vector and appends one
 *           result per tid
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with two per-task codes
 *           2. Call DisableTaskNotification with two tids
 * @tc.expect: Returns E_OK; rets contain both codes; request parcel carries
 *             both tids
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, DisableTaskNotificationTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OTHER));
    });
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    ExceptionErrorCode ret = proxy.DisableTaskNotification({ "d1", "d2" }, rets);

    EXPECT_EQ(ret, ExceptionErrorCode::E_OK);
    ASSERT_EQ(rets.size(), 2u);
    EXPECT_EQ(rets[0], ExceptionErrorCode::E_OK);
    EXPECT_EQ(rets[1], ExceptionErrorCode::E_OTHER);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_DISABLE_TASK_NOTIFICATIONS));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    std::vector<std::string> tids;
    request.ReadStringVector(&tids);
    ASSERT_EQ(tids.size(), 2u);
    EXPECT_EQ(tids[0], "d1");
    EXPECT_EQ(tids[1], "d2");
}

/**
 * @tc.name: QueryMimeTypeTest001
 * @tc.desc: QueryMimeType writes the tid and returns the reply mime type
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and a mime string
 *           2. Call QueryMimeType
 * @tc.expect: Returns E_OK and mimeType carries the reply value; request
 *             parcel carries the tid
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, QueryMimeTypeTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteString("application/pdf");
    });
    RequestServiceProxy proxy(object);

    std::string mimeType;
    int32_t ret = proxy.QueryMimeType("q1", mimeType);

    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(mimeType, "application/pdf");

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_QUERYMIMETYPE));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadString(), "q1");
}

/**
 * @tc.name: QueryMimeTypeTest002
 * @tc.desc: QueryMimeType propagates the reply error code
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_MIMETYPE_NOT_FOUND
 *           2. Call QueryMimeType
 * @tc.expect: Returns E_MIMETYPE_NOT_FOUND
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, QueryMimeTypeTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_MIMETYPE_NOT_FOUND));
    });
    RequestServiceProxy proxy(object);

    std::string mimeType;
    int32_t ret = proxy.QueryMimeType("q1", mimeType);
    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_MIMETYPE_NOT_FOUND));
}

/**
 * @tc.name: SearchTest001
 * @tc.desc: Search serializes the Filter fields in order and parses the tid
 *           list
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with a two-entry tid list
 *           2. Call Search with a populated Filter
 * @tc.expect: Returns E_OK; tids parsed; request parcel carries bundle,
 *             before, after, state, action, mode
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, SearchTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteUint32(2);
        reply.WriteString("tid-a");
        reply.WriteString("tid-b");
    });
    RequestServiceProxy proxy(object);

    Filter filter;
    filter.bundle = "com.search.bundle";
    filter.before = QUERY_BEFORE;
    filter.after = QUERY_AFTER;
    filter.state = State::COMPLETED;
    filter.action = Action::DOWNLOAD;
    filter.mode = Mode::BACKGROUND;

    std::vector<std::string> tids;
    int32_t ret = proxy.Search(filter, tids);

    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_OK));
    ASSERT_EQ(tids.size(), 2u);
    EXPECT_EQ(tids[0], "tid-a");
    EXPECT_EQ(tids[1], "tid-b");

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_SEARCH));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadString(), "com.search.bundle");
    EXPECT_EQ(request.ReadInt64(), QUERY_BEFORE);
    EXPECT_EQ(request.ReadInt64(), QUERY_AFTER);
    EXPECT_EQ(request.ReadUint32(), static_cast<uint32_t>(State::COMPLETED));
    EXPECT_EQ(request.ReadUint32(), static_cast<uint32_t>(Action::DOWNLOAD));
    EXPECT_EQ(request.ReadUint32(), static_cast<uint32_t>(Mode::BACKGROUND));
}

/**
 * @tc.name: GetTaskTest001
 * @tc.desc: GetTask parses a Config reply through ParcelHelper::UnMarshalConfig
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and a Config stream in the
 *              UnMarshalConfig field order
 *           2. Call GetTask with tid and token
 * @tc.expect: Returns E_OK and the config carries the scripted values; request
 *             parcel carries tid then token
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, GetTaskTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, WriteGetTaskConfigReply);
    RequestServiceProxy proxy(object);

    Config config;
    int32_t ret = proxy.GetTask("gt1", "token-1", config);

    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_OK));
    ExpectGetTaskConfig(config);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_GETTASK));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadString(), "gt1");
    EXPECT_EQ(request.ReadString(), "token-1");
}

/**
 * @tc.name: GetTaskTest002
 * @tc.desc: GetTask returns E_CHANNEL_NOT_OPEN without failing when the
 *           service reports the channel is not open
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_CHANNEL_NOT_OPEN and no config body
 *           2. Call GetTask
 * @tc.expect: Returns E_CHANNEL_NOT_OPEN
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, GetTaskTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_CHANNEL_NOT_OPEN));
    });
    RequestServiceProxy proxy(object);

    Config config;
    int32_t ret = proxy.GetTask("gt2", "token-2", config);
    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_CHANNEL_NOT_OPEN));
}

/**
 * @tc.name: CreateTest001
 * @tc.desc: Create wraps CreateTasks and returns the stringified tid on
 *           success
 * @tc.precon: NA
 * @tc.step: 1. Script a successful CreateTasks reply for one task
 *           2. Call Create
 * @tc.expect: Returns E_OK and tid is "21"
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, CreateTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(21);
    });
    RequestServiceProxy proxy(object);

    std::string tid;
    int32_t ret = proxy.Create(MakeFullConfig(), tid);
    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(tid, "21");
}

/**
 * @tc.name: StartStopResumeTest001
 * @tc.desc: The single-task Start/Stop/Resume wrappers forward their per-task
 *           result
 * @tc.precon: NA
 * @tc.step: 1. Script a StartTasks reply with E_OK
 *           2. Call Start, then Stop and Resume with the same script
 * @tc.expect: All three return E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, StartStopResumeTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    RequestServiceProxy proxy(object);

    auto okReply = [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
    };
    object->ScriptReply(OHOS::ERR_NONE, okReply);
    EXPECT_EQ(proxy.Start("w1"), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(proxy.Stop("w1"), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(proxy.Resume("w1"), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_RESUME));
}

/**
 * @tc.name: PauseRemoveTest001
 * @tc.desc: Pause forwards the per-task result; Remove suppresses errors for
 *           API9 callers but keeps them for API10
 * @tc.precon: NA
 * @tc.step: 1. Script a PauseTasks reply with per-task E_TASK_NOT_FOUND
 *           2. Call Pause, expect the per-task code
 *           3. Script a RemoveTasks reply with per-task E_TASK_NOT_FOUND
 *           4. Call Remove with Version::API9 and expect E_OK, then with
 *              Version::API10 and expect E_TASK_NOT_FOUND
 * @tc.expect: API9 compat behavior documented
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, PauseRemoveTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    RequestServiceProxy proxy(object);

    auto notFoundReply = [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    };
    object->ScriptReply(OHOS::ERR_NONE, notFoundReply);
    EXPECT_EQ(proxy.Pause("p1", Version::API10), static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));

    object->ScriptReply(OHOS::ERR_NONE, notFoundReply);
    EXPECT_EQ(proxy.Remove("r1", Version::API9), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    object->ScriptReply(OHOS::ERR_NONE, notFoundReply);
    EXPECT_EQ(proxy.Remove("r1", Version::API10), static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
}

/**
 * @tc.name: OpenChannelTest001
 * @tc.desc: OpenChannel reads the reply fd on success and returns the error
 *           code on failure
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and a valid fd
 *           2. Call OpenChannel
 *           3. Script the reply with E_OTHER and call again
 * @tc.expect: Success returns E_OK with the fd; failure returns E_OTHER
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, OpenChannelTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    int rawFd = open("/dev/null", O_RDONLY);
    ASSERT_GE(rawFd, 0);
    object->ScriptReply(OHOS::ERR_NONE, [rawFd](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteFileDescriptor(rawFd);
    });
    RequestServiceProxy proxy(object);

    int sockFd = -1;
    int32_t ret = proxy.OpenChannel(sockFd);
    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_GE(sockFd, 0);
    close(sockFd);
    close(rawFd);

    object->ScriptReply(OHOS::ERR_NONE,
        [](OHOS::MessageParcel &reply) { reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OTHER)); });
    int fd2 = -1;
    EXPECT_EQ(proxy.OpenChannel(fd2), static_cast<int32_t>(ExceptionErrorCode::E_OTHER));
}

/**
 * @tc.name: SubscribeUnsubscribeTest001
 * @tc.desc: Subscribe writes the tid and returns the reply code; Unsubscribe
 *           returns E_OK after a successful send
 * @tc.precon: NA
 * @tc.step: 1. Script a reply with E_OK and call Subscribe/Unsubscribe
 * @tc.expect: Subscribe returns E_OK, Unsubscribe returns E_OK; the Subscribe
 *             request parcel carries the tid
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, SubscribeUnsubscribeTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE,
        [](OHOS::MessageParcel &reply) { reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK)); });
    RequestServiceProxy proxy(object);

    EXPECT_EQ(proxy.Subscribe("s1"), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadString(), "s1");

    EXPECT_EQ(proxy.Unsubscribe("s1"), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_UNSUBSCRIBE));
}

/**
 * @tc.name: SubRunCountTest001
 * @tc.desc: SubRunCount writes the listener object and returns the reply code;
 *           UnsubRunCount returns E_OK after a successful send
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK
 *           2. Call SubRunCount with the stub instance as listener
 *           3. Call UnsubRunCount
 * @tc.expect: Both return E_OK
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, SubRunCountTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE,
        [](OHOS::MessageParcel &reply) { reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK)); });
    RequestServiceProxy proxy(object);

    OHOS::sptr<NotifyInterface> listener = RunCountNotifyStub::GetInstance();
    EXPECT_EQ(proxy.SubRunCount(listener), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(proxy.UnsubRunCount(), static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_UNSUB_RUNCOUNT));
}

/**
 * @tc.name: GroupTasksTest001
 * @tc.desc: CreateGroup serializes gauge and notification fields and parses
 *           the gid; AttachGroup/DeleteGroup write the gid and forward the
 *           reply code
 * @tc.precon: NA
 * @tc.step: 1. Script the CreateGroup reply with a gid string
 *           2. Call CreateGroup with a populated notification
 *           3. Call AttachGroup and DeleteGroup with an error reply
 * @tc.expect: gid parsed; error codes forwarded; request parcels carry the
 *             scripted fields
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, GroupTasksTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    RequestServiceProxy proxy(object);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) { reply.WriteString("group-1"); });
    Notification notification;
    notification.title = "group-title";
    notification.text = "group-text";
    notification.wantAgent = std::nullopt;
    notification.disable = false;
    notification.visibility = 0b01;
    notification.hideStopButton = false;

    std::string gid;
    int32_t ret = proxy.CreateGroup(gid, true, notification);
    EXPECT_EQ(ret, static_cast<int32_t>(ExceptionErrorCode::E_OK));
    EXPECT_EQ(gid, "group-1");

    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_TRUE(request.ReadBool());
    EXPECT_TRUE(request.ReadBool());
    EXPECT_EQ(request.ReadString(), "group-title");
    EXPECT_TRUE(request.ReadBool());
    EXPECT_EQ(request.ReadString(), "group-text");
    EXPECT_FALSE(request.ReadBool());
    EXPECT_FALSE(request.ReadBool());
    EXPECT_EQ(request.ReadUint32(), 0b01);
    EXPECT_TRUE(request.ReadBool());
    EXPECT_FALSE(request.ReadBool());

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_GROUP_NOT_FOUND));
    });
    EXPECT_EQ(
        proxy.AttachGroup("group-1", { "t1", "t2" }), static_cast<int32_t>(ExceptionErrorCode::E_GROUP_NOT_FOUND));
    EXPECT_EQ(proxy.DeleteGroup("group-1"), static_cast<int32_t>(ExceptionErrorCode::E_GROUP_NOT_FOUND));
    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_DELETE_GROUP));
}

/**
 * @tc.name: UnMarshalConfigTest001
 * @tc.desc: UnMarshalConfig parses the get-task wire format field by field
 * @tc.precon: NA
 * @tc.step: 1. Write a Config in the UnMarshalConfig field order into a parcel
 *           2. Call ParcelHelper::UnMarshalConfig
 * @tc.expect: All fields are parsed to their written values
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, UnMarshalConfigTest001, TestSize.Level1)
{
    OHOS::MessageParcel data;
    WriteDemoConfigParcel(data);

    Config config;
    ParcelHelper::UnMarshalConfig(data, config);

    ASSERT_EQ(config.action, Action::DOWNLOAD);
    EXPECT_EQ(config.bundleName, "com.demo");
    EXPECT_EQ(config.version, Version::API9);
    ExpectDemoConfig(config);
}

/**
 * @tc.name: UnMarshalConfigTest002
 * @tc.desc: UnMarshalConfig stops parsing safely when a vector length is
 *           absurd, leaving later fields at defaults
 * @tc.precon: NA
 * @tc.step: 1. Write a valid base section, then a header length of 0xFFFFFFFF
 *           2. Call ParcelHelper::UnMarshalConfig
 * @tc.expect: Base fields parse, headers/extras/files stay empty and version
 *             keeps its default
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, UnMarshalConfigTest002, TestSize.Level1)
{
    OHOS::MessageParcel data;
    data.WriteUint32(static_cast<uint32_t>(Action::UPLOAD));
    data.WriteUint32(static_cast<uint32_t>(Mode::FOREGROUND));
    data.WriteUint32(0);
    data.WriteBool(false);
    data.WriteUint32(static_cast<uint32_t>(Network::ANY));
    data.WriteBool(false);
    data.WriteBool(false);
    data.WriteBool(true);
    data.WriteBool(true);
    data.WriteUint32(0);
    data.WriteInt64(0);
    data.WriteInt64(-1);
    data.WriteBool(false);
    data.WriteBool(false);
    data.WriteUint32(0);
    data.WriteBool(false);
    data.WriteBool(false);
    data.WriteString("com.bad");
    data.WriteString("https://example.com/bad");
    data.WriteString("");
    data.WriteString("");
    data.WriteString("GET");
    data.WriteUint32(0xFFFFFFFFu); // absurd header length

    Config config;
    config.version = Version::API10;
    ParcelHelper::UnMarshalConfig(data, config);

    EXPECT_EQ(config.action, Action::UPLOAD);
    EXPECT_EQ(config.bundleName, "com.bad");
    EXPECT_TRUE(config.headers.empty());
    EXPECT_TRUE(config.extras.empty());
    EXPECT_TRUE(config.files.empty());
    // Parsing aborted before the version field, so the pre-set value survives.
    EXPECT_EQ(config.version, Version::API10);
}

/**
 * @tc.name: QueryTest001
 * @tc.desc: Query wraps QueryTasks for a single tid and fills the out TaskInfo
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK, one per-task code and a TaskInfo
 *              stream
 *           2. Call Query with one tid
 * @tc.expect: Returns E_OK, the TaskInfo is filled and the request parcel
 *             carries the tid
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, QueryTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        WriteTaskInfoParcel(reply);
    });
    RequestServiceProxy proxy(object);

    TaskInfo info;
    int32_t ret = proxy.Query("task-1", info);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(info.tid, "task-1");
    EXPECT_EQ(info.bundle, "bundle");
    EXPECT_EQ(info.tries, INFO_TRIES);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_QUERY));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "task-1");
}

/**
 * @tc.name: QueryTest002
 * @tc.desc: Query propagates the SendRequest failure and the per-task error
 *           code, leaving the out TaskInfo untouched
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 and call Query
 *           2. Script the reply with E_OK and a per-task E_TASK_NOT_FOUND
 *           3. Call Query again
 * @tc.expect: First call returns E_SERVICE_ERROR, second returns the per-task
 *             code, and info keeps its preset tid
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, QueryTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    TaskInfo info;
    info.tid = "keep";
    EXPECT_EQ(proxy.Query("task-1", info), E_SERVICE_ERROR);
    EXPECT_EQ(info.tid, "keep");

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    EXPECT_EQ(proxy.Query("task-2", info), E_TASK_NOT_FOUND);
    EXPECT_EQ(info.tid, "keep");
}

/**
 * @tc.name: TouchTest001
 * @tc.desc: Touch wraps TouchTasks with tid and token and fills the out
 *           TaskInfo
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and a TaskInfo stream
 *           2. Call Touch with a tid and a token
 * @tc.expect: Returns E_OK, the TaskInfo is filled and the request parcel
 *             carries the tid then the token
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, TouchTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        WriteTaskInfoParcel(reply);
    });
    RequestServiceProxy proxy(object);

    TaskInfo info;
    int32_t ret = proxy.Touch("t1", "tk1", info);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(info.tid, "task-1");
    EXPECT_EQ(info.tries, INFO_TRIES);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_TOUCH));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "t1");
    EXPECT_EQ(request.ReadString(), "tk1");
}

/**
 * @tc.name: TouchTest002
 * @tc.desc: Touch propagates the SendRequest failure and the per-task error
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 and call Touch
 *           2. Script the reply with E_OK and a per-task E_TASK_NOT_FOUND
 *           3. Call Touch again
 * @tc.expect: First call returns E_SERVICE_ERROR, second returns the per-task
 *             code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, TouchTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    TaskInfo info;
    EXPECT_EQ(proxy.Touch("t1", "tk1", info), E_SERVICE_ERROR);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    EXPECT_EQ(proxy.Touch("t2", "tk2", info), E_TASK_NOT_FOUND);
}

/**
 * @tc.name: ShowTest001
 * @tc.desc: Show wraps ShowTasks for a single tid and fills the out TaskInfo
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and a TaskInfo stream
 *           2. Call Show with one tid
 * @tc.expect: Returns E_OK, the TaskInfo is filled and the request parcel
 *             carries the tid
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, ShowTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        WriteTaskInfoParcel(reply);
    });
    RequestServiceProxy proxy(object);

    TaskInfo info;
    int32_t ret = proxy.Show("task-1", info);

    EXPECT_EQ(ret, E_OK);
    EXPECT_EQ(info.tid, "task-1");
    EXPECT_EQ(info.bundle, "bundle");

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_SHOW));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "task-1");
}

/**
 * @tc.name: ShowTest002
 * @tc.desc: Show propagates the SendRequest failure and the per-task error
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 and call Show
 *           2. Script the reply with E_OK and a per-task E_TASK_NOT_FOUND
 *           3. Call Show again
 * @tc.expect: First call returns E_SERVICE_ERROR, second returns the per-task
 *             code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, ShowTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    TaskInfo info;
    EXPECT_EQ(proxy.Show("task-1", info), E_SERVICE_ERROR);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    EXPECT_EQ(proxy.Show("task-2", info), E_TASK_NOT_FOUND);
}

/**
 * @tc.name: SetMaxSpeedTest001
 * @tc.desc: SetMaxSpeed wraps SetMaxSpeeds for a single tid, returning the
 *           per-task code on the reply path
 * @tc.precon: NA
 * @tc.step: 1. Script the reply with E_OK and call SetMaxSpeed
 *           2. Verify the request parcel carries the tid then the speed
 *           3. Repeat with a SendRequest failure and a per-task E_TASK_MODE
 * @tc.expect: Success returns E_OK, failures propagate E_SERVICE_ERROR and
 *             E_TASK_MODE
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, SetMaxSpeedTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
    });
    RequestServiceProxy proxy(object);

    EXPECT_EQ(proxy.SetMaxSpeed("s1", 4096), E_OK);

    EXPECT_EQ(object->LastCode(), static_cast<uint32_t>(RequestInterfaceCode::CMD_SET_MAX_SPEED));
    OHOS::MessageParcel request;
    ASSERT_TRUE(object->FillRequestParcel(request));
    EXPECT_EQ(request.ReadInterfaceToken(), REQUEST_SERVICE_DESCRIPTOR);
    EXPECT_EQ(request.ReadUint32(), 1u);
    EXPECT_EQ(request.ReadString(), "s1");
    EXPECT_EQ(request.ReadInt64(), 4096);

    object->ScriptReply(-1, nullptr);
    EXPECT_EQ(proxy.SetMaxSpeed("s2", 1), E_SERVICE_ERROR);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_OK));
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_MODE));
    });
    EXPECT_EQ(proxy.SetMaxSpeed("s3", 1), E_TASK_MODE);
}

/**
 * @tc.name: StartTasksTest003
 * @tc.desc: StartTasks reports a non-REMOTE_DIED IPC failure through
 *           SysEventLog and propagates a non-OK top-level reply code
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 and call StartTasks
 *           2. Script the reply with a top-level E_TASK_NOT_FOUND and call
 *              again
 * @tc.expect: First call returns E_SERVICE_ERROR, second returns the top-level
 *             code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, StartTasksTest003, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.StartTasks({ "1" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    EXPECT_EQ(proxy.StartTasks({ "1" }, rets), ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: StopResumeTasksTest002
 * @tc.desc: StopTasks and ResumeTasks report non-REMOTE_DIED IPC failures
 *           through SysEventLog and propagate non-OK top-level reply codes
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 and call StopTasks then
 *              ResumeTasks
 *           2. Script top-level E_TASK_NOT_FOUND replies and call both again
 * @tc.expect: All four calls return E_SERVICE_ERROR or the top-level code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, StopResumeTasksTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.StopTasks({ "7" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.ResumeTasks({ "7" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    EXPECT_EQ(proxy.StopTasks({ "7" }, rets), ExceptionErrorCode::E_TASK_NOT_FOUND);
    EXPECT_EQ(proxy.ResumeTasks({ "7" }, rets), ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: PauseTasksTest002
 * @tc.desc: PauseTasks reports a non-REMOTE_DIED IPC failure through
 *           SysEventLog and propagates a non-OK top-level reply code
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 and call PauseTasks
 *           2. Script the reply with a top-level E_TASK_NOT_FOUND and call
 *              again
 * @tc.expect: First call returns E_SERVICE_ERROR, second returns the top-level
 *             code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, PauseTasksTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.PauseTasks({ "3" }, Version::API10, rets), ExceptionErrorCode::E_SERVICE_ERROR);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    EXPECT_EQ(proxy.PauseTasks({ "3" }, Version::API10, rets), ExceptionErrorCode::E_TASK_NOT_FOUND);
}

/**
 * @tc.name: RemoveTasksTest002
 * @tc.desc: RemoveTasks reports a non-REMOTE_DIED IPC failure through
 *           SysEventLog and propagates a non-OK top-level reply code
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1 and call RemoveTasks
 *           2. Script the reply with a top-level E_TASK_NOT_FOUND and call
 *              again
 * @tc.expect: First call returns E_SERVICE_ERROR, second returns the top-level
 *             code
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, RemoveTasksTest002, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.RemoveTasks({ "8" }, Version::API10, rets), ExceptionErrorCode::E_SERVICE_ERROR);

    object->ScriptReply(OHOS::ERR_NONE, [](OHOS::MessageParcel &reply) {
        reply.WriteInt32(static_cast<int32_t>(ExceptionErrorCode::E_TASK_NOT_FOUND));
    });
    EXPECT_EQ(proxy.RemoveTasks({ "8" }, Version::API10, rets), ExceptionErrorCode::E_TASK_NOT_FOUND);
}

static void FillQueryFilter(Filter &filter, const char *bundle)
{
    filter.bundle = bundle;
    filter.before = QUERY_BEFORE;
    filter.after = QUERY_AFTER;
    filter.state = State::COMPLETED;
    filter.action = Action::DOWNLOAD;
    filter.mode = Mode::BACKGROUND;
}

static void FillNotification(Notification &notification, const char *title, const char *text)
{
    notification.title = title;
    notification.text = text;
    notification.wantAgent = std::nullopt;
    notification.disable = false;
    notification.visibility = 0b01;
    notification.hideStopButton = false;
}

/**
 * @tc.name: ProxyRemoteDiedAllTest001
 * @tc.desc: Every proxied IPC method reports E_SERVICE_ERROR when SendRequest
 *           reports a dead remote, skipping the SysEventLog fault record
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return REMOTE_DIED_ERROR
 *           2. Call each proxied method once
 * @tc.expect: ExceptionErrorCode methods return E_SERVICE_ERROR and int32_t
 *             methods return its numeric value
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, ProxyRemoteDiedAllTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(REMOTE_DIED_ERROR, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<Config> configs = { MakeFullConfig() };
    std::vector<TaskRet> taskRets;
    EXPECT_EQ(proxy.CreateTasks(configs, taskRets), ExceptionErrorCode::E_SERVICE_ERROR);

    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.StartTasks({ "rd1" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.StopTasks({ "rd1" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.ResumeTasks({ "rd1" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.PauseTasks({ "rd1" }, Version::API10, rets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.RemoveTasks({ "rd1" }, Version::API10, rets), ExceptionErrorCode::E_SERVICE_ERROR);

    std::vector<TaskInfoRet> infoRets;
    EXPECT_EQ(proxy.QueryTasks({ "rd1" }, infoRets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.ShowTasks({ "rd1" }, infoRets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.TouchTasks({ TaskIdAndToken{ .tid = "rd1", .token = "tk1" } }, infoRets),
        ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.SetMaxSpeeds(std::vector<SpeedConfig>{ SpeedConfig{ .tid = "rd1", .maxSpeed = 4096 } }, rets),
        ExceptionErrorCode::E_SERVICE_ERROR);

    EXPECT_EQ(proxy.SetMode("rd1", Mode::FOREGROUND), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.DisableTaskNotification({ "rd1" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);

    Filter filter;
    FillQueryFilter(filter, "com.remotedied.bundle");
    std::vector<std::string> tids;
    std::string mimeType;
    int32_t sockFd = -1;
    EXPECT_EQ(proxy.Search(filter, tids), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.QueryMimeType("rd1", mimeType), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.OpenChannel(sockFd), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.Subscribe("rd1"), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.Unsubscribe("rd1"), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));

    OHOS::sptr<NotifyInterface> listener = RunCountNotifyStub::GetInstance();
    EXPECT_EQ(proxy.SubRunCount(listener), REMOTE_DIED_ERROR);
    EXPECT_EQ(proxy.UnsubRunCount(), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));

    std::string gid;
    Notification notification;
    FillNotification(notification, "rd-title", "rd-text");
    EXPECT_EQ(proxy.CreateGroup(gid, true, notification), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.AttachGroup("rg1", { "rd1" }), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.DeleteGroup("rg1"), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
}

/**
 * @tc.name: ProxySendFailAllTest001
 * @tc.desc: Proxy methods without dedicated failure cases report E_SERVICE_ERROR
 *           when SendRequest fails with a generic error and record the fault
 * @tc.precon: NA
 * @tc.step: 1. Script SendRequest to return -1
 *           2. Call each proxied method once
 * @tc.expect: ExceptionErrorCode methods return E_SERVICE_ERROR and int32_t
 *             methods return its numeric value
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(RequestServiceProxyTest, ProxySendFailAllTest001, TestSize.Level1)
{
    auto object = OHOS::sptr<MockRemoteObject>::MakeSptr(REQUEST_SERVICE_DESCRIPTOR);
    object->ScriptReply(-1, nullptr);
    RequestServiceProxy proxy(object);

    std::vector<ExceptionErrorCode> rets;
    EXPECT_EQ(proxy.StartTasks({ "sf1" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.SetMode("sf1", Mode::FOREGROUND), ExceptionErrorCode::E_SERVICE_ERROR);
    EXPECT_EQ(proxy.DisableTaskNotification({ "sf1" }, rets), ExceptionErrorCode::E_SERVICE_ERROR);

    Filter filter;
    FillQueryFilter(filter, "com.sendfail.bundle");
    std::vector<std::string> tids;
    std::string mimeType;
    int32_t sockFd = -1;
    EXPECT_EQ(proxy.Search(filter, tids), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.QueryMimeType("sf1", mimeType), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.OpenChannel(sockFd), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.Subscribe("sf1"), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.Unsubscribe("sf1"), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));

    OHOS::sptr<NotifyInterface> listener = RunCountNotifyStub::GetInstance();
    EXPECT_EQ(proxy.SubRunCount(listener), -1);
    EXPECT_EQ(proxy.UnsubRunCount(), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));

    std::string gid;
    Notification notification;
    FillNotification(notification, "sf-title", "sf-text");
    EXPECT_EQ(proxy.CreateGroup(gid, true, notification), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.AttachGroup("rg1", { "sf1" }), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
    EXPECT_EQ(proxy.DeleteGroup("rg1"), static_cast<int32_t>(ExceptionErrorCode::E_SERVICE_ERROR));
}
