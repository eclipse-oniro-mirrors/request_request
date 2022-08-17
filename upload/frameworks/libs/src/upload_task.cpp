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

#include <thread>
#include "curl/curl.h"
#include "curl/easy.h"
#include "hitrace_meter.h"
#include "hisysevent.h"
#include "upload_task.h"

namespace OHOS::Request::Upload {
const int USLEEPRUN = 50 * 1000;
UploadTask::UploadTask(std::shared_ptr<UploadConfig>& uploadConfig)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "UploadTask. In.");
    uploadConfig_ = uploadConfig;
    curlAdp_ = nullptr;
    state_ = STATE_INIT;
    uploadedSize_ = 0;
    totalSize_ = 0;
    progressCallback_ = nullptr;
    headerReceiveCallback_ = nullptr;
    failCallback_ = nullptr;
    completeCallback_ = nullptr;
    context_ = nullptr;
}

UploadTask::~UploadTask()
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "~UploadTask. In.");
    std::lock_guard<std::mutex> guard(mutex_);
    SetCallback(TYPE_PROGRESS_CALLBACK, nullptr);
    SetCallback(TYPE_HEADER_RECEIVE_CALLBACK, nullptr);
    SetCallback(TYPE_FAIL_CALLBACK, nullptr);
    Remove();
}

bool UploadTask::Remove()
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "Remove. In.");
    if (curlAdp_ != nullptr) {
        return curlAdp_->Remove();
    }
    ClearFileArray();
    return true;
}

void UploadTask::On(Type type, void *callback)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "On. In.");
    std::lock_guard<std::mutex> guard(mutex_);
    SetCallback(type, callback);
}

void UploadTask::Off(Type type)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "Off. In.");
    std::lock_guard<std::mutex> guard(mutex_);
    SetCallback(type, nullptr);
}

void UploadTask::Off(Type type, void *callback)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "Off. In.");

    std::lock_guard<std::mutex> guard(mutex_);
    if (callback == nullptr) {
        return;
    }

    if (type == TYPE_PROGRESS_CALLBACK && progressCallback_ != nullptr) {
        ((IProgressCallback*)callback)->Progress(uploadedSize_, totalSize_);
    }  else {
        UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "Off. type[%{public}d] not match.", type);
    }
    SetCallback(type, nullptr);
}

void UploadTask::SetCallback(Type type, void *callback)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "SetCallback. In.");
    if (type == TYPE_PROGRESS_CALLBACK) {
        progressCallback_ = (IProgressCallback*)callback;
        if (progressCallback_ && uploadedSize_ > 0) {
            progressCallback_->Progress(uploadedSize_, totalSize_);
        }
    } else if (type == TYPE_HEADER_RECEIVE_CALLBACK) {
        headerReceiveCallback_ = (IHeaderReceiveCallback*)callback;
        if (headerReceiveCallback_ && headerArray_.empty() == false) {
            for (auto header : headerArray_) {
                if (header.length() > 0) {
                    headerReceiveCallback_->HeaderReceive(header);
                }
            }
            headerArray_.clear();
        }
    } else if (type == TYPE_FAIL_CALLBACK) {
        failCallback_ = (IFailCallback*)callback;
        if (failCallback_ && state_ == STATE_FAILURE) {
            failCallback_->Fail(taskStates_);
        }
    } else if (type == TYPE_COMPLETE_CALLBACK) {
        completeCallback_ = (ICompleteCallback*)callback;
        if (completeCallback_ && state_ == STATE_SUCCESS) {
            completeCallback_->Complete(taskStates_);
        }
    } else {
        UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "SetCallback. type[%{public}d] not match.", type);
    }
}

void UploadTask::SetContext(std::shared_ptr<OHOS::AbilityRuntime::Context> context)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "SetContext. In.");
    context_ = context;
}

void UploadTask::Run(void *arg)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "Run. In.");
    usleep(USLEEPRUN);
    ((UploadTask*)arg)->OnRun();
    if (((UploadTask*)arg)->uploadConfig_->protocolVersion == "L5") {
        if (((UploadTask*)arg)->uploadConfig_->fcomplete) {
            ((UploadTask*)arg)->uploadConfig_->fcomplete();
            UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "Complete.");
        }
    }
}

void UploadTask::OnRun()
{
    std::string traceParam = "url:" + uploadConfig_->url + "file num:" + std::to_string(uploadConfig_->files.size());
    HitraceScoped trace(HITRACE_TAG_MISC, "exec upload task " + traceParam);
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "OnRun. In.");
    state_ = STATE_RUNNING;
    obtainFile_ =  std::make_shared<ObtainFile>();
    GetFileArray();
    if (fileArray_.empty()) {
        return;
    }
    curlAdp_ = std::make_shared<CUrlAdp>(fileArray_, uploadConfig_);
    TaskResult taskResult;
    curlAdp_->DoUpload((IUploadTask*)this, taskResult);
    ClearFileArray();
    if (taskResult.failCount != 0) {
        ReportTaskFault(taskResult);
    }
}

void UploadTask::ReportTaskFault(TaskResult taskResult) const
{
    OHOS::HiviewDFX::HiSysEvent::Write(OHOS::HiviewDFX::HiSysEvent::Domain::REQUEST,
        REQUEST_TASK_FAULT,
        OHOS::HiviewDFX::HiSysEvent::EventType::FAULT,
        TASKS_TYPE, UPLOAD,
        TOTAL_FILE_NUM, fileArray_.size(),
        FAIL_FILE_NUM, taskResult.failCount,
        SUCCESS_FILE_NUM, taskResult.successCount,
        ERROR_INFO, taskResult.errorCode);
}

void UploadTask::OnProgress(curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "OnProgress. In.");
    std::lock_guard<std::mutex> guard(mutex_);
    uploadedSize_ = ulnow;
    if (uploadedSize_ == totalSize_) {
        state_ = STATE_SUCCESS;
    }
    if (progressCallback_) {
        progressCallback_->Progress(uploadedSize_, totalSize_);
    }
}

void UploadTask::OnHeaderReceive(char *buffer, size_t size, size_t nitems)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "OnHeaderReceive. In.");
    std::lock_guard<std::mutex> guard(mutex_);
    std::string header(buffer, size * nitems);
    header_ = header;
    if (headerReceiveCallback_) {
        headerReceiveCallback_->HeaderReceive(header_);
    } else {
        headerArray_.push_back(header);
    }
}

void UploadTask::OnFail(const std::vector<TaskState> &taskStates)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "OnFail. In.");
    {
        std::lock_guard<std::mutex> guard(mutex_);
        taskStates_ = taskStates;
        state_ = STATE_FAILURE;
    }
    if (failCallback_) {
        failCallback_->Fail(taskStates);
    }
}

void UploadTask::OnComplete(const std::vector<TaskState> &taskStates)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "OnComplete. In.");
    {
        std::lock_guard<std::mutex> guard(mutex_);
        taskStates_ = taskStates;
    }
    if (completeCallback_) {
        completeCallback_->Complete(taskStates_);
    }
}

void UploadTask::ExecuteTask()
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "ExecuteTask. In.");
    thread_ = std::make_unique<std::thread>(UploadTask::Run, this);
    thread_handle_ = thread_->native_handle();
    thread_->detach();
}

std::vector<FileData>& UploadTask::GetFileArray()
{
    UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "GetFileArray. In.");
    std::vector<TaskState> taskStates;
    TaskState taskState;
    unsigned int fileSize = 0;
    FileData data;
    FILE *file;
    totalSize_ = 0;

    for (auto f : uploadConfig_->files) {
        UPLOAD_HILOGD(UPLOAD_MODULE_FRAMEWORK, "filename is %{public}s", f.filename.c_str());
        unsigned int error = obtainFile_->GetFile(&file, f.uri, fileSize, context_);
        if (error != UPLOAD_ERRORCODE_NO_ERROR) {
            taskState = curlAdp_->SetTaskState(f.filename, error, FILE_READ_FAILED);
            taskStates.push_back(taskState);
            OnFail(taskStates);
            ClearFileArray();
            totalSize_ = 0;
            return fileArray_;
        }
        taskState = curlAdp_->SetTaskState(f.filename, error, FILE_READ_SUCCEEDED);
        taskStates.push_back(taskState);
        data.fp = file;
        std::size_t position = f.uri.find_last_of("/");
        if (position != std::string::npos) {
            data.filename = std::string(f.uri, position + 1);
            data.filename.erase(data.filename.find_last_not_of(" ") + 1);
        }
        data.name = f.name;
        data.type = f.type;
        fileArray_.push_back(data);
        totalSize_ += fileSize;
    }
    return fileArray_;
}

void UploadTask::ClearFileArray()
{
    for (auto &file : fileArray_) {
        if (file.fp != NULL) {
            fclose(file.fp);
        }
        file.name = "";
    }
    fileArray_.clear();
}

std::vector<std::string> UploadTask::StringSplit(const std::string& str, char delim)
{
    std::size_t previous = 0;
    std::size_t current = str.find(delim);
    std::vector<std::string> elems;
    while (current != std::string::npos) {
        if (current > previous) {
            elems.push_back(str.substr(previous, current - previous));
        }
        previous = current + 1;
        current = str.find(delim, previous);
    }
    if (previous != str.size()) {
        elems.push_back(str.substr(previous));
    }
    return elems;
}
} // namespace OHOS::Request::Upload