/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_DPS_VIDEO_POST_PROCESSOR_H
#define OHOS_CAMERA_DPS_VIDEO_POST_PROCESSOR_H

#include "basic_definitions.h"
#include "deferred_video_job.h"
#include "iservstat_listener_hdi.h"
#include "ivideo_process_callbacks.h"
#include "media_manager_proxy.h"
#include "v1_4/ivideo_process_service.h"
#include "v1_4/ivideo_process_callback.h"
#include "video_process_result.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
using OHOS::HDI::Camera::V1_3::IVideoProcessSession;
using OHOS::HDI::Camera::V1_3::StreamDescription;
using OHOS::HDI::Camera::V1_1::StreamInfo_V1_1;
using HDI::Camera::V1_0::BufferProducerSequenceable;

class VideoPostProcessor : public std::enable_shared_from_this<VideoPostProcessor> {
public:
    ~VideoPostProcessor();

    void Initialize();
    bool GetPendingVideos(std::vector<std::string>& pendingVideos);
    void SetExecutionMode(ExecutionMode executionMode);
    void SetDefaultExecutionMode();
    void ProcessRequest(const DeferredVideoWorkPtr& work);
    void RemoveRequest(const std::string& videoId);
    void PauseRequest(const std::string& videoId, const SchedulerType& type);
    DeferredVideoWorkPtr GetRunningWork(const std::string& videoId);
    void OnProcessDone(const std::string& videoId, std::unique_ptr<MediaUserInfo> userInfo);
    void OnError(const std::string& videoId, DpsError errorCode);
    void OnStateChanged(HdiStatus hdiStatus);
    void OnSessionDied();

protected:
    explicit VideoPostProcessor(const int32_t userId);

private:
    class VideoServiceListener;
    class SessionDeathRecipient;
    class VideoProcessListener;

    void ConnectService();
    void DisconnectService();
    DpsError PrepareStreams(const std::string& videoId, const int inputFd);
    bool ProcessStream(const StreamDescription& stream);
    void ReleaseStreams();
    void SetStreamInfo(const StreamDescription& stream, sptr<BufferProducerSequenceable>& producer);
    HDI::Camera::V1_0::StreamIntent GetIntent(HDI::Camera::V1_3::MediaStreamType type);
    bool StartMpeg(const std::string& videoId, const sptr<IPCFileDescriptor>& inputFd);
    bool StopMpeg(const MediaResult result, const DeferredVideoWorkPtr& work);
    void ReleaseMpeg();
    void StartTimer(const std::string& videoId, const DeferredVideoWorkPtr& work);
    void StopTimer(const DeferredVideoWorkPtr& work);
    void OnTimerOut(const std::string& videoId);
    void OnServiceChange(const HDI::ServiceManager::V1_0::ServiceStatus& status);
    void copyFileByFd(const int srcFd, const int dstFd);
    DpsError MapHdiError(OHOS::HDI::Camera::V1_2::ErrorCode errorCode);

    inline sptr<IVideoProcessSession> GetVideoSession()
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        return session_;
    }

    inline void SetVideoSession(const sptr<IVideoProcessSession>& session)
    {
        std::lock_guard<std::mutex> lock(sessionMutex_);
        session_ = session;
    }

    std::mutex sessionMutex_;
    const int32_t userId_;
    sptr<IVideoProcessSession> session_ {nullptr};
    std::shared_ptr<MediaManagerProxy> mediaManagerProxy_ {nullptr};
    std::shared_ptr<VideoProcessResult> processResult_ {nullptr};
    sptr<VideoServiceListener> serviceListener_;
    sptr<SessionDeathRecipient> sessionDeathRecipient_;
    sptr<VideoProcessListener> processListener_;
    std::vector<StreamInfo_V1_1> allStreamInfo_ {};
    std::unordered_map<std::string, DeferredVideoWorkPtr> runningWork_ {};
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_VIDEO_POST_PROCESSOR_H