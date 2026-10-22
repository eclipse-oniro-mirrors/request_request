/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OBTAIN_FILE_
#define OBTAIN_FILE_

#include <stdio.h>

#include "ability_context.h"
#include "context.h"
#include "data_ability_helper.h"
#include "i_file_adapter.h"

#define SPLIT_ZERO 0
#define SPLIT_ONE 1
#define SPLIT_TWO 2
#define SPLIT_THREE 3

namespace OHOS::Request::Upload {
// Opens upload source files by URI scheme: "dataability" URIs resolve
// through a data ability (read-only fd), "internal://cache/<name>" to a
// file in the app cache dir; any other scheme is UNSUPPORT_URI. Also
// reports the file size in bytes. The heavy lifting goes through the
// IFileAdapter seam.
class ObtainFile {
public:
    ObtainFile();
    ~ObtainFile();
    /// Dispatches by URI scheme; on failure *file is null / fileSize 0
    /// and the UploadErrorCode is returned.
    uint32_t GetFile(FILE **file, const std::string &fileUri, uint32_t &fileSize,
        std::shared_ptr<OHOS::AbilityRuntime::Context> &context);

protected:
    /// fdopen("r") of the data ability's fd; the FILE* owns the fd.
    uint32_t GetDataAbilityFile(FILE **file, const std::string &fileUri, uint32_t &fileSize,
        std::shared_ptr<OHOS::AbilityRuntime::Context> &context);
    /// Cache-dir file by relative name, fopen("r").
    uint32_t GetInternalFile(FILE **file, const std::string &fileUri, uint32_t &fileSize,
        std::shared_ptr<OHOS::AbilityRuntime::Context> &context);
    /// Strips the mandatory "internal://cache/" prefix.
    bool SplitPath(const std::string &fileUri, std::string &fileName);
    /// The realpath must resolve to the path itself — symlinks and
    /// missing files are rejected.
    bool IsValidPath(const std::string &filePath);

private:
    std::shared_ptr<IFileAdapter> fileAdapter_;
};
} // namespace OHOS::Request::Upload
#endif