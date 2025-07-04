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

use std::collections::hash_map::Entry;
use std::collections::HashMap;
use std::sync::{Arc, Mutex, Once, OnceLock};

use cache_core::{CacheManager, RamCache};
use netstack_rs::info::{DownloadInfo, DownloadInfoMgr};
use request_utils::observe::network::NetRegistrar;
use request_utils::task_id::TaskId;

use crate::download::task::{DownloadTask, Downloader, TaskHandle};
use crate::download::CacheDownloadError;
use crate::observe::NetObserver;

#[allow(unused_variables)]
pub trait PreloadCallback: Send {
    fn on_success(&mut self, data: Arc<RamCache>, task_id: &str) {}
    fn on_fail(&mut self, error: CacheDownloadError, task_id: &str) {}
    fn on_cancel(&mut self) {}
    fn on_progress(&mut self, progress: u64, total: u64) {}
}

pub struct CacheDownloadService {
    running_tasks: Mutex<HashMap<TaskId, Arc<Mutex<DownloadTask>>>>,
    cache_manager: CacheManager,
    info_mgr: Arc<DownloadInfoMgr>,
    net_registrar: NetRegistrar,
}

pub struct DownloadRequest<'a> {
    pub url: &'a str,
    pub headers: Option<Vec<(&'a str, &'a str)>>,
}

impl<'a> DownloadRequest<'a> {
    pub fn new(url: &'a str) -> Self {
        Self { url, headers: None }
    }

    pub fn headers(&mut self, headers: Vec<(&'a str, &'a str)>) -> &mut Self {
        self.headers = Some(headers);
        self
    }
}

impl CacheDownloadService {
    fn new() -> Self {
        Self {
            running_tasks: Mutex::new(HashMap::new()),
            cache_manager: CacheManager::new(),
            info_mgr: Arc::new(DownloadInfoMgr::new()),
            net_registrar: NetRegistrar::new(),
        }
    }

    pub fn get_instance() -> &'static Self {
        static DOWNLOAD_AGENT: OnceLock<CacheDownloadService> = OnceLock::new();
        static ONCE: Once = Once::new();
        let cache_download = DOWNLOAD_AGENT.get_or_init(CacheDownloadService::new);

        ONCE.call_once(|| {
            let old_hook = std::panic::take_hook();
            std::panic::set_hook(Box::new(move |info| {
                error!("Panic occurred {:?}", info);
                old_hook(info);
            }));
            cache_download.cache_manager.restore_files();
            cache_download.net_registrar.add_observer(NetObserver);
            if let Err(e) = cache_download.net_registrar.register() {
                error!("Failed to register network observer: {:?}", e);
            }
        });

        cache_download
    }

    pub fn cancel(&self, url: &str) {
        let task_id = TaskId::from_url(url);
        if let Some(updater) = self.running_tasks.lock().unwrap().get(&task_id).cloned() {
            updater.lock().unwrap().cancel();
        }
    }

    pub(crate) fn reset_all_tasks(&self) {
        let running_tasks = self.running_tasks.lock().unwrap();
        for task in running_tasks.values() {
            task.lock().unwrap().handle.reset();
        }
    }

    pub fn remove(&self, url: &str) {
        let task_id = TaskId::from_url(url);
        self.cache_manager.remove(task_id);
    }

    pub fn contains(&self, url: &str) -> bool {
        let task_id = TaskId::from_url(url);
        self.cache_manager.contains(&task_id)
    }

    pub fn preload(
        &'static self,
        request: DownloadRequest,
        mut callback: Box<dyn PreloadCallback>,
        update: bool,
        downloader: Downloader,
    ) -> Option<TaskHandle> {
        let url = request.url;
        let task_id = TaskId::from_url(url);
        info!("preload task {}", task_id.brief());

        if !update {
            if let Err(ret) = self.fetch_with_callback(&task_id, callback) {
                callback = ret;
            } else {
                info!("{} fetch success", task_id.brief());
                let handle = TaskHandle::new(task_id);
                handle.set_completed();
                return Some(handle);
            }
        }

        loop {
            let updater = match self.running_tasks.lock().unwrap().entry(task_id.clone()) {
                Entry::Occupied(entry) => entry.get().clone(),
                Entry::Vacant(entry) => {
                    let download_task = DownloadTask::new(
                        task_id.clone(),
                        &self.cache_manager,
                        self.info_mgr.clone(),
                        request,
                        callback,
                        downloader,
                        0,
                    );
                    match download_task {
                        Some(task) => {
                            let updater = Arc::new(Mutex::new(task));
                            let handle = updater.lock().unwrap().task_handle();
                            entry.insert(updater);
                            return Some(handle);
                        }
                        None => return None,
                    }
                }
            };

            let mut updater = updater.lock().unwrap();
            match updater.try_add_callback(callback) {
                Ok(()) => return Some(updater.task_handle()),
                Err(mut cb) => {
                    if update {
                        info!("add callback failed, update task {}", task_id.brief());
                    } else if let Err(callback) = self.fetch_with_callback(&task_id, cb) {
                        error!("{} fetch fail after update", task_id.brief());
                        cb = callback;
                    } else {
                        info!("{} fetch success", task_id.brief());
                        let handle = TaskHandle::new(task_id);
                        handle.set_completed();
                        return Some(handle);
                    }

                    if !updater.remove_flag {
                        let seq = updater.seq + 1;
                        let download_task = DownloadTask::new(
                            task_id.clone(),
                            &self.cache_manager,
                            self.info_mgr.clone(),
                            request,
                            cb,
                            downloader,
                            seq,
                        );
                        match download_task {
                            Some(task) => {
                                *updater = task;
                                return Some(updater.task_handle());
                            }
                            None => return None,
                        }
                    } else {
                        callback = cb;
                    }
                }
            };
        }
    }

    pub fn fetch(&'static self, url: &str) -> Option<Arc<RamCache>> {
        let task_id = TaskId::from_url(url);
        self.cache_manager.fetch(&task_id)
    }

    pub(crate) fn task_finish(&self, task_id: &TaskId, seq: usize) {
        let Some(updater) = self.running_tasks.lock().unwrap().get(task_id).cloned() else {
            return;
        };
        let mut updater = updater.lock().unwrap();
        if updater.seq == seq {
            updater.remove_flag = true;
            self.running_tasks.lock().unwrap().remove(task_id);
        }
    }

    pub fn set_file_cache_size(&self, size: u64) {
        info!("set file cache size to {}", size);
        self.cache_manager.set_file_cache_size(size);
    }

    pub fn set_ram_cache_size(&self, size: u64) {
        info!("set ram cache size to {}", size);
        self.cache_manager.set_ram_cache_size(size);
    }

    pub fn set_info_list_size(&self, size: u16) {
        self.info_mgr.update_info_list_size(size);
    }

    pub fn get_download_info(&self, url: &str) -> Option<DownloadInfo> {
        let task_id = TaskId::from_url(url);
        self.info_mgr.get_download_info(task_id)
    }

    fn fetch_with_callback(
        &'static self,
        task_id: &TaskId,
        mut callback: Box<dyn PreloadCallback>,
    ) -> Result<(), Box<dyn PreloadCallback>> {
        let task_id = task_id.clone();
        if let Some(cache) = self.cache_manager.fetch(&task_id) {
            crate::spawn(move || callback.on_success(cache, task_id.brief()));
            Ok(())
        } else {
            Err(callback)
        }
    }
}

#[cfg(test)]
mod test {
    use std::collections::HashSet;
    use std::io::{BufReader, Lines};
    use std::net::TcpStream;
    use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
    use std::sync::{Arc, LazyLock};
    use std::thread;
    use std::time::Duration;

    use request_utils::test::log::init;
    use request_utils::test::server::test_server;

    use super::*;
    use crate::download::CANCEL;

    const ERROR_IP: &str = "127.12.31.12";
    const NO_DATA: usize = 1359;
    const TEST_URL: &str = "http://www.baidu.com";

    #[cfg(feature = "ohos")]
    const DOWNLOADER: Downloader = Downloader::Netstack;

    #[cfg(not(feature = "ohos"))]
    const DOWNLOADER: Downloader = Downloader::Ylong;

    struct TestCallbackN;
    impl PreloadCallback for TestCallbackN {}

    struct TestCallbackS {
        flag: Arc<AtomicUsize>,
    }

    impl PreloadCallback for TestCallbackS {
        fn on_success(&mut self, data: Arc<RamCache>, _task_id: &str) {
            if data.size() != 0 {
                self.flag.fetch_add(1, Ordering::SeqCst);
            } else {
                self.flag.store(NO_DATA, Ordering::SeqCst);
            }
        }
    }

    struct TestCallbackF {
        flag: Arc<Mutex<String>>,
    }

    impl PreloadCallback for TestCallbackF {
        fn on_fail(&mut self, error: CacheDownloadError, _task_id: &str) {
            *self.flag.lock().unwrap() = error.message().to_string();
        }
    }

    struct TestCallbackC {
        flag: Arc<AtomicUsize>,
    }

    impl PreloadCallback for TestCallbackC {
        fn on_cancel(&mut self) {
            self.flag.fetch_add(1, Ordering::SeqCst);
        }
    }

    #[test]
    fn ut_preload_success() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let success_flag = Arc::new(AtomicUsize::new(0));
        let callback = Box::new(TestCallbackS {
            flag: success_flag.clone(),
        });
        let handle = SERVICE.preload(DownloadRequest::new(TEST_URL), callback, true, DOWNLOADER);
        assert!(handle.is_some());
        let handle = handle.unwrap();
        while !handle.is_finish() {
            thread::sleep(Duration::from_millis(500));
        }
        assert_eq!(success_flag.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn ut_preload_success_add_callback() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let success_flag_0 = Arc::new(AtomicUsize::new(0));
        let callback_0 = Box::new(TestCallbackS {
            flag: success_flag_0.clone(),
        });

        let success_flag_1 = Arc::new(AtomicUsize::new(0));
        let callback_1 = Box::new(TestCallbackS {
            flag: success_flag_1.clone(),
        });

        let handle = SERVICE.preload(DownloadRequest::new(TEST_URL), callback_0, true, DOWNLOADER);
        SERVICE.preload(DownloadRequest::new(TEST_URL), callback_1, true, DOWNLOADER);
        assert!(handle.is_some());
        let handle = handle.unwrap();
        while !handle.is_finish() {
            thread::sleep(Duration::from_millis(500));
        }
        assert_eq!(success_flag_0.load(Ordering::SeqCst), 1);
        assert_eq!(success_flag_1.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn ut_preload_fail() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let error = Arc::new(Mutex::new(String::new()));
        let callback = Box::new(TestCallbackF {
            flag: error.clone(),
        });
        let handle = SERVICE.preload(DownloadRequest::new(ERROR_IP), callback, true, DOWNLOADER);
        assert!(handle.is_some());
        let handle = handle.unwrap();
        while !handle.is_finish() {
            thread::sleep(Duration::from_millis(500));
        }
        assert!(!error.lock().unwrap().as_str().is_empty());
    }

    #[test]
    fn ut_preload_fail_add_callback() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let error_0 = Arc::new(Mutex::new(String::new()));
        let callback_0 = Box::new(TestCallbackF {
            flag: error_0.clone(),
        });
        let error_1 = Arc::new(Mutex::new(String::new()));
        let callback_1 = Box::new(TestCallbackF {
            flag: error_1.clone(),
        });

        let handle = SERVICE.preload(DownloadRequest::new(ERROR_IP), callback_0, true, DOWNLOADER);
        assert!(handle.is_some());
        let handle = handle.unwrap();
        SERVICE.preload(DownloadRequest::new(ERROR_IP), callback_1, true, DOWNLOADER);
        while !handle.is_finish() {
            thread::sleep(Duration::from_millis(500));
        }

        assert!(!error_0.lock().unwrap().as_str().is_empty());
        assert!(!error_1.lock().unwrap().as_str().is_empty());
    }

    #[test]
    fn ut_preload_cancel_0() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let cancel_flag = Arc::new(AtomicUsize::new(0));
        let callback = Box::new(TestCallbackC {
            flag: cancel_flag.clone(),
        });
        let handle = SERVICE.preload(DownloadRequest::new(TEST_URL), callback, true, DOWNLOADER);
        assert!(handle.is_some());
        let mut handle = handle.unwrap();
        handle.cancel();
        while handle.state() != CANCEL {
            std::thread::sleep(Duration::from_millis(500));
        }

        assert_eq!(cancel_flag.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn ut_preload_cancel_1() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let cancel_flag = Arc::new(AtomicUsize::new(0));
        let callback = Box::new(TestCallbackC {
            flag: cancel_flag.clone(),
        });
        let handle = SERVICE.preload(DownloadRequest::new(TEST_URL), callback, true, DOWNLOADER);
        SERVICE.cancel(TEST_URL);
        assert!(handle.is_some());
        let handle = handle.unwrap();
        while handle.state() != CANCEL {
            std::thread::sleep(Duration::from_millis(500));
        }
        assert_eq!(cancel_flag.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn ut_preload_cancel_add_callback() {
        init();
        let test_url = "https://www.gitee.com";

        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let cancel_flag_0 = Arc::new(AtomicUsize::new(0));
        let callback_0 = Box::new(TestCallbackC {
            flag: cancel_flag_0.clone(),
        });
        let cancel_flag_1 = Arc::new(AtomicUsize::new(0));
        let callback_1 = Box::new(TestCallbackC {
            flag: cancel_flag_1.clone(),
        });

        let handle_0 =
            SERVICE.preload(DownloadRequest::new(test_url), callback_0, true, DOWNLOADER);
        let handle_1 =
            SERVICE.preload(DownloadRequest::new(test_url), callback_1, true, DOWNLOADER);
        assert!(handle_0.is_some());
        assert!(handle_1.is_some());
        let mut handle_0 = handle_0.unwrap();
        let mut handle_1 = handle_1.unwrap();
        handle_0.cancel();
        assert_eq!(cancel_flag_0.load(Ordering::SeqCst), 0);
        assert_eq!(cancel_flag_1.load(Ordering::SeqCst), 0);
        handle_1.cancel();
        assert!(handle_0.is_finish());
        assert!(handle_1.is_finish());

        while handle_1.state() != CANCEL {
            std::thread::sleep(Duration::from_millis(500));
        }
        assert_eq!(cancel_flag_0.load(Ordering::SeqCst), 1);
        assert_eq!(cancel_flag_1.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn ut_preload_already_success() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let handle = SERVICE.preload(
            DownloadRequest::new(TEST_URL),
            Box::new(TestCallbackN),
            true,
            DOWNLOADER,
        );
        assert!(handle.is_some());
        let handle = handle.unwrap();
        while !handle.is_finish() {
            thread::sleep(Duration::from_millis(500));
        }
        let success_flag = Arc::new(AtomicUsize::new(0));
        let callback = Box::new(TestCallbackS {
            flag: success_flag.clone(),
        });
        SERVICE.preload(DownloadRequest::new(TEST_URL), callback, false, DOWNLOADER);
        std::thread::sleep(Duration::from_millis(50));
        assert_eq!(success_flag.load(Ordering::SeqCst), 1);
    }

    #[test]
    fn ut_preload_local_headers() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);

        let headers = vec![
            ("User-Agent", "Mozilla/5.0"),
            ("Accept", "text/html"),
            ("Accept-Language", "en-US"),
            ("Accept-Encoding", "gzip, deflate"),
            ("Connection", "keep-alive"),
        ];
        let mut headers_clone: HashSet<String> = headers
            .iter()
            .map(|(k, v)| format!("{}:{}", k.to_ascii_lowercase(), v.to_ascii_lowercase()))
            .collect();

        let flag = Arc::new(AtomicBool::new(true));
        let flag_clone = flag.clone();
        let test_f = move |mut lines: Lines<BufReader<&mut TcpStream>>| {
            for line in lines.by_ref() {
                let line = line.unwrap();
                let line = line.to_ascii_lowercase();
                if line.is_empty() {
                    break;
                }
                headers_clone.remove(&line);
            }
            if headers_clone.is_empty() {
                flag_clone.store(true, Ordering::SeqCst);
            }
        };
        let server = test_server(test_f);
        let mut request = DownloadRequest::new(&server);
        request.headers(headers);
        let success_flag = Arc::new(AtomicUsize::new(0));
        let callback = Box::new(TestCallbackS {
            flag: success_flag.clone(),
        });
        let handle = SERVICE.preload(request, callback, true, DOWNLOADER);
        assert!(handle.is_some());
        let handle = handle.unwrap();
        while !handle.is_finish() {
            thread::sleep(Duration::from_millis(500));
        }
        assert!(flag.load(Ordering::SeqCst));
        assert_eq!(success_flag.load(Ordering::SeqCst), NO_DATA);
    }

    #[test]
    fn ut_preload_fetch() {
        init();
        static SERVICE: LazyLock<CacheDownloadService> = LazyLock::new(CacheDownloadService::new);
        let success_flag = Arc::new(AtomicUsize::new(0));
        let callback = Box::new(TestCallbackS {
            flag: success_flag.clone(),
        });
        let handle = SERVICE.preload(DownloadRequest::new(TEST_URL), callback, true, DOWNLOADER);
        assert!(handle.is_some());
        let handle = handle.unwrap();
        while !handle.is_finish() {
            thread::sleep(Duration::from_millis(500));
        }
        let cache = SERVICE.fetch(TEST_URL);
        assert!(cache.is_some());
        assert_eq!(success_flag.load(Ordering::SeqCst), 1);
    }
}
