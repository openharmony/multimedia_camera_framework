/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "output/video_output.h"
#include "media_log.h"
#include "camera_util.h"

namespace OHOS {
namespace CameraStandard {
VideoOutput::VideoOutput(sptr<IStreamRepeat> &streamRepeat)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE::VIDEO_OUTPUT), streamRepeat_(streamRepeat) {
}

void VideoOutput::SetCallback(std::shared_ptr<VideoCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("VideoOutput::SetCallback: Unregistering application callback!");
    }
    appCallback_ = callback;
    return;
}

std::vector<float> VideoOutput::GetSupportedFps()
{
    return {};
}

float VideoOutput::GetFps()
{
    return 0;
}

int32_t VideoOutput::SetFps(float fps)
{
    return CAMERA_OK;
}

int32_t VideoOutput::Start()
{
    return streamRepeat_->Start();
}

int32_t VideoOutput::Stop()
{
    return streamRepeat_->Stop();
}

void VideoOutput::Release()
{
    int32_t errCode = streamRepeat_->Release();
    if (errCode != CAMERA_OK) {
        MEDIA_ERR_LOG("Failed to release VideoOutput!, errCode: %{public}d", errCode);
    }
    return;
}

sptr<IStreamRepeat> VideoOutput::GetStreamRepeat()
{
    return streamRepeat_;
}
} // CameraStandard
} // OHOS