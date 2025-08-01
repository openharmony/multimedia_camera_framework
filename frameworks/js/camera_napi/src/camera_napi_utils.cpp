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

#include <map>
#include "camera_napi_utils.h"
#include "camera_error_code.h"
#include "camera_log.h"
#include "napi/native_api.h"

namespace OHOS {
namespace CameraStandard {
bool CameraNapiUtils::mEnableSecure = false;
static const std::map<int32_t, std::string> errorCodeToMessage = {
    {7400102, "Operation not allowed."},
    {7400103, "Session not config."},
    {7400201, "Camera service fatal error."},
};

std::string CameraNapiUtils::GetErrorMessage(int32_t errorCode)
{
    auto it = errorCodeToMessage.find(errorCode);
    if (it != errorCodeToMessage.end()) {
        return it->second;
    } else {
        return "";
    }
}

void CameraNapiUtils::CreateNapiErrorObject(napi_env env, int32_t errorCode, const char* errString,
    std::unique_ptr<JSAsyncContextOutput> &jsContext)
{
    napi_get_undefined(env, &jsContext->data);
    napi_value napiErrorCode = nullptr;
    napi_value napiErrorMsg = nullptr;
    if (errString == nullptr || strlen(errString) == 0) {
        napi_create_string_utf8(env, GetErrorMessage(errorCode).c_str(), NAPI_AUTO_LENGTH, &napiErrorMsg);
    } else {
        napi_create_string_utf8(env, errString, NAPI_AUTO_LENGTH, &napiErrorMsg);
    }
    std::string errorCodeStr = std::to_string(errorCode);
    napi_create_string_utf8(env, errorCodeStr.c_str(), NAPI_AUTO_LENGTH, &napiErrorCode);

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

void CameraNapiUtils::InvokeJSAsyncMethodWithUvWork(napi_env env, napi_deferred deferred,
    napi_ref callbackRef, const JSAsyncContextOutput &asyncContext)
{
    napi_value retVal;
    napi_value callback = nullptr;
    std::string funcName = asyncContext.funcName;
    MEDIA_INFO_LOG("%{public}s, context->InvokeJSAsyncMethodWithUvWork start", funcName.c_str());
    /* Deferred is used when JS Callback method expects a promise value */
    if (deferred) {
        if (asyncContext.status) {
            napi_resolve_deferred(env, deferred, asyncContext.data);
            MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethodWithUvWork napi_resolve_deferred", funcName.c_str());
        } else {
            napi_reject_deferred(env, deferred, asyncContext.error);
            MEDIA_ERR_LOG("%{public}s, InvokeJSAsyncMethodWithUvWork napi_reject_deferred", funcName.c_str());
        }
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethodWithUvWork end deferred", funcName.c_str());
    } else {
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethodWithUvWork callback", funcName.c_str());
        napi_value result[ARGS_TWO];
        result[PARAM0] = asyncContext.error;
        result[PARAM1] = asyncContext.data;
        napi_get_reference_value(env, callbackRef, &callback);
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethodWithUvWork napi_call_function start", funcName.c_str());
        napi_call_function(env, nullptr, callback, ARGS_TWO, result, &retVal);
        MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethodWithUvWork napi_call_function end", funcName.c_str());
        napi_delete_reference(env, callbackRef);
    }
    MEDIA_INFO_LOG("%{public}s, InvokeJSAsyncMethodWithUvWork inner end", funcName.c_str());
}

int32_t CameraNapiUtils::IncrementAndGet(uint32_t& num)
{
    int32_t temp = num & 0x00ffffff;
    if (temp >= 0xffff) {
        num = num & 0xff000000;
    }
    num++;
    return num;
}

bool CameraNapiUtils::GetEnableSecureCamera()
{
    return mEnableSecure;
}

void CameraNapiUtils::IsEnableSecureCamera(bool isEnable)
{
    mEnableSecure = isEnable;
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
            isPass = (argc >= ARGS_ONE) && (valueTypeArray[0] == napi_object);
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

        case PHOTO_OUT_BURST_CAPTURE:
            if (argc == ARGS_ONE) {
                isPass = valueTypeArray[0] == napi_object;
            }  else {
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
        napi_throw_type_error(env, errorCode.c_str(), "Parameter missing or parameter type incorrect.");
    }
    return isPass;
}

bool CameraNapiUtils::CheckError(napi_env env, int32_t retCode)
{
    if ((retCode != 0)) {
        std::string errorCode = std::to_string(retCode);
        napi_throw_error(env, errorCode.c_str(), GetErrorMessage(retCode).c_str());
        return false;
    }
    return true;
}

double CameraNapiUtils::FloatToDouble(float val)
{
    const double precision = 1000000.0;
    return std::round(val * precision) / precision;
}

std::string CameraNapiUtils::GetStringArgument(napi_env env, napi_value value)
{
    napi_valuetype valueNapiType = napi_undefined;
    napi_typeof(env, value, &valueNapiType);
    CHECK_RETURN_RET(valueNapiType != napi_string, "");
    size_t stringSize = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &stringSize);
    CHECK_RETURN_RET(status != napi_ok || stringSize == 0, "");
    std::string strValue = std::string(stringSize, '\0');
    status = napi_get_value_string_utf8(env, value, strValue.data(), stringSize + 1, &stringSize);
    CHECK_RETURN_RET(status != napi_ok || stringSize == 0, "");
    return strValue;
}

bool CameraNapiUtils::IsSameNapiValue(napi_env env, napi_value valueSrc, napi_value valueDst)
{
    CHECK_RETURN_RET(valueSrc == nullptr && valueDst == nullptr, true);
    CHECK_RETURN_RET(valueSrc == nullptr || valueDst == nullptr, false);
    bool isEquals = false;
    CHECK_RETURN_RET_ELOG(napi_strict_equals(env, valueSrc, valueDst, &isEquals) != napi_ok, false,
        "get napi_strict_equals failed");
    return isEquals;
}

napi_status CameraNapiUtils::CallPromiseFun(
    napi_env env, napi_value promiseValue, void* data, napi_callback thenCallback, napi_callback catchCallback)
{
    MEDIA_DEBUG_LOG("CallPromiseFun Start");
    bool isPromise = false;
    napi_is_promise(env, promiseValue, &isPromise);
    CHECK_RETURN_RET_ELOG(!isPromise, napi_invalid_arg, "CallPromiseFun promiseValue is not promise");
    // Create promiseThen
    napi_value promiseThen = nullptr;
    napi_get_named_property(env, promiseValue, "then", &promiseThen);
    CHECK_RETURN_RET_ELOG(promiseThen == nullptr, napi_invalid_arg, "CallPromiseFun get promiseThen failed");
    napi_value thenValue;
    napi_status ret = napi_create_function(env, "thenCallback", NAPI_AUTO_LENGTH, thenCallback, data, &thenValue);
    CHECK_RETURN_RET_ELOG(ret != napi_ok, ret, "CallPromiseFun thenCallback got exception");
    napi_value catchValue;
    ret = napi_create_function(env, "catchCallback", NAPI_AUTO_LENGTH, catchCallback, data, &catchValue);
    CHECK_RETURN_RET_ELOG(ret != napi_ok, ret, "CallPromiseFun catchCallback got exception");
    napi_value thenReturnValue;
    constexpr uint32_t THEN_ARGC = 2;
    napi_value thenArgv[THEN_ARGC] = { thenValue, catchValue };
    ret = napi_call_function(env, promiseValue, promiseThen, THEN_ARGC, thenArgv, &thenReturnValue);
    CHECK_RETURN_RET_ELOG(ret != napi_ok, ret, "CallPromiseFun PromiseThen got exception");
    MEDIA_DEBUG_LOG("CallPromiseFun End");
    return napi_ok;
}

std::vector<napi_property_descriptor> CameraNapiUtils::GetPropertyDescriptor(
    std::vector<std::vector<napi_property_descriptor>> descriptors)
{
    MEDIA_DEBUG_LOG("GetPropertyDescriptor Start %zu", descriptors.size());
    std::vector<napi_property_descriptor> properties = {};
    std::for_each(descriptors.begin(), descriptors.end(),
                  [&properties](const std::vector<napi_property_descriptor> descriptor) {
                      properties.insert(properties.end(), descriptor.begin(), descriptor.end());
                  });
    return properties;
};

napi_status CameraNapiUtils::CreateObjectWithPropName(
    napi_env env, napi_value* result, size_t property_count, const char** keys)
{
    napi_value values[property_count];
    for (size_t i = 0; i < property_count; i++) {
        napi_get_undefined(env, &values[i]);
    }
    return napi_create_object_with_named_properties(env, result, property_count, keys, values);
}

napi_status CameraNapiUtils::CreateObjectWithPropNameAndValues(
    napi_env env, napi_value* result, size_t property_count, const char** keys, const std::vector<std::string> values)
{
    napi_value napiValues[property_count];
    for (size_t i = 0; i < property_count; i++) {
        napi_create_string_utf8(env, values[i].data(), values[i].size(), &napiValues[i]);
    }
    return napi_create_object_with_named_properties(env, result, property_count, keys, napiValues);
}

size_t CameraNapiUtils::GetNapiArgs(napi_env env, napi_callback_info callbackInfo)
{
    size_t argsSize = 0;
    napi_get_cb_info(env, callbackInfo, &argsSize, nullptr, nullptr, nullptr);
    return argsSize;
}

void CameraNapiUtils::CreateFrameRateJSArray(napi_env env, std::vector<int32_t> frameRateRange, napi_value &result)
{
    MEDIA_DEBUG_LOG("CreateFrameRateJSArray called");
    CHECK_PRINT_ELOG(frameRateRange.empty(), "frameRateRange is empty");

    napi_status status = napi_create_object(env, &result);
    if (status == napi_ok) {
        napi_value minRate;
        status = napi_create_int32(env, frameRateRange[0], &minRate);
        napi_value maxRate;
        status = napi_create_int32(env, frameRateRange[1], &maxRate);
        CHECK_PRINT_ELOG(status != napi_ok || napi_set_named_property(env, result, "min", minRate) != napi_ok ||
            napi_set_named_property(env, result, "max", maxRate) != napi_ok,
            "Failed to create frameRateArray with napi wrapper object.");
    }
}

napi_value CameraNapiUtils::CreateSupportFrameRatesJSArray(
    napi_env env, std::vector<std::vector<int32_t>> supportedFrameRatesRange)
{
    MEDIA_DEBUG_LOG("CreateFrameRateJSArray called");
    napi_value supportedFrameRateArray = nullptr;
    CHECK_PRINT_ELOG(supportedFrameRatesRange.empty(), "frameRateRange is empty");

    napi_status status = napi_create_array(env, &supportedFrameRateArray);
    if (status == napi_ok) {
        for (size_t i = 0; i < supportedFrameRatesRange.size(); i++) {
            napi_value supportedFrameRateItem;
            CreateFrameRateJSArray(env, supportedFrameRatesRange[i], supportedFrameRateItem);
            CHECK_RETURN_RET_ELOG(napi_set_element(env, supportedFrameRateArray, i, supportedFrameRateItem) !=
                napi_ok, nullptr, "Failed to create supportedFrameRateArray with napi wrapper object.");
        }
    }
    return supportedFrameRateArray;
}

napi_value CameraNapiUtils::ProcessingPhysicalApertures(napi_env env, std::vector<std::vector<float>> physicalApertures)
{
    napi_value result = nullptr;
    napi_create_array(env, &result);
    size_t zoomRangeSize = 2;
    size_t zoomMinIndex = 0;
    size_t zoomMaxIndex = 1;
    for (size_t i = 0; i < physicalApertures.size(); i++) {
        if (physicalApertures[i].size() <= zoomRangeSize) {
            continue;
        }
        napi_value zoomRange;
        napi_create_array(env, &zoomRange);
        napi_value physicalApertureRange;
        napi_create_array(env, &physicalApertureRange);
        for (size_t y = 0; y < physicalApertures[i].size(); y++) {
            napi_value value;
            napi_create_double(env, CameraNapiUtils::FloatToDouble(physicalApertures[i][y]), &value);
            if (y == zoomMinIndex) {
                napi_set_element(env, zoomRange, y, value);
                napi_set_named_property(env, zoomRange, "min", value);
                continue;
            }
            if (y == zoomMaxIndex) {
                napi_set_element(env, zoomRange, y, value);
                napi_set_named_property(env, zoomRange, "max", value);
                continue;
            }
            napi_set_element(env, physicalApertureRange, y - zoomRangeSize, value);
        }
        napi_value obj;
        napi_create_object(env, &obj);
        napi_set_named_property(env, obj, "zoomRange", zoomRange);
        napi_set_named_property(env, obj, "apertures", physicalApertureRange);
        napi_set_element(env, result, i, obj);
    }
    return result;
}

napi_value CameraNapiUtils::CreateJSArray(napi_env env, napi_status &status,
    std::vector<int32_t> nativeArray)
{
    MEDIA_DEBUG_LOG("CameraNapiUtils::CreateJSArray is called");
 
    napi_value item = nullptr;
    napi_value jsArray = nullptr;
 
    if (nativeArray.empty()) {
        MEDIA_ERR_LOG("nativeArray is empty");
        for (size_t i = 0; i < nativeArray.size(); i++) {
            napi_create_int32(env, nativeArray[i], &item);
            if (napi_set_element(env, jsArray, i, item) != napi_ok) {
                MEDIA_ERR_LOG("CameraNapiUtils::CreateJSArray Failed to create profile napi wrapper object");
                return nullptr;
            }
        }
    }
    return jsArray;
}

napi_value CameraNapiUtils::ParseMetadataObjectTypes(napi_env env, napi_value arrayParam,
    std::vector<MetadataObjectType> &metadataObjectTypes)
{
    MEDIA_DEBUG_LOG("ParseMetadataObjectTypes is called");
    napi_value result;
    uint32_t length = 0;
    napi_value value;
    int32_t metadataType = 0;
    napi_get_array_length(env, arrayParam, &length);
    napi_valuetype type = napi_undefined;
    const int32_t invalidType = -1;
    for (uint32_t i = 0; i < length; i++) {
        napi_get_element(env, arrayParam, i, &value);
        napi_typeof(env, value, &type);
        CHECK_RETURN_RET(type != napi_number, nullptr);
        napi_get_value_int32(env, value, &metadataType);
        if (metadataType < static_cast<int32_t>(MetadataObjectType::INVALID) &&
            metadataType > static_cast<int32_t>(MetadataObjectType::BASE_TRACKING_REGION)) {
            metadataType = invalidType;
        }
        metadataObjectTypes.push_back(static_cast<MetadataObjectType>(metadataType));
    }
    napi_get_boolean(env, true, &result);
    return result;
}
} // namespace CameraStandard
} // namespace OHOS
