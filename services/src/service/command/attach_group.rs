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

use ipc::parcel::MsgParcel;
use ipc::{IpcResult, IpcStatusCode};

use crate::error::ErrorCode;
use crate::manage::events::TaskManagerEvent;
use crate::service::RequestServiceStub;

impl RequestServiceStub {
    pub(crate) fn attach_group(
        &self,
        data: &mut MsgParcel,
        reply: &mut MsgParcel,
    ) -> IpcResult<()> {
        let Ok(group_id) = data.read::<String>()?.parse::<u32>() else {
            error!("End Service attach_group, group_id, failed: group_id not valid",);
            reply.write(&(ErrorCode::GroupNotFound as i32))?;
            return Ok(());
        };
        let task_ids = data.read::<Vec<String>>()?;

        let uid = ipc::Skeleton::calling_uid();

        let mut parse_ids = Vec::with_capacity(task_ids.len());

        for task_id in task_ids.iter() {
            let Ok(task_id) = task_id.parse::<u32>() else {
                error!("End Service attach_group, task_id, failed: task_id not valid");
                reply.write(&(ErrorCode::TaskNotFound as i32))?;
                return Ok(());
            };
            if !self.check_task_uid(task_id, uid) {
                error!(
                    "End Service attach_group, task_id: {}, failed: task_id not belong to uid",
                    task_id
                );
                reply.write(&(ErrorCode::TaskNotFound as i32))?;
                return Ok(());
            }
            parse_ids.push(task_id);
        }
        let (event, rx) = TaskManagerEvent::attach_group(uid, parse_ids, group_id);
        if !self.task_manager.lock().unwrap().send_event(event) {
            return Err(IpcStatusCode::Failed);
        }

        let ret = match rx.get() {
            Some(ret) => ret,
            None => {
                error!(
                    "End Service attach_group, task_id: {:?}, group_id: {}, failed: receives ret failed",
                    task_ids, group_id
                );
                ErrorCode::Other
            }
        };
        if ret != ErrorCode::ErrOk {
            error!(
                "End Service attach_group, task_id: {:?}, group_id: {}, failed: ret is not ErrOk",
                task_ids, group_id
            );
        }
        reply.write(&(ret as i32))?;
        Ok(())
    }
}
