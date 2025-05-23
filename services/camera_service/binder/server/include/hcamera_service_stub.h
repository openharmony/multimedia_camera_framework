/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_HCAMERA_SERVICE_STUB_H
#define OHOS_CAMERA_HCAMERA_SERVICE_STUB_H
#define EXPORT_API __attribute__((visibility("default")))

#include "camera_util.h"
#include "icamera_service.h"
#include "iremote_stub.h"
#include "input/camera_death_recipient.h"
#include "input/i_standard_camera_listener.h"

namespace OHOS {
namespace CameraStandard {
class HCameraRgmProxy : public IRemoteProxy<ICameraBroker> {
public:
    explicit HCameraRgmProxy(const sptr<IRemoteObject> &impl);
    ~HCameraRgmProxy() = default;

    int32_t NotifyCloseCamera(std::string cameraId) override;
    int32_t NotifyMuteCamera(bool muteMode) override;
private:
    static inline BrokerDelegator<HCameraRgmProxy> delegator_;
};

class EXPORT_API HCameraServiceStub : public IRemoteStub<ICameraService> {
public:
    HCameraServiceStub();
    ~HCameraServiceStub();
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                        MessageParcel &reply, MessageOption &option) override;

private:
    int HandleGetCameras(MessageParcel &data, MessageParcel &reply);
    int HandleGetCameraIds(MessageParcel &data, MessageParcel &reply);
    int HandleGetCameraAbility(MessageParcel &data, MessageParcel &reply);
    int HandleCreateCameraDevice(MessageParcel &data, MessageParcel &reply);
    int HandleSetCameraCallback(MessageParcel &data, MessageParcel &reply);
    int HandleSetMuteCallback(MessageParcel &data, MessageParcel &reply);
    int HandleSetTorchCallback(MessageParcel &data, MessageParcel &reply);
    int HandleSetFoldStatusCallback(MessageParcel &data, MessageParcel &reply);
    int HandleCreateCaptureSession(MessageParcel &data, MessageParcel &reply);
    int HandleCreateDeferredPhotoProcessingSession(MessageParcel &data, MessageParcel &reply);
    int HandleCreateDeferredVideoProcessingSession(MessageParcel &data, MessageParcel &reply);
    int HandleCreatePhotoOutput(MessageParcel &data, MessageParcel &reply);
    int HandleCreatePreviewOutput(MessageParcel &data, MessageParcel &reply);
    int HandleCreateDeferredPreviewOutput(MessageParcel &data, MessageParcel &reply);
    int HandleCreateDepthDataOutput(MessageParcel &data, MessageParcel &reply);
    int HandleCreateMetadataOutput(MessageParcel &data, MessageParcel &reply);
    int HandleCreateVideoOutput(MessageParcel &data, MessageParcel &reply);
    int HandleIsTorchSupported(MessageParcel &data, MessageParcel &reply);
    int HandleIsCameraMuteSupported(MessageParcel &data, MessageParcel &reply);
    int HandleMuteCamera(MessageParcel &data, MessageParcel &reply);
    int HandleMuteCameraPersist(MessageParcel &data, MessageParcel &reply);
    int HandleIsCameraMuted(MessageParcel &data, MessageParcel &reply);
    int HandleGetTorchStatus(MessageParcel &data, MessageParcel &reply);
    int HandlePrelaunchCamera(MessageParcel &data, MessageParcel &reply);
    int HandlePreSwitchCamera(MessageParcel& data, MessageParcel& reply);
    int HandleSetPrelaunchConfig(MessageParcel &data, MessageParcel &reply);
    int HandleSetTorchLevel(MessageParcel &data, MessageParcel &reply);
    int HandleAllowOpenByOHSide(MessageParcel& data, MessageParcel& reply);
    int HandleNotifyCameraState(MessageParcel& data);
    int HandleSetPeerCallback(MessageParcel& data);
    int HandleUnsetPeerCallback(MessageParcel& data);
    int HandleProxyForFreeze(MessageParcel& data, MessageParcel& reply);
    int HandleResetAllFreezeStatus(MessageParcel& data, MessageParcel& reply);
    int HandleGetDmDeviceInfo(MessageParcel& data, MessageParcel& reply);
    int HandleGetCameraOutputStatus(MessageParcel& data, MessageParcel& reply);
    int HandleRequireMemorySize(MessageParcel& data, MessageParcel& reply);
    int HandleGetIdforCameraConcurrentType(MessageParcel& data, MessageParcel& reply);
    int HandleGetConcurrentCameraAbility(MessageParcel& data, MessageParcel& reply);
    int DestroyStubObj() override;
    int DestroyStubForPid(pid_t pid);
    void ClientDied(pid_t pid);
    void ClearCameraListenerByPid(pid_t pid);
    int SetListenerObject(const sptr<IRemoteObject> &object) override;
    int SetListenerObject(MessageParcel &data, MessageParcel &reply);
    int HandleCheckWhiteList(MessageParcel& data, MessageParcel& reply);
    virtual int32_t UnSetAllCallback(pid_t pid);
    virtual int32_t CloseCameraForDestory(pid_t pid);

    SafeMap<pid_t, sptr<IStandardCameraListener>> cameraListenerMap_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HCAMERA_SERVICE_STUB_H

