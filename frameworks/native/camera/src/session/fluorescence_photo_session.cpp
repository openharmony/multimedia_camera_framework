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
 
#include "session/fluorescence_photo_session.h"
#include "camera_log.h"
 
namespace OHOS {
namespace CameraStandard {
FluorescencePhotoSession::~FluorescencePhotoSession()
{
}
 
bool FluorescencePhotoSession::CanAddOutput(sptr<CaptureOutput> &output)
{
    MEDIA_DEBUG_LOG("Enter Into FluorescencePhotoSession::CanAddOutput");
    CHECK_ERROR_RETURN_RET_LOG(!IsSessionConfiged() || output == nullptr, false,
        "FluorescencePhotoSession::CanAddOutput operation is Not allowed!");
    return output->GetOutputType() != CAPTURE_OUTPUT_TYPE_VIDEO &&
        CaptureSession::CanAddOutput(output);
}
} // namespace CameraStandard
} // namespace OHOS