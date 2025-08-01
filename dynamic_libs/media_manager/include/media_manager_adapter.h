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
#ifndef OHOS_CAMERA_DPS_MEDIA_MANAGER_ADAPTER_H
#define OHOS_CAMERA_DPS_MEDIA_MANAGER_ADAPTER_H

#include "media_manager_interface.h"
#include "mpeg_manager_factory.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class MediaManagerAdapter : public MediaManagerIntf {
public:
    MediaManagerAdapter();
    ~MediaManagerAdapter() override;
    int32_t MpegAcquire(const std::string& requestId, const sptr<IPCFileDescriptor>& inputFd) override;
    int32_t MpegUnInit(const int32_t result) override;
    sptr<IPCFileDescriptor> MpegGetResultFd() override;
    void MpegAddUserMeta(std::unique_ptr<MediaUserInfo> userInfo) override;
    uint64_t MpegGetProcessTimeStamp() override;
    sptr<Surface> MpegGetSurface() override;
    sptr<Surface> MpegGetMakerSurface() override;
    void MpegSetMarkSize(int32_t size) override;
    int32_t MpegRelease() override;
private:
    std::shared_ptr<MpegManager> mpegManager_ = {nullptr};
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_MEDIA_MANAGER_ADAPTER_H