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

#include "hstream_depth_data_callback_stub.h"
#include "camera_log.h"
#include "camera_service_ipc_interface_code.h"

namespace OHOS {
namespace CameraStandard {
int HStreamDepthDataCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    int errCode = -1;

    CHECK_ERROR_RETURN_RET(data.ReadInterfaceToken() != GetDescriptor(), errCode);
    switch (code) {
        case static_cast<uint32_t>(StreamDepthDataCallbackInterfaceCode::CAMERA_STREAM_DEPTH_DATA_ON_ERROR):
            errCode = HStreamDepthDataCallbackStub::HandleOnDepthDataError(data);
            break;
        default:
            MEDIA_ERR_LOG("HStreamDepthDataCallbackStub request code %{public}u not handled", code);
            errCode = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }
    return errCode;
}

int HStreamDepthDataCallbackStub::HandleOnDepthDataError(MessageParcel& data)
{
    int32_t errorType = static_cast<int32_t>(data.ReadUint64());
    return OnDepthDataError(errorType);
}

} // namespace CameraStandard
} // namespace OHOS
