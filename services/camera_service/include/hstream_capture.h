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

#ifndef OHOS_CAMERA_H_STREAM_CAPTURE_H
#define OHOS_CAMERA_H_STREAM_CAPTURE_H
#include <condition_variable>
#define EXPORT_API __attribute__((visibility("default")))

#include <atomic>
#include <cstdint>
#include <iostream>
#include <refbase.h>

#include "camera_metadata_info.h"
#include "stream_capture_stub.h"
#include "hstream_common.h"
#include "v1_0/istream_operator.h"
#include "v1_2/istream_operator.h"
#include "safe_map.h"
#include "icamera_ipc_checker.h"

namespace OHOS {
namespace CameraStandard {
using OHOS::HDI::Camera::V1_0::BufferProducerSequenceable;
using namespace OHOS::HDI::Camera::V1_0;
class PhotoAssetIntf;
class PictureIntf;
class CameraServerPhotoProxy;
class HStreamOperator;
class ConcurrentMap {
public:
    void Insert(const int32_t& key, const std::shared_ptr<PhotoAssetIntf>& value);
    std::shared_ptr<PhotoAssetIntf> Get(const int32_t& key);
    void Release();
    void Erase(const int32_t& key);
    std::mutex& GetMutex(const int32_t& key);
    std::condition_variable& GetCv(const int32_t& key);
    bool ReadyToUnlock(const int32_t& key, const int32_t& step, const int32_t& mode);
    void IncreaseCaptureStep(const int32_t& key);
private:
    std::map<int32_t, std::shared_ptr<PhotoAssetIntf>> map_;
    std::map<int32_t, std::mutex> mutexes_;
    std::map<int32_t, int32_t> step_;
    std::map<int32_t, std::condition_variable> cv_;
    std::mutex map_mutex_;
};
constexpr const char* BURST_UUID_UNSET = "";
class EXPORT_API HStreamCapture : public StreamCaptureStub, public HStreamCommon, public ICameraIpcChecker {
public:
    HStreamCapture(sptr<OHOS::IBufferProducer> producer, int32_t format, int32_t width, int32_t height);
    ~HStreamCapture();

    int32_t LinkInput(wptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator,
        std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility) override;
    void SetStreamInfo(StreamInfo_V1_1 &streamInfo) override;
    int32_t SetThumbnail(bool isEnabled, const sptr<OHOS::IBufferProducer> &producer) override;
    int32_t EnableRawDelivery(bool enabled) override;
    int32_t EnableMovingPhoto(bool enabled) override;
    int32_t SetBufferProducerInfo(const std::string& bufName, const sptr<OHOS::IBufferProducer> &producer) override;
    int32_t DeferImageDeliveryFor(int32_t type) override;
    int32_t Capture(const std::shared_ptr<OHOS::Camera::CameraMetadata> &captureSettings) override;
    int32_t CancelCapture() override;
    int32_t ConfirmCapture() override;
    int32_t ReleaseStream(bool isDelay) override;
    int32_t Release() override;
    int32_t SetCallback(const sptr<IStreamCaptureCallback> &callback) override;
    int32_t UnSetCallback() override;
    int32_t OnCaptureStarted(int32_t captureId);
    int32_t OnCaptureStarted(int32_t captureId, uint32_t exposureTime);
    int32_t OnCaptureEnded(int32_t captureId, int32_t frameCount);
    int32_t OnCaptureError(int32_t captureId, int32_t errorType);
    int32_t OnFrameShutter(int32_t captureId, uint64_t timestamp);
    int32_t OnFrameShutterEnd(int32_t captureId, uint64_t timestamp);
    int32_t OnCaptureReady(int32_t captureId, uint64_t timestamp);
    int32_t OnOfflineDeliveryFinished(int32_t captureId);
    void DumpStreamInfo(CameraInfoDumper& infoDumper) override;
    void SetRotation(const std::shared_ptr<OHOS::Camera::CameraMetadata> &captureMetadataSetting_, int32_t captureId);
    void SetMode(int32_t modeName);
    int32_t GetMode();
    int32_t IsDeferredPhotoEnabled() override;
    int32_t IsDeferredVideoEnabled() override;
    int32_t SetMovingPhotoVideoCodecType(int32_t videoCodecType) override;
    int32_t GetMovingPhotoVideoCodecType();
    int32_t OperatePermissionCheck(uint32_t interfaceCode) override;
    int32_t CallbackEnter([[maybe_unused]] uint32_t code) override;
    int32_t CallbackExit([[maybe_unused]] uint32_t code, [[maybe_unused]] int32_t result) override;
    SafeMap<int32_t, int32_t> rotationMap_ = {};
    bool IsBurstCapture(int32_t captureId) const;
    bool IsBurstCover(int32_t captureId) const;
    int32_t GetCurBurstSeq(int32_t captureId) const;
    std::string GetBurstKey(int32_t captureId) const;
    void SetBurstImages(int32_t captureId, std::string imageId);
    void CheckResetBurstKey(int32_t captureId);
    int32_t SetCameraPhotoRotation(bool isEnable) override;
    int32_t CreateMediaLibraryPhotoAssetProxy(int32_t captureId);
    int32_t UpdateMediaLibraryPhotoAssetProxy(const sptr<CameraPhotoProxy>& photoProxy) override;
    std::shared_ptr<PhotoAssetIntf> GetPhotoAssetInstance(int32_t captureId);
    bool GetAddPhotoProxyEnabled();
    int32_t AcquireBufferToPrepareProxy(int32_t captureId) override;
    int32_t EnableOfflinePhoto(bool isEnable) override;
    bool IsHasEnableOfflinePhoto();
    void SwitchToOffline();
    bool IsHasSwitchToOffline();
    void SetStreamOperator(wptr<HStreamOperator> hStreamOperator);
    int32_t CreateMediaLibrary(const sptr<CameraPhotoProxy>& photoProxy, std::string& uri, int32_t& cameraShotType,
        std::string& burstKey, int64_t timestamp) override;
    int32_t CreateMediaLibrary(const std::shared_ptr<PictureIntf>& picture, const sptr<CameraPhotoProxy> &photoProxy,
        std::string &uri, int32_t &cameraShotType, std::string& burstKey, int64_t timestamp) override;

    int32_t CallbackParcel(
        [[maybe_unused]] uint32_t code,
        [[maybe_unused]] MessageParcel& data,
        [[maybe_unused]] MessageParcel& reply,
        [[maybe_unused]] MessageOption& option) override;

private:
    int32_t CheckBurstCapture(const std::shared_ptr<OHOS::Camera::CameraMetadata>& captureSettings,
                              const int32_t &preparedCaptureId);
    int32_t PrepareBurst(int32_t captureId);
    void ResetBurst();
    void ResetBurstKey(int32_t captureId);
    void EndBurstCapture(const std::shared_ptr<OHOS::Camera::CameraMetadata>& captureMetadataSetting_);
    void ProcessCaptureInfoPhoto(CaptureInfo& captureInfoPhoto,
        const std::shared_ptr<OHOS::Camera::CameraMetadata>& captureSettings, int32_t captureId);
    void SetCameraPhotoProxyInfo(sptr<CameraServerPhotoProxy> cameraPhotoProxy);
    sptr<IStreamCaptureCallback> streamCaptureCallback_;
    void FillingPictureExtendStreamInfos(StreamInfo_V1_1 &streamInfo, int32_t format);
    void FillingRawAndThumbnailStreamInfo(StreamInfo_V1_1 &streamInfo);
    void UpdateJpegBasicInfo(const std::shared_ptr<OHOS::Camera::CameraMetadata> &captureMetadataSetting,
        int32_t& rotation);
    std::mutex callbackLock_;
    int32_t thumbnailSwitch_;
    int32_t rawDeliverySwitch_;
    int32_t movingPhotoSwitch_;
    std::condition_variable testDelay_;
    std::mutex testDelayMutex_;
    sptr<BufferProducerSequenceable> thumbnailBufferQueue_;
    sptr<BufferProducerSequenceable> rawBufferQueue_;
    sptr<BufferProducerSequenceable> gainmapBufferQueue_;
    sptr<BufferProducerSequenceable> deepBufferQueue_;
    sptr<BufferProducerSequenceable> exifBufferQueue_;
    sptr<BufferProducerSequenceable> debugBufferQueue_;
    int32_t modeName_;
    int32_t deferredPhotoSwitch_;
    int32_t deferredVideoSwitch_;
    bool enableCameraPhotoRotation_ = false;
    std::atomic<bool> isCaptureReady_ = true;
    std::string curBurstKey_ = BURST_UUID_UNSET;
    bool isBursting_ = false;
    std::map<int32_t, std::vector<std::string>> burstImagesMap_;
    std::map<int32_t, std::string> burstkeyMap_;
    std::map<int32_t, int32_t> burstNumMap_;
    mutable std::mutex burstLock_;
    int32_t burstNum_;
    int32_t videoCodecType_ = 0;
    std::mutex photoAssetLock_;
    ConcurrentMap photoAssetProxy_;
    bool mEnableOfflinePhoto_ = false;
    bool mSwitchToOfflinePhoto_ = false;
    int32_t mlastCaptureId = 0;
    wptr<HStreamOperator> hStreamOperator_;
    std::map<int32_t, std::unique_ptr<std::mutex>> mutexMap;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_H_STREAM_CAPTURE_H
