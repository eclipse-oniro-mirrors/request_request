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

#![warn(unused)]

//! File-based cache implementation for task data.
//!
//! This module provides functionality for managing file-based caches,
//! including:
//! - Directory management for cache storage (current and history directories
//!   are held in `OnceLock`s initialized by `init_curr_store_dir` /
//!   `init_history_store_dir`)
//! - File cache creation, restoration, and deletion
//! - Synchronization between RAM and disk storage
//! - Directory observation for cache maintenance
//!
//! The implementation ensures thread-safe access to cache resources and
//! provides mechanisms for persisting data across application restarts.

use std::fs::{self, DirEntry, File, OpenOptions};
use std::io::{self, Seek, Write};
use std::os::unix::fs::MetadataExt;
use std::path::{Path, PathBuf};
use std::str::FromStr;
use std::sync::{Arc, Mutex, OnceLock};
use std::time::SystemTime;

use request_utils::task_id::TaskId;

use super::ram::RamCache;
use crate::manage::CacheManager;

/// Suffix appended to files that are fully written and finalized.
///
/// This suffix is used to indicate that a cache file has been completely
/// written and is ready for use. Files without this suffix may be incomplete
/// and are considered invalid.
const FINISH_SUFFIX: &str = "_F";

/// Current cache directory, initialized exactly once by `init_curr_store_dir`.
///
/// Reads through `OnceLock::get` establish the necessary happens-before
/// relation with the initializing write, so no unsynchronized access is
/// possible.
static CURR_STORE_DIR: OnceLock<PathBuf> = OnceLock::new();

/// History directory spawner, initialized exactly once by
/// `init_history_store_dir`.
///
/// Reads through `OnceLock::get` establish the necessary happens-before
/// relation with the initializing write, so no unsynchronized access is
/// possible.
static HISTORY_STORE_DIR: OnceLock<DirObservSpawner> = OnceLock::new();

/// Initializes the history directory for cache storage.
///
/// Sets up the history directory with the provided `HistoryDir` instance and
/// starts directory observation using the given spawner function.
///
/// # Arguments
/// * `history` - The history directory to use for cache storage
/// * `spawner` - Function to spawn directory observation process
///
/// # Safety
/// This function is thread-safe and will only initialize the history directory
/// once.
pub fn init_history_store_dir(history: Arc<HistoryDir>, spawner: fn(PathBuf, Arc<HistoryDir>)) {
    {
        // Get current directory and start observation
        let curr_dir = get_curr_store_dir();
        let mut is_observe = history.is_observe.lock().unwrap();
        spawner(curr_dir, history.clone());
        *is_observe = true;
    }
    // `set()` keeps the explicit init-time semantics of the old `call_once`:
    // the side effects above still run exactly once, and a racing second
    // caller's value is dropped — both callers derive the same state.
    let _ = HISTORY_STORE_DIR.set(DirObservSpawner::new(history, spawner));
}

/// Initializes the current directory for cache storage.
///
/// Sets up the current directory where cache files will be stored.
///
/// # Safety
/// This function is thread-safe and will only initialize the current directory
/// once.
pub fn init_curr_store_dir() {
    let curr_dir = get_curr_store_dir();
    // `set()` keeps the explicit init-time semantics of the old `call_once`
    // (the `create_dir_all` side effect happens at init, not at first read);
    // the second caller's value is dropped — both callers derive the same
    // path.
    let _ = CURR_STORE_DIR.set(curr_dir);
}

/// Gets the path to the current cache directory.
///
/// Returns the path to the directory where cache files are stored. On
/// OpenHarmony systems, it uses the application's cache directory, falling back
/// to a default path if that fails. On other systems, it uses the current
/// directory.
///
/// # Returns
/// Path to the cache directory
///
/// # Notes
/// This function creates the directory if it doesn't exist.
pub fn get_curr_store_dir() -> PathBuf {
    #[cfg(feature = "ohos")]
    let mut path = match request_application::application::wrapper::get_cache_dir() {
        Some(dir) => PathBuf::from_str(&dir).unwrap(),
        None => {
            error!("get cache dir failed");
            // Fallback to standard cache directory if context retrieval fails
            PathBuf::from_str("/data/storage/el2/base/cache").unwrap()
        }
    };
    #[cfg(not(feature = "ohos"))]
    let mut path = PathBuf::from_str("./").unwrap();

    path.push("preload_caches");
    // Ensure the directory exists
    if let Err(e) = fs::create_dir_all(path.as_path()) {
        error!("create cache dir error {}", e);
    }
    path
}

/// Checks if the history directory has been initialized.
///
/// # Returns
/// `true` if the history directory has been initialized, `false` otherwise
pub fn is_history_init() -> bool {
    HISTORY_STORE_DIR.get().is_some()
}

/// Returns the current store directory after ensuring it exists.
///
/// Mirrors the semantics of the former `FileStoreDir::exist()`: ensures the
/// history directory exists (spawning observation if it was just created) and
/// creates the current directory if needed.
///
/// # Returns
/// The current directory path if it is initialized and usable, None otherwise
fn curr_dir_checked() -> Option<&'static Path> {
    let curr = CURR_STORE_DIR.get()?;
    // Ensure history directory exists
    if let Some(history) = HISTORY_STORE_DIR.get() {
        if !history.exist() && history.create() {
            history.spawn_observe(curr.clone());
        }
    }
    // Ensure current directory exists
    if !curr.is_dir() {
        if let Err(e) = fs::create_dir_all(curr.as_path()) {
            error!("try create current cache dir error {}", e);
            return None;
        }
    }
    Some(curr.as_path())
}

/// Joins a path to the current store directory.
///
/// Ensures the directory exists before joining.
///
/// # Arguments
/// * `path` - Path to join with the current directory
///
/// # Returns
/// Joined path if the directory exists, None otherwise
fn join_curr_dir(path: String) -> Option<PathBuf> {
    Some(curr_dir_checked()?.join(path))
}

/// Gets the current store directory as a path.
///
/// Ensures the directory exists before returning.
///
/// # Returns
/// The current directory path if it exists, None otherwise
fn curr_dir_as_path() -> Option<&'static Path> {
    curr_dir_checked()
}

/// Manages directory observation for cache maintenance.
///
/// Combines a history directory with a function to spawn directory observation,
/// allowing for automatic monitoring of cache directories.
pub(crate) struct DirObservSpawner {
    /// History directory to observe
    history: Arc<HistoryDir>,
    /// Function to spawn directory observation
    spawner: fn(PathBuf, Arc<HistoryDir>),
}

impl DirObservSpawner {
    /// Creates a new DirObservSpawner.
    ///
    /// # Arguments
    /// * `history` - History directory to observe
    /// * `spawner` - Function to spawn directory observation process
    pub(crate) fn new(history: Arc<HistoryDir>, spawner: fn(PathBuf, Arc<HistoryDir>)) -> Self {
        Self { history, spawner }
    }

    /// Checks if the history directory exists.
    ///
    /// # Returns
    /// `true` if the history directory exists, `false` otherwise
    pub(crate) fn exist(&self) -> bool {
        self.history.exist()
    }

    /// Creates the history directory if it doesn't exist.
    ///
    /// # Returns
    /// `true` if the directory was created successfully, `false` otherwise
    pub fn create(&self) -> bool {
        self.history.create()
    }

    /// Spawns directory observation if not already observing.
    ///
    /// Starts the directory observation process for the history directory.
    ///
    /// # Arguments
    /// * `curr` - Current directory path to pass to the spawner
    pub fn spawn_observe(&self, curr: PathBuf) {
        let mut is_observe = self.history.is_observe.lock().unwrap();
        if !*is_observe {
            // Only spawn observation if not already observing
            (self.spawner)(curr, self.history.clone());
            *is_observe = true;
        }
    }
}

/// Represents a history directory for cache storage.
///
/// This struct manages a directory used for storing historical cache data,
/// with a flag to track whether the directory is being observed.
pub struct HistoryDir {
    /// Path to the history directory
    dir: PathBuf,
    /// Mutex-protected flag indicating if the directory is being observed
    pub is_observe: Mutex<bool>,
}

impl HistoryDir {
    /// Creates a new HistoryDir with the specified path.
    ///
    /// # Arguments
    /// * `dir` - Path to the history directory
    pub fn new(dir: PathBuf) -> Self {
        Self {
            dir,
            is_observe: Mutex::new(false),
        }
    }

    /// Checks if the history directory exists.
    ///
    /// # Returns
    /// `true` if the directory exists, `false` otherwise
    pub fn exist(&self) -> bool {
        self.dir.is_dir()
    }

    /// Creates the history directory if it doesn't exist.
    ///
    /// # Returns
    /// `true` if the directory was created successfully, `false` otherwise
    pub fn create(&self) -> bool {
        if let Err(e) = fs::create_dir_all(self.dir.as_path()) {
            error!("try create history dir error {}", e);
            false
        } else {
            true
        }
    }

    /// Stops directory observation.
    ///
    /// Sets the observation flag to false, indicating that the directory
    /// is no longer being monitored.
    pub fn stop_observe(&self) {
        let mut is_observe = self.is_observe.lock().unwrap();
        *is_observe = false;
    }

    /// Gets the string representation of the directory path.
    ///
    /// # Returns
    /// String representation of the path if valid UTF-8, None otherwise
    pub fn dir_path(&self) -> Option<&str> {
        self.dir.to_str()
    }
}

/// Represents a file-based cache for a specific task.
///
/// This struct manages a cache file associated with a task ID, handling
/// creation, access, and cleanup of the cache file.
pub(crate) struct FileCache {
    /// Size of the cached file content in bytes.
    size: u64,
    /// ID of the task associated with this cache
    task_id: TaskId,
}

impl FileCache {
    /// Creates a new `FileCache` for the given task and size.
    pub(crate) fn new(task_id: TaskId, size: u64) -> Self {
        Self { size, task_id }
    }

    /// Returns the cached file size in bytes.
    pub(crate) fn size(&self) -> u64 {
        self.size
    }

    /// Returns the task ID associated with this cache.
    pub(crate) fn task_id(&self) -> &TaskId {
        &self.task_id
    }

    /// Removes the cache file for the given task from disk.
    ///
    /// A "no such file or directory" error is logged at debug level (expected
    /// when the file is already gone); other errors are logged at error level.
    pub(crate) fn remove_file(task_id: &TaskId) {
        if let Some(path) = Self::path(task_id) {
            if let Err(e) = fs::remove_file(path) {
                // Different logging levels based on error type
                if let Some(2) = e.raw_os_error() {
                    // Error 2 is typically "No such file or directory" - not a critical error
                    debug!("{} drop file error: {}", task_id.brief(), e);
                } else {
                    error!("{} drop file error: {}", task_id.brief(), e);
                }
            }
        }
    }

    /// Creates a cache file and writes the contents of the RAM cache to it.
    ///
    /// Opens the final cache file path (which already carries the finish
    /// suffix) and writes directly into it, truncating any previous content.
    /// The write is not atomic: a crash or power loss mid-write leaves a
    /// finish-suffixed file that index rebuilding may mistake for a complete
    /// cache entry.
    ///
    /// # Arguments
    /// * `task_id` - ID of the task to create the file for
    /// * `cache` - RAM cache to write to disk
    ///
    /// # Returns
    /// `Ok(())` if successful, `Err(io::Error)` if any file operation fails
    pub(crate) fn create_file(task_id: &TaskId, cache: Arc<RamCache>) -> Result<(), io::Error> {
        if let Some(path) = Self::path(task_id) {
            // Create the file and write cache contents
            let mut file = OpenOptions::new()
                .write(true)
                .create(true)
                .truncate(true)
                .open(path.as_path())?;
            io::copy(&mut cache.cursor(), &mut file)?;
            file.flush()?;
            file.rewind()?;
            return Ok(());
        }
        Err(io::Error::new(
            io::ErrorKind::NotFound,
            "cache store dir not created.",
        ))
    }

    /// Opens the cache file for the given task for reading.
    ///
    /// # Arguments
    /// * `task_id` - ID of the task whose cache file should be opened.
    ///
    /// # Errors
    /// Returns `io::Error` if the cache directory is not initialized or the
    /// file cannot be opened.
    pub(crate) fn open(task_id: &TaskId) -> Result<File, io::Error> {
        if let Some(path) = Self::path(task_id) {
            OpenOptions::new().read(true).open(path)
        } else {
            Err(io::Error::new(io::ErrorKind::NotFound, "file not found."))
        }
    }

    /// Reads a task's cache file into a RAM cache and registers it with the
    /// cache manager.
    ///
    /// # Arguments
    /// * `task_id` - ID of the task whose cache file should be read.
    /// * `handle` - Cache manager used to allocate the RAM cache.
    ///
    /// # Errors
    /// Returns `io::Error` if the file cannot be opened or read.
    pub(crate) fn read(task_id: &TaskId, handle: &'static CacheManager) -> io::Result<RamCache> {
        let mut file = Self::open(task_id).map_err(|e| {
            error!("{:?} open file failed {:?}", task_id.brief(), e);
            e
        })?;
        let size = file.metadata()?.size();
        FileCache::copy_file_to_cache(task_id, handle, &mut file, Some(size as usize))
    }

    /// Reads a task's cache file into a RAM cache without registering it with
    /// the cache manager.
    ///
    /// # Arguments
    /// * `task_id` - ID of the task whose cache file should be read.
    /// * `handle` - Cache manager used to allocate the RAM cache.
    ///
    /// # Errors
    /// Returns `io::Error` if the file cannot be opened or read.
    pub(crate) fn read_but_not_cache(
        task_id: &TaskId,
        handle: &'static CacheManager,
    ) -> io::Result<RamCache> {
        let mut file = Self::open(task_id).map_err(|e| {
            error!("{:?} file open failed {:?}", task_id.brief(), e);
            e
        })?;
        FileCache::copy_file_to_cache(task_id, handle, &mut file, None)
    }

    /// Copies the contents of a file into a new RAM cache.
    ///
    /// # Arguments
    /// * `task_id` - ID of the task the cache belongs to.
    /// * `handle` - Cache manager used to allocate the RAM cache.
    /// * `file` - File to read from.
    /// * `size` - Expected content size, or `None` if unknown.
    ///
    /// # Errors
    /// Returns `io::Error` if copying from the file fails.
    fn copy_file_to_cache(
        task_id: &TaskId,
        handle: &'static CacheManager,
        file: &mut File,
        size: Option<usize>,
    ) -> io::Result<RamCache> {
        let mut cache = RamCache::new(task_id.clone(), handle, size);
        io::copy(file, &mut cache).map_err(|e| {
            error!("{:?} copy file failed {:?}", task_id.brief(), e);
            e
        })?;
        Ok(cache)
    }

    /// Gets the path to the cache file for the given task ID.
    ///
    /// # Arguments
    /// * `task_id` - ID of the task to get the path for
    ///
    /// # Returns
    /// Path to the cache file if the directory exists, None otherwise
    pub(crate) fn path(task_id: &TaskId) -> Option<PathBuf> {
        join_curr_dir(task_id.to_string() + FINISH_SUFFIX)
    }
}

/// Metadata for a persisted cache file.
pub(crate) struct FileCacheInfo {
    /// Task ID associated with the cache file.
    task_id: TaskId,
    /// Modification time of the cache file.
    time: SystemTime,
    /// Size of the cache file in bytes.
    size: u64,
}

impl FileCacheInfo {
    /// Creates a new `FileCacheInfo` from the given task ID, time, and size.
    pub(crate) fn new(task_id: TaskId, time: SystemTime, size: u64) -> Self {
        Self {
            task_id,
            time,
            size,
        }
    }
    /// Returns the task ID associated with this cache file.
    pub(crate) fn task_id(&self) -> &TaskId {
        &self.task_id
    }
    /// Returns the modification time of this cache file.
    pub(crate) fn time(&self) -> SystemTime {
        self.time
    }
    /// Returns the size of this cache file in bytes.
    pub(crate) fn size(&self) -> u64 {
        self.size
    }
}

/// Returns an iterator over the metadata of all valid cache files in the
/// current store directory.
///
/// Returns `None` if the cache directory has not been initialized.
pub(crate) fn get_cached_files_info() -> Option<impl Iterator<Item = FileCacheInfo>> {
    curr_dir_as_path().map(get_info_from_path)
}

/// Scans a directory and returns metadata for each valid cache file, sorted
/// by modification time.
///
/// # Arguments
/// * `path` - Directory to scan for cache files.
pub(crate) fn get_info_from_path(path: &Path) -> impl Iterator<Item = FileCacheInfo> {
    // Read the directory contents
    let files = match fs::read_dir(path) {
        Ok(files) => files,
        Err(e) => {
            error!("restore read dir error {}", e);
            // Return empty iterator if directory can't be read
            return vec![].into_iter();
        }
    };
    // Process and filter the directory entries
    let mut v = files
        .into_iter()
        .filter_map(|entry| match get_entry_file_info(entry) {
            Ok(info) => Some(info),
            Err(e) => {
                error!("restore file error {}", e);
                None
            }
        })
        .collect::<Vec<_>>();

    v.sort_by_key(|info| info.time());
    v.into_iter()
}

/// Extracts cache file metadata from a directory entry.
///
/// Incomplete files (missing the finish suffix) are removed from disk.
///
/// # Arguments
/// * `entry` - Directory entry to inspect.
///
/// # Errors
/// Returns `io::Error` if the entry cannot be read, the file name is invalid
/// UTF-8, the file is incomplete, or metadata cannot be retrieved.
pub(crate) fn get_entry_file_info(entry: io::Result<DirEntry>) -> Result<FileCacheInfo, io::Error> {
    let entry = entry?;
    // Get the file name and validate it
    let file_name = entry.file_name();
    let file_name = file_name.to_str().ok_or(io::Error::new(
        io::ErrorKind::InvalidData,
        format!("invalid file name {:?}", file_name),
    ))?;

    // Check for the finish suffix to ensure the file is complete
    if !file_name.ends_with(FINISH_SUFFIX) {
        // Remove incomplete files
        let _ = fs::remove_file(entry.path());
        return Err(io::Error::new(
            io::ErrorKind::InvalidData,
            format!("incomplete file {}", file_name),
        ));
    }

    // Extract the task ID from the file name
    let task_id = TaskId::new(file_name.trim_end_matches(FINISH_SUFFIX).to_string());
    // Get the modification time
    let time = entry.metadata()?.modified()?;
    let size = entry.metadata()?.len();
    Ok(FileCacheInfo::new(task_id, time, size))
}

#[cfg(test)]
mod ut_file {
    // Include unit tests
    include!("../../tests/ut/data/ut_file.rs");
}
