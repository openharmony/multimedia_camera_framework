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

#ifndef OHOS_CAMERA_PHOTO_OUTPUT_H
#define OHOS_CAMERA_PHOTO_OUTPUT_H

#include <cstdint>
#include <iostream>
#include <mutex>

#include "camera_metadata_info.h"
#include "capture_output.h"
#include "hstream_capture_callback_stub.h"
#include "istream_capture.h"
#include "camera_photo_proxy.h"

namespace OHOS::Media {
    class Picture;
}
namespace OHOS {
namespace CameraStandard {

class PhotoStateCallback {
public:
    PhotoStateCallback() = default;
    virtual ~PhotoStateCallback() = default;

    /**
     * @brief Called when camera capture started.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     */
    virtual void OnCaptureStarted(const int32_t captureID) const = 0;

    /**
     * @brief Called when camera capture started.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     */
    virtual void OnCaptureStarted(const int32_t captureID, uint32_t exposureTime) const = 0;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     * @param frameCount Obtain the constant number of frames for the photo capture callback.
     */
    virtual void OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const = 0;

    /**
     * @brief Called when frame Shutter.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    virtual void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const = 0;

    /**
     * @brief Called when frame Shutter end.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    virtual void OnFrameShutterEnd(const int32_t captureId, const uint64_t timestamp) const = 0;

    /**
     * @brief Called when capture ready end.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    virtual void OnCaptureReady(const int32_t captureId, const uint64_t timestamp) const = 0;

    /**
     * @brief Called when EstimatedCaptureDuration.
     *
     * @param duration Obtain the duration for the photo capture callback.
     */
    virtual void OnEstimatedCaptureDuration(const int32_t duration) const = 0;

    /**
     * @brief Called when error occured during camera capture.
     *
     * @param captureId Indicates the pointer in which captureId will be requested.
     * @param errorCode Indicates a {@link ErrorCode} which will give information for photo capture callback error
     */
    virtual void OnCaptureError(const int32_t captureId, const int32_t errorCode) const = 0;
};

class [[deprecated]] PhotoCallback {
public:
    PhotoCallback() = default;
    virtual ~PhotoCallback() = default;

    /**
     * @brief Called when camera capture started.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     */
    virtual void OnCaptureStarted(const int32_t captureID) const = 0;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     * @param frameCount Obtain the constant number of frames for the photo capture callback.
     */
    virtual void OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const = 0;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    virtual void OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const = 0;

    /**
     * @brief Called when error occured during camera capture.
     *
     * @param captureId Indicates the pointer in which captureId will be requested.
     * @param errorCode Indicates a {@link ErrorCode} which will give information for photo capture callback error
     */
    virtual void OnCaptureError(const int32_t captureId, const int32_t errorCode) const = 0;
};

typedef struct Location {
    /**
     * Latitude.
     */
    double latitude = -1;
    /**
     * Longitude.
     */
    double longitude = -1;
    /**
     * Altitude.
     */
    double altitude = -1;
} Location;

class PhotoCaptureSetting {
public:
    enum QualityLevel {
        QUALITY_LEVEL_HIGH = 0,
        QUALITY_LEVEL_MEDIUM,
        QUALITY_LEVEL_LOW,
        HIGH_QUALITY [[deprecated]] = 0,
        NORMAL_QUALITY [[deprecated]],
        LOW_QUALITY [[deprecated]]
    };
    enum RotationConfig { Rotation_0 = 0, Rotation_90 = 90, Rotation_180 = 180, Rotation_270 = 270 };
    PhotoCaptureSetting();
    virtual ~PhotoCaptureSetting() = default;

    /**
     * @brief Get the quality level for the photo capture settings.
     *
     * @return Returns the quality level.
     */
    QualityLevel GetQuality();

    /**
     * @brief Set the quality level for the photo capture settings.
     *
     * @param qualityLevel to be set.
     */
    void SetQuality(QualityLevel qualityLevel);

    /**
     * @brief Get rotation information for the photo capture settings.
     *
     * @return Returns the RotationConfig.
     */
    RotationConfig GetRotation();

    /**
     * @brief Set the Rotation for the photo capture settings.
     *
     * @param rotationvalue to be set.
     */
    void SetRotation(RotationConfig rotationvalue);

    /**
     * @brief Set the GPS Location for the photo capture settings.
     *
     * @param latitude value to be set.
     * @param longitude value to be set.
     */
    void SetGpsLocation(double latitude, double longitude);

    /**
     * @brief Set the GPS Location for the photo capture settings.
     *
     * @param location value to be set.
     */
    void SetLocation(std::shared_ptr<Location>& location);

    /**
     * @brief Get the GPS Location for the photo capture settings.
     *
     * @param location value to be set.
     */
    void GetLocation(std::shared_ptr<Location>& location);

    /**
     * @brief Set the mirror option for the photo capture.
     *
     * @param boolean true/false to set/unset mirror respectively.
     */
    void SetMirror(bool enable);

    /**
     * @brief Get mirror information for the photo capture settings.
     *
     * @return Returns the mirror information.
     */
    bool GetMirror();

    /**
     * @brief Set burst capture state.
     */
    void SetBurstCaptureState(uint8_t burstState);

    /**
     * @brief Get the photo capture settings metadata information.
     *
     * @return Returns the pointer where CameraMetadata information is present.
     */
    std::shared_ptr<OHOS::Camera::CameraMetadata> GetCaptureMetadataSetting();

private:
    std::shared_ptr<OHOS::Camera::CameraMetadata> captureMetadataSetting_;
    std::shared_ptr<Location> location_;
    std::mutex locationMutex_;
};

constexpr uint8_t CAPTURE_PHOTO = 1 << 0;
constexpr uint8_t CAPTURE_DEFERRED_PHOTO = 1 << 1;
constexpr uint8_t CAPTURE_PHOTO_ASSET = 1 << 2;
constexpr int32_t CAPTURE_ROTATION_BASE = 360;

class PhotoOutput : public CaptureOutput {
public:
    explicit PhotoOutput(sptr<IBufferProducer> bufferProducer);
    virtual ~PhotoOutput();

    /**
     * @brief Set the photo callback.
     *
     * @param callback Requested for the pointer where photo callback is present.
     */
    void SetCallback(std::shared_ptr<PhotoStateCallback> callback);

    /**
     * @brief Set the thumbnail callback.
     *
     * @param listener set IBufferConsumerListener when on interface is called.
     */
    void SetThumbnailListener(sptr<IBufferConsumerListener>& listener);

    /**
     * @brief Get the photo rotation.
     *
     * @return result of the photo rotation angle.
     */
    int32_t GetPhotoRotation(int32_t imageRotation);

    /**
     * @brief Set the Thumbnail profile.
     *
     * @param isEnabled quickThumbnail is enabled.
     */
    int32_t SetThumbnail(bool isEnabled);

    /**
     * @brief To enable the raw imgage delivery.
     *
     * @return Returns the result of the raw imgage delivery enable.
     */
    int32_t EnableRawDelivery(bool enabled);

    /**
     * @brief Set the Thumbnail profile.
     *
     * @param isEnabled quickThumbnail is enabled.
     */
    int32_t SetRawPhotoInfo(sptr<Surface>& surface);

    /**
     * @brief Set the photo callback.
     *
     * @param callback Requested for the pointer where photo callback is present.
     */
    [[deprecated]] void SetCallback(std::shared_ptr<PhotoCallback> callback);

    /**
     * @brief Photo capture request using photocapturesettings.
     *
     * @param photoCaptureSettings Requested for the photoCaptureSettings object which has metadata
     * information such as: Rotation, Location, Mirror & Quality.
     */
    int32_t Capture(std::shared_ptr<PhotoCaptureSetting> photoCaptureSettings);

    /**
     * @brief Initiate for the photo capture.
     */
    int32_t Capture();

    /**
     * @brief cancelling the photo capture. Applicable only for burst/ continuous capture.
     */
    int32_t CancelCapture();

    /**
     * @brief cancelling the photo capture. Applicable only for burst/ continuous capture.
     */
    int32_t ConfirmCapture();

    int32_t CreateStream() override;

    /**
     * @brief Releases the instance of PhotoOutput.
     */
    int32_t Release() override;

    /**
     * @brief Get the application callback information.
     *
     * @return Returns the pointer to PhotoStateCallback.
     */
    std::shared_ptr<PhotoStateCallback> GetApplicationCallback();

    /**
     * @brief To check the photo capture is mirrored or not.
     *
     * @return Returns true/false if the photo capture is mirrored/not-mirrored respectively.
     */
    bool IsMirrorSupported();

    /**
     * @brief To check the quick thumbnail is supported or not.
     *
     * @return Returns true/false if the quick thumbnail is supported/not-supported respectively.
     */
    int32_t IsQuickThumbnailSupported();

    /**
     * @brief To check the raw image devlivery is supported or not.
     *
     * @return Returns true/false if the raw image devlivery is supported/not-supported respectively.
     */
    int32_t IsRawDeliverySupported();

    /**
     * @brief Set the deferredImageDelivery type.
     *
     */
    int32_t DeferImageDeliveryFor(DeferredDeliveryImageType type);

    /**
     * @brief To check the deferredImageDelivery capability is supported or not.
     *
     * @return Returns true/false if the deferredImageDelivery is supported/not-supported respectively.
     */
    int32_t IsDeferredImageDeliverySupported(DeferredDeliveryImageType type);

    /**
     * @brief Set the callbackFlag when on photoAssetAvailable.
     */
    void SetCallbackFlag(uint8_t callbackFlag);

    /**
     * @brief Set the flag when on native surface.
     */
    void SetNativeSurface(bool isNativeSurface);

    /**
     * @brief To check the deferredImageDelivery capability is enable or not.
     *
     * @return Returns true/false if the deferredImageDelivery is enable/not-enable respectively.
     */
    int32_t IsDeferredImageDeliveryEnabled(DeferredDeliveryImageType type);

    void ProcessSnapshotDurationUpdates(int32_t snapshotDuration);

    /**
     * @brief To check the auto high quality photo is supported or not.
     *
     * @return Returns true/false if the auto high quality photo is supported/not-supported respectively.
     */
    int32_t IsAutoHighQualityPhotoSupported(int32_t& isAutoHighQualityPhotoSupported);

    /**
     * @brief To enable the auto high quality photo.
     *
     * @return Returns the result of the auto high quality photo enable.
     */
    int32_t EnableAutoHighQualityPhoto(bool enabled);

    /**
     * @brief To get status by callbackFlags.
     *
     * @return Returns the result to check enable deferred.
     */
    bool IsEnableDeferred();

    /**
     * @brief Get default photo capture setting.
     *
     * @return default photo capture setting.
     */
    std::shared_ptr<PhotoCaptureSetting> GetDefaultCaptureSetting();
    int32_t SetMovingPhotoVideoCodecType(int32_t videoCodecType);
    sptr<Surface> thumbnailSurface_;

    sptr<Surface> rawPhotoSurface_;

    sptr<Surface> deferredSurface_;

    sptr<Surface> gainmapSurface_;
    sptr<Surface> deepSurface_;
    sptr<Surface> exifSurface_;
    sptr<Surface> debugSurface_;
    sptr<SurfaceBuffer> gainmapSurfaceBuffer_;
    sptr<SurfaceBuffer> deepSurfaceBuffer_;
    sptr<SurfaceBuffer> exifSurfaceBuffer_;
    sptr<SurfaceBuffer> debugSurfaceBuffer_;
    bool IsYuvOrHeifPhoto();
    void CreateMultiChannel();

    void SetAuxiliaryPhotoHandle(uint32_t handle);
    uint32_t GetAuxiliaryPhotoHandle();
    std::unique_ptr<Media::Picture> picture_;
    sptr<CameraPhotoProxy> photoProxy_;
    uint32_t watchDogHandle_;
    std::mutex watchDogHandleMutex_;
    std::map<int32_t, int32_t> caputreIdAuxiliaryCountMap_;
    std::map<int32_t, int32_t> caputreIdCountMap_;
private:
    std::mutex callbackMutex_;
    uint8_t callbackFlag_ = CAPTURE_DEFERRED_PHOTO;
    bool isNativeSurface_ = false;
    DeferredDeliveryImageType deferredType_ = DeferredDeliveryImageType::DELIVERY_NONE;
    std::shared_ptr<PhotoStateCallback> appCallback_;
    sptr<IStreamCaptureCallback> cameraSvcCallback_;
    std::shared_ptr<PhotoCaptureSetting> defaultCaptureSetting_;
    void CameraServerDied(pid_t pid) override;
};

class HStreamCaptureCallbackImpl : public HStreamCaptureCallbackStub {
public:
    explicit HStreamCaptureCallbackImpl(PhotoOutput* photoOutput) : innerPhotoOutput_(photoOutput) {}

    ~HStreamCaptureCallbackImpl() = default;

    /**
     * @brief Called when camera capture started.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     */
    int32_t OnCaptureStarted(const int32_t captureId) override;

    /**
     * @brief Called when camera capture started.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     */
    int32_t OnCaptureStarted(const int32_t captureId, uint32_t exposureTime) override;
    /**
     * @brief Called when camera capture ended.
     *
     * @param captureID Obtain the constant capture id for the photo capture callback.
     * @param frameCount Obtain the constant number of frames for the photo capture callback.
     */
    int32_t OnCaptureEnded(const int32_t captureId, const int32_t frameCount) override;

    /**
     * @brief Called when error occured during camera capture.
     *
     * @param captureId Indicates the pointer in which captureId will be requested.
     * @param errorCode Indicates a {@link ErrorCode} which will give information for photo capture callback error
     */
    int32_t OnCaptureError(const int32_t captureId, const int32_t errorCode) override;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    int32_t OnFrameShutter(const int32_t captureId, const uint64_t timestamp) override;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    int32_t OnFrameShutterEnd(const int32_t captureId, const uint64_t timestamp) override;

    /**
     * @brief Called when camera capture ended.
     *
     * @param captureId Obtain the constant capture id for the photo capture callback.
     * @param timestamp Represents timestamp information for the photo capture callback
     */
    int32_t OnCaptureReady(const int32_t captureId, const uint64_t timestamp) override;

    inline sptr<PhotoOutput> GetPhotoOutput()
    {
        if (innerPhotoOutput_ == nullptr) {
            return nullptr;
        }
        return innerPhotoOutput_.promote();
    }

private:
    wptr<PhotoOutput> innerPhotoOutput_ = nullptr;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_PHOTO_OUTPUT_H
