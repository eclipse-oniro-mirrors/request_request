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

#ifndef VPN_DNS_RESOLVER_H
#define VPN_DNS_RESOLVER_H

#include <stdint.h>

#include "cxx.h"

namespace OHOS::Request {
// System-API adapters for the vpn dns source selection. The policy itself -
// scope matching, the unique-cover rule, the fallback assembly - lives on
// the Rust side (services/src/utils/vpn_dns.rs); this header only reaches
// the platform services the Rust core must not touch directly.

// Shared with Rust via the cxx bridge (services/src/utils/mod.rs): the raw,
// unvalidated config of one connected vpn. Defined by the generated bridge
// header; only forward-declared here.
struct VpnScope;

// Resolves the task's owning OS account from the raw uid via the os_account
// API (the uid-to-account mapping is a system allocation policy). Returns
// -1 when the lookup fails.
int32_t GetOsAccountLocalIdFromUid(uint32_t uid);

// Realtime query of the user's connected vpns straight from the vpn
// service, nothing cached: the scope lists and dns lists stay raw - the
// cleaning happens at the musl-equivalent ingestion site.
void QueryConnectedVpnScopes(int32_t user, rust::vec<VpnScope> &vpnScopes);

// The default network's link properties carry the DHCP-provided DNS
// servers: the only source on forms without netmanager_ext, and the
// fallback everywhere else.
void GetDefaultNetworkDnsServers(rust::vec<rust::string> &dnsServers);
} // namespace OHOS::Request

#endif // VPN_DNS_RESOLVER_H
