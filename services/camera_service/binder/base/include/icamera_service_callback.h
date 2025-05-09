/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_ICAMERA_SERVICE_CALLBACK_H
#define OHOS_CAMERA_ICAMERA_SERVICE_CALLBACK_H

#include "iremote_broker.h"

namespace OHOS {
namespace CameraStandard {
enum CameraStatus {
    CAMERA_STATUS_APPEAR = 0,
    CAMERA_STATUS_DISAPPEAR,
    CAMERA_STATUS_AVAILABLE,
    CAMERA_STATUS_UNAVAILABLE,
    CAMERA_SERVER_UNAVAILABLE
};

enum FlashStatus {
    FLASH_STATUS_OFF = 0,
    FLASH_STATUS_ON,
    FLASH_STATUS_UNAVAILABLE
};

enum TorchStatus {
    TORCH_STATUS_OFF = 0,
    TORCH_STATUS_ON,
    TORCH_STATUS_UNAVAILABLE
};

enum class CallbackInvoker : int32_t {
    CAMERA_HOST = 1,
    APPLICATION,
};

enum FoldStatus {
    UNKNOWN_FOLD = 0,
    EXPAND,
    FOLDED,
    HALF_FOLD
};

class ICameraServiceCallback : public IRemoteBroker {
public:
    virtual int32_t OnCameraStatusChanged(const std::string& cameraId, const CameraStatus status,
        const std::string& bundleName = "") = 0;
    virtual int32_t OnFlashlightStatusChanged(const std::string& cameraId, const FlashStatus status) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ICameraServiceCallback");
};

class ICameraMuteServiceCallback : public IRemoteBroker {
public:
    virtual int32_t OnCameraMute(bool muteMode) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ICameraMuteServiceCallback");
};

class ITorchServiceCallback : public IRemoteBroker {
public:
    virtual int32_t OnTorchStatusChange(const TorchStatus status) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"ITorchServiceCallback");
};

class IFoldServiceCallback : public IRemoteBroker {
public:
    virtual int32_t OnFoldStatusChanged(const FoldStatus status) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IFoldServiceCallback");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ICAMERA_SERVICE_CALLBACK_H

