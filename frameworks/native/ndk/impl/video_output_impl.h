/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_VIDEO_OUTPUT_IMPL_H
#define OHOS_VIDEO_OUTPUT_IMPL_H

#include "kits/native/include/camera/camera.h"
#include "kits/native/include/camera/video_output.h"
#include "output/video_output.h"

struct Camera_VideoOutput {
public:
    explicit Camera_VideoOutput(OHOS::sptr<OHOS::CameraStandard::VideoOutput> &innerVideoOutput);
    ~Camera_VideoOutput();

    Camera_ErrorCode RegisterCallback(VideoOutput_Callbacks* callback);

    Camera_ErrorCode UnregisterCallback(VideoOutput_Callbacks* callback);

    Camera_ErrorCode Start();

    Camera_ErrorCode Stop();

    Camera_ErrorCode Release();

    OHOS::sptr<OHOS::CameraStandard::VideoOutput> GetInnerVideoOutput();

private:
    OHOS::sptr<OHOS::CameraStandard::VideoOutput> innerVideoOutput_;
};
#endif // OHOS_CAMERA_CAPTURE_INPUT_H
