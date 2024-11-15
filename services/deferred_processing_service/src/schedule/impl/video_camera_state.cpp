/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "video_camera_state.h"

#include "dp_log.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
VideoCameraState::VideoCameraState(int32_t stateValue) : ISchedulerVideoState(stateValue)
{
    DP_DEBUG_LOG("entered.");
}

VideoCameraState::~VideoCameraState()
{
    DP_DEBUG_LOG("entered.");
}

VideoCameraState::VideoSchedulerInfo VideoCameraState::ReevaluateSchedulerInfo()
{
    DP_DEBUG_LOG("VideoCameraState: %{public}d", stateValue_);
    bool isNeedStop = (
        stateValue_ == CameraSessionStatus::SYSTEM_CAMERA_OPEN ||
        stateValue_ == CameraSessionStatus::NORMAL_CAMERA_OPEN);
    return {isNeedStop, false};
}
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS