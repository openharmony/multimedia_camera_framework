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

#include "hstream_operator_manager.h"
#include "hstream_operator.h"
#include "camera_dynamic_loader.h"
#include "camera_log.h"
#include "moving_photo_proxy.h"

namespace OHOS {
namespace CameraStandard {
sptr<HStreamOperatorManager> HStreamOperatorManager::streamOperatorManager_;
std::mutex HStreamOperatorManager::instanceMutex_;

HStreamOperatorManager::HStreamOperatorManager() {}

HStreamOperatorManager::~HStreamOperatorManager()
{
    MEDIA_INFO_LOG("~HStreamOperatorManager");
    CHECK_RETURN(streamOperatorManagerMap_.empty());
    MEDIA_INFO_LOG("~HStreamOperatorManager clear maps");
    // avoid double free
    {
        decltype(streamOperatorManagerMap_) tempOpMap = std::move(streamOperatorManagerMap_);
    }
    MEDIA_INFO_LOG("~HStreamOperatorManager free dynamic lib");
    CameraDynamicLoader::FreeDynamicLibDelayed(MEDIA_LIB_SO, LIB_DELAYED_UNLOAD_TIME);
}

sptr<HStreamOperatorManager> &HStreamOperatorManager::GetInstance()
{
    if (HStreamOperatorManager::streamOperatorManager_ == nullptr) {
        std::unique_lock<std::mutex> lock(instanceMutex_);
        if (HStreamOperatorManager::streamOperatorManager_ == nullptr) {
            MEDIA_INFO_LOG("Initializing stream operator manager instance");
            HStreamOperatorManager::streamOperatorManager_ = new HStreamOperatorManager();
        }
    }
    return HStreamOperatorManager::streamOperatorManager_;
}

void HStreamOperatorManager::AddStreamOperator(sptr<HStreamOperator> hStreamOperator)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    MEDIA_INFO_LOG("HStreamOperatorManager::AddStreamOperator start");
    int32_t streamOperatorId = GenerateStreamOperatorId();
    streamOperatorManagerMap_[streamOperatorId] = hStreamOperator;
    hStreamOperator->SetStreamOperatorId(streamOperatorId);
    MEDIA_INFO_LOG("HStreamOperatorManager::AddStreamOperator end hStreamOperatorId is %{public}d", streamOperatorId);
    return;
}

void HStreamOperatorManager::RemoveStreamOperator(int32_t& hStreamOperatorId)
{
    MEDIA_INFO_LOG("HStreamOperatorManager::RemoveStreamOperator hStreamOperatorId is %{public}d", hStreamOperatorId);
    std::lock_guard<std::mutex> lock(mapMutex_);
    CHECK_RETURN(hStreamOperatorId < 0);
    auto streamOperator = streamOperatorManagerMap_.find(hStreamOperatorId);
    CHECK_RETURN_ELOG(streamOperator == streamOperatorManagerMap_.end(), "not found hStreamOperatorId: %{public}d",
        hStreamOperatorId);
    streamOperatorManagerMap_.erase(hStreamOperatorId);
    if (streamOperatorManagerMap_.size() == 0) {
        CameraDynamicLoader::FreeDynamicLibDelayed(MEDIA_LIB_SO, LIB_DELAYED_UNLOAD_TIME);
    }
    MEDIA_INFO_LOG("HStreamOperatorManager::RemoveStreamOperator end");
    return;
}

void HStreamOperatorManager::AddTaskManager(int32_t& hStreamOperatorId,
    sptr<AvcodecTaskManagerIntf> avcodecTaskManagerProxy)
{
    CHECK_RETURN_ELOG(
        avcodecTaskManagerProxy == nullptr, "HStreamOperatorManager::AddTaskManager avcodecTaskManagerProxy is null");
    MEDIA_INFO_LOG("HStreamOperatorManager::AddTaskManager hStreamOperatorId is %{public}d", hStreamOperatorId);
    taskManagerMap_.EnsureInsert(hStreamOperatorId, avcodecTaskManagerProxy);
}

void HStreamOperatorManager::RemoveTaskManager(int32_t& hStreamOperatorId)
{
    MEDIA_INFO_LOG("HStreamOperatorManager::RemoveTaskManager hStreamOperatorId is %{public}d", hStreamOperatorId);
    sptr<AvcodecTaskManagerIntf> avcodecTaskManagerProxy = nullptr;
    taskManagerMap_.Find(hStreamOperatorId, avcodecTaskManagerProxy);
    CHECK_RETURN_ELOG(!avcodecTaskManagerProxy, "not found hStreamOperatorId: %{public}d", hStreamOperatorId);
    thread asyncThread = thread([hStreamOperatorId, avcodecTaskManagerProxy]() {
        CAMERA_SYNC_TRACE;
        MEDIA_INFO_LOG(
            "HStreamOperatorManager::RemoveTaskManager thread hStreamOperatorId: %{public}d", hStreamOperatorId);
        int32_t delayTime = avcodecTaskManagerProxy == nullptr ||
            (avcodecTaskManagerProxy && avcodecTaskManagerProxy->isEmptyVideoFdMap()) ? 0 : 30;
        std::this_thread::sleep_for(std::chrono::seconds(delayTime));
    });
    taskManagerMap_.Erase(hStreamOperatorId);
    asyncThread.detach();
}

void HStreamOperatorManager::UpdateStreamOperator(int32_t& hStreamOperatorId)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    MEDIA_INFO_LOG("HStreamOperatorManager::UpdateStreamOperator hStreamOperatorId is %{public}d", hStreamOperatorId);
    auto StreamOperator = streamOperatorManagerMap_.find(hStreamOperatorId);
    CHECK_RETURN(StreamOperator == streamOperatorManagerMap_.end());
}

int32_t HStreamOperatorManager::GetOfflineOutputSize()
{
    MEDIA_INFO_LOG("HStreamOperatorManager::DfxReport size is %{public}zu", streamOperatorManagerMap_.size());
    int32_t offlineOutputCount = 0;
    for (auto streamOperator : streamOperatorManagerMap_) {
        int32_t tempSize = (streamOperator.second)->GetOfflineOutptSize();
        offlineOutputCount = offlineOutputCount + tempSize;
    }
    if (offlineOutputCount > 2) { // 2 is the threshold of the statistics
        MEDIA_INFO_LOG("HStreamOperatorManager::DfxReport offlineOutputCount is %{public}d", offlineOutputCount);
    }
    return offlineOutputCount;
}

std::vector<sptr<HStreamOperator>> HStreamOperatorManager::GetStreamOperatorByPid(pid_t pidRequest)
{
    std::lock_guard<std::mutex> lock(mapMutex_);
    std::vector<sptr<HStreamOperator>> streamOperatorVec = {};
    for (auto streamOperator : streamOperatorManagerMap_) {
        if (pidRequest == (streamOperator.second)->GetPid()) {
            streamOperatorVec.push_back(streamOperator.second);
            MEDIA_INFO_LOG("HStreamOperatorManager::GetCameraByPid find");
        }
    }
    MEDIA_INFO_LOG("HStreamOperatorManager::GetStreamOperatorByPid pid is %{public}d size is %{public}zu", pidRequest,
        streamOperatorVec.size());
    return streamOperatorVec;
}
} // namespace CameraStandard
} // namespace OHOS
