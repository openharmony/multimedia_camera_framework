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

#ifndef OHOS_CAMERA_CAMERA_INPUT_H
#define OHOS_CAMERA_CAMERA_INPUT_H

#include <iostream>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "camera_death_recipient.h"
#include "camera_device.h"
#include "camera_device_ability_items.h"
#include "camera_info.h"
#include "capture_input.h"
#include "camera_device_service_callback_stub.h"
#include "icamera_device_service.h"
#include "icamera_device_service_callback.h"
#include "metadata_common_utils.h"
#include "output/camera_output_capability.h"
#include "camera_timer.h"
#include "color_space_info_parse.h"
namespace OHOS {
namespace CameraStandard {
class ErrorCallback {
public:
    ErrorCallback() = default;
    virtual ~ErrorCallback() = default;
    virtual void OnError(const int32_t errorType, const int32_t errorMsg) const = 0;
};

class CameraOcclusionDetectCallback {
public:
    CameraOcclusionDetectCallback() = default;
    virtual ~CameraOcclusionDetectCallback() = default;
    virtual void OnCameraOcclusionDetected(const uint8_t isCameraOcclusion, const uint8_t isCameraLensDirty) const = 0;
};

class ResultCallback {
public:
    ResultCallback() = default;
    virtual ~ResultCallback() = default;
    virtual void OnResult(const uint64_t timestamp,
        const std::shared_ptr<OHOS::Camera::CameraMetadata> &result) const = 0;
};

class CameraInput : public CaptureInput {
public:
    [[deprecated]] explicit CameraInput(sptr<ICameraDeviceService>& deviceObj, sptr<CameraInfo>& camera);
    explicit CameraInput(sptr<ICameraDeviceService>& deviceObj, sptr<CameraDevice>& camera);
    virtual ~CameraInput();

    /**
    * @brief open camera.
    */
    int Open() override;

    /**
    * @brief open secure camera.
    */
    int Open(bool isEnableSecureCamera, uint64_t* secureSeqId) override;

    /**
    * @brief open camera with CameraConcurrentType.
    */
    int Open(int32_t cameraConcurrentType) override;

    /**
    * @brief close camera.
    */
    int Close() override;

    /**
    * @brief closeDelayed camera.
    */
    int closeDelayed(int32_t delayTime);

    /**
    * @brief create new device control setting.
    */
    [[deprecated]] void LockForControl();

    /**
    * @brief submit device control setting.
    *
    * @return Returns CAMERA_OK is success.
    */
    [[deprecated]] int32_t UnlockForControl();

    /**
    * @brief Get the supported format for photo.
    *
    * @return Returns vector of camera_format_t supported format for photo.
    */
    [[deprecated]] std::vector<camera_format_t> GetSupportedPhotoFormats();

    /**
    * @brief Get the supported format for video.
    *
    * @return Returns vector of camera_format_t supported format for video.
    */
    [[deprecated]] std::vector<camera_format_t> GetSupportedVideoFormats();

    /**
    * @brief Get the supported format for preview.
    *
    * @return Returns vector of camera_format_t supported format for preview.
    */
    [[deprecated]] std::vector<camera_format_t> GetSupportedPreviewFormats();

    /**
    * @brief Get the supported exposure modes.
    *
    * @return Returns vector of camera_exposure_mode_enum_t supported exposure modes.
    */
    [[deprecated]] std::vector<camera_exposure_mode_enum_t> GetSupportedExposureModes();

    /**
    * @brief Query whether given exposure mode supported.
    *
    * @param camera_exposure_mode_enum_t exposure mode to query.
    * @return True is supported false otherwise.
    */
    [[deprecated]] bool IsExposureModeSupported(camera_exposure_mode_enum_t exposureMode);

    /**
    * @brief Set exposure mode.
    *
    * @param camera_exposure_mode_enum_t exposure mode to be set.
    */
    [[deprecated]] void SetExposureMode(camera_exposure_mode_enum_t exposureMode);

    /**
    * @brief Get the current exposure mode.
    *
    * @return Returns current exposure mode.
    */
    [[deprecated]] camera_exposure_mode_enum_t GetExposureMode();

    /**
    * @brief Get the supported Focus modes.
    *
    * @return Returns vector of camera_focus_mode_enum_t supported exposure modes.
    */
    [[deprecated]] std::vector<camera_focus_mode_enum_t> GetSupportedFocusModes();

    /**
    * @brief Get exposure compensation range.
    *
    * @return Returns supported exposure compensation range.
    */
    [[deprecated]] std::vector<float> GetExposureBiasRange();

    /**
    * @brief Set exposure compensation value.
    *
    * @param exposure compensation value to be set.
    */
    [[deprecated]] void SetExposureBias(float exposureBias);

    /**
    * @brief Get exposure compensation value.
    *
    * @return Returns current exposure compensation value.
    */
    [[deprecated]] float GetExposureValue();

    /**
    * @brief Query whether given focus mode supported.
    *
    * @param camera_focus_mode_enum_t focus mode to query.
    * @return True is supported false otherwise.
    */
    [[deprecated]] bool IsFocusModeSupported(camera_focus_mode_enum_t focusMode);

    /**
    * @brief Set Focus mode.
    *
    * @param camera_focus_mode_enum_t focus mode to be set.
    */
    [[deprecated]] void SetFocusMode(camera_focus_mode_enum_t focusMode);

    /**
    * @brief Get the current focus mode.
    *
    * @return Returns current focus mode.
    */
    [[deprecated]] camera_focus_mode_enum_t GetFocusMode();

    /**
    * @brief Get focal length.
    *
    * @return Returns focal length value.
    */
    [[deprecated]] float GetFocalLength();

    /**
    * @brief Get the supported Zoom Ratio range.
    *
    * @return Returns vector<float> of supported Zoom ratio range.
    */
    [[deprecated]] std::vector<float> GetSupportedZoomRatioRange();

    /**
    * @brief Get the current Zoom Ratio.
    *
    * @return Returns current Zoom Ratio.
    */
    [[deprecated]] float GetZoomRatio();

    /**
    * @brief Set Zoom ratio.
    *
    * @param Zoom ratio to be set.
    */
    [[deprecated]] void SetZoomRatio(float zoomRatio);

    /**
    * @brief Get the supported Focus modes.
    *
    * @return Returns vector of camera_focus_mode_enum_t supported exposure modes.
    */
    [[deprecated]] std::vector<camera_flash_mode_enum_t> GetSupportedFlashModes();

    /**
    * @brief Get the current flash mode.
    *
    * @return Returns current flash mode.
    */
    [[deprecated]] camera_flash_mode_enum_t GetFlashMode();

    /**
    * @brief Set flash mode.
    *
    * @param camera_flash_mode_enum_t flash mode to be set.
    */
    [[deprecated]] void SetFlashMode(camera_flash_mode_enum_t flashMode);

    /**
    * @brief Set the error callback.
    * which will be called when error occurs.
    *
    * @param The ErrorCallback pointer.
    */
    void SetErrorCallback(std::shared_ptr<ErrorCallback> errorCallback);

    /**
    * @brief Set the result callback.
    * which will be called when result callback.
    *
    * @param The ResultCallback pointer.
    */
    void SetResultCallback(std::shared_ptr<ResultCallback> resultCallback);

    /**
    * @brief Set the CameraOcclusionDetect callback.
    * which will be called when CameraOcclusionDetect callback .
    *
    * @param The CameraOcclusionDetect pointer.
    */
    void SetOcclusionDetectCallback(std::shared_ptr<CameraOcclusionDetectCallback> cameraOcclusionDetectCallback);

    /**
    * @brief Release camera input.
    */
    int Release() override;

    /**
    * @brief Get the camera Id.
    *
    * @return Returns the camera Id.
    */
    std::string GetCameraId();

    /**
    * @brief Get Camera Device.
    *
    * @return Returns Camera Device pointer.
    */
    sptr<ICameraDeviceService> GetCameraDevice();

    /**
    * @brief Set Camera Device pointer.
    *
    */
    void SetCameraDevice(sptr<ICameraDeviceService> deviceObj);

    /**
    * @brief Get ErrorCallback pointer.
    *
    * @return Returns ErrorCallback pointer.
    */
    std::shared_ptr<ErrorCallback> GetErrorCallback();

    /**
    * @brief Get ResultCallback pointer.
    *
    * @return Returns ResultCallback pointer.
    */
    std::shared_ptr<ResultCallback>  GetResultCallback();

    /**
    * @brief Get CameraOcclusionDetectCallback pointer.
    *
    * @return Returns CameraOcclusionDetectCallback pointer.
    */
    std::shared_ptr<CameraOcclusionDetectCallback>  GetOcclusionDetectCallback();

    /**
    * @brief get the camera info associated with the device.
    *
    * @return Returns camera info.
    */
    void SetCameraDeviceInfo(sptr<CameraDevice> CameraObj);

    /**
    * @brief set the camera used as position with the device.
    *
    * @return Returns camera info.
    */
    void SetInputUsedAsPosition(CameraPosition usedAsPosition);

    /**
    *@brief set the cameraObj.
    */
    sptr<CameraDevice> GetCameraDeviceInfo() override;

    /**
    * @brief This function is called when there is device setting state change
    * and process the state callback.
    *
    * @param result metadata got from callback from service layer.
    */
    void ProcessCallbackUpdates(const uint64_t timestamp, const std::shared_ptr<OHOS::Camera::CameraMetadata> &result);

    /**
    * @brief This function is called when there is focus state change
    * and process the focus state callback.
    *
    * @param result metadata got from callback from service layer.
    */
    [[deprecated]] void ProcessAutoFocusUpdates(const std::shared_ptr<OHOS::Camera::CameraMetadata> &result);

    /**
    * @brief This function is called when there is exposure state change
    * and process the exposure state callback.
    *
    * @param result metadata got from callback from service layer.
    */
    [[deprecated]] void ProcessAutoExposureUpdates(const std::shared_ptr<OHOS::Camera::CameraMetadata> &result);

    /**
    * @brief Get current Camera Settings.
    *
    * @return Returns string encoded metadata setting.
    */
    std::string GetCameraSettings();

    /**
    * @brief set the camera metadata setting.
    *
    * @param string encoded camera metadata setting.
    * @return Returns 0 if success or appropriate error code if failed.
    */
    int32_t SetCameraSettings(std::string setting);

    std::shared_ptr<camera_metadata_item_t> GetMetaSetting(uint32_t metaTag);

    int32_t GetCameraAllVendorTags(std::vector<vendorTag_t> &infos);

    bool MergeMetadata(const std::shared_ptr<OHOS::Camera::CameraMetadata> srcMetadata,
        std::shared_ptr<OHOS::Camera::CameraMetadata> dstMetadata);
    void SwitchCameraDevice(sptr<ICameraDeviceService> &deviceObj, sptr<CameraDevice> &cameraObj);
    void InitCameraInput();
    void GetMetadataFromService(sptr<CameraDevice> &cameraObj);
    void ControlAuxiliary(AuxiliaryType type, AuxiliaryStatus status);
    void RecoveryOldDevice();
    CameraConcurrentLimtedCapability limtedCapabilitySave_;
    int32_t isConcurrentLimted_ = 0;
    std::queue<uint32_t>timeQueue_;
    void UnregisterTime();
private:
    std::mutex deviceObjMutex_;
    std::mutex errorCallbackMutex_;
    std::mutex cameraDeviceInfoMutex_;
    std::mutex resultCallbackMutex_;
    std::mutex occlusionCallbackMutex_;
    sptr<ICameraDeviceService> deviceObj_;
    sptr<CameraDevice> cameraObj_;
    std::shared_ptr<ResultCallback> resultCallback_;
    std::shared_ptr<ErrorCallback> errorCallback_;
    std::shared_ptr<CameraOcclusionDetectCallback> cameraOcclusionDetectCallback_;
    sptr<ICameraDeviceServiceCallback> CameraDeviceSvcCallback_;
    std::mutex interfaceMutex_;
    sptr<CameraDeathRecipient> deathRecipient_ = nullptr;
    void CameraServerDied(pid_t pid);
    int32_t UpdateSetting(std::shared_ptr<OHOS::Camera::CameraMetadata> changedMetadata);
    int32_t CameraDevicePhysicOpen(sptr<ICameraDeviceService> cameraDevicePhysic, int32_t cameraConcurrentType);
    void InputRemoveDeathRecipient();
    std::map<CameraPosition, camera_position_enum> positionMapping;
};

class CameraDeviceServiceCallback : public CameraDeviceServiceCallbackStub {
public:
    std::mutex deviceCallbackMutex_;
    wptr<CameraInput> camInput_ = nullptr;
    CameraDeviceServiceCallback() : camInput_(nullptr) {
    }

    explicit CameraDeviceServiceCallback(CameraInput* camInput) : camInput_(camInput) {
    }

    ~CameraDeviceServiceCallback()
    {
        std::lock_guard<std::mutex> lock(deviceCallbackMutex_);
        camInput_ = nullptr;
    }

    int32_t OnError(const int32_t errorType, const int32_t errorMsg) override;

    int32_t OnResult(const uint64_t timestamp, const std::shared_ptr<OHOS::Camera::CameraMetadata> &result) override;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_INPUT_H

