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

#ifndef OHOS_CAMERA_DPS_VIDEO_PHOTO_PROCESS_STATE_H
#define OHOS_CAMERA_DPS_VIDEO_PHOTO_PROCESS_STATE_H

#include "istate.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class VideoPhotoProcessState : public IState {
public:
    explicit VideoPhotoProcessState(SchedulerType type, int32_t stateValue);

protected:
    SchedulerInfo ReevaluateSchedulerInfo() override;
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_VIDEO_PHOTO_PROCESS_STATE_H