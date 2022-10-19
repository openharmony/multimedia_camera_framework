/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#include "input/camera_mute_listener_napi.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
using OHOS::HiviewDFX::HiLog;

CameraMuteListenerNapi::CameraMuteListenerNapi(napi_env env, napi_ref ref): env_(env), callbackRef_(ref)
{
    MEDIA_INFO_LOG("Enter CameraMuteListenerNapi::CameraMuteListenerNapi");
}

CameraMuteListenerNapi::~CameraMuteListenerNapi()
{
    MEDIA_INFO_LOG("Enter CameraMuteListenerNapi::~CameraMuteListenerNapi");
}

void CameraMuteListenerNapi::OnCameraMute(bool muteMode)
{
    MEDIA_INFO_LOG("Enter CameraMuteListenerNapi::OnCameraMute");
    napi_value result[ARGS_TWO];
    napi_value callback = nullptr;
    napi_value retVal;
    napi_get_reference_value(env_, callbackRef_, &callback);
    napi_get_undefined(env_, &result[PARAM0]);
    napi_get_boolean(env_, muteMode, &result[PARAM1]);
    napi_call_function(env_, nullptr, callback, ARGS_TWO, result, &retVal);
}
} // namespace CameraStandard
} // namespace OHOS
