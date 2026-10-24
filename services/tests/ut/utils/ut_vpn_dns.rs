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

use super::ffi::VpnScope;
use super::vpn_dns::{find_unique_covering_vpn, is_bundle_in_scope};

fn make_scope(accepted: &[&str], refused: &[&str]) -> VpnScope {
    VpnScope {
        accepted: accepted.iter().map(|name| name.to_string()).collect(),
        refused: refused.iter().map(|name| name.to_string()).collect(),
        dns_servers: Vec::new(),
    }
}

// @tc.name: ut_vpn_dns_accepted_list_covers_listed_bundle
// @tc.desc: a non-empty accepted list covers exactly its listed bundles
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with an accepted list of two bundles
// @tc.expect: listed bundles are covered, others are not
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_accepted_list_covers_listed_bundle() {
    let scope = make_scope(&["com.example.one", "com.example.two"], &[]);
    assert!(is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "com.example.one"
    ));
    assert!(is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "com.example.two"
    ));
    assert!(!is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "com.example.three"
    ));
}

// @tc.name: ut_vpn_dns_both_lists_cover_nothing
// @tc.desc: the trusted/blocked lists are documented as not configurable
// together, so a config carrying content on both sides is invalid and
// covers no bundle - the default network decides, even for a bundle listed
// in the accepted list
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with disjoint both-lists content and
// with the query bundle in both lists
// @tc.expect: no bundle is covered in either shape
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_both_lists_cover_nothing() {
    let disjoint = make_scope(&["com.example.one"], &["com.example.two"]);
    assert!(!is_bundle_in_scope(
        &disjoint.accepted,
        &disjoint.refused,
        "com.example.one"
    ));
    assert!(!is_bundle_in_scope(
        &disjoint.accepted,
        &disjoint.refused,
        "com.example.two"
    ));
    assert!(!is_bundle_in_scope(
        &disjoint.accepted,
        &disjoint.refused,
        "com.example.three"
    ));
    let overlapping = make_scope(
        &["com.example.one"],
        &["com.example.one", "com.example.two"],
    );
    assert!(!is_bundle_in_scope(
        &overlapping.accepted,
        &overlapping.refused,
        "com.example.one"
    ));
    assert!(!is_bundle_in_scope(
        &overlapping.accepted,
        &overlapping.refused,
        "com.example.two"
    ));
}

// @tc.name: ut_vpn_dns_accepted_list_is_case_sensitive
// @tc.desc: matching is exact: the service converts exactly these names to
// uids, so no case folding happens on this side either
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with case-folded and extended names
// @tc.expect: neither matches
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_accepted_list_is_case_sensitive() {
    let scope = make_scope(&["com.example.one"], &[]);
    assert!(!is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "Com.Example.One"
    ));
    assert!(!is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "com.example.one.extra"
    ));
}

// @tc.name: ut_vpn_dns_refused_list_excludes_listed_bundle
// @tc.desc: with an empty accepted list, a refused list covers every bundle
// not listed
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with a refused-list scope
// @tc.expect: the listed bundle is excluded, others are covered
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_refused_list_excludes_listed_bundle() {
    let scope = make_scope(&[], &["com.example.blocked"]);
    assert!(!is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "com.example.blocked"
    ));
    assert!(is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "com.example.allowed"
    ));
}

// @tc.name: ut_vpn_dns_empty_lists_cover_all_bundles
// @tc.desc: with both lists empty every bundle is covered, the service-side
// default branch
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with two empty lists
// @tc.expect: any bundle is covered
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_empty_lists_cover_all_bundles() {
    let scope = make_scope(&[], &[]);
    assert!(is_bundle_in_scope(
        &scope.accepted,
        &scope.refused,
        "com.example.anything"
    ));
}

// @tc.name: ut_vpn_dns_only_empty_entries_declare_global_vpn
// @tc.desc: empty-string entries are dirty values the service never
// resolves to a uid (IsGlobalVpn counts only non-empty names): a list
// holding only [""] behaves as an empty list - a global vpn - instead of
// an empty whitelist covering nobody
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with accepted=[""] and with
// refused=[""]
// @tc.expect: any bundle is covered in both shapes
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_only_empty_entries_declare_global_vpn() {
    let dirty_accepted = make_scope(&[""], &[]);
    assert!(is_bundle_in_scope(
        &dirty_accepted.accepted,
        &dirty_accepted.refused,
        "com.example.anything"
    ));
    let dirty_refused = make_scope(&[], &[""]);
    assert!(is_bundle_in_scope(
        &dirty_refused.accepted,
        &dirty_refused.refused,
        "com.example.anything"
    ));
}

// @tc.name: ut_vpn_dns_empty_string_entry_never_matches
// @tc.desc: a dirty empty-string entry neither creates the whitelist branch
// nor matches a bundle: ["", "com.example.one"] covers exactly
// com.example.one
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with a dirty accepted list and with
// a dirty refused list
// @tc.expect: only the effective entry decides coverage
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_empty_string_entry_never_matches() {
    let dirty_accepted = make_scope(&["", "com.example.one"], &[]);
    assert!(is_bundle_in_scope(
        &dirty_accepted.accepted,
        &dirty_accepted.refused,
        "com.example.one"
    ));
    assert!(!is_bundle_in_scope(
        &dirty_accepted.accepted,
        &dirty_accepted.refused,
        "com.example.two"
    ));
    let dirty_refused = make_scope(&[], &["", "com.example.blocked"]);
    assert!(!is_bundle_in_scope(
        &dirty_refused.accepted,
        &dirty_refused.refused,
        "com.example.blocked"
    ));
    assert!(is_bundle_in_scope(
        &dirty_refused.accepted,
        &dirty_refused.refused,
        "com.example.allowed"
    ));
}

// @tc.name: ut_vpn_dns_both_lists_rule_runs_on_raw_lists
// @tc.desc: the both-lists check sees the raw configured lists: dirty
// empty-string entries neither un-configure a side nor rescue the config -
// any content on both sides, even a lone "", marks the config invalid
// @tc.precon: NA
// @tc.step: 1. Call is_bundle_in_scope with dirty entries alongside
// effective content on the other side, and with a lone dirty entry on one
// side
// @tc.expect: no bundle is covered in every shape
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_both_lists_rule_runs_on_raw_lists() {
    let dirty_accepted = make_scope(&["", "com.example.one"], &["com.example.two"]);
    assert!(!is_bundle_in_scope(
        &dirty_accepted.accepted,
        &dirty_accepted.refused,
        "com.example.one"
    ));
    assert!(!is_bundle_in_scope(
        &dirty_accepted.accepted,
        &dirty_accepted.refused,
        "com.example.two"
    ));
    let only_dirty_accepted = make_scope(&[""], &["com.example.two"]);
    assert!(!is_bundle_in_scope(
        &only_dirty_accepted.accepted,
        &only_dirty_accepted.refused,
        "com.example.one"
    ));
    assert!(!is_bundle_in_scope(
        &only_dirty_accepted.accepted,
        &only_dirty_accepted.refused,
        "com.example.two"
    ));
    let only_dirty_refused = make_scope(&["com.example.one"], &[""]);
    assert!(!is_bundle_in_scope(
        &only_dirty_refused.accepted,
        &only_dirty_refused.refused,
        "com.example.one"
    ));
    assert!(!is_bundle_in_scope(
        &only_dirty_refused.accepted,
        &only_dirty_refused.refused,
        "com.example.two"
    ));
}

// @tc.name: ut_vpn_dns_unique_returns_sole_covering_index
// @tc.desc: the sole covering scope is returned by index with cover count 1
// @tc.precon: NA
// @tc.step: 1. Call find_unique_covering_vpn with three scopes where exactly
// one covers the bundle
// @tc.expect: the covering index
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_unique_returns_sole_covering_index() {
    // Scope 0 is a blacklist vpn refusing the query bundle itself, so it does
    // not cover it - a blacklist vpn refusing some other name would cover the
    // bundle and turn the overlap ambiguous instead.
    let scopes = vec![
        make_scope(&[], &["com.example.two"]),
        make_scope(&["com.example.two"], &[]),
        make_scope(&["com.example.three"], &[]),
    ];
    assert_eq!(
        find_unique_covering_vpn(&scopes, "com.example.two"),
        Some(1)
    );
}

// @tc.name: ut_vpn_dns_unique_falls_back_on_ambiguous_scope
// @tc.desc: overlapping scopes are ambiguous: no covering index is
// returned, so the caller falls back instead of guessing a priority
// @tc.precon: NA
// @tc.step: 1. Call find_unique_covering_vpn with two covering scopes
// @tc.expect: None
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_unique_falls_back_on_ambiguous_scope() {
    let scopes = vec![
        make_scope(&["com.example.one"], &[]),
        make_scope(&[], &["com.example.other"]),
    ];
    assert_eq!(find_unique_covering_vpn(&scopes, "com.example.one"), None);
}

// @tc.name: ut_vpn_dns_unique_falls_back_when_none_covers
// @tc.desc: no covering scope: no covering index is returned
// @tc.precon: NA
// @tc.step: 1. Call find_unique_covering_vpn with a bundle no scope covers
// @tc.expect: None
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_unique_falls_back_when_none_covers() {
    let scopes = vec![make_scope(&["com.example.one"], &[])];
    assert_eq!(find_unique_covering_vpn(&scopes, "com.example.two"), None);
}

// @tc.name: ut_vpn_dns_unique_handles_empty_scope_list
// @tc.desc: an empty scope list covers nothing: no covering index is
// returned
// @tc.precon: NA
// @tc.step: 1. Call find_unique_covering_vpn with no scopes
// @tc.expect: None
// @tc.type: FUNC
#[test]
fn ut_vpn_dns_unique_handles_empty_scope_list() {
    let scopes: Vec<VpnScope> = Vec::new();
    assert_eq!(find_unique_covering_vpn(&scopes, "com.example.one"), None);
}
