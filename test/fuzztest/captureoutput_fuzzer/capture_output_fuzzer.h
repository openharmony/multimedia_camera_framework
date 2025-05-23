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

#ifndef CAPTURE_OUTPUT_FUZZER_H
#define CAPTURE_OUTPUT_FUZZER_H

#include "capture_output.h"
#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace CameraStandard {

class CaptureOutputTest : public CaptureOutput {
public:
    CaptureOutputTest(CaptureOutputType outputType,
        StreamType streamType, sptr<IBufferProducer> bufferProducer,
        sptr<IStreamCommon> stream): CaptureOutput(outputType, streamType, bufferProducer, stream){};
    int32_t Release() override
    {
        return 0;
    }
    void CameraServerDied(pid_t pid) override {}
    int32_t CreateStream() override
    {
        return 0;
    }
    void CaptureOutputTest1()
    {
        CaptureOutput::GetOutputTypeString();
    }
};

class CaptureOutputFuzzer {
public:
static std::shared_ptr<CaptureOutput> fuzz_;
static CaptureOutputTest captureOutputTest;
static void CaptureOutputFuzzTest(FuzzedDataProvider& fdp);
};
} //CameraStandard
} //OHOS
#endif //CAPTURE_OUTPUT_FUZZER_H