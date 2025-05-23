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

#ifndef OHOS_CAMERA_ISTREAM_CAPTURE_H
#define OHOS_CAMERA_ISTREAM_CAPTURE_H

#include <cstdint>
#include "camera_metadata_info.h"
#include "istream_capture_callback.h"
#include "istream_common.h"
#include "surface.h"

namespace OHOS {
namespace CameraStandard {
class CameraPhotoProxy;
class PictureIntf;
class IStreamCapture : public IStreamCommon {
public:
    virtual int32_t Capture(const std::shared_ptr<OHOS::Camera::CameraMetadata> &captureSettings) = 0;

    virtual int32_t CancelCapture() = 0;

    virtual int32_t ConfirmCapture() = 0;

    virtual int32_t SetCallback(sptr<IStreamCaptureCallback> &callback) = 0;

    virtual int32_t UnSetCallback() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t SetThumbnail(bool isEnabled, const sptr<OHOS::IBufferProducer> &producer) = 0;

    virtual int32_t EnableRawDelivery(bool enabled) = 0;

    virtual int32_t EnableMovingPhoto(bool enabled) = 0;

    virtual int32_t SetBufferProducerInfo(const std::string bufName, const sptr<OHOS::IBufferProducer> &producer) = 0;

    virtual int32_t DeferImageDeliveryFor(int32_t type) = 0;

    virtual int32_t IsDeferredPhotoEnabled() = 0;

    virtual int32_t IsDeferredVideoEnabled() = 0;

    virtual int32_t SetMovingPhotoVideoCodecType(int32_t videoCodecType) = 0;

    virtual int32_t SetCameraPhotoRotation(bool isEnable) = 0;

    virtual int32_t UpdateMediaLibraryPhotoAssetProxy(sptr<CameraPhotoProxy> photoProxy) = 0;

    virtual int32_t AcquireBufferToPrepareProxy(int32_t captureId) = 0;

    virtual int32_t EnableOfflinePhoto(bool isEnable) = 0;
    
    virtual int32_t CreateMediaLibrary(sptr<CameraPhotoProxy> &photoProxy,
        std::string &uri, int32_t &cameraShotType, std::string &burstKey, int64_t timestamp) = 0;

    virtual int32_t CreateMediaLibrary(std::shared_ptr<PictureIntf> picture, sptr<CameraPhotoProxy> &photoProxy,
        std::string &uri, int32_t &cameraShotType, std::string &burstKey, int64_t timestamp) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamCapture");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_CAPTURE_H
