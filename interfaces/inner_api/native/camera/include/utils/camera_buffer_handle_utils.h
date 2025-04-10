/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_BUFFER_HANDLE_UTILS_H
#define OHOS_CAMERA_BUFFER_HANDLE_UTILS_H

#include <stdint.h>
#include <buffer_handle.h>

namespace OHOS {
namespace CameraStandard {

struct CameraBufferExtraData {
    int64_t imageId = 0;
    int32_t deferredProcessingType;
    int32_t photoWidth;
    int32_t photoHeight;
    int32_t extraDataSize = 0;
    int32_t deferredImageFormat = 0;
    int32_t isDegradedImage;
    uint64_t size;
    int32_t captureId;
};

BufferHandle *CameraAllocateBufferHandle(uint32_t reserveInts, uint32_t reserveFds);

int32_t CameraFreeBufferHandle(BufferHandle *handle);

BufferHandle *CameraCloneBufferHandle(const BufferHandle *handle);
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_BUFFER_HANDLE_UTILS_H
