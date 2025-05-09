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

#include "hstream_capture_callback_stub.h"
#include "camera_log.h"
#include "camera_service_ipc_interface_code.h"

namespace OHOS {
namespace CameraStandard {
int HStreamCaptureCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    CHECK_ERROR_RETURN_RET(data.ReadInterfaceToken() != GetDescriptor(), errCode);
    switch (code) {
        case static_cast<uint32_t>(StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_CAPTURE_STARTED):
            errCode = HandleOnCaptureStarted(data);
            break;
        case static_cast<uint32_t>(StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_CAPTURE_STARTED_V1_2):
            errCode = HandleOnCaptureStarted_V1_2(data);
            break;
        case static_cast<uint32_t>(StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_CAPTURE_ENDED):
            errCode = HandleOnCaptureEnded(data);
            break;
        case static_cast<uint32_t>(StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_CAPTURE_ERROR):
            errCode = HandleOnCaptureError(data);
            break;
        case static_cast<uint32_t>(StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_FRAME_SHUTTER):
            errCode = HandleOnFrameShutter(data);
            break;
        case static_cast<uint32_t>(StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_FRAME_SHUTTER_END):
            errCode = HandleOnFrameShutterEnd(data);
            break;
        case static_cast<uint32_t>(StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_CAPTURE_READY):
            errCode = HandleOnCaptureReady(data);
            break;
        case static_cast<uint32_t>(
            StreamCaptureCallbackInterfaceCode::CAMERA_STREAM_CAPTURE_ON_OFFLINE_DELIVERY_FINISHED):
            errCode = HandleOnOfflineDeliveryFinished(data);
            break;
        default:
            MEDIA_ERR_LOG("HStreamCaptureCallbackStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return errCode;
}

int HStreamCaptureCallbackStub::HandleOnCaptureStarted(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();

    return OnCaptureStarted(captureId);
}

int HStreamCaptureCallbackStub::HandleOnCaptureStarted_V1_2(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();
    uint32_t exposureTime = data.ReadUint32();

    return OnCaptureStarted(captureId, exposureTime);
}

int HStreamCaptureCallbackStub::HandleOnCaptureEnded(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();
    int32_t frameCount = data.ReadInt32();

    return OnCaptureEnded(captureId, frameCount);
}

int HStreamCaptureCallbackStub::HandleOnCaptureError(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();
    int32_t errorCode = data.ReadInt32();

    return OnCaptureError(captureId, errorCode);
}

int HStreamCaptureCallbackStub::HandleOnFrameShutter(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();
    uint64_t timestamp = data.ReadUint64();

    return OnFrameShutter(captureId, timestamp);
}

int HStreamCaptureCallbackStub::HandleOnFrameShutterEnd(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();
    uint64_t timestamp = data.ReadUint64();

    return OnFrameShutterEnd(captureId, timestamp);
}

int HStreamCaptureCallbackStub::HandleOnCaptureReady(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();
    uint64_t timestamp = data.ReadUint64();

    return OnCaptureReady(captureId, timestamp);
}

int HStreamCaptureCallbackStub::HandleOnOfflineDeliveryFinished(MessageParcel& data)
{
    int32_t captureId = data.ReadInt32();
    return OnOfflineDeliveryFinished(captureId);
}
} // namespace CameraStandard
} // namespace OHOS

