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

#include "request_blob_serialize.h"

#include <securec.h>

#include "log.h"

namespace {
// A single string longer than UINT8_MAX cannot be represented by the 1-byte
// length header used by CStringToBlob, so the whole vector is rejected.
constexpr uint32_t MAX_STRING_LEN_IN_BLOB = UINT8_MAX;
} // namespace

std::vector<uint8_t> CFormItemToBlob(const CFormItem *cpointer, uint32_t length)
{
    std::vector<uint8_t> blob;
    if (cpointer == nullptr) {
        return blob;
    }
    for (uint32_t i = 0; i < length; ++i) {
        const CFormItem &obj = cpointer[i];
        const uint8_t *objBytes = reinterpret_cast<const uint8_t *>(&obj);
        blob.insert(blob.end(), objBytes, objBytes + sizeof(CFormItem));
        blob.insert(blob.end(), obj.name.cStr, obj.name.cStr + obj.name.len);
        blob.insert(blob.end(), obj.value.cStr, obj.value.cStr + obj.value.len);
    }
    return blob;
}

std::vector<CFormItem> BlobToCFormItem(const std::vector<uint8_t> &blob)
{
    std::vector<CFormItem> vec;
    size_t position = 0;
    while (position < blob.size()) {
        CFormItem obj;
        if (position + sizeof(CFormItem) > blob.size()) {
            REQUEST_HILOGE("corrupted form_items blob, drop remain items");
            break;
        }
        memcpy_s(&obj, sizeof(CFormItem), blob.data() + position, sizeof(CFormItem));
        position += sizeof(CFormItem);

        // Cumulative bound check: value is read at position + name.len, so each
        // length must fit into what is left after the previous ones. Subtracting
        // only after a passed check keeps `remaining` exact and overflow-free.
        size_t remaining = blob.size() - position;
        if (obj.name.len > remaining || obj.value.len > remaining - obj.name.len) {
            REQUEST_HILOGE("corrupted form_items blob, drop remain items");
            break;
        }
        obj.name.cStr = new char[obj.name.len];
        memcpy_s(obj.name.cStr, obj.name.len, blob.data() + position, obj.name.len);
        position += obj.name.len;

        obj.value.cStr = new char[obj.value.len];
        memcpy_s(obj.value.cStr, obj.value.len, blob.data() + position, obj.value.len);
        position += obj.value.len;

        vec.push_back(obj);
    }
    return vec;
}

std::vector<uint8_t> CFileSpecToBlob(const CFileSpec *cpointer, uint32_t length)
{
    std::vector<uint8_t> blob;
    if (cpointer == nullptr) {
        return blob;
    }
    for (uint32_t i = 0; i < length; ++i) {
        const CFileSpec &obj = cpointer[i];
        const uint8_t *objBytes = reinterpret_cast<const uint8_t *>(&obj);
        blob.insert(blob.end(), objBytes, objBytes + sizeof(CFileSpec));
        blob.insert(blob.end(), obj.name.cStr, obj.name.cStr + obj.name.len);
        blob.insert(blob.end(), obj.path.cStr, obj.path.cStr + obj.path.len);
        blob.insert(blob.end(), obj.fileName.cStr, obj.fileName.cStr + obj.fileName.len);
        blob.insert(blob.end(), obj.mimeType.cStr, obj.mimeType.cStr + obj.mimeType.len);
        blob.emplace_back(obj.is_user_file);
    }
    return blob;
}

std::vector<CFileSpec> BlobToCFileSpec(const std::vector<uint8_t> &blob)
{
    std::vector<CFileSpec> vec;
    size_t position = 0;
    while (position < blob.size()) {
        CFileSpec obj;
        if (position + sizeof(CFileSpec) > blob.size()) {
            REQUEST_HILOGE("corrupted file_specs blob, drop remain specs");
            break;
        }
        memcpy_s(&obj, sizeof(CFileSpec), blob.data() + position, sizeof(CFileSpec));
        position += sizeof(CFileSpec);

        // Cumulative bound check (see BlobToCFormItem): each field is read at
        // the position after all previous ones, and the trailing flag byte
        // needs one more byte beyond the four strings.
        size_t remaining = blob.size() - position;
        if (obj.name.len > remaining || obj.path.len > remaining - obj.name.len
            || obj.fileName.len > remaining - obj.name.len - obj.path.len
            || obj.mimeType.len > remaining - obj.name.len - obj.path.len - obj.fileName.len
            || remaining - obj.name.len - obj.path.len - obj.fileName.len - obj.mimeType.len < 1) {
            REQUEST_HILOGE("corrupted file_specs blob, drop remain specs");
            break;
        }
        obj.name.cStr = new char[obj.name.len];
        memcpy_s(obj.name.cStr, obj.name.len, blob.data() + position, obj.name.len);
        position += obj.name.len;

        obj.path.cStr = new char[obj.path.len];
        memcpy_s(obj.path.cStr, obj.path.len, blob.data() + position, obj.path.len);
        position += obj.path.len;

        obj.fileName.cStr = new char[obj.fileName.len];
        memcpy_s(obj.fileName.cStr, obj.fileName.len, blob.data() + position, obj.fileName.len);
        position += obj.fileName.len;

        obj.mimeType.cStr = new char[obj.mimeType.len];
        memcpy_s(obj.mimeType.cStr, obj.mimeType.len, blob.data() + position, obj.mimeType.len);
        position += obj.mimeType.len;

        obj.is_user_file = blob[position];
        position += 1;

        vec.push_back(obj);
    }
    return vec;
}

std::vector<uint8_t> CStringToBlob(const CStringWrapper *cpointer, uint32_t length)
{
    std::vector<uint8_t> blob;
    if (cpointer == nullptr) {
        return blob;
    }
    for (uint32_t i = 0; i < length; ++i) {
        const CStringWrapper &obj = cpointer[i];
        if (obj.len > MAX_STRING_LEN_IN_BLOB) {
            REQUEST_HILOGE("string too long for blob, len: %{public}u", obj.len);
            return {};
        }
        blob.push_back(static_cast<uint8_t>(obj.len));
        blob.insert(blob.end(), obj.cStr, obj.cStr + obj.len);
    }
    return blob;
}

std::vector<std::string> BlobToStringVec(const std::vector<uint8_t> &blob)
{
    std::vector<std::string> vec;
    size_t position = 0;
    while (position < blob.size()) {
        size_t len = static_cast<size_t>(blob[position++]);
        if (position + len > blob.size()) {
            REQUEST_HILOGE("corrupted blob, drop remain strings");
            break;
        }
        std::string str(blob.begin() + position, blob.begin() + position + len);
        position += len;

        vec.push_back(std::move(str));
    }

    return vec;
}

// convert vector<CFormItem> to vector<FormItem>
std::vector<FormItem> VecToFormItem(const std::vector<CFormItem> &cvec)
{
    std::vector<FormItem> vec;
    for (const CFormItem &obj : cvec) {
        FormItem formItem;
        formItem.name = std::string(obj.name.cStr, obj.name.len);
        formItem.value = std::string(obj.value.cStr, obj.value.len);
        vec.push_back(std::move(formItem));
        //release memory of obj(new)
        delete[] obj.name.cStr;
        delete[] obj.value.cStr;
    }
    return vec;
}

// convert vector<CFileSpec> to vector<FileSpec>
std::vector<FileSpec> VecToFileSpec(const std::vector<CFileSpec> &cvec)
{
    std::vector<FileSpec> vec;
    for (const CFileSpec &obj : cvec) {
        FileSpec fileSpec;
        fileSpec.name = std::string(obj.name.cStr, obj.name.len);
        fileSpec.path = std::string(obj.path.cStr, obj.path.len);
        fileSpec.fileName = std::string(obj.fileName.cStr, obj.fileName.len);
        fileSpec.mimeType = std::string(obj.mimeType.cStr, obj.mimeType.len);
        fileSpec.is_user_file = obj.is_user_file;
        vec.push_back(std::move(fileSpec));
        //release memory of obj(new)
        delete[] obj.name.cStr;
        delete[] obj.path.cStr;
        delete[] obj.fileName.cStr;
        delete[] obj.mimeType.cStr;
    }
    return vec;
}
