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

#ifndef HSTREAM_METADATA_CALLBACK_STUB_FUZZER_H
#define HSTREAM_METADATA_CALLBACK_STUB_FUZZER_H

#include "stream_metadata_callback_stub.h"
#include "fuzzer/FuzzedDataProvider.h"

namespace OHOS {
namespace CameraStandard {
class HStreamMetadataCallbackStubFuzz : public StreamMetadataCallbackStub {
public:
    int32_t OnMetadataResult(
        const int32_t streamId, const std::shared_ptr<OHOS::Camera::CameraMetadata> &result) override
    {
        return 0;
    }
};

class HStreamMetadataCallbackStubFuzzer {
public:
    static std::shared_ptr<HStreamMetadataCallbackStubFuzz> fuzz_;
    static void HStreamMetadataCallbackStubFuzzerTest(FuzzedDataProvider& fdp);
};
}  // namespace CameraStandard
}  // namespace OHOS
#endif  // HSTREAM_METADATA_CALLBACK_STUB_FUZZER_H