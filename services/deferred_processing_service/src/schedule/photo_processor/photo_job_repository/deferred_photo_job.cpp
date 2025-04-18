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

#include "deferred_photo_job.h"
#include "dp_log.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {


DeferredPhotoWork::DeferredPhotoWork(DeferredPhotoJobPtr jobPtr, ExecutionMode mode)
    : jobPtr_(jobPtr),
      executionMode_(mode)
{
    //DPS_LOG
}

DeferredPhotoWork::~DeferredPhotoWork()
{
    jobPtr_ = nullptr;
}

DeferredPhotoJobPtr DeferredPhotoWork::GetDeferredPhotoJob()
{
    return jobPtr_;
}

ExecutionMode DeferredPhotoWork::GetExecutionMode()
{
    return executionMode_;
}

DeferredPhotoJob::DeferredPhotoJob(const std::string& imageId, bool discardable, DpsMetadata& metadata)
    : imageId_(imageId),
      discardable_(discardable),
      metadata_(metadata),
      prePriority_(PhotoJobPriority::NONE),
      curPriority_(PhotoJobPriority::NONE),
      runningPriority_(PhotoJobPriority::NONE),
      preStatus_(PhotoJobStatus::NONE),
      curStatus_(PhotoJobStatus::NONE),
      photoJobType_(0)
{
    //DPS_LOG
}

DeferredPhotoJob::~DeferredPhotoJob()
{
    //DPS_LOG
}

PhotoJobPriority DeferredPhotoJob::GetCurPriority()
{
    DP_DEBUG_LOG("imageId: %{public}s, current priority: %{public}d, previous priority: %{public}d",
        imageId_.c_str(), curPriority_, prePriority_);
    return curPriority_;
}

PhotoJobPriority DeferredPhotoJob::GetPrePriority()
{
    DP_DEBUG_LOG("imageId: %{public}s, current priority: %{public}d, previous priority: %{public}d",
        imageId_.c_str(), curPriority_, prePriority_);
    return prePriority_;
}

PhotoJobPriority DeferredPhotoJob::GetRunningPriority()
{
    DP_DEBUG_LOG("imageId: %{public}s, current priority: %{public}d, previous priority: %{public}d,"
        "running priority: %{public}d", imageId_.c_str(), curPriority_, prePriority_, runningPriority_);
    return runningPriority_;
}

PhotoJobStatus DeferredPhotoJob::GetCurStatus()
{
    DP_DEBUG_LOG("imageId: %{public}s, current status: %{public}d, previous status: %{public}d",
        imageId_.c_str(), curStatus_, preStatus_);
    return curStatus_;
}

PhotoJobStatus DeferredPhotoJob::GetPreStatus()
{
    DP_DEBUG_LOG("imageId: %{public}s, current status: %{public}d, previous status: %{public}d",
        imageId_.c_str(), curStatus_, preStatus_);
    return preStatus_;
}

std::string DeferredPhotoJob::GetImageId()
{
    return imageId_;
}


int32_t DeferredPhotoJob::GetDeferredProcType()
{
    int32_t type;
    metadata_.Get(DEFERRED_PROCESSING_TYPE_KEY, type);
    DP_DEBUG_LOG("imageId: %{public}s, deferred proc type: %{public}d", imageId_.c_str(), type);
    return type;
}

bool DeferredPhotoJob::GetDiscardable()
{
    return discardable_;
}

void DeferredPhotoJob::SetPhotoJobType(int photoJobType)
{
    photoJobType_ = photoJobType;
}

int DeferredPhotoJob::GetPhotoJobType()
{
    return photoJobType_;
}

bool DeferredPhotoJob::SetJobStatus(PhotoJobStatus status)
{
    DP_DEBUG_LOG("imageId: %{public}s, current status: %{public}d, "
        "previous status: %{public}d, status to set: %{public}d",
        imageId_.c_str(), curStatus_, preStatus_, status);
    if (curStatus_ == status) {
        return false;
    } else {
        preStatus_ = curStatus_;
        curStatus_ = status;
        return true;
    }
}

bool DeferredPhotoJob::SetJobPriority(PhotoJobPriority priority)
{
    DP_DEBUG_LOG("imageId: %{public}s, current priority: %{public}d, "
        "previous priority: %{public}d, priority to set: %{public}d",
        imageId_.c_str(), curPriority_, prePriority_, priority);
    if (curPriority_ == priority) {
        return false;
    } else {
        prePriority_ = curPriority_;
        curPriority_ = priority;
        return true;
    }
}

void DeferredPhotoJob::RecordJobRunningPriority()
{
    DP_DEBUG_LOG("imageId: %{public}s, priority recorded: %{public}d, "
        "current priority: %{public}d, previous priority: %{public}d",
        imageId_.c_str(), curPriority_, prePriority_, curPriority_);
    runningPriority_ = curPriority_;
}
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS