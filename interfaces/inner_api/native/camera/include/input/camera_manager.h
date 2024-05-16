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

#ifndef OHOS_CAMERA_CAMERA_MANAGER_H
#define OHOS_CAMERA_CAMERA_MANAGER_H

#include <cstdint>
#include <iostream>
#include <mutex>
#include <refbase.h>
#include <thread>
#include <vector>

#include "camera_stream_info_parse.h"
#include "deferred_proc_session/deferred_photo_proc_session.h"
#include "hcamera_listener_stub.h"
#include "hcamera_service_callback_stub.h"
#include "hcamera_service_proxy.h"
#include "icamera_device_service.h"
#include "input/camera_death_recipient.h"
#include "input/camera_device.h"
#include "input/camera_info.h"
#include "input/camera_input.h"
#include "istream_common.h"
#include "istream_repeat.h"
#include "output/camera_output_capability.h"
#include "output/metadata_output.h"
#include "output/photo_output.h"
#include "output/preview_output.h"
#include "output/video_output.h"
#include "safe_map.h"

namespace OHOS {
namespace CameraStandard {
enum CameraDeviceStatus {
    CAMERA_DEVICE_STATUS_UNAVAILABLE = 0,
    CAMERA_DEVICE_STATUS_AVAILABLE
};

enum FlashlightStatus {
    FLASHLIGHT_STATUS_OFF = 0,
    FLASHLIGHT_STATUS_ON,
    FLASHLIGHT_STATUS_UNAVAILABLE
};

enum TorchMode {
    TORCH_MODE_OFF = 0,
    TORCH_MODE_ON,
    TORCH_MODE_AUTO
};

struct CameraStatusInfo {
    sptr<CameraInfo> cameraInfo;
    sptr<CameraDevice> cameraDevice;
    CameraStatus cameraStatus;
    ~CameraStatusInfo()
    {
        cameraInfo = nullptr;
        cameraDevice = nullptr;
    }
};

struct TorchStatusInfo {
    bool isTorchAvailable;
    bool isTorchActive;
    float torchLevel;
};

typedef enum OutputCapStreamType {
    PREVIEW = 0,
    VIDEO_STREAM = 1,
    STILL_CAPTURE = 2,
    POST_VIEW = 3,
    ANALYZE = 4,
    CUSTOM = 5
} OutputCapStreamType;

class CameraManagerCallback {
public:
    CameraManagerCallback() = default;
    virtual ~CameraManagerCallback() = default;
    virtual void OnCameraStatusChanged(const CameraStatusInfo &cameraStatusInfo) const = 0;
    virtual void OnFlashlightStatusChanged(const std::string &cameraID, const FlashStatus flashStatus) const = 0;
};

class CameraMuteListener {
public:
    CameraMuteListener() = default;
    virtual ~CameraMuteListener() = default;
    virtual void OnCameraMute(bool muteMode) const = 0;
};

class TorchListener {
public:
    TorchListener() = default;
    virtual ~TorchListener() = default;
    virtual void OnTorchStatusChange(const TorchStatusInfo &torchStatusInfo) const = 0;
};

class CameraManager : public RefBase {
public:
    virtual ~CameraManager();
    /**
     * @brief Get camera manager instance.
     *
     * @return Returns pointer to camera manager instance.
     */
    static sptr<CameraManager>& GetInstance();

    /**
     * @brief Get all available cameras.
     *
     * @return Returns vector of cameraDevice of available camera.
     */
    std::vector<sptr<CameraDevice>> GetSupportedCameras();

    /**
    * @brief Get support modes.
    *
    * @return Returns array the mode of current CameraDevice.
    */
    std::vector<SceneMode> GetSupportedModes(sptr<CameraDevice>& camera);

    /**
     * @brief Get extend output capaility of the mode of the given camera.
     *
     * @param Camera device for which extend capability need to be fetched.
     * @return Returns vector the ability of the mode of cameraDevice of available camera.
     */
    sptr<CameraOutputCapability> GetSupportedOutputCapability(sptr<CameraDevice>& camera, int32_t modeName = 0);

    /**
     * @brief Create camera input instance with provided camera position and type.
     *
     * @param The cameraDevice for which input has to be created.
     * @return Returns pointer to camera input instance.
     */

    sptr<CameraInput> CreateCameraInput(CameraPosition position, CameraType cameraType);

    /**
     * @brief Create camera input instance with provided camera position and type.
     *
     * @param The cameraDevice for which input has to be created.
     * @param Returns pointer to camera input instance.
     * @return Returns error code.
     */
    int CreateCameraInput(CameraPosition position, CameraType cameraType, sptr<CameraInput>* pCameraInput);

    /**
     * @brief Create camera input instance.
     *
     * @param The cameraDevice for which input has to be created.
     * @return Returns pointer to camera input instance.
     */
    sptr<CameraInput> CreateCameraInput(sptr<CameraDevice>& camera);

    /**
     * @brief Create camera input instance.
     *
     * @param The cameraDevice for which input has to be created.
     * @param Returns pointer to camera input instance.
     * @return Returns error code.
     */
    int CreateCameraInput(sptr<CameraDevice>& camera, sptr<CameraInput>* pCameraInput);

    /**
     * @brief Get all available cameras.
     *
     * @return Returns vector of cameraInfo of available camera.
     */
    [[deprecated]] std::vector<sptr<CameraInfo>> GetCameras();

    /**
     * @brief Create camera input instance.
     *
     * @param The cameraInfo for which input has to be created.
     * @return Returns pointer to camera input instance.
     */
    [[deprecated]] sptr<CameraInput> CreateCameraInput(sptr<CameraInfo>& camera);

    /**
     * @brief Create capture session.
     *
     * @return Returns pointer to capture session.
     */
    sptr<CaptureSession> CreateCaptureSession();

    /**
    * @brief Create capture session.
    *
    * @return Returns pointer to capture session.
    */
    sptr<CaptureSession> CreateCaptureSession(SceneMode mode);

    /**
     * @brief Create capture session.
     *
     * @param Returns pointer to capture session.
     * @return Returns error code.
     */
    int CreateCaptureSession(sptr<CaptureSession>* pCaptureSession);

    /**
     * @brief Create photo output instance using surface.
     *
     * @param The surface to be used for photo output.
     * @return Returns pointer to photo output instance.
     */
    sptr<PhotoOutput> CreatePhotoOutput(Profile& profile, sptr<IBufferProducer>& surface);

    /**
    * @brief Create deferred photo processing session.
    *
    * @return Returns pointer to capture session.
    */
    static sptr<DeferredPhotoProcSession> CreateDeferredPhotoProcessingSession(int userId,
        std::shared_ptr<IDeferredPhotoProcSessionCallback> callback);

    /**
    * @brief Create deferred photo processing session.
    *
    * @param Returns pointer to capture session.
    * @return Returns error code.
    */
    static int CreateDeferredPhotoProcessingSession(int userId,
        std::shared_ptr<IDeferredPhotoProcSessionCallback> callback,
        sptr<DeferredPhotoProcSession> *pDeferredPhotoProcSession);

    /**
     * @brief Create photo output instance.
     *
     * @param profile photo profile.
     * @param surface photo buffer surface.
     * @param pPhotoOutput pointer to photo output instance.
     * @return Returns error code.
     */
    int CreatePhotoOutput(Profile& profile, sptr<IBufferProducer>& surface, sptr<PhotoOutput>* pPhotoOutput);

    /**
     * @brief Create photo output instance without profile.
     *
     * @param surface photo buffer surface.
     * @param pPhotoOutput pointer to photo output instance.
     * @return Returns error code.
     */
    int CreatePhotoOutputWithoutProfile(sptr<IBufferProducer>& surface, sptr<PhotoOutput>* pPhotoOutput);

    /**
     * @brief Create photo output instance using surface.
     *
     * @param The surface to be used for photo output.
     * @return Returns pointer to photo output instance.
     */
    [[deprecated]] sptr<PhotoOutput> CreatePhotoOutput(sptr<IBufferProducer>& surface);

    /**
     * @brief Create photo output instance using IBufferProducer.
     *
     * @param The IBufferProducer to be used for photo output.
     * @param The format to be used for photo capture.
     * @return Returns pointer to photo output instance.
     */
    [[deprecated]] sptr<PhotoOutput> CreatePhotoOutput(const sptr<OHOS::IBufferProducer>& producer, int32_t format);

    /**
     * @brief Create video output instance using surface.
     *
     * @param The surface to be used for video output.
     * @return Returns pointer to video output instance.
     */
    sptr<VideoOutput> CreateVideoOutput(VideoProfile& profile, sptr<Surface>& surface);

    /**
     * @brief Create video output instance using surface.
     *
     * @param The surface to be used for video output.
     * @param Returns pointer to video output instance.
     * @return Returns error code.
     */
    int CreateVideoOutput(VideoProfile& profile, sptr<Surface>& surface, sptr<VideoOutput>* pVideoOutput);

    /**
     * @brief Create video output instance without profile.
     *
     * @param surface video buffer surface.
     * @param pVideoOutput pointer to video output instance.
     * @return Returns error code.
     */
    int CreateVideoOutputWithoutProfile(sptr<Surface>& surface, sptr<VideoOutput>* pVideoOutput);

    /**
     * @brief Create video output instance using surface.
     *
     * @param The surface to be used for video output.
     * @return Returns pointer to video output instance.
     */
    [[deprecated]] sptr<VideoOutput> CreateVideoOutput(sptr<Surface>& surface);

    /**
     * @brief Create video output instance using IBufferProducer.
     *
     * @param The IBufferProducer to be used for video output.
     * @param The format to be used for video capture.
     * @return Returns pointer to video output instance.
     */
    [[deprecated]] sptr<VideoOutput> CreateVideoOutput(const sptr<OHOS::IBufferProducer>& producer, int32_t format);

    /**
     * @brief Create preview output instance using surface.
     *
     * @param The surface to be used for preview.
     * @return Returns pointer to preview output instance.
     */
    sptr<PreviewOutput> CreatePreviewOutput(Profile& profile, sptr<Surface> surface);

    /**
     * @brief Create preview output instance.
     *
     * @param profile preview profile.
     * @param surface preview buffer surface.
     * @param pPhotoOutput pointer to photo preview instance.
     * @return Returns error code.
     */
    int CreatePreviewOutput(Profile& profile, sptr<Surface> surface, sptr<PreviewOutput>* pPreviewOutput);

    /**
     * @brief Create preview output instance without profile.
     *
     * @param surface preview buffer surface.
     * @param pPhotoOutput pointer to photo preview instance.
     * @return Returns error code.
     */
    int CreatePreviewOutputWithoutProfile(sptr<Surface> surface, sptr<PreviewOutput>* pPreviewOutput);

    /**
     * @brief Create preview output instance using surface.
     *
     * @param The surface to be used for preview.
     * @return Returns pointer to preview output instance.
     */
    [[deprecated]] sptr<PreviewOutput> CreatePreviewOutput(sptr<Surface> surface);

    /**
     * @brief Create preview output instance using IBufferProducer.
     *
     * @param The IBufferProducer to be used for preview output.
     * @param The format to be used for preview.
     * @return Returns pointer to video preview instance.
     */
    [[deprecated]] sptr<PreviewOutput> CreatePreviewOutput(const sptr<OHOS::IBufferProducer>& producer, int32_t format);

    /**
     * @brief Create preview output instance using surface.
     *
     * @param The surface to be used for preview.
     * @return Returns pointer to preview output instance.
     */
    sptr<PreviewOutput> CreateDeferredPreviewOutput(Profile& profile);

    /**
     * @brief Create preview output instance using surface.
     *
     * @param The surface to be used for preview.
     * @param Returns pointer to preview output instance.
     * @return Returns error code.
     */
    int CreateDeferredPreviewOutput(Profile& profile, sptr<PreviewOutput>* pPreviewOutput);

    /**
     * @brief Create preview output instance using surface
     * with custom width and height.
     *
     * @param The surface to be used for preview.
     * @param preview width.
     * @param preview height.
     * @return Returns pointer to preview output instance.
     */
    [[deprecated]] sptr<PreviewOutput> CreateCustomPreviewOutput(sptr<Surface> surface, int32_t width, int32_t height);

    /**
     * @brief Create preview output instance using IBufferProducer
     * with custom width and height.
     *
     * @param The IBufferProducer to be used for preview output.
     * @param The format to be used for preview.
     * @param preview width.
     * @param preview height.
     * @return Returns pointer to preview output instance.
     */
    [[deprecated]] sptr<PreviewOutput> CreateCustomPreviewOutput(
        const sptr<OHOS::IBufferProducer>& producer, int32_t format, int32_t width, int32_t height);

    /**
     * @brief Create metadata output instance.
     *
     * @return Returns pointer to metadata output instance.
     */
    sptr<MetadataOutput> CreateMetadataOutput();

    /**
     * @brief Create metadata output instance.
     *
     * @param Returns pointer to metadata output instance.
     * @return Returns error code.
     */
    int CreateMetadataOutput(sptr<MetadataOutput>& pMetadataOutput);

    /**
     * @brief Set camera manager callback.
     *
     * @param CameraManagerCallback pointer.
     */
    void SetCallback(std::shared_ptr<CameraManagerCallback> callback);

    /**
     * @brief Get the application callback.
     *
     * @return CameraManagerCallback pointer is set by application.
     */
    std::shared_ptr<CameraManagerCallback> GetApplicationCallback();

    /**
     * @brief Get cameraDevice of specific camera id.
     *
     * @param std::string camera id.
     * @return Returns pointer to cameraDevice of given Id if found else return nullptr.
     */
    sptr<CameraDevice> GetCameraDeviceFromId(std::string cameraId);

    /**
     * @brief Get cameraInfo of specific camera id.
     *
     * @param std::string camera id.
     * @return Returns pointer to cameraInfo of given Id if found else return nullptr.
     */
    [[deprecated]] sptr<CameraInfo> GetCameraInfo(std::string cameraId);

    /**
     * @brief Get the support of camera mute mode.
     *
     * @return Returns true is supported, false is not supported.
     */
    bool IsCameraMuteSupported();

    /**
     * @brief Get camera mute mode.
     *
     * @return Returns true is in mute, else is not in mute.
     */
    bool IsCameraMuted();

    /**
     * @brief Mute the camera
     *
     * @return.
     */
    void MuteCamera(bool muteMode);

    /**
     * @brief register camera mute listener
     *
     * @param CameraMuteListener listener object.
     * @return.
     */
    void RegisterCameraMuteListener(std::shared_ptr<CameraMuteListener> listener);

    /**
     * @brief get the camera mute listener
     *
     * @return CameraMuteListener point..
     */
    std::shared_ptr<CameraMuteListener> GetCameraMuteListener();

    /**
     * @brief prelaunch the camera
     *
     * @return Server error code.
     */
    int32_t PrelaunchCamera();

    /**
     * @brief Pre-switch camera
     *
     * @return Server error code.
     */
    int32_t PreSwitchCamera(const std::string cameraId);

    /**
     * @brief set prelaunch config
     *
     * @return.
     */
    int32_t SetPrelaunchConfig(std::string cameraId, RestoreParamTypeOhos restoreParamType, int activeTime,
        EffectParam effectParam);

    /**
     * @brief Get the support of camera pre launch mode.
     *
     * @return Returns true is supported, false is not supported.
     */
    bool IsPrelaunchSupported(sptr<CameraDevice> camera);

    /**
     * @brief register torch listener
     *
     * @param TorchListener listener object.
     * @return.
     */
    void RegisterTorchListener(std::shared_ptr<TorchListener> listener);

    /**
     * @brief get the camera mute listener
     *
     * @return TorchListener point..
     */
    std::shared_ptr<TorchListener> GetTorchListener();

    SafeMap<std::thread::id, std::shared_ptr<CameraManagerCallback>> GetCameraMngrCallbackMap();
    SafeMap<std::thread::id, std::shared_ptr<CameraMuteListener>> GetCameraMuteListenerMap();
    SafeMap<std::thread::id, std::shared_ptr<TorchListener>> GetTorchListenerMap();
    /**
     * @brief check device if support torch
     *
     * @return Returns true is supported, false is not supported.
     */
    bool IsTorchSupported();

    /**
     * @brief check mode if device can support
     *
     * @return Returns true is supported, false is not supported.
     */
    bool IsTorchModeSupported(TorchMode mode);

    /**
     * @brief get current torchmode
     *
     * @return Returns current torchmode
     */
    TorchMode GetTorchMode();

    /**
     * @brief set torch mode
     *
     * @return.
     */
    int32_t SetTorchMode(TorchMode mode);

    /**
     * @brief update torch mode
     *
     */
    void UpdateTorchMode(TorchMode mode);

    /**
    * @brief Initializes the camera list.
    *
    */
    void InitCameraList();

    int32_t CreatePreviewOutputStream(
        sptr<IStreamRepeat>& streamPtr, Profile& profile, const sptr<OHOS::IBufferProducer>& producer);

    int32_t CreateVideoOutputStream(
        sptr<IStreamRepeat>& streamPtr, Profile& profile, const sptr<OHOS::IBufferProducer>& producer);

    int32_t CreatePhotoOutputStream(
        sptr<IStreamCapture>& streamPtr, Profile& profile, const sptr<OHOS::IBufferProducer>& producer);
    /**
    * @brief clear remote stub obj.
    *
    */
    int32_t DestroyStubObj();

    static const std::string surfaceFormat;

protected:
    // Only for UT
    explicit CameraManager(sptr<ICameraService> serviceProxy) : serviceProxyPrivate_(serviceProxy)
    {
        // Construct method add mutex lock is not necessary. Ignore g_instanceMutex.
        CameraManager::g_cameraManager = this;
        InitCameraList();
    }

private:
    explicit CameraManager();
    void InitCameraManager();
    void SetCameraServiceCallback(sptr<ICameraServiceCallback>& callback);
    void SetCameraMuteServiceCallback(sptr<ICameraMuteServiceCallback>& callback);
    void SetTorchServiceCallback(sptr<ITorchServiceCallback>& callback);
    void CreateAndSetCameraServiceCallback();
    void CreateAndSetCameraMuteServiceCallback();
    void CreateAndSetTorchServiceCallback();

    sptr<CaptureSession> CreateCaptureSessionImpl(SceneMode mode, sptr<ICaptureSession> session);
    int32_t CreateListenerObject(sptr<ICameraService>& serviceProxy);
    void CameraServerDied(pid_t pid);

    int32_t CreateCameraServerDeathRecipient(sptr<ICameraService>& serviceProxy);
    void RemoveCameraServerDeathRecipient();

    void CreateProfile4StreamType(OutputCapStreamType streamType, uint32_t modeIndex,
        uint32_t streamIndex, ExtendInfo extendInfo);
    static const std::unordered_map<camera_format_t, CameraFormat> metaToFwCameraFormat_;
    static const std::unordered_map<CameraFormat, camera_format_t> fwToMetaCameraFormat_;
    void ParseExtendCapability(const int32_t modeName, const camera_metadata_item_t& item);
    void ParseBasicCapability(
        std::shared_ptr<OHOS::Camera::CameraMetadata> metadata, const camera_metadata_item_t& item);
    void AlignVideoFpsProfile(std::vector<sptr<CameraDevice>>& cameraObjList);
    void SetProfile(sptr<CameraDevice>& cameraObj);
    SceneMode GetFallbackConfigMode(SceneMode profileMode);
    void ParseCapability(sptr<CameraDevice>& camera, const int32_t modeName, camera_metadata_item_t& item,
        std::shared_ptr<OHOS::Camera::CameraMetadata> metadata);
    std::recursive_mutex cameraListMutex_;
    std::mutex vectorMutex_;
    int CreateCameraDevice(std::string cameraId, sptr<ICameraDeviceService> *pICameraDeviceService);
    camera_format_t GetCameraMetadataFormat(CameraFormat format);
    bool GetDmDeviceInfo();
    bool isDistributeCamera(std::string cameraId, dmDeviceInfo& deviceInfo);
    int32_t SetTorchLevel(float level);

    int32_t ValidCreateOutputStream(Profile& profile, const sptr<OHOS::IBufferProducer>& producer);

    inline sptr<ICameraService> GetServiceProxy()
    {
        std::lock_guard<std::mutex> lock(serviceProxyMutex_);
        return serviceProxyPrivate_;
    }

    inline void SetServiceProxy(sptr<ICameraService> proxy)
    {
        std::lock_guard<std::mutex> lock(serviceProxyMutex_);
        serviceProxyPrivate_ = proxy;
    }

    std::mutex serviceProxyMutex_;
    sptr<ICameraService> serviceProxyPrivate_;

    std::mutex deathRecipientMutex_;
    sptr<CameraDeathRecipient> deathRecipient_ = nullptr;

    static sptr<CameraManager> g_cameraManager;
    static std::mutex g_instanceMutex;
    sptr<ICameraServiceCallback> cameraSvcCallback_;
    sptr<ICameraMuteServiceCallback> cameraMuteSvcCallback_;
    sptr<ITorchServiceCallback> torchSvcCallback_;

    SafeMap<std::thread::id, std::shared_ptr<CameraManagerCallback>> cameraMngrCallbackMap_;
    SafeMap<std::thread::id, std::shared_ptr<CameraMuteListener>> cameraMuteListenerMap_;
    SafeMap<std::thread::id, std::shared_ptr<TorchListener>> torchListenerMap_;

    std::vector<sptr<CameraDevice>> cameraObjList_ = {};
    std::vector<sptr<CameraInfo>> dcameraObjList_ = {};
    std::vector<dmDeviceInfo> distributedCamInfo_;
    std::map<std::string, dmDeviceInfo> distributedCamInfoAndId_;
    class DeviceInitCallBack;

    std::map<std::string, std::vector<Profile>> modePhotoProfiles_ = {};
    std::map<std::string, std::vector<Profile>> modePreviewProfiles_ = {};
    std::vector<Profile> photoProfiles_ = {};
    std::vector<Profile> previewProfiles_ = {};
    std::vector<VideoProfile> vidProfiles_ = {};
    sptr<CameraInput> cameraInput_;
    TorchMode torchMode_ = TorchMode::TORCH_MODE_OFF;
};

class CameraMuteServiceCallback : public HCameraMuteServiceCallbackStub {
public:
    explicit CameraMuteServiceCallback(sptr<CameraManager> cameraManager) : cameraManager_(cameraManager) {}
    int32_t OnCameraMute(bool muteMode) override;

private:
    wptr<CameraManager> cameraManager_ = nullptr;
};

class CameraStatusServiceCallback : public HCameraServiceCallbackStub {
public:
    explicit CameraStatusServiceCallback(sptr<CameraManager> cameraManager) : cameraManager_(cameraManager) {}
    int32_t OnCameraStatusChanged(const std::string& cameraId, const CameraStatus status) override;
    int32_t OnFlashlightStatusChanged(const std::string& cameraId, const FlashStatus status) override;

private:
    wptr<CameraManager> cameraManager_ = nullptr;
};

class TorchServiceCallback : public HTorchServiceCallbackStub {
public:
    explicit TorchServiceCallback(sptr<CameraManager> cameraManager) : cameraManager_(cameraManager) {}
    int32_t OnTorchStatusChange(const TorchStatus status) override;

private:
    wptr<CameraManager> cameraManager_ = nullptr;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_MANAGER_H
