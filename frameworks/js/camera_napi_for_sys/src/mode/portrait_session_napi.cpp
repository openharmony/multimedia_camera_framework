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

#include "mode/portrait_session_napi.h"

#include "camera_napi_utils.h"
#include "input/camera_manager_for_sys.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;

thread_local napi_ref PortraitSessionNapi::sConstructor_ = nullptr;

PortraitSessionNapi::PortraitSessionNapi() : env_(nullptr)
{
}
PortraitSessionNapi::~PortraitSessionNapi()
{
    MEDIA_DEBUG_LOG("~PortraitSessionNapi is called");
}
void PortraitSessionNapi::PortraitSessionNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    MEDIA_DEBUG_LOG("PortraitSessionNapiDestructor is called");
    PortraitSessionNapi* cameraObj = reinterpret_cast<PortraitSessionNapi*>(nativeObject);
    if (cameraObj != nullptr) {
        delete cameraObj;
    }
}
void PortraitSessionNapi::Init(napi_env env)
{
    MEDIA_DEBUG_LOG("Init is called");
    napi_status status;
    napi_value ctorObj;
    std::vector<napi_property_descriptor> portrait_props = {
        DECLARE_NAPI_FUNCTION("getSupportedPortraitEffects", GetSupportedPortraitEffects),
        DECLARE_NAPI_FUNCTION("getPortraitEffect", GetPortraitEffect),
        DECLARE_NAPI_FUNCTION("setPortraitEffect", SetPortraitEffect),
    };

    std::vector<std::vector<napi_property_descriptor>> descriptors = { camera_process_props,
        CameraSessionForSysNapi::camera_process_sys_props, CameraSessionForSysNapi::camera_output_capability_sys_props,
        CameraSessionForSysNapi::camera_ability_sys_props, flash_props, CameraSessionForSysNapi::flash_sys_props,
        auto_exposure_props, focus_props, CameraSessionForSysNapi::focus_sys_props, zoom_props,
        CameraSessionForSysNapi::zoom_sys_props, filter_props, beauty_sys_props, color_effect_sys_props,
        macro_props, color_management_props, portrait_props, aperture_sys_props };
    std::vector<napi_property_descriptor> portrait_session_props = CameraNapiUtils::GetPropertyDescriptor(descriptors);
    status = napi_define_class(env, PORTRAIT_SESSION_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               PortraitSessionNapiConstructor, nullptr,
                               portrait_session_props.size(),
                               portrait_session_props.data(), &ctorObj);
    CHECK_RETURN_ELOG(status != napi_ok, "PortraitSessionNapi defined class failed");
    int32_t refCount = 1;
    status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
    CHECK_RETURN_ELOG(status != napi_ok, "PortraitSessionNapi Init failed");
    MEDIA_DEBUG_LOG("PortraitSessionNapi Init success");
}
napi_value PortraitSessionNapi::CreateCameraSession(napi_env env)
{
    MEDIA_DEBUG_LOG("CreateCameraSession is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;
    if (sConstructor_ == nullptr) {
        PortraitSessionNapi::Init(env);
        CHECK_RETURN_RET_ELOG(sConstructor_ == nullptr, result, "sConstructor_ is null");
    }
    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sCameraSessionForSys_ =
            CameraManagerForSys::GetInstance()->CreateCaptureSessionForSys(SceneMode::PORTRAIT);
        if (sCameraSessionForSys_ == nullptr) {
            MEDIA_ERR_LOG("Failed to create Portrait session instance");
            napi_get_undefined(env, &result);
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sCameraSessionForSys_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            MEDIA_DEBUG_LOG("success to create Portrait session napi instance");
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create Portrait session napi instance");
        }
    }
    MEDIA_ERR_LOG("Failed to create Portrait session napi instance last");
    napi_get_undefined(env, &result);
    return result;
}

napi_value PortraitSessionNapi::PortraitSessionNapiConstructor(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("PortraitSessionNapiConstructor is called");
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<PortraitSessionNapi> obj = std::make_unique<PortraitSessionNapi>();
        obj->env_ = env;
        CHECK_RETURN_RET_ELOG(sCameraSessionForSys_ == nullptr, result, "sCameraSessionForSys_ is null");
        obj->portraitSession_ = static_cast<PortraitSession*>(sCameraSessionForSys_.GetRefPtr());
        obj->cameraSessionForSys_ = obj->portraitSession_;
        obj->cameraSession_ = obj->portraitSession_;
        CHECK_RETURN_RET_ELOG(obj->portraitSession_ == nullptr, result, "portraitSession_ is null");
        status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
            PortraitSessionNapi::PortraitSessionNapiDestructor, nullptr, nullptr);
        if (status == napi_ok) {
            obj.release();
            return thisVar;
        } else {
            MEDIA_ERR_LOG("PortraitSessionNapi Failure wrapping js to native napi");
        }
    }
    MEDIA_ERR_LOG("PortraitSessionNapi call Failed!");
    return result;
}


napi_value PortraitSessionNapi::GetSupportedPortraitEffects(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("GetPortraitEffect is called");
    napi_status status;
    napi_value result = nullptr;
    napi_value argv[ARGS_ZERO];
    size_t argc = ARGS_ZERO;
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);

    napi_get_undefined(env, &result);
    status = napi_create_array(env, &result);
    CHECK_RETURN_RET_ELOG(status != napi_ok, result, "napi_create_array call Failed!");
    PortraitSessionNapi* portraitSessionNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&portraitSessionNapi));
    if (status == napi_ok && portraitSessionNapi != nullptr && portraitSessionNapi->portraitSession_ != nullptr) {
        std::vector<PortraitEffect> PortraitEffects =
            portraitSessionNapi->portraitSession_->GetSupportedPortraitEffects();
        MEDIA_INFO_LOG("PortraitSessionNapi::GetSupportedPortraitEffect len = %{public}zu",
            PortraitEffects.size());
        if (!PortraitEffects.empty()) {
            for (size_t i = 0; i < PortraitEffects.size(); i++) {
                PortraitEffect portaitEffect = PortraitEffects[i];
                napi_value value;
                napi_create_int32(env, portaitEffect, &value);
                napi_set_element(env, result, i, value);
            }
        }
    } else {
        MEDIA_ERR_LOG("GetSupportedPortraitEffect call Failed!");
    }
    return result;
}

napi_value PortraitSessionNapi::GetPortraitEffect(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("GetPortraitEffect is called");
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ZERO;
    napi_value argv[ARGS_ZERO];
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);

    napi_get_undefined(env, &result);
    PortraitSessionNapi* portraitSessionNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&portraitSessionNapi));
    if (status == napi_ok && portraitSessionNapi != nullptr && portraitSessionNapi->portraitSession_ != nullptr) {
        PortraitEffect portaitEffect = portraitSessionNapi->portraitSession_->GetPortraitEffect();
        napi_create_int32(env, portaitEffect, &result);
    } else {
        MEDIA_ERR_LOG("GetPortraitEffect call Failed!");
    }
    return result;
}

napi_value PortraitSessionNapi::SetPortraitEffect(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("SetPortraitEffect is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);

    napi_get_undefined(env, &result);
    PortraitSessionNapi* portraitSessionNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&portraitSessionNapi));
    if (status == napi_ok && portraitSessionNapi != nullptr && portraitSessionNapi->portraitSession_ != nullptr) {
        int32_t value;
        napi_get_value_int32(env, argv[PARAM0], &value);
        PortraitEffect portaitEffect = static_cast<PortraitEffect>(value);
        portraitSessionNapi->portraitSession_->LockForControl();
        portraitSessionNapi->portraitSession_->
                SetPortraitEffect(static_cast<PortraitEffect>(portaitEffect));
        MEDIA_INFO_LOG("PortraitSessionNapi setPortraitEffect set portaitEffect %{public}d!", portaitEffect);
        portraitSessionNapi->portraitSession_->UnlockForControl();
    } else {
        MEDIA_ERR_LOG("setPortraitEffect call Failed!");
    }
    return result;
}
} // namespace CameraStandard
} // namespace OHOS