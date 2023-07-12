/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "camera_napi_utils.h"

namespace OHOS {
namespace CameraStandard {
int32_t CameraNapiUtils::MapExposureModeEnumFromJs(
    int32_t jsExposureMode, camera_exposure_mode_enum_t &nativeExposureMode)
{
    MEDIA_INFO_LOG("js exposure mode = %{public}d", jsExposureMode);
    switch (jsExposureMode) {
        case EXPOSURE_MODE_LOCKED:
            nativeExposureMode = OHOS_CAMERA_EXPOSURE_MODE_LOCKED;
            break;
        case EXPOSURE_MODE_AUTO:
            nativeExposureMode = OHOS_CAMERA_EXPOSURE_MODE_AUTO;
            break;
        case EXPOSURE_MODE_CONTINUOUS_AUTO:
            nativeExposureMode = OHOS_CAMERA_EXPOSURE_MODE_CONTINUOUS_AUTO;
            break;
        default:
            MEDIA_ERR_LOG("Invalid exposure mode value received from application");
            return -1;
    }

    return 0;
}

void CameraNapiUtils::MapExposureModeEnum(camera_exposure_mode_enum_t nativeExposureMode, int32_t &jsExposureMode)
{
    MEDIA_INFO_LOG("native exposure mode = %{public}d", static_cast<int32_t>(nativeExposureMode));
    switch (nativeExposureMode) {
        case OHOS_CAMERA_EXPOSURE_MODE_LOCKED:
            jsExposureMode = EXPOSURE_MODE_LOCKED;
            break;
        case OHOS_CAMERA_EXPOSURE_MODE_AUTO:
            jsExposureMode = EXPOSURE_MODE_AUTO;
            break;
        case OHOS_CAMERA_EXPOSURE_MODE_CONTINUOUS_AUTO:
            jsExposureMode = EXPOSURE_MODE_CONTINUOUS_AUTO;
            break;
        default:
            MEDIA_ERR_LOG("Received native exposure mode is not supported with JS");
            jsExposureMode = -1;
    }
}

void CameraNapiUtils::MapFocusStateEnum(FocusCallback::FocusState nativeFocusState, int32_t &jsFocusState)
{
    MEDIA_INFO_LOG("native focus state = %{public}d", static_cast<int32_t>(nativeFocusState));
    switch (nativeFocusState) {
        case FocusCallback::SCAN:
            jsFocusState = FOCUS_STATE_SCAN;
            break;
        case FocusCallback::FOCUSED:
            jsFocusState = FOCUS_STATE_FOCUSED;
            break;
        case FocusCallback::UNFOCUSED:
        default:
            jsFocusState = FOCUS_STATE_UNFOCUSED;
    }
}

void CameraNapiUtils::MapExposureStateEnum(
    ExposureCallback::ExposureState nativeExposureState, int32_t &jsExposureState)
{
    MEDIA_INFO_LOG("native exposure state = %{public}d", static_cast<int32_t>(nativeExposureState));
    switch (nativeExposureState) {
        case ExposureCallback::SCAN:
            jsExposureState = EXPOSURE_STATE_SCAN;
            break;
        case ExposureCallback::CONVERGED:
        default:
            jsExposureState = EXPOSURE_STATE_CONVERGED;
    }
}

void CameraNapiUtils::MapCameraPositionEnum(camera_position_enum_t nativeCamPos, int32_t &jsCameraPosition)
{
    MEDIA_INFO_LOG("native cam pos = %{public}d", static_cast<int32_t>(nativeCamPos));
    switch (nativeCamPos) {
        case OHOS_CAMERA_POSITION_FRONT:
            jsCameraPosition = CAMERA_POSITION_FRONT;
            break;
        case OHOS_CAMERA_POSITION_BACK:
            jsCameraPosition = CAMERA_POSITION_BACK;
            break;
        case OHOS_CAMERA_POSITION_OTHER:
        default:
            jsCameraPosition = CAMERA_POSITION_UNSPECIFIED;
    }
}

int32_t CameraNapiUtils::MapCameraPositionEnumFromJs(int32_t jsCameraPosition, camera_position_enum_t &nativeCamPos)
{
    MEDIA_INFO_LOG("js cam pos = %{public}d", jsCameraPosition);
    switch (jsCameraPosition) {
        case CAMERA_POSITION_FRONT:
            nativeCamPos = OHOS_CAMERA_POSITION_FRONT;
            break;
        case CAMERA_POSITION_BACK:
            nativeCamPos = OHOS_CAMERA_POSITION_BACK;
            break;
        case CAMERA_POSITION_UNSPECIFIED:
            nativeCamPos = OHOS_CAMERA_POSITION_OTHER;
            break;
        default:
            MEDIA_ERR_LOG("Invalid camera position value received from application");
            return -1;
    }

    return 0;
}

void CameraNapiUtils::MapCameraFormatEnum(camera_format_t nativeCamFormat, int32_t &jsCameraFormat)
{
    MEDIA_INFO_LOG("native cam format = %{public}d", static_cast<int32_t>(nativeCamFormat));
    switch (nativeCamFormat) {
        case OHOS_CAMERA_FORMAT_YCRCB_420_SP:
            jsCameraFormat = CAMERA_FORMAT_YUV_420_SP;
            break;
        case OHOS_CAMERA_FORMAT_JPEG:
            jsCameraFormat = CAMERA_FORMAT_JPEG;
            break;
        case OHOS_CAMERA_FORMAT_RGBA_8888:
        case OHOS_CAMERA_FORMAT_YCBCR_420_888:
        default:
            jsCameraFormat = -1;
            MEDIA_ERR_LOG("The native camera format is not supported with JS");
    }
}

void CameraNapiUtils::MapMetadataObjSupportedTypesEnum(
    MetadataObjectType nativeMetadataObjType, int32_t &jsMetadataObjType)
{
    MEDIA_INFO_LOG("native metadata Object Type = %{public}d", static_cast<int32_t>(nativeMetadataObjType));
    switch (nativeMetadataObjType) {
        case MetadataObjectType::FACE:
            jsMetadataObjType = JSMetadataObjectType::FACE;
            break;
        default:
            jsMetadataObjType = -1;
            MEDIA_ERR_LOG("Native Metadata object type not supported with JS");
    }
}

void CameraNapiUtils::MapMetadataObjSupportedTypesEnumFromJS(int32_t jsMetadataObjType,
    MetadataObjectType &nativeMetadataObjType, bool &isValid)
{
    MEDIA_INFO_LOG("JS metadata Object Type = %{public}d", jsMetadataObjType);
    switch (jsMetadataObjType) {
        case JSMetadataObjectType::FACE:
            nativeMetadataObjType = MetadataObjectType::FACE;
            break;
        default:
            isValid = false;
            MEDIA_ERR_LOG("JS Metadata object type not supported with native");
    }
}

int32_t CameraNapiUtils::MapCameraFormatEnumFromJs(int32_t jsCameraFormat, camera_format_t &nativeCamFormat)
{
    MEDIA_INFO_LOG("js cam format = %{public}d", jsCameraFormat);
    switch (jsCameraFormat) {
        case CAMERA_FORMAT_YUV_420_SP:
            nativeCamFormat = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
            break;
        case CAMERA_FORMAT_JPEG:
            nativeCamFormat = OHOS_CAMERA_FORMAT_JPEG;
            break;
        default:
            MEDIA_ERR_LOG("Invalid or unsupported camera format value received from application");
            return -1;
    }

    return 0;
}

void CameraNapiUtils::MapCameraTypeEnum(camera_type_enum_t nativeCamType, int32_t &jsCameraType)
{
    MEDIA_INFO_LOG("native cam type = %{public}d", static_cast<int32_t>(nativeCamType));
    switch (nativeCamType) {
        case OHOS_CAMERA_TYPE_WIDE_ANGLE:
            jsCameraType = CAMERA_TYPE_WIDE_ANGLE;
            break;
        case OHOS_CAMERA_TYPE_ULTRA_WIDE:
            jsCameraType = CAMERA_TYPE_ULTRA_WIDE;
            break;
        case OHOS_CAMERA_TYPE_TELTPHOTO:
            jsCameraType = CAMERA_TYPE_TELEPHOTO;
            break;
        case OHOS_CAMERA_TYPE_TRUE_DEAPTH:
            jsCameraType = CAMERA_TYPE_TRUE_DEPTH;
            break;
        case OHOS_CAMERA_TYPE_LOGICAL:
            MEDIA_ERR_LOG("Logical camera type is treated as unspecified");
            jsCameraType = CAMERA_TYPE_DEFAULT;
            break;
        case OHOS_CAMERA_TYPE_UNSPECIFIED:
        default:
            jsCameraType = CAMERA_TYPE_DEFAULT;
    }
}

int32_t CameraNapiUtils::MapCameraTypeEnumFromJs(int32_t jsCameraType, camera_type_enum_t &nativeCamType)
{
    MEDIA_INFO_LOG("js cam type = %{public}d", jsCameraType);
    switch (jsCameraType) {
        case CAMERA_TYPE_WIDE_ANGLE:
            nativeCamType = OHOS_CAMERA_TYPE_WIDE_ANGLE;
            break;
        case CAMERA_TYPE_ULTRA_WIDE:
            nativeCamType = OHOS_CAMERA_TYPE_ULTRA_WIDE;
            break;
        case CAMERA_TYPE_TELEPHOTO:
            nativeCamType = OHOS_CAMERA_TYPE_TELTPHOTO;
            break;
        case CAMERA_TYPE_TRUE_DEPTH:
            nativeCamType = OHOS_CAMERA_TYPE_TRUE_DEAPTH;
            break;
        case CAMERA_TYPE_DEFAULT:
            nativeCamType = OHOS_CAMERA_TYPE_UNSPECIFIED;
            break;
        default:
            MEDIA_ERR_LOG("Invalid camera type value received from application");
            return -1;
    }

    return 0;
}

void CameraNapiUtils::MapCameraConnectionTypeEnum(
    camera_connection_type_t nativeCamConnType, int32_t &jsCameraConnType)
{
    MEDIA_INFO_LOG("native cam connection type = %{public}d", static_cast<int32_t>(nativeCamConnType));
    switch (nativeCamConnType) {
        case OHOS_CAMERA_CONNECTION_TYPE_REMOTE:
            jsCameraConnType = CAMERA_CONNECTION_REMOTE;
            break;
        case OHOS_CAMERA_CONNECTION_TYPE_USB_PLUGIN:
            jsCameraConnType = CAMERA_CONNECTION_USB_PLUGIN;
            break;
        case OHOS_CAMERA_CONNECTION_TYPE_BUILTIN:
        default:
            jsCameraConnType = CAMERA_CONNECTION_BUILT_IN;
    }
}

int32_t CameraNapiUtils::MapQualityLevelFromJs(int32_t jsQuality, PhotoCaptureSetting::QualityLevel &nativeQuality)
{
    MEDIA_INFO_LOG("js quality level = %{public}d", jsQuality);
    switch (jsQuality) {
        case QUALITY_LEVEL_HIGH:
            nativeQuality = PhotoCaptureSetting::QUALITY_LEVEL_HIGH;
            break;
        case QUALITY_LEVEL_MEDIUM:
            nativeQuality = PhotoCaptureSetting::QUALITY_LEVEL_MEDIUM;
            break;
        case QUALITY_LEVEL_LOW:
            nativeQuality = PhotoCaptureSetting::QUALITY_LEVEL_LOW;
            break;
        default:
            MEDIA_ERR_LOG("Invalid quality value received from application");
            return -1;
    }

    return 0;
}

int32_t CameraNapiUtils::MapImageRotationFromJs(int32_t jsRotation, PhotoCaptureSetting::RotationConfig &nativeRotation)
{
    MEDIA_INFO_LOG("js rotation = %{public}d", jsRotation);
    switch (jsRotation) {
        case ROTATION_0:
            nativeRotation = PhotoCaptureSetting::Rotation_0;
            break;
        case ROTATION_90:
            nativeRotation = PhotoCaptureSetting::Rotation_90;
            break;
        case ROTATION_180:
            nativeRotation = PhotoCaptureSetting::Rotation_180;
            break;
        case ROTATION_270:
            nativeRotation = PhotoCaptureSetting::Rotation_270;
            break;
        default:
            MEDIA_ERR_LOG("Invalid rotation value received from application");
            return -1;
    }

    return 0;
}

void CameraNapiUtils::MapCameraStatusEnum(CameraStatus deviceStatus, int32_t &jsCameraStatus)
{
    MEDIA_INFO_LOG("native camera status = %{public}d", static_cast<int32_t>(deviceStatus));
    switch (deviceStatus) {
        case CAMERA_DEVICE_STATUS_UNAVAILABLE:
            jsCameraStatus = JS_CAMERA_STATUS_UNAVAILABLE;
            break;
        case CAMERA_DEVICE_STATUS_AVAILABLE:
            jsCameraStatus = JS_CAMERA_STATUS_AVAILABLE;
            break;
        default:
            MEDIA_ERR_LOG("Received native camera status is not supported with JS");
            jsCameraStatus = -1;
    }
}

void CameraNapiUtils::VideoStabilizationModeEnum(
    CameraVideoStabilizationMode nativeVideoStabilizationMode, int32_t &jsVideoStabilizationMode)
{
    MEDIA_INFO_LOG(
        "native video stabilization mode = %{public}d", static_cast<int32_t>(nativeVideoStabilizationMode));
    switch (nativeVideoStabilizationMode) {
        case OHOS_CAMERA_VIDEO_STABILIZATION_OFF:
            jsVideoStabilizationMode = OFF;
            break;
        case OHOS_CAMERA_VIDEO_STABILIZATION_LOW:
            jsVideoStabilizationMode = LOW;
            break;
        case OHOS_CAMERA_VIDEO_STABILIZATION_MIDDLE:
            jsVideoStabilizationMode = MIDDLE;
            break;
        case OHOS_CAMERA_VIDEO_STABILIZATION_HIGH:
            jsVideoStabilizationMode = HIGH;
            break;
        case OHOS_CAMERA_VIDEO_STABILIZATION_AUTO:
            jsVideoStabilizationMode = AUTO;
            break;
        default:
            MEDIA_ERR_LOG("Received native video stabilization mode is not supported with JS");
            jsVideoStabilizationMode = -1;
    }
}

void CameraNapiUtils::CreateNapiErrorObject(napi_env env, int32_t errorCode, const char* errString,
    std::unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_get_undefined(env, &jsContext->data);
    napi_value napiErrorCode = nullptr;
    napi_value napiErrorMsg = nullptr;

    std::string errorCodeStr = std::to_string(errorCode);
    napi_create_string_utf8(env, errorCodeStr.c_str(), NAPI_AUTO_LENGTH, &napiErrorCode);
    napi_create_string_utf8(env, errString, NAPI_AUTO_LENGTH, &napiErrorMsg);

    napi_create_object(env, &jsContext->error);
    napi_set_named_property(env, jsContext->error, "code", napiErrorCode);
    napi_set_named_property(env, jsContext->error, "message", napiErrorMsg);

    jsContext->status = false;
}

void CameraNapiUtils::InvokeJSAsyncMethod(napi_env env, napi_deferred deferred,
    napi_ref callbackRef, napi_async_work work, const JSAsyncContextOutput &asyncContext)
{
    napi_value retVal;
    napi_value callback = nullptr;
    std::string funcName = asyncContext.funcName;
    MEDIA_INFO_LOG("%{public}s, context->InvokeJSAsyncMethod start", funcName.c_str());
    /* Deferred is used when JS Callback method expects a promise value */
    if (deferred) {
        if (asyncContext.status) {
            napi_resolve_deferred(env, deferred, asyncContext.data);
            MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethod napi_resolve_deferred", funcName.c_str());
        } else {
            napi_reject_deferred(env, deferred, asyncContext.error);
            MEDIA_ERR_LOG("%{public}s, InvokeJSAsyncMethod napi_reject_deferred", funcName.c_str());
        }
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethod end deferred", funcName.c_str());
    } else {
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethod callback", funcName.c_str());
        napi_value result[ARGS_TWO];
        result[PARAM0] = asyncContext.error;
        result[PARAM1] = asyncContext.data;
        napi_get_reference_value(env, callbackRef, &callback);
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethod napi_call_function start", funcName.c_str());
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethod napi_call_function end", funcName.c_str());
        napi_delete_reference(env, callbackRef);
    }
    napi_delete_async_work(env, work);
    MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethod inner end", funcName.c_str());
}

int32_t CameraNapiUtils::IncreamentAndGet(uint32_t &num)
{
    int32_t temp = num & 0x00ffffff;
    if (temp >= 0xffff) {
        num = num & 0xff000000;
    }
    num++;
    return num;
}

bool CameraNapiUtils::CheckInvalidArgument(napi_env env, size_t argc, int32_t length,
    napi_value *argv, CameraSteps step)
{
    bool isPass = true;
    napi_valuetype valueTypeArray[length];
    for (int32_t i = 0; i < length; i++) {
        napi_typeof(env, argv[i], &valueTypeArray[i]);
    }
    switch (step) {
        case CREATE_CAMERA_INPUT_INSTANCE:
            if (argc == ARGS_ONE) {
                isPass = valueTypeArray[0] == napi_object;
            } else if (argc == ARGS_TWO) {
                isPass = (valueTypeArray[0] == napi_number) && (valueTypeArray[1] == napi_number);
            } else {
                isPass = false;
            }
            break;

        case CREATE_PREVIEW_OUTPUT_INSTANCE:
            isPass = (argc == ARGS_TWO) &&
                        (valueTypeArray[0] == napi_object) && (valueTypeArray[1] == napi_string);
            break;

        case CREATE_PHOTO_OUTPUT_INSTANCE:
            isPass = (argc == ARGS_TWO) &&
                        (valueTypeArray[0] == napi_object) && (valueTypeArray[1] == napi_string);
            break;

        case CREATE_VIDEO_OUTPUT_INSTANCE:
            isPass = (argc == ARGS_TWO) &&
                        (valueTypeArray[0] == napi_object) && (valueTypeArray[1] == napi_string);
            break;

        case CREATE_METADATA_OUTPUT_INSTANCE:
            isPass = argc == ARGS_ONE;
            if (argc == ARGS_ONE) {
                napi_is_array(env, argv[0], &isPass);
            } else {
                isPass = false;
            }
            break;

        case ADD_INPUT:
            isPass = (argc == ARGS_ONE) && (valueTypeArray[0] == napi_object);
            break;

        case REMOVE_INPUT:
            isPass = (argc == ARGS_ONE) && (valueTypeArray[0] == napi_object);
            break;

        case ADD_OUTPUT:
            isPass = (argc == ARGS_ONE) && (valueTypeArray[0] == napi_object);
            break;

        case REMOVE_OUTPUT:
            isPass = (argc == ARGS_ONE) && (valueTypeArray[0] == napi_object);
            break;

        case PHOTO_OUT_CAPTURE:
            if (argc == ARGS_ZERO) {
                isPass = true;
            } else if (argc == ARGS_ONE) {
                isPass = (valueTypeArray[0] == napi_object) || (valueTypeArray[0] == napi_function) ||
                    (valueTypeArray[0] == napi_undefined);
            } else if (argc == ARGS_TWO) {
                isPass = (valueTypeArray[0] == napi_object) && (valueTypeArray[1] == napi_function);
            } else {
                isPass = false;
            }
            break;

        case ADD_DEFERRED_SURFACE:
            if (argc == ARGS_ONE) {
                isPass = valueTypeArray[0] == napi_string;
            } else if (argc == ARGS_TWO) {
                isPass = (valueTypeArray[0] == napi_string) && (valueTypeArray[1] == napi_function);
            } else {
                isPass = false;
            }
            break;

        case CREATE_DEFERRED_PREVIEW_OUTPUT:
            if (argc == ARGS_ONE) {
                isPass = valueTypeArray[0] == napi_object;
            }  else {
                isPass = false;
            }
            break;

        default:
            break;
    }
    if (!isPass) {
        std::string errorCode = std::to_string(CameraErrorCode::INVALID_ARGUMENT);
        napi_throw_type_error(env, errorCode.c_str(), "");
    }
    return isPass;
}

bool CameraNapiUtils::CheckError(napi_env env, int32_t retCode)
{
    if ((retCode != 0)) {
        std::string errorCode = std::to_string(retCode);
        napi_throw_error(env, errorCode.c_str(), "");
        return false;
    }
    return true;
}

double CameraNapiUtils::FloatToDouble(float val)
{
    const double precision = 1000000.0;
    val *= precision;
    double result = static_cast<double>(val / precision);
    return result;
}

bool CameraNapiUtils::CheckSystemApp(napi_env env)
{
    uint64_t tokenId = IPCSkeleton::GetSelfTokenID();
    int32_t errorCode = CameraErrorCode::NO_SYSTEM_APP_PERMISSION;
    std::string errorMessage = "System api can be invoked only by system applications";
    if (!Security::AccessToken::TokenIdKit::IsSystemAppByFullTokenID(tokenId)) {
        if (napi_throw_error(env, std::to_string(errorCode).c_str(), errorMessage.c_str()) != napi_ok) {
            MEDIA_ERR_LOG("failed to throw err, code=%{public}d, msg=%{public}s.", errorCode, errorMessage.c_str());
        }
        return false;
    }
    return true;
}
} // namespace CameraStandard
} // namespace OHOS
