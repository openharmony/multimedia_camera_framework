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

#include "hstream_depth_data_callback_proxy.h"
#include "camera_log.h"
#include "camera_service_ipc_interface_code.h"
#include "istream_depth_data_callback.h"

namespace OHOS {
namespace CameraStandard {
HStreamDepthDataCallbackProxy::HStreamDepthDataCallbackProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStreamDepthDataCallback>(impl) { }

int32_t HStreamDepthDataCallbackProxy::OnDepthDataError(int32_t errorCode)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    option.SetFlags(option.TF_ASYNC);

    data.WriteInterfaceToken(GetDescriptor());
    data.WriteInt32(errorCode);

    int error = Remote()->SendRequest(static_cast<uint32_t>(
        StreamDepthDataCallbackInterfaceCode::CAMERA_STREAM_DEPTH_DATA_ON_ERROR), data, reply, option);
    CHECK_ERROR_PRINT_LOG(error != ERR_NONE,
        "HStreamDepthDataCallbackProxy OnDepthDataError failed, error: %{public}d", error);

    return error;
}

} // namespace CameraStandard
} // namespace OHOS