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

#include "output/video_output_napi.h"

#include <uv.h>

#include "camera_napi_const.h"
#include "camera_napi_object_types.h"
#include "camera_napi_param_parser.h"
#include "camera_napi_security_utils.h"
#include "camera_napi_template_utils.h"
#include "camera_napi_utils.h"
#include "camera_napi_worker_queue_keeper.h"
#include "camera_output_capability.h"
#include "listener_base.h"
#include "napi/native_api.h"
#include "napi/native_common.h"

namespace OHOS {
namespace CameraStandard {
namespace {
void AsyncCompleteCallback(napi_env env, napi_status status, void* data)
{
    auto context = static_cast<VideoOutputAsyncContext*>(data);
    CHECK_ERROR_RETURN_LOG(context == nullptr, "VideoOutputNapi AsyncCompleteCallback context is null");
    MEDIA_INFO_LOG("VideoOutputNapi AsyncCompleteCallback %{public}s, status = %{public}d", context->funcName.c_str(),
        context->status);
    std::unique_ptr<JSAsyncContextOutput> jsContext = std::make_unique<JSAsyncContextOutput>();
    jsContext->status = context->status;
    if (!context->status) {
        CameraNapiUtils::CreateNapiErrorObject(env, context->errorCode, context->errorMsg.c_str(), jsContext);
    } else {
        napi_get_undefined(env, &jsContext->data);
    }
    if (!context->funcName.empty() && context->taskId > 0) {
        // Finish async trace
        CAMERA_FINISH_ASYNC_TRACE(context->funcName, context->taskId);
        jsContext->funcName = context->funcName;
    }
    if (context->work != nullptr) {
        CameraNapiUtils::InvokeJSAsyncMethod(env, context->deferred, context->callbackRef, context->work, *jsContext);
    }
    context->FreeHeldNapiValue(env);
    delete context;
}
} // namespace

thread_local napi_ref VideoOutputNapi::sConstructor_ = nullptr;
thread_local sptr<VideoOutput> VideoOutputNapi::sVideoOutput_ = nullptr;
thread_local uint32_t VideoOutputNapi::videoOutputTaskId = CAMERA_VIDEO_OUTPUT_TASKID;

VideoCallbackListener::VideoCallbackListener(napi_env env) : ListenerBase(env) {}

void VideoCallbackListener::UpdateJSCallbackAsync(VideoOutputEventType eventType, const VideoCallbackInfo& info) const
{
    MEDIA_DEBUG_LOG("UpdateJSCallbackAsync is called");
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
    std::unique_ptr<VideoOutputCallbackInfo> callbackInfo =
        std::make_unique<VideoOutputCallbackInfo>(eventType, info, shared_from_this());
    work->data = callbackInfo.get();
    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t* work) {}, [] (uv_work_t* work, int status) {
        VideoOutputCallbackInfo* callbackInfo = reinterpret_cast<VideoOutputCallbackInfo *>(work->data);
        if (callbackInfo) {
            auto listener = callbackInfo->listener_.lock();
            if (listener) {
                listener->UpdateJSCallback(callbackInfo->eventType_, callbackInfo->info_);
            }
            delete callbackInfo;
        }
        delete work;
    }, uv_qos_user_initiated);
    if (ret) {
        MEDIA_ERR_LOG("failed to execute work");
        delete work;
    }  else {
        callbackInfo.release();
    }
}

void VideoCallbackListener::OnFrameStarted() const
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("OnFrameStarted is called");
    VideoCallbackInfo info;
    UpdateJSCallbackAsync(VideoOutputEventType::VIDEO_FRAME_START, info);
}

void VideoCallbackListener::OnFrameEnded(const int32_t frameCount) const
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("OnFrameEnded is called, frameCount: %{public}d", frameCount);
    VideoCallbackInfo info;
    info.frameCount = frameCount;
    UpdateJSCallbackAsync(VideoOutputEventType::VIDEO_FRAME_END, info);
}

void VideoCallbackListener::OnError(const int32_t errorCode) const
{
    MEDIA_DEBUG_LOG("OnError is called, errorCode: %{public}d", errorCode);
    VideoCallbackInfo info;
    info.errorCode = errorCode;
    UpdateJSCallbackAsync(VideoOutputEventType::VIDEO_FRAME_ERROR, info);
}

void VideoCallbackListener::OnDeferredVideoEnhancementInfo(const CaptureEndedInfoExt captureEndedInfo) const
{
    MEDIA_DEBUG_LOG("OnDeferredVideoEnhancementInfo is called");
    VideoCallbackInfo info;
    info.isDeferredVideoEnhancementAvailable = captureEndedInfo.isDeferredVideoEnhancementAvailable;
    info.videoId = captureEndedInfo.videoId;
    MEDIA_INFO_LOG("OnDeferredVideoEnhancementInfo isDeferredVideo:%{public}d videoId:%{public}s ",
        info.isDeferredVideoEnhancementAvailable, info.videoId.c_str());
    UpdateJSCallbackAsync(VideoOutputEventType::VIDEO_DEFERRED_ENHANCEMENT, info);
}

void VideoCallbackListener::ExecuteOnDeferredVideoCb(const VideoCallbackInfo& info) const
{
    MEDIA_INFO_LOG("ExecuteOnDeferredVideoCb");
    napi_value result[ARGS_TWO] = {nullptr, nullptr};
    napi_value retVal;

    napi_get_undefined(env_, &result[PARAM0]);
    napi_get_undefined(env_, &result[PARAM1]);
    
    napi_value propValue;
    napi_create_object(env_, &result[PARAM1]);
    napi_get_boolean(env_, info.isDeferredVideoEnhancementAvailable, &propValue);
    napi_set_named_property(env_, result[PARAM1], "isDeferredVideoEnhancementAvailable", propValue);
    napi_create_string_utf8(env_, info.videoId.c_str(), NAPI_AUTO_LENGTH, &propValue);
    napi_set_named_property(env_, result[PARAM1], "videoId", propValue);

    ExecuteCallbackNapiPara callbackNapiPara { .recv = nullptr, .argc = ARGS_TWO, .argv = result, .result = &retVal };
    ExecuteCallback(CONST_VIDEO_DEFERRED_ENHANCEMENT, callbackNapiPara);
}

void VideoCallbackListener::UpdateJSCallback(VideoOutputEventType eventType, const VideoCallbackInfo& info) const
{
    MEDIA_DEBUG_LOG("UpdateJSCallback is called");
    switch (eventType) {
        // case VideoOutputEventType::VIDEO_FRAME_START:
        // case VideoOutputEventType::VIDEO_FRAME_END:
        // case VideoOutputEventType::VIDEO_FRAME_ERROR:
        // case VideoOutputEventType::VIDEO_INVALID_TYPE:
        //     break;
        case VideoOutputEventType::VIDEO_DEFERRED_ENHANCEMENT:
            ExecuteOnDeferredVideoCb(info);
            break;
        default:
            MEDIA_ERR_LOG("Incorrect photo callback event type received from JS");
    }

    napi_value result[ARGS_ONE];
    napi_value retVal;
    napi_value propValue;
    std::string eventName = VideoOutputEventTypeHelper.GetKeyString(eventType);
    if (eventName.empty()) {
        MEDIA_WARNING_LOG(
            "VideoCallbackListener::UpdateJSCallback, event type is invalid %d", static_cast<int32_t>(eventType));
        return;
    }
    if (eventType == VideoOutputEventType::VIDEO_FRAME_ERROR) {
        napi_create_object(env_, &result[PARAM0]);
        napi_create_int32(env_, info.errorCode, &propValue);
        napi_set_named_property(env_, result[PARAM0], "code", propValue);
    } else {
        napi_get_undefined(env_, &result[PARAM0]);
    }
    ExecuteCallbackNapiPara callbackNapiPara { .recv = nullptr, .argc = ARGS_ONE, .argv = result, .result = &retVal };
    ExecuteCallback(eventName, callbackNapiPara);
}

VideoOutputNapi::VideoOutputNapi() : env_(nullptr) {}

VideoOutputNapi::~VideoOutputNapi()
{
    MEDIA_DEBUG_LOG("~VideoOutputNapi is called");
}

void VideoOutputNapi::VideoOutputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint)
{
    MEDIA_DEBUG_LOG("VideoOutputNapiDestructor is called");
    VideoOutputNapi* videoOutput = reinterpret_cast<VideoOutputNapi*>(nativeObject);
    if (videoOutput != nullptr) {
        delete videoOutput;
    }
}

napi_value VideoOutputNapi::Init(napi_env env, napi_value exports)
{
    MEDIA_DEBUG_LOG("Init is called");
    napi_status status;
    napi_value ctorObj;
    int32_t refCount = 1;

    napi_property_descriptor video_output_props[] = {
        DECLARE_NAPI_FUNCTION("start", Start),
        DECLARE_NAPI_FUNCTION("stop", Stop),
        DECLARE_NAPI_FUNCTION("setFrameRate", SetFrameRate),
        DECLARE_NAPI_FUNCTION("getActiveFrameRate", GetActiveFrameRate),
        DECLARE_NAPI_FUNCTION("getSupportedFrameRates", GetSupportedFrameRates),
        DECLARE_NAPI_FUNCTION("isMirrorSupported", IsMirrorSupported),
        DECLARE_NAPI_FUNCTION("enableMirror", EnableMirror),
        DECLARE_NAPI_FUNCTION("release", Release),
        DECLARE_NAPI_FUNCTION("on", On),
        DECLARE_NAPI_FUNCTION("once", Once),
        DECLARE_NAPI_FUNCTION("off", Off),
        DECLARE_NAPI_FUNCTION("getActiveProfile", GetActiveProfile),
        DECLARE_NAPI_FUNCTION("getSupportedVideoMetaTypes", GetSupportedVideoMetaTypes),
        DECLARE_NAPI_FUNCTION("attachMetaSurface", AttachMetaSurface),
        DECLARE_NAPI_FUNCTION("getVideoRotation", GetVideoRotation),
        DECLARE_NAPI_FUNCTION("isAutoDeferredVideoEnhancementSupported", IsAutoDeferredVideoEnhancementSupported),
        DECLARE_NAPI_FUNCTION("isAutoDeferredVideoEnhancementEnabled", IsAutoDeferredVideoEnhancementEnabled),
        DECLARE_NAPI_FUNCTION("enableAutoDeferredVideoEnhancement", EnableAutoDeferredVideoEnhancement),
        DECLARE_NAPI_FUNCTION("getSupportedRotations", GetSupportedRotations),
        DECLARE_NAPI_FUNCTION("isRotationSupported", IsRotationSupported),
        DECLARE_NAPI_FUNCTION("setRotation", SetRotation),
    };

    status = napi_define_class(env, CAMERA_VIDEO_OUTPUT_NAPI_CLASS_NAME, NAPI_AUTO_LENGTH,
                               VideoOutputNapiConstructor, nullptr,
                               sizeof(video_output_props) / sizeof(video_output_props[PARAM0]),
                               video_output_props, &ctorObj);
    if (status == napi_ok) {
        status = napi_create_reference(env, ctorObj, refCount, &sConstructor_);
        if (status == napi_ok) {
            status = napi_set_named_property(env, exports, CAMERA_VIDEO_OUTPUT_NAPI_CLASS_NAME, ctorObj);
            if (status == napi_ok) {
                return exports;
            }
        }
    }
    MEDIA_ERR_LOG("Init call Failed!");
    return nullptr;
}

// Constructor callback
napi_value VideoOutputNapi::VideoOutputNapiConstructor(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("VideoOutputNapiConstructor is called");
    napi_status status;
    napi_value result = nullptr;
    napi_value thisVar = nullptr;

    napi_get_undefined(env, &result);
    CAMERA_NAPI_GET_JS_OBJ_WITH_ZERO_ARGS(env, info, status, thisVar);

    if (status == napi_ok && thisVar != nullptr) {
        std::unique_ptr<VideoOutputNapi> obj = std::make_unique<VideoOutputNapi>();
        if (obj != nullptr) {
            obj->env_ = env;
            obj->videoOutput_ = sVideoOutput_;

            status = napi_wrap(env, thisVar, reinterpret_cast<void*>(obj.get()),
                               VideoOutputNapi::VideoOutputNapiDestructor, nullptr, nullptr);
            if (status == napi_ok) {
                obj.release();
                return thisVar;
            } else {
                MEDIA_ERR_LOG("Failure wrapping js to native napi");
            }
        }
    }
    MEDIA_ERR_LOG("VideoOutputNapiConstructor call Failed!");
    return result;
}

sptr<VideoOutput> VideoOutputNapi::GetVideoOutput()
{
    return videoOutput_;
}

bool VideoOutputNapi::IsVideoOutput(napi_env env, napi_value obj)
{
    MEDIA_DEBUG_LOG("IsVideoOutput is called");
    bool result = false;
    napi_status status;
    napi_value constructor = nullptr;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        status = napi_instanceof(env, obj, constructor, &result);
        if (status != napi_ok) {
            result = false;
        }
    }
    return result;
}

napi_value VideoOutputNapi::GetActiveProfile(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("VideoOutputNapi::GetActiveProfile is called");
    VideoOutputNapi* videoOutputNapi = nullptr;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::GetActiveProfile parse parameter occur error");
        return nullptr;
    }
    auto profile = videoOutputNapi->videoOutput_->GetVideoProfile();
    if (profile == nullptr) {
        return CameraNapiUtils::GetUndefinedValue(env);
    }
    return CameraNapiObjVideoProfile(*profile).GenerateNapiValue(env);
}

static napi_value CreateJSArray(napi_env env, napi_status status, std::vector<VideoMetaType> nativeArray)
{
    MEDIA_DEBUG_LOG("CreateJSArray is called");
    napi_value jsArray = nullptr;
    napi_value item = nullptr;
 
    if (nativeArray.empty()) {
        MEDIA_ERR_LOG("nativeArray is empty");
    }
 
    status = napi_create_array(env, &jsArray);
    if (status == napi_ok) {
        for (size_t i = 0; i < nativeArray.size(); i++) {
            napi_create_int32(env, nativeArray[i], &item);
            if (napi_set_element(env, jsArray, i, item) != napi_ok) {
                MEDIA_ERR_LOG("Failed to create profile napi wrapper object");
                return nullptr;
            }
        }
    }
    return jsArray;
}
 
napi_value VideoOutputNapi::GetSupportedVideoMetaTypes(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("GetSupportedVideoMetaTypes is called");
    napi_status status;
    napi_value result;
    size_t argc = ARGS_ZERO;
    napi_value argv[ARGS_ZERO];
    napi_value thisVar = nullptr;
 
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO), "requires no parameter.");
 
    napi_get_undefined(env, &result);
    VideoOutputNapi* videoOutputNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&videoOutputNapi));
    if (status == napi_ok && videoOutputNapi != nullptr) {
        std::vector<VideoMetaType> videoMetaType = videoOutputNapi->videoOutput_->GetSupportedVideoMetaTypes();
        result = CreateJSArray(env, status, videoMetaType);
    } else {
        MEDIA_ERR_LOG("GetSupportedVideoMetaTypes call failed!");
    }
    return result;
}
 
napi_value VideoOutputNapi::AttachMetaSurface(napi_env env, napi_callback_info info)
{
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
 
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
 
    napi_get_undefined(env, &result);
    VideoOutputNapi* videoOutputNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&videoOutputNapi));
    if (status == napi_ok && videoOutputNapi != nullptr) {
        char buffer[PATH_MAX];
        size_t surfaceId;
        napi_get_value_string_utf8(env, argv[PARAM0], buffer, PATH_MAX, &surfaceId);
        uint32_t videoMetaType;
        napi_get_value_uint32(env, argv[PARAM1], &videoMetaType);
 
        uint64_t iSurfaceId;
        std::istringstream iss((std::string(buffer)));
        iss >> iSurfaceId;
        sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(iSurfaceId);
        if (surface == nullptr) {
            MEDIA_ERR_LOG("failed to get surface from SurfaceUtils");
        }
        videoOutputNapi->videoOutput_->AttachMetaSurface(surface, static_cast<VideoMetaType>(videoMetaType));
    } else {
        MEDIA_ERR_LOG("VideoOutputNapi::AttachMetaSurface failed!");
    }
    return result;
}

napi_value VideoOutputNapi::CreateVideoOutput(napi_env env, VideoProfile &profile, std::string surfaceId)
{
    MEDIA_DEBUG_LOG("CreateVideoOutput is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        uint64_t iSurfaceId;
        std::istringstream iss(surfaceId);
        iss >> iSurfaceId;
        sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(iSurfaceId);
        if (surface == nullptr) {
            MEDIA_ERR_LOG("failed to get surface from SurfaceUtils");
            return result;
        }
        surface->SetUserData(CameraManager::surfaceFormat, std::to_string(profile.GetCameraFormat()));
        int retCode = CameraManager::GetInstance()->CreateVideoOutput(profile, surface, &sVideoOutput_);
        if (!CameraNapiUtils::CheckError(env, retCode)) {
            return nullptr;
        }
        if (sVideoOutput_ == nullptr) {
            MEDIA_ERR_LOG("failed to create VideoOutput");
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sVideoOutput_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create video output instance");
        }
    }
    napi_get_undefined(env, &result);
    MEDIA_ERR_LOG("CreateVideoOutput call Failed!");
    return result;
}

napi_value VideoOutputNapi::CreateVideoOutput(napi_env env, std::string surfaceId)
{
    MEDIA_DEBUG_LOG("VideoOutputNapi::CreateVideoOutput is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    napi_value constructor;

    status = napi_get_reference_value(env, sConstructor_, &constructor);
    if (status == napi_ok) {
        uint64_t iSurfaceId;
        std::istringstream iss(surfaceId);
        iss >> iSurfaceId;
        sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(iSurfaceId);
        if (surface == nullptr) {
            MEDIA_ERR_LOG("failed to get surface from SurfaceUtils");
            return result;
        }
        int retCode = CameraManager::GetInstance()->CreateVideoOutputWithoutProfile(surface, &sVideoOutput_);
        if (!CameraNapiUtils::CheckError(env, retCode)) {
            return nullptr;
        }
        if (sVideoOutput_ == nullptr) {
            MEDIA_ERR_LOG("failed to create VideoOutput");
            return result;
        }
        status = napi_new_instance(env, constructor, 0, nullptr, &result);
        sVideoOutput_ = nullptr;
        if (status == napi_ok && result != nullptr) {
            return result;
        } else {
            MEDIA_ERR_LOG("Failed to create video output instance");
        }
    }
    napi_get_undefined(env, &result);
    MEDIA_ERR_LOG("CreateVideoOutput call Failed!");
    return result;
}

napi_value VideoOutputNapi::Start(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("Start is called");
    std::unique_ptr<VideoOutputAsyncContext> asyncContext = std::make_unique<VideoOutputAsyncContext>(
        "VideoOutputNapi::Start", CameraNapiUtils::IncrementAndGet(videoOutputTaskId));
    auto asyncFunction =
        std::make_shared<CameraNapiAsyncFunction>(env, "Start", asyncContext->callbackRef, asyncContext->deferred);
    CameraNapiParamParser jsParamParser(env, info, asyncContext->objectInfo, asyncFunction);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "invalid argument")) {
        MEDIA_ERR_LOG("VideoOutputNapi::Start invalid argument");
        return nullptr;
    }
    asyncContext->HoldNapiValue(env, jsParamParser.GetThisVar());
    napi_status status = napi_create_async_work(
        env, nullptr, asyncFunction->GetResourceName(),
        [](napi_env env, void* data) {
            MEDIA_INFO_LOG("VideoOutputNapi::Start running on worker");
            auto context = static_cast<VideoOutputAsyncContext*>(data);
            CHECK_ERROR_RETURN_LOG(context->objectInfo == nullptr, "VideoOutputNapi::Start async info is nullptr");
            CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
            CameraNapiWorkerQueueKeeper::GetInstance()->ConsumeWorkerQueueTask(context->queueTask, [&context]() {
                context->errorCode = context->objectInfo->videoOutput_->Start();
                context->status = true;
                MEDIA_INFO_LOG("VideoOutputNapi::Start errorCode:%{public}d", context->errorCode);
            });
        },
        AsyncCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to create napi_create_async_work for VideoOutputNapi::Start");
        asyncFunction->Reset();
    } else {
        asyncContext->queueTask =
            CameraNapiWorkerQueueKeeper::GetInstance()->AcquireWorkerQueueTask("VideoOutputNapi::Start");
        napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
        asyncContext.release();
    }
    if (asyncFunction->GetAsyncFunctionType() == ASYNC_FUN_TYPE_PROMISE) {
        return asyncFunction->GetPromise();
    }
    return CameraNapiUtils::GetUndefinedValue(env);
}

napi_value VideoOutputNapi::Stop(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("Stop is called");
    std::unique_ptr<VideoOutputAsyncContext> asyncContext = std::make_unique<VideoOutputAsyncContext>(
        "VideoOutputNapi::Stop", CameraNapiUtils::IncrementAndGet(videoOutputTaskId));
    auto asyncFunction =
        std::make_shared<CameraNapiAsyncFunction>(env, "Stop", asyncContext->callbackRef, asyncContext->deferred);
    CameraNapiParamParser jsParamParser(env, info, asyncContext->objectInfo, asyncFunction);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "invalid argument")) {
        MEDIA_ERR_LOG("VideoOutputNapi::Stop invalid argument");
        return nullptr;
    }
    asyncContext->HoldNapiValue(env, jsParamParser.GetThisVar());
    napi_status status = napi_create_async_work(
        env, nullptr, asyncFunction->GetResourceName(),
        [](napi_env env, void* data) {
            MEDIA_INFO_LOG("VideoOutputNapi::Stop running on worker");
            auto context = static_cast<VideoOutputAsyncContext*>(data);
            CHECK_ERROR_RETURN_LOG(context->objectInfo == nullptr, "VideoOutputNapi::Stop async info is nullptr");
            CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
            CameraNapiWorkerQueueKeeper::GetInstance()->ConsumeWorkerQueueTask(context->queueTask, [&context]() {
                context->errorCode = context->objectInfo->videoOutput_->Stop();
                context->status = true;
                MEDIA_INFO_LOG("VideoOutputNapi::Stop errorCode:%{public}d", context->errorCode);
            });
        },
        AsyncCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to create napi_create_async_work for VideoOutputNapi::Stop");
        asyncFunction->Reset();
    } else {
        asyncContext->queueTask =
            CameraNapiWorkerQueueKeeper::GetInstance()->AcquireWorkerQueueTask("VideoOutputNapi::Stop");
        napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
        asyncContext.release();
    }
    if (asyncFunction->GetAsyncFunctionType() == ASYNC_FUN_TYPE_PROMISE) {
        return asyncFunction->GetPromise();
    }
    return CameraNapiUtils::GetUndefinedValue(env);
}

napi_value VideoOutputNapi::SetFrameRate(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("SetFrameRate is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result;
    size_t argc = ARGS_TWO;
    napi_value argv[ARGS_TWO] = {0};
    napi_value thisVar = nullptr;
 
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_TWO), "requires 2 parameters maximum.");
 
    napi_get_undefined(env, &result);
    VideoOutputNapi* videoOutputNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&videoOutputNapi));
    if (status == napi_ok && videoOutputNapi != nullptr) {
        int32_t minFrameRate;
        napi_get_value_int32(env, argv[PARAM0], &minFrameRate);
        int32_t maxFrameRate;
        napi_get_value_int32(env, argv[PARAM1], &maxFrameRate);
        int32_t retCode = videoOutputNapi->videoOutput_->SetFrameRate(minFrameRate, maxFrameRate);
        if (!CameraNapiUtils::CheckError(env, retCode)) {
            MEDIA_ERR_LOG("VideoOutputNapi::SetFrameRate! %{public}d", retCode);
            return result;
        }
    } else {
        MEDIA_ERR_LOG("SetFrameRate call Failed!");
    }
    return result;
}
 
napi_value VideoOutputNapi::GetActiveFrameRate(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("GetFrameRate is called");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result;
    size_t argc = ARGS_ZERO;
    napi_value argv[ARGS_ZERO];
    napi_value thisVar = nullptr;
 
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO), "requires no parameter.");
 
    napi_get_undefined(env, &result);
    VideoOutputNapi* videoOutputNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&videoOutputNapi));
    if (status == napi_ok && videoOutputNapi != nullptr) {
        std::vector<int32_t> frameRateRange = videoOutputNapi->videoOutput_->GetFrameRateRange();
        CameraNapiUtils::CreateFrameRateJSArray(env, frameRateRange, result);
    } else {
        MEDIA_ERR_LOG("GetFrameRate call failed!");
    }
    return result;
}
 
napi_value VideoOutputNapi::GetSupportedFrameRates(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("GetSupportedFrameRates is called");
 
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result;
    size_t argc = ARGS_ZERO;
    napi_value argv[ARGS_ZERO];
    napi_value thisVar = nullptr;
 
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    NAPI_ASSERT(env, (argc == ARGS_ZERO), "requires no parameter.");
    napi_get_undefined(env, &result);
    VideoOutputNapi* videoOutputNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&videoOutputNapi));
    if (status == napi_ok && videoOutputNapi != nullptr) {
        std::vector<std::vector<int32_t>> supportedFrameRatesRange =
                                          videoOutputNapi->videoOutput_->GetSupportedFrameRates();
        result = CameraNapiUtils::CreateSupportFrameRatesJSArray(env, supportedFrameRatesRange);
    } else {
        MEDIA_ERR_LOG("GetSupportedFrameRates call failed!");
    }
    return result;
}

napi_value VideoOutputNapi::GetVideoRotation(napi_env env, napi_callback_info info)
{
    MEDIA_DEBUG_LOG("GetVideoRotation is called!");
    CAMERA_SYNC_TRACE;
    napi_status status;
    napi_value result = nullptr;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);

    napi_get_undefined(env, &result);
    VideoOutputNapi* videoOutputNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&videoOutputNapi));
    if (status == napi_ok && videoOutputNapi != nullptr) {
        int32_t imageRotation;
        napi_status ret = napi_get_value_int32(env, argv[PARAM0], &imageRotation);
        if (ret != napi_ok) {
            CameraNapiUtils::ThrowError(env, INVALID_ARGUMENT,
                "GetVideoRotation parameter missing or parameter type incorrect.");
            return result;
        }
        int32_t retCode = videoOutputNapi->videoOutput_->GetVideoRotation(imageRotation);
        if (retCode == SERVICE_FATL_ERROR) {
            CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR,
                "GetVideoRotation Camera service fatal error.");
            return result;
        }
        napi_create_int32(env, retCode, &result);
        MEDIA_INFO_LOG("VideoOutputNapi GetVideoRotation! %{public}d", retCode);
    } else {
        MEDIA_ERR_LOG("VideoOutputNapi GetVideoRotation! called failed!");
    }
    return result;
}

napi_value VideoOutputNapi::IsMirrorSupported(napi_env env, napi_callback_info info)
{
    auto result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi IsMirrorSupported is called!");
        return result;
    }
    MEDIA_DEBUG_LOG("VideoOutputNapi::IsMirrorSupported is called");
 
    VideoOutputNapi* videoOutputNapi = nullptr;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsMirrorSupported parse parameter occur error");
        return result;
    }
    if (videoOutputNapi->videoOutput_ == nullptr) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsMirrorSupported get native object fail");
        CameraNapiUtils::ThrowError(env, INVALID_ARGUMENT, "get native object fail");
        return result;
    }
    bool isMirrorSupported = videoOutputNapi->videoOutput_->IsMirrorSupported();
    if (isMirrorSupported) {
        napi_get_boolean(env, true, &result);
        return result;
    }
    MEDIA_ERR_LOG("VideoOutputNapi::IsMirrorSupported is not supported");
    napi_get_boolean(env, false, &result);
    return result;
}
 
napi_value VideoOutputNapi::EnableMirror(napi_env env, napi_callback_info info)
{
    auto result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi EnableMirror is called!");
        return result;
    }
    MEDIA_DEBUG_LOG("VideoOutputNapi::EnableMirror is called");
    VideoOutputNapi* videoOutputNapi = nullptr;
    bool isEnable;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi, isEnable);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsMirrorSupported parse parameter occur error");
        return result;
    }
    if (videoOutputNapi->videoOutput_ == nullptr) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsMirrorSupported get native object fail");
        CameraNapiUtils::ThrowError(env, INVALID_ARGUMENT, "get native object fail");
        return result;
    }
 
    int32_t retCode = videoOutputNapi->videoOutput_->enableMirror(isEnable);
    if (!CameraNapiUtils::CheckError(env, retCode)) {
        MEDIA_ERR_LOG("PhotoOutputNapi::EnableAutoHighQualityPhoto fail %{public}d", retCode);
    }
    return result;
}

napi_value VideoOutputNapi::Release(napi_env env, napi_callback_info info)
{
    MEDIA_INFO_LOG("VideoOutputNapi::Release is called");
    std::unique_ptr<VideoOutputAsyncContext> asyncContext = std::make_unique<VideoOutputAsyncContext>(
        "VideoOutputNapi::Release", CameraNapiUtils::IncrementAndGet(videoOutputTaskId));
    auto asyncFunction =
        std::make_shared<CameraNapiAsyncFunction>(env, "Release", asyncContext->callbackRef, asyncContext->deferred);
    CameraNapiParamParser jsParamParser(env, info, asyncContext->objectInfo, asyncFunction);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "invalid argument")) {
        MEDIA_ERR_LOG("VideoOutputNapi::Release invalid argument");
        return nullptr;
    }
    asyncContext->HoldNapiValue(env, jsParamParser.GetThisVar());
    napi_status status = napi_create_async_work(
        env, nullptr, asyncFunction->GetResourceName(),
        [](napi_env env, void* data) {
            MEDIA_INFO_LOG("VideoOutputNapi::Release running on worker");
            auto context = static_cast<VideoOutputAsyncContext*>(data);
            CHECK_ERROR_RETURN_LOG(context->objectInfo == nullptr, "VideoOutputNapi::Release async info is nullptr");
            CAMERA_START_ASYNC_TRACE(context->funcName, context->taskId);
            CameraNapiWorkerQueueKeeper::GetInstance()->ConsumeWorkerQueueTask(context->queueTask, [&context]() {
                context->errorCode = context->objectInfo->videoOutput_->Release();
                context->status = context->errorCode == CameraErrorCode::SUCCESS;
            });
        },
        AsyncCompleteCallback, static_cast<void*>(asyncContext.get()), &asyncContext->work);
    if (status != napi_ok) {
        MEDIA_ERR_LOG("Failed to create napi_create_async_work for VideoOutputNapi::Release");
        asyncFunction->Reset();
    } else {
        asyncContext->queueTask =
            CameraNapiWorkerQueueKeeper::GetInstance()->AcquireWorkerQueueTask("VideoOutputNapi::Release");
        napi_queue_async_work_with_qos(env, asyncContext->work, napi_qos_user_initiated);
        asyncContext.release();
    }
    if (asyncFunction->GetAsyncFunctionType() == ASYNC_FUN_TYPE_PROMISE) {
        return asyncFunction->GetPromise();
    }
    return CameraNapiUtils::GetUndefinedValue(env);
}

void VideoOutputNapi::RegisterFrameStartCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args, bool isOnce)
{
    if (videoCallback_ == nullptr) {
        videoCallback_ = make_shared<VideoCallbackListener>(env);
        videoOutput_->SetCallback(videoCallback_);
    }
    videoCallback_->SaveCallbackReference(CONST_VIDEO_FRAME_START, callback, isOnce);
}

void VideoOutputNapi::UnregisterFrameStartCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args)
{
    if (videoCallback_ == nullptr) {
        MEDIA_ERR_LOG("videoCallback is null");
        return;
    }
    videoCallback_->RemoveCallbackRef(CONST_VIDEO_FRAME_START, callback);
}

void VideoOutputNapi::RegisterFrameEndCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args, bool isOnce)
{
    if (videoCallback_ == nullptr) {
        videoCallback_ = make_shared<VideoCallbackListener>(env);
        videoOutput_->SetCallback(videoCallback_);
    }
    videoCallback_->SaveCallbackReference(CONST_VIDEO_FRAME_END, callback, isOnce);
}
void VideoOutputNapi::UnregisterFrameEndCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args)
{
    if (videoCallback_ == nullptr) {
        MEDIA_ERR_LOG("videoCallback is null");
        return;
    }
    videoCallback_->RemoveCallbackRef(CONST_VIDEO_FRAME_END, callback);
}

void VideoOutputNapi::RegisterErrorCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args, bool isOnce)
{
    if (videoCallback_ == nullptr) {
        videoCallback_ = make_shared<VideoCallbackListener>(env);
        videoOutput_->SetCallback(videoCallback_);
    }
    videoCallback_->SaveCallbackReference(CONST_VIDEO_FRAME_ERROR, callback, isOnce);
}

void VideoOutputNapi::UnregisterErrorCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args)
{
    if (videoCallback_ == nullptr) {
        MEDIA_ERR_LOG("videoCallback is null");
        return;
    }
    videoCallback_->RemoveCallbackRef(CONST_VIDEO_FRAME_ERROR, callback);
}

void VideoOutputNapi::RegisterDeferredVideoCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args, bool isOnce)
{
    if (videoCallback_ == nullptr) {
        videoCallback_ = make_shared<VideoCallbackListener>(env);
        videoOutput_->SetCallback(videoCallback_);
    }
    videoCallback_->SaveCallbackReference(CONST_VIDEO_DEFERRED_ENHANCEMENT, callback, isOnce);
}

void VideoOutputNapi::UnregisterDeferredVideoCallbackListener(
    const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args)
{
    if (videoCallback_ == nullptr) {
        MEDIA_ERR_LOG("videoCallback is null");
        return;
    }
    videoCallback_->RemoveCallbackRef(CONST_VIDEO_DEFERRED_ENHANCEMENT, callback);
}

const VideoOutputNapi::EmitterFunctions& VideoOutputNapi::GetEmitterFunctions()
{
    static const EmitterFunctions funMap = {
        { CONST_VIDEO_FRAME_START, {
            &VideoOutputNapi::RegisterFrameStartCallbackListener,
            &VideoOutputNapi::UnregisterFrameStartCallbackListener } },
        { CONST_VIDEO_FRAME_END, {
            &VideoOutputNapi::RegisterFrameEndCallbackListener,
            &VideoOutputNapi::UnregisterFrameEndCallbackListener } },
        { CONST_VIDEO_FRAME_ERROR, {
            &VideoOutputNapi::RegisterErrorCallbackListener,
            &VideoOutputNapi::UnregisterErrorCallbackListener } },
        { CONST_VIDEO_DEFERRED_ENHANCEMENT, {
            &VideoOutputNapi::RegisterDeferredVideoCallbackListener,
            &VideoOutputNapi::UnregisterDeferredVideoCallbackListener } }};
    return funMap;
}

napi_value VideoOutputNapi::On(napi_env env, napi_callback_info info)
{
    return ListenerTemplate<VideoOutputNapi>::On(env, info);
}

napi_value VideoOutputNapi::Once(napi_env env, napi_callback_info info)
{
    return ListenerTemplate<VideoOutputNapi>::Once(env, info);
}

napi_value VideoOutputNapi::Off(napi_env env, napi_callback_info info)
{
    return ListenerTemplate<VideoOutputNapi>::Off(env, info);
}

napi_value VideoOutputNapi::IsAutoDeferredVideoEnhancementSupported(napi_env env, napi_callback_info info)
{
    napi_value result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi IsAutoDeferredVideoEnhancementSupported is called!");
        return result;
    }
    MEDIA_DEBUG_LOG("VideoOutputNapi::IsAutoDeferredVideoEnhancementSupported is called");
 
    VideoOutputNapi* videoOutputNapi = nullptr;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi);
    if (!jsParamParser.AssertStatus(SERVICE_FATL_ERROR, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsAutoDeferredVideoEnhancementSupported parse parameter occur error");
        return result;
    }
    if (videoOutputNapi->videoOutput_ == nullptr) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsAutoDeferredVideoEnhancementSupported get native object fail");
        CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR, "get native object fail");
        return result;
    }
    int32_t res = videoOutputNapi->videoOutput_->IsAutoDeferredVideoEnhancementSupported();
    if (res > 1) {
        CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR, "inner fail");
        return result;
    }
    napi_get_boolean(env, res, &result);
    return result;
}

napi_value VideoOutputNapi::IsAutoDeferredVideoEnhancementEnabled(napi_env env, napi_callback_info info)
{
    napi_value result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi IsAutoDeferredVideoEnhancementEnabled is called!");
        return result;
    }
    MEDIA_DEBUG_LOG("VideoOutputNapi::IsAutoDeferredVideoEnhancementEnabled is called");
 
    VideoOutputNapi* videoOutputNapi = nullptr;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi);
    if (!jsParamParser.AssertStatus(SERVICE_FATL_ERROR, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsAutoDeferredVideoEnhancementEnabled parse parameter occur error");
        return result;
    }
    if (videoOutputNapi->videoOutput_ == nullptr) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsAutoDeferredVideoEnhancementEnabled get native object fail");
        CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR, "get native object fail");
        return result;
    }
    int32_t res = videoOutputNapi->videoOutput_->IsAutoDeferredVideoEnhancementEnabled();
    if (res > 1) {
        CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR, "inner fail");
        return result;
    }
    napi_get_boolean(env, res, &result);
    return result;
}

napi_value VideoOutputNapi::EnableAutoDeferredVideoEnhancement(napi_env env, napi_callback_info info)
{
    napi_value result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi EnableAutoDeferredVideoEnhancement is called!");
        return result;
    }
    napi_status status;
    size_t argc = ARGS_ONE;
    napi_value argv[ARGS_ONE] = {0};
    napi_value thisVar = nullptr;
    CAMERA_NAPI_GET_JS_ARGS(env, info, argc, argv, thisVar);
    if (argc != ARGS_ONE) {
        CameraNapiUtils::ThrowError(env, INVALID_ARGUMENT, "requires one parameter");
        return result;
    }
    int32_t res = 0;
    napi_get_undefined(env, &result);
    VideoOutputNapi* videoOutputNapi = nullptr;
    status = napi_unwrap(env, thisVar, reinterpret_cast<void**>(&videoOutputNapi));
    if (status == napi_ok && videoOutputNapi != nullptr) {
        bool isEnable;
        napi_get_value_bool(env, argv[PARAM0], &isEnable);
        res = videoOutputNapi->videoOutput_->EnableAutoDeferredVideoEnhancement(isEnable);
    }
    if (res > 0) {
        CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR, "inner fail");
    }
    return result;
}

napi_value VideoOutputNapi::GetSupportedRotations(napi_env env, napi_callback_info info)
{
    napi_value result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi GetSupportedRotations is called!");
        return result;
    }
    MEDIA_DEBUG_LOG("VideoOutputNapi::GetSupportedRotations is called");
 
    VideoOutputNapi* videoOutputNapi = nullptr;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi);
    if (!jsParamParser.AssertStatus(SERVICE_FATL_ERROR, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::GetSupportedRotations parse parameter occur error");
        return result;
    }
    if (videoOutputNapi->videoOutput_ == nullptr) {
        MEDIA_ERR_LOG("VideoOutputNapi::GetSupportedRotations get native object fail");
        CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR, "get native object fail");
        return result;
    }
    std::vector<int32_t> supportedRotations;
    int32_t retCode = videoOutputNapi->videoOutput_->GetSupportedRotations(supportedRotations);
    if (!CameraNapiUtils::CheckError(env, retCode)) {
        return nullptr;
    }
    napi_status status = napi_create_array(env, &result);
    CHECK_ERROR_RETURN_RET_LOG(status != napi_ok, result, "napi_create_array call Failed!");
    for (size_t i = 0; i < supportedRotations.size(); i++) {
        int32_t value = supportedRotations[i];
        napi_value element;
        napi_create_int32(env, value, &element);
        napi_set_element(env, result, i, element);
    }
    return result;
}

napi_value VideoOutputNapi::IsRotationSupported(napi_env env, napi_callback_info info)
{
    napi_value result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi IsRotationSupported is called!");
        return result;
    }
    MEDIA_DEBUG_LOG("VideoOutputNapi::IsRotationSupported is called");
 
    VideoOutputNapi* videoOutputNapi = nullptr;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi);
    if (!jsParamParser.AssertStatus(SERVICE_FATL_ERROR, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsRotationSupported parse parameter occur error");
        return result;
    }
    if (videoOutputNapi->videoOutput_ == nullptr) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsRotationSupported get native object fail");
        CameraNapiUtils::ThrowError(env, SERVICE_FATL_ERROR, "get native object fail");
        return result;
    }
    bool isSupported = false;
    int32_t retCode = videoOutputNapi->videoOutput_->IsRotationSupported(isSupported);
    if (!CameraNapiUtils::CheckError(env, retCode)) {
        MEDIA_ERR_LOG("VideoOutputNapi::IsRotationSupported fail %{public}d", retCode);
    }
    napi_get_boolean(env, isSupported, &result);
    return result;
}

napi_value VideoOutputNapi::SetRotation(napi_env env, napi_callback_info info)
{
    napi_value result = CameraNapiUtils::GetUndefinedValue(env);
    if (!CameraNapiSecurity::CheckSystemApp(env)) {
        MEDIA_ERR_LOG("SystemApi SetRotation is called!");
        return result;
    }
    MEDIA_DEBUG_LOG("VideoOutputNapi::SetRotation is called");
    VideoOutputNapi* videoOutputNapi = nullptr;
    int32_t rotation;
    CameraNapiParamParser jsParamParser(env, info, videoOutputNapi, rotation);
    if (!jsParamParser.AssertStatus(INVALID_ARGUMENT, "parse parameter occur error")) {
        MEDIA_ERR_LOG("VideoOutputNapi::SetRotation parse parameter occur error");
        return result;
    }
    if (videoOutputNapi->videoOutput_ == nullptr) {
        MEDIA_ERR_LOG("VideoOutputNapi::SetRotation get native object fail");
        CameraNapiUtils::ThrowError(env, INVALID_ARGUMENT, "get native object fail");
        return result;
    }
    int32_t retCode = videoOutputNapi->videoOutput_->SetRotation(rotation);
    if (!CameraNapiUtils::CheckError(env, retCode)) {
        MEDIA_ERR_LOG("VideoOutputNapi::SetRotation fail %{public}d", retCode);
    }
    return result;
}
} // namespace CameraStandard
} // namespace OHOS
