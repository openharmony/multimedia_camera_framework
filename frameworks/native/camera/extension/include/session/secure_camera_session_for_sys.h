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

#ifndef MULTIMEDIA_CAMERA_FRAMEWORK_MASTER_SECURE_CAMERA_SESSION_H
#define MULTIMEDIA_CAMERA_FRAMEWORK_MASTER_SECURE_CAMERA_SESSION_H

#include "capture_session_for_sys.h"
#include "icapture_session.h"

namespace OHOS {
namespace CameraStandard {
class SecureCameraSessionForSys : public CaptureSessionForSys {
public:
    explicit SecureCameraSessionForSys(sptr<ICaptureSession> &secureCameraSession)
        : CaptureSessionForSys(secureCameraSession) {}

    ~SecureCameraSessionForSys();

    /**
     * @brief the given Ouput for secure-stream can be added to session.
     *
     * @param CaptureOutput to be added to session.
     */
    int32_t AddSecureOutput(sptr<CaptureOutput>& output);
private:
    volatile bool isSetSecureOutput_ = false;
};
} // namespace CameraStandard
} // namespace OHOS
#endif //MULTIMEDIA_CAMERA_FRAMEWORK_MASTER_SECURE_CAMERA_SESSION_H