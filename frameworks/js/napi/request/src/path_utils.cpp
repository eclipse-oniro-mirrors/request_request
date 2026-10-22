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

#include "path_utils.h"

#include <sstream>
#include <string>
#include <tuple>
#include <utility>

#include "log.h"
#include "storage_acl.h"

namespace OHOS::Request {

static constexpr int ACL_SUCC = 0;

// SA side reading and writing are aware of the `Other` permission of `UGO`;
// otherwise, it will cause concurrency with the Set ACL and generate `Permission denied`.
// ACL entries granting the download_server SA (uid 3815, EXPECT_UID in
// runcount_notify_stub.cpp) access to app files: rw for downloaded files,
// r for uploaded files, x to traverse directories, --- to revoke.
static const std::string SA_PERMISSION_U_RW = "u:3815:rw";
static const std::string SA_PERMISSION_U_R = "u:3815:r";
static const std::string SA_PERMISSION_U_X = "u:3815:x";
static const std::string SA_PERMISSION_U_CLEAN = "u:3815:---";
// App sandbox base directories (el1/el2 device-level, el5 user-level).
static const std::string AREA1 = "/data/storage/el1/base";
static const std::string AREA2 = "/data/storage/el2/base";
static const std::string AREA5 = "/data/storage/el5/base";

// Refcount table behind AddPathsToMap/SubPathsToMap: path -> (isFile,
// reference count); the ACL is revoked when the count drops to zero.
// Guarded by pathMutex_.
static std::mutex pathMutex_;
static std::map<std::string, std::tuple<bool, uint32_t>> pathMap_;

bool PathUtils::CheckBelongAppBaseDir(const std::string &filepath)
{
    return (filepath.find(AREA1) == 0) || filepath.find(AREA2) == 0 || filepath.find(AREA5) == 0;
}

// "/A/B/C" -> ["/A", "/A/B", "/A/B/C"]
std::vector<std::string> SplitPath(const std::string &path)
{
    std::vector<std::string> result;
    if (path.empty() || path[0] != '/') {
        return result;
    }

    result.reserve(std::count(path.begin(), path.end(), '/') + 1);
    std::string currentPath = "";
    size_t pos = 1;
    while (pos < path.size()) {
        size_t nextPos = path.find('/', pos);
        if (nextPos == std::string::npos) {
            nextPos = path.size();
        }
        if (nextPos > pos) {
            currentPath += ("/" + path.substr(pos, nextPos - pos));
            result.emplace_back(currentPath);
        }
        pos = nextPos + 1;
    }
    return result;
}

// ["/A", "/A/B", "/A/B/C"] -> [("/A", false), ("/A/B", false), ("/A/B/C", true)]
std::vector<std::pair<std::string, bool>> SelectPath(const std::vector<std::string> &paths)
{
    std::vector<std::pair<std::string, bool>> result;
    if (paths.empty())
        return result;

    for (const auto &elem : paths) {
        if (!PathUtils::CheckBelongAppBaseDir(elem)) {
            continue;
        }
        result.emplace_back(elem, false);
    }

    if (!result.empty()) {
        result.back().second = true;
    }
    return result;
}

// Picks the ACL entry by kind: for files it depends on the action (upload
// only needs the SA to read, download needs read-write), ancestors get
// execute-only to traverse.
bool AddAcl(const std::string &path, const bool isFile, const Action action)
{
    std::string entry;
    if (isFile) {
        if (action == Action::UPLOAD) {
            entry = SA_PERMISSION_U_R;
        } else {
            entry = SA_PERMISSION_U_RW;
        }
    } else {
        entry = SA_PERMISSION_U_X;
    }
    if (StorageDaemon::AclSetAccess(path, entry) != ACL_SUCC) {
        REQUEST_HILOGE("Add Acl Failed, %{public}s", PathUtils::ShieldPath(path).c_str());
        return false;
    };
    return true;
}

bool SubAcl(const std::string &path)
{
    std::string entry = SA_PERMISSION_U_CLEAN;
    if (StorageDaemon::AclSetAccess(path, entry) != ACL_SUCC) {
        REQUEST_HILOGE("Sub Acl Failed, %{public}s", PathUtils::ShieldPath(path).c_str());
        return false;
    };
    return true;
}

// Grants one path: registers it (or bumps its refcount) in pathMap_ and
// applies the ACL. Re-applying on an existing entry keeps the ACL set even
// if an earlier Sub failed to clean up.
bool AddOnePathToMap(const std::string &path, const bool isFile, const Action action)
{
    std::lock_guard<std::mutex> lockGuard(pathMutex_);
    auto it = pathMap_.find(path);
    if (it == pathMap_.end()) {
        if (!AddAcl(path, isFile, action)) {
            return false;
        }
        pathMap_.emplace(path, std::tuple(isFile, 1));
    } else {
        // It is necessary to ensure that the permissions are set.
        if (!AddAcl(path, isFile, action)) {
            return false;
        }
        auto &[iFile, count] = it->second;
        iFile = isFile;
        count++;
    }
    return true;
}

// Releases one reference; the last one revokes the ACL and drops the map
// entry. Anomalies (unknown path, changed isFile, zero count) are logged
// but never crash the caller.
bool SubOnePathToMap(const std::string &path, const bool isFile)
{
    std::lock_guard<std::mutex> lockGuard(pathMutex_);
    auto it = pathMap_.find(path);
    if (it == pathMap_.end()) {
        REQUEST_HILOGE("SubOnePathToMap no path, %{public}s", PathUtils::ShieldPath(path).c_str());
        return false;
    }
    auto &[iFile, count] = it->second;
    if (iFile != isFile) {
        REQUEST_HILOGE("SubOnePathToMap path changed, %{public}s", PathUtils::ShieldPath(path).c_str());
    }
    if (count <= 0) {
        REQUEST_HILOGE("SubOnePathToMap count 0, %{public}s", PathUtils::ShieldPath(path).c_str());
        pathMap_.erase(it);
        return false;
    }
    count--;
    if (count == 0) {
        const bool ret = SubAcl(path);
        pathMap_.erase(it);
        return ret;
    }
    return true;
}

void SubPathsVec(const std::vector<std::pair<std::string, bool>> &paths)
{
    for (auto &elem : paths) {
        SubOnePathToMap(elem.first, elem.second);
    }
    return;
}

// Grants the whole chain; if a component fails, releases the ones already
// granted before returning false (no half-granted chain is left behind).
bool PathUtils::AddPathsToMap(const std::string &path, const Action action)
{
    std::vector<std::pair<std::string, bool>> paths = SelectPath(SplitPath(path));
    if (paths.empty()) {
        return false;
    }
    std::vector<std::pair<std::string, bool>> completePaths;
    completePaths.reserve(paths.size());
    for (auto &elem : paths) {
        if (!AddOnePathToMap(elem.first, elem.second, action)) {
            SubPathsVec(completePaths);
            return false;
        }
        completePaths.emplace_back(elem);
    }
    return true;
}

bool PathUtils::SubPathsToMap(const std::string &path)
{
    std::vector<std::pair<std::string, bool>> paths = SelectPath(SplitPath(path));
    if (paths.empty()) {
        return false;
    }
    SubPathsVec(paths);
    return true;
}

// "abcde" -> "**cde"
std::string ShieldStr(const std::string &s)
{
    if (s.empty()) {
        return "";
    }
    size_t n = s.length();
    size_t halfLen = n / 2;
    return std::string(halfLen, '*') + s.substr(halfLen);
}

// "/ab/abcde" -> "/*b/**cde"
std::string PathUtils::ShieldPath(const std::string &path)
{
    std::istringstream iss(path);
    std::string token;
    std::string result;

    while (std::getline(iss, token, '/')) {
        if (token.empty()) {
            continue;
        }
        result += '/';
        result += ShieldStr(token);
    }

    return result;
}
} // namespace OHOS::Request
