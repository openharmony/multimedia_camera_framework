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

#ifndef OHOS_DEFERRED_PROCESSING_SERVICE_BUFFER_INFO_H
#define OHOS_DEFERRED_PROCESSING_SERVICE_BUFFER_INFO_H

#include "ipc_file_descriptor.h"
#include "shared_buffer.h"

namespace OHOS::Media {
    class Picture;
}
namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class BufferInfo {
public:
    BufferInfo(const std::shared_ptr<SharedBuffer>& sharedBuffer, int32_t dataSize, bool isHighQuality,
        bool isCloudImageEnhanceSupported);
    ~BufferInfo();
    
    sptr<IPCFileDescriptor> GetIPCFileDescriptor();

    inline int32_t GetDataSize()
    {
        return dataSize_;
    }

    inline bool IsHighQuality()
    {
        return isHighQuality_;
    }

    inline bool IsCloudImageEnhanceSupported()
    {
        return isCloudImageEnhanceSupported_;
    }

private:
    std::shared_ptr<SharedBuffer> sharedBuffer_;
    const int32_t dataSize_;
    const bool isHighQuality_;
    bool isCloudImageEnhanceSupported_;
};
class BufferInfoExt {
public:
    explicit BufferInfoExt(std::shared_ptr<Media::Picture> picture, long dataSize, bool isHighQuality,
        bool isCloudImageEnhanceSupported);
    ~BufferInfoExt();
    std::shared_ptr<Media::Picture> GetPicture();
    long GetDataSize();
    bool IsHighQuality();
    bool IsCloudImageEnhanceSupported();

private:
    std::shared_ptr<Media::Picture> picture_;
    long dataSize_;
    bool isHighQuality_;
    bool isCloudImageEnhanceSupported_;
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_DEFERRED_PROCESSING_SERVICE_BUFFER_INFO_H