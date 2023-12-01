// Copyright (C) 2023 Huawei Device Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#[derive(Clone, Copy, PartialEq, Debug)]
pub(crate) enum Reason {
    Default = 0,
    TaskSurvivalOneMonth,
    WaittingNetWorkOneday,
    StoppedByNewFrontTask,
    RunningTaskMeetLimits,
    UserOperation,
    AppBackgroundOrTerminate,
    NetWorkOffline,
    UnSupportedNetWorkType,
    BuildClientFailed,
    BuildRequestFailed,
    GetFileSizeFailed,
    ContinuousTaskTimeOut,
    ConnectError,
    RequestError,
    UploadFileError,
    RedirectError,
    ProtocolError,
    IoError,
    UnSupportRangeRequest,
    OthersError,
}

impl From<u8> for Reason {
    fn from(value: u8) -> Self {
        match value {
            0 => Reason::Default,
            1 => Reason::TaskSurvivalOneMonth,
            2 => Reason::WaittingNetWorkOneday,
            3 => Reason::StoppedByNewFrontTask,
            4 => Reason::RunningTaskMeetLimits,
            5 => Reason::UserOperation,
            6 => Reason::AppBackgroundOrTerminate,
            7 => Reason::NetWorkOffline,
            8 => Reason::UnSupportedNetWorkType,
            9 => Reason::BuildClientFailed,
            10 => Reason::BuildRequestFailed,
            11 => Reason::GetFileSizeFailed,
            12 => Reason::ContinuousTaskTimeOut,
            13 => Reason::ConnectError,
            14 => Reason::RequestError,
            15 => Reason::UploadFileError,
            16 => Reason::RedirectError,
            17 => Reason::ProtocolError,
            18 => Reason::IoError,
            19 => Reason::UnSupportRangeRequest,
            _ => Reason::OthersError,
        }
    }
}

impl Reason {
    pub(crate) fn to_str(self) -> &'static str {
        match self {
            Reason::Default => "",
            Reason::TaskSurvivalOneMonth => "The task has not been completed for a month yet",
            Reason::WaittingNetWorkOneday => "The task waiting for network recovery has not been completed for a day yet",
            Reason::StoppedByNewFrontTask => "Stopped by a new front task",
            Reason::RunningTaskMeetLimits => "Too many task in running state",
            Reason::UserOperation => "User operation",
            Reason::AppBackgroundOrTerminate => "The app is background or terminate",
            Reason::NetWorkOffline => "NetWork is offline",
            Reason::UnSupportedNetWorkType => "NetWork type not meet the task config",
            Reason::BuildClientFailed => "Build client error",
            Reason::BuildRequestFailed => "Build request error",
            Reason::GetFileSizeFailed => "Failed because cannot get the file size from the server and the precise is setted true by user",
            Reason::ContinuousTaskTimeOut => "Continuous processing task time out",
            Reason::ConnectError => "Connect error",
            Reason::RequestError => "Request error",
            Reason::UploadFileError => "There are some files upload failed",
            Reason::RedirectError => "Redirect error",
            Reason::ProtocolError => "Http protocol error",
            Reason::IoError => "Io Error",
            Reason::UnSupportRangeRequest => "The server is not support range request",
            Reason::OthersError => "Some other error occured",
        }
    }
}