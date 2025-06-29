/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "mode/aperture_video_session_napi.h"
#include "aperture_video_session.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;

thread_local napi_ref ApertureVideoSessionNapi::sConstructor_ = nullptr;

ApertureVideoSessionNapi::ApertureVideoSessionNapi() : env_(nullptr), wrapper_(nullptr) {}

ApertureVideoSessionNapi::~ApertureVideoSessionNapi()
{
    MEDIA_DEBUG_LOG("~ApertureVideoSessionNapi is called");
    CHECK_EXECUTE(wrapper_ != nullptr, napi_delete_reference(env_, wrapper_));
}

void ApertureVideoSessionNapi::ApertureVideoSessionNapiDestructor(
    napi_env env, void* nativeObject, void* finalize_hint)
{
    MEDIA_DEBUG_LOG("ApertureVideoSessionNapi is called");
    ApertureVideoSessionNapi* cameraObj = reinterpret_cast<ApertureVideoSessionNapi*>(nativeObject);
    if (cameraObj != nullptr) {
        delete cameraObj;
    }
}

napi_value ApertureVideoSessionNapi::Init(napi_env env, napi_value exports)
{
    MEDIA_DEBUG_LOG("Init is called");
    napi_status status;
    napi_value ctorObj;
    std::vector<std::vector<napi_property_descriptor>> descriptors = { camera_process_props, auto_exposure_props,
        color_effect_props, flash_props, focus_props, zoom_props, aperture_props };
    std::vector<napi_property_descriptor> session_props = CameraNapiUtils::GetPropertyDescriptor(descriptors);
    status = napi_define_class(env, APERTURE_VIDEO_SESSION_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
        ApertureVideoSessionNapiConstructor, nullptr, session_props.size(), session_props.data(), &ctorObj);
    if (status == napi_ok) {
        int32_t refCount = 1;
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, APERTURE_VIDEO_SESSION_NAPI_CLASS_NAME, ctorObj);
            CHECK_ERROR_RETURN_RET(status == napi_ok, exports);
        }
    }
    MEDIA_ERR_LOG("Init call Failed!");
    return nullptr;
}

napi_value ApertureVideoSessionNapi::CreateCameraSession(napi_env env)
{
    MEDIA_DEBUG_LOG("CreateCameraSession is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;
    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sCameraSession_ = CameraManager::GetInstance()->CreateCaptureSession(SceneMode::APERTURE_VIDEO);
        if (sCameraSession_ == nullptr) {
            MEDIA_ERR_LOG("ApertureVideoSessionNapi::CreateCameraSession Failed to create instance");
            napi_get_undefined(env, &result);
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sCameraSession_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            MEDIA_DEBUG_LOG("ApertureVideoSessionNapi::CreateCameraSession success to create napi instance");
            return result;
        } else {
            MEDIA_ERR_LOG("ApertureVideoSessionNapi::CreateCameraSession Failed to create napi instance");
        }
    }
    MEDIA_ERR_LOG("ApertureVideoSessionNapi::CreateCameraSession Failed to create napi instance last");
    napi_get_undefined(env, &result);
    return result;
}

napi_value ApertureVideoSessionNapi::ApertureVideoSessionNapiConstructor(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("ApertureVideoSessionNapiConstructor is called");
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<ApertureVideoSessionNapi> obj = std::make_unique<ApertureVideoSessionNapi>();
        obj->env_ = env;
        CHECK_ERROR_RETURN_RET_LOG(sCameraSession_ == nullptr, result, "sCameraSession_ is null");
        obj->apertureVideoSession_ = static_cast<ApertureVideoSession*>(sCameraSession_.GetRefPtr());
        obj->cameraSession_ = obj->apertureVideoSession_;
        CHECK_ERROR_RETURN_RET_LOG(obj->apertureVideoSession_ == nullptr, result, "apertureVideoSession_ is null");
        status = napi_wrap(
            env, thisVar, reinterpret_cast<void*>(obj.get()), ApertureVideoSessionNapiDestructor, nullptr, nullptr);
        if (status == napi_ok) {
            obj.release();
            return thisVar;
        } else {
            MEDIA_ERR_LOG("ApertureVideoSessionNapiConstructor Failure wrapping js to native napi");
        }
    }
    MEDIA_ERR_LOG("ApertureVideoSessionNapiConstructor call Failed!");
    return result;
}
} // namespace CameraStandard
} // namespace OHOS
