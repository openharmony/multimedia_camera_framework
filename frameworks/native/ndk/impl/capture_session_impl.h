/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAPTURE_SESSION_IMPL_H
#define OHOS_CAPTURE_SESSION_IMPL_H

#include "input/camera_manager.h"
#include "kits/native/include/camera/camera.h"
#include "kits/native/include/camera/capture_session.h"
#include "session/capture_session.h"
#include "camera_input_impl.h"
#include "preview_output_impl.h"
#include "video_output_impl.h"
#include "photo_output_impl.h"
#include "metadata_output_impl.h"

struct Camera_CaptureSession {
public:
    explicit Camera_CaptureSession(OHOS::sptr<OHOS::CameraStandard::CaptureSession> &innerCaptureSession);
    ~Camera_CaptureSession();

    Camera_ErrorCode RegisterCallback(CaptureSession_Callbacks* callback);

    Camera_ErrorCode UnregisterCallback(CaptureSession_Callbacks* callback);

    Camera_ErrorCode RegisterSmoothZoomInfoCallback(OH_CaptureSession_OnSmoothZoomInfo smoothZoomInfoCallback);

    Camera_ErrorCode UnregisterSmoothZoomInfoCallback(OH_CaptureSession_OnSmoothZoomInfo smoothZoomInfoCallback);

    Camera_ErrorCode SetSessionMode(Camera_SceneMode sceneMode);

    Camera_ErrorCode AddSecureOutput(Camera_PreviewOutput* previewOutput);

    Camera_ErrorCode BeginConfig();

    Camera_ErrorCode CommitConfig();

    Camera_ErrorCode AddInput(Camera_Input* cameraInput);

    Camera_ErrorCode RemoveInput(Camera_Input* cameraInput);

    Camera_ErrorCode AddPreviewOutput(Camera_PreviewOutput* previewOutput);

    Camera_ErrorCode RemovePreviewOutput(Camera_PreviewOutput* previewOutput);

    Camera_ErrorCode AddPhotoOutput(Camera_PhotoOutput* photoOutput);

    Camera_ErrorCode RemovePhotoOutput(Camera_PhotoOutput* photoOutput);

    Camera_ErrorCode AddVideoOutput(Camera_VideoOutput* videoOutput);

    Camera_ErrorCode RemoveVideoOutput(Camera_VideoOutput* videoOutput);

    Camera_ErrorCode AddMetaDataOutput(Camera_MetadataOutput* metadataOutput);

    Camera_ErrorCode RemoveMetaDataOutput(Camera_MetadataOutput* metadataOutput);

    Camera_ErrorCode IsVideoStabilizationModeSupported(Camera_VideoStabilizationMode mode, bool* isSupported);

    Camera_ErrorCode GetVideoStabilizationMode(Camera_VideoStabilizationMode* mode);

    Camera_ErrorCode SetVideoStabilizationMode(Camera_VideoStabilizationMode mode);

    Camera_ErrorCode GetZoomRatioRange(float* minZoom, float* maxZoom);

    Camera_ErrorCode GetZoomRatio(float* zoom);

    Camera_ErrorCode SetZoomRatio(float zoom);

    Camera_ErrorCode SetSmoothZoom(float targetZoom, Camera_SmoothZoomMode smoothZoomMode);

    Camera_ErrorCode IsFocusModeSupported(Camera_FocusMode focusMode, bool* isSupported);

    Camera_ErrorCode GetFocusMode(Camera_FocusMode* focusMode);

    Camera_ErrorCode SetFocusMode(Camera_FocusMode focusMode);

    Camera_ErrorCode SetQualityPrioritization(Camera_QualityPrioritization qualityPrioritization);

    Camera_ErrorCode SetFocusPoint(Camera_Point focusPoint);

    Camera_ErrorCode GetFocusPoint(Camera_Point* focusPoint);

    Camera_ErrorCode GetFocalLength(float* focalLength);

    Camera_ErrorCode HasFlash(bool* hasFlash);

    Camera_ErrorCode IsFlashModeSupported(Camera_FlashMode flashMode, bool* isSupported);

    Camera_ErrorCode GetFlashMode(Camera_FlashMode* flashMode);

    Camera_ErrorCode SetFlashMode(Camera_FlashMode flashMode);

    Camera_ErrorCode IsExposureModeSupported(Camera_ExposureMode exposureMode, bool* isSupported);

    Camera_ErrorCode GetExposureMode(Camera_ExposureMode* exposureMode);

    Camera_ErrorCode SetExposureMode(Camera_ExposureMode exposureMode);

    Camera_ErrorCode GetMeteringPoint(Camera_Point* point);

    Camera_ErrorCode SetMeteringPoint(Camera_Point point);

    Camera_ErrorCode GetExposureBiasRange(float* minExposureBias, float* maxExposureBias, float* step);

    Camera_ErrorCode SetExposureBias(float exposureBias);

    Camera_ErrorCode GetExposureBias(float* exposureBias);

    Camera_ErrorCode GetExposureValue(float* exposureValue);

    Camera_ErrorCode GetSupportedColorSpaces(OH_NativeBuffer_ColorSpace** colorSpace, uint32_t* size);

    Camera_ErrorCode GetActiveColorSpace(OH_NativeBuffer_ColorSpace* colorSpace);

    Camera_ErrorCode DeleteColorSpaces(OH_NativeBuffer_ColorSpace* colorSpace);

    Camera_ErrorCode GetSupportedEffectTypes(Camera_ControlCenterEffectType** types, uint32_t* size);

    Camera_ErrorCode DeleteEffectTypes(Camera_ControlCenterEffectType* types);

    Camera_ErrorCode EnableControlCenter(bool enabled);
    
    Camera_ErrorCode SetActiveColorSpace(OH_NativeBuffer_ColorSpace colorSpace);

    Camera_ErrorCode Start();

    Camera_ErrorCode Stop();

    Camera_ErrorCode Release();

    Camera_ErrorCode CanAddInput(Camera_Input* cameraInput, bool* isSuccessful);

    Camera_ErrorCode CanAddPreviewOutput(Camera_PreviewOutput* previewOutput, bool* isSuccessful);

    Camera_ErrorCode CanAddPhotoOutput(Camera_PhotoOutput* photoOutput, bool* isSuccessful);

    Camera_ErrorCode CanAddVideoOutput(Camera_VideoOutput* cameraOutput, bool* isSuccessful);

    Camera_ErrorCode CanPreconfig(Camera_PreconfigType preconfigType, bool* canPreconfig);

    Camera_ErrorCode CanPreconfigWithRatio(Camera_PreconfigType preconfigType, Camera_PreconfigRatio preconfigRatio,
        bool* canPreconfig);

    Camera_ErrorCode Preconfig(Camera_PreconfigType preconfigType);

    Camera_ErrorCode PreconfigWithRatio(Camera_PreconfigType preconfigType, Camera_PreconfigRatio preconfigRatio);

    Camera_ErrorCode RegisterAutoDeviceSwitchStatusCallback(
        OH_CaptureSession_OnAutoDeviceSwitchStatusChange autoDeviceSwitchStatusChange);

    Camera_ErrorCode UnregisterAutoDeviceSwitchStatusCallback(
        OH_CaptureSession_OnAutoDeviceSwitchStatusChange autoDeviceSwitchStatusChange);

    Camera_ErrorCode RegisterSystemPressureLevelCallback(
        OH_CaptureSession_OnSystemPressureLevelChange systemPressureLevel);
    
    Camera_ErrorCode UnRegisterSystemPressureLevelCallback(
        OH_CaptureSession_OnSystemPressureLevelChange systemPressureLevel);

    Camera_ErrorCode RegisterControlCenterEffectStatusChangeCallback(
        OH_CaptureSession_OnControlCenterEffectStatusChange controlCenterStatusChange);
    
    Camera_ErrorCode UnregisterControlCenterEffectStatusChangeCallback(
        OH_CaptureSession_OnControlCenterEffectStatusChange controlCenterStatusChange);

    Camera_ErrorCode IsAutoDeviceSwitchSupported(bool* isSupported);

    Camera_ErrorCode IsControlCenterSupported(bool* isSupported);

    Camera_ErrorCode EnableAutoDeviceSwitch(bool enabled);

    Camera_ErrorCode IsMacroSupported(bool* isSupported);

    Camera_ErrorCode EnableMacro(bool enabled);

    Camera_ErrorCode IsWhiteBalanceModeSupported(
            Camera_WhiteBalanceMode whiteBalanceMode, bool* isSupported);

    Camera_ErrorCode GetWhiteBalanceMode(Camera_WhiteBalanceMode* whiteBalanceMode);

    Camera_ErrorCode GetWhiteBalanceRange(int32_t *minColorTemperature, int32_t *maxColorTemperature);

    Camera_ErrorCode GetWhiteBalance(int32_t *colorTemperature);

    Camera_ErrorCode SetWhiteBalance(int32_t colorTemperature);

    Camera_ErrorCode SetWhiteBalanceMode(Camera_WhiteBalanceMode whiteBalanceMode);

private:
    OHOS::sptr<OHOS::CameraStandard::CaptureSession> innerCaptureSession_;
};
#endif // OHOS_CAPTURE_SESSION_IMPL_H