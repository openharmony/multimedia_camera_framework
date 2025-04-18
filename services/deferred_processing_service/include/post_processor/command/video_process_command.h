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

#ifndef OHOS_CAMERA_DPS_VIDEO_PROCESS_COMMAND_H
#define OHOS_CAMERA_DPS_VIDEO_PROCESS_COMMAND_H

#include "command.h"
#include "scheduler_manager.h"
#include "video_post_processor.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class VideoProcessCommand : public Command {
public:
    VideoProcessCommand(const int32_t userId);
    ~VideoProcessCommand() = default;

protected:
    int32_t Initialize();

    const int32_t userId_;
    std::atomic<bool> initialized_ {false};
    std::shared_ptr<VideoPostProcessor> videoPostProcess_ {nullptr};
};

class VideoProcessSuccessCommand : public VideoProcessCommand {
    DECLARE_CMD_CLASS(VideoProcessSuccessCommand);
public:
    VideoProcessSuccessCommand(const int32_t userId, const std::string& videoId,
        std::unique_ptr<MediaUserInfo> userInfo = nullptr)
        : VideoProcessCommand(userId), videoId_(videoId), userInfo_(std::move(userInfo))
    {
        DP_DEBUG_LOG("entered.");
    }
    ~VideoProcessSuccessCommand() = default;

protected:
    int32_t Executing() override;

    const std::string videoId_;
    std::unique_ptr<MediaUserInfo> userInfo_;
};

class VideoProcessFailedCommand : public VideoProcessCommand {
    DECLARE_CMD_CLASS(VideoProcessFailedCommand);
public:
    VideoProcessFailedCommand(const int32_t userId, const std::string& videoId, DpsError errorCode)
        : VideoProcessCommand(userId), videoId_(videoId), error_(errorCode)
    {
        DP_DEBUG_LOG("entered.");
    }
    ~VideoProcessFailedCommand() = default;

protected:
    int32_t Executing() override;

    const std::string videoId_;
    DpsError error_;
};

class VideoStateChangedCommand : public VideoProcessCommand {
    DECLARE_CMD_CLASS(VideoStateChangedCommand);
public:
    VideoStateChangedCommand(const int32_t userId, HdiStatus status)
        : VideoProcessCommand(userId), status_(status)
    {
        DP_DEBUG_LOG("entered.");
    }
    ~VideoStateChangedCommand() = default;

protected:
    int32_t Executing() override;

    HdiStatus status_;
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_VIDEO_PROCESS_COMMAND_H