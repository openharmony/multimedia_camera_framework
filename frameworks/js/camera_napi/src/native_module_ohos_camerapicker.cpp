/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "camera_log.h"
#include "picker/camera_picker_napi.h"

namespace OHOS {
namespace CameraStandard {
/*
 * Function registering all props and functions of ohos.camera module
 */
static napi_value Export(napi_env env, napi_value exports)
{
    MEDIA_DEBUG_LOG("Export called()");
    CameraPickerNapi::Init(env, exports);
    MEDIA_DEBUG_LOG("Export is end");
    return exports;
}

/*
 * module define
 */
static napi_module g_module = { .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Export,
    .nm_modname = "multimedia.cameraPicker",
    .nm_priv = (reinterpret_cast<void*>(0)),
    .reserved = { 0 } };

/*
 * module register
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    MEDIA_DEBUG_LOG("RegisterModule is called");
    napi_module_register(&g_module);
}
} // namespace CameraStandard
} // namespace OHOS
