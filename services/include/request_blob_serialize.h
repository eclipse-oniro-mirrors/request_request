/*
* Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef REQUEST_BLOB_SERIALIZE_H
#define REQUEST_BLOB_SERIALIZE_H

#include <cstdint>
#include <string>
#include <vector>

#include "c_form_item.h"
#include "c_string_wrapper.h"

/*
 * Blob serialization helpers for the request_task table. Each *ToBlob writer
 * pairs with the matching *BlobTo* reader; blobs written here are only
 * decoded by these readers.
 *
 * CStringToBlob / BlobToStringVec use a 1-byte length header per string, so a
 * single string longer than UINT8_MAX cannot be represented and is rejected
 * on the write side. All readers bound-check every length header against the
 * remaining blob size, so a corrupted or hostile blob yields a partial
 * (or empty) result instead of reading out of bounds.
 */
std::vector<uint8_t> CFormItemToBlob(const CFormItem *cpointer, uint32_t length);
std::vector<CFormItem> BlobToCFormItem(const std::vector<uint8_t> &blob);
std::vector<uint8_t> CFileSpecToBlob(const CFileSpec *cpointer, uint32_t length);
std::vector<CFileSpec> BlobToCFileSpec(const std::vector<uint8_t> &blob);
std::vector<uint8_t> CStringToBlob(const CStringWrapper *cpointer, uint32_t length);
std::vector<std::string> BlobToStringVec(const std::vector<uint8_t> &blob);

// convert vector<CFormItem> to vector<FormItem>
std::vector<FormItem> VecToFormItem(const std::vector<CFormItem> &cvec);
// convert vector<CFileSpec> to vector<FileSpec>
std::vector<FileSpec> VecToFileSpec(const std::vector<CFileSpec> &cvec);

#endif // REQUEST_BLOB_SERIALIZE_H
