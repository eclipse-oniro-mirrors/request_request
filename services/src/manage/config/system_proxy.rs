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

//! System proxy configuration management.
//!
//! This module provides functionality to manage and access system proxy settings
//! for network requests. It interacts with the underlying system to retrieve proxy
//! configuration parameters including host, port, and exclusion lists.

use crate::utils::c_wrapper::CStringWrapper;

/// Snapshot of the system proxy read from the C++ side under one lock.
///
/// All three fields describe the same proxy update, so callers converting
/// them to strings see a consistent triple.
#[repr(C)]
pub(crate) struct HttpProxySnapshot {
    pub(crate) host: CStringWrapper,
    pub(crate) port: CStringWrapper,
    pub(crate) exclusion_list: CStringWrapper,
}

/// Manages system proxy settings for network requests.
///
/// Provides access to the system's proxy configuration and subscribes to proxy
/// setting changes. Acts as a wrapper around system-level proxy functionality.
#[derive(Clone)]
pub(crate) struct SystemProxyManager;

impl SystemProxyManager {
    /// Initializes the system proxy manager and registers for proxy setting updates.
    ///
    /// # Returns
    ///
    /// A new instance of `SystemProxyManager` with registered proxy subscription.
    ///
    /// # Safety
    ///
    /// Calls unsafe FFI function `RegisterProxySubscriber()` to subscribe to system
    /// proxy changes.
    pub(crate) fn init() -> Self {
        unsafe {
            RegisterProxySubscriber();
        }
        Self
    }

    /// Takes a consistent snapshot of the current proxy configuration.
    ///
    /// The C++ side reads host, port, and exclusion list under one lock, so
    /// the returned snapshot always describes a single proxy update.
    ///
    /// # Returns
    ///
    /// An `HttpProxySnapshot` with the current proxy settings.
    ///
    /// # Safety
    ///
    /// Calls unsafe FFI function `GetHttpProxySnapshot()` to retrieve the
    /// proxy snapshot from the system.
    pub(crate) fn snapshot(&self) -> HttpProxySnapshot {
        unsafe { GetHttpProxySnapshot() }
    }
}

// C API functions for accessing system proxy settings
#[cfg(feature = "oh")]
extern "C" {
    /// Registers a subscriber for system proxy configuration changes.
    ///
    /// # Safety
    ///
    /// This is an FFI function that interacts with system components.
    pub(crate) fn RegisterProxySubscriber();

    /// Retrieves a consistent snapshot of the current proxy configuration.
    ///
    /// # Returns
    ///
    /// An `HttpProxySnapshot` containing the proxy host address, port number,
    /// and exclusion list.
    ///
    /// # Safety
    ///
    /// This is an FFI function that interacts with system components.
    pub(crate) fn GetHttpProxySnapshot() -> HttpProxySnapshot;
}
