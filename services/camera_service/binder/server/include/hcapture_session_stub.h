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

#ifndef OHOS_CAMERA_HCAPTURE_SESSION_STUB_H
#define OHOS_CAMERA_HCAPTURE_SESSION_STUB_H

#include "camera_photo_proxy.h"
#include "icamera_ipc_checker.h"
#include "icapture_session.h"
#include "iremote_stub.h"
#include "istream_capture.h"
#include "istream_depth_data.h"
#include "istream_metadata.h"
#include "istream_repeat.h"
namespace OHOS {
namespace CameraStandard {
class HCaptureSessionStub : public IRemoteStub<ICaptureSession>, public ICameraIpcChecker {
public:
    int OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply, MessageOption& option) override;

private:
    int32_t HandleCanAddInput(MessageParcel& data, MessageParcel& reply);
    int32_t HandleAddInput(MessageParcel& data);
    int32_t HandleAddOutput(MessageParcel& data);
    int32_t HandleRemoveInput(MessageParcel& data);
    int32_t HandleRemoveOutput(MessageParcel& data);
    int32_t HandleSaveRestoreParam(MessageParcel& data);
    int32_t HandleSetCallback(MessageParcel& data);
    int32_t HandleGetSessionState(MessageParcel& reply);
    int32_t HandleGetActiveColorSpace(MessageParcel& reply);
    int32_t HandleSetColorSpace(MessageParcel& data);
    int32_t HandleSetSmoothZoom(MessageParcel& data, MessageParcel& reply);
    int32_t HandleSetFeatureMode(MessageParcel& data);
    int32_t HandleEnableMovingPhoto(MessageParcel& data);
    int32_t HandleStartMovingPhotoCapture(MessageParcel& data);
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HCAPTURE_SESSION_STUB_H
