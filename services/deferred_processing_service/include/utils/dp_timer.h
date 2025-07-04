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

#ifndef OHOS_CAMERA_DPS_TIMER_H
#define OHOS_CAMERA_DPS_TIMER_H

#include <timer.h>
#include "singleton.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class DpsTimer : public Singleton<DpsTimer> {
    DECLARE_SINGLETON(DpsTimer)

public:
    using TimerCallback = std::function<void()>;

    uint32_t StartTimer(const TimerCallback& callback, uint32_t interval);
    void StopTimer(uint32_t& timerId);

private:
    std::unique_ptr<OHOS::Utils::Timer> timer_ {nullptr};
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif  // OHOS_CAMERA_DPS_TIMER_H