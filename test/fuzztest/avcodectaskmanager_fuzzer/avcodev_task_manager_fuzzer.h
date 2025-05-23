 /*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_TASK_MANAGER_FUZZER_H
#define AVCODEC_TASK_MANAGER_FUZZER_H

#include "avcodec_task_manager.h"
#include <memory>
#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace CameraStandard {
using namespace DeferredProcessing;
class AvcodecTaskManagerFuzzer {
public:
static std::shared_ptr<AvcodecTaskManager> fuzz_;
static void AvcodecTaskManagerFuzzTest(FuzzedDataProvider& fdp);
};
} //CameraStandard
} //OHOS
#endif //AVCODEC_TASK_MANAGER_FUZZER_H