/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef REQUEST_PATH_UTILS_H
#define REQUEST_PATH_UTILS_H

#include <sys/stat.h>

#include <cstdint>
#include <map>
#include <mutex>
#include <vector>

#include "request_common.h"

namespace OHOS::Request {
// Refcounted ACL grants that let the download_server SA (uid 3815) read
// and write app-sandbox files on behalf of tasks (see docs/agent/
// feature-client.md for the client/server split).
class PathUtils {
public:
    /// Grants, along the app-base ancestor chain of path, a traverse (x)
    /// ACL per directory and a per-action file ACL (download: rw,
    /// upload: r), refcounted per path and rolled back on partial failure.
    /// Returns false when nothing lies under an app base directory or any
    /// grant fails. Called when a task is created with sandbox files.
    static bool AddPathsToMap(const std::string &path, const Action action);
    /// Releases one reference per component previously granted by
    /// AddPathsToMap; revokes the ACL when the last reference goes.
    /// Called when a task is destroyed or deleted.
    static bool SubPathsToMap(const std::string &path);
    /// True when filepath starts with one of the app sandbox base
    /// directories (el1/el2/el5); only the prefix is checked.
    static bool CheckBelongAppBaseDir(const std::string &filepath);
    /// Masks each path segment for logging ("/ab/abcde" -> "/*b/**cde").
    static std::string ShieldPath(const std::string &path);
};
} // namespace OHOS::Request
#endif // REQUEST_PATH_UTILS_H