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

#include "camera_napi_security_utils.h"
#include "camera_xcollie.h"
#include "dynamic_loader/camera_napi_ex_manager.h"
#include "input/camera_napi.h"
#include "napi/native_common.h"

namespace OHOS {
namespace CameraStandard {
using namespace std;
thread_local napi_ref CameraNapi::sConstructor_ = nullptr;
thread_local napi_ref g_ignoreRef_ = nullptr;

CameraNapi::CameraNapi() : env_(nullptr)
{
    MEDIA_INFO_LOG("CameraNapi::CameraNapi constructor");
}

CameraNapi::~CameraNapi()
{
    MEDIA_INFO_LOG("CameraNapi::~CameraNapi destructor");
}

// Constructor callback
napi_value CameraNapi::CameraNapiConstructor(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("CameraNapiConstructor is called");
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraNapi> obj = std::make_unique<CameraNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               CameraNapi::CameraNapiDestructor, nullptr, nullptr);
            if (status == napi_ok) {
                (void)obj.release();
                return thisVar;
            } else {
                MEDIA_ERR_LOG("CameraNapiConstructor Failure wrapping js to native napi");
            }
        }
    }
    MEDIA_ERR_LOG("CameraNapiConstructor call Failed!");
    return result;
}

void CameraNapi::CameraNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    MEDIA_DEBUG_LOG("CameraNapiDestructor is called");
    CameraNapi* camera = reinterpret_cast<CameraNapi*>(nativeObject);
    if (camera != nullptr) {
        delete camera;
    }
}

napi_value CameraNapi::Init(napi_env env, napi_value exports)
{
    MEDIA_DEBUG_LOG("Init is called");
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;
    napi_property_descriptor camera_properties[] = {
        DECLARE_NAPI_FUNCTION("getCameraManagerTest", CreateCameraManagerInstance)
    };

    napi_property_descriptor camera_static_prop[] = {
        DECLARE_NAPI_STATIC_FUNCTION("getCameraManager", CreateCameraManagerInstance),
        DECLARE_NAPI_STATIC_FUNCTION("getModeManager", CreateModeManagerInstance),
        DECLARE_NAPI_PROPERTY("FlashMode", CreateObjectWithMap(env, "FlashMode", mapFlashMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("ExposureMode", CreateObjectWithMap(env, "ExposureMode", mapExposureMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "ExposureState", CreateObjectWithMap(env, "ExposureState", mapExposureState, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("FocusMode", CreateObjectWithMap(env, "FocusMode", mapFocusMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("QualityPrioritization",
            CreateObjectWithMap(env, "QualityPrioritization", mapQualityPrioritization, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("FocusState", CreateObjectWithMap(env, "FocusState", mapFocusState, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "CameraPosition", CreateObjectWithMap(env, "CameraPosition", mapCameraPosition, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("CameraType", CreateObjectWithMap(env, "CameraType", mapCameraType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "ConnectionType", CreateObjectWithMap(env, "ConnectionType", mapConnectionType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("CameraFormat", CreateObjectWithMap(env, "CameraFormat", mapCameraFormat, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("CameraStatus", CreateObjectWithMap(env, "CameraStatus", mapCameraStatus, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "ImageRotation", CreateObjectWithMap(env, "ImageRotation", mapImageRotation, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("QualityLevel", CreateObjectWithMap(env, "QualityLevel", mapQualityLevel, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("VideoStabilizationMode",
            CreateObjectWithMap(env, "VideoStabilizationMode", mapVideoStabilizationMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "MetadataObjectType", CreateObjectWithMap(env, "MetadataObjectType", mapMetadataObjectType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("HostNameType", CreateObjectWithMap(env, "HostNameType", mapHostNameType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "HostDeviceType", CreateObjectWithMap(env, "HostDeviceType", mapHostDeviceType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("CameraMode", CreateObjectWithMap(env, "SceneMode", mapSceneMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("SceneMode", CreateObjectWithMap(env, "SceneMode", mapSceneMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "PreconfigType", CreateObjectWithMap(env, "PreconfigType", mapPreconfigType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "PreconfigRatio", CreateObjectWithMap(env, "PreconfigRatio", mapPreconfigRatio, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("FilterType", CreateObjectWithMap(env, "FilterType", mapFilterType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("BeautyType", CreateObjectWithMap(env, "BeautyType", mapBeautyType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "PortraitEffect", CreateObjectWithMap(env, "PortraitEffect", mapPortraitEffect, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("TorchMode", CreateObjectWithMap(env, "TorchMode", mapTorchMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "CameraErrorCode", CreateObjectWithMap(env, "CameraErrorCode", mapCameraErrorCode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("CameraInputErrorCode",
            CreateObjectWithMap(env, "CameraInputErrorCode", mapCameraInputErrorCode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("CaptureSessionErrorCode",
            CreateObjectWithMap(env, "CaptureSessionErrorCode", mapCaptureSessionErrorCode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("PreviewOutputErrorCode",
            CreateObjectWithMap(env, "PreviewOutputErrorCode", mapPreviewOutputErrorCode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("PhotoOutputErrorCode",
            CreateObjectWithMap(env, "PhotoOutputErrorCode", mapPhotoOutputErrorCode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("VideoOutputErrorCode",
            CreateObjectWithMap(env, "VideoOutputErrorCode", mapVideoOutputErrorCode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("MetadataOutputErrorCode",
            CreateObjectWithMap(env, "MetadataOutputErrorCode", mapMetadataOutputErrorCode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("DeferredDeliveryImageType",
            CreateObjectWithMap(env, "DeferredDeliveryImageType", mapDeferredDeliveryImageType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "SmoothZoomMode", CreateObjectWithMap(env, "SmoothZoomMode", mapSmoothZoomMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "ColorEffectType", CreateObjectWithMap(env, "ColorEffectType", mapColorEffectType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "RestoreParamType", CreateObjectWithMap(env, "RestoreParamType", mapRestoreParamType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("ExposureMeteringMode",
            CreateObjectWithMap(env, "ExposureMeteringMode", mapExposureMeteringMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("EffectSuggestionType",
            CreateObjectWithMap(env, "EffectSuggestionType", mapEffectSuggestionType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("VideoMetaType",
            CreateObjectWithMap(env, "VideoMetaType", mapVideoMetaType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("PolicyType", CreateObjectWithMap(env, "PolicyType", mapPolicyType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "SceneFeatureType", CreateObjectWithMap(env, "SceneFeatureType", mapSceneFeatureType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("FoldStatus", CreateObjectWithMap(env, "FoldStatus", mapFoldStatus, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY(
            "LightPaintingType", CreateObjectWithMap(env, "LightPaintingType", mapLightPaintingType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("TimeLapseRecordState",
            CreateObjectWithMap(env, "TimeLapseRecordState", mapTimeLapseRecordState, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("TimeLapsePreviewType",
            CreateObjectWithMap(env, "TimeLapsePreviewType", mapTimeLapsePreviewType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("VideoCodecType",
            CreateObjectWithMap(env, "VideoCodecType", mapVideoCodecType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("TripodStatus",
            CreateObjectWithMap(env, "TripodStatus", mapTripodStatus, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("Emotion",
            CreateObjectWithMap(env, "Emotion", mapMetaFaceEmotion, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("UsageType", CreateObjectWithMap(env, "UsageType", mapUsageType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("PortraitThemeType",
            CreateObjectWithMap(env, "PortraitThemeType", mapPortraitThemeType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("FocusRangeType",
            CreateObjectWithMap(env, "FocusRangeType", mapFocusRangeType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("FocusDrivenType",
            CreateObjectWithMap(env, "FocusDrivenType", mapFocusDrivenType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("ColorReservationType",
            CreateObjectWithMap(env, "ColorReservationType", mapColorReservationType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("FocusTrackingMode",
            CreateObjectWithMap(env, "FocusTrackingMode", mapFocusTrackingMode, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("CameraConcurrentType",
            CreateObjectWithMap(env, "CameraConcurrentType", mapCameraConcurrentType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("AuxiliaryType",
            CreateObjectWithMap(env, "AuxiliaryType", mapAuxiliaryType, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("AuxiliaryStatus",
            CreateObjectWithMap(env, "AuxiliaryStatus", mapAuxiliaryStatus, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("SlowMotionStatus",
            CreateObjectWithMap(env, "SlowMotionStatus", mapSlowMotionStatus, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("LightStatus",
            CreateObjectWithMap(env, "LightStatus", mapLightStatus, g_ignoreRef_)),
        DECLARE_NAPI_PROPERTY("WhiteBalanceMode",
            CreateObjectWithMap(env, "WhiteBalanceMode", mapWhiteBalanceMode, g_ignoreRef_)),
    };

    status = napi_define_class(env, CAMERA_LIB_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH, CameraNapiConstructor,
                               nullptr, sizeof(camera_properties) / sizeof(camera_properties[PARAM0]),
                               camera_properties, &ctorObj);
    if (status == napi_ok) {
        if (napi_create_reference(env, ctorObj, refCount, &sConstructor_) == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_LIB_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok && napi_define_properties(env, exports,
                sizeof(camera_static_prop) / sizeof(camera_static_prop[PARAM0]), camera_static_prop) == napi_ok) {
                return exports;
            }
        }
    }
    MEDIA_ERR_LOG("Init call Failed!");
    return nullptr;
}

napi_status CameraNapi::AddNamedProperty(napi_env env, napi_value object,
                                         const std::string name, int32_t enumValue)
{
    napi_status status;
    napi_value enumNapiValue;

    status = napi_create_int32(env, enumValue, &enumNapiValue);
    if (status == napi_ok) {
        status = napi_set_named_property(env, object, name.c_str(), enumNapiValue);
    }

    return status;
}

napi_value CameraNapi::CreateCameraManagerInstance(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("CreateCameraManagerInstance is called");
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);

    napi_get_undefined(env, &result);
    result = CameraManagerNapi::CreateCameraManager(env);
    MEDIA_INFO_LOG("CreateCameraManagerInstance::CreateCameraManager() is end");
    return result;
}

napi_value CameraNapi::CreateModeManagerInstance(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("CreateModeManagerInstance is called");
    CHECK_RETURN_RET_ELOG(!CameraNapiSecurity::CheckSystemApp(env), nullptr,
        "SystemApi CreateModeManagerInstance is called!");
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;

    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);

    napi_get_undefined(env, &result);
    auto cameraNapiExProxy = CameraNapiExManager::GetCameraNapiExProxy(CameraNapiExProxyUserType::MODE_MANAGER_NAPI);
    CHECK_RETURN_RET_ELOG(cameraNapiExProxy == nullptr, nullptr, "cameraNapiExProxy is nullptr");
    result = cameraNapiExProxy->CreateModeManager(env);
    return result;
}

napi_value CameraNapi::CreateObjectWithMap(napi_env env,
                                           const std::string objectName,
                                           const std::unordered_map<std::string, int32_t>& inputMap,
                                           napi_ref& outputRef)
{
    MEDIA_DEBUG_LOG("Create %{public}s is called", objectName.c_str());
    napi_value result = nullptr;
    napi_status status;

    status = napi_create_object(env, &result);
    if (status == napi_ok) {
        std::string propName;
        for (auto itr = inputMap.begin(); itr != inputMap.end(); ++itr) {
            propName = itr->first;
            status = AddNamedProperty(env, result, propName, itr->second);
            if (status != napi_ok) {
                MEDIA_ERR_LOG("Failed to add %{public}s property!", objectName.c_str());
                break;
            }
            propName.clear();
        }
    }
    if (status == napi_ok) {
        status = napi_create_reference(env, result, 1, &outputRef);
        CHECK_RETURN_RET(status == napi_ok, result);
    }
    MEDIA_ERR_LOG("Create %{public}s call Failed!", objectName.c_str());
    napi_get_undefined(env, &result);

    return result;
}
} // namespace CameraStandard
} // namespace OHOS
