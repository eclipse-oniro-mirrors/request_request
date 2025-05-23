/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_REQUEST_DOWNLOAD_MANAGER_H
#define OHOS_REQUEST_DOWNLOAD_MANAGER_H

#include <optional>

#include "i_notify_data_listener.h"
#include "i_response_listener.h"
#include "request_common.h"
#include "visibility.h"

namespace OHOS::Request {

class RequestManager {
public:
    REQUEST_API static const std::unique_ptr<RequestManager> &GetInstance();
    REQUEST_API ExceptionErrorCode CreateTasks(const std::vector<Config> &configs, std::vector<TaskRet> &rets);
    REQUEST_API ExceptionErrorCode StartTasks(
        const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets);
    REQUEST_API ExceptionErrorCode StopTasks(
        const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets);
    REQUEST_API ExceptionErrorCode ResumeTasks(
        const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets);
    REQUEST_API ExceptionErrorCode RemoveTasks(
        const std::vector<std::string> &tids, const Version version, std::vector<ExceptionErrorCode> &rets);
    REQUEST_API ExceptionErrorCode PauseTasks(
        const std::vector<std::string> &tids, const Version version, std::vector<ExceptionErrorCode> &rets);
    REQUEST_API ExceptionErrorCode ShowTasks(const std::vector<std::string> &tids, std::vector<TaskInfoRet> &rets);
    REQUEST_API ExceptionErrorCode ShowBatchProgress(
        const std::vector<std::string> &tids, std::vector<TaskProgressRet> &rets);
    REQUEST_API ExceptionErrorCode TouchTasks(
        const std::vector<TaskIdAndToken> &tidTokens, std::vector<TaskInfoRet> &rets);
    REQUEST_API ExceptionErrorCode SetMaxSpeeds(
        const std::vector<SpeedConfig> &speedConfig, std::vector<ExceptionErrorCode> &rets);
    REQUEST_API ExceptionErrorCode DisableTaskNotification(
        const std::vector<std::string> &tids, std::vector<ExceptionErrorCode> &rets);
    REQUEST_API ExceptionErrorCode SetMode(const std::string &tid, const Mode mode);

    REQUEST_API int32_t Create(const Config &config, int32_t seq, std::string &tid);
    REQUEST_API int32_t GetTask(const std::string &tid, const std::string &token, Config &config);
    REQUEST_API int32_t Start(const std::string &tid);
    REQUEST_API int32_t Stop(const std::string &tid);
    REQUEST_API int32_t Query(const std::string &tid, TaskInfo &info);
    REQUEST_API int32_t Touch(const std::string &tid, const std::string &token, TaskInfo &info);
    REQUEST_API int32_t Search(const Filter &filter, std::vector<std::string> &tids);
    REQUEST_API int32_t Show(const std::string &tid, TaskInfo &info);
    REQUEST_API int32_t ShowProgress(const std::string &tid, TaskProgress &taskProgress);
    REQUEST_API int32_t Pause(const std::string &tid, const Version version);
    REQUEST_API int32_t QueryMimeType(const std::string &tid, std::string &mimeType);
    REQUEST_API int32_t Remove(const std::string &tid, const Version version);
    REQUEST_API int32_t Resume(const std::string &tid);
    REQUEST_API int32_t SetMaxSpeed(const std::string &tid, const int64_t maxSpeed);

    REQUEST_API int32_t Subscribe(const std::string &taskId);
    REQUEST_API int32_t Unsubscribe(const std::string &taskId);

    REQUEST_API int32_t AddListener(
        const std::string &taskId, const SubscribeType &type, const std::shared_ptr<IResponseListener> &listener);
    REQUEST_API int32_t RemoveListener(
        const std::string &taskId, const SubscribeType &type, const std::shared_ptr<IResponseListener> &listener);
    REQUEST_API int32_t AddListener(
        const std::string &taskId, const SubscribeType &type, const std::shared_ptr<INotifyDataListener> &listener);
    REQUEST_API int32_t RemoveListener(
        const std::string &taskId, const SubscribeType &type, const std::shared_ptr<INotifyDataListener> &listener);
    REQUEST_API void RemoveAllListeners(const std::string &taskId);

    REQUEST_API void RestoreListener(void (*callback)());
    REQUEST_API void LoadRequestServer();
    REQUEST_API bool IsSaReady();
    REQUEST_API void ReopenChannel();
    REQUEST_API bool SubscribeSA();
    REQUEST_API bool UnsubscribeSA();
    REQUEST_API int32_t GetNextSeq();

    REQUEST_API int32_t CreateGroup(
        std::string &gid, const bool gauge, Notification &notification);
    REQUEST_API int32_t AttachGroup(const std::string &gid, const std::vector<std::string> &tid);
    REQUEST_API int32_t DeleteGroup(const std::string &gid);

private:
    RequestManager() = default;
    RequestManager(const RequestManager &) = delete;
    RequestManager(RequestManager &&) = delete;
    RequestManager &operator=(const RequestManager &) = delete;
};

} // namespace OHOS::Request
#endif // OHOS_REQUEST_DOWNLOAD_MANAGER_H
