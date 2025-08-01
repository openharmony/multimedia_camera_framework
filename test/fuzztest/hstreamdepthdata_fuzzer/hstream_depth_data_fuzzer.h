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

#ifndef HSTREAM_DEPTH_DATA_FUZZER_H
#define HSTREAM_DEPTH_DATA_FUZZER_H

#include "hstream_depth_data.h"
#include "stream_depth_data_stub.h"
#include "hstream_common.h"

namespace OHOS {
namespace CameraStandard {

class HStreamDepthDataFuzzer {
public:
static std::shared_ptr<HStreamDepthData> fuzz_;
static void HStreamDepthDataFuzzTest();
};

} //CameraStandard
} //OHOS
#endif //HSTREAM_DEPTH_DATA_FUZZER_H