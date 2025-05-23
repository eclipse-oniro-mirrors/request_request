// Copyright (C) 2023 Huawei Device Co., Ltd.
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

pub(crate) struct QosChanges {
    pub(crate) download: Option<Vec<QosDirection>>,
    pub(crate) upload: Option<Vec<QosDirection>>,
}

impl QosChanges {
    pub(crate) fn new() -> Self {
        Self {
            upload: None,
            download: None,
        }
    }
}
#[derive(Debug)]
pub(crate) struct QosDirection {
    uid: u64,
    task_id: u32,
    direction: QosLevel,
}

impl QosDirection {
    pub(crate) fn uid(&self) -> u64 {
        self.uid
    }

    pub(crate) fn task_id(&self) -> u32 {
        self.task_id
    }

    pub(crate) fn direction(&self) -> QosLevel {
        self.direction
    }

    pub(crate) fn new(uid: u64, task_id: u32, direction: QosLevel) -> Self {
        Self {
            uid,
            task_id,
            direction,
        }
    }
}

// QosLevel's enum value means max speed (B/s)
#[derive(PartialEq, Eq, Clone, Copy, Debug)]
pub(crate) enum QosLevel {
    High = 0,
    Low = 400 * 1024,
    Middle = 800 * 1024,
}
