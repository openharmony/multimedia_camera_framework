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

#include "hstream_metadata_callback_stub.h"
#include "camera_log.h"
#include "metadata_utils.h"
#include "camera_service_ipc_interface_code.h"

namespace OHOS {
namespace CameraStandard {
int HStreamMetadataCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    CHECK_ERROR_RETURN_RET(data.ReadInterfaceToken() != GetDescriptor(), errCode);
    switch (code) {
        case static_cast<uint32_t>(StreamMetadataCallbackInterfaceCode::CAMERA_META_OPERATOR_ON_RESULT):
            errCode = HStreamMetadataCallbackStub::HandleMetadataResult(data);
            break;
        default:
            MEDIA_ERR_LOG("HCameraDeviceCallbackStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }
    return errCode;
}

int HStreamMetadataCallbackStub::HandleMetadataResult(MessageParcel& data)
{
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = nullptr;
    int32_t streamId = data.ReadInt32();

    OHOS::Camera::MetadataUtils::DecodeCameraMetadata(data, metadata);
    return OnMetadataResult(streamId, metadata);
}
} // namespace CameraStandard
} // namespace OHOS
