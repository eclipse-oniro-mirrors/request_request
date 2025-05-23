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

use std::fs::File;
use std::os::fd::AsRawFd;
use std::os::unix::io::FromRawFd;

use ipc::parcel::MsgParcel;
use ipc::{IpcResult, IpcStatusCode};

use crate::error::ErrorCode;
use crate::service::RequestServiceStub;

impl RequestServiceStub {
    pub(crate) fn open_channel(&self, reply: &mut MsgParcel) -> IpcResult<()> {
        let pid = ipc::Skeleton::calling_pid();
        info!("Service open_channel pid {}", pid);
        match self.client_manager.open_channel(pid) {
            Ok(ud_fd) => {
                // `as_raw_fd` does not track the ownership or life cycle of this fd.
                let fd = ud_fd.as_raw_fd();
                let file = unsafe { File::from_raw_fd(fd) };
                info!("End open_channel fd {}", fd);
                reply.write(&(ErrorCode::ErrOk as i32))?;
                reply.write_file(file)?;
                Ok(())
            }
            Err(err) => {
                error!("End Service open_channel, failed: {:?}", err);
                sys_event!(
                    ExecError,
                    DfxCode::INVALID_IPC_MESSAGE_A26,
                    &format!("End Service open_channel, failed: {:?}", err)
                );
                reply.write(&(ErrorCode::ParameterCheck as i32))?;
                Err(IpcStatusCode::Failed)
            }
        }
    }
}
