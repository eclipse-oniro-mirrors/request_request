#!/bin/bash
# Copyright (C) 2026 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Generate a self-signed CA + server cert pair for the HTTPS mock server.
# The server cert SAN covers 127.0.0.1/localhost — the in-process mock
# address — so curl's VERIFYHOST=2 check passes.
#
# The consumed artifacts are NOT checked in as files: paste the generated
# test_ca.pem / test_server_cert.pem / test_server_key.pem contents into the
# TestCert constants in test/unittest/common/mock_server/src/mock_http_server.cpp.
# test_ca_key.pem and server.csr are generation intermediates and are never
# stored anywhere.
#
# Usage: bash gen_certs.sh
set -e
cd "$(dirname "$0")"

# 1. Self-signed CA (test_ca.pem) — used as PreloadOptions.caPath on the client.
openssl genrsa -out test_ca_key.pem 2048 2>/dev/null
openssl req -x509 -new -nodes -key test_ca_key.pem -sha256 -days 3650 \
  -subj "/CN=Test Preload CA" -out test_ca.pem

# 2. Server cert signed by the CA — used by MockHttpServer::StartTls().
openssl genrsa -out test_server_key.pem 2048 2>/dev/null
openssl req -new -key test_server_key.pem -subj "/CN=localhost" -out server.csr
openssl x509 -req -in server.csr -CA test_ca.pem -CAkey test_ca_key.pem \
  -CAcreateserial -out test_server_cert.pem -days 3650 -sha256 \
  -extfile server_cert_ext.cnf -extensions v3_ext

echo "Certs generated. Paste the three PEM contents into TestCert in"
echo "test/unittest/common/mock_server/src/mock_http_server.cpp:"
echo "  test_ca.pem              -> TestCert::CA_PEM"
echo "  test_server_cert.pem     -> TestCert::SERVER_CERT_PEM"
echo "  test_server_key.pem      -> TestCert::SERVER_KEY_PEM"
