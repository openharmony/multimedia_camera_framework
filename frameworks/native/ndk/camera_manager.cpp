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

#include "kits/native/include/camera/camera_manager.h"
#include "impl/camera_manager_impl.h"
#include "camera_log.h"
#include "hilog/log.h"

Camera_ErrorCode OH_CameraManager_RegisterFoldStatusInfoCallback(Camera_Manager* cameraManager,
    OH_CameraManager_OnFoldStatusInfoChange foldStatusInfoCallback)
{
    CHECK_ERROR_RETURN_RET_LOG(cameraManager == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, cameraManager is null!");
    CHECK_ERROR_RETURN_RET_LOG(foldStatusInfoCallback == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback is null!");
    cameraManager->RegisterFoldStatusCallback(foldStatusInfoCallback);
    return CAMERA_OK;
}

Camera_ErrorCode OH_CameraManager_UnregisterFoldStatusInfoCallback(Camera_Manager* cameraManager,
    OH_CameraManager_OnFoldStatusInfoChange foldStatusInfoCallback)
{
    CHECK_ERROR_RETURN_RET_LOG(cameraManager == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, cameraManager is null!");
    CHECK_ERROR_RETURN_RET_LOG(foldStatusInfoCallback == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback is null!");
    cameraManager->UnregisterFoldStatusCallback(foldStatusInfoCallback);
    return CAMERA_OK;
}

#ifdef __cplusplus
extern "C" {
#endif

Camera_ErrorCode OH_Camera_GetCameraManager(Camera_Manager** cameraManager)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    *cameraManager = new Camera_Manager();
    return CAMERA_OK;
}

Camera_ErrorCode OH_Camera_DeleteCameraManager(Camera_Manager* cameraManager)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    delete cameraManager;
    return CAMERA_OK;
}

Camera_ErrorCode OH_CameraManager_RegisterCallback(Camera_Manager* cameraManager, CameraManager_Callbacks* callback)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, callback is null!");
    CHECK_AND_RETURN_RET_LOG(callback->onCameraStatus != nullptr,
        CAMERA_INVALID_ARGUMENT, "Invaild argument, callback onCameraStatus is null!");

    cameraManager->RegisterCallback(callback);
    return CAMERA_OK;
}

Camera_ErrorCode OH_CameraManager_UnregisterCallback(Camera_Manager* cameraManager, CameraManager_Callbacks* callback)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, callback is null!");
    CHECK_AND_RETURN_RET_LOG(callback->onCameraStatus != nullptr,
        CAMERA_INVALID_ARGUMENT, "Invaild argument, callback onCameraStatus is null!");

    cameraManager->UnregisterCallback(callback);
    return CAMERA_OK;
}

Camera_ErrorCode OH_CameraManager_RegisterTorchStatusCallback(Camera_Manager* cameraManager,
    OH_CameraManager_TorchStatusCallback torchStatusCallback)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(torchStatusCallback != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, callback is null!");
    cameraManager->RegisterTorchStatusCallback(torchStatusCallback);
    return CAMERA_OK;
}

Camera_ErrorCode OH_CameraManager_UnregisterTorchStatusCallback(Camera_Manager* cameraManager,
    OH_CameraManager_TorchStatusCallback torchStatusCallback)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(torchStatusCallback != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, callback is null!");
    cameraManager->UnregisterTorchStatusCallback(torchStatusCallback);
    return CAMERA_OK;
}

Camera_ErrorCode OH_CameraManager_GetSupportedCameras(Camera_Manager* cameraManager,
    Camera_Device** cameras, uint32_t* size)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(cameras != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameras is null!");
    CHECK_AND_RETURN_RET_LOG(size!= nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, size is null!");

    return cameraManager->GetSupportedCameras(cameras, size);
}

Camera_ErrorCode OH_CameraManager_DeleteSupportedCameras(Camera_Manager* cameraManager,
    Camera_Device* cameras, uint32_t size)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(cameras != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameras is null!");
    uint32_t sizeMax = 0;
    cameraManager->GetSupportedCameras(&cameras, &sizeMax);
    CHECK_AND_RETURN_RET_LOG(size <= sizeMax, CAMERA_INVALID_ARGUMENT,
        "Invaild argument,size is invaild");

    return cameraManager->DeleteSupportedCameras(cameras, size);
}

Camera_ErrorCode OH_CameraManager_GetSupportedCameraOutputCapability(Camera_Manager* cameraManager,
    const Camera_Device* camera, Camera_OutputCapability** cameraOutputCapability)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(camera != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, camera is null!");
    CHECK_AND_RETURN_RET_LOG(cameraOutputCapability != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraOutputCapability is null!");

    return cameraManager->GetSupportedCameraOutputCapability(camera, cameraOutputCapability);
}

Camera_ErrorCode OH_CameraManager_GetSupportedCameraOutputCapabilityWithSceneMode(Camera_Manager* cameraManager,
    const Camera_Device* camera, Camera_SceneMode sceneMode, Camera_OutputCapability** cameraOutputCapability)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_GetSupportedCameraOutputCapabilityWithSceneMode is called.");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(camera != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, camera is null!");
    CHECK_AND_RETURN_RET_LOG(cameraOutputCapability != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraOutputCapability is null!");

    return cameraManager->GetSupportedCameraOutputCapabilityWithSceneMode(camera, sceneMode, cameraOutputCapability);
}

Camera_ErrorCode OH_CameraManager_DeleteSupportedCameraOutputCapability(Camera_Manager* cameraManager,
    Camera_OutputCapability* cameraOutputCapability)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(cameraOutputCapability != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraOutputCapability is null!");

    return cameraManager->DeleteSupportedCameraOutputCapability(cameraOutputCapability);
}

Camera_ErrorCode OH_CameraManager_IsCameraMuted(Camera_Manager* cameraManager, bool* isCameraMuted)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_IsCameraMuted is called");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");

    return cameraManager->IsCameraMuted(isCameraMuted);
}

Camera_ErrorCode OH_CameraManager_CreateCaptureSession(Camera_Manager* cameraManager,
    Camera_CaptureSession** captureSession)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(captureSession != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, captureSession is null!");

    return cameraManager->CreateCaptureSession(captureSession);
}

Camera_ErrorCode OH_CameraManager_CreateCameraInput(Camera_Manager* cameraManager,
    const Camera_Device* camera, Camera_Input** cameraInput)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(camera != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraDevice is null!");
    CHECK_AND_RETURN_RET_LOG(cameraInput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraInput is null!");

    return cameraManager->CreateCameraInput(camera, cameraInput);
}

Camera_ErrorCode OH_CameraManager_CreateCameraInput_WithPositionAndType(Camera_Manager* cameraManager,
    Camera_Position position, Camera_Type type, Camera_Input** cameraInput)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_CreateCameraInput_WithPositionAndType is called");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(cameraInput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraInput is null!");
    return cameraManager->CreateCameraInputWithPositionAndType(position, type, cameraInput);
}

Camera_ErrorCode OH_CameraManager_CreatePreviewOutput(Camera_Manager* cameraManager, const Camera_Profile* profile,
    const char* surfaceId, Camera_PreviewOutput** previewOutput)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(profile != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, profile is null!");
    CHECK_AND_RETURN_RET_LOG(surfaceId != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, surfaceId is null!");
    CHECK_AND_RETURN_RET_LOG(previewOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, previewOutput is null!");

    return cameraManager->CreatePreviewOutput(profile, surfaceId, previewOutput);
}

Camera_ErrorCode OH_CameraManager_CreatePreviewOutputUsedInPreconfig(Camera_Manager* cameraManager,
    const char* surfaceId, Camera_PreviewOutput** previewOutput)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_CreatePreviewOutputUsedInPreconfig is called.");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(surfaceId != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, surfaceId is null!");
    CHECK_AND_RETURN_RET_LOG(previewOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, previewOutput is null!");

    return cameraManager->CreatePreviewOutputUsedInPreconfig(surfaceId, previewOutput);
}

Camera_ErrorCode OH_CameraManager_CreatePhotoOutput(Camera_Manager* cameraManager, const Camera_Profile* profile,
    const char* surfaceId, Camera_PhotoOutput** photoOutput)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_CreatePhotoOutput is called");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(profile != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, profile is null!");
    CHECK_AND_RETURN_RET_LOG(surfaceId != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, surfaceId is null!");
    CHECK_AND_RETURN_RET_LOG(photoOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, photoOutput is null!");

    return cameraManager->CreatePhotoOutput(profile, surfaceId, photoOutput);
}

Camera_ErrorCode OH_CameraManager_CreatePhotoOutputWithoutSurface(Camera_Manager* cameraManager,
    const Camera_Profile* profile, Camera_PhotoOutput** photoOutput)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_CreatePhotoOutputWithoutSurface is called");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(profile != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, profile is null!");
    CHECK_AND_RETURN_RET_LOG(photoOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, photoOutput is null!");

    return cameraManager->CreatePhotoOutputWithoutSurface(profile, photoOutput);
}

Camera_ErrorCode OH_CameraManager_CreatePhotoOutputUsedInPreconfig(Camera_Manager* cameraManager,
    const char* surfaceId, Camera_PhotoOutput** photoOutput)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_CreatePhotoOutputUsedInPreconfig is called.");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(surfaceId != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, surfaceId is null!");
    CHECK_AND_RETURN_RET_LOG(photoOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, photoOutput is null!");

    return cameraManager->CreatePhotoOutputUsedInPreconfig(surfaceId, photoOutput);
}

Camera_ErrorCode OH_CameraManager_CreateVideoOutput(Camera_Manager* cameraManager, const Camera_VideoProfile* profile,
    const char* surfaceId, Camera_VideoOutput** videoOutput)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(profile != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, profile is null!");
    CHECK_AND_RETURN_RET_LOG(surfaceId != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, surfaceId is null!");
    CHECK_AND_RETURN_RET_LOG(videoOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, videoOutput is null!");

    return cameraManager->CreateVideoOutput(profile, surfaceId, videoOutput);
}

Camera_ErrorCode OH_CameraManager_CreateVideoOutputUsedInPreconfig(Camera_Manager* cameraManager,
    const char* surfaceId, Camera_VideoOutput** videoOutput)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_CreateVideoOutputUsedInPreconfig is called.");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(surfaceId != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, surfaceId is null!");
    CHECK_AND_RETURN_RET_LOG(videoOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, videoOutput is null!");

    return cameraManager->CreateVideoOutputUsedInPreconfig(surfaceId, videoOutput);
}

Camera_ErrorCode OH_CameraManager_CreateMetadataOutput(Camera_Manager* cameraManager,
    const Camera_MetadataObjectType* type, Camera_MetadataOutput** metadataOutput)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(type != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, type is null!");
    CHECK_AND_RETURN_RET_LOG(metadataOutput != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, metadataOutput is null!");

    return cameraManager->CreateMetadataOutput(type, metadataOutput);
}

Camera_ErrorCode OH_CameraDevice_GetCameraOrientation(Camera_Device* camera, uint32_t* orientation)
{
    CHECK_AND_RETURN_RET_LOG(camera != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraDevice is null!");
    CHECK_AND_RETURN_RET_LOG(orientation!= nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, orientation is null!");
    return Camera_Manager::GetCameraOrientation(camera, orientation);
}

Camera_ErrorCode OH_CameraDevice_GetHostDeviceName(Camera_Device* camera, char** hostDeviceName)
{
    CHECK_ERROR_RETURN_RET_LOG(camera == nullptr, CAMERA_INVALID_ARGUMENT,
        "lnvalid argument, cameraDevice is null!");
    CHECK_ERROR_RETURN_RET_LOG(hostDeviceName == nullptr, CAMERA_INVALID_ARGUMENT,
        "lnvalid argument, hostDeviceName is null!");
    return Camera_Manager::GetHostDeviceName(camera, hostDeviceName);
}

Camera_ErrorCode OH_CameraDevice_GetHostDeviceType(Camera_Device* camera, Camera_HostDeviceType* hostDeviceType)
{
    CHECK_ERROR_RETURN_RET_LOG(camera == nullptr, CAMERA_INVALID_ARGUMENT,
        "lnvalid argument, cameraDevice is null!");
    CHECK_ERROR_RETURN_RET_LOG(hostDeviceType == nullptr, CAMERA_INVALID_ARGUMENT,
        "lnvalid argument, hostDeviceType is null!");
    return Camera_Manager::GetHostDeviceType(camera, hostDeviceType);
}

Camera_ErrorCode OH_CameraManager_GetSupportedSceneModes(Camera_Device* camera,
    Camera_SceneMode** sceneModes, uint32_t* size)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_GetSupportedSceneModes is called.");
    CHECK_AND_RETURN_RET_LOG(camera != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, camera is null!");
    CHECK_AND_RETURN_RET_LOG(sceneModes != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, sceneModes is null!");
    CHECK_AND_RETURN_RET_LOG(size != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, size is null!");

    return Camera_Manager::GetSupportedSceneModes(camera, sceneModes, size);
}

Camera_ErrorCode OH_CameraManager_DeleteSceneModes(Camera_Manager* cameraManager, Camera_SceneMode* sceneModes)
{
    MEDIA_DEBUG_LOG("OH_CameraManager_DeleteSceneModes is called.");
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(sceneModes != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, sceneModes is null!");

    return cameraManager->DeleteSceneModes(sceneModes);
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_CameraManager_IsTorchSupported(Camera_Manager* cameraManager,
    bool* isTorchSupported)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");
    CHECK_AND_RETURN_RET_LOG(isTorchSupported != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, isTorchSupported is null!");

    return cameraManager->IsTorchSupported(isTorchSupported);
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_CameraManager_IsTorchSupportedByTorchMode(Camera_Manager* cameraManager,
    Camera_TorchMode torchMode, bool* isTorchSupported)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null");
    CHECK_AND_RETURN_RET_LOG(isTorchSupported != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, isTorchSupported is null");

    return cameraManager->IsTorchSupportedByTorchMode(torchMode, isTorchSupported);
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_CameraManager_SetTorchMode(Camera_Manager* cameraManager, Camera_TorchMode torchMode)
{
    CHECK_AND_RETURN_RET_LOG(cameraManager != nullptr, CAMERA_INVALID_ARGUMENT,
        "Invaild argument, cameraManager is null!");

    return cameraManager->SetTorchMode(torchMode);
}
#ifdef __cplusplus
}
#endif