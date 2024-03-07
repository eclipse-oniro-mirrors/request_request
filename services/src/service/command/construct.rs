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

use std::collections::HashMap;
use std::fs::File;

use ipc_rust::{
    get_calling_token_id, get_calling_uid, BorrowedMsgParcel, IMsgParcel, IpcResult, IpcStatusCode,
};

use crate::error::ErrorCode;
use crate::manage::events::EventMessage;
use crate::service::ability::RequestAbility;
use crate::service::permission::PermissionChecker;
use crate::service::{get_calling_bundle, open_file_readonly, open_file_readwrite};
use crate::task::config::{Action, CommonTaskConfig, Network, TaskConfig, Version};
use crate::task::info::Mode;
use crate::utils::form_item::{FileSpec, FormItem};
use crate::utils::generate_task_id;

pub(crate) struct Construct;

impl Construct {
    pub(crate) fn execute(
        data: &BorrowedMsgParcel,
        reply: &mut BorrowedMsgParcel,
    ) -> IpcResult<()> {
        info!("Service construct");

        if !PermissionChecker::check_internet() {
            error!("Service construct: no INTERNET permission");
            reply.write(&(ErrorCode::Permission as i32))?;
            return Err(IpcStatusCode::Failed);
        }

        let action: u32 = data.read()?;
        let action: Action = Action::from(action as u8);

        let version: u32 = data.read()?;
        let version: Version = Version::from(version as u8);

        let mode: u32 = data.read()?;
        let mode: Mode = Mode::from(mode as u8);

        let cover: bool = data.read()?;

        let network: u32 = data.read()?;
        let network: Network = Network::from(network as u8);

        let metered: bool = data.read()?;

        let roaming: bool = data.read()?;

        let retry: bool = data.read()?;

        let redirect: bool = data.read()?;

        let background: bool = data.read()?;

        let index: u32 = data.read()?;

        let begins: i64 = data.read()?;

        let ends: i64 = data.read()?;

        let gauge: bool = data.read()?;

        let precise: bool = data.read()?;

        let priority: u32 = data.read()?;

        let url: String = data.read()?;

        let title: String = data.read()?;

        let method: String = data.read()?;

        let token: String = data.read()?;

        let description: String = data.read()?;

        let data_base: String = data.read()?;

        let bundle = get_calling_bundle();
        // Creates task_id here, move it to task_manager later?
        let task_id = generate_task_id();
        let uid = get_calling_uid();
        let token_id = get_calling_token_id();

        let mut certs_path = Vec::new();
        let certs_path_size: u32 = data.read()?;
        if certs_path_size > data.get_readable_bytes() {
            error!("Service construct: certs_path_size too large");
            reply.write(&(ErrorCode::IpcSizeTooLarge as i32))?;
            return Err(IpcStatusCode::Failed);
        }
        for _ in 0..certs_path_size {
            let cert_path: String = data.read()?;
            certs_path.push(cert_path);
        }

        let mut form_items = Vec::new();
        let form_size: u32 = data.read()?;
        if form_size > data.get_readable_bytes() {
            error!("Service construct: form_size too large");
            reply.write(&(ErrorCode::IpcSizeTooLarge as i32))?;
            return Err(IpcStatusCode::Failed);
        }
        for _ in 0..form_size {
            let name: String = data.read()?;
            let value: String = data.read()?;
            form_items.push(FormItem { name, value });
        }

        let mut files = Vec::<File>::new();
        let mut file_specs: Vec<FileSpec> = Vec::new();
        let file_size: u32 = data.read()?;
        if file_size > data.get_readable_bytes() {
            error!("Service construct: file_specs size too large");
            reply.write(&(ErrorCode::IpcSizeTooLarge as i32))?;
            return Err(IpcStatusCode::Failed);
        }
        for _ in 0..file_size {
            let name: String = data.read()?;
            let path: String = data.read()?;
            let file_name: String = data.read()?;
            let mime_type: String = data.read()?;
            if action == Action::UpLoad {
                let file = open_file_readonly(uid, &bundle, &path)?;
                files.push(file);
            } else {
                let file = open_file_readwrite(uid, &bundle, &path)?;
                files.push(file);
            }
            let _fd_error: i32 = data.read()?;
            file_specs.push(FileSpec {
                name,
                path,
                file_name,
                mime_type,
            });
        }

        // Response bodies fd.
        let body_file_size: u32 = data.read()?;
        if body_file_size > data.get_readable_bytes() {
            error!("Service construct: body_file size too large");
            reply.write(&(ErrorCode::IpcSizeTooLarge as i32))?;
            return Err(IpcStatusCode::Failed);
        }
        let mut body_files = Vec::new();
        let mut body_file_names: Vec<String> = Vec::new();
        for _ in 0..body_file_size {
            let file_name: String = data.read()?;
            let body_file = open_file_readwrite(uid, &bundle, &file_name)?;
            body_file_names.push(file_name);
            body_files.push(body_file);
        }

        let header_size: u32 = data.read()?;
        if header_size > data.get_readable_bytes() {
            error!("Service construct: header size too large");
            reply.write(&(ErrorCode::IpcSizeTooLarge as i32))?;
            return Err(IpcStatusCode::Failed);
        }
        let mut headers: HashMap<String, String> = HashMap::new();
        for _ in 0..header_size {
            let key: String = data.read()?;
            let value: String = data.read()?;
            headers.insert(key, value);
        }

        let extras_size: u32 = data.read()?;
        if extras_size > data.get_readable_bytes() {
            error!("Service construct: extras size too large");
            reply.write(&(ErrorCode::IpcSizeTooLarge as i32))?;
            return Err(IpcStatusCode::Failed);
        }
        let mut extras: HashMap<String, String> = HashMap::new();
        for _ in 0..extras_size {
            let key: String = data.read()?;
            let value: String = data.read()?;
            extras.insert(key, value);
        }

        let task_config = TaskConfig {
            bundle,
            url,
            title,
            description,
            method,
            headers,
            data: data_base,
            token,
            extras,
            version,
            form_items,
            file_specs,
            body_file_names,
            certs_path,
            common_data: CommonTaskConfig {
                task_id,
                uid,
                token_id,
                action,
                mode,
                cover,
                network,
                metered,
                roaming,
                retry,
                redirect,
                index,
                begins: begins as u64,
                ends,
                gauge,
                precise,
                priority,
                background,
            },
        };

        debug!("Service construct: task_config constructed");
        debug!("Service construct: target files {:?}", files);

        let (event, rx) = EventMessage::construct(task_config, files, body_files);
        if !RequestAbility::task_manager().send_event(event) {
            return Err(IpcStatusCode::Failed);
        }
        let ret = match rx.get() {
            Some(ret) => ret,
            None => {
                error!("Service construct: receives ret failed");
                return Err(IpcStatusCode::Failed);
            }
        };

        debug!("Service construct: construct event sent to manager");

        if version != Version::API10 {
            let (event, _) = EventMessage::start(uid, task_id);
            if !RequestAbility::task_manager().send_event(event) {
                return Err(IpcStatusCode::Failed);
            }
            debug!("Service construct: start event sent to manager");
        }

        reply.write(&(ret as i32))?;
        if ret != ErrorCode::ErrOk {
            error!("Service construct: construct task failed");
            return Err(IpcStatusCode::Failed);
        }
        debug!("Service construct: task id {}", task_id);
        reply.write(&(task_id as i32))?;
        Ok(())
    }
}