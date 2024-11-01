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

#ifndef OHOS_CAMERA_XCOLLIE_H
#define OHOS_CAMERA_XCOLLIE_H

#include <string>
#include "xcollie/xcollie_define.h"

namespace OHOS {
class CameraXCollie {
public:
    explicit CameraXCollie(const std::string& tag,
      uint32_t flag = (HiviewDFX::XCOLLIE_FLAG_LOG | HiviewDFX::XCOLLIE_FLAG_RECOVERY),
      uint32_t timeoutSeconds = 10, std::function<void(void *)> func = nullptr, void* arg = nullptr);
    ~CameraXCollie();

    void CancelCameraXCollie();

private:
    int32_t id_;
    std::string tag_;
    bool isCanceled_;
};
}
#endif // OHOS_CAMERA_XCOLLIE_H