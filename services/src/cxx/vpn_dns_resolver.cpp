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

#include "vpn_dns_resolver.h"

#include <string>
#include <vector>

#include "cxx.h"
#include "log.h"
#include "net_conn_client.h"
#include "utils/mod.rs.h"
#ifdef REQUEST_SUPPORT_NETMANAGER_EXT
#include "errors.h"
#include "networkvpn_client.h"
#include "os_account_manager.h"
#endif

namespace OHOS::Request {
using namespace OHOS::NetManagerStandard;

namespace {
#ifdef REQUEST_SUPPORT_NETMANAGER_EXT
std::string JoinList(const std::vector<std::string> &items)
{
    std::string joined = "[";
    for (size_t i = 0; i < items.size(); ++i) {
        if (i != 0) {
            joined += ",";
        }
        joined += items[i];
    }
    joined += "]";
    return joined;
}

// The queried vpn's scope lists and dns list, as the vpn service handed
// them over.
void LogVpnConfig(size_t index, const VpnConfig &vpn)
{
    REQUEST_HILOGI("[VpnDns] vpn[%{public}zu]: dns=%{public}s accepted=%{public}s refused=%{public}s",
        index, JoinList(vpn.dnsAddresses_).c_str(), JoinList(vpn.acceptedApplications_).c_str(),
        JoinList(vpn.refusedApplications_).c_str());
}

// Realtime query: the connected-vpn list of one user straight from the vpn
// service, nothing cached. Every queried vpn is logged with its scope
// lists and dns list.
std::vector<sptr<VpnConfig>> QueryConnectedVpnList(int32_t userId)
{
    std::vector<sptr<VpnConfig>> vpnConfigList;
    int32_t ret = NetworkVpnClient::GetInstance().GetConnectedVpnConfigList(userId, vpnConfigList);
    if (ret != 0) {
        REQUEST_HILOGE("QueryConnectedVpnConfigList failed: userId=%{public}d err=%{public}d", userId, ret);
        return vpnConfigList;
    }
    for (size_t i = 0; i < vpnConfigList.size(); ++i) {
        if (vpnConfigList[i] != nullptr) {
            LogVpnConfig(i, *vpnConfigList[i]);
        }
    }
    return vpnConfigList;
}

#endif // REQUEST_SUPPORT_NETMANAGER_EXT
} // namespace

// Resolves the task's owning OS account from the raw uid via the os_account
// API (the uid-to-account mapping is a system allocation policy, not a
// local constant). Returns -1 when the lookup fails.
int32_t GetOsAccountLocalIdFromUid(uint32_t uid)
{
#ifdef REQUEST_SUPPORT_NETMANAGER_EXT
    int32_t user = -1;
    ErrCode ret = AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(static_cast<int32_t>(uid), user);
    if (ret != ERR_OK) {
        REQUEST_HILOGE("GetOsAccountLocalIdFromUid failed: uid=%{public}u err=%{public}d", uid, ret);
        return -1;
    }
    return user;
#else
    (void)uid;
    return -1;
#endif
}

// The task socket is fchowned to the task app's uid, so its packets route
// into the vpn whose uid ranges cover that user; the queried list already
// contains only vpns of that user. The scope decision itself lives on the
// Rust side (services/src/utils/vpn_dns.rs); this adapter hands over the
// raw configs.
void QueryConnectedVpnScopes(int32_t user, rust::vec<VpnScope> &vpnScopes)
{
#ifdef REQUEST_SUPPORT_NETMANAGER_EXT
    // realtime per task build: no cache, the query is the source of truth
    std::vector<sptr<VpnConfig>> vpnConfigList = QueryConnectedVpnList(user);
    for (const auto &vpn : vpnConfigList) {
        if (vpn == nullptr) {
            continue;
        }
        VpnScope scope;
        for (const auto &app : vpn->acceptedApplications_) {
            scope.accepted.push_back(rust::String(app));
        }
        for (const auto &app : vpn->refusedApplications_) {
            scope.refused.push_back(rust::String(app));
        }
        for (const auto &dns : vpn->dnsAddresses_) {
            scope.dns_servers.push_back(rust::String(dns));
        }
        vpnScopes.push_back(scope);
    }
#else
    (void)user;
    (void)vpnScopes;
#endif
}

// The default network's link properties carry the DHCP-provided DNS
// servers. This is the only source on forms without netmanager_ext, and the
// fallback everywhere else.
void GetDefaultNetworkDnsServers(rust::vec<rust::string> &dnsServers)
{
    NetHandle netHandle;
    int32_t ret = NetConnClient::GetInstance().GetDefaultNet(netHandle);
    if (ret != 0) {
        REQUEST_HILOGE("GetDefaultNet failed: %{public}d", ret);
        return;
    }
    NetLinkInfo linkInfo;
    ret = NetConnClient::GetInstance().GetConnectionProperties(netHandle, linkInfo);
    if (ret != 0) {
        REQUEST_HILOGE("GetConnectionProperties failed: %{public}d", ret);
        return;
    }
    for (const auto &dns : linkInfo.dnsList_) {
        dnsServers.push_back(rust::string(dns.address_));
    }
}
} // namespace OHOS::Request
