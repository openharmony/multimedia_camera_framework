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

#ifndef OHOS_DEFERRED_PROCESSING_SERVICE_TASK_MANAGER_H
#define OHOS_DEFERRED_PROCESSING_SERVICE_TASK_MANAGER_H
#define EXPORT_API __attribute__((visibility("default")))

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "task_registry.h"
#include "thread_pool.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class TaskManager {
public:
    EXPORT_API TaskManager(const std::string& name, uint32_t  numThreads, bool serial);
    EXPORT_API ~TaskManager();
    void CreateDelayedTaskGroupIfNeed();
    void BeginBackgroundTasks();
    void EndBackgroundTasks();
    bool RegisterTaskGroup(const std::string& name, TaskFunc func, bool serial, TaskGroupHandle& handle);
    bool DeregisterTaskGroup(const std::string& name, TaskGroupHandle& handle);
    EXPORT_API bool SubmitTask(std::function<void()> task);
    bool SubmitTask(std::function<void()> task, uint32_t delayMilli);
    bool SubmitTask(TaskGroupHandle handle, std::any param = std::any());
    EXPORT_API void CancelAllTasks();
    EXPORT_API bool IsEmpty();
private:
    void Initialize();
    bool RegisterTaskGroup(const std::string& name, TaskFunc func, bool serial, bool delayTask,
        TaskGroupHandle& handle);
    void DoDefaultWorks(std::any param);

    const std::string name_;
    const uint32_t numThreads_;
    bool serial_ = true;
    std::unique_ptr<ThreadPool> pool_;
    std::unique_ptr<TaskRegistry> taskRegistry_;
    std::shared_ptr<TaskGroupHandle> defaultTaskHandle_ =
        std::make_shared<TaskGroupHandle>(INVALID_TASK_GROUP_HANDLE);
    TaskGroupHandle delayedTaskHandle_;
};
} //namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_DEFERRED_PROCESSING_SERVICE_TASK_MANAGER_H
