// Copyright (C) 2026 Huawei Device Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

//! DNS source selection for a task's HTTP client: which server list the
//! client resolves names with. The policy - scope matching, the
//! unique-cover rule, the fallback assembly - is pure Rust and unit-tests
//! without any system dependency; the C++ side
//! (services/src/cxx/vpn_dns_resolver.cpp) only adapters the platform
//! services this must not touch directly: os_account, the vpn service
//! query and the netconn default-network properties.

use super::ffi;

/// The scope decision mirrors NetVpnImpl (netmanager_ext) on its
/// judgments:
/// - the branch: an accepted list with effective entries covers only its listed
///   bundles, a refused list covers every bundle it does not list, and lists
///   without effective entries cover all bundles. The two lists are not
///   supported to be configured together: whatever the entries contain, a
///   config carrying content on both sides is invalid outright, covers no
///   bundle and the caller falls back to the default network - a deliberate
///   divergence from NetVpnImpl's accepted-wins routing branch;
/// - effectiveness: within a valid config, empty-string entries are dirty
///   values the service never resolves to a uid (IsGlobalVpn counts only
///   non-empty names, GetAppsUids silently drops names the bundle manager
///   rejects - "" always fails), so they never decide the branch and never
///   match a bundle; a list holding only them declares a global vpn, not an
///   empty whitelist covering nobody. They do not un-configure a side: the
///   both-lists check runs on the raw lists.
pub(crate) fn is_bundle_in_scope(accepted: &[String], refused: &[String], bundle: &str) -> bool {
    let matches = |apps: &[String]| apps.iter().any(|app| !app.is_empty() && app == bundle);
    // The lists are not supported to be configured together: content on
    // both sides, whatever it is, marks the config invalid - the default
    // network decides, nothing matches.
    if !accepted.is_empty() && !refused.is_empty() {
        return false;
    }
    if accepted.iter().any(|app| !app.is_empty()) {
        return matches(accepted);
    }
    !matches(refused)
}

/// Exactly one vpn may cover a bundle: overlapping scopes are ambiguous
/// and the caller falls back to the default network instead of guessing a
/// priority. Returns the covering index only when exactly one scope
/// covers.
pub(crate) fn find_unique_covering_vpn(scopes: &[ffi::VpnScope], bundle: &str) -> Option<usize> {
    let mut matched: Option<usize> = None;
    for (index, scope) in scopes.iter().enumerate() {
        if !is_bundle_in_scope(&scope.accepted, &scope.refused, bundle) {
            continue;
        }
        if matched.is_some() {
            return None;
        }
        matched = Some(index);
    }
    matched
}

/// Queries the DNS server list effective for the task bundle at client
/// build time: the covering vpn owned by the task user, otherwise the
/// default network's. Always succeeds; an empty list means no usable source
/// and nothing is passed to the client.
#[cfg(feature = "oh")]
pub(crate) fn get_dns_servers(bundle: &str, uid: u64) -> Vec<String> {
    let mut servers: Vec<String> = Vec::new();
    // No uid-to-user arithmetic here: the uid-to-account mapping is a
    // system allocation policy, resolved via the os_account API adapter.
    let user = ffi::GetOsAccountLocalIdFromUid(uid as u32);
    if user >= 0 {
        // realtime per task build: no cache, the query is the source of truth
        let mut scopes: Vec<ffi::VpnScope> = Vec::new();
        ffi::QueryConnectedVpnScopes(user, &mut scopes);
        if let Some(index) = find_unique_covering_vpn(&scopes, bundle) {
            servers = scopes[index].dns_servers.clone();
            info!(
                "[VpnDns] user {} bundle {} covered by vpn[{}], dns servers: {:?}",
                user, bundle, index, servers
            );
        }
    }
    if servers.is_empty() {
        ffi::GetDefaultNetworkDnsServers(&mut servers);
        info!(
            "[VpnDns] user {} bundle {} falls back to default network, dns servers: {:?}",
            user, bundle, servers
        );
    }
    servers
}
