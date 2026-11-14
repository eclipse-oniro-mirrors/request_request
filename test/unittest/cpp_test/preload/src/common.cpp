/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "common.h"

#include "request_preload.h"
using namespace OHOS::Request;
using OHOS::Request::Test::MockHttpServer;
using OHOS::Request::Test::HTTP_STATUS_OK;
using OHOS::Request::Test::HTTP_STATUS_NOT_FOUND;

namespace {
// Body sizes of the historical public files these local mocks replace, so
// byte-count assertions keep their original meaning.
constexpr int TEST_TXT_SIZE = 1042003;    // gitee downloadTests test.txt
constexpr int MOVIE_SIZE = 318465;        // w3cschool demosource movie.mp4
constexpr int BDSTATIC_SIZE = 2048;       // bdstatic cae_h264 sample (chunked)
constexpr int ROOT_BODY_SIZE = 1024;      // baidu root catch-all page
// A 404 body must stay >= 1 KiB, otherwise netstack never fires the fail
// callback (it swallows short error bodies).
constexpr int NOT_FOUND_BODY_SIZE = 1024;
constexpr int NOT_FOUND_BODY_LARGE_SIZE = 1 << 20; // 1 MiB, see comment below
} // namespace

// Shared HTTP mock server. Routes are registered once on first use; the
// server lives for the whole process so every fixture reuses the same port.
const std::string &GetMockBaseUrl()
{
    static std::string baseUrl;
    static MockHttpServer server;
    if (baseUrl.empty()) {
        // Order matters: specific paths first, "/" last as a catch-all.
        server.AddRoute("/test.txt", HTTP_STATUS_OK, std::string(TEST_TXT_SIZE, 'A'));
        server.AddRoute("/movie.mp4", HTTP_STATUS_OK, std::string(MOVIE_SIZE, 'B'));
        server.AddRoute("/bdstatic.mp4", HTTP_STATUS_OK, std::string(BDSTATIC_SIZE, 'D'), "video/mp4", true);
        // 404 with a large body so progress + fail callbacks both fire.
        // common_progress throttles at PROGRESS_INTERVAL=8, so the body must
        // be large enough for curl to call progress >=8 times after the first
        // data chunk lands (on_data_receive sets data_receive=true).
        std::string notFoundBody(NOT_FOUND_BODY_SIZE, 'N');
        std::string notFoundBodyLarge(NOT_FOUND_BODY_LARGE_SIZE, 'N');
        server.AddRoute("/fqwert/aaaaa", HTTP_STATUS_NOT_FOUND, notFoundBodyLarge);
        server.AddRoute("/fqwert/aaaaaa", HTTP_STATUS_NOT_FOUND, notFoundBodyLarge);
        server.AddRoute("/notExistResource.txt", HTTP_STATUS_NOT_FOUND, notFoundBody);
        server.AddRoute("/test_not_exist.txt", HTTP_STATUS_NOT_FOUND, notFoundBody);
        server.AddRoute("/", HTTP_STATUS_OK, std::string(ROOT_BODY_SIZE, 'C'));  // baidu root catch-all
        server.Start();
        baseUrl = server.GetBaseUrl();
    }
    return baseUrl;
}

// Shared HTTPS mock server for the SSL-specific test cases. The certificate
// and key are embedded in the mock server source; on first use the CA is also
// staged to TEST_CA_PATH so curl clients can point caPath at a real file.
const std::string &GetMockHttpsBaseUrl()
{
    static std::string baseUrl;
    static MockHttpServer server;
    if (baseUrl.empty()) {
        server.AddRoute("/test.txt", HTTP_STATUS_OK, std::string(TEST_TXT_SIZE, 'A'));
        // Failure to stage the CA is not fatal: the https cases would then
        // simply fail at the certificate check instead of hanging.
        (void)MockHttpServer::WriteTestCaFile(TEST_CA_PATH);
        server.StartTls();
        baseUrl = server.GetBaseUrl();
    }
    return baseUrl;
}
TestCallback::TestCallback(size_t size)
{
    auto flagS = std::make_shared<std::atomic_bool>(false);
    auto flagF = std::make_shared<std::atomic_bool>(false);
    auto flagC = std::make_shared<std::atomic_bool>(false);
    auto flagP = std::make_shared<std::atomic_bool>(false);
    this->callback = PreloadCallback{
        .OnSuccess =
            [flagS, size](const std::shared_ptr<Data> &&data, const std::string &taskId) {
                if (size == 0 || data->bytes().length() == size) {
                    flagS->store(true);
                }
            },
        .OnCancel = [flagC]() { flagC->store(true); },
        .OnFail = [flagF](const PreloadError &error, const std::string &taskId) { flagF->store(true); },
        .OnProgress = [flagP](uint64_t current, uint64_t total) { flagP->store(true); },
    };
    this->flagS = flagS;
    this->flagF = flagF;
    this->flagC = flagC;
    this->flagP = flagP;
}