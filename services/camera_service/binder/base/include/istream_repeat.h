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

#ifndef OHOS_CAMERA_ISTREAM_REPEAT_H
#define OHOS_CAMERA_ISTREAM_REPEAT_H

#include <cstdint>
#include "istream_common.h"
#include "istream_repeat_callback.h"
#include "surface.h"

namespace OHOS {
namespace CameraStandard {
class IStreamRepeat : public IStreamCommon {
public:
    virtual int32_t Start() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t SetCallback(sptr<IStreamRepeatCallback>& callback) = 0;

    virtual int32_t UnSetCallback() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t AddDeferredSurface(const sptr<OHOS::IBufferProducer>& producer) = 0;

    virtual int32_t ForkSketchStreamRepeat(
        int32_t width, int32_t height, sptr<IStreamRepeat>& sketchStream, float sketchRatio) = 0;

    virtual int32_t UpdateSketchRatio(float sketchRatio) = 0;

    virtual int32_t RemoveSketchStreamRepeat() = 0;

    virtual int32_t SetFrameRate(int32_t minFrameRate, int32_t maxFrameRate) = 0;

    virtual int32_t EnableSecure(bool isEnable = false) = 0;
    
    virtual int32_t SetMirror(bool isEnable) = 0;

    virtual int32_t GetMirror(bool& isEnable) = 0;

    virtual int32_t AttachMetaSurface(const sptr<OHOS::IBufferProducer>& producer, int32_t videoMetaType) = 0;

    virtual int32_t SetCameraRotation(bool isEnable, int32_t rotation) = 0;

    virtual int32_t SetCameraApi(uint32_t apiCompatibleVersion) = 0;

    virtual int32_t ToggleAutoVideoFrameRate(bool isEnable) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamRepeat");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_REPEAT_H
