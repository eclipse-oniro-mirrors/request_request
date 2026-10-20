/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef REQUEST_INOTIFY_EVENT_LISTENT_H
#define REQUEST_INOTIFY_EVENT_LISTENT_H

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace fs = std::filesystem;

namespace rust {
inline namespace cxxbridge1 {
template<typename T> class Box;
} // namespace cxxbridge1
} // namespace rust

namespace OHOS::Request {

// fdsan ownership tag stamped on the inotify and epoll file descriptors
// (fdsan_exchange_owner_tag on setup, fdsan_close_with_tag on teardown), so
// that descriptors closed by a foreign owner are attributed to this module
// instead of aborting with an anonymous ownership mismatch.
static constexpr uint64_t REQUEST_FDSAN_TAG = 0xC01C50;

struct DirRebuilder;

/**
 * @brief Watches a directory with inotify and rebuilds the cache when it is
 * removed.
 *
 * An epoll loop waits for IN_DELETE_SELF / IN_MOVE_SELF on the watched
 * directory; on such an event the Rust-side DirRebuilder callback is invoked
 * so the cache store directory and index can be recreated. Exposed to Rust
 * through the cxx bridge (native_ffi.h); non-copyable and non-movable because
 * it owns raw file descriptors and the callback's raw pointer.
 */
class DirectoryMonitor {
public:
    // Takes ownership of the Rust callback Box (reclaimed in the destructor)
    // and remembers directory as the watch target.
    DirectoryMonitor(const std::string &directory, rust::Box<DirRebuilder> callback);
    DirectoryMonitor(DirectoryMonitor &&other) = delete;
    DirectoryMonitor &operator=(DirectoryMonitor &&other) = delete;

    DirectoryMonitor(const DirectoryMonitor &) = delete;
    DirectoryMonitor &operator=(const DirectoryMonitor &) = delete;
    // Stops the watch loop (if running) and releases the callback Box and the
    // inotify/epoll resources.
    ~DirectoryMonitor();
    // Sets up inotify and epoll, then enters the watch loop; blocks until the
    // watched directory is deleted/moved or Stop() is called.
    void Start();
    // Asks the watch loop to exit; does not block.
    void Stop();

private:
    // epoll wait loop; returns once running_ becomes false.
    int Run();
    // Creates the inotify instance and adds the watch; returns the watch
    // descriptor or -1.
    int SetupInotify();
    // Creates the epoll instance and registers the inotify fd.
    int SetupEpoll();
    // Registers fd with epoll for the given events.
    int AddToEpoll(int fd, uint32_t events);
    // Reads pending inotify events and dispatches the callback on directory
    // removal.
    void HandleInotify();
    // Removes the watch and closes the fdsan-tagged descriptors.
    void Cleanup();

    // Directory being watched.
    fs::path directory_;
    // Rust-side rebuild callback; raw pointer into the Box taken over in the
    // constructor and reclaimed in the destructor (into_raw/from_raw).
    DirRebuilder *callback_;

    // inotify file descriptor; -1 when not set up.
    int inotify_fd_ = -1;
    // epoll file descriptor; -1 when not set up.
    int epoll_fd_ = -1;
    // Watch descriptor from inotify_add_watch; -1 when not watching.
    int watch_descriptor_ = -1;
    // Cleared by Stop() to end the watch loop.
    std::atomic<bool> running_{ false };
};
} // namespace OHOS::Request

#endif