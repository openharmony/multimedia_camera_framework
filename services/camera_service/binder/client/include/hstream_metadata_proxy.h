/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_HSTREAM_METADATA_PROXY_H
#define OHOS_CAMERA_HSTREAM_METADATA_PROXY_H

#include "iremote_proxy.h"
#include "istream_metadata.h"
#include "camera_service_ipc_interface_code.h"

namespace OHOS {
namespace CameraStandard {
class HStreamMetadataProxy : public IRemoteProxy<IStreamMetadata> {
public:
    explicit HStreamMetadataProxy(const sptr<IRemoteObject> &impl);
    virtual ~HStreamMetadataProxy() = default;

    int32_t Start() override;

    int32_t Stop() override;

    int32_t Release() override;

    int32_t SetCallback(sptr<IStreamMetadataCallback>& callback) override;
    
    int32_t UnSetCallback() override;
    
    int32_t EnableMetadataType(std::vector<int32_t> metadataTypes) override;

    int32_t DisableMetadataType(std::vector<int32_t> metadataTypes) override;
private:
    static inline BrokerDelegator<HStreamMetadataProxy> delegator_;
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_HSTREAM_METADATA_PROXY_H
