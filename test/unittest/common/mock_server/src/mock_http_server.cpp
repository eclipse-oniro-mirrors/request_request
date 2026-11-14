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

#include "mock_http_server.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>

#include "openssl/err.h"
#include "openssl/ssl.h"

namespace OHOS::Request::Test {

// Embedded test certificates (see TestCert in the header): regenerated
// by tools/gen_certs.sh and pasted here so no .pem files are checked in.
namespace TestCert {
const char *CA_PEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDFTCCAf2gAwIBAgIUI0HxBFbnarJwkxC5Co64xy1h+FkwDQYJKoZIhvcNAQEL\n"
    "BQAwGjEYMBYGA1UEAwwPVGVzdCBQcmVsb2FkIENBMB4XDTI2MDgwMTA3MjUxNFoX\n"
    "DTM2MDcyOTA3MjUxNFowGjEYMBYGA1UEAwwPVGVzdCBQcmVsb2FkIENBMIIBIjAN\n"
    "BgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAoeMwA5Vce4kepeHZ0ZztF9ibWpBB\n"
    "6KohNkpRTv2f2b/YTM5fBmLhsCUYD234vfBf0qGeoreXfEwIHg6+EQyvDMGnh7Nc\n"
    "0FnCgTOsHg9gGfzXMGBfG5+j+B0YV7QInefGAKe6/sF7eTNk7tBxlDkCzQO68PKQ\n"
    "NJwLWr+gE694JdB9jDKa3CkydDyq51A+SAYamzAsj9zxSiGoGgCLiYItPNx06NDd\n"
    "V45AEY2yv3SdFdRjhPaG5IsT+UgI1VDE7x20E8Xuk7Tir2MojIRkdAK1HByN32Kf\n"
    "wopKVaTbCViUETIoSyNWkKKx16Kjt443zftTxm8Os3HrVAj13lIctNCqMwIDAQAB\n"
    "o1MwUTAdBgNVHQ4EFgQUwLq6KWIEZ0wJ8IJpHnkJaS9d9vwwHwYDVR0jBBgwFoAU\n"
    "wLq6KWIEZ0wJ8IJpHnkJaS9d9vwwDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0B\n"
    "AQsFAAOCAQEAHE2BAnfkT0X1Pbwa9JNUiUVvX1gBRIsy5bPvZWlZM3eEdrW10gwy\n"
    "hhTxgftIegQtWfQOMJ/JOBWzRV/9ef00pVlUkIIix/rIZF8i1+2iyc84GeACuGy7\n"
    "01q2riBMGTEQoc7ceN37GpEx3yPJ/SjVDPaeCzRYAj9s6Yw7Kwy+8XuuCF++sGlP\n"
    "mwLrVQqaY7kJz7QJut+vp5caTaN2pcFEPthZ6NlC1sjjl6Za8+JkD++qk7a2jOZh\n"
    "ecfXzn+NJmk6BAbiondmom79EmHBUeLCRFl54JCHhptS0F7TmIbn02YjLoOLJ+PX\n"
    "7tiys3VbGZ+liqs2BdJAGaPqjv3YvPp+wg==\n"
    "-----END CERTIFICATE-----\n";

const char *SERVER_CERT_PEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDGjCCAgKgAwIBAgIUeX+TAt+DEbAi7Zk9km2MxTQoelQwDQYJKoZIhvcNAQEL\n"
    "BQAwGjEYMBYGA1UEAwwPVGVzdCBQcmVsb2FkIENBMB4XDTI2MDgwMTA3MjUxNFoX\n"
    "DTM2MDcyOTA3MjUxNFowFDESMBAGA1UEAwwJbG9jYWxob3N0MIIBIjANBgkqhkiG\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAujnBFy7b+FDn32K4gbmNIxUfSGQO/NvVMQ6y\n"
    "ISj6UyWiNxw0HDnwlZJmnfNPvs5KVvBmfwwat8kHgWyh0fjpnS2XDV6ork4hPXgz\n"
    "B/mgi7SohjWttqq69jDoxH5eFcw6+JrJA7vVr7BpmigtazdeuGkzW3VgIfxpXyVg\n"
    "BPHPaKNDyGgxlAMYX4QgIyowN1yCDQg7OodlSnFobp7ZiyUXA9odXa6QPGtHhwYq\n"
    "ItwusPxTS9NjJILyBysURGIGMRBxb1vygqCS94opGRIK0e5YGH4nfIGnFT+lQRYh\n"
    "pocrtftSA4uZbGGM0ZsUCfcOTHbN3tBzCMeU9+11bBJVKAmFuQIDAQABo14wXDAa\n"
    "BgNVHREEEzARgglsb2NhbGhvc3SHBH8AAAEwHQYDVR0OBBYEFDo1GxZ9O1Mkxxuq\n"
    "cY+Y7pIxN8dOMB8GA1UdIwQYMBaAFMC6uiliBGdMCfCCaR55CWkvXfb8MA0GCSqG\n"
    "SIb3DQEBCwUAA4IBAQBJZVpYVOlSiEUBo0lpgkDo+xhx34Kbdmjn+4P6fcbPiscd\n"
    "C2MDyLIfz6OZ5KWR1epf/nmkRwfem2+aOf/gLGkxwpfJy8/PCkScDp27DYLsRySh\n"
    "RCSCoM9LNTm+2F5JWXrePzrEoKVb0lTi6nbIbOMstHv4ATiINrQhqKIGk/JtOgkG\n"
    "U+AxdSExvC5kd0mn4d1ZiM+WsvDBIUaANfg80dN+A4z0bDE20kZPka0HAv2ul3P/\n"
    "97SpG6LN82zPop9ANAwEPnoFqYa1jRyjhk18NbcaDqfrHfqwL4M9E83PCvTGayjR\n"
    "cC8TUNLUGOoW7ikG1HmJriReHXUNKp8OFvCMiwjV\n"
    "-----END CERTIFICATE-----\n";

const char *SERVER_KEY_PEM =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvAIBADANBgkqhkiG9w0BAQEFAASCBKYwggSiAgEAAoIBAQC6OcEXLtv4UOff\n"
    "YriBuY0jFR9IZA7829UxDrIhKPpTJaI3HDQcOfCVkmad80++zkpW8GZ/DBq3yQeB\n"
    "bKHR+OmdLZcNXqiuTiE9eDMH+aCLtKiGNa22qrr2MOjEfl4VzDr4mskDu9WvsGma\n"
    "KC1rN164aTNbdWAh/GlfJWAE8c9oo0PIaDGUAxhfhCAjKjA3XIINCDs6h2VKcWhu\n"
    "ntmLJRcD2h1drpA8a0eHBioi3C6w/FNL02MkgvIHKxREYgYxEHFvW/KCoJL3iikZ\n"
    "EgrR7lgYfid8gacVP6VBFiGmhyu1+1IDi5lsYYzRmxQJ9w5Mds3e0HMIx5T37XVs\n"
    "ElUoCYW5AgMBAAECggEAAzz2zYv4eHfTFL+Jh1aJaYE7PrKgG/3ur5uQ/DiTXYlP\n"
    "g3XB6YJvtRdOe3B6hzrctuhxE2q4fYNfljZdvBIf7f/rBI+Fr2eg1mORJjJG1cMP\n"
    "W2t4+ntukV8Xy90WjpQgBUhiVpNB469CbzmQktDFYxkpZgdAMI5MOHXkvQISgGAV\n"
    "zBRenrrUXfOnu9XJXVgKsYyxlOxpKJp4k0P7xkr2OT4YGTKDuVtRSd+tLNN5wM1I\n"
    "PbTAHpEWe5QjiSmwZre2lHqGbLu5eD2YyRI6BzRyzFP1MCjAGiMCY1mHsSfGod+5\n"
    "RAcKb5urDngK44M6bsEE4b3RtIkdsg1eljTLIQdpIQKBgQDV9rbW8bwfUycuJgrY\n"
    "/p2KjrODFPYmrRUGZaaXBxb+e2+RHwd3Fjj8xcdMmngR3KHPa4oU51od0flqq5wY\n"
    "B1Xqks8uwLkKIGYUjVFA7/adjcOXJYVLU+Tz4UU5eTc72z3bBx1+hLn0pTCj9icA\n"
    "X9UltCqAXPPls4TP4FBe8AHQmQKBgQDez/QhQQi4AajLWhqmu8wBLukXJTQ5GiIk\n"
    "kzyk3u1VULSxqUtjPl7C9uYQOp/W1ctq8uIdYV1zwiA7fgBIy2np1h5Fr0gRyrsw\n"
    "BYZeqeg6PErmfMYPJJglA5KafJujtgU/MfYp/XYOkHvyeiVgLTGw+0amCb2bM8r7\n"
    "YId2PqXyIQKBgGc8S8n4BwHCOERO/KroZxEEe17CzWWp21wjzbZPKGwPGKyXoR13\n"
    "bnWf4+J1IXvUzYnr5ErjuZprxoEOXhlF6NFulmKDgIR/T2GZMdw4K/CnBvk1lFPd\n"
    "HboCNkld0CWkSfsVOsjht2twDMD/sp++nXQ8sskOkGuDK5C1NXIpMWb5AoGAZahB\n"
    "fU6buMBhD+6bTElQiZWqmXnK+uaWf1OCDsL7S7qXxps0HyKcv51AWFU3CiDfP3tz\n"
    "eO4sYh1G8X3KiWvtddfQVgDkA+IYR4+tv9ELeE576e8xukT6+aBIqh2CMU5h1HRq\n"
    "GLxgucJoGrhqBxh3nIT4tdap4H7f9JMVGvj4Y6ECgYA2UgVreWRqc1gSCTNyJPZx\n"
    "xtFsxhPRNkb1Ab0z1JAjnvpewbCw9oVUWbSOWvmATI0dPJgvN/j4OYrj2hBCpBJ8\n"
    "JDBMW7g4Hg9U/qK8NkLYWyckXmhO8HbQlYGAx5ra1PUsiPl7UdnDLRCHh8Xiaa4S\n"
    "kJXcjV4r+dKeIMWhPAAOvw==\n"
    "-----END PRIVATE KEY-----\n";
}  // namespace TestCert
namespace {
constexpr int BACKLOG = 16;
constexpr int READ_BUF_SIZE = 4096;
// Cap for the non-blocking TLS handshake poll; a client speaking an
// unsupported protocol (e.g. TLCP only) is given up after this elapsed.
constexpr int TLS_HANDSHAKE_TIMEOUT_MS = 5000;

// Parse the request-target path out of a request line like "GET /a/b?x=1 HTTP/1.1".
std::string ParseRequestPath(const std::string &req)
{
    // Find the first request line.
    auto sp = req.find(' ');
    if (sp == std::string::npos) {
        return "/";
    }
    auto start = sp + 1;
    auto sp2 = req.find(' ', start);
    if (sp2 == std::string::npos) {
        return req.substr(start);
    }
    return req.substr(start, sp2 - start);
}

std::string StatusReason(int status)
{
    switch (status) {
        case HTTP_STATUS_OK: return "OK";
        case HTTP_STATUS_NOT_FOUND: return "Not Found";
        case HTTP_STATUS_FOUND: return "Found";
        case HTTP_STATUS_UNAUTHORIZED: return "Unauthorized";
        default: return "OK";
    }
}

// Chunk-encode `body` into the HTTP chunked transfer format.
std::string ChunkEncode(const std::string &body)
{
    std::ostringstream out;
    const size_t chunkSize = 512;
    for (size_t i = 0; i < body.size(); i += chunkSize) {
        size_t n = std::min(chunkSize, body.size() - i);
        out << std::hex << n << "\r\n";
        out.write(body.data() + i, n);
        out << "\r\n";
    }
    out << "0\r\n\r\n";
    return out.str();
}
}  // namespace

MockHttpServer::MockHttpServer()
    : listenFd_(-1), port_(0), tls_(false), running_(false), sslCtx_(nullptr) {}

MockHttpServer::~MockHttpServer()
{
    running_ = false;
    if (listenFd_ >= 0) {
        shutdown(listenFd_, SHUT_RDWR);  // unblock accept()
        close(listenFd_);
        listenFd_ = -1;
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    if (sslCtx_ != nullptr) {
        SSL_CTX_free(static_cast<SSL_CTX *>(sslCtx_));
        sslCtx_ = nullptr;
    }
}

void MockHttpServer::AddRoute(const std::string &path, int status, const std::string &body,
                              const std::string &contentType, bool chunked)
{
    routes_.emplace_back(path, MockRoute{status, body, contentType, chunked});
}

std::string MockHttpServer::GetBaseUrl() const
{
    std::string scheme = tls_ ? "https" : "http";
    return scheme + "://127.0.0.1:" + std::to_string(port_);
}

std::string MockHttpServer::BuildResponse(const std::string &requestPath) const
{
    // First-registered route whose path is found in the request path wins.
    const MockRoute *hit = nullptr;
    for (const auto &entry : routes_) {
        if (requestPath.find(entry.first) != std::string::npos) {
            hit = &entry.second;
            break;
        }
    }
    if (hit == nullptr) {
        // Default: 404 with non-empty body so progress callbacks still fire.
        std::string body = "Not Found";
        std::ostringstream h;
        h << "HTTP/1.1 " << HTTP_STATUS_NOT_FOUND << " Not Found\r\nContent-Type: text/plain\r\n"
             "Content-Length: "
          << body.size() << "\r\nConnection: close\r\n\r\n" << body;
        return h.str();
    }
    std::ostringstream h;
    h << "HTTP/1.1 " << hit->status << " " << StatusReason(hit->status) << "\r\n"
      << "Content-Type: " << hit->contentType << "\r\n";
    if (hit->chunked) {
        h << "Transfer-Encoding: chunked\r\nConnection: close\r\n\r\n";
        return h.str() + ChunkEncode(hit->body);
    }
    h << "Content-Length: " << hit->body.size() << "\r\nConnection: close\r\n\r\n";
    return h.str() + hit->body;
}

void MockHttpServer::Start()
{
    tls_ = false;
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0;  // OS assigns the port.
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(listenFd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(listenFd_);
        listenFd_ = -1;
        return;
    }
    sockaddr_in actual{};
    socklen_t len = sizeof(actual);
    getsockname(listenFd_, reinterpret_cast<sockaddr *>(&actual), &len);
    port_ = ntohs(actual.sin_port);
    listen(listenFd_, BACKLOG);
    running_ = true;
    thread_ = std::thread(&MockHttpServer::RunLoop, this, listenFd_, false);
}

bool MockHttpServer::WriteTestCaFile(const std::string &path)
{
    std::ofstream out(path, std::ios::trunc | std::ios::binary);
    if (!out.is_open()) {
        return false;
    }
    out << TestCert::CA_PEM;
    return out.good();
}

void MockHttpServer::StartTls()
{
    tls_ = true;
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == nullptr) {
        return;
    }
    // Relax TLS version so clients negotiating TLS 1.2 still succeed.
    SSL_CTX_set_min_proto_version(ctx, TLS1_VERSION);
    // Load the embedded certificate and key from memory (no files needed).
    BIO *certBio = BIO_new_mem_buf(TestCert::SERVER_CERT_PEM, -1);
    X509 *cert = certBio != nullptr ? PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr) : nullptr;
    BIO *keyBio = BIO_new_mem_buf(TestCert::SERVER_KEY_PEM, -1);
    EVP_PKEY *key = keyBio != nullptr ? PEM_read_bio_PrivateKey(keyBio, nullptr, nullptr, nullptr) : nullptr;
    bool loaded = cert != nullptr && key != nullptr && SSL_CTX_use_certificate(ctx, cert) == 1 &&
                  SSL_CTX_use_PrivateKey(ctx, key) == 1;
    X509_free(cert);
    EVP_PKEY_free(key);
    BIO_free(certBio);
    BIO_free(keyBio);
    if (!loaded) {
        SSL_CTX_free(ctx);
        return;
    }
    sslCtx_ = ctx;

    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ < 0) {
        SSL_CTX_free(ctx);
        sslCtx_ = nullptr;
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0;
    int opt = 1;
    setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (bind(listenFd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        close(listenFd_);
        listenFd_ = -1;
        SSL_CTX_free(ctx);
        sslCtx_ = nullptr;
        return;
    }
    sockaddr_in actual{};
    socklen_t len = sizeof(actual);
    getsockname(listenFd_, reinterpret_cast<sockaddr *>(&actual), &len);
    port_ = ntohs(actual.sin_port);
    listen(listenFd_, BACKLOG);
    running_ = true;
    thread_ = std::thread(&MockHttpServer::RunLoop, this, listenFd_, true);
}

namespace {
// Drives one non-blocking SSL_accept attempt for HandshakeTls. Returns
// true when the handshake loop should keep retrying (SSL_accept wants more
// data and the poll for it succeeds within the handshake timeout); returns
// false when the handshake finished, hit a hard error, or the poll timed
// out (client speaks an unsupported protocol, e.g. TLCP).
bool SslAcceptStep(SSL *ssl, int clientFd)
{
    int ret = SSL_accept(ssl);
    if (ret > 0) {
        return false;  // handshake done
    }
    int err = SSL_get_error(ssl, ret);
    if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
        return false;  // hard error
    }
    pollfd pfd{};
    pfd.fd = clientFd;
    pfd.events = (err == SSL_ERROR_WANT_READ) ? POLLIN : POLLOUT;
    pfd.revents = 0;
    return poll(&pfd, 1, TLS_HANDSHAKE_TIMEOUT_MS) > 0;
}
}  // namespace

void *MockHttpServer::HandshakeTls(int clientFd, void *ctx)
{
    SSL_CTX *sslCtx = static_cast<SSL_CTX *>(ctx);
    SSL *ssl = SSL_new(sslCtx);
    if (ssl == nullptr) {
        return nullptr;
    }
    SSL_set_fd(ssl, clientFd);
    // Non-blocking handshake so a client speaking an unsupported protocol
    // (e.g. TLCP) cannot block the accept loop. Each SslAcceptStep performs
    // one attempt plus the poll for more data; the loop keeps going only
    // while the step reports the handshake is still pending.
    int flags = fcntl(clientFd, F_GETFL, 0);
    fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);
    while (SslAcceptStep(ssl, clientFd)) {
        // Retry: SSL_accept wants more data and it arrived in time.
    }
    fcntl(clientFd, F_SETFL, flags);  // restore blocking mode
    if (SSL_is_init_finished(ssl) != 1) {
        SSL_free(ssl);
        return nullptr;
    }
    return ssl;
}

void MockHttpServer::HandleConnection(int clientFd, void *sslPtr)
{
    SSL *ssl = static_cast<SSL *>(sslPtr);
    std::string req;
    char buf[READ_BUF_SIZE];
    // Read until the peer closes or the end of the HTTP headers is seen
    // (that is enough for routing).
    bool reading = true;
    while (reading) {
        int n = ssl != nullptr ? SSL_read(ssl, buf, sizeof(buf)) : recv(clientFd, buf, sizeof(buf), 0);
        if (n <= 0) {
            reading = false;
        } else {
            req.append(buf, n);
            if (req.find("\r\n\r\n") != std::string::npos) {
                reading = false;  // end of headers is enough for routing
            }
        }
    }
    std::string path = ParseRequestPath(req);
    std::string response = BuildResponse(path);
    if (ssl != nullptr) {
        SSL_write(ssl, response.data(), static_cast<int>(response.size()));
        SSL_shutdown(ssl);
        SSL_free(ssl);
    } else {
        send(clientFd, response.data(), response.size(), 0);
    }
    close(clientFd);
}

void MockHttpServer::RunLoop(int listenFd, bool tls)
{
    void *ctx = tls ? sslCtx_ : nullptr;
    while (running_) {
        sockaddr_in client{};
        socklen_t clen = sizeof(client);
        int clientFd = accept(listenFd, reinterpret_cast<sockaddr *>(&client), &clen);
        if (clientFd < 0) {
            break;  // shutdown() in destructor unblocks accept().
        }
        void *ssl = (ctx != nullptr) ? HandshakeTls(clientFd, ctx) : nullptr;
        if (ctx != nullptr && ssl == nullptr) {
            // TLS handshake failed (e.g. client spoke TLCP only); the
            // connection is torn down and the client observes a fail callback.
            close(clientFd);
            continue;
        }
        // Handle this connection on a detached thread so concurrent loads work.
        std::thread(&MockHttpServer::HandleConnection, this, clientFd, ssl).detach();
    }
}
}  // namespace OHOS::Request::Test
