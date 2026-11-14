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

#ifndef REQUEST_TEST_MOCK_HTTP_SERVER_H
#define REQUEST_TEST_MOCK_HTTP_SERVER_H

#include <atomic>
#include <string>
#include <thread>
#include <vector>

namespace OHOS::Request::Test {
// HTTP status codes referenced by mock routes and response building.
constexpr int HTTP_STATUS_OK = 200;
constexpr int HTTP_STATUS_NOT_FOUND = 404;
constexpr int HTTP_STATUS_FOUND = 302;
constexpr int HTTP_STATUS_UNAUTHORIZED = 401;

// Self-signed test certificates (PEM contents) used by the HTTPS mock
// server. They are embedded as source constants so the repository carries
// no certificate files; regenerate with tools/gen_certs.sh and paste the
// output into mock_http_server.cpp. The server certificate SAN covers
// DNS:localhost and IP:127.0.0.1.
namespace TestCert {
extern const char *CA_PEM;          // trust anchor for curl caPath
extern const char *SERVER_CERT_PEM; // mock server certificate
extern const char *SERVER_KEY_PEM;  // mock server private key
}  // namespace TestCert

// Route entry: maps a URL path suffix to a fixed HTTP response.
struct MockRoute {
    int status;                // HTTP status code, e.g. HTTP_STATUS_OK / HTTP_STATUS_NOT_FOUND
    std::string body;          // response body bytes
    std::string contentType;   // Content-Type header value
    bool chunked;              // use Transfer-Encoding: chunked instead of Content-Length
};

// Minimal in-process HTTP/HTTPS mock server for unit tests.
//
// Binds 127.0.0.1:0 (OS-assigned port), accepts connections on a background
// thread, and replies with fixed responses registered via AddRoute(). Each
// connection is handled on a detached thread so concurrent loads work; the
// server stays alive across multiple requests so retry test cases work.
//
// Register routes with AddRoute() before calling Start() (plain HTTP) or
// StartTls() (HTTPS, self-signed cert), then build request URLs from
// GetBaseUrl() + path.
class MockHttpServer {
public:
    MockHttpServer();
    ~MockHttpServer();

    MockHttpServer(const MockHttpServer &) = delete;
    MockHttpServer &operator=(const MockHttpServer &) = delete;

    // Start HTTP listener on 127.0.0.1:0. Blocks until the port is assigned.
    void Start();
    // Start HTTPS listener using the embedded TestCert certificate/key.
    // Blocks until the port is assigned.
    void StartTls();

    // Base URL, e.g. "http://127.0.0.1:38211" or "https://127.0.0.1:38211".
    std::string GetBaseUrl() const;

    // Register a response for requests whose URL path ends with `path`.
    // Routes are checked in registration order; first match wins.
    // Must be called before Start()/StartTls().
    void AddRoute(const std::string &path, int status, const std::string &body,
                  const std::string &contentType = "text/plain", bool chunked = false);

    // Write the embedded test CA (TestCert::CA_PEM) to `path` so clients
    // that take a CA file (curl caPath) can trust the mock server.
    // Overwrites an existing file. Returns true on success.
    static bool WriteTestCaFile(const std::string &path);

private:
    void RunLoop(int listenFd, bool tls);
    // Drive the non-blocking TLS handshake for one accepted connection.
    // Returns the SSL* (as void*) on success, or nullptr on failure/timeout;
    // the caller closes clientFd and continues on nullptr.
    void *HandshakeTls(int clientFd, void *ctx);
    // Read one request, build and send the response, then close the socket.
    void HandleConnection(int clientFd, void *ssl);
    std::string BuildResponse(const std::string &requestPath) const;
    int listenFd_;
    int port_;
    bool tls_;
    std::atomic<bool> running_;
    std::thread thread_;
    std::vector<std::pair<std::string, MockRoute>> routes_;
    // Opaque SSL_CTX* — kept as void* so <openssl/ssl.h> stays out of the header.
    void *sslCtx_;
};
}  // namespace OHOS::Request::Test
#endif  // REQUEST_TEST_MOCK_HTTP_SERVER_H
