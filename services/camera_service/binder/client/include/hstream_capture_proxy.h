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

#ifndef OHOS_CAMERA_HSTREAM_CAPTURE_PROXY_H
#define OHOS_CAMERA_HSTREAM_CAPTURE_PROXY_H

#include "istream_capture.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace CameraStandard {
class HStreamCaptureProxy : public IRemoteProxy<IStreamCapture> {
public:
    explicit HStreamCaptureProxy(const sptr<IRemoteObject> &impl);

    virtual ~HStreamCaptureProxy() = default;

    int32_t Capture(const std::shared_ptr<OHOS::Camera::CameraMetadata> &captureSettings) override;

    int32_t CancelCapture() override;

    int32_t ConfirmCapture() override;

    int32_t Release() override;

    int32_t SetCallback(sptr<IStreamCaptureCallback> &callback) override;

    int32_t UnSetCallback() override;

    int32_t SetThumbnail(bool isEnabled, const sptr<OHOS::IBufferProducer> &producer) override;

    int32_t EnableRawDelivery(bool enabled) override;

    int32_t EnableMovingPhoto(bool enabled) override;

    int32_t SetBufferProducerInfo(const std::string bufName, const sptr<OHOS::IBufferProducer> &producer) override;

    int32_t DeferImageDeliveryFor(int32_t type) override;

    int32_t IsDeferredPhotoEnabled() override;

    int32_t IsDeferredVideoEnabled() override;

    int32_t SetMovingPhotoVideoCodecType(int32_t videoCodecType) override;

    int32_t SetCameraPhotoRotation(bool isEnable) override;

    int32_t UpdateMediaLibraryPhotoAssetProxy(sptr<CameraPhotoProxy> photoProxy) override;

    int32_t AcquireBufferToPrepareProxy(int32_t captureId) override;

    int32_t EnableOfflinePhoto(bool isEnable) override;

    int32_t CreateMediaLibrary(sptr<CameraPhotoProxy> &photoProxy,
        std::string &uri, int32_t &cameraShotType, std::string &burstKey, int64_t timestamp) override;

    int32_t CreateMediaLibrary(std::shared_ptr<PictureIntf> picture, sptr<CameraPhotoProxy> &photoProxy,
        std::string &uri, int32_t &cameraShotType, std::string &burstKey, int64_t timestamp) override;

private:
    static inline BrokerDelegator<HStreamCaptureProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HSTREAM_CAPTURE_PROXY_H
