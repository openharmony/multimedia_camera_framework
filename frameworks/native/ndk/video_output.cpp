/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "kits/native/include/camera/video_output.h"
#include "impl/video_output_impl.h"
#include "camera_log.h"
#include "hilog/log.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @since 11
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_RegisterCallback(Camera_VideoOutput* videoOutput, VideoOutput_Callbacks* callback)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(callback == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback is null!");
    CHECK_RETURN_RET_ELOG(callback->onFrameStart == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback onFrameStart is null!");
    CHECK_RETURN_RET_ELOG(callback->onFrameEnd == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback onFrameEnd is null!");
    CHECK_RETURN_RET_ELOG(callback->onError == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback onError is null!");

    videoOutput->RegisterCallback(callback);
    return CAMERA_OK;
}

/**
 * @since 11
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_UnregisterCallback(Camera_VideoOutput* videoOutput, VideoOutput_Callbacks* callback)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(callback == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback is null!");
    CHECK_RETURN_RET_ELOG(callback->onFrameStart == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback onFrameStart is null!");
    CHECK_RETURN_RET_ELOG(callback->onFrameEnd == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback onFrameEnd is null!");
    CHECK_RETURN_RET_ELOG(callback->onError == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, callback onError is null!");

    videoOutput->UnregisterCallback(callback);
    return CAMERA_OK;
}

/**
 * @since 11
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_Start(Camera_VideoOutput* videoOutput)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");

    return videoOutput->Start();
}

/**
 * @since 11
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_Stop(Camera_VideoOutput* videoOutput)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");

    return videoOutput->Stop();
}

/**
 * @since 11
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_Release(Camera_VideoOutput* videoOutput)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");

    Camera_ErrorCode retCode = videoOutput->Release();
    if (videoOutput != nullptr) {
        delete videoOutput;
    }
    return retCode;
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_GetActiveProfile(Camera_VideoOutput* videoOutput, Camera_VideoProfile** profile)
{
    MEDIA_DEBUG_LOG("OH_VideoOutput_GetActiveProfile is called.");
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(profile == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, profile is null!");

    return videoOutput->GetVideoProfile(profile);
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_DeleteProfile(Camera_VideoProfile* profile)
{
    MEDIA_DEBUG_LOG("OH_VideoOutput_DeleteProfile is called.");
    CHECK_RETURN_RET_ELOG(profile == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, profile is null!");

    delete profile;
    profile = nullptr;
    return CAMERA_OK;
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_GetSupportedFrameRates(Camera_VideoOutput* videoOutput,
    Camera_FrameRateRange** frameRateRange, uint32_t* size)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(frameRateRange == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, frameRateRange is null!");
    CHECK_RETURN_RET_ELOG(size == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, size is null!");

    return videoOutput->GetSupportedFrameRates(frameRateRange, size);
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_DeleteFrameRates(Camera_VideoOutput* videoOutput,
    Camera_FrameRateRange* frameRateRange)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(frameRateRange == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, frameRateRange is null!");

    return videoOutput->DeleteFrameRates(frameRateRange);
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_SetFrameRate(Camera_VideoOutput* videoOutput,
    int32_t minFps, int32_t maxFps)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");

    return videoOutput->SetFrameRate(minFps, maxFps);
}
/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_GetActiveFrameRate(Camera_VideoOutput* videoOutput,
    Camera_FrameRateRange* frameRateRange)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(frameRateRange == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, frameRateRange is null!");

    return videoOutput->GetActiveFrameRate(frameRateRange);
}

/**
 * @since 15
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_IsMirrorSupported(Camera_VideoOutput* videoOutput, bool* isSupported)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(isSupported == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, isSupported is null!");
    return videoOutput->IsMirrorSupported(isSupported);
}

/**
 * @since 15
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_EnableMirror(Camera_VideoOutput* videoOutput, bool mirrorMode)
{
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    return videoOutput->EnableMirror(mirrorMode);
}

/**
 * @since 12
 * @version 1.0
 */
Camera_ErrorCode OH_VideoOutput_GetVideoRotation(Camera_VideoOutput* videoOutput, int deviceDegree,
    Camera_ImageRotation* imageRotation)
{
    MEDIA_DEBUG_LOG("OH_VideoOutput_GetVideoRotation is called.");
    CHECK_RETURN_RET_ELOG(videoOutput == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, videoOutput is null!");
    CHECK_RETURN_RET_ELOG(imageRotation == nullptr, CAMERA_INVALID_ARGUMENT,
        "Invalid argument, imageRotation is null!");
    return videoOutput->GetVideoRotation(deviceDegree, imageRotation);
}
#ifdef __cplusplus
}
#endif