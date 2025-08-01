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

#ifndef IMAGE_SOURCE_PROXY_FUZZER_H
#define IMAGE_SOURCE_PROXY_FUZZER_H

#include "image_source_proxy.h"
#include <fuzzer/FuzzedDataProvider.h>

namespace OHOS {
namespace CameraStandard {

class ImageSourceProxyFuzzer {
public:
    static void ImageSourceProxyFuzzerTest(FuzzedDataProvider& fdp);
    static std::shared_ptr<ImageSourceProxy> imageSourceProxyFuzz_;
};

} // CameraStandard
} // OHOS
#endif // IMAGE_SOURCE_PROXY_FUZZER_H