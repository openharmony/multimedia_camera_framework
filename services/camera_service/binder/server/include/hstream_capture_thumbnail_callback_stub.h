/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_HSTREAM_CAPTURE_THUMBNAIL_CALLBACK_STUB_H
#define OHOS_CAMERA_HSTREAM_CAPTURE_THUMBNAIL_CALLBACK_STUB_H

#include "iremote_stub.h"
#include "istream_capture_thumbnail_callback.h"

namespace OHOS {
namespace CameraStandard {

class HStreamCaptureThumbnailCallbackStub : public IRemoteStub<IStreamCaptureThumbnailCallback> {
public:
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data,
                                MessageParcel &reply, MessageOption &option) override;

private:
    int HandleOnThumbnailAvailable(MessageParcel& data);
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HSTREAM_CAPTURE_THUMBNAIL_CALLBACK_STUB_H
