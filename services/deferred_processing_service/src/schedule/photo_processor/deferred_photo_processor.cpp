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

#include "deferred_photo_processor.h"

#include "dp_log.h"
#include "dps_event_report.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
DeferredPhotoProcessor::DeferredPhotoProcessor(const int32_t userId, TaskManager* taskManager,
    std::shared_ptr<PhotoJobRepository> repository, std::shared_ptr<IImageProcessCallbacks> callbacks)
    : userId_(userId),
      taskManager_(taskManager),
      repository_(repository),
      postProcessor_(nullptr),
      callbacks_(callbacks),
      requestedImages_()
{
    DP_DEBUG_LOG("entered");
    postProcessor_ = std::make_shared<PhotoPostProcessor>(userId_, taskManager, this);
}

DeferredPhotoProcessor::~DeferredPhotoProcessor()
{
    DP_DEBUG_LOG("entered");
    taskManager_ = nullptr;
    repository_ = nullptr;
    postProcessor_ = nullptr;
    callbacks_ = nullptr;
}

void DeferredPhotoProcessor::Initialize()
{
    postProcessor_->Initialize();
}

void DeferredPhotoProcessor::AddImage(const std::string& imageId, bool discardable, DpsMetadata& metadata)
{
    DP_INFO_LOG("entered");
    repository_->AddDeferredJob(imageId, discardable, metadata);
    return;
}

void DeferredPhotoProcessor::RemoveImage(const std::string& imageId, bool restorable)
{
    DP_INFO_LOG("entered");
    if (requestedImages_.count(imageId) != 0) {
        requestedImages_.erase(imageId);
    }
    DP_CHECK_ERROR_RETURN_LOG(repository_ == nullptr, "repository_ is nullptr");

    if (restorable == false) {
        if (repository_->GetJobStatus(imageId) == PhotoJobStatus::RUNNING) {
            DP_CHECK_ERROR_RETURN_LOG(postProcessor_ == nullptr, "postProcessor_ is nullptr, RemoveImage failed.");
            postProcessor_->Interrupt();
        }
        DP_CHECK_ERROR_RETURN_LOG(postProcessor_ == nullptr, "postProcessor_ is nullptr, RemoveImage failed.");
        postProcessor_->RemoveImage(imageId);
    }
    repository_->RemoveDeferredJob(imageId, restorable);
}

void DeferredPhotoProcessor::RestoreImage(const std::string& imageId)
{
    DP_INFO_LOG("entered");
    repository_->RestoreJob(imageId);
    return;
}

void DeferredPhotoProcessor::ProcessImage(const std::string& appName, const std::string& imageId)
{
    DP_INFO_LOG("entered");
    if (!repository_->IsOfflineJob(imageId)) {
        DP_INFO_LOG("imageId is not offlineJob %{public}s", imageId.c_str());
        return;
    }
    requestedImages_.insert(imageId);
    bool isImageIdValid = repository_->RequestJob(imageId);
    if (!isImageIdValid) {
        if (callbacks_) {
            callbacks_->OnError(userId_, imageId, DpsError::DPS_ERROR_IMAGE_PROC_INVALID_PHOTO_ID);
        }
    } else {
        if (repository_->GetJobPriority(postedImageId_) != PhotoJobPriority::HIGH) {
            Interrupt();
        }
    }
    return;
}

void DeferredPhotoProcessor::CancelProcessImage(const std::string& imageId)
{
    DP_INFO_LOG("entered");
    if (requestedImages_.count(imageId) != 0) {
        requestedImages_.erase(imageId);
    }
    repository_->CancelJob(imageId);
    return;
}

void DeferredPhotoProcessor::OnProcessDone(const int32_t userId, const std::string& imageId,
    std::shared_ptr<BufferInfo> bufferInfo)
{
    DP_INFO_LOG("entered, userId: %{public}d, imageId: %{public}s.", userId, imageId.c_str());
    //如果已经非高优先级，且任务结果不是全质量的图，那么不用返回给上层了，等下次出全质量图再返回
    if (!(bufferInfo->IsHighQuality())) {
        DP_INFO_LOG("not high quality photo");
        if ((repository_->GetJobPriority(imageId) != PhotoJobPriority::HIGH)) {
            DP_INFO_LOG("not high quality and not high priority, need retry");
            repository_->SetJobPending(imageId);
            return;
        } else {
            DP_INFO_LOG("not high quality, but high priority, and process as normal job before, need retry");
            if (repository_->GetJobRunningPriority(imageId) != PhotoJobPriority::HIGH) {
                repository_->SetJobPending(imageId);
                return;
            }
        }
    }
    repository_->SetJobCompleted(imageId);
    callbacks_->OnProcessDone(userId, imageId, std::move(bufferInfo));
}

void DeferredPhotoProcessor::OnProcessDoneExt(const int32_t userId, const std::string& imageId,
    std::shared_ptr<BufferInfoExt> bufferInfo)
{
    DP_INFO_LOG("entered");
    //如果已经非高优先级，且任务结果不是全质量的图，那么不用返回给上层了，等下次出全质量图再返回
    if (!(bufferInfo->IsHighQuality())) {
        DP_INFO_LOG("not high quality photo");
        if ((repository_->GetJobPriority(imageId) != PhotoJobPriority::HIGH)) {
            DP_INFO_LOG("not high quality and not high priority, need retry");
            repository_->SetJobPending(imageId);
            return;
        } else {
            DP_INFO_LOG("not high quality, but high priority, and process as normal job before, need retry");
            if (repository_->GetJobRunningPriority(imageId) != PhotoJobPriority::HIGH) {
                repository_->SetJobPending(imageId);
                return;
            }
        }
    }
    repository_->SetJobCompleted(imageId);
    callbacks_->OnProcessDoneExt(userId, imageId, std::move(bufferInfo));
}

void DeferredPhotoProcessor::OnError(const int32_t userId, const std::string& imageId, DpsError errorCode)
{
    DP_INFO_LOG("entered");
    if (errorCode == DpsError::DPS_ERROR_IMAGE_PROC_INTERRUPTED &&
        repository_->GetJobPriority(imageId) == PhotoJobPriority::HIGH &&
        requestedImages_.count(imageId) != 0) {
        repository_->SetJobPending(imageId);
        return;
    }

    if (!IsFatalError(errorCode)) {
        if (repository_->GetJobPriority(imageId) == PhotoJobPriority::HIGH) {
            if (repository_->GetJobRunningPriority(imageId) != PhotoJobPriority::HIGH) {
                repository_->SetJobPending(imageId);
                return;
            }
        } else {
            repository_->SetJobFailed(imageId);
            return;
        }
    }
    repository_->SetJobFailed(imageId);
    callbacks_->OnError(userId, imageId, errorCode);
    return;
}

void DeferredPhotoProcessor::OnStateChanged(const int32_t userId, DpsStatus statusCode)
{
    (void)(userId);
    (void)(statusCode);
}

void DeferredPhotoProcessor::NotifyScheduleState(DpsStatus status)
{
    if (callbacks_) {
        callbacks_->OnStateChanged(userId_, status);
    }
}

void DeferredPhotoProcessor::PostProcess(DeferredPhotoWorkPtr work)
{
    DP_INFO_LOG("entered");
    auto executionMode = work->GetExecutionMode();
    auto imageId = work->GetDeferredPhotoJob()->GetImageId();
    if (requestedImages_.count(imageId) != 0) {
        requestedImages_.erase(imageId);
    }
    postedImageId_ = imageId;
    repository_->SetJobRunning(imageId);
    postProcessor_->SetExecutionMode(executionMode);
    postProcessor_->ProcessImage(imageId);
    DPSEventReport::GetInstance().ReportImageModeChange(executionMode);
    return;
}

void DeferredPhotoProcessor::SetDefaultExecutionMode()
{
    DP_INFO_LOG("entered");
    postProcessor_->SetDefaultExecutionMode();
}

void DeferredPhotoProcessor::Interrupt()
{
    DP_INFO_LOG("entered");
    postProcessor_->Interrupt();
    return;
}

int DeferredPhotoProcessor::GetConcurrency(ExecutionMode mode)
{
    DP_INFO_LOG("entered");
    return postProcessor_->GetConcurrency(mode);
}

bool DeferredPhotoProcessor::GetPendingImages(std::vector<std::string>& pendingImages)
{
    DP_INFO_LOG("entered");
    if (postProcessor_ == nullptr) {
        DP_ERR_LOG("postProcessor_ is nullptr");
        return false;
    }
    bool isSuccess = postProcessor_->GetPendingImages(pendingImages);
    if (isSuccess) {
        return true;
    }
    return false;
}

bool DeferredPhotoProcessor::IsFatalError(DpsError errorCode)
{
    DP_INFO_LOG("entered, code: %{public}d", errorCode);
    if (errorCode == DpsError::DPS_ERROR_IMAGE_PROC_FAILED ||
        errorCode == DpsError::DPS_ERROR_IMAGE_PROC_INVALID_PHOTO_ID) {
        return true;
    } else {
        return false;
    }
}
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS