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

#include "hstream_repeat_proxy.h"
#include "camera_log.h"
#include "camera_service_ipc_interface_code.h"

namespace OHOS {
namespace CameraStandard {
HStreamRepeatProxy::HStreamRepeatProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStreamRepeat>(impl) { }

HStreamRepeatProxy::~HStreamRepeatProxy()
{
    MEDIA_INFO_LOG("~HStreamRepeatProxy is called");
}

int32_t HStreamRepeatProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(GetDescriptor());
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(StreamRepeatInterfaceCode::CAMERA_START_VIDEO_RECORDING), data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Start failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(GetDescriptor());
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(StreamRepeatInterfaceCode::CAMERA_STOP_VIDEO_RECORDING), data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Stop failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    data.WriteInterfaceToken(GetDescriptor());
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(StreamRepeatInterfaceCode::CAMERA_STREAM_REPEAT_RELEASE), data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy Stop failed, error: %{public}d", error);
    }
    return error;
}

int32_t HStreamRepeatProxy::SetCallback(sptr<IStreamRepeatCallback> &callback)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (callback == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetCallback callback is null");
        return IPC_PROXY_ERR;
    }

    data.WriteInterfaceToken(GetDescriptor());
    data.WriteRemoteObject(callback->AsObject());

    int error = Remote()->SendRequest(
        static_cast<uint32_t>(StreamRepeatInterfaceCode::CAMERA_STREAM_REPEAT_SET_CALLBACK), data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy SetCallback failed, error: %{public}d", error);
    }

    return error;
}

int32_t HStreamRepeatProxy::AddDeferredSurface(const sptr<OHOS::IBufferProducer> &producer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    if (producer == nullptr) {
        MEDIA_ERR_LOG("HStreamRepeatProxy AddDeferredSurface producer is null");
        return IPC_PROXY_ERR;
    }

    data.WriteInterfaceToken(GetDescriptor());
    data.WriteRemoteObject(producer->AsObject());
    int error = Remote()->SendRequest(
        static_cast<uint32_t>(StreamRepeatInterfaceCode::CAMERA_ADD_DEFERRED_SURFACE), data, reply, option);
    if (error != ERR_NONE) {
        MEDIA_ERR_LOG("HStreamRepeatProxy::AddDeferredSurface failed, error: %{public}d", error);
        return error;
    }

    return error;
}
} // namespace CameraStandard
} // namespace OHOS
