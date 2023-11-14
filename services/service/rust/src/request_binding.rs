/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//! rust to C++ interface
#![allow(unused_variables)]
// C interface for check permission
use super::{enumration::*, progress::RequestTaskMsg, form_item::CFileSpec,
            form_item::CFormItem, task_info::*, task_config::*, c_string_wrapper::*, filter::*};
use std::ffi::{c_char, c_void};

type APPSTATECB = extern "C" fn(i32, i32);
type NETWORKCB = extern "C" fn();

extern "C" {
    pub fn RequestCheckPermission(tokenId: u64, permission: CStringWrapper) -> bool;
    pub fn RequestInitServiceHandler();
    pub fn RequestPostTask(f: extern "C" fn());
    pub fn RequestBackgroundNotify(
        msg: RequestTaskMsg,
        path: *const c_char,
        pathLen: i32,
        percent: u32,
    );
    pub fn IsOnline() -> bool;
    pub fn RegisterNetworkCallback(f: NETWORKCB);
    pub fn RegisterAPPStateCallback(f: APPSTATECB);
    pub fn GetNetworkInfo() -> *const NetworkInfo;
    pub fn GetTopBundleName() -> CStringWrapper;
    pub fn DeleteCTaskInfo(ptr: *const CTaskInfo);
    pub fn DeleteCTaskConfig(ptr: *const CTaskConfig);
    pub fn DeleteCTaskConfigs(ptr: *const*const CTaskConfig);
    pub fn DeleteCStringPtr(ptr: *const CStringWrapper);
    pub fn DeleteChar(ptr: *const c_char);
    pub fn DeleteCFormItem(ptr: *const CFormItem);
    pub fn DeleteCFileSpec(ptr: *const CFileSpec);
    pub fn DeleteCEachFileStatus(ptr: *const CEachFileStatus);
    pub fn DeleteCVectorWrapper(ptr: *const u32);
    pub fn HasRequestTaskRecord(taskId: u32) -> bool;
    pub fn RecordRequestTaskInfo(taskInfo: *const CTaskInfo) -> bool;
    pub fn UpdateRequestTaskInfo(taskId: u32, updateInfo: *const CUpdateInfo) -> bool;
    pub fn Touch(taskId: u32, uid: u64, token: CStringWrapper) -> *const CTaskInfo;
    pub fn Query(taskId: u32, queryAction: Action) -> *const CTaskInfo;
    pub fn Search(filter: CFilter) -> CVectorWrapper;
    pub fn HasTaskConfigRecord(taskId: u32) -> bool;
    pub fn RecordRequestTaskConfig(taskConfig: *const CTaskConfig) -> bool;
    pub fn QueryAllTaskConfig() -> *const*const CTaskConfig;
    pub fn QueryTaskConfigLen() -> i32;
    pub fn CleanTaskConfigTable(taskId: u32, uid: u64) -> bool;
    pub fn RequestIsSystemAPI(tokenId: u64) -> bool;
    pub fn GetCallingBundle(tokenId: u64) -> CStringWrapper;
    pub fn PublishStateChangeEvents(bundleName: *const c_char, bundleNameLen: u32, taskId: u32, state: i32);
}
