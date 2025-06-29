/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef PHOTO_OUTPUT_NAPI_H_
#define PHOTO_OUTPUT_NAPI_H_

#include <cstdint>
#include <memory>
#include <mutex>

#include "camera_napi_event_emitter.h"
#include "camera_napi_template_utils.h"
#include "input/camera_device.h"
#include "input/camera_manager.h"
#include "listener_base.h"
#include "native_image.h"
#include "output/camera_output_capability.h"
#include "output/photo_output.h"
#include "pixel_map.h"

namespace OHOS::Media {
    class PixelMap;
}
namespace OHOS {
namespace CameraStandard {
class PictureIntf;
const std::string dataWidth = "dataWidth";
const std::string dataHeight = "dataHeight";
static const std::string CONST_CAPTURE_START = "captureStart";
static const std::string CONST_CAPTURE_END = "captureEnd";
static const std::string CONST_CAPTURE_FRAME_SHUTTER = "frameShutter";
static const std::string CONST_CAPTURE_ERROR = "error";
static const std::string CONST_CAPTURE_PHOTO_AVAILABLE = "photoAvailable";
static const std::string CONST_CAPTURE_DEFERRED_PHOTO_AVAILABLE = "deferredPhotoProxyAvailable";
static const std::string CONST_CAPTURE_PHOTO_ASSET_AVAILABLE = "photoAssetAvailable";
static const std::string CONST_CAPTURE_FRAME_SHUTTER_END = "frameShutterEnd";
static const std::string CONST_CAPTURE_READY = "captureReady";
static const std::string CONST_CAPTURE_ESTIMATED_CAPTURE_DURATION = "estimatedCaptureDuration";
static const std::string CONST_CAPTURE_START_WITH_INFO = "captureStartWithInfo";

static const std::string CONST_CAPTURE_QUICK_THUMBNAIL = "quickThumbnail";
static const char CAMERA_PHOTO_OUTPUT_NAPI_CLASS_NAME[] = "PhotoOutput";

static const std::string CONST_GAINMAP_SURFACE = "gainmap";
static const std::string CONST_DEEP_SURFACE = "deep";
static const std::string CONST_EXIF_SURFACE = "exif";
static const std::string CONST_DEBUG_SURFACE = "debug";
static const std::string CONST_CAPTURE_OFFLINE_DELIVERY_FINISHED = "offlineDeliveryFinished";

struct CallbackInfo {
    int32_t captureID;
    uint64_t timestamp = 0;
    int32_t frameCount = 0;
    int32_t errorCode;
    int32_t duration;
};

enum PhotoOutputEventType {
    CAPTURE_START,
    CAPTURE_END,
    CAPTURE_FRAME_SHUTTER,
    CAPTURE_FRAME_SHUTTER_END,
    CAPTURE_READY,
    CAPTURE_ERROR,
    CAPTURE_INVALID_TYPE,
    CAPTURE_PHOTO_AVAILABLE,
    CAPTURE_DEFERRED_PHOTO_AVAILABLE,
    CAPTURE_PHOTO_ASSET_AVAILABLE,
    CAPTURE_ESTIMATED_CAPTURE_DURATION,
    CAPTURE_START_WITH_INFO,
    CAPTURE_OFFLINE_DELIVERY_FINISHED
};

static EnumHelper<PhotoOutputEventType> PhotoOutputEventTypeHelper({
        {CAPTURE_START, CONST_CAPTURE_START},
        {CAPTURE_END, CONST_CAPTURE_END},
        {CAPTURE_FRAME_SHUTTER, CONST_CAPTURE_FRAME_SHUTTER},
        {CAPTURE_ERROR, CONST_CAPTURE_ERROR},
        {CAPTURE_PHOTO_AVAILABLE, CONST_CAPTURE_PHOTO_AVAILABLE},
        {CAPTURE_DEFERRED_PHOTO_AVAILABLE, CONST_CAPTURE_DEFERRED_PHOTO_AVAILABLE},
        {CAPTURE_PHOTO_ASSET_AVAILABLE, CONST_CAPTURE_PHOTO_ASSET_AVAILABLE},
        {CAPTURE_FRAME_SHUTTER_END, CONST_CAPTURE_FRAME_SHUTTER_END},
        {CAPTURE_READY, CONST_CAPTURE_READY},
        {CAPTURE_ESTIMATED_CAPTURE_DURATION, CONST_CAPTURE_ESTIMATED_CAPTURE_DURATION},
        {CAPTURE_START_WITH_INFO, CONST_CAPTURE_START_WITH_INFO},
        {CAPTURE_OFFLINE_DELIVERY_FINISHED, CONST_CAPTURE_OFFLINE_DELIVERY_FINISHED}
    },
    PhotoOutputEventType::CAPTURE_INVALID_TYPE
);
enum SurfaceType {
    GAINMAP_SURFACE = 0,
    DEEP_SURFACE = 1,
    EXIF_SURFACE = 2,
    DEBUG_SURFACE = 3,
    INVALID_SURFACE = -1,
};

static EnumHelper<SurfaceType> SurfaceTypeHelper({
        {GAINMAP_SURFACE, CONST_GAINMAP_SURFACE},
        {DEEP_SURFACE, CONST_DEEP_SURFACE},
        {EXIF_SURFACE, CONST_EXIF_SURFACE},
        {DEBUG_SURFACE, CONST_DEBUG_SURFACE},
    },
    SurfaceType::INVALID_SURFACE
);

class PhotoBufferProcessor : public Media::IBufferProcessor {
public:
    explicit PhotoBufferProcessor(sptr<Surface> photoSurface) : photoSurface_(photoSurface) {}
    ~PhotoBufferProcessor()
    {
        photoSurface_ = nullptr;
    }
    void BufferRelease(sptr<SurfaceBuffer>& buffer) override
    {
        if (photoSurface_ != nullptr) {
            photoSurface_->ReleaseBuffer(buffer, -1);
        }
    }

private:
    sptr<Surface> photoSurface_ = nullptr;
};

class PhotoListener : public IBufferConsumerListener,
                      public ListenerBase,
                      public std::enable_shared_from_this<PhotoListener> {
public:
    explicit PhotoListener(napi_env env, const sptr<Surface> photoSurface, wptr<PhotoOutput> photoOutput);
    virtual ~PhotoListener();
    void OnBufferAvailable() override;
    void SaveCallback(const std::string eventName, napi_value callback);
    void RemoveCallback(const std::string eventName, napi_value callback);
    void ExecuteDeepCopySurfaceBuffer();

    void ClearTaskManager();
    std::shared_ptr<DeferredProcessing::TaskManager> GetDefaultTaskManager();

private:

    void UpdateJSCallback(sptr<Surface> photoSurface) const;
    void UpdateJSCallbackAsync(sptr<Surface> photoSurface) const;
    void UpdatePictureJSCallback(int32_t captureId, const string uri, int32_t cameraShotType,
        const std::string burstKey) const;
    void UpdateMainPictureStageOneJSCallback(sptr<SurfaceBuffer> surfaceBuffer, int64_t timestamp) const;
    void ExecutePhoto(sptr<SurfaceBuffer> surfaceBfuffer, int64_t timestamp) const;
    void ExecuteDeferredPhoto(sptr<SurfaceBuffer> surfaceBuffer) const;
    void DeepCopyBuffer(sptr<SurfaceBuffer> newSurfaceBuffer, sptr<SurfaceBuffer> surfaceBuffer,
        int32_t captureId) const;
    void ExecutePhotoAsset(sptr<SurfaceBuffer> surfaceBuffer, bool isHighQuality, int64_t timestamp) const;
    void CreateMediaLibrary(sptr<SurfaceBuffer> surfaceBuffer, BufferHandle* bufferHandle, bool isHighQuality,
        std::string& uri, int32_t& cameraShotType, std::string &burstKey, int64_t timestamp) const;
    void AssembleAuxiliaryPhoto(int64_t timestamp, int32_t captureId);
    void ProcessAuxiliaryPhoto(int64_t timestamp, sptr<PhotoOutput> photoOutput, int32_t captureId);
    int32_t GetAuxiliaryPhotoCount(sptr<SurfaceBuffer> surfaceBuffer);
    sptr<Surface> photoSurface_;
    wptr<PhotoOutput> photoOutput_;
    shared_ptr<PhotoBufferProcessor> bufferProcessor_;
    uint8_t callbackFlag_ = 0;
    std::mutex taskManagerMutex_;
    std::shared_ptr<DeferredProcessing::TaskManager> taskManager_ = nullptr;
};

class RawPhotoListener : public IBufferConsumerListener, public ListenerBase,
    public std::enable_shared_from_this<RawPhotoListener> {
public:
    explicit RawPhotoListener(napi_env env, const sptr<Surface> rawPhotoSurface);
    ~RawPhotoListener() = default;
    void OnBufferAvailable() override;

private:
    sptr<Surface> rawPhotoSurface_;
    shared_ptr<PhotoBufferProcessor> bufferProcessor_;
    void UpdateJSCallback(sptr<Surface> rawPhotoSurface) const;
    void UpdateJSCallbackAsync(sptr<Surface> rawPhotoSurface) const;
    void ExecuteRawPhoto(sptr<SurfaceBuffer> rawPhotoSurface) const;
};

class AuxiliaryPhotoListener : public IBufferConsumerListener {
public:
    explicit AuxiliaryPhotoListener(const std::string surfaceName, const sptr<Surface> surface,
        wptr<PhotoOutput> photoOutput);
    ~AuxiliaryPhotoListener() = default;
    void OnBufferAvailable() override;
    void ExecuteDeepCopySurfaceBuffer();
private:
    void DeepCopyBuffer(sptr<SurfaceBuffer> newSurfaceBuffer, sptr<SurfaceBuffer> surfaceBuffer, int32_t) const;
    std::string surfaceName_;
    sptr<Surface> surface_;
    wptr<PhotoOutput> photoOutput_;
    shared_ptr<PhotoBufferProcessor> bufferProcessor_;
};

class PictureListener : public RefBase {
public:
    explicit PictureListener() = default;
    ~PictureListener() = default;
    void InitPictureListeners(napi_env env, wptr<PhotoOutput> photoOutput);
    sptr<AuxiliaryPhotoListener> gainmapImageListener;
    sptr<AuxiliaryPhotoListener> deepImageListener;
    sptr<AuxiliaryPhotoListener> exifImageListener;
    sptr<AuxiliaryPhotoListener> debugImageListener;
};

class PhotoOutputCallback : public PhotoStateCallback,
                            public ListenerBase,
                            public std::enable_shared_from_this<PhotoOutputCallback> {
public:
    explicit PhotoOutputCallback(napi_env env);
    ~PhotoOutputCallback() = default;

    void OnCaptureStarted(const int32_t captureID) const override;
    void OnCaptureStarted(const int32_t captureID, uint32_t exposureTime) const override;
    void OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const override;
    void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const override;
    void OnFrameShutterEnd(const int32_t captureId, const uint64_t timestamp) const override;
    void OnCaptureReady(const int32_t captureId, const uint64_t timestamp) const override;
    void OnCaptureError(const int32_t captureId, const int32_t errorCode) const override;
    void OnEstimatedCaptureDuration(const int32_t duration) const override;
    void OnOfflineDeliveryFinished(const int32_t captureId) const override;

private:
    void UpdateJSCallback(PhotoOutputEventType eventType, const CallbackInfo& info) const;
    void UpdateJSCallbackAsync(PhotoOutputEventType eventType, const CallbackInfo& info) const;
    void ExecuteCaptureStartCb(const CallbackInfo& info) const;
    void ExecuteCaptureStartWithInfoCb(const CallbackInfo& info) const;
    void ExecuteCaptureEndCb(const CallbackInfo& info) const;
    void ExecuteFrameShutterCb(const CallbackInfo& info) const;
    void ExecuteCaptureErrorCb(const CallbackInfo& info) const;
    void ExecuteFrameShutterEndCb(const CallbackInfo& info) const;
    void ExecuteCaptureReadyCb(const CallbackInfo& info) const;
    void ExecuteEstimatedCaptureDurationCb(const CallbackInfo& info) const;
    void ExecuteOfflineDeliveryFinishedCb(const CallbackInfo& info) const;
};

struct PhotoOutputCallbackInfo {
    PhotoOutputEventType eventType_;
    CallbackInfo info_;
    weak_ptr<const PhotoOutputCallback> listener_;
    PhotoOutputCallbackInfo(
        PhotoOutputEventType eventType, CallbackInfo info, shared_ptr<const PhotoOutputCallback> listener)
        : eventType_(eventType), info_(info), listener_(listener)
    {}
};

class ThumbnailListener : public IBufferConsumerListener, public ListenerBase {
public:
    explicit ThumbnailListener(napi_env env, const sptr<PhotoOutput> photoOutput);
    virtual ~ThumbnailListener();
    void OnBufferAvailable() override;
    std::shared_ptr<DeferredProcessing::TaskManager> taskManager_ = nullptr;
    
    void ClearTaskManager();
    std::shared_ptr<DeferredProcessing::TaskManager> GetDefaultTaskManager();
private:
    wptr<PhotoOutput> photoOutput_;
    void UpdateJSCallback() const;
    void UpdateJSCallbackAsync();
    void UpdateJSCallback(int32_t captureId, int64_t timestamp, unique_ptr<Media::PixelMap>) const;
    void UpdateJSCallbackAsync(int32_t captureId, int64_t timestamp, unique_ptr<Media::PixelMap>);
    void ExecuteDeepCopySurfaceBuffer();
    
    unique_ptr<Media::PixelMap> CreatePixelMapFromSurfaceBuffer(sptr<SurfaceBuffer> &surfaceBuffer,
        int32_t width, int32_t height, bool isHdr);
    unique_ptr<Media::PixelMap> SetPixelMapYuvInfo(sptr<SurfaceBuffer> &surfaceBuffer,
        unique_ptr<Media::PixelMap> pixelMap, bool isHdr);
    void DeepCopyBuffer(sptr<SurfaceBuffer> newSurfaceBuffer, sptr<SurfaceBuffer> surfaceBuffer,
        int32_t thumbnailWidth, int32_t thumbnailHeight, bool isHdr) const;

    static constexpr int32_t PLANE_Y = 0;
    static constexpr int32_t PLANE_U = 1;
    static constexpr uint8_t PIXEL_SIZE_HDR_YUV = 3;
    static constexpr uint8_t  HDR_PIXEL_SIZE = 2;
    static constexpr uint8_t SDR_PIXEL_SIZE = 1;

    std::mutex taskManagerMutex_;
    int32_t recThumb_ = 0;
    int32_t acqThumb_ = 0;
};

struct ThumbnailListenerInfo {
    wptr<ThumbnailListener> listener_;
    int32_t captureId_;
    int64_t timestamp_;
    unique_ptr<Media::PixelMap> pixelMap_;
    ThumbnailListenerInfo(sptr<ThumbnailListener> listener, int32_t captureId,
        int64_t timestamp, unique_ptr<Media::PixelMap> pixelMap)
        : listener_(listener), captureId_(captureId), timestamp_(timestamp), pixelMap_(std::move(pixelMap))
    {}
};

struct PhotoListenerInfo {
    sptr<Surface> photoSurface_;
    weak_ptr<const PhotoListener> listener_;
    PhotoListenerInfo(sptr<Surface> photoSurface, shared_ptr<const PhotoListener> listener)
        : photoSurface_(photoSurface), listener_(listener)
    {}
    int32_t captureId = 0;
    std::string uri = "";
    int32_t cameraShotType = 0;
    std::string burstKey = "";
    sptr<SurfaceBuffer> surfaceBuffer = nullptr;
    int64_t timestamp = 0;
};

struct RawPhotoListenerInfo {
    sptr<Surface> rawPhotoSurface_;
    weak_ptr<const RawPhotoListener> listener_;
    RawPhotoListenerInfo(sptr<Surface> rawPhotoSurface, shared_ptr<const RawPhotoListener> listener)
        : rawPhotoSurface_(rawPhotoSurface), listener_(listener)
    {}
};

struct PhotoOutputAsyncContext;
class PhotoOutputNapi : public CameraNapiEventEmitter<PhotoOutputNapi> {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreatePhotoOutput(napi_env env, Profile& profile, std::string surfaceId);
    static napi_value CreatePhotoOutput(napi_env env, std::string surfaceId);

    static napi_value Capture(napi_env env, napi_callback_info info);
    static napi_value BurstCapture(napi_env env, napi_callback_info info);
    static napi_value ConfirmCapture(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);
    static napi_value IsMirrorSupported(napi_env env, napi_callback_info info);
    static napi_value EnableMirror(napi_env env, napi_callback_info info);
    static napi_value EnableQuickThumbnail(napi_env env, napi_callback_info info);
    static napi_value IsQuickThumbnailSupported(napi_env env, napi_callback_info info);
    static napi_value EnableRawDelivery(napi_env env, napi_callback_info info);
    static napi_value IsRawDeliverySupported(napi_env env, napi_callback_info info);
    static napi_value DeferImageDeliveryFor(napi_env env, napi_callback_info info);
    static napi_value IsDeferredImageDeliverySupported(napi_env env, napi_callback_info info);
    static napi_value IsDeferredImageDeliveryEnabled(napi_env env, napi_callback_info info);
    static napi_value GetSupportedMovingPhotoVideoCodecTypes(napi_env env, napi_callback_info info);
    static napi_value SetMovingPhotoVideoCodecType(napi_env env, napi_callback_info info);
    static napi_value IsDepthDataDeliverySupported(napi_env env, napi_callback_info info);
    static napi_value EnableDepthDataDelivery(napi_env env, napi_callback_info info);
    static bool IsPhotoOutput(napi_env env, napi_value obj);
    static napi_value GetActiveProfile(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value Once(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);
    static napi_value IsAutoHighQualityPhotoSupported(napi_env env, napi_callback_info info);
    static napi_value EnableAutoHighQualityPhoto(napi_env env, napi_callback_info info);
    static napi_value IsAutoCloudImageEnhancementSupported(napi_env env, napi_callback_info info);
    static napi_value EnableAutoCloudImageEnhancement(napi_env env, napi_callback_info info);
    static napi_value IsMovingPhotoSupported(napi_env env, napi_callback_info info);
    static napi_value EnableMovingPhoto(napi_env env, napi_callback_info info);
    static napi_value GetPhotoRotation(napi_env env, napi_callback_info info);
    static napi_value IsAutoAigcPhotoSupported(napi_env env, napi_callback_info info);
    static napi_value EnableAutoAigcPhoto(napi_env env, napi_callback_info info);
    static napi_value IsOfflineSupported(napi_env env, napi_callback_info info);
    static napi_value EnableOfflinePhoto(napi_env env, napi_callback_info info);

    PhotoOutputNapi();
    ~PhotoOutputNapi() override;

    sptr<PhotoOutput> GetPhotoOutput();
    bool GetEnableMirror();

    const EmitterFunctions& GetEmitterFunctions() override;

private:
    static void PhotoOutputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value PhotoOutputNapiConstructor(napi_env env, napi_callback_info info);

    void CreateMultiChannelPictureLisenter(napi_env env);
    void CreateSingleChannelPhotoLisenter(napi_env env);
    void RegisterQuickThumbnailCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterQuickThumbnailCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterPhotoAvailableCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterPhotoAvailableCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterDeferredPhotoProxyAvailableCallbackListener(const std::string& eventName, napi_env env,
        napi_value callback, const std::vector<napi_value>& args, bool isOnce);
    void UnregisterDeferredPhotoProxyAvailableCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterPhotoAssetAvailableCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterPhotoAssetAvailableCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterCaptureStartCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterCaptureStartCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterCaptureEndCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterCaptureEndCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterFrameShutterCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterFrameShutterCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterErrorCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterErrorCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterFrameShutterEndCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterFrameShutterEndCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterReadyCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterReadyCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterEstimatedCaptureDurationCallbackListener(const std::string& eventName, napi_env env,
        napi_value callback, const std::vector<napi_value>& args, bool isOnce);
    void UnregisterEstimatedCaptureDurationCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterCaptureStartWithInfoCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterCaptureStartWithInfoCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterOfflineDeliveryFinishedCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterOfflineDeliveryFinishedCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);

    static thread_local napi_ref sConstructor_;
    static thread_local sptr<PhotoOutput> sPhotoOutput_;

    sptr<PhotoOutput> photoOutput_;
    std::shared_ptr<Profile> profile_;
    bool isQuickThumbnailEnabled_ = false;
    bool isDeferredPhotoEnabled_ = false;
    bool isMirrorEnabled_ = false;
    sptr<ThumbnailListener> thumbnailListener_;
    sptr<PhotoListener> photoListener_;
    sptr<RawPhotoListener> rawPhotoListener_;
    sptr<PictureListener> pictureListener_;
    std::shared_ptr<PhotoOutputCallback> photoOutputCallback_;
    static thread_local uint32_t photoOutputTaskId;
    static thread_local napi_ref rawCallback_;
};

struct PhotoOutputNapiCaptureSetting {
    int32_t quality = -1;
};

struct PhotoOutputAsyncContext : public AsyncContext {
    PhotoOutputAsyncContext(std::string funcName, int32_t taskId) : AsyncContext(funcName, taskId) {};
    int32_t quality = -1;
    int32_t rotation = -1;
    bool isMirror = false;
    bool isMirrorSettedByUser = false;
    bool hasPhotoSettings = false;
    bool isSupported = false;
    shared_ptr<Location> location;
    PhotoOutputNapi* objectInfo = nullptr;
    std::string surfaceId;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* PHOTO_OUTPUT_NAPI_H_ */