/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_HCAMERA_DEVICE_PROXY_H
#define OHOS_CAMERA_HCAMERA_DEVICE_PROXY_H

#include "icamera_device_service.h"
#include "iremote_proxy.h"

namespace OHOS {
namespace CameraStandard {
class HCameraDeviceProxy : public IRemoteProxy<ICameraDeviceService> {
public:
    explicit HCameraDeviceProxy(const sptr<IRemoteObject> &impl);

    virtual ~HCameraDeviceProxy();

    int32_t Open() override;

    int32_t OpenSecureCamera(uint64_t* secureSeqId) override;

    int32_t Open(int32_t concurrentTypeofcamera) override;

    int32_t Close() override;

    int32_t Release() override;

    int32_t closeDelayed() override;

    int32_t SetCallback(sptr<ICameraDeviceServiceCallback> &callback) override;

    int32_t UnSetCallback() override;

    int32_t UpdateSetting(const std::shared_ptr<OHOS::Camera::CameraMetadata> &settings) override;

    int32_t SetUsedAsPosition(uint8_t value) override;

    int32_t GetStatus(std::shared_ptr<OHOS::Camera::CameraMetadata> &metaIn,
                std::shared_ptr<OHOS::Camera::CameraMetadata> &metaOut) override;

    int32_t GetEnabledResults(std::vector<int32_t> &results) override;

    int32_t EnableResult(std::vector<int32_t> &results) override;

    int32_t DisableResult(std::vector<int32_t> &results) override;

    int32_t SetDeviceRetryTime() override;

private:
    static inline BrokerDelegator<HCameraDeviceProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HCAMERA_DEVICE_PROXY_H
