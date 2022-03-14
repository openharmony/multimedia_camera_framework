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

#ifndef CAMERA_MANAGER_NAPI_H_
#define CAMERA_MANAGER_NAPI_H_

#include <securec.h>

#include "display_type.h"
#include "media_log.h"
#include "hilog/log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include "input/camera_manager.h"
#include "input/camera_info.h"
#include "output/capture_output.h"

#include "input/camera_input_napi.h"
#include "input/camera_info_napi.h"
#include "output/preview_output_napi.h"
#include "output/photo_output_napi.h"
#include "output/video_output_napi.h"
#include "session/camera_session_napi.h"
#include "camera_napi_utils.h"
#include "camera_manager_callback_napi.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

namespace OHOS {
namespace CameraStandard {
static const std::string CAMERA_MANAGER_NAPI_CLASS_NAME = "CameraManager";

class CameraManagerNapi {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateCameraManagerInstance(napi_env env);

    CameraManagerNapi();
    ~CameraManagerNapi();

private:
    static void CameraManagerNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value CameraManagerNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetCameras(napi_env env, napi_callback_info info);
    static napi_value CreateCameraInputInstance(napi_env env, napi_callback_info info);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_ref sConstructor_;

    napi_env env_;
    napi_ref wrapper_;
    sptr<CameraManager> cameraManager_;
};

struct CameraManagerNapiAsyncContext {
    napi_env env;
    napi_async_work work;
    napi_deferred deferred;
    napi_ref callbackRef;
    bool status;
    CameraManagerNapi *objectInfo;
    std::string cameraId;
    camera_position_enum_t cameraPosition;
    camera_type_enum_t cameraType;
    sptr<CameraInput> cameraInput;
    sptr<CameraInfo> cameraInfo;
    std::vector<sptr<CameraInfo>> cameraObjList;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_MANAGER_NAPI_H_ */
