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

#include "hstream_capture_stub.h"
#include "camera_log.h"
#include "camera_util.h"
#include "metadata_utils.h"
#include "camera_service_ipc_interface_code.h"

namespace OHOS {
namespace CameraStandard {
int HStreamCaptureStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    DisableJeMalloc();
    int errCode = -1;

    CHECK_AND_RETURN_RET(data.ReadInterfaceToken() == GetDescriptor(), errCode);
    switch (code) {
        case static_cast<uint32_t>(StreamCaptureInterfaceCode::CAMERA_STREAM_CAPTURE_START):
            errCode = HStreamCaptureStub::HandleCapture(data);
            break;
        case static_cast<uint32_t>(StreamCaptureInterfaceCode::CAMERA_STREAM_CAPTURE_CANCEL):
            errCode = CancelCapture();
            break;
        case static_cast<uint32_t>(StreamCaptureInterfaceCode::CAMERA_STREAM_CAPTURE_CONFIRM):
            errCode = ConfirmCapture();
            break;
        case static_cast<uint32_t>(StreamCaptureInterfaceCode::CAMERA_STREAM_CAPTURE_SET_CALLBACK):
            errCode = HStreamCaptureStub::HandleSetCallback(data);
            break;
        case static_cast<uint32_t>(StreamCaptureInterfaceCode::CAMERA_STREAM_CAPTURE_RELEASE):
            errCode = Release();
            break;
        case static_cast<uint32_t>(StreamCaptureInterfaceCode::CAMERA_SERVICE_SET_THUMBNAIL):
            errCode = HandleSetThumbnail(data);
            break;
        default:
            MEDIA_ERR_LOG("HStreamCaptureStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HStreamCaptureStub::HandleCapture(MessageParcel &data)
{
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = nullptr;
    OHOS::Camera::MetadataUtils::DecodeCameraMetadata(data, metadata);

    return Capture(metadata);
}

int HStreamCaptureStub::HandleSetThumbnail(MessageParcel &data)
{
    sptr<IRemoteObject> remoteObj = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(remoteObj != nullptr, IPC_STUB_INVALID_DATA_ERR,
                             "HCameraServiceStub HandleCreatePhotoOutput BufferProducer is null");
    sptr<OHOS::IBufferProducer> producer = iface_cast<OHOS::IBufferProducer>(remoteObj);
    bool isEnabled = data.ReadBool();
    int32_t ret = SetThumbnail(isEnabled, producer);
    MEDIA_DEBUG_LOG("HCameraServiceStub HandleSetThumbnail result: %{public}d", ret);
    return ret;
}

int HStreamCaptureStub::HandleSetCallback(MessageParcel &data)
{
    auto remoteObject = data.ReadRemoteObject();
    CHECK_AND_RETURN_RET_LOG(remoteObject != nullptr, IPC_STUB_INVALID_DATA_ERR,
                             "HStreamCaptureStub HandleSetCallback StreamCaptureCallback is null");

    auto callback = iface_cast<IStreamCaptureCallback>(remoteObject);

    return SetCallback(callback);
}
} // namespace CameraStandard
} // namespace OHOS
