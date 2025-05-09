/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_DPS_SCHEDULE_MANAGER_H
#define OHOS_CAMERA_DPS_SCHEDULE_MANAGER_H

#include "deferred_photo_controller.h"
#include "deferred_video_controller.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class SchedulerManager {
public:
    static std::shared_ptr<SchedulerManager> Create();
    ~SchedulerManager();

    // 用于session获取对应的processor，如果有则直接返回 如果没有则创建后返回 这里的callback是sessioncoordinator对象
    // 创建任务仓库 processor  并把仓库传给processor，再创建processor对应的controller
    // controller向任务仓库注册任务监听（关注任务添加、任务running个数改变）+向事件监听模块注册监听
    int32_t Initialize();
    void CreatePhotoProcessor(const int32_t userId, const std::shared_ptr<IImageProcessCallbacks>& callbacks);
    std::shared_ptr<PhotoPostProcessor> GetPhotoPostProcessor(const int32_t userId);
    std::shared_ptr<DeferredPhotoProcessor> GetPhotoProcessor(const int32_t userId);
    void CreateVideoProcessor(const int32_t userId, const std::shared_ptr<IVideoProcessCallbacks>& callbacks);
    std::shared_ptr<VideoPostProcessor> GetVideoPostProcessor(const int32_t userId);
    std::shared_ptr<DeferredVideoProcessor> GetVideoProcessor(const int32_t userId);
    std::shared_ptr<DeferredVideoController> GetVideoController(const int32_t userId);

protected:
    SchedulerManager();

private:
    std::unordered_map<int32_t, std::shared_ptr<PhotoPostProcessor>> photoPosts_ {};
    std::unordered_map<int32_t, std::shared_ptr<DeferredPhotoProcessor>> photoProcessors_ {};
    std::unordered_map<int32_t, std::shared_ptr<DeferredPhotoController>> photoController_ {};
    std::unordered_map<int32_t, std::shared_ptr<VideoPostProcessor>> videoPosts_ {};
    std::unordered_map<int32_t, std::shared_ptr<DeferredVideoProcessor>> videoProcessors_ {};
    std::unordered_map<int32_t, std::shared_ptr<DeferredVideoController>> videoController_ {};
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_SCHEDULE_MANAGER_H