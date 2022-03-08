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

#ifndef UPLOAD_CONFIG
#define UPLOAD_CONFIG

#include <string>
#include <vector>

namespace OHOS::Request::Upload {
struct File {
    std::string filename;
    std::string name;
    std::string uri;
    std::string type;
};

struct RequestData {
    std::string name;
    std::string value;
};

struct UploadConfig {
    std::string url;
    std::string header;
    std::string method;
    std::vector<File> files;
    std::vector<RequestData> data;
};

struct FileData {
    FILE *fp;
    std::string name;
    void *adp;
};
} // end of OHOS::Request::Upload
#endif