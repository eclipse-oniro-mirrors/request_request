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

//! Command handling module for the request service.
//! 
//! This module provides core command functionality for managing download and upload tasks,
//! including task creation, control operations, information retrieval, and subscription management.
//! Submodules implement specific command handlers for different operations.

use crate::error::ErrorCode;

/// Task creation and configuration
mod construct;
/// Task information dumping utilities
mod dump;
/// Task configuration retrieval
mod get_task;
/// Notification system integration
mod notification_bar;
/// Channel establishment for data transfer
mod open_channel;
/// Task pause operations
mod pause;
/// Task state and information queries
mod query;
/// MIME type detection for resources
mod query_mime_type;
/// Task deletion operations
mod remove;
/// Task resumption operations
mod resume;
/// Task searching functionality
mod search;
/// Bandwidth control for tasks
mod set_max_speed;
/// Task execution mode configuration
mod set_mode;
/// Task visibility management
mod show;
/// Task start operations
mod start;
/// Task termination operations
mod stop;
/// Running count subscription
mod sub_runcount;
/// Task event subscription
mod subscribe;
/// Task metadata updates
mod touch;
/// Running count unsubscription
mod unsub_runcount;
/// Task event unsubscription
mod unsubscribe;

/// Maximum number of concurrent control operations allowed.
pub(crate) const CONTROL_MAX: usize = 500;

/// Maximum number of concurrent information retrieval operations allowed.
pub(crate) const GET_INFO_MAX: usize = 200;

/// Maximum number of concurrent task construction operations allowed.
pub(crate) const CONSTRUCT_MAX: usize = 100;

/// Sets an error code at a specific index in a vector.
///
/// Updates the error code at the specified index if the index is valid.
/// Logs an error message if the index is out of bounds.
///
/// # Arguments
///
/// * `vec` - Vector of error codes to modify.
/// * `index` - Position in the vector to update.
/// * `code` - Error code to set at the specified index.
///
/// # Notes
///
/// This function silently handles out-of-bounds indices by logging an error
/// instead of panicking.
pub(crate) fn set_code_with_index(vec: &mut [ErrorCode], index: usize, code: ErrorCode) {
    if let Some(c) = vec.get_mut(index) {
        *c = code;
    } else {
        // Log error instead of panicking to prevent service crashes
        error!("out index: {}", index);
    }
}

/// Sets an error code in a vector of error code and value tuples.
///
/// Updates the error code portion of the tuple at the specified index if the
/// index is valid. Logs an error message if the index is out of bounds.
///
/// # Arguments
///
/// * `vec` - Vector of (error code, value) tuples to modify.
/// * `index` - Position in the vector to update.
/// * `code` - Error code to set at the specified index.
///
/// # Notes
///
/// This function silently handles out-of-bounds indices by logging an error
/// instead of panicking. The generic type `T` allows this function to work with
/// any value type paired with error codes.
pub(crate) fn set_code_with_index_other<T>(
    vec: &mut [(ErrorCode, T)],
    index: usize,
    code: ErrorCode,
) {
    if let Some((c, _t)) = vec.get_mut(index) {
        *c = code;
    } else {
        // Log error instead of panicking to prevent service crashes
        error!("out index: {}", index);
    }
}
