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

#ifndef OHOS_CAMERA_ISTREAM_CAPTURE_CALLBACK_H
#define OHOS_CAMERA_ISTREAM_CAPTURE_CALLBACK_H

#include "iremote_broker.h"

namespace OHOS {
namespace CameraStandard {
class IStreamCaptureCallback : public IRemoteBroker {
public:
    virtual int32_t OnCaptureStarted(int32_t captureId) = 0;

    virtual int32_t OnCaptureStarted(int32_t captureId, uint32_t exposureTime) = 0;

    virtual int32_t OnCaptureEnded(int32_t captureId, int32_t frameCount) = 0;

    virtual int32_t OnCaptureError(int32_t captureId, int32_t errorType) = 0;

    virtual int32_t OnFrameShutter(int32_t captureId, uint64_t timestamp) = 0;

    virtual int32_t OnFrameShutterEnd(int32_t captureId, uint64_t timestamp) = 0;

    virtual int32_t OnCaptureReady(int32_t captureId, uint64_t timestamp) = 0;

    virtual int32_t OnOfflineDeliveryFinished(int32_t captureId) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamCaptureCallback");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_CAPTURE_CALLBACK_H

