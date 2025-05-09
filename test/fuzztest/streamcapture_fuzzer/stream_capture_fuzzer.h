/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef STREAM_CAPTURE_FUZZER_H
#define STREAM_CAPTURE_FUZZER_H
#define FUZZ_PROJECT_NAME "streamcapture_fuzzer"
#include <iostream>
#include "surface.h"
#include "hstream_capture.h"

namespace OHOS {
namespace CameraStandard {
void StreamCaptureFuzzTest(uint8_t *rawData, size_t size);
void StreamCaptureFuzzTestGetPermission();
} //CameraStandard
} //OHOS
#endif //STREAM_CAPTURE_FUZZER_H

