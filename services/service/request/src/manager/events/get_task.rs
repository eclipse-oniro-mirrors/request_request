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

use crate::manager::TaskManager;
use crate::task::config::TaskConfig;

impl TaskManager {
    pub(crate) fn get_task_api(&self, uid: u64, task_id: u32, token: String) -> Option<TaskConfig> {
        debug!("TaskManager get a task, uid:{}, task_id:{}", uid, task_id);

        match self.get_task(uid, task_id) {
            Some(value) => {
                debug!("found task in task_map");
                if value.conf.token.eq(token.as_str()) {
                    return Some(value.conf.clone());
                }
                None
            }
            None => {
                debug!("get task not in task_map");
                if let Some(config_map) = self.query_all_task_config() {
                    if let Some(config) = config_map.get(&task_id) {
                        return Some(config.clone());
                    }
                }
                None
            }
        }
    }
}