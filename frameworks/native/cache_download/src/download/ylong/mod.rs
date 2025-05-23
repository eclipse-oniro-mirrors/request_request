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

mod client;

use std::collections::HashMap;
use std::pin::Pin;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::Arc;
use std::task::{Context, Poll};

use client::client;
use ylong_http_client::async_impl::{
    Body, DownloadOperator, Downloader, PercentEncoder, RequestBuilder,
};
use ylong_http_client::{ErrorKind, HttpClientError, StatusCode};

use super::callback::PrimeCallback;
use super::common::{CommonHandle, CommonError, CommonResponse};
use crate::services::DownloadRequest;

impl CommonError for HttpClientError {
    fn code(&self) -> i32 {
        self.error_kind() as i32
    }

    fn msg(&self) -> String {
        self.to_string()
    }
}

impl CommonResponse for Response {
    fn code(&self) -> u32 {
        self.status().as_u16() as u32
    }
}

struct Operator<'a> {
    callback: &'a mut PrimeCallback,
    abort_flag: Arc<AtomicBool>,
    headers: HashMap<String, String>,
}

impl<'a> DownloadOperator for Operator<'a> {
    fn poll_download(
        self: Pin<&mut Self>,
        _cx: &mut Context<'_>,
        data: &[u8],
    ) -> Poll<Result<usize, HttpClientError>> {
        let me = self.get_mut();
        me.callback.common_data_receive(data, || {
            me.headers.get("content-length").map(|v| v.parse().unwrap())
        });
        Poll::Ready(Ok(data.len()))
    }

    fn poll_progress(
        self: Pin<&mut Self>,
        _cx: &mut Context<'_>,
        downloaded: u64,
        total: Option<u64>,
    ) -> Poll<Result<(), HttpClientError>> {
        let me = self.get_mut();
        me.callback
            .common_progress(total.unwrap_or_default(), downloaded, 0, 0);
        if me.abort_flag.load(Ordering::Acquire) {
            Poll::Ready(Err(HttpClientError::user_aborted()))
        } else {
            Poll::Ready(Ok(()))
        }
    }
}

pub struct DownloadTask;

impl DownloadTask {
    pub(super) fn run(
        request: DownloadRequest,
        mut callback: PrimeCallback,
    ) -> Arc<dyn CommonHandle> {
        let url = match PercentEncoder::encode(request.url) {
            Ok(url) => url,
            Err(e) => {
                callback.common_fail(e);
                return Arc::new(CancelHandle::new(Arc::new(AtomicBool::new(false))));
            }
        };
        callback.set_running();
        let flag = Arc::new(AtomicBool::new(false));
        let handle = Arc::new(CancelHandle::new(flag.clone()));
        let mut headers = None;
        if let Some(h) = request.headers {
            headers = Some(
                h.iter()
                    .map(|a| (a.0.to_string(), a.1.to_string()))
                    .collect(),
            );
        }
        ylong_runtime::spawn(async move {
            if let Err(e) = download(url, headers, &mut callback, flag).await {
                if e.error_kind() == ErrorKind::UserAborted {
                    callback.common_cancel();
                } else {
                    callback.common_fail(e);
                }
            }
        });
        handle
    }
}

pub async fn download(
    url: String,
    headers: Option<Vec<(String, String)>>,
    callback: &mut PrimeCallback,
    abort_flag: Arc<AtomicBool>,
) -> Result<(), HttpClientError> {
    let mut request = RequestBuilder::new().url(url.as_str()).method("GET");

    if let Some(header) = headers {
        for (k, v) in header {
            request = request.append_header(k.as_str(), v.as_str());
        }
    }
    let request = request.body(Body::empty()).unwrap();

    let response = client().request(request).await?;
    let status = response.status();

    let operator = Operator {
        callback: callback,
        abort_flag: abort_flag,
        headers: response
            .headers()
            .into_iter()
            .map(|(key, value)| (key.to_string(), value.to_string().unwrap()))
            .collect(),
    };
    let mut downloader = Downloader::builder()
        .body(response)
        .operator(operator)
        .build();
    downloader.download().await?;

    let response = Response { status: status };
    callback.common_success(response);
    Ok(())
}

pub struct Response {
    status: StatusCode,
}

impl Response {
    pub fn status(&self) -> StatusCode {
        self.status
    }
}

pub struct CancelHandle {
    inner: Arc<AtomicBool>,
    count: AtomicUsize,
}

impl CancelHandle {
    fn new(inner: Arc<AtomicBool>) -> Self {
        Self {
            inner,
            count: (AtomicUsize::new(1)),
        }
    }
}

impl CommonHandle for CancelHandle {
    fn cancel(&self) -> bool {
        if self.count.fetch_sub(1, std::sync::atomic::Ordering::SeqCst) == 1 {
            self.inner.store(true, Ordering::Release);
            true
        } else {
            false
        }
    }

    fn add_count(&self) {
        self.count.fetch_add(1, std::sync::atomic::Ordering::SeqCst);
    }
}
