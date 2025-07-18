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

#include "output/depth_data_output.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <variant>

#include "camera_error_code.h"
#include "camera_log.h"
#include "camera_output_capability.h"
#include "camera_util.h"
#include "session/capture_session.h"

namespace OHOS {
namespace CameraStandard {

int32_t DepthDataOutputCallbackImpl::OnDepthDataError(int32_t errorCode)
{
    auto item = depthDataOutput_.promote();
    if (item != nullptr) {
        auto callback = item->GetApplicationCallback();
        if (callback != nullptr) {
            callback->OnDepthDataError(errorCode);
        } else {
            MEDIA_INFO_LOG("Discarding DepthDataOutputCallbackImpl::OnDepthDataError callback in depthoutput");
        }
    } else {
        MEDIA_INFO_LOG("DepthDataOutputCallbackImpl::OnDepthDataError DepthDataOutput is nullptr");
    }
    return CAMERA_OK;
}

DepthDataOutput::DepthDataOutput(sptr<IBufferProducer> bufferProducer)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE_DEPTH_DATA, StreamType::DEPTH, bufferProducer, nullptr)
{
    DepthDataFormat_ = 0;
    DepthDataSize_.height = 0;
    DepthDataSize_.width = 0;
}

DepthDataOutput::~DepthDataOutput()
{
    MEDIA_DEBUG_LOG("Enter Into DepthDataOutput::~DepthDataOutput()");
}

int32_t DepthDataOutput::Start()
{
    CAMERA_SYNC_TRACE;
    SetDataAccuracy(GetDepthProfile()->GetDataAccuracy());
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    MEDIA_DEBUG_LOG("Enter Into DepthDataOutput::Start");
    auto captureSession = GetSession();
    CHECK_RETURN_RET_ELOG(captureSession == nullptr || !captureSession->IsSessionCommited(),
        CameraErrorCode::SESSION_NOT_CONFIG, "DepthDataOutput Failed to Start!, session not config");
    CHECK_RETURN_RET_ELOG(GetStream() == nullptr, CameraErrorCode::SERVICE_FATL_ERROR,
        "DepthDataOutput Failed to Start!, GetStream is nullptr");
    auto stream = GetStream();
    sptr<IStreamDepthData> itemStream = static_cast<IStreamDepthData*>(stream.GetRefPtr());
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    CHECK_PRINT_ELOG(itemStream == nullptr, "DepthDataOutput::Start itemStream is nullptr");
    if (itemStream) {
        errCode = itemStream->Start();
        CHECK_PRINT_ELOG(errCode != CAMERA_OK, "DepthDataOutput Failed to Start!, errCode: %{public}d", errCode);
    }
    return ServiceToCameraError(errCode);
}

int32_t DepthDataOutput::Stop()
{
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    MEDIA_DEBUG_LOG("Enter Into DepthDataOutput::Stop");
    CHECK_RETURN_RET_ELOG(GetStream() == nullptr, CameraErrorCode::SERVICE_FATL_ERROR,
        "DepthDataOutput Failed to Stop!, GetStream is nullptr");
    auto stream = GetStream();
    sptr<IStreamDepthData> itemStream = static_cast<IStreamDepthData*>(stream.GetRefPtr());
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    CHECK_PRINT_ELOG(itemStream == nullptr, "DepthDataOutput::Stop itemStream is nullptr");
    if (itemStream) {
        errCode = itemStream->Stop();
        CHECK_PRINT_ELOG(errCode != CAMERA_OK, "DepthDataOutput Failed to Stop!, errCode: %{public}d", errCode);
    }
    return ServiceToCameraError(errCode);
}

int32_t DepthDataOutput::SetDataAccuracy(int32_t dataAccuracy)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    MEDIA_DEBUG_LOG("Enter Into DepthDataOutput::SetDataAccuracy");
    CHECK_RETURN_RET_ELOG(GetStream() == nullptr, CameraErrorCode::SERVICE_FATL_ERROR,
        "DepthDataOutput Failed to SetDataAccuracy!, GetStream is nullptr");
    auto stream = GetStream();
    sptr<IStreamDepthData> itemStream = static_cast<IStreamDepthData*>(stream.GetRefPtr());
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (itemStream) {
        errCode = itemStream->SetDataAccuracy(dataAccuracy);
        CHECK_PRINT_ELOG(errCode != CAMERA_OK,
            "DepthDataOutput Failed to SetDataAccuracy!, errCode: %{public}d", errCode);
    } else {
        MEDIA_ERR_LOG("DepthDataOutput::SetDataAccuracy itemStream is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t DepthDataOutput::CreateStream()
{
    MEDIA_INFO_LOG("DepthDataOutput::CreateStream enter");
    return CameraErrorCode::SUCCESS;
}

int32_t DepthDataOutput::Release()
{
    {
        std::lock_guard<std::mutex> lock(outputCallbackMutex_);
        svcCallback_ = nullptr;
        appCallback_ = nullptr;
    }
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    MEDIA_DEBUG_LOG("Enter Into DepthDataOutput::Release");
    CHECK_RETURN_RET_ELOG(GetStream() == nullptr, CameraErrorCode::SERVICE_FATL_ERROR,
        "DepthDataOutput Failed to Release!, GetStream is nullptr");
    auto stream = GetStream();
    sptr<IStreamDepthData> itemStream = static_cast<IStreamDepthData*>(stream.GetRefPtr());
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (itemStream) {
        errCode = itemStream->Release();
        CHECK_PRINT_ELOG(errCode != CAMERA_OK, "Failed to release DepthDataOutput!, errCode: %{public}d", errCode);
    } else {
        MEDIA_ERR_LOG("DepthDataOutput::Release() itemStream is nullptr");
    }
    CaptureOutput::Release();
    return ServiceToCameraError(errCode);
}

void DepthDataOutput::SetCallback(std::shared_ptr<DepthDataStateCallback> callback)
{
    std::lock_guard<std::mutex> lock(outputCallbackMutex_);
    appCallback_ = callback;
    if (appCallback_ != nullptr) {
        if (svcCallback_ == nullptr) {
            svcCallback_ = new (std::nothrow) DepthDataOutputCallbackImpl(this);
            if (svcCallback_ == nullptr) {
                MEDIA_ERR_LOG("new DepthDataOutputCallbackImpl Failed to register callback");
                appCallback_ = nullptr;
                return;
            }
        }
        CHECK_RETURN_ELOG(GetStream() == nullptr, "DepthDataOutput Failed to SetCallback!, GetStream is nullptr");
        auto stream = GetStream();
        sptr<IStreamDepthData> itemStream = static_cast<IStreamDepthData*>(stream.GetRefPtr());
        int32_t errorCode = CAMERA_OK;
        if (itemStream) {
            errorCode = itemStream->SetCallback(svcCallback_);
        } else {
            MEDIA_ERR_LOG("DepthDataOutput::SetCallback itemStream is nullptr");
        }
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("DepthDataOutput::SetCallback Failed to register callback, errorCode: %{public}d", errorCode);
            svcCallback_ = nullptr;
            appCallback_ = nullptr;
        }
    }
    return;
}

std::shared_ptr<DepthDataStateCallback> DepthDataOutput::GetApplicationCallback()
{
    std::lock_guard<std::mutex> lock(outputCallbackMutex_);
    return appCallback_;
}

void DepthDataOutput::CameraServerDied(pid_t pid)
{
    MEDIA_ERR_LOG("camera server has died, pid:%{public}d!", pid);
    std::lock_guard<std::mutex> lock(outputCallbackMutex_);
    if (appCallback_ != nullptr) {
        MEDIA_DEBUG_LOG("appCallback not nullptr");
        int32_t serviceErrorType = ServiceToCameraError(CAMERA_INVALID_STATE);
        appCallback_->OnDepthDataError(serviceErrorType);
    }
}
} // namespace CameraStandard
} // namespace OHOS
