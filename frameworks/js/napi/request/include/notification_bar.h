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

#ifndef REQUEST_NOTIFICATION_BAR_H
#define REQUEST_NOTIFICATION_BAR_H

#include "napi/native_node_api.h"
#include "napi_utils.h"

namespace OHOS::Request {

// Notification-bar group progress: the group part of the request.agent
// JS API. createGroup mints a group (server-generated gid) with an
// optional customized notification; attachGroup binds task ids to a group
// so their progress is aggregated in one notification; deleteGroup removes
// the group. All three forward to the server via RequestManager.
napi_value createGroup(napi_env env, napi_callback_info info);
napi_value attachGroup(napi_env env, napi_callback_info info);
napi_value deleteGroup(napi_env env, napi_callback_info info);

} // namespace OHOS::Request

#endif