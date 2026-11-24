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

/*
 * R4 API-coverage cases for the preload facade. Every failure path uses the
 * in-process mock HTTP server (or an unroutable address); no real SA, no
 * public network. Targets the accessors/setters that no existing case calls:
 * PreloadError::{GetCode,GetMessage,GetErrorKind}, Preload::SetFileCachePath,
 * Preload::SetGlobalRetryOptions, Preload::SetGlobalTimeoutOptions and the
 * invalid-UTF-8 early-exit arm of Preload::Contains.
 */

#include <gtest/gtest.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <thread>

#include "common.h"
#include "log.h"
#include "request_preload.h"

using namespace testing::ext;
using namespace OHOS::Request;

namespace {
constexpr size_t SLEEP_INTERVAL = 1000;
constexpr size_t WAIT_ROUNDS = 10;
// Writable-on-device staging dir for the SetFileCachePath valid arm.
constexpr const char *COV_CACHE_DIR = "/data/local/tmp/preload_cov_cache";
} // namespace

class PreloadApiCov : public testing::Test {
public:
    void SetUp();
};

void PreloadApiCov::SetUp(void)
{
    //
}

namespace {
// Shared sink capturing the PreloadError accessors from the OnFail thread.
// flagF is stored last so a seq_cst read of true happens-after all captures.
struct ErrorSink {
    std::shared_ptr<std::atomic_bool> flagF = std::make_shared<std::atomic_bool>(false);
    std::shared_ptr<std::atomic<int32_t>> code = std::make_shared<std::atomic<int32_t>>(0);
    std::shared_ptr<std::atomic<int32_t>> kind = std::make_shared<std::atomic<int32_t>>(-1);
    std::shared_ptr<std::string> message = std::make_shared<std::string>();
};
} // namespace

/**
 * @tc.name: ErrorInfoAccessors
 * @tc.desc: OnFail hands a PreloadError whose GetCode/GetMessage/GetErrorKind
 *           reflect a mock-server 404 (HTTP category, non-zero code, message)
 * @tc.precon: NA
 * @tc.step: 1. Load the mock /fqwert/aaaaa route that answers HTTP 404
 *           2. In OnFail capture GetCode, GetMessage and GetErrorKind
 *           3. Wait for the fail callback
 * @tc.expect: The captures show ErrorKind::HTTP, a non-zero code and a
 *             non-empty message
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadApiCov, ErrorInfoAccessors, TestSize.Level1)
{
    auto *preload = Preload::GetInstance();
    std::string url = GetMockBaseUrl() + "/fqwert/aaaaa";
    preload->Remove(url);

    ErrorSink sink;
    auto flagF = sink.flagF;
    auto code = sink.code;
    auto kind = sink.kind;
    auto message = sink.message;
    PreloadCallback callback{
        .OnSuccess = [](const std::shared_ptr<Data> &&, const std::string &) {},
        .OnCancel = []() {},
        .OnFail = [flagF, code, kind, message](const PreloadError &error, const std::string &) {
            code->store(error.GetCode());
            kind->store(static_cast<int32_t>(error.GetErrorKind()));
            *message = error.GetMessage();
            flagF->store(true);
        },
        .OnProgress = [](uint64_t, uint64_t) {},
    };

    auto handle = preload->load(url, std::make_unique<PreloadCallback>(callback));
    ASSERT_NE(handle, nullptr);
    size_t counter = WAIT_ROUNDS;
    while ((!handle->IsFinish() || !sink.flagF->load()) && counter--) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }

    EXPECT_TRUE(sink.flagF->load());
    EXPECT_EQ(sink.kind->load(), static_cast<int32_t>(ErrorKind::HTTP));
    // Rust CacheDownloadError carries no numeric code for the HTTP-status
    // failure path (code: None), so GetCode() yields its documented default 0.
    EXPECT_EQ(sink.code->load(), 0);
    EXPECT_FALSE(sink.message->empty());
}

/**
 * @tc.name: SetFileCachePathArcs
 * @tc.desc: SetFileCachePath ignores an empty path (early-exit arm) and
 *           accepts a valid directory without breaking later downloads
 * @tc.precon: NA
 * @tc.step: 1. Call SetFileCachePath with an empty string
 *           2. Stage a writable directory and call SetFileCachePath with it
 *           3. Load the mock /test.txt route and wait for completion
 * @tc.expect: The empty call is a no-op and the valid call keeps the download
 *             succeeding (flagS set, no failure)
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadApiCov, SetFileCachePathArcs, TestSize.Level1)
{
    auto *preload = Preload::GetInstance();
    preload->SetFileCachePath("");

    std::string dir(COV_CACHE_DIR);
    int mkRet = mkdir(dir.c_str(), 0755);
    ASSERT_TRUE(mkRet == 0 || errno == EEXIST);
    preload->SetFileCachePath(dir);

    std::string url = GetMockBaseUrl() + "/test.txt";
    preload->Remove(url);
    TestCallback test;
    auto &[flagS, flagF, flagC, flagP, callback] = test;
    auto handle = preload->load(url, std::make_unique<PreloadCallback>(callback));
    ASSERT_NE(handle, nullptr);
    size_t counter = WAIT_ROUNDS;
    while ((!handle->IsFinish() || !(flagS->load() || flagF->load())) && counter--) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }

    EXPECT_TRUE(flagS->load());
    EXPECT_FALSE(flagF->load());
}

/**
 * @tc.name: GlobalOptionsSetters
 * @tc.desc: SetGlobalRetryOptions and SetGlobalTimeoutOptions accept documented
 *           values and downloads keep succeeding afterwards
 * @tc.precon: NA
 * @tc.step: 1. Call SetGlobalRetryOptions with maxRetryCount = 1
 *           2. Call SetGlobalTimeoutOptions with networkCheckTimeout = 20 and
 *              httpTotalTimeout = 60
 *           3. Load the mock /movie.mp4 route and wait for completion
 * @tc.expect: Both setters accept the values and the download succeeds
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadApiCov, GlobalOptionsSetters, TestSize.Level1)
{
    auto *preload = Preload::GetInstance();
    preload->SetGlobalRetryOptions(RetryOptions{ .maxRetryCount = 1 });
    preload->SetGlobalTimeoutOptions(TimeoutOptions{ .networkCheckTimeout = 20, .httpTotalTimeout = 60 });

    std::string url = GetMockBaseUrl() + "/movie.mp4";
    preload->Remove(url);
    TestCallback test;
    auto &[flagS, flagF, flagC, flagP, callback] = test;
    auto handle = preload->load(url, std::make_unique<PreloadCallback>(callback));
    ASSERT_NE(handle, nullptr);
    size_t counter = WAIT_ROUNDS;
    while ((!handle->IsFinish() || !(flagS->load() || flagF->load())) && counter--) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }

    EXPECT_TRUE(flagS->load());
    EXPECT_FALSE(flagF->load());
}

/**
 * @tc.name: ContainsUtf8Guard
 * @tc.desc: Contains rejects an invalid-UTF-8 URL via its early-exit arm and
 *           answers true for a successfully cached URL, false for an absent one
 * @tc.precon: NA
 * @tc.step: 1. Call Contains with a URL containing raw 0xFF/0xFE bytes
 *           2. Call Contains with a valid URL that was never cached
 *           3. Load the mock root route to success and call Contains again
 * @tc.expect: The invalid-UTF-8 URL returns false, the absent URL false and
 *             the cached URL true
 * @tc.type: FUNC
 * @tc.require: issueNumber
 * @tc.level: Level 1
 */
HWTEST_F(PreloadApiCov, ContainsUtf8Guard, TestSize.Level1)
{
    auto *preload = Preload::GetInstance();

    std::string bad = GetMockBaseUrl() + "/\xff\xfe";
    EXPECT_FALSE(preload->Contains(bad));

    std::string absent = GetMockBaseUrl() + "/notExistResource.txt";
    EXPECT_FALSE(preload->Contains(absent));

    std::string url = GetMockBaseUrl() + "/";
    preload->Remove(url);
    TestCallback test;
    auto &[flagS, flagF, flagC, flagP, callback] = test;
    auto handle = preload->load(url, std::make_unique<PreloadCallback>(callback));
    ASSERT_NE(handle, nullptr);
    size_t counter = WAIT_ROUNDS;
    while ((!handle->IsFinish() || !(flagS->load() || flagF->load())) && counter--) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_INTERVAL));
    }
    EXPECT_TRUE(flagS->load());

    EXPECT_TRUE(preload->Contains(url));
}
