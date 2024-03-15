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

use std::ffi::CString;
use std::io::SeekFrom;
use std::ops::Deref;
use std::sync::atomic::{
    AtomicBool, AtomicI64, AtomicU32, AtomicU64, AtomicU8, AtomicUsize, Ordering,
};
use std::sync::{Arc, Mutex};
use std::thread::sleep;
use std::time::Duration;

use ylong_http_client::async_impl::{Body, Client, Request, RequestBuilder, Response};
use ylong_http_client::{ErrorKind, HttpClientError};
use ylong_runtime::io::{AsyncSeekExt, AsyncWriteExt};

use super::config::{Network, Version};
use super::download::download;
use super::ffi::{HasRequestTaskRecord, PublishStateChangeEvents};
use super::info::{CommonTaskInfo, Mode, State, TaskInfo, UpdateInfo};
use super::notify::{EachFileStatus, NotifyData, Progress};
use super::reason::Reason;
use super::upload::upload;
use crate::error::ErrorCode;
use crate::manage::keeper::SAKeeper;
use crate::manage::monitor::IsOnline;
use crate::manage::task_manager::SystemConfig;
use crate::service::ability::RequestAbility;
use crate::service::runcount::RunCountEvent;
use crate::task::client::build_client;
use crate::task::config::{Action, TaskConfig};
use crate::task::ffi::{
    GetNetworkInfo, RequestBackgroundNotify, RequestTaskMsg, UpdateRequestTask,
};
use crate::task::files::{AttachedFiles, Files};
use crate::task::notify::SubscribeType;
use crate::utils::c_wrapper::CStringWrapper;
use crate::utils::{get_current_timestamp, hashmap_to_string};

cfg_oh! {
    use crate::manage::notifier::Notifier;
}

const RETRY_INTERVAL: u64 = 20;

pub(crate) struct RequestTask {
    pub(crate) conf: TaskConfig,
    pub(crate) client: Client,
    pub(crate) files: Files,
    pub(crate) body_files: Files,
    pub(crate) ctime: u64,
    pub(crate) mime_type: Mutex<String>,
    pub(crate) progress: Mutex<Progress>,
    pub(crate) tries: AtomicU32,
    pub(crate) status: Mutex<TaskStatus>,
    pub(crate) retry: AtomicBool,
    pub(crate) get_file_info: AtomicBool,
    pub(crate) retry_for_request: AtomicBool,
    pub(crate) code: Mutex<Vec<Reason>>,
    pub(crate) background_notify_time: AtomicU64,
    pub(crate) file_total_size: AtomicI64,
    pub(crate) resume: AtomicBool,
    pub(crate) seek_flag: AtomicBool,
    pub(crate) range_request: AtomicBool,
    pub(crate) range_response: AtomicBool,
    pub(crate) restored: AtomicBool,
    pub(crate) skip_bytes: AtomicU64,
    pub(crate) upload_counts: AtomicUsize,
    pub(crate) rate_limiting: AtomicBool,
    #[allow(unused)]
    pub(crate) app_state: Arc<AtomicU8>,
    pub(crate) last_notify: AtomicU64,
}

impl RequestTask {
    pub(crate) fn new(
        config: TaskConfig,
        system: SystemConfig,
        app_state: Arc<AtomicU8>,
        info: Option<TaskInfo>,
    ) -> Result<RequestTask, ErrorCode> {
        if !check_configs(&config) {
            return Err(ErrorCode::Other);
        }

        let files = AttachedFiles::open(&config).map_err(|_| ErrorCode::FileOperationErr)?;

        let client = build_client(&config, &system).map_err(|_| ErrorCode::Other)?;

        let file_len = files.files.len();
        let action = config.common_data.action;
        let time = get_current_timestamp();

        let file_total_size = match action {
            Action::Upload => {
                let mut file_total_size = 0i64;
                // If the total size overflows, ignore it.
                for size in files.sizes.iter() {
                    file_total_size += *size;
                }
                file_total_size
            }
            Action::Download => info
                .as_ref()
                .map(|info| info.progress.sizes[0])
                .unwrap_or(-1),
            _ => unreachable!("Action::Any in RequestTask::new never reach"),
        };

        // If `TaskInfo` is provided, use data of it.
        let ctime = info
            .as_ref()
            .map(|info| info.common_data.ctime)
            .unwrap_or(time);
        let mime_type = info
            .as_ref()
            .map(|info| info.mime_type.clone())
            .unwrap_or_default();
        let tries = info
            .as_ref()
            .map(|info| info.common_data.tries)
            .unwrap_or(0);
        let upload_counts = info
            .as_ref()
            .map(|info| info.progress.common_data.index)
            .unwrap_or(0);
        let status = info
            .as_ref()
            .map(|info| TaskStatus {
                waiting_network_time: None,
                mtime: time,
                state: State::from(info.progress.common_data.state),
                reason: Reason::from(info.common_data.reason),
            })
            .unwrap_or(TaskStatus::new(time));
        let retry = info
            .as_ref()
            .map(|info| info.common_data.retry)
            .unwrap_or(false);
        let progress = info
            .map(|info| info.progress)
            .unwrap_or(Progress::new(files.sizes));

        Ok(RequestTask {
            conf: config,
            client,
            files: files.files,
            body_files: files.body_files,
            ctime,
            mime_type: Mutex::new(mime_type),
            progress: Mutex::new(progress),
            tries: AtomicU32::new(tries),
            status: Mutex::new(status),
            retry: AtomicBool::new(retry),
            get_file_info: AtomicBool::new(false),
            retry_for_request: AtomicBool::new(false),
            code: Mutex::new(vec![Reason::Default; file_len]),
            background_notify_time: AtomicU64::new(time),
            file_total_size: AtomicI64::new(file_total_size),
            resume: AtomicBool::new(false),
            seek_flag: AtomicBool::new(false),
            range_request: AtomicBool::new(false),
            range_response: AtomicBool::new(false),
            restored: AtomicBool::new(false),
            skip_bytes: AtomicU64::new(0),
            upload_counts: AtomicUsize::new(upload_counts),
            rate_limiting: AtomicBool::new(false),
            app_state,
            last_notify: AtomicU64::new(time),
        })
    }

    pub(crate) fn build_notify_data(&self) -> NotifyData {
        let mut vec = Vec::new();
        let size = self.conf.file_specs.len();
        let guard = self.code.lock().unwrap();
        for i in 0..size {
            vec.push(EachFileStatus {
                path: self.conf.file_specs[i].path.clone(),
                reason: guard[i],
                message: guard[i].to_str().into(),
            });
        }
        NotifyData {
            progress: self.progress.lock().unwrap().clone(),
            action: self.conf.common_data.action,
            version: self.conf.version,
            each_file_status: vec,
            task_id: self.conf.common_data.task_id,
            _uid: self.conf.common_data.uid,
        }
    }

    pub(crate) fn record_waitting_network_time(&self) {
        let mut staus = self.status.lock().unwrap();
        staus.waiting_network_time = Some(get_current_timestamp());
    }

    pub(crate) fn check_net_work_status(&self) -> bool {
        if !self.is_satisfied_configuration() {
            if self.conf.version == Version::API10
                && self.conf.common_data.mode == Mode::BackGround
                && self.conf.common_data.retry
            {
                self.set_status(State::Waiting, Reason::UnsupportedNetworkType);
            } else {
                self.set_status(State::Failed, Reason::UnsupportedNetworkType);
            }
            return false;
        }
        true
    }

    pub(crate) fn net_work_online(&self) -> bool {
        if unsafe { !IsOnline() } {
            if self.conf.version == Version::API10
                && self.conf.common_data.mode == Mode::BackGround
                && self.conf.common_data.retry
            {
                self.set_status(State::Waiting, Reason::NetworkOffline);
            } else {
                let retry_times = 20;
                for _ in 0..retry_times {
                    if unsafe { IsOnline() } {
                        return true;
                    }
                    sleep(Duration::from_millis(RETRY_INTERVAL));
                }
                self.set_status(State::Failed, Reason::NetworkOffline);
            }
            return false;
        }
        true
    }

    pub(crate) fn build_request_builder(&self) -> RequestBuilder {
        let url = self.conf.url.clone();
        let method = match self.conf.method.to_uppercase().as_str() {
            "PUT" => "PUT",
            "POST" => "POST",
            "GET" => "GET",
            _ => match self.conf.common_data.action {
                Action::Upload => {
                    if self.conf.version == Version::API10 {
                        "PUT"
                    } else {
                        "POST"
                    }
                }
                Action::Download => "GET",
                _ => "",
            },
        };
        let mut request = RequestBuilder::new().method(method).url(url.as_str());
        for (key, value) in self.conf.headers.iter() {
            request = request.header(key.as_str(), value.as_str());
        }
        request
    }

    async fn clear_downloaded_file(&self) -> bool {
        let file = self.files.get_mut(0).unwrap();
        let res = file.set_len(0).await;
        match res {
            Err(e) => {
                error!("clear download file error: {:?}", e);
                self.set_status(State::Failed, Reason::IoError);
                false
            }
            _ => {
                debug!("set len success");
                match file.seek(SeekFrom::Start(0)).await {
                    Err(e) => {
                        error!("seek err is {:?}", e);
                        self.set_status(State::Failed, Reason::IoError);
                        false
                    }
                    Ok(_) => {
                        debug!("seek success");
                        let mut progress_guard = self.progress.lock().unwrap();
                        progress_guard.common_data.total_processed = 0;
                        progress_guard.processed[0] = 0;
                        true
                    }
                }
            }
        }
    }

    pub(crate) async fn build_download_request(&self) -> Option<Request> {
        let mut request_builder = self.build_request_builder();
        let mut begins = self.conf.common_data.begins;
        let ends = self.conf.common_data.ends;
        self.range_response.store(false, Ordering::SeqCst);
        if self.resume.load(Ordering::SeqCst) || begins > 0 || ends >= 0 {
            self.range_request.store(true, Ordering::SeqCst);
            self.skip_bytes.store(0, Ordering::SeqCst);
            if self.resume.load(Ordering::SeqCst) {
                let if_range = {
                    let progress_guard = self.progress.lock().unwrap();
                    let etag = progress_guard.extras.get("etag");
                    let last_modified = progress_guard.extras.get("last-modified");
                    if let Some(etag) = etag {
                        request_builder = request_builder.header("If-Range", etag.as_str());
                        true
                    } else if let Some(last_modified) = last_modified {
                        request_builder =
                            request_builder.header("If-Range", last_modified.as_str());
                        true
                    } else {
                        false
                    }
                };
                if !if_range {
                    // unable to verify file consistency, need download again
                    if begins == 0 && ends < 0 {
                        self.range_request.store(false, Ordering::SeqCst);
                    }
                    if !self.clear_downloaded_file().await {
                        return None;
                    }
                }
            }
            let file = self.files.get_mut(0).unwrap();
            let current_len = file.metadata().await.unwrap().len();
            begins += current_len;
            // Modifys the progress to the current file size.
            // It will be recorded to the database later during download.
            let mut progress_guard = self.progress.lock().unwrap();
            progress_guard.processed[0] = current_len as usize;
            progress_guard.common_data.total_processed = current_len as usize;
            if self.range_request.load(Ordering::SeqCst) {
                let range = if ends < 0 {
                    format!("bytes={begins}-")
                } else {
                    format!("bytes={begins}-{ends}")
                };
                request_builder = request_builder.header("Range", range.as_str());
            }
        } else {
            self.range_request.store(false, Ordering::SeqCst);
        }
        let result = request_builder.body(Body::slice(self.conf.data.clone()));
        match result {
            Ok(value) => Some(value),
            Err(e) => {
                error!("build download request error is {:?}", e);
                self.set_status(State::Failed, Reason::BuildRequestFailed);
                None
            }
        }
    }

    pub(crate) fn get_file_info(&self, response: &Response) -> bool {
        if self.get_file_info.load(Ordering::SeqCst) {
            return true;
        }
        self.get_file_info.store(true, Ordering::SeqCst);
        let content_type = response.headers().get("content-type");
        if let Some(mime_type) = content_type {
            if let Ok(value) = mime_type.to_string() {
                *self.mime_type.lock().unwrap() = value;
            }
        }

        let content_length = response.headers().get("content-length");
        if let Some(len) = content_length {
            let length = len.to_string();
            match length {
                Ok(value) => {
                    let len = value.parse::<i64>();
                    match len {
                        Ok(v) => {
                            let mut guard = self.progress.lock().unwrap();
                            if !self.restored.load(Ordering::SeqCst) {
                                guard.sizes[0] = v;
                            }
                            self.file_total_size.store(v, Ordering::SeqCst);
                            debug!("the download task content-length is {}", v);
                        }
                        Err(e) => {
                            error!("convert string to i64 error: {:?}", e);
                        }
                    }
                }
                Err(e) => {
                    error!("convert header value to string error: {:?}", e);
                }
            }
        } else {
            error!("cannot get content-length of the task");
            if self.conf.common_data.precise {
                self.set_status(State::Failed, Reason::GetFileSizeFailed);
                return false;
            }
        }
        true
    }

    fn handle_body_transfer_error(&self) {
        if unsafe { !IsOnline() } {
            match self.conf.version {
                Version::API9 => {
                    if self.conf.common_data.action == Action::Download {
                        self.set_status(State::Waiting, Reason::NetworkOffline);
                    } else {
                        self.set_status(State::Failed, Reason::NetworkOffline);
                    }
                }
                Version::API10 => {
                    if self.conf.common_data.mode == Mode::FrontEnd || !self.conf.common_data.retry
                    {
                        self.set_status(State::Failed, Reason::NetworkOffline);
                    } else {
                        self.set_status(State::Waiting, Reason::NetworkOffline);
                    }
                }
            }
        } else {
            let index = self.progress.lock().unwrap().common_data.index;
            self.set_code(index, Reason::OthersError);
        }
    }

    pub(crate) fn handle_download_error(&self, result: &Result<(), HttpClientError>) -> bool {
        match result {
            Ok(_) => true,
            Err(err) => {
                error!("download err is {:?}", err);
                match err.error_kind() {
                    ErrorKind::Timeout => {
                        self.set_status(State::Failed, Reason::ContinuousTaskTimeout);
                    }
                    // user triggered
                    ErrorKind::UserAborted => return true,
                    ErrorKind::BodyTransfer | ErrorKind::BodyDecode => {
                        self.handle_body_transfer_error()
                    }
                    _ => {
                        self.set_status(State::Failed, Reason::OthersError);
                    }
                }
                false
            }
        }
    }

    pub(crate) async fn handle_response_error(
        &self,
        response: &Result<Response, HttpClientError>,
    ) -> bool {
        let index = self.progress.lock().unwrap().common_data.index;
        match response {
            Ok(r) => {
                let http_response_code = r.status();
                info!("the http response code is {}", http_response_code);
                if http_response_code.is_server_error()
                    || (http_response_code.as_u16() != 408 && http_response_code.is_client_error())
                    || http_response_code.is_redirection()
                {
                    self.set_code(index, Reason::ProtocolError);
                    return false;
                }
                if http_response_code.as_u16() == 408 {
                    if !self.retry_for_request.load(Ordering::SeqCst) {
                        self.retry_for_request.store(true, Ordering::SeqCst);
                    } else {
                        self.set_code(index, Reason::ProtocolError);
                    }
                    return false;
                }
                if self.range_request.load(Ordering::SeqCst) {
                    match http_response_code.as_u16() {
                        206 => {
                            self.range_response.store(true, Ordering::SeqCst);
                        }
                        200 => {
                            self.range_response.store(false, Ordering::SeqCst);
                            if self.resume.load(Ordering::SeqCst) {
                                if !self.clear_downloaded_file().await {
                                    return false;
                                }
                            } else {
                                self.set_code(index, Reason::UnsupportedRangeRequest);
                                return false;
                            }
                        }
                        _ => {}
                    }
                }
                true
            }
            Err(e) => {
                error!("http client err is {:?}", e);
                match e.error_kind() {
                    ErrorKind::UserAborted => self.set_code(index, Reason::UserOperation),
                    ErrorKind::Timeout => self.set_code(index, Reason::ContinuousTaskTimeout),
                    ErrorKind::Request => self.set_code(index, Reason::RequestError),
                    ErrorKind::Redirect => self.set_code(index, Reason::RedirectError),
                    ErrorKind::Connect | ErrorKind::ConnectionUpgrade => {
                        self.set_code(index, Reason::ConnectError)
                    }
                    ErrorKind::BodyTransfer => self.handle_body_transfer_error(),
                    _ => self.set_code(index, Reason::OthersError),
                }
                false
            }
        }
    }

    pub(crate) fn notify_response(&self, response: &Response) {
        let tid = self.conf.common_data.task_id;
        let version: String = response.version().as_str().into();
        let status_code: u32 = response.status().as_u16() as u32;
        let status_message: String;
        if let Some(reason) = response.status().reason() {
            status_message = reason.into();
        } else {
            error!("bad status_message {:?}", status_code);
            return;
        }
        let headers = response.headers().clone();
        debug!("notify_response");
        RequestAbility::client_manager().send_response(
            tid,
            version,
            status_code,
            status_message,
            headers,
        )
    }

    pub(crate) fn record_response_header(&self, response: &Result<Response, HttpClientError>) {
        if let Ok(r) = response {
            self.notify_response(r);
            let mut guard = self.progress.lock().unwrap();
            guard.extras.clear();
            for (k, v) in r.headers() {
                if let Ok(value) = v.to_string() {
                    guard.extras.insert(k.to_string().to_lowercase(), value);
                }
            }
        }
    }

    pub(crate) async fn record_upload_response(
        &self,
        index: usize,
        response: Result<Response, HttpClientError>,
    ) {
        self.record_response_header(&response);
        if let Ok(mut r) = response {
            let yfile = match self.body_files.get_mut(index) {
                Some(yfile) => yfile,
                None => return,
            };

            loop {
                let mut buf = [0u8; 1024];
                let size = r.data(&mut buf).await;
                let size = match size {
                    Ok(size) => size,
                    Err(_e) => break,
                };

                if size == 0 {
                    break;
                }
                let _ = yfile.write_all(&buf[..size]).await;
            }
            // Makes sure all the data has been written to the target file.
            let _ = yfile.sync_all().await;
        }
        if self.conf.version == Version::API9 && self.conf.common_data.action == Action::Upload {
            let notify_data = self.build_notify_data();
            #[cfg(feature = "oh")]
            Notifier::service_front_notify(SubscribeType::HeaderReceive, notify_data);
        }
    }

    fn set_code(&self, index: usize, code: Reason) {
        if code == Reason::UploadFileError {
            return;
        }
        let mut code_guard = self.code.lock().unwrap();
        if index < code_guard.len() && code_guard[index] == Reason::Default {
            debug!("set code");
            code_guard[index] = code;
        }
    }

    pub(crate) fn reset_code(&self, index: usize) {
        let file_counts = self.conf.file_specs.len();
        let mut code_guard = self.code.lock().unwrap();
        if index < file_counts {
            debug!("reset code");
            code_guard[index] = Reason::Default;
        }
    }

    pub(crate) fn set_status(&self, state: State, reason: Reason) -> bool {
        debug!("set status");
        {
            let mut current_status = self.status.lock().unwrap();
            if state == current_status.state && reason == current_status.reason {
                return true;
            }
            let mut progress_guard = self.progress.lock().unwrap();
            let index = progress_guard.common_data.index;
            let current_state = current_status.state;
            debug!(
                "set state {:?}, reason {:?} current_state {:?}",
                state, reason, current_state
            );
            match state {
                State::Paused | State::Stopped => {
                    if current_state != State::Running
                        && current_state != State::Retrying
                        && current_state != State::Waiting
                    {
                        return false;
                    }
                    self.set_code(index, reason);
                }
                State::Completed => {
                    if current_state != State::Running && current_state != State::Retrying {
                        return false;
                    }
                }
                State::Failed => {
                    if current_state == State::Completed
                        || current_state == State::Removed
                        || current_state == State::Stopped
                    {
                        return false;
                    }
                    self.set_code(index, reason);
                    let file_counts = self.conf.file_specs.len();
                    for i in index..file_counts {
                        self.set_code(i, reason);
                    }
                }

                State::Waiting => {
                    if current_state == State::Completed || current_state == State::Removed {
                        return false;
                    }
                    self.set_code(index, reason);
                }

                State::Removed => self.set_code(index, reason),
                _ => {}
            }
            current_status.mtime = get_current_timestamp();
            progress_guard.common_data.state = state as u8;
            if current_state != State::Running && state == State::Running {
                let event = RunCountEvent::change_runcount(1);
                RequestAbility::runcount_manager().send_event(event);
            } else if current_state == State::Running && state != State::Running {
                let event = RunCountEvent::change_runcount(-1);
                RequestAbility::runcount_manager().send_event(event);
            }
            current_status.state = state;
            current_status.reason = reason;
            debug!("current state is {:?}, reason is {:?}", state, reason);
        }
        if state == State::Waiting {
            self.record_waitting_network_time();
        }
        self.record_task_info();
        self.state_change_notify(state);
        true
    }

    fn state_change_notify(&self, state: State) {
        if state == State::Initialized
            || (self.progress.lock().unwrap().common_data.total_processed == 0
                && (state == State::Running || state == State::Retrying))
        {
            return;
        }

        debug!("state change notification");
        let notify_data = self.build_notify_data();
        #[cfg(feature = "oh")]
        Notifier::service_front_notify(SubscribeType::Progress, notify_data.clone());
        let bundle = CString::new(self.conf.bundle.as_str()).unwrap();
        match state {
            State::Completed => {
                unsafe {
                    PublishStateChangeEvents(
                        bundle.as_ptr(),
                        self.conf.bundle.len() as u32,
                        self.conf.common_data.task_id,
                        State::Completed as i32,
                    );
                }
                #[cfg(feature = "oh")]
                Notifier::service_front_notify(SubscribeType::Complete, notify_data)
            }
            State::Failed => {
                unsafe {
                    PublishStateChangeEvents(
                        bundle.as_ptr(),
                        self.conf.bundle.len() as u32,
                        self.conf.common_data.task_id,
                        State::Failed as i32,
                    );
                }
                #[cfg(feature = "oh")]
                Notifier::service_front_notify(SubscribeType::Fail, notify_data)
            }
            State::Paused | State::Waiting =>
            {
                #[cfg(feature = "oh")]
                Notifier::service_front_notify(SubscribeType::Pause, notify_data)
            }
            _ => {}
        }
        self.background_notify();
    }

    fn record_task_info(&self) {
        debug!(
            "RequestTask record task info, task_id:{}",
            self.conf.common_data.task_id
        );

        let has_record = unsafe { HasRequestTaskRecord(self.conf.common_data.task_id) };
        if has_record {
            let update_info = self.get_update_info();
            let sizes: String = format!("{:?}", update_info.progress.sizes);
            let processed: String = format!("{:?}", update_info.progress.processed);
            let extras = hashmap_to_string(&update_info.progress.extras);
            let each_file_status = update_info
                .each_file_status
                .iter()
                .map(|x| x.to_c_struct())
                .collect();
            let c_update_info =
                update_info.to_c_struct(&sizes, &processed, &extras, &each_file_status);
            let ret = unsafe { UpdateRequestTask(self.conf.common_data.task_id, &c_update_info) };
            debug!("update database ret is {}", ret);
        }
    }

    fn get_each_file_status(&self) -> Vec<EachFileStatus> {
        let mut vec = Vec::new();
        let size = self.conf.file_specs.len();
        let guard = self.code.lock().unwrap();
        for i in 0..size {
            vec.push(EachFileStatus {
                path: self.conf.file_specs[i].path.clone(),
                reason: guard[i],
                message: guard[i].to_str().into(),
            });
        }
        vec
    }

    fn get_update_info(&self) -> UpdateInfo {
        let status = self.status.lock().unwrap();
        let progress = self.progress.lock().unwrap();
        UpdateInfo {
            mtime: status.mtime,
            reason: status.reason as u8,
            tries: self.tries.load(Ordering::SeqCst),
            mime_type: self.mime_type.lock().unwrap().clone(),
            progress: progress.clone(),
            each_file_status: self.get_each_file_status(),
        }
    }

    pub(crate) fn show(&self) -> TaskInfo {
        let status = self.status.lock().unwrap();
        let progress = self.progress.lock().unwrap();
        TaskInfo {
            bundle: self.conf.bundle.clone(),
            url: self.conf.url.clone(),
            data: self.conf.data.clone(),
            token: self.conf.token.clone(),
            form_items: self.conf.form_items.clone(),
            file_specs: self.conf.file_specs.clone(),
            title: self.conf.title.clone(),
            description: self.conf.description.clone(),
            mime_type: {
                match self.conf.version {
                    Version::API10 => match self.conf.common_data.action {
                        Action::Download => match self.conf.headers.get("Content-Type") {
                            None => "".into(),
                            Some(v) => v.clone(),
                        },
                        Action::Upload => "multipart/form-data".into(),
                        _ => "".into(),
                    },
                    Version::API9 => self.mime_type.lock().unwrap().clone(),
                }
            },
            progress: progress.clone(),
            extras: progress.extras.clone(),
            each_file_status: self.get_each_file_status(),
            common_data: CommonTaskInfo {
                task_id: self.conf.common_data.task_id,
                uid: self.conf.common_data.uid,
                action: self.conf.common_data.action as u8,
                mode: self.conf.common_data.mode as u8,
                ctime: self.ctime,
                mtime: status.mtime,
                reason: status.reason as u8,
                gauge: self.conf.common_data.gauge,
                retry: match self.conf.common_data.mode {
                    Mode::FrontEnd => false,
                    _ => self.conf.common_data.retry,
                },
                tries: self.tries.load(Ordering::SeqCst),
                version: self.conf.version as u8,
                priority: self.conf.common_data.priority,
            },
        }
    }

    // only use for download task
    pub(crate) fn query_mime_type(&self) -> String {
        self.mime_type.lock().unwrap().clone()
    }

    pub(crate) fn is_satisfied_configuration(&self) -> bool {
        if self.conf.common_data.network == Network::Any {
            return true;
        }
        unsafe {
            let network_info = GetNetworkInfo();
            if !self.conf.common_data.roaming && (*network_info).is_roaming {
                error!("not allow roaming");
                return false;
            }
            if !self.conf.common_data.metered && (*network_info).is_metered {
                error!("not allow metered");
                return false;
            }
            if (*network_info).network_type != self.conf.common_data.network {
                error!("dismatch network type");
                return false;
            }
        };
        true
    }

    pub(crate) fn background_notify(&self) {
        if self.conf.version == Version::API9 && !self.conf.common_data.background {
            return;
        }
        if self.conf.version == Version::API10 && self.conf.common_data.mode == Mode::FrontEnd {
            return;
        }
        let mut file_total_size = self.file_total_size.load(Ordering::SeqCst);
        let total_processed = self.progress.lock().unwrap().common_data.total_processed as u64;
        if file_total_size <= 0 || total_processed == 0 {
            return;
        }
        if self.conf.common_data.action == Action::Download {
            if self.conf.common_data.ends < 0 {
                file_total_size -= self.conf.common_data.begins as i64;
            } else {
                file_total_size =
                    self.conf.common_data.ends - self.conf.common_data.begins as i64 + 1;
            }
        }
        self.background_notify_time
            .store(get_current_timestamp(), Ordering::SeqCst);
        let index = self.progress.lock().unwrap().common_data.index;
        if index >= self.conf.file_specs.len() {
            return;
        }
        let percent = total_processed * 100 / (file_total_size as u64);
        debug!("background notify");
        let task_msg = RequestTaskMsg {
            task_id: self.conf.common_data.task_id,
            uid: self.conf.common_data.uid as i32,
            action: self.conf.common_data.action as u8,
        };
        unsafe {
            let c_path = CStringWrapper::from(self.conf.file_specs[index].path.as_str());
            let c_file_name = CStringWrapper::from(self.conf.file_specs[index].file_name.as_str());
            RequestBackgroundNotify(task_msg, c_path, c_file_name, percent as u32);
        };
    }

    pub(crate) fn get_upload_info(&self, index: usize) -> (bool, u64) {
        let guard = self.progress.lock().unwrap();
        let file_size = guard.sizes[index];
        let mut is_partial_upload = false;
        let mut upload_file_length: u64 = file_size as u64 - guard.processed[index] as u64;
        if file_size == 0 {
            return (is_partial_upload, upload_file_length);
        }
        if index as u32 != self.conf.common_data.index {
            return (is_partial_upload, upload_file_length);
        }
        let begins = self.conf.common_data.begins;
        let mut ends = self.conf.common_data.ends;
        if ends < 0 || ends >= file_size {
            ends = file_size - 1;
        }
        if begins >= file_size as u64 || begins > ends as u64 {
            return (is_partial_upload, upload_file_length);
        }
        is_partial_upload = true;
        upload_file_length = ends as u64 - begins + 1 - guard.processed[index] as u64;
        (is_partial_upload, upload_file_length)
    }
}

pub(crate) struct RunningTask {
    task: Arc<RequestTask>,
    // `_unloader` is never used when executing the task.
    _unloader: SAKeeper,
}

impl RunningTask {
    pub(crate) fn new(task: Arc<RequestTask>, unloader: SAKeeper) -> Self {
        Self {
            task,
            _unloader: unloader,
        }
    }
}

impl Deref for RunningTask {
    type Target = Arc<RequestTask>;

    fn deref(&self) -> &Self::Target {
        &self.task
    }
}

pub(crate) async fn run(task: RunningTask) {
    info!("run the task which id is {}", task.conf.common_data.task_id);
    if !task.net_work_online() || !task.check_net_work_status() {
        return;
    }
    let action = task.conf.common_data.action;
    match action {
        Action::Download => loop {
            task.reset_code(0);

            download(task.clone()).await;

            let state = task.status.lock().unwrap().state;
            if state != State::Running && state != State::Retrying {
                break;
            }
            let code = task.code.lock().unwrap()[0];
            if code != Reason::Default {
                task.set_status(State::Failed, code);
                break;
            }
        },
        Action::Upload => {
            let state = task.status.lock().unwrap().state;
            if state == State::Retrying {
                let index = {
                    let mut progress_guard = task.progress.lock().unwrap();
                    let index = progress_guard.common_data.index;
                    progress_guard.common_data.total_processed -= progress_guard.processed[index];
                    progress_guard.processed[index] = 0;
                    index
                };
                let file = task.files.get_mut(index).unwrap();
                let mut begins = task.conf.common_data.begins;
                let (is_partial_upload, _) = task.get_upload_info(index);
                if !is_partial_upload {
                    begins = 0;
                }
                if let Err(e) = file.seek(SeekFrom::Start(begins)).await {
                    task.set_code(index, Reason::IoError);
                    error!("seek err is {:?}", e);
                }
            }
            upload(task.clone()).await;
        }
        _ => {}
    }
    info!("run end");
}

#[derive(Clone, Debug)]
pub(crate) struct TaskStatus {
    pub(crate) waiting_network_time: Option<u64>,
    pub(crate) mtime: u64,
    pub(crate) state: State,
    pub(crate) reason: Reason,
}

impl TaskStatus {
    pub(crate) fn new(mtime: u64) -> Self {
        TaskStatus {
            waiting_network_time: None,
            mtime,
            state: State::Created,
            reason: Reason::Default,
        }
    }
}

pub(crate) fn check_configs(config: &TaskConfig) -> bool {
    const EL1: &str = "/data/storage/el1/base/";
    const EL2: &str = "/data/storage/el2/base/";

    let mut result = true;
    for (idx, spec) in config.file_specs.iter().enumerate() {
        let path = &spec.path;
        if !spec.is_user_file && !path.starts_with(EL1) && !path.starts_with(EL2) {
            error!("File path invalid - path: {}, idx: {}", path, idx);
            result = false;
            break;
        }
    }

    result
}
