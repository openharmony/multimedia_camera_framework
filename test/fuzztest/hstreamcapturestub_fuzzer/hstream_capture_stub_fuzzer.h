 /*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef CAMERA_INPUT_FUZZER_H
#define CAMERA_INPUT_FUZZER_H

#include "hstream_capture_stub.h"

namespace OHOS {
namespace CameraStandard {
class PictureIntf;
class HStreamCaptureStubFuzz : public HStreamCaptureStub {
public:
    int32_t Capture(const std::shared_ptr<OHOS::Camera::CameraMetadata> &captureSettings) override
    {
        return 0;
    }
    int32_t CancelCapture() override
    {
        return 0;
    }
    int32_t ConfirmCapture() override
    {
        return 0;
    }
    int32_t SetCallback(sptr<IStreamCaptureCallback> &callback) override
    {
        return 0;
    }
    int32_t UnSetCallback() override
    {
        return 0;
    }
    int32_t Release() override
    {
        return 0;
    }
    int32_t SetThumbnail(bool isEnabled, const sptr<OHOS::IBufferProducer> &producer) override
    {
        return 0;
    }
    int32_t EnableRawDelivery(bool enabled) override
    {
        return 0;
    }
    int32_t EnableMovingPhoto(bool enabled) override
    {
        return 0;
    }
    int32_t SetBufferProducerInfo(const std::string bufName, const sptr<OHOS::IBufferProducer> &producer) override
    {
        return 0;
    }
    int32_t DeferImageDeliveryFor(int32_t type) override
    {
        return 0;
    }
    int32_t IsDeferredPhotoEnabled() override
    {
        return 0;
    }
    int32_t IsDeferredVideoEnabled() override
    {
        return 0;
    }
    int32_t SetMovingPhotoVideoCodecType(int32_t videoCodecType) override
    {
        return 0;
    }
    int32_t SetCameraPhotoRotation(bool isEnable) override
    {
        return 0;
    }
    int32_t UpdateMediaLibraryPhotoAssetProxy(sptr<CameraPhotoProxy> photoProxy) override
    {
        return 0;
    }
    int32_t AcquireBufferToPrepareProxy(int32_t captureId) override
    {
        return 0;
    }
    int32_t OperatePermissionCheck(uint32_t interfaceCode) override
    {
        return 0;
    }

    int32_t EnableOfflinePhoto(bool isEnable)override
    {
        return 0;
    }
    
    int32_t CreateMediaLibrary(sptr<CameraPhotoProxy> &photoProxy,
        std::string &uri, int32_t &cameraShotType, std::string &burstKey, int64_t timestamp) override
    {
        return 0;
    }
    int32_t CreateMediaLibrary(std::shared_ptr<PictureIntf> picture, sptr<CameraPhotoProxy> &photoProxy,
        std::string &uri, int32_t &cameraShotType, std::string &burstKey, int64_t timestamp) override
    {
        return 0;
    }
};
class HStreamCaptureStubFuzzer {
public:
static std::shared_ptr<HStreamCaptureStubFuzz> fuzz_;
static void HStreamCaptureStubFuzzTest1();
static void HStreamCaptureStubFuzzTest2();
static void HStreamCaptureStubFuzzTest3();
static void HStreamCaptureStubFuzzTest4();
static void HStreamCaptureStubFuzzTest5();
static void HStreamCaptureStubFuzzTest6();
static void HStreamCaptureStubFuzzTest7();
static void HStreamCaptureStubFuzzTest8();
static void HStreamCaptureStubFuzzTest9();
static void HStreamCaptureStubFuzzTest10();
};
} //CameraStandard
} //OHOS
#endif //CAMERA_INPUT_FUZZER_H