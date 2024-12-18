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

#ifndef OHOS_CAMERA_HCAMERA_SERVICE_CALLBACK_PROXY_H
#define OHOS_CAMERA_HCAMERA_SERVICE_CALLBACK_PROXY_H
#define EXPORT_API __attribute__((visibility("default")))

#include "iremote_proxy.h"
#include "icamera_service_callback.h"

namespace OHOS {
namespace CameraStandard {
class EXPORT_API HCameraServiceCallbackProxy : public IRemoteProxy<ICameraServiceCallback> {
public:
    explicit HCameraServiceCallbackProxy(const sptr<IRemoteObject> &impl);
    virtual ~HCameraServiceCallbackProxy() = default;

    int32_t OnCameraStatusChanged(const std::string& cameraId, const CameraStatus status,
        const std::string& bundleName) override;
    int32_t OnFlashlightStatusChanged(const std::string& cameraId, const FlashStatus status) override;

private:
    static inline BrokerDelegator<HCameraServiceCallbackProxy> delegator_;
};

class EXPORT_API HCameraMuteServiceCallbackProxy : public IRemoteProxy<ICameraMuteServiceCallback> {
public:
    explicit HCameraMuteServiceCallbackProxy(const sptr<IRemoteObject> &impl);
    virtual ~HCameraMuteServiceCallbackProxy() = default;

    int32_t OnCameraMute(bool muteMode) override;
private:
    static inline BrokerDelegator<HCameraMuteServiceCallbackProxy> delegator_;
};

class EXPORT_API HTorchServiceCallbackProxy : public IRemoteProxy<ITorchServiceCallback> {
public:
    explicit HTorchServiceCallbackProxy(const sptr<IRemoteObject> &impl);
    virtual ~HTorchServiceCallbackProxy() = default;

    int32_t OnTorchStatusChange(const TorchStatus torchStatus) override;
private:
    static inline BrokerDelegator<HTorchServiceCallbackProxy> delegator_;
};

class EXPORT_API HFoldServiceCallbackProxy : public IRemoteProxy<IFoldServiceCallback> {
public:
    explicit HFoldServiceCallbackProxy(const sptr<IRemoteObject> &impl);
    virtual ~HFoldServiceCallbackProxy() = default;

    int32_t OnFoldStatusChanged(const FoldStatus foldStatus) override;
private:
    static inline BrokerDelegator<HFoldServiceCallbackProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HCAMERA_SERVICE_CALLBACK_PROXY_H

