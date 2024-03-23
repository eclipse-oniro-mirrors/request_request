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

use ipc_rust::{get_calling_pid, BorrowedMsgParcel, IpcResult, IpcStatusCode};

use crate::error::ErrorCode;
use crate::service::ability::RequestAbility;
use crate::service::runcount::RunCountEvent;

pub(crate) struct UnsubRunCount;

impl UnsubRunCount {
    pub(crate) fn execute(
        _data: &BorrowedMsgParcel,
        reply: &mut BorrowedMsgParcel,
    ) -> IpcResult<()> {
        info!("Service runcount unsubscribe");
        let pid = get_calling_pid();
        // let uid = get_calling_uid();
        debug!("Service runcount unsubscribe: pid is {}", pid);

        let (event, rx) = RunCountEvent::unsub_runcount(pid);
        RequestAbility::runcount_manager().send_event(event);

        let ret = match rx.get() {
            Some(ret) => ret,
            None => {
                error!("Service runcount unsubscribe: receives ret failed");
                return Err(IpcStatusCode::Failed);
            }
        };
        reply.write(&(ret as i32))?;
        if ret != ErrorCode::ErrOk {
            error!(
                "Service runcount unsubscribe: on failed for ret is {}",
                ret as i32
            );
            return Err(IpcStatusCode::Failed);
        }
        Ok(())
    }
}