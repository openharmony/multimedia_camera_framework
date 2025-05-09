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

#ifndef OHOS_CAMERA_MANAGER_IMPL_H
#define OHOS_CAMERA_MANAGER_IMPL_H

#include <mutex>
#include <unordered_map>

#include "camera_input_impl.h"
#include "capture_session_impl.h"
#include "input/camera_manager.h"
#include "kits/native/include/camera/camera.h"
#include "kits/native/include/camera/camera_manager.h"
#include "metadata_output_impl.h"
#include "ndk_callback_map.h"
#include "photo_output_impl.h"
#include "preview_output_impl.h"
#include "video_output_impl.h"

namespace OHOS::CameraStandard {
class InnerCameraManagerTorchStatusCallback;
class InnerCameraManagerCameraStatusCallback;
class InnerCameraManagerFoldStatusCallback;
}
struct Camera_Manager {
public:
    Camera_Manager();
    ~Camera_Manager();

    Camera_ErrorCode RegisterCallback(CameraManager_Callbacks* cameraStatusCallback);

    Camera_ErrorCode UnregisterCallback(CameraManager_Callbacks* cameraStatusCallback);

    Camera_ErrorCode RegisterTorchStatusCallback(OH_CameraManager_TorchStatusCallback torchStatusCallback);

    Camera_ErrorCode UnregisterTorchStatusCallback(OH_CameraManager_TorchStatusCallback torchStatusCallback);

    Camera_ErrorCode GetSupportedCameras(Camera_Device** cameras, uint32_t* size);

    Camera_ErrorCode DeleteSupportedCameras(Camera_Device* cameras, uint32_t size);

    Camera_ErrorCode GetSupportedCameraOutputCapability(const Camera_Device* camera,
        Camera_OutputCapability** cameraOutputCapability);

    Camera_ErrorCode GetSupportedCameraOutputCapabilityWithSceneMode(const Camera_Device* camera,
        Camera_SceneMode sceneMode, Camera_OutputCapability** cameraOutputCapability);

    Camera_ErrorCode DeleteSupportedCameraOutputCapability(Camera_OutputCapability* cameraOutputCapability);

    Camera_ErrorCode IsCameraMuted(bool* isCameraMuted);

    Camera_ErrorCode CreateCaptureSession(Camera_CaptureSession** captureSession);

    Camera_ErrorCode CreateCameraInput(const Camera_Device* camera, Camera_Input** cameraInput);

    Camera_ErrorCode CreateCameraInputWithPositionAndType(Camera_Position position, Camera_Type type,
        Camera_Input** cameraInput);

    Camera_ErrorCode CreatePreviewOutput(const Camera_Profile* profile, const char* surfaceId,
        Camera_PreviewOutput** previewOutput);

    Camera_ErrorCode CreatePreviewOutputUsedInPreconfig(const char* surfaceId, Camera_PreviewOutput** previewOutput);

    Camera_ErrorCode CreatePhotoOutput(const Camera_Profile* profile, const char* surfaceId,
        Camera_PhotoOutput** photoOutput);

    Camera_ErrorCode CreatePhotoOutputWithoutSurface(const Camera_Profile* profile,
        Camera_PhotoOutput** photoOutput);

    Camera_ErrorCode CreatePhotoOutputUsedInPreconfig(const char* surfaceId, Camera_PhotoOutput** photoOutput);

    Camera_ErrorCode CreateVideoOutput(const Camera_VideoProfile* profile, const char* surfaceId,
        Camera_VideoOutput** videoOutput);

    Camera_ErrorCode CreateVideoOutputUsedInPreconfig(const char* surfaceId, Camera_VideoOutput** videoOutput);

    Camera_ErrorCode CreateMetadataOutput(const Camera_MetadataObjectType* type,
        Camera_MetadataOutput** metadataOutput);

    Camera_ErrorCode IsTorchSupported(bool* isTorchSupported);

    Camera_ErrorCode IsTorchSupportedByTorchMode(Camera_TorchMode torchMode, bool* isTorchSupported);

    Camera_ErrorCode SetTorchMode(Camera_TorchMode torchMode);

    Camera_ErrorCode GetCameraDevice(Camera_Position position, Camera_Type type, Camera_Device *camera);

    Camera_ErrorCode GetCameraConcurrentInfos(const Camera_Device *camera, uint32_t deviceSize,
        Camera_ConcurrentInfo **CameraConcurrentInfo, uint32_t *infoSize);

    static Camera_ErrorCode GetCameraOrientation(Camera_Device* cameras, uint32_t* orientation);

    static Camera_ErrorCode GetHostDeviceName(Camera_Device* camera, char** hostDeviceName);

    static Camera_ErrorCode GetHostDeviceType(Camera_Device* camera, Camera_HostDeviceType* hostDeviceType);

    static Camera_ErrorCode GetSupportedSceneModes(Camera_Device* camera,
        Camera_SceneMode** sceneModes, uint32_t* size);

    Camera_ErrorCode DeleteSceneModes(Camera_SceneMode* sceneModes);

    Camera_ErrorCode RegisterFoldStatusCallback(OH_CameraManager_OnFoldStatusInfoChange foldStatusCallback);

    Camera_ErrorCode UnregisterFoldStatusCallback(OH_CameraManager_OnFoldStatusInfoChange foldStatusCallback);

private:
    Camera_ErrorCode GetSupportedPreviewProfiles(Camera_OutputCapability* outCapability,
        std::vector<OHOS::CameraStandard::Profile> &previewProfiles);

    Camera_ErrorCode GetSupportedPhotoProfiles(Camera_OutputCapability* outCapability,
        std::vector<OHOS::CameraStandard::Profile> &photoProfiles);

    Camera_ErrorCode GetSupportedVideoProfiles(Camera_OutputCapability* outCapability,
        std::vector<OHOS::CameraStandard::VideoProfile> &videoProfiles);

    Camera_ErrorCode GetSupportedMetadataTypeList(Camera_OutputCapability* outCapability,
        std::vector<OHOS::CameraStandard::MetadataObjectType> &metadataTypeList);
    
    Camera_ErrorCode SetCameraConcurrentInfothis(const Camera_Device *camera, uint32_t deviceSize,
        Camera_ConcurrentInfo *CameraConcurrentInfothis,
        std::vector<bool> &cameraConcurrentType, std::vector<std::vector<OHOS::CameraStandard::SceneMode>> &modes,
        std::vector<std::vector<OHOS::sptr<OHOS::CameraStandard::CameraOutputCapability>>> &outputCapabilities);

    OHOS::sptr<OHOS::CameraStandard::CameraManager> cameraManager_;

    static NDKCallbackMap<CameraManager_Callbacks*, OHOS::CameraStandard::InnerCameraManagerCameraStatusCallback>
        cameraStatusCallbackMap_;

    static NDKCallbackMap<OH_CameraManager_OnFoldStatusInfoChange,
        OHOS::CameraStandard::InnerCameraManagerFoldStatusCallback>
        cameraFoldStatusCallbackMap_;

    static NDKCallbackMap<OH_CameraManager_TorchStatusCallback,
        OHOS::CameraStandard::InnerCameraManagerTorchStatusCallback>
        torchStatusCallbackMap_;

    static thread_local OHOS::sptr<OHOS::Surface> photoSurface_;
};
#endif // OHOS_CAMERA_CAPTURE_INPUT_H