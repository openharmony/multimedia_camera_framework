/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_ISTREAM_METADATA_CALLBACK_H
#define OHOS_CAMERA_ISTREAM_METADATA_CALLBACK_H

#include "iremote_broker.h"
#include "camera_metadata_info.h"

namespace OHOS {
namespace CameraStandard {
class IStreamMetadataCallback : public IRemoteBroker {
public:
    virtual int32_t OnMetadataResult(const int32_t streamId,
        const std::shared_ptr<OHOS::Camera::CameraMetadata> &result) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamMetadataCallback");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_METADATA_CALLBACK_H