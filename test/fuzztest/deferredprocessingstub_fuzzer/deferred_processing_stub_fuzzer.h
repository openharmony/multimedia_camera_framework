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

#ifndef DEFERREDPROCESSING_FUZZER_H
#define DEFERREDPROCESSING_FUZZER_H
#define FUZZ_PROJECT_NAME "deferredprocessing_fuzzer"
#include <iostream>
#include "deferred_photo_processing_session.h"
#include "deferred_photo_processor.h"
#include "ideferred_photo_processing_session_callback.h"
#include "deferred_photo_processing_session_callback_stub.h"
#include "deferred_video_processing_session.h"
#include "ideferred_video_processing_session_callback.h"
#include "deferred_video_processing_session_callback_stub.h"
namespace OHOS {
namespace CameraStandard {
using namespace OHOS::CameraStandard::DeferredProcessing;
void DeferredProcessingFuzzTestGetPermission();
void DeferredProcessingPhotoFuzzTest(uint8_t *rawData, size_t size);
void DeferredProcessingVideoFuzzTest(uint8_t *rawData, size_t size);

class IDeferredPhotoProcessingSessionCallbackFuzz : public DeferredPhotoProcessingSessionCallbackStub {
public:
    explicit IDeferredPhotoProcessingSessionCallbackFuzz() = default;
    virtual ~IDeferredPhotoProcessingSessionCallbackFuzz() = default;
    inline int32_t OnProcessImageDone(const std::string &imageId,
        sptr<IPCFileDescriptor> ipcFd, const long bytes, uint32_t cloudImageEnhanceFlag) override
    {
        return 0;
    }
    inline int32_t OnProcessImageDone(const std::string &imageId, std::shared_ptr<PictureIntf> picture,
        uint32_t cloudImageEnhanceFlag) override
    {
        return 0;
    }
    inline int32_t OnDeliveryLowQualityImage(const std::string &imageId,
        std::shared_ptr<PictureIntf> picture) override
    {
        return 0;
    }
    inline int32_t OnError(const std::string &imageId, const ErrorCode errorCode) override
    {
        return 0;
    }
    inline int32_t OnStateChanged(const StatusCode status) override
    {
        return 0;
    }
};

class IDeferredVideoProcessingSessionCallbackFuzz : public DeferredVideoProcessingSessionCallbackStub {
public:
    explicit IDeferredVideoProcessingSessionCallbackFuzz() = default;
    virtual ~IDeferredVideoProcessingSessionCallbackFuzz() = default;
    inline ErrCode OnProcessVideoDone(const std::string& videoId, const sptr<IPCFileDescriptor>& fd) override
    {
        return 0;
    }
    inline ErrCode OnError(const std::string& videoId, int32_t errorCode) override
    {
        return 0;
    }
    inline ErrCode OnStateChanged(int32_t status) override
    {
        return 0;
    }
};

void TestBufferInfo(uint8_t *rawData, size_t size);
} //CameraStandard
} //OHOS
#endif //DEFERREDPROCESSING_FUZZER_H

