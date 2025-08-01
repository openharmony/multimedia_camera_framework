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

#ifndef HCAPTURE_SEESION_FUZZER_H
#define HCAPTURE_SEESION_FUZZER_H

#include "hcapture_session.h"
#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace CameraStandard {

class HCaptureSessionFuzzer {
public:
static void HCaptureSessionFuzzTest1(FuzzedDataProvider& fdp);
static void HCaptureSessionFuzzTest2(FuzzedDataProvider& fdp);
static void HCaptureSessionFuzzTest3(FuzzedDataProvider& fdp);
static void HCaptureSessionFuzzTest4(FuzzedDataProvider& fdp);
};

} //CameraStandard
} //CameraStandard
#endif //HCAPTURE_SEESION_FUZZER_H