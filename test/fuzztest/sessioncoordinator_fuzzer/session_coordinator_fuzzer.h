/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SESSION_COORDINATOR_FUZZER_H
#define SESSION_COORDINATOR_FUZZER_H

#include "session_coordinator.h"
#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace CameraStandard {
class SessionCoordinatorFuzzer {
public:
static std::shared_ptr<DeferredProcessing::SessionCoordinator> fuzz_;
static void SessionCoordinatorFuzzTest(FuzzedDataProvider& fdp);
};
} //CameraStandard
} //OHOS
#endif //SESSION_COORDINATOR_FUZZER_H