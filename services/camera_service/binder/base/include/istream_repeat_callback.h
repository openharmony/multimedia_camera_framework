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

#ifndef OHOS_CAMERA_ISTREAM_REPEAT_CALLBACK_H
#define OHOS_CAMERA_ISTREAM_REPEAT_CALLBACK_H

#include <cstdint>
#include "iremote_broker.h"

namespace OHOS {
namespace CameraStandard {
enum class SketchStatus : int32_t {
    STOPED = 0,
    STARTED = 1,
    STOPPING = 2,
    STARTING = 3
};

typedef struct {
    SketchStatus status;
    float sketchRatio;
} SketchStatusData;

struct CaptureEndedInfoExt {
    int streamId;
    int frameCount;
    bool isDeferredVideoEnhancementAvailable;
    std::string videoId;
};

class IStreamRepeatCallback : public IRemoteBroker {
public:
    virtual int32_t OnFrameStarted() = 0;

    virtual int32_t OnFrameEnded(int32_t frameCount) = 0;

    virtual int32_t OnFrameError(int32_t errorCode) = 0;

    virtual int32_t OnSketchStatusChanged(SketchStatus status) = 0;

    virtual int32_t OnDeferredVideoEnhancementInfo(CaptureEndedInfoExt captureEndedInfo) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamRepeatCallback");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_REPEAT_CALLBACK_H
