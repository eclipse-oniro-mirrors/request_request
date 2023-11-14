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

use std::{ffi::CString, ffi::c_char, fs::File, pin::Pin, thread::sleep, time::Duration, cell::UnsafeCell};
use super::{
    enumration::*, progress::*, task_info::*, task_config::*, task_manager::*, utils::*, request_binding::*,
    log::LOG_LABEL, request_service_ability::*,
};
use crate::trace::TraceScope;
use crate::sys_event::{SysEvent, build_number_param, build_str_param};
use hilog_rust::*;
use std::io::{Read, SeekFrom};
use std::sync::atomic::{AtomicBool, AtomicI64, AtomicU32, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};
use std::task::{Context, Poll};

use ylong_http_client::async_impl::{
    Client, DownloadOperator, Downloader, MultiPart, Part, UploadOperator, Uploader,
};
use ylong_http_client::{
    Body, Certificate, ErrorKind, HttpClientError, Method, Redirect, Request, RequestBuilder,
    Response, SpeedLimit, Timeout, TlsVersion,
};
use ylong_runtime::fs::File as YlongFile;
use ylong_runtime::io::{AsyncRead, AsyncSeek, AsyncWrite, ReadBuf, AsyncWriteExt, AsyncSeekExt};

static CONNECT_TIMEOUT: u64 = 60;
static LOW_SPEED_TIME: u64 = 60;
static LOW_SPEED_LIMIT: u64 = 1;
static SECONDS_IN_ONE_WEEK: u64 = 7 * 24 * 60 * 60;
static FRONT_NOTIFY_INTERVAL: u64 = 1000;
static BACKGROUND_NOTIFY_INTERVAL: u64 = 3000;
static RETRY_INTERVAL: u64 = 20;
#[derive(Clone, Debug)]
pub struct TaskStatus {
    pub waitting_network_time: Option<u64>,
    pub mtime: u64,
    pub state: State,
    pub reason: Reason,
}

impl Default for TaskStatus {
    fn default() -> Self {
        TaskStatus {
            waitting_network_time: None,
            mtime: get_current_timestamp(),
            state: State::CREATED,
            reason: Reason::Default,
        }
    }
}

struct Files(UnsafeCell<Vec<YlongFile>>);

impl Files {
    fn get(&self, index: usize) -> Option<&YlongFile> {
        unsafe { &*self.0.get() }.get(index)
    }
}

unsafe impl Sync for Files {}
unsafe impl Send for Files {}

// Need to release file timely.
struct BodyFiles(UnsafeCell<Vec<Option<YlongFile>>>);
unsafe impl Sync for BodyFiles {}
unsafe impl Send for BodyFiles {}

pub struct RequestTask {
    pub conf: Arc<TaskConfig>,
    pub uid: u64,
    pub task_id: u32,
    pub ctime: u64,
    pub mime_type: Mutex<String>,
    pub progress: Mutex<Progress>,
    pub tries: AtomicU32,
    pub status: Mutex<TaskStatus>,
    pub retry: AtomicBool,
    pub get_file_info: AtomicBool,
    pub retry_for_request: AtomicBool,
    pub retry_for_speed: AtomicBool,
    pub code: Mutex<Vec<Reason>>,
    pub background_notify_time: AtomicU64,
    pub file_total_size: AtomicI64,
    pub resume: AtomicBool,
    files: Files,
    body_files: BodyFiles,
    seek_flag: AtomicBool,
    range_request: AtomicBool,
    range_response: AtomicBool,
    restored: AtomicBool,
    skip_bytes: AtomicU64,
    upload_counts: AtomicU32,
    client: Option<Client>,
}

struct TaskReader {
    task: Arc<RequestTask>,
}

struct TaskOperator {
    task: Arc<RequestTask>,
}

impl TaskOperator {
    fn poll_progress_common(&self, _cx: &mut Context<'_>) -> Poll<Result<(), HttpClientError>> {
        let state = self.task.status.lock().unwrap().state;
        if (state != State::RUNNING && state != State::RETRYING)
            || (self.task.conf.version == Version::API10 && !self.task.check_net_work_status())
        {
            debug!(LOG_LABEL, "pause the task");
            return Poll::Ready(Err(HttpClientError::user_aborted()));
        }
        let last_front_notify_time = TaskManager::get_instance().front_notify_time;
        let version = self.task.conf.version;
        let mode = self.task.conf.common_data.mode;
        if get_current_timestamp() - last_front_notify_time >= FRONT_NOTIFY_INTERVAL {
            let notify_data = self.task.build_notify_data();
            TaskManager::get_instance().front_notify("progress".into(), &notify_data);
        }
        let gauge = self.task.conf.common_data.gauge;
        if version == Version::API9 || gauge {
            let last_background_notify_time =
                self.task.background_notify_time.load(Ordering::SeqCst);
            if get_current_timestamp() - last_background_notify_time >= BACKGROUND_NOTIFY_INTERVAL {
                self.task.background_notify();
            }
        }
        Poll::Ready(Ok(()))
    }

    fn poll_write_partial_file(
        &self,
        cx: &mut Context<'_>,
        data: &[u8],
        begins: u64,
        ends: i64,
    ) -> Poll<Result<usize, HttpClientError>> {
        let data_size = data.len();
        let skip_size = self.task.skip_bytes.load(Ordering::SeqCst);
        if skip_size + data_size as u64 <= begins {
            self.task.skip_bytes.fetch_add(data_size as u64, Ordering::SeqCst);
            return Poll::Ready(Ok(data_size));
        }
        let remain_skip_bytes = (begins - skip_size) as usize;
        let mut data = &data[remain_skip_bytes..];
        self.task.skip_bytes.store(begins, Ordering::SeqCst);
        if ends >= 0 {
            let total_bytes = ends as u64 - begins + 1;
            let written_bytes = self.task.progress.lock().unwrap().processed[0] as u64;
            if written_bytes == total_bytes {
                return Poll::Ready(Err(HttpClientError::user_aborted()));
            }
            if data.len() as u64 + written_bytes >= total_bytes {
                let remain_bytes = (total_bytes - written_bytes) as usize;
                data = &data[..remain_bytes];
            }
        }
        self.poll_write_file(cx, data, remain_skip_bytes)
    }

    fn poll_write_file(
        &self,
        cx: &mut Context<'_>,
        data: &[u8],
        skip_size: usize,
    ) -> Poll<Result<usize, HttpClientError>> {
        let file = unsafe { &mut *self.task.files.0.get() }.get_mut(0).unwrap();
        let mut progress_guard = self.task.progress.lock().unwrap();
        match Pin::new(file).poll_write(cx, data) {
            Poll::Ready(Ok(size)) => {
                progress_guard.processed[0] += size;
                progress_guard.common_data.total_processed += size;
                Poll::Ready(Ok(size + skip_size))
            }
            Poll::Pending => Poll::Pending,
            Poll::Ready(Err(e)) => Poll::Ready(Err(HttpClientError::other(Some(e)))),
        }
    }
}

impl DownloadOperator for TaskOperator {
    fn poll_download(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        data: &[u8],
    ) -> Poll<Result<usize, HttpClientError>> {
        if self.task.range_request.load(Ordering::SeqCst) {
            if self.task.range_response.load(Ordering::SeqCst) {
                return self.poll_write_file(cx, data, 0);
            }
            // write partial response data
            let begins = self.task.conf.common_data.begins;
            let ends = self.task.conf.common_data.ends;
            return self.poll_write_partial_file(cx, data, begins, ends);
        }
        return self.poll_write_file(cx, data, 0);
    }

    fn poll_progress(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        _downloaded: u64,
        _total: Option<u64>,
    ) -> Poll<Result<(), HttpClientError>> {
        self.poll_progress_common(cx)
    }
}

impl UploadOperator for TaskOperator {
    fn poll_progress(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        _uploaded: u64,
        _total: Option<u64>,
    ) -> Poll<Result<(), HttpClientError>> {
        self.poll_progress_common(cx)
    }
}

impl AsyncRead for TaskReader {
    fn poll_read(
        self: Pin<&mut Self>,
        cx: &mut Context<'_>,
        buf: &mut ReadBuf<'_>,
    ) -> Poll<std::io::Result<()>> {
        let index = self.task.progress.lock().unwrap().common_data.index;
        let file = unsafe { &mut *self.task.files.0.get() }
            .get_mut(index)
            .unwrap();
        let (is_partial_upload, total_upload_bytes) = self.task.get_upload_info(index);
        let mut progress_guard = self.task.progress.lock().unwrap();
        if !is_partial_upload {
            let filled_len = buf.filled().len();
            match Pin::new(file).poll_read(cx, buf) {
                Poll::Ready(Ok(_)) => {
                    let current_filled_len = buf.filled().len();
                    let upload_size = current_filled_len - filled_len;
                    progress_guard.processed[index] += upload_size;
                    progress_guard.common_data.total_processed += upload_size;
                    return Poll::Ready(Ok(()));
                }
                Poll::Pending => Poll::Pending,
                Poll::Ready(Err(e)) => Poll::Ready(Err(e)),
            }
        } else {
            let begins = self.task.conf.common_data.begins;
            if !self.task.seek_flag.load(Ordering::SeqCst) {
                match Pin::new(file).poll_seek(cx,SeekFrom::Start(begins)) {
                    Poll::Ready(Err(e)) => {
                        error!(LOG_LABEL, "seek err is {:?}", e);
                        return Poll::Ready(Err(e));
                    }
                    _ => self.task.seek_flag.store(true, Ordering::SeqCst),
                }
            }
            let buf_filled_len = buf.filled().len();
            let mut read_buf = buf.take(total_upload_bytes as usize);
            let filled_len = read_buf.filled().len();
            let file = unsafe { &mut *self.task.files.0.get() }
                .get_mut(index)
                .unwrap();
            match Pin::new(file).poll_read(cx, &mut read_buf) {
                Poll::Ready(Ok(_)) => {
                    let current_filled_len = read_buf.filled().len();
                    let upload_size = current_filled_len - filled_len;
                    // need update buf.filled and buf.initialized
                    unsafe {
                        buf.assume_init(upload_size);
                    }
                    buf.set_filled(buf_filled_len + upload_size);
                    progress_guard.processed[index] += upload_size;
                    progress_guard.common_data.total_processed += upload_size;
                    Poll::Ready(Ok(()))
                }
                Poll::Pending => Poll::Pending,
                Poll::Ready(Err(e)) => Poll::Ready(Err(e)),
            }
        }
    }
}

impl RequestTask {
    pub fn constructor(conf: Arc<TaskConfig>, uid: u64, task_id: u32, files: Vec<File>, body_files: Vec<File>) -> Self {
        let mut sizes: Vec<i64> = Vec::<i64>::new();
        match conf.common_data.action {
            Action::DOWNLOAD => sizes.push(-1),
            Action::UPLOAD => {
                for f in files.iter() {
                    let file_size = f.metadata().unwrap().len() as i64;
                    debug!(LOG_LABEL, "file size size is {}",  @public(file_size));
                    sizes.push(file_size);
                }
            }
            _ => {},
        }
        let file_count = files.len();
        let action = conf.common_data.action;
        let mut task = RequestTask {
            conf,
            uid,
            task_id,
            ctime: get_current_timestamp(),
            files: Files(UnsafeCell::new(
                files.into_iter().map(|f| YlongFile::new(f)).collect(),
            )),
            body_files: BodyFiles(UnsafeCell::new(
                body_files.into_iter().map(|f| Some(YlongFile::new(f))).collect(),
            )),
            mime_type: Mutex::new(String::new()),
            progress: Mutex::new(Progress::new(sizes)),
            tries: AtomicU32::new(0),
            status: Mutex::new(TaskStatus::default()),
            retry: AtomicBool::new(false),
            get_file_info: AtomicBool::new(false),
            retry_for_request: AtomicBool::new(false),
            retry_for_speed: AtomicBool::new(false),
            code: Mutex::new(vec![Reason::Default; file_count]),
            background_notify_time: AtomicU64::new(get_current_timestamp()),
            file_total_size: AtomicI64::new(-1),
            resume: AtomicBool::new(false),
            seek_flag: AtomicBool::new(false),
            range_request: AtomicBool::new(false),
            range_response: AtomicBool::new(false),
            restored: AtomicBool::new(false),
            skip_bytes: AtomicU64::new(0),
            upload_counts: AtomicU32::new(0),
            client: None,
        };
        task.client = task.build_client();
        if action == Action::UPLOAD {
            task.file_total_size.store(task.get_upload_file_total_size() as i64, Ordering::SeqCst);
        }
        task
    }

    pub fn restore_task(conf: Arc<TaskConfig>, info: TaskInfo) -> Self {
        let progress_index = info.progress.common_data.index;
        let uid = info.common_data.uid;
        let action = conf.common_data.action;
        let mut files: Vec<File> = Vec::new();
        let mut body_files: Vec<File> = Vec::new();
        for fs in &conf.file_specs {
            if action == Action::UPLOAD {
                match RequestAbility::open_file_readonly(uid, &conf.bundle, &fs.path) {
                    Ok(file) => { files.push(file); },
                    Err(e) => { error!(LOG_LABEL, "open file RO failed, err is {:?}", e); },
                }
            } else {
                match RequestAbility::open_file_readwrite(uid, &conf.bundle, &fs.path) {
                    Ok(file) => { files.push(file); },
                    Err(e) => { error!(LOG_LABEL, "open file RW failed, err is {:?}", e); },
                }
            }
        }
        for name in &conf.body_file_names {
            match RequestAbility::open_file_readwrite(uid, &conf.bundle, &name) {
                Ok(body_file) => { body_files.push(body_file); },
                Err(e) => { error!(LOG_LABEL, "open body_file failed, err is {:?}", e); },
            }
        }
        let file_count = files.len();
        let mut task = RequestTask {
            conf,
            uid,
            task_id: info.common_data.task_id,
            ctime: info.common_data.ctime,
            files: Files(UnsafeCell::new(
                files.into_iter().map(|f| YlongFile::new(f)).collect(),
            )),
            body_files: BodyFiles(UnsafeCell::new(
                body_files.into_iter().map(|f| Some(YlongFile::new(f))).collect(),
            )),
            mime_type: Mutex::new(info.mime_type),
            progress:  Mutex::new(info.progress.clone()),
            tries: AtomicU32::new(info.common_data.tries),
            status: Mutex::new(TaskStatus {
                waitting_network_time: None,
                mtime: get_current_timestamp(),
                state: State::from(info.progress.common_data.state),
                reason: Reason::from(info.common_data.reason),
            }),
            retry: AtomicBool::new(info.common_data.retry),
            get_file_info: AtomicBool::new(false),
            retry_for_request: AtomicBool::new(false),
            retry_for_speed: AtomicBool::new(false),
            code: Mutex::new(vec![Reason::Default; file_count]),
            background_notify_time: AtomicU64::new(get_current_timestamp()),
            file_total_size: AtomicI64::new(-1),
            resume: AtomicBool::new(false),
            seek_flag: AtomicBool::new(false),
            range_request: AtomicBool::new(false),
            range_response: AtomicBool::new(false),
            restored: AtomicBool::new(true),
            skip_bytes: AtomicU64::new(0),
            upload_counts: AtomicU32::new(progress_index as u32),
            client: None,
        };
        task.client = task.build_client();
        match action {
            Action::UPLOAD =>
                task.file_total_size.store(task.get_upload_file_total_size() as i64, Ordering::SeqCst),
            Action::DOWNLOAD =>
                task.file_total_size.store(info.progress.sizes[progress_index] as i64, Ordering::SeqCst),
            _ => {}
        }
        task

    }

    pub fn build_notify_data(&self) -> NotifyData {
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
            task_id: self.task_id,
            uid: self.uid,
            bundle: self.conf.bundle.clone(),
        }
    }

    pub fn record_waitting_network_time(&self) {
        let mut staus = self.status.lock().unwrap();
        staus.waitting_network_time = Some(get_current_timestamp());
    }

    pub fn check_net_work_status(&self) -> bool {
        if !self.is_satisfied_configuration() {
            if self.conf.version == Version::API10
                && self.conf.common_data.mode == Mode::BACKGROUND
                && self.conf.common_data.retry
            {
                self.set_status(State::WAITING, Reason::UnSupportedNetWorkType);
            } else {
                self.set_status(State::FAILED, Reason::UnSupportedNetWorkType);
            }
            return false;
        }
        true
    }

    pub fn net_work_online(&self) -> bool {
        if unsafe { !IsOnline() } {
            if self.conf.version == Version::API10
                && self.conf.common_data.mode == Mode::BACKGROUND
                && self.conf.common_data.retry
            {
                self.set_status(State::WAITING, Reason::NetWorkOffline);
            } else {
                let retry_times = 20;
                for _ in 0..retry_times {
                    if unsafe { IsOnline() } {
                        return true;
                    }
                    sleep(Duration::from_millis(RETRY_INTERVAL));
                }
                self.set_status(State::FAILED, Reason::NetWorkOffline);
            }
            return false;
        }
        true
    }

    fn build_client(&self) -> Option<Client> {
        let mut client = Client::builder()
            .connect_timeout(Timeout::from_secs(CONNECT_TIMEOUT))
            .request_timeout(Timeout::from_secs(SECONDS_IN_ONE_WEEK))
            .min_tls_version(TlsVersion::TLS_1_2);

        if self.conf.common_data.redirect {
            client = client.redirect(Redirect::limited(usize::MAX));
        } else {
            client = client.redirect(Redirect::none());
        }

        if self.conf.url.contains("https") {
            let mut buf = Vec::new();
            let file = File::open("/etc/ssl/certs/cacert.pem");
            match file {
                Ok(mut f) => {
                    f.read_to_end(&mut buf).unwrap();
                    let cert = Certificate::from_pem(&buf).unwrap();
                    client = client.add_root_certificate(cert);
                }
                Err(e) => {
                    error!(LOG_LABEL, "open cacert.pem failed, error is {:?}",  @public(e));
                    self.set_status(State::FAILED, Reason::IoError);
                    return None;
                }
            }
        }
        let result = client.build();
        match result {
            Ok(value) => Some(value),
            Err(e) => {
                error!(LOG_LABEL, "build client error is {:?}",  @public(e));
                self.set_status(State::FAILED, Reason::BuildClientFailed);
                return None;
            }
        }
    }

    fn build_request_builder(&self) -> RequestBuilder {
        let url = self.conf.url.clone();
        let method = match self.conf.method.to_uppercase().as_str() {
            "PUT" => "PUT",
            "POST" => "POST",
            "GET" => "GET",
            _ => match self.conf.common_data.action {
                Action::UPLOAD => {
                    if self.conf.version == Version::API10 {
                        "PUT"
                    } else {
                        "POST"
                    }
                }
                Action::DOWNLOAD => "GET",
                _ => "",
            },
        };
        let method = Method::try_from(method).unwrap();
        let mut request = RequestBuilder::new().method(method).url(url.as_str());
        for (key, value) in self.conf.headers.iter() {
            request = request.header(key.as_str(), value.as_str());
        }
        request
    }

    async fn clear_downloaded_file(&self) -> bool {
        let file = unsafe { &mut *self.files.0.get() }.get_mut(0).unwrap();
        let res = file.set_len(0).await;
        match res {
            Err(e) => {
                error!(LOG_LABEL, "clear download file error: {:?}", e);
                self.set_status(State::FAILED, Reason::IoError);
                false
            }
            _ => {
                debug!(LOG_LABEL, "set len success");
                match file.seek(SeekFrom::Start(0)).await{
                    Err(e) => {
                        error!(LOG_LABEL, "seek err is {:?}", e);
                        self.set_status(State::FAILED, Reason::IoError);
                        false
                    }
                    Ok(_) => {
                        debug!(LOG_LABEL, "seek success");
                        let mut progress_guard = self.progress.lock().unwrap();
                        progress_guard.common_data.total_processed = 0;
                        progress_guard.processed[0] = 0;
                        true
                    }
                }
            }
        } 
    }

    async fn build_download_request(&self) -> Option<Request<String>> {
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
                    if etag.is_some() {
                        request_builder = request_builder.header("If-Range", etag.unwrap().as_str());
                        true
                    } else if last_modified.is_some() {
                        request_builder = request_builder.header("If-Range", last_modified.unwrap().as_str());
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
            begins += self.progress.lock().unwrap().processed[0] as u64;
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
        let result = request_builder.body(self.conf.data.clone());
        match result {
            Ok(value) => {
                return Some(value);
            }
            Err(e) => {
                error!(LOG_LABEL, "build download request error is {:?}",  @public(e));
                self.set_status(State::FAILED, Reason::BuildRequestFailed);
                return None;
            }
        }
    }

    fn get_file_info(&self, response: &Response) -> bool {
        if self.get_file_info.load(Ordering::SeqCst) {
            return true;
        }
        self.get_file_info.store(true, Ordering::SeqCst);
        let content_type = response.headers().get("content-type");
        if let Some(mime_type) = content_type {
            if let Ok(value) = mime_type.to_str() {
                *self.mime_type.lock().unwrap() = value.into();
            }
        }

        let content_length = response.headers().get("content-length");
        if let Some(len) = content_length {
            let length = len.to_str();
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
                            debug!(LOG_LABEL, "the download task content-length is {}",  @public(v));
                        }
                        Err(e) => { error!(LOG_LABEL, "convert string to i64 error: {:?}",  @public(e)); },
                    }
                }
                Err(e) => { error!(LOG_LABEL, "convert header value to string error: {:?}",  @public(e)); },
            }
        } else {
            error!(LOG_LABEL, "cannot get content-length of the task");
            if self.conf.common_data.precise {
                self.set_status(State::FAILED, Reason::GetFileSizeFailed);
                return false;
            }
        }
        true
    }

    fn handle_body_transfer_error(&self) {
        if unsafe { !IsOnline() } {
            match self.conf.version {
                Version::API9 => {
                    if self.conf.common_data.action == Action::DOWNLOAD {
                        self.set_status(State::WAITING, Reason::NetWorkOffline);
                    } else {
                        self.set_status(State::FAILED, Reason::NetWorkOffline);
                    }
                }
                Version::API10 => {
                    if self.conf.common_data.mode == Mode::FRONTEND || !self.conf.common_data.retry
                    {
                        self.set_status(State::FAILED, Reason::NetWorkOffline);
                    } else {
                        self.set_status(State::WAITING, Reason::NetWorkOffline);
                    }
                }
            }
        } else {
            let index = self.progress.lock().unwrap().common_data.index;
            self.set_code(index, Reason::OthersError);
        }
    }

    fn handle_download_error(&self, result: &Result<(), HttpClientError>) -> bool {
        match result {
            Ok(_) => return true,
            Err(err) => {
                error!(LOG_LABEL, "download err is {:?}", @public(err));
                match err.error_kind() {
                    ErrorKind::Timeout => {
                        self.set_status(State::FAILED, Reason::ContinuousTaskTimeOut);
                    }
                    // user triggered
                    ErrorKind::UserAborted => return true,
                    ErrorKind::BodyTransfer => self.handle_body_transfer_error(),
                    _ => {
                        self.set_status(State::FAILED, Reason::OthersError);
                    }
                }
                return false;
            }
        }
    }

    async fn handle_response_error(&self, response: &Result<Response, HttpClientError>) -> bool {
        let index = self.progress.lock().unwrap().common_data.index;
        match response {
            Ok(r) => {
                let http_response_code = r.status();
                info!(LOG_LABEL, "the http response code is {}", @public(http_response_code));
                if http_response_code.is_server_error()
                    || (http_response_code.as_u16() != 408
                        && http_response_code.is_client_error())
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
                        206 => { self.range_response.store(true, Ordering::SeqCst); },
                        200 => {
                            self.range_response.store(false, Ordering::SeqCst);
                            if self.resume.load(Ordering::SeqCst) {
                                if !self.clear_downloaded_file().await {
                                    return false;
                                }
                            } else {
                                self.set_code(index, Reason::UnSupportRangeRequest);
                                return false;
                            }
                        },
                        _ => {},
                    }
                }
                return true;
            }
            Err(e) => {
                error!(LOG_LABEL, "http client err is {:?}", @public(e));
                match e.error_kind() {
                    ErrorKind::UserAborted => self.set_code(index, Reason::UserOperation),
                    ErrorKind::Timeout => self.set_code(index, Reason::ContinuousTaskTimeOut),
                    ErrorKind::Request => self.set_code(index, Reason::RequestError),
                    ErrorKind::Redirect => self.set_code(index, Reason::RedirectError),
                    ErrorKind::Connect | ErrorKind::ConnectionUpgrade => self.set_code(index, Reason::ConnectError),
                    ErrorKind::BodyTransfer => self.handle_body_transfer_error(),
                    _ => self.set_code(index, Reason::OthersError),
                }
                return false;
            }
        }
    }

    fn record_response_header(&self, response: &Result<Response, HttpClientError>) {
        if let Ok(r) = response {
            let mut guard = self.progress.lock().unwrap();
            guard.extras.clear();
            for (k, v) in r.headers() {
                if let Ok(value) = v.to_str() {
                    guard.extras.insert(k.to_string().to_lowercase(), value.into());
                }
            }
        }
    }

    async fn record_upload_response(
        &self,
        index: usize,
        response: Result<Response, HttpClientError>,
    ) {
        self.record_response_header(&response);
        if let Ok(mut r) = response {
            let mut yfile = match unsafe { &mut *self.body_files.0.get() }.get_mut(index) {
                Some(yfile) => match yfile.take() {
                    Some(yf) => yf,
                    None => return,
                },
                None => return,
            };

            loop {
                let mut buf = [0u8; 1024];
                let size = r.body_mut().data(&mut buf).await;
                let size = match size {
                    Ok(size) => size,
                    Err(_e) => break,
                };

                if size == 0 {
                    break;
                }
                let r = yfile.write_all(&buf[..size]).await;
            }
            // Makes sure all the data has been written to the target file.
            let _ = yfile.sync_all().await;
        }
        if self.conf.version == Version::API9 && self.conf.common_data.action == Action::UPLOAD {
            let notify_data = self.build_notify_data();
            TaskManager::get_instance().front_notify("headerReceive".into(), &notify_data);
        }
    }

    fn set_code(&self, index: usize, code: Reason) {
        if code == Reason::UploadFileError {
            return;
        }
        let mut code_guard = self.code.lock().unwrap();
        if index < code_guard.len() {
            if code_guard[index] == Reason::Default {
                debug!(LOG_LABEL, "set code");
                code_guard[index] = code;
            }
        }
    }

    fn reset_code(&self, index: usize) {
        let file_counts = self.conf.file_specs.len();
        let mut code_guard = self.code.lock().unwrap();
        if index < file_counts {
            debug!(LOG_LABEL, "reset code");
            code_guard[index] = Reason::Default;
        }
    }

    pub fn set_status(&self, state: State, reason: Reason) -> bool {
        debug!(LOG_LABEL, "set status");
        {
            let mut current_status = self.status.lock().unwrap();
            if state == current_status.state && reason == current_status.reason {
                return true;
            }
            let mut progress_guard = self.progress.lock().unwrap();
            let index = progress_guard.common_data.index;
            let current_state = current_status.state;
            debug!(LOG_LABEL, "set state {:?}, reason {:?} current_state {:?}",
                @public(state), @public(reason), @public(current_state));
            match state {
                State::PAUSED | State::STOPPED => {
                    if current_state != State::RUNNING
                        && current_state != State::RETRYING
                        && current_state != State::WAITING
                    {
                        return false;
                    }
                    self.set_code(index, reason);
                }
                State::COMPLETED => {
                    if current_state != State::RUNNING && current_state != State::RETRYING {
                        return false;
                    }
                }
                State::FAILED | State::WAITING => {
                    if current_state == State::COMPLETED || current_state == State::REMOVED
                        || current_state == State::STOPPED || current_state == State::FAILED
                    {
                        return false;
                    }
                    self.set_code(index, reason);
                    if state == State::FAILED {
                        let file_counts = self.conf.file_specs.len();
                        for i in index..file_counts {
                            self.set_code(i, reason);
                        }
                    }
                }
                State::REMOVED => self.set_code(index, reason),
                _ => {}
            }
            current_status.mtime = get_current_timestamp();
            progress_guard.common_data.state = state as u8;
            current_status.state = state;
            current_status.reason = reason;
            info!(LOG_LABEL, "current state is {:?}, reason is {:?}", @public(state), @public(reason));
        }
        if state == State::WAITING {
            self.record_waitting_network_time();
        }
        self.record_task_info();
        self.state_change_notify(state);
        true
    }

    fn state_change_notify(&self, state: State) {
        if state == State::INITIALIZED
            || (self.progress.lock().unwrap().common_data.total_processed == 0
                && (state == State::RUNNING || state == State::RETRYING))
        {
            return;
        }
        debug!(LOG_LABEL, "state change notification");
        let version = self.conf.version;
        let mode = self.conf.common_data.mode;
        let notify_data = self.build_notify_data();
        let bundle = self.conf.bundle.clone();
        TaskManager::get_instance().front_notify("progress".into(), &notify_data);
        match state {
            State::COMPLETED => {
                unsafe { PublishStateChangeEvents(CString::new(bundle.as_str()).unwrap().as_ptr(), bundle.len() as u32, self.task_id, State::COMPLETED as i32); }
                TaskManager::get_instance().front_notify("complete".into(), &notify_data)
            }
            State::FAILED => {
                unsafe { PublishStateChangeEvents(CString::new(bundle.as_str()).unwrap().as_ptr(), bundle.len() as u32, self.task_id, State::FAILED as i32); }
                TaskManager::get_instance().front_notify("fail".into(), &notify_data)
            }
            State::PAUSED | State::WAITING => {
                TaskManager::get_instance().front_notify("pause".into(), &notify_data)
            }
            State::REMOVED => {
                TaskManager::get_instance().front_notify("remove".into(), &notify_data)
            }
            _ => {}
        }
        self.background_notify();
    }

    fn record_task_info(&self) {
        TaskManager::get_instance().recording_rdb_num.fetch_add(1, Ordering::SeqCst);
        let has_record = unsafe { HasRequestTaskRecord(self.task_id) };
        if !has_record {
            let task_info = self.show();
            let info_set = task_info.build_info_set();
            let c_task_info = task_info.to_c_struct(&info_set);
            let ret = unsafe { RecordRequestTaskInfo(&c_task_info) };
            info!(LOG_LABEL, "insert database ret is {}", @public(ret));
        } else {
            let update_info = self.get_update_info();
            let sizes: String = format!("{:?}", update_info.progress.sizes);
            let processed: String = format!("{:?}", update_info.progress.processed);
            let extras = hashmap_to_string(&update_info.progress.extras);
            let each_file_status = update_info.each_file_status.iter().map(|x| x.to_c_struct()).collect();
            let c_update_info = update_info.to_c_struct(&sizes, &processed, &extras, &each_file_status);
            let ret = unsafe { UpdateRequestTaskInfo(self.task_id, &c_update_info)};
            info!(LOG_LABEL, "update database ret is {}", @public(ret));
        }
        TaskManager::get_instance().recording_rdb_num.fetch_sub(1, Ordering::SeqCst);
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

    fn get_update_info(&self) -> UpdateInfo{
        let status = self.status.lock().unwrap();
        let progress = self.progress.lock().unwrap();
        UpdateInfo {
            mtime: status.mtime,
            reason: status.reason as u8,
            tries: self.tries.load(Ordering::SeqCst),
            progress: progress.clone(),
            each_file_status: self.get_each_file_status(),
        }
    }

    pub fn show(&self) -> TaskInfo {
        let status = self.status.lock().unwrap();
        let progress = self.progress.lock().unwrap();
        TaskInfo {
            bundle: self.conf.bundle.clone(),
            url: self.conf.url.clone(),
            data: self.conf.data.clone(),
            token: self.conf.token.clone(),
            form_items: self.conf.form_items.clone(),
            file_specs: self.conf.file_specs.clone(),
            body_file_names: self.conf.body_file_names.clone(),
            title: self.conf.title.clone(),
            description: self.conf.description.clone(),
            mime_type: {
                match self.conf.version {
                    Version::API10 => match self.conf.common_data.action {
                        Action::DOWNLOAD => match self.conf.headers.get("Content-Type") {
                            None => "".into(),
                            Some(v) => v.clone(),
                        },
                        Action::UPLOAD => "multipart/form-data".into(),
                        _ => "".into(),
                    },
                    Version::API9 => self.mime_type.lock().unwrap().clone(),
                }
            },
            progress: progress.clone(),
            extras: progress.extras.clone(),
            each_file_status: self.get_each_file_status(),
            common_data: CommonTaskInfo {
                task_id: self.task_id,
                uid: self.uid,
                action: self.conf.common_data.action as u8,
                mode: self.conf.common_data.mode as u8,
                ctime: self.ctime,
                mtime: status.mtime,
                reason: status.reason as u8,
                gauge: self.conf.common_data.gauge,
                retry: match self.conf.common_data.mode {
                    Mode::FRONTEND => false,
                    _ => self.conf.common_data.retry,
                },
                tries: self.tries.load(Ordering::SeqCst),
                version: self.conf.version as u8,
            },
        }
    }

    // only use for download task
    pub fn query_mime_type(&self) -> String {
        self.mime_type.lock().unwrap().clone()
    }

    pub fn is_satisfied_configuration(&self) -> bool {
        if self.conf.common_data.network == Network::ANY {
            return true;
        }
        unsafe {
            let network_info = GetNetworkInfo();
            if (!self.conf.common_data.roaming && (*network_info).isRoaming) {
                error!(LOG_LABEL, "not allow roaming");
                return false;
            }
            if (!self.conf.common_data.metered && (*network_info).isMetered) {
                error!(LOG_LABEL, "not allow metered");
                return false;
            }
            if ((*network_info).networkType != self.conf.common_data.network) {
                error!(LOG_LABEL, "dismatch network type");
                return false;
            }
        };
        true
    }

    fn background_notify(&self) {
        if self.conf.version == Version::API9 && !self.conf.common_data.background {
            return;
        }
        if self.conf.version == Version::API10 && self.conf.common_data.mode == Mode::FRONTEND {
            return;
        }
        let mut file_total_size = self.file_total_size.load(Ordering::SeqCst);
        let total_processed = self.progress.lock().unwrap().common_data.total_processed as u64;
        if file_total_size <= 0 || total_processed == 0 {
            return;
        }
        if self.conf.common_data.action == Action::DOWNLOAD {
            if self.conf.common_data.ends < 0 {
                file_total_size -= self.conf.common_data.begins as i64;
            } else {
                file_total_size = self.conf.common_data.ends - self.conf.common_data.begins as i64 + 1;
            }
        }
        self.background_notify_time.store(get_current_timestamp(), Ordering::SeqCst);
        let index = self.progress.lock().unwrap().common_data.index;
        if index >= self.conf.file_specs.len() {
            return;
        }
        let file_path = self.conf.file_specs[index].path.as_ptr() as *const c_char;
        let file_path_len = self.conf.file_specs[index].path.as_bytes().len() as i32;
        let percent = total_processed * 100 / (file_total_size as u64);
        debug!(LOG_LABEL, "background notify");
        let task_msg = RequestTaskMsg {
            task_id: self.task_id,
            uid: self.uid as i32,
            action: self.conf.common_data.action as u8,
        };
        unsafe {
            RequestBackgroundNotify(
                task_msg,
                file_path,
                file_path_len,
                percent as u32,
            );
        };
    }

    fn get_upload_info(&self, index: usize) -> (bool, u64) {
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
        return (is_partial_upload, upload_file_length);
    }

    fn get_upload_file_total_size(&self) -> u64 {
        let mut file_total_size = 0;
        for i in 0..self.conf.file_specs.len() {
            let (_, upload_size) = self.get_upload_info(i);
            file_total_size += upload_size;
        }
        file_total_size
    }
}

pub async fn run(task: Arc<RequestTask>) {
    info!(LOG_LABEL, "run the task which id is {}", @public(task.task_id));
    if !task.net_work_online() || !task.check_net_work_status() {
        return;
    }
    let action = task.conf.common_data.action;
    match action {
        Action::DOWNLOAD => loop {
            task.reset_code(0);
            download(task.clone()).await;
            let state = task.status.lock().unwrap().state;
            if state != State::RUNNING && state != State::RETRYING {
                break;
            }
            let code = task.code.lock().unwrap()[0];
            if code != Reason::Default {
                task.set_status(State::FAILED, code);
                break;
            }
        },
        Action::UPLOAD => {
            let state = task.status.lock().unwrap().state;
            if state == State::RETRYING {
                let index = {
                    let mut progress_guard = task.progress.lock().unwrap();
                    let index = progress_guard.common_data.index;
                    progress_guard.common_data.total_processed -= progress_guard.processed[index];
                    progress_guard.processed[index] = 0;
                    index
                };
                let file = unsafe { &mut *task.files.0.get() }.get_mut(index).unwrap();
                let mut begins = task.conf.common_data.begins;
                let (is_partial_upload, _) = task.get_upload_info(index);
                if !is_partial_upload {
                    begins = 0;
                }
                match file.seek(SeekFrom::Start(begins)).await {
                    Err(e) => {
                        task.set_code(index, Reason::IoError);
                        error!(LOG_LABEL, "seek err is {:?}", e);
                    }
                    Ok(_) => {}
                }
            }
            upload(task.clone()).await;
        }
        _ => {},
    }
    info!(LOG_LABEL, "run end");
}

async fn download(task: Arc<RequestTask>) {
    download_inner(task.clone()).await;

    // If `Reason` is not `Default`, records this sys event.
    let reason = task.code.lock().unwrap()[0];
    if reason != Reason::Default {
        SysEvent::task_fault()
            .param(build_str_param!(SysEvent::TASKS_TYPE, "DOWNLOAD"))
            .param(build_number_param!(SysEvent::TOTAL_FILE_NUM, 1))
            .param(build_number_param!(SysEvent::FAIL_FILE_NUM, 1))
            .param(build_number_param!(SysEvent::SUCCESS_FILE_NUM, 0))
            .param(build_number_param!(SysEvent::ERROR_INFO, reason as i32))
            .write();
    }
}

async fn download_inner(task: Arc<RequestTask>) {
    info!(LOG_LABEL, "begin download");

    // Ensures `_trace` can only be freed when this function exits.
    let _trace = TraceScope::trace("download file");

    if task.client.is_none() {
        return;
    }
    let request = task.build_download_request().await;
    if request.is_none() {
        return;
    }
    let request = request.unwrap();

    let name = task.conf.file_specs[0].path.as_str();
    let download = task.progress.lock().unwrap().processed[0];
    // Ensures `_trace` can only be freed when this function exits.
    let _trace = TraceScope::trace(
        &format!("download file name: {name} downloaded size: {download}")
    );

    let response = task.client.as_ref().unwrap().request(request).await;
    task.record_response_header(&response);
    if !task.handle_response_error(&response).await {
        error!(LOG_LABEL, "response error");
        return;
    }
    let response = response.unwrap();
    if !task.get_file_info(&response) {
        return;
    }
    let mut downloader = build_downloader(task.clone(), response);
    let result = downloader.download().await;
    if !task.handle_download_error(&result) {
        error!(LOG_LABEL, "handle_download_error");
        return;
    }
    // Makes sure all the data has been written to the target file.
    if let Some(file) = task.files.get(0) {
        let _ = file.sync_all().await;
    }
    task.set_status(State::COMPLETED, Reason::Default);
}

fn build_downloader(task: Arc<RequestTask>, response: Response) -> Downloader<TaskOperator> {
    let task_operator = TaskOperator { task };
    let downloader = Downloader::builder()
        .body(response)
        .operator(task_operator)
        .timeout(Timeout::from_secs(SECONDS_IN_ONE_WEEK))
        .speed_limit(SpeedLimit::new().min_speed(LOW_SPEED_LIMIT, LOW_SPEED_TIME))
        .build();
    downloader
}

async fn upload(task: Arc<RequestTask>) {
    info!(LOG_LABEL, "begin upload");

    let url = task.conf.url.as_str();
    let num = task.conf.file_specs.len();
    // Ensures `_trace` can only be freed when this function exits.
    let _trace = TraceScope::trace(&format!("exec upload task url: {url} file num: {num}"));

    let size = task.conf.file_specs.len();
    if task.client.is_none() {
        return;
    }
    let index = task.progress.lock().unwrap().common_data.index;
    info!(LOG_LABEL, "index is {}", @public(index));
    for i in index..size {
        task.progress.lock().unwrap().common_data.index = i;
        let result: bool;
        match task.conf.headers.get("Content-Type") {
            None => {
                if task.conf.method.to_uppercase().eq("POST") {
                    result = upload_one_file(task.clone(), i, build_multipart_request).await;
                } else {
                    result = upload_one_file(task.clone(), i, build_stream_request).await;
                }
            }
            Some(v) => {
                if v == "multipart/form-data" {
                    result = upload_one_file(task.clone(), i, build_multipart_request).await;
                } else {
                    result = upload_one_file(task.clone(), i, build_stream_request).await;
                }
            }
        }
        if result {
            info!(LOG_LABEL, "upload one file success, which index is {}", @public(i));
            task.upload_counts.fetch_add(1, Ordering::SeqCst);
        }
        let state = task.status.lock().unwrap().state;
        if state != State::RUNNING && state != State::RETRYING {
            return;
        }
    }

    let uploaded = task.upload_counts.load(Ordering::SeqCst);
    if uploaded == size as u32 {
        task.set_status(State::COMPLETED, Reason::Default);
    } else {
        task.set_status(State::FAILED, Reason::UploadFileError);

        // Records sys event.
        SysEvent::task_fault()
            .param(build_str_param!(SysEvent::TASKS_TYPE, "UPLOAD"))
            .param(build_number_param!(SysEvent::TOTAL_FILE_NUM, size))
            .param(build_number_param!(SysEvent::FAIL_FILE_NUM, size as u32 - uploaded))
            .param(build_number_param!(SysEvent::SUCCESS_FILE_NUM, uploaded))
            .param(build_number_param!(SysEvent::ERROR_INFO, Reason::UploadFileError as i32))
            .write();
    }

    info!(LOG_LABEL, "upload end");
}

async fn upload_one_file<F, T>(
    task: Arc<RequestTask>,
    index: usize,
    build_upload_request: F,
) -> bool
where
    F: Fn(Arc<RequestTask>, usize) -> Option<Request<T>>,
    T: Body,
{
    info!(LOG_LABEL, "begin upload one file");

    let (_, size) = task.get_upload_info(index);
    let name = task.conf.file_specs[index].file_name.as_str();
    // Ensures `_trace` can only be freed when this function exits.
    let _trace = TraceScope::trace(&format!("upload file name:{name} index:{index} size:{size}"));

    loop {
        task.reset_code(index);
        let request = build_upload_request(task.clone(), index);
        if request.is_none() {
            return false;
        }
        let response = task.client.as_ref().unwrap().request(request.unwrap()).await;
        if task.handle_response_error(&response).await {
            task.code.lock().unwrap()[index] = Reason::Default;
            task.record_upload_response(index, response).await;
            return true;
        }
        task.record_upload_response(index, response).await;
        let code = task.code.lock().unwrap()[index];
        if code != Reason::Default {
            error!(LOG_LABEL, "upload {} file fail, which reason is {}", @public(index), @public(code as u32));
            return false;
        }
        let state = task.status.lock().unwrap().state;
        if state != State::RUNNING && state != State::RETRYING {
            return false;
        }
    }
}

fn build_stream_request(
    task: Arc<RequestTask>,
    index: usize,
) -> Option<Request<Uploader<TaskReader, TaskOperator>>> {
    info!(LOG_LABEL, "build stream request");
    let task_reader = TaskReader { task: task.clone() };
    let task_operator = TaskOperator { task: task.clone() };
    let mut request_builder = task.build_request_builder();
    if task.conf.headers.get("Content-Type").is_none() {
        request_builder = request_builder.header("Content-Type", "application/octet-stream");
    }
    let (_, upload_length) = task.get_upload_info(index);
    info!(LOG_LABEL, "upload length is {}", @public(upload_length));
    request_builder = request_builder.header("Content-Length", upload_length.to_string().as_str());
    let uploader = Uploader::builder()
        .reader(task_reader)
        .operator(task_operator)
        .total_bytes(Some(upload_length as u64))
        .build();
    let request = request_builder.body(uploader);
    build_request_common(&task, index, request)
}

fn build_multipart_request(
    task: Arc<RequestTask>,
    index: usize,
) -> Option<Request<Uploader<MultiPart, TaskOperator>>> {
    info!(LOG_LABEL, "build multipart request");
    let task_reader = TaskReader { task: task.clone() };
    let task_operator = TaskOperator { task: task.clone() };
    let mut multi_part = MultiPart::new();
    for item in task.conf.form_items.iter() {
        let part = Part::new()
            .name(item.name.as_str())
            .body(item.value.as_str());
        multi_part = multi_part.part(part);
    }
    let (_, upload_length) = task.get_upload_info(index);
    info!(LOG_LABEL, "upload length is {}", @public(upload_length));
    let part = Part::new()
        .name(task.conf.file_specs[index].name.as_str())
        .file_name(task.conf.file_specs[index].file_name.as_str())
        .mime(task.conf.file_specs[index].mime_type.as_str())
        .length(Some(upload_length))
        .stream(task_reader);

    multi_part = multi_part.part(part);
    let uploader = Uploader::builder()
        .multipart(multi_part)
        .operator(task_operator)
        .build();

    let request_builder = task.build_request_builder();
    let request: Result<Request<Uploader<MultiPart, TaskOperator>>, HttpClientError> =
        request_builder.multipart(uploader);
    build_request_common(&task, index, request)
}

fn build_request_common<T: Body>(
    task: &Arc<RequestTask>,
    index: usize,
    request: Result<Request<T>, HttpClientError>,
) -> Option<Request<T>> {
    match request {
        Ok(value) => {
            info!(LOG_LABEL, "build upload request success");
            return Some(value);
        }
        Err(e) => {
            error!(LOG_LABEL, "build upload request error is {:?}", @public(e));
            {
                let mut guard = task.code.lock().unwrap();
                for i in index..guard.len() {
                    guard[i] = Reason::BuildRequestFailed;
                }
            }
            task.set_status(State::FAILED, Reason::BuildRequestFailed);
            return None;
        }
    }
}
