// Copyright (C) 2025 Huawei Device Co., Ltd.
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

//! Cache download functionality for animation resources.
//! 
//! This module provides functions for downloading, canceling, and configuring cache settings
//! for animation resources. It serves as a bridge between the ETS interface and the native
//! cache download service.

use ani_rs::business_error::BusinessError;
use preload_native::services::{CacheDownloadService, DownloadRequest};
use preload_native::traits::PreloadCallback;
use preload_native::cache_download;

use crate::bridge::CacheDownloadOptions;

/// Empty callback implementation for preload operations.
///
/// Provides a no-op implementation of the `PreloadCallback` trait for use in download requests.
struct Callback;

impl PreloadCallback for Callback {}

/// Initiates a download of a resource with the specified URL and options.
///
/// Creates a new download request, configures it with any provided headers, and submits
/// it to the cache download service for preloading.
///
/// # Parameters
///
/// * `url` - The URL of the resource to download
/// * `options` - Configuration options for the download, including optional headers
///
/// # Returns
///
/// * `Ok(())` if the download was successfully initiated
/// * `Err(BusinessError)` if there was an error initiating the download
///
/// # Examples
///
/// ```rust
/// use ani_cache_download::cache_download::{download, CacheDownloadOptions};
/// use ani_rs::business_error::BusinessError;
/// 
/// // Basic download
/// let result: Result<(), BusinessError> = download(
///     "https://example.com/resource.mp4".to_string(),
///     CacheDownloadOptions { header: None }
/// );
/// 
/// // Download with headers
/// let mut headers = std::collections::HashMap::new();
/// headers.insert("Authorization".to_string(), "Bearer token123".to_string());
/// let result: Result<(), BusinessError> = download(
///     "https://example.com/resource.mp4".to_string(),
///     CacheDownloadOptions { header: Some(headers) }
/// );
/// ```
#[ani_rs::native]
fn download(url: String, options: CacheDownloadOptions) -> Result<(), BusinessError> {
    let mut request = DownloadRequest::new(&url);
    // Create a boxed callback to handle download events
    let callback = Box::new(Callback);
    // Apply headers if provided in options
    if let Some(headers) = options.header {
        request.headers(headers);
    }
    // Initiate preloading with Netstack downloader and auto-refresh enabled
    CacheDownloadService::get_instance().preload(
        request,
        callback,
        true,  // Enable auto-refresh of cached resources
        cache_download::Downloader::Netstack,
    );
    Ok(())
}

/// Cancels a previously initiated download by URL.
///
/// Sends a cancel request to the cache download service for the specified URL.
///
/// # Parameters
///
/// * `url` - The URL of the resource download to cancel
///
/// # Returns
///
/// * `Ok(())` if the cancel request was successfully submitted
/// * `Err(BusinessError)` if there was an error submitting the cancel request
///
/// # Examples
///
/// ```rust
/// use ani_cache_download::cache_download::cancel;
/// use ani_rs::business_error::BusinessError;
/// 
/// // Cancel a download
/// let result: Result<(), BusinessError> = cancel("https://example.com/resource.mp4".to_string());
/// ```
#[ani_rs::native]
fn cancel(url: String) -> Result<(), BusinessError> {
    CacheDownloadService::get_instance().cancel(&url);
    Ok(())
}

/// Sets the maximum memory (RAM) cache size in bytes.
///
/// Configures the RAM cache size for the cache download service.
///
/// # Parameters
///
/// * `size` - The maximum size of the memory cache in bytes
///
/// # Returns
///
/// * `Ok(())` if the cache size was successfully updated
/// * `Err(BusinessError)` if there was an error updating the cache size
///
/// # Examples
///
/// ```rust
/// use ani_cache_download::cache_download::set_memory_cache_size;
/// use ani_rs::business_error::BusinessError;
/// 
/// // Set memory cache size to 50MB
/// let result: Result<(), BusinessError> = set_memory_cache_size(50 * 1024 * 1024);
/// ```
#[ani_rs::native]
fn set_memory_cache_size(size: i64) -> Result<(), BusinessError> {
    // Convert signed i64 to unsigned u64 for cache size
    CacheDownloadService::get_instance().set_ram_cache_size(size as u64);
    Ok(())
}

/// Sets the maximum file cache size in bytes.
///
/// Configures the file system cache size for the cache download service.
///
/// # Parameters
///
/// * `size` - The maximum size of the file cache in bytes
///
/// # Returns
///
/// * `Ok(())` if the cache size was successfully updated
/// * `Err(BusinessError)` if there was an error updating the cache size
///
/// # Examples
///
/// ```rust
/// use ani_cache_download::cache_download::set_file_cache_size;
/// use ani_rs::business_error::BusinessError;
/// 
/// // Set file cache size to 500MB
/// let result: Result<(), BusinessError> = set_file_cache_size(500 * 1024 * 1024);
/// ```
#[ani_rs::native]
fn set_file_cache_size(size: i64) -> Result<(), BusinessError> {
    // Convert signed i64 to unsigned u64 for cache size
    CacheDownloadService::get_instance().set_file_cache_size(size as u64);
    Ok(())
}
