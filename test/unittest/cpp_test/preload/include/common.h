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

#ifndef REQUEST_PRELOAD_TEST_COMMON_H
#define REQUEST_PRELOAD_TEST_COMMON_H

#include <atomic>
#include <memory>
#include <string>

#include "mock_http_server.h"
#include "request_preload.h"

// Path where the embedded test CA (TestCert::CA_PEM) is staged at runtime so
// PreloadOptions.caPath can point curl at it; the HTTPS mock server itself
// loads its certificate from memory, so no files are pushed or checked in.
inline const std::string TEST_CA_PATH = "/data/local/tmp/test_ca.pem";

// Lazily start the shared HTTP mock server (127.0.0.1, OS-assigned port) and
// register every route the preload tests need. Returns the base URL, e.g.
// "http://127.0.0.1:38211". Idempotent: routes are only registered once.
const std::string &GetMockBaseUrl();

// Lazily start the shared HTTPS mock server (self-signed cert). Separate
// instance/port from the HTTP one so both can coexist. Returns e.g.
// "https://127.0.0.1:38212".
const std::string &GetMockHttpsBaseUrl();

struct TestCallback {
    TestCallback(size_t size = 0);
    std::shared_ptr<std::atomic_bool> flagS;
    std::shared_ptr<std::atomic_bool> flagF;
    std::shared_ptr<std::atomic_bool> flagC;
    std::shared_ptr<std::atomic_bool> flagP;
    OHOS::Request::PreloadCallback callback;
};

#endif