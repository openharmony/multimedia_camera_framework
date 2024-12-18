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

#include "input/camera_input_napi.h"

#include <cstdint>
#include <memory>
#include <uv.h>

#include "camera_device.h"
#include "camera_error_code.h"
#include "camera_log.h"
#include "camera_napi_const.h"
#include "camera_napi_param_parser.h"
#include "camera_napi_security_utils.h"
#include "camera_napi_utils.h"
#include "camera_napi_worker_queue_keeper.h"
#include "input/camera_napi.h"
#include "js_native_api.h"
#include "napi/native_common.h"

namespace OHOS {
namespace CameraStandard {
namespace {
void AsyncCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<CameraInputAsyncContext*>(data);
    CHECK_ERROR_RETURN_LOG(context == nullptr, "CameraInputNapi AsyncCompleteCallback context is null");
    MEDIA_INFO_LOG("CameraInputNapi AsyncCompleteCallback %{public}s, status = %{public}d", context->funcName.c_str(),
        context->status);
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = context->status;
    if (!context->status) {
        CameraNapiUtils::CreateNapiErrorObject(env, context->errorCode, context->errorMsg.c_str(), jsContext);
    } else {
        if (context->isEnableSecCam) {
            napi_create_bigint_uint64(env, context->secureCameraSeqId, &jsContext->data);
        } else {
            napi_get_undefined(env, &jsContext->data);
        }
    }
    if (!context->funcName.empty() && context->taskId > 0) {
        // Finish async trace
        CAMERA_FINISH_ASYNC_TRACE(context->funcName, context->taskId);
        jsContext->funcName = context->funcName.c_str();
    }
    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    context->FreeHeldNapiValue(env);
    delete context;
}
} // namespace

using namespace std;
thread_local napi_ref CameraInputNapi::sConstructor_ = nullptr;
thread_local sptr<CameraInput> CameraInputNapi::sCameraInput_ = nullptr;
thread_local uint32_t CameraInputNapi::cameraInputTaskId = CAMERA_INPUT_TASKID;

void ErrorCallbackListener::OnErrorCallbackAsync(const int32_t errorType, const int32_t errorMsg) const
{
    MEDIA_DEBUG_LOG("OnErrorCallbackAsync is called");
    uv_loop_s* loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("failed to get event loop");
        return;
    }
    uv_work_t* work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("failed to allocate work");
        return;
    }
    std::unique_ptr<ErrorCallbackInfo> callbackInfo =
        std::make_unique<ErrorCallbackInfo>(errorType, errorMsg, shared_from_this());
    work->data = callbackInfo.get();
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t* work) {}, [] (uv_work_t* work, int status) {
        ErrorCallbackInfo* callbackInfo = reinterpret_cast<ErrorCallbackInfo *>(work->data);
        if (callbackInfo) {
            auto listener = callbackInfo->listener_.lock();
            if (listener) {
                listener->OnErrorCallback(callbackInfo->errorType_, callbackInfo->errorMsg_);
            }
            delete callbackInfo;
        }
        delete work;
    }, uv_qos_user_initiated);
    if (ret) {
        MEDIA_ERR_LOG("failed to execute work");
        delete work;
    } else {
        callbackInfo.release();
    }
}

void ErrorCallbackListener::OnErrorCallback(const int32_t errorType, const int32_t errorMsg) const
{
    MEDIA_DEBUG_LOG("OnErrorCallback is called");
    napi_value result;
    napi_value retVal;
    napi_value propValue;

    napi_create_int32(env_, errorType, &propValue);
    napi_create_object(env_, &result);
    napi_set_named_property(env_, result, "code", propValue);
    ExecuteCallbackNapiPara callbackNapiPara { .recv = nullptr, .argc = ARGS_ONE, .argv = &result, .result = &retVal };
    ExecuteCallback("error", callbackNapiPara);
}

void ErrorCallbackListener::OnError(const int32_t errorType, const int32_t errorMsg) const
{
    MEDIA_DEBUG_LOG("OnError is called!, errorType: %{public}d", errorType);
    OnErrorCallbackAsync(errorType, errorMsg);
}

void OcclusionDetectCallbackListener::OnCameraOcclusionDetectedCallback(const uint8_t isCameraOcclusion,
    const uint8_t isCameraLensDirty) const
{
    MEDIA_DEBUG_LOG("OnCameraOcclusionDetectedCallback is called");
    napi_value result[ARGS_TWO];
    napi_value retVal;
    napi_value propValue;

    napi_get_undefined(env_, &result[PARAM0]);
    napi_create_object(env_, &result[PARAM1]);
    napi_get_boolean(env_, isCameraOcclusion == 1 ? true : false, &propValue);
    napi_set_named_property(env_, result[PARAM1], "isCameraOccluded", propValue);

    napi_value propValueForLensDirty = nullptr;
    napi_get_boolean(env_, isCameraLensDirty == 1 ? true : false, &propValueForLensDirty);
    napi_set_named_property(env_, result[PARAM1], "isCameraLensDirty", propValueForLensDirty);

    ExecuteCallbackNapiPara callbackNapiPara { .recv = nullptr, .argc = ARGS_TWO, .argv = result, .result = &retVal };
    ExecuteCallback("cameraOcclusionDetect", callbackNapiPara);
}
 
void OcclusionDetectCallbackListener::OnCameraOcclusionDetectedCallbackAsync(
    const uint8_t isCameraOcclusion, const uint8_t isCameraLensDirty) const
{
    MEDIA_DEBUG_LOG("OnCameraOcclusionDetectedCallbackAsync is called");
    uv_loop_s* loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    if (!loop) {
        MEDIA_ERR_LOG("failed to get event loop");
        return;
    }
    uv_work_t* work = new(std::nothrow) uv_work_t;
    if (!work) {
        MEDIA_ERR_LOG("failed to allocate work");
        return;
    }
    std::unique_ptr<CameraOcclusionDetectResult> callbackInfo =
        std::make_unique<CameraOcclusionDetectResult>(isCameraOcclusion, isCameraLensDirty, shared_from_this());
    work->data = callbackInfo.get();
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t* work) {}, [] (uv_work_t* work, int status) {
        CameraOcclusionDetectResult* callbackInfo = reinterpret_cast<CameraOcclusionDetectResult *>(work->data);
        if (callbackInfo) {
            auto listener = callbackInfo->listener_.lock();
            if (listener) {
                listener->OnCameraOcclusionDetectedCallback(callbackInfo->isCameraOccluded_,
                                                            callbackInfo->isCameraLensDirty_);
            }
            delete callbackInfo;
        }
        delete work;
    }, uv_qos_user_initiated);
    if (ret) {
        MEDIA_ERR_LOG("failed to execute work");
        delete work;
    } else {
        callbackInfo.release();
    }
}
 
void OcclusionDetectCallbackListener::OnCameraOcclusionDetected(const uint8_t isCameraOcclusion,
    const uint8_t isCameraLensDirty) const
{
    MEDIA_DEBUG_LOG("OnCameraOcclusionDetected is called!, "
                    "isCameraOcclusion: %{public}u, isCameraLensDirty: %{public}u",
                    isCameraOcclusion, isCameraLensDirty);
    OnCameraOcclusionDetectedCallbackAsync(isCameraOcclusion, isCameraLensDirty);
}

CameraInputNapi::CameraInputNapi() : env_(nullptr)
{
}

CameraInputNapi::~CameraInputNapi()
{
    MEDIA_INFO_LOG("~CameraInputNapi is called");
}

void CameraInputNapi::CameraInputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    MEDIA_INFO_LOG("CameraInputNapiDestructor is called");
    CameraInputNapi* cameraObj = reinterpret_cast<CameraInputNapi*>(nativeObject);
    if (cameraObj != nullptr) {
        delete cameraObj;
    }
}

napi_value CameraInputNapi::Init(napi_env env, napi_value exports)
{
    MEDIA_DEBUG_LOG("Init is called");
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    // todo: Open and Close in native have not implemented
    napi_property_descriptor camera_input_props[] = {
        DECLARE_NAPI_FUNCTION("open", Open),
        DECLARE_NAPI_FUNCTION("close", Close),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("once", Once),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("usedAsPosition", UsedAsPosition)
    };

    status = napi_define_class(env, CAMERA_INPUT_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               CameraInputNapiConstructor, nullptr,
                               sizeof(camera_input_props) / sizeof(camera_input_props[PARAM0]),
                               camera_input_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_INPUT_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }
    MEDIA_ERR_LOG("Init call Failed!");
    return nullptr;
}

// Constructor callback
napi_value CameraInputNapi::CameraInputNapiConstructor(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("CameraInputNapiConstructor is called");
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<CameraInputNapi> obj = std::make_unique<CameraInputNapi>();
        obj->env_ = env;
        obj->cameraInput_ = sCameraInput_;
        status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                           CameraInputNapi::CameraInputNapiDestructor, nullptr, nullptr);
        if (status == napi_ok) {
            obj.release();
            return thisVar;
        } else {
            MEDIA_ERR_LOG("Failure wrapping js to native napi");
        }
    }
    MEDIA_ERR_LOG("CameraInputNapiConstructor call Failed!");
    return result;
}

napi_value CameraInputNapi::CreateCameraInput(napi_env env, sptr<CameraInput> cameraInput)
{
    MEDIA_INFO_LOG("CreateCameraInput is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;
    if (cameraInput == nullptr) {
        return result;
    }
    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        sCameraInput_ = cameraInput;
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sCameraInput_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create Camera input instance");
        }
    }
    napi_get_undefined(env, &result);
    MEDIA_ERR_LOG("CreateCameraInput call Failed!");
    return result;
}

sptr<CameraInput> CameraInputNapi::GetCameraInput()
{
    return cameraInput_;
}

napi_value CameraInputNapi::Open(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("Open is called");
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>(
        "CameraInputNapi::Open", CameraNapiUtils::IncrementAndGet(cameraInputTaskId));
    bool isEnableSecureCamera = false;
    auto asyncFunction =
        std::make_shared<CameraNapiAsyncFunction>(env, "Open", asyncContext->callbackRef, asyncContext->deferred);
    CameraNapiParamParser jsParamParser(env, info, asyncContext->objectInfo, asyncFunction, isEnableSecureCamera);
    if (jsParamParser.IsStatusOk()) {
        CameraNapiUtils::IsEnableSecureCamera(isEnableSecureCamera);
        MEDIA_DEBUG_LOG("set  EnableSecureCamera CameraInputNapi::Open");
    } else {
        MEDIA_WARNING_LOG("CameraInputNapi::Open check secure parameter fail, try open without secure flag");
        jsParamParser = CameraNapiParamParser(env, info, asyncContext->objectInfo, asyncFunction);
    }
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "invalid argument")) {
        MEDIA_ERR_LOG("CameraInputNapi::Open invalid argument");
        return nullptr;
    }
    asyncContext->HoldNapiValue(env, jsParamParser.GetThisVar());
    napi_status status = napi_create_async_work(
        env, nullptr, asyncFunction->GetResourceName(),
        [](napi_env env, void* data) {
            MEDIA_INFO_LOG("CameraInputNapi::Open running on worker");
            auto context = static_cast<CameraInputAsyncContext*>(data);
            CHECK_ERROR_RETURN_LOG(context->objectInfo == nullptr, "CameraInputNapi::Open async info is nullptr");
            CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
            CameraNapiWorkerQueueKeeper::GetInstance()->ConsumeWorkerQueueTask(context->queueTask, [&context]() {
                context->isEnableSecCam = CameraNapiUtils::GetEnableSecureCamera();
                MEDIA_DEBUG_LOG("CameraInputNapi::Open context->isEnableSecCam %{public}d", context->isEnableSecCam);
                if (context->isEnableSecCam) {
                    context->errorCode = context->objectInfo->GetCameraInput()->Open(true, &context->secureCameraSeqId);
                    MEDIA_INFO_LOG("CameraInputNapi::Open, SeqId = %{public}" PRIu64 "", context->secureCameraSeqId);
                } else {
                    context->errorCode = context->objectInfo->GetCameraInput()->Open();
                }
                context->status = context->errorCode == CameraErrorCode::SUCCESS;
                CameraNapiUtils::IsEnableSecureCamera(false);
            });
        },
        AsyncCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraInputNapi::Open");
        asyncFunction->Reset();
    } else {
        asyncContext->queueTask =
            CameraNapiWorkerQueueKeeper::GetInstance()->AcquireWorkerQueueTask("CameraInputNapi::Open");
        napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
        asyncContext.release();
    }
    if (asyncFunction->GetAsyncFunctionType() == ASYNC_FUN_TYPE_PROMISE) {
        return asyncFunction->GetPromise();
    }
    return CameraNapiUtils::GetUndefinedValue(env);
}

napi_value CameraInputNapi::Close(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("Close is called");
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>(
        "CameraInputNapi::Close", CameraNapiUtils::IncrementAndGet(cameraInputTaskId));
    auto asyncFunction =
        std::make_shared<CameraNapiAsyncFunction>(env, "Close", asyncContext->callbackRef, asyncContext->deferred);
    CameraNapiParamParser jsParamParser(env, info, asyncContext->objectInfo, asyncFunction);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "invalid argument")) {
        MEDIA_ERR_LOG("CameraInputNapi::Close invalid argument");
        return nullptr;
    }
    asyncContext->HoldNapiValue(env, jsParamParser.GetThisVar());
    napi_status status = napi_create_async_work(
        env, nullptr, asyncFunction->GetResourceName(),
        [](napi_env env, void* data) {
            MEDIA_INFO_LOG("CameraInputNapi::Close running on worker");
            auto context = static_cast<CameraInputAsyncContext*>(data);
            CHECK_ERROR_RETURN_LOG(context->objectInfo == nullptr, "CameraInputNapi::Close async info is nullptr");
            CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
            CameraNapiWorkerQueueKeeper::GetInstance()->ConsumeWorkerQueueTask(context->queueTask, [&context]() {
                context->errorCode = context->objectInfo->GetCameraInput()->Close();
                context->status = context->errorCode == CameraErrorCode::SUCCESS;
                CameraNapiUtils::IsEnableSecureCamera(false);
            });
        },
        AsyncCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraInputNapi::Close");
        asyncFunction->Reset();
    } else {
        asyncContext->queueTask =
            CameraNapiWorkerQueueKeeper::GetInstance()->AcquireWorkerQueueTask("CameraInputNapi::Close");
        napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
        asyncContext.release();
    }
    if (asyncFunction->GetAsyncFunctionType() == ASYNC_FUN_TYPE_PROMISE) {
        return asyncFunction->GetPromise();
    }
    return CameraNapiUtils::GetUndefinedValue(env);
}

napi_value CameraInputNapi::Release(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("Release is called");
    std::unique_ptr<CameraInputAsyncContext> asyncContext = std::make_unique<CameraInputAsyncContext>(
        "CameraInputNapi::Release", CameraNapiUtils::IncrementAndGet(cameraInputTaskId));
    auto asyncFunction =
        std::make_shared<CameraNapiAsyncFunction>(env, "Release", asyncContext->callbackRef, asyncContext->deferred);
    CameraNapiParamParser jsParamParser(env, info, asyncContext->objectInfo, asyncFunction);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "invalid argument")) {
        MEDIA_ERR_LOG("CameraInputNapi::Release invalid argument");
        return nullptr;
    }
    asyncContext->HoldNapiValue(env, jsParamParser.GetThisVar());
    napi_status status = napi_create_async_work(
        env, nullptr, asyncFunction->GetResourceName(),
        [](napi_env env, void* data) {
            MEDIA_INFO_LOG("CameraInputNapi::Release running on worker");
            auto context = static_cast<CameraInputAsyncContext*>(data);
            CHECK_ERROR_RETURN_LOG(context->objectInfo == nullptr, "CameraInputNapi::Release async info is nullptr");
            CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
            CameraNapiWorkerQueueKeeper::GetInstance()->ConsumeWorkerQueueTask(context->queueTask, [&context]() {
                context->errorCode = context->objectInfo->GetCameraInput()->Release();
                context->status = context->errorCode == CameraErrorCode::SUCCESS;
            });
        },
        AsyncCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to create napi_create_async_work for CameraInputNapi::Release");
        asyncFunction->Reset();
    } else {
        asyncContext->queueTask =
            CameraNapiWorkerQueueKeeper::GetInstance()->AcquireWorkerQueueTask("CameraInputNapi::Release");
        napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
        asyncContext.release();
    }
    if (asyncFunction->GetAsyncFunctionType() == ASYNC_FUN_TYPE_PROMISE) {
        return asyncFunction->GetPromise();
    }
    return CameraNapiUtils::GetUndefinedValue(env);
}

void CameraInputNapi::RegisterErrorCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args, bool isOnce)
{
    MEDIA_INFO_LOG("CameraInputNapi::RegisterErrorCallbackListener arg size is %{public}zu", args.size());
    CameraNapiObject emptyDevice { {} };
    CameraNapiParamParser jsParamParser(env, args, emptyDevice);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "Could not able to read cameraDevice argument!")) {
        MEDIA_ERR_LOG("CameraInputNapi::RegisterErrorCallbackListener Could not able to read cameraDevice argument!");
        return;
    }

    // Set callback for error
    if (errorCallback_ == nullptr) {
        errorCallback_ = make_shared<ErrorCallbackListener>(env);
        cameraInput_->SetErrorCallback(errorCallback_);
    }
    errorCallback_->SaveCallbackReference(eventName, callback, isOnce);
    MEDIA_INFO_LOG("CameraInputNapi::RegisterErrorCallbackListener success");
}

void CameraInputNapi::UnregisterErrorCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args)
{
    MEDIA_INFO_LOG("CameraInputNapi::UnregisterErrorCallbackListener arg size is %{public}zu", args.size());
    CameraNapiObject emptyDevice { {} };
    CameraNapiParamParser jsParamParser(env, args, emptyDevice);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "Could not able to read cameraDevice argument!")) {
        MEDIA_ERR_LOG("CameraInputNapi::UnregisterErrorCallbackListener Could not able to read cameraDevice argument!");
        return;
    }

    if (errorCallback_ == nullptr) {
        MEDIA_ERR_LOG("errorCallback is null");
        return;
    }
    errorCallback_->RemoveCallbackRef(eventName, callback);
    MEDIA_INFO_LOG("CameraInputNapi::UnregisterErrorCallbackListener success");
}

void CameraInputNapi::RegisterOcclusionDetectCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args, bool isOnce)
{
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi RegisterOcclusionDetectCallbackListener is called!");
        return;
    }
    if (occlusionDetectCallback_ == nullptr) {
        occlusionDetectCallback_ = make_shared<OcclusionDetectCallbackListener>(env);
        cameraInput_->SetOcclusionDetectCallback(occlusionDetectCallback_);
    }
    occlusionDetectCallback_->SaveCallbackReference(eventName, callback, isOnce);
    MEDIA_INFO_LOG("CameraInputNapi::RegisterOcclusionDetectCallbackListener success");
}
 
void CameraInputNapi::UnregisterOcclusionDetectCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args)
{
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi UnregisterOcclusionDetectCallbackListener is called!");
        return;
    }
    if (occlusionDetectCallback_ == nullptr) {
        MEDIA_ERR_LOG("occlusionDetectCallback is null");
        return;
    }
    occlusionDetectCallback_->RemoveCallbackRef(eventName, callback);
    MEDIA_INFO_LOG("CameraInputNapi::RegisterOcclusionDetectCallbackListener success");
}

const CameraInputNapi::EmitterFunctions& CameraInputNapi::GetEmitterFunctions()
{
    static const EmitterFunctions funMap = {
        { "error", {
            &CameraInputNapi::RegisterErrorCallbackListener,
            &CameraInputNapi::UnregisterErrorCallbackListener } },
        { "cameraOcclusionDetect", {
            &CameraInputNapi::RegisterOcclusionDetectCallbackListener,
            &CameraInputNapi::UnregisterOcclusionDetectCallbackListener } } };
    return funMap;
}

napi_value CameraInputNapi::On(napi_env env, napi_callback_info info)
{
    return ListenerTemplate<CameraInputNapi>::On(env, info);
}

napi_value CameraInputNapi::Once(napi_env env, napi_callback_info info)
{
    return ListenerTemplate<CameraInputNapi>::Once(env, info);
}

napi_value CameraInputNapi::Off(napi_env env, napi_callback_info info)
{
    return ListenerTemplate<CameraInputNapi>::Off(env, info);
}

napi_value CameraInputNapi::UsedAsPosition(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("CameraInputNapi::UsedAsPosition is called");
    CameraInputNapi* cameraInputNapi = nullptr;
    int32_t cameraPosition;
    CameraNapiParamParser jsParamParser(env, info, cameraInputNapi, cameraPosition);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "input usedAsPosition with invalid arguments!")) {
        MEDIA_ERR_LOG("CameraInputNapi::UsedAsPosition invalid arguments!");
        return nullptr;
    }
    MEDIA_INFO_LOG("CameraInputNapi::UsedAsPosition params: %{public}d", cameraPosition);
    cameraInputNapi->cameraInput_->SetInputUsedAsPosition(static_cast<const CameraPosition>(cameraPosition));
    return CameraNapiUtils::GetUndefinedValue(env);
}
} // namespace CameraStandard
} // namespace OHOS
