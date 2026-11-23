// Copyright (C) 2024 Huawei Device Co., Ltd.
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

use super::*;

/// SHA-256 renders as a 256-bit digest, i.e. exactly 64 hex characters.
const SHA256_HEX_LEN: usize = 64;

/// HiLog domain of the request subsystem, as documented on hilog_print.
const TEST_LOG_DOMAIN: u32 = 0xD001100;

// @tc.name: ut_wrapper_log_type_values
// @tc.desc: Verify LogType enum has correct values
// @tc.precon: NA
// @tc.step: 1. Check the values of LogType variants
// @tc.expect: All LogType variants have expected integer values
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 0
#[test]
fn ut_wrapper_log_type_values() {
    assert_eq!(ffi::LogType::LOG_TYPE_MIN.repr, 0);
    assert_eq!(ffi::LogType::LOG_APP.repr, 0);
    assert_eq!(ffi::LogType::LOG_INIT.repr, 1);
    assert_eq!(ffi::LogType::LOG_CORE.repr, 3);
    assert_eq!(ffi::LogType::LOG_KMSG.repr, 4);
    assert_eq!(ffi::LogType::LOG_ONLY_PRERELEASE.repr, 5);
    assert_eq!(ffi::LogType::LOG_TYPE_MAX.repr, 6);
}

// @tc.name: ut_wrapper_log_level_values
// @tc.desc: Verify LogLevel enum has correct values
// @tc.precon: NA
// @tc.step: 1. Check the values of LogLevel variants
// @tc.expect: All LogLevel variants have expected integer values
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 0
#[test]
fn ut_wrapper_log_level_values() {
    assert_eq!(ffi::LogLevel::LOG_LEVEL_MIN.repr, 0);
    assert_eq!(ffi::LogLevel::LOG_DEBUG.repr, 3);
    assert_eq!(ffi::LogLevel::LOG_INFO.repr, 4);
    assert_eq!(ffi::LogLevel::LOG_WARN.repr, 5);
    assert_eq!(ffi::LogLevel::LOG_ERROR.repr, 6);
    assert_eq!(ffi::LogLevel::LOG_FATAL.repr, 7);
    assert_eq!(ffi::LogLevel::LOG_LEVEL_MAX.repr, 8);
}

// @tc.name: ut_wrapper_sha256_valid_input
// @tc.desc: Test SHA256 against the fixed known-answer vector of the
//           11-byte ASCII message "hello world"
// @tc.precon: NA
// @tc.step: 1. Call SHA256 with "hello world"
//           2. Compare the result with the published SHA-256 value of that
//              message
// @tc.expect: SHA256 returns the exact published hex digest
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 1
#[test]
fn ut_wrapper_sha256_valid_input() {
    let result = SHA256("hello world");
    assert_eq!(
        result,
        "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9"
    );
}

// @tc.name: ut_wrapper_sha256_empty_input
// @tc.desc: Test SHA256 against the fixed known-answer vector of the empty
//           message (the FIPS 180-4 boundary case)
// @tc.precon: NA
// @tc.step: 1. Call SHA256 with an empty string
//           2. Compare the result with the published SHA-256 value of the
//              empty message
// @tc.expect: SHA256 returns the exact published hex digest
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 1
#[test]
fn ut_wrapper_sha256_empty_input() {
    let result = SHA256("");
    assert_eq!(
        result,
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    );
}

// @tc.name: ut_wrapper_sha256_abc_input
// @tc.desc: Test SHA256 against the FIPS 180-4 example vector "abc"
// @tc.precon: NA
// @tc.step: 1. Call SHA256 with "abc"
//           2. Compare the result with the published SHA-256 value of that
//              message
// @tc.expect: SHA256 returns the exact published hex digest
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 1
#[test]
fn ut_wrapper_sha256_abc_input() {
    let result = SHA256("abc");
    assert_eq!(
        result,
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
    );
}

// @tc.name: ut_wrapper_sha256_long_input
// @tc.desc: Test SHA256 with a multi-block input spanning many hash rounds
// @tc.precon: NA
// @tc.step: 1. Build a 10000-character input string
//           2. Call SHA256 with this input
//           3. Check the result shape: fixed hex digest length and hex
//              alphabet
// @tc.expect: SHA256 returns 64 lowercase-friendly hex characters
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 2
#[test]
fn ut_wrapper_sha256_long_input() {
    let long_input = "a".repeat(10000);
    let result = SHA256(&long_input);
    assert_eq!(result.len(), SHA256_HEX_LEN);
    assert!(result.chars().all(|c| c.is_ascii_hexdigit()));
}

// @tc.name: ut_wrapper_hilog_print_basic
// @tc.desc: Test basic functionality of hilog_print
// @tc.precon: NA
// @tc.step: 1. Call hilog_print with sample parameters
// @tc.expect: Function executes without panicking
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 1
#[test]
fn ut_wrapper_hilog_print_basic() {
    hilog_print(
        ffi::LogLevel::LOG_INFO,
        TEST_LOG_DOMAIN,
        "test_tag",
        "Test log message".to_string(),
    );
}
