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

#ifndef STREAM_CAPTURE_STUB_FUZZER_H
#define STREAM_CAPTURE_STUB_FUZZER_H

#include "hstream_capture_stub.h"
#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace CameraStandard {
namespace StreamCaptureStubFuzzer {

void Test(uint8_t *data, size_t size);
void Test_OnRemoteRequest(FuzzedDataProvider& fdp);
void Test_HandleCapture(FuzzedDataProvider& fdp);
void Test_HandleSetThumbnail(FuzzedDataProvider& fdp);
void Test_HandleSetBufferProducerInfo(FuzzedDataProvider& fdp);
void Test_HandleEnableDeferredType(FuzzedDataProvider& fdp);
void Test_HandleSetCallback(FuzzedDataProvider& fdp);

void CheckPermission();
std::shared_ptr<OHOS::Camera::CameraMetadata> MakeMetadata(FuzzedDataProvider& fdp);

} //StreamCaptureStubFuzzer
} //CameraStandard
} //OHOS
#endif //STREAM_CAPTURE_STUB_FUZZER_H