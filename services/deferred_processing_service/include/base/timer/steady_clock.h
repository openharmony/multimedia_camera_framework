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

#ifndef OHOS_DEFERRED_PROCESSING_SERVICE_STEADY_CLOCK_H
#define OHOS_DEFERRED_PROCESSING_SERVICE_STEADY_CLOCK_H
#define EXPORT_API __attribute__((visibility("default")))

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>
#include "timer/camera_deferred_timer.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class SteadyClock {
public:
    EXPORT_API static uint64_t GetTimestampMilli();
    static uint64_t GetTimestampMicro();
    static uint64_t GetElapsedTimeMs(uint64_t startMs);
    static std::chrono::milliseconds GetRemainingTimeMs(uint64_t expirationTimeMs);
    SteadyClock();
    ~SteadyClock();
    void Reset();
    uint64_t GetElapsedTimeMs();

private:
    std::chrono::time_point<std::chrono::steady_clock> start_;
};
} //namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_DEFERRED_PROCESSING_SERVICE_STEADY_CLOCK_H
