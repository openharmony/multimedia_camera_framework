/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
 
#ifndef OHOS_CAMERA_PHOTO_SESSION_FOR_SYS_H
#define OHOS_CAMERA_PHOTO_SESSION_FOR_SYS_H
 
#include "capture_session_for_sys.h"
#include "icapture_session.h"

namespace OHOS {
namespace CameraStandard {
class PhotoSessionForSys : public CaptureSessionForSys {
public:
    explicit PhotoSessionForSys(sptr<ICaptureSession> &photoSessionForSys): CaptureSessionForSys(photoSessionForSys) {}
    ~PhotoSessionForSys() = default;

    /**
     * @brief Determine if the given Ouput can be added to session.
     *
     * @param CaptureOutput to be added to session.
     */
    bool CanAddOutput(sptr<CaptureOutput>& output) override;

    /**
     * @brief Check the preconfig type is supported or not.
     *
     * @param preconfigType The target preconfig type.
     * @param preconfigRatio The target ratio enum
     *
     * @return True if the preconfig type is supported, false otherwise.
     */
    bool CanPreconfig(PreconfigType preconfigType, ProfileSizeRatio preconfigRatio) override;

    /**
     * @brief Set the preconfig type.
     *
     * @param preconfigType The target preconfig type.
     * @param preconfigRatio The target ratio enum
     *
     * @return Camera error code.
     */
    int32_t Preconfig(PreconfigType preconfigType, ProfileSizeRatio preconfigRatio) override;

    /**
     * @brief Photo-Session can set frame rate range.
     *
     * @param minFps Min frame rate of range.
     * @param minFps Max frame rate of range.
     */
    bool CanSetFrameRateRange(int32_t minFps, int32_t maxFps, CaptureOutput* curOutput) override;

protected:
    std::shared_ptr<PreconfigProfiles> GeneratePreconfigProfiles(
        PreconfigType preconfigType, ProfileSizeRatio preconfigRatio) override;

private:
    bool IsPreconfigProfilesLegal(std::shared_ptr<PreconfigProfiles> configs);
    bool IsPhotoProfileLegal(sptr<CameraDevice>& device, Profile& photoProfile);
    bool IsPreviewProfileLegal(sptr<CameraDevice>& device, Profile& previewProfile);
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_PHOTO_SESSION_FOR_SYS_H