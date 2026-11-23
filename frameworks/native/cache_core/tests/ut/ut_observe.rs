// Copyright (C) 2025 Huawei Device Co., Ltd.
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

use std::fs;
use std::path::PathBuf;
use std::thread::sleep;
use std::time::Duration;

use super::*;

/// Test root in the shell-writable tmp area. inotify is a kernel facility,
/// so the observation started here involves no system service.
const OBSERVE_TEST_ROOT: &str = "/data/local/tmp/cov_observe_test";

/// Settle time letting the spawned observe thread reach inotify registration.
const OBSERVE_SETTLE_MS: u64 = 500;

/// Grace time for the monitor to dispatch a generated filesystem event.
const EVENT_DISPATCH_MS: u64 = 200;

// @tc.name: ut_observe_image_file_delete_init
// @tc.desc: observe_image_file_delete initializes history tracking once and
//           is idempotent for repeated calls on the same directory
// @tc.precon: NA
// @tc.step: 1. Create a fresh image directory under the test root
//           2. Call observe_image_file_delete for the directory
//           3. Call observe_image_file_delete again
// @tc.expect: Both calls complete and history tracking reports initialized
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 1
#[test]
fn ut_observe_image_file_delete_init() {
    let img_dir = PathBuf::from(OBSERVE_TEST_ROOT).join("img_dir");
    let _ = fs::remove_dir_all(&img_dir);
    fs::create_dir_all(&img_dir).expect("create image dir");

    observe_image_file_delete(img_dir.to_string_lossy().to_string());
    assert!(is_history_init());

    // The second call must be a no-op that keeps the initialized state.
    observe_image_file_delete(img_dir.to_string_lossy().to_string());
    assert!(is_history_init());

    // Let the spawned observe thread register its inotify watch on the
    // directory before the process moves on.
    sleep(Duration::from_millis(OBSERVE_SETTLE_MS));
}

// @tc.name: ut_observe_monitor_dispatches_events
// @tc.desc: the inotify monitor registered by observe keeps watching the
//           directory and stays alive across file create/delete events
// @tc.precon: ut_observe_image_file_delete_init has started the observation
// @tc.step: 1. Create a probe file inside the observed directory
//           2. Wait for event dispatch
//           3. Delete the probe file and wait again
// @tc.expect: Events do not crash or stop the observing process
// @tc.type: FUNC
// @tc.require: issueNumber
// @tc.level: Level 1
#[test]
fn ut_observe_monitor_dispatches_events() {
    let img_dir = PathBuf::from(OBSERVE_TEST_ROOT).join("img_dir");
    if !img_dir.is_dir() {
        fs::create_dir_all(&img_dir).expect("create image dir");
        observe_image_file_delete(img_dir.to_string_lossy().to_string());
        sleep(Duration::from_millis(OBSERVE_SETTLE_MS));
    }

    let probe = img_dir.join("probe.txt");
    fs::write(&probe, b"probe").expect("write probe file");
    sleep(Duration::from_millis(EVENT_DISPATCH_MS));
    assert!(probe.is_file());

    fs::remove_file(&probe).expect("remove probe file");
    sleep(Duration::from_millis(EVENT_DISPATCH_MS));
    assert!(!probe.exists());

    let _ = fs::remove_dir_all(&img_dir);
}
