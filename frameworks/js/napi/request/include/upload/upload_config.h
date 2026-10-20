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

#include <map>
#include <string>
#include <vector>

#include "curl/curl.h"

namespace OHOS::Request::Upload {
// One file source parsed from the old upload option's "files" array:
// filename (local file path / display name of the form part), name (form
// field name), uri (source URI, e.g. internal://cache/) and type (MIME).
struct File {
    std::string filename;
    std::string name;
    std::string uri;
    std::string type;
};

// One extra form field parsed from the option's "data" array.
struct RequestData {
    std::string name;
    std::string value;
};

// Response payload of one upload as delivered to the success callback:
// HTTP status code, body and the accumulated response header text.
struct UploadResponse {
    int32_t code;
    std::string data;
    std::string headers;
};

// Fully parsed config of one old-style upload task. The three callbacks
// are std::function wrappers over JS functions pinned by the NAPI layer
// and must only run on the JS thread. method selects the wire form: PUT
// uploads a raw body (CURLOPT_UPLOAD), anything else a multipart/form-data
// POST. protocolVersion tags the API era ("API3") and comes from the
// caller, not the JS option.
struct UploadConfig {
    std::string url;
    std::map<std::string, std::string> header;
    std::string method;
    std::vector<File> files;
    std::vector<RequestData> data;
    std::function<void(UploadResponse &data)> fsuccess;
    std::function<void(std::string &data, int32_t &code)> ffail;
    std::function<void()> fcomplete;
    std::string protocolVersion;
};
class CUrlAdp;
// Per-file runtime state of one sequential upload step (CUrlAdp::DoUpload
// copies each entry into its working mfileData_ before transfer):
// fp — opened source file; adp — back-pointer that keeps the CUrlAdp
// alive for the duration of the curl callbacks; upsize/totalsize —
// uploaded and total bytes (upsize is maintained by ProgressCallback);
// list — this request's curl_slist of extra headers, built by SetHeadData
// and freed by DoUpload after each file; responseHead/headSendFlag/
// httpCode — response header accumulation state driven by
// HeaderCallback; result — the file's UploadErrorCode.
struct FileData {
    FILE *fp;
    std::string filename;
    std::string name;
    std::string type;
    std::shared_ptr<CUrlAdp> adp;
    int64_t upsize;
    int64_t totalsize;
    uint32_t fileIndex;
    struct curl_slist *list;
    std::vector<std::string> responseHead;
    int32_t headSendFlag;
    int32_t httpCode;
    uint32_t result;
};
} // namespace OHOS::Request::Upload
#endif