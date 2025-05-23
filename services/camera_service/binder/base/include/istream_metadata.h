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

#ifndef OHOS_CAMERA_ISTREAM_METADATA_H
#define OHOS_CAMERA_ISTREAM_METADATA_H

#include "istream_metadata_callback.h"
#include "istream_common.h"

namespace OHOS {
namespace CameraStandard {
enum class MetadataObjectType : int32_t {
    INVALID = -1,
    FACE = 0,
    HUMAN_BODY = 1,
    CAT_FACE = 2,
    CAT_BODY = 3,
    DOG_FACE = 4,
    DOG_BODY = 5,
    SALIENT_DETECTION = 6,
    BAR_CODE_DETECTION = 7,
    BASE_FACE_DETECTION = 8,
};
class IStreamMetadata : public IStreamCommon {
public:
    virtual int32_t Start() = 0;

    virtual int32_t Stop() = 0;

    virtual int32_t Release() = 0;

    virtual int32_t SetCallback(sptr<IStreamMetadataCallback>& callback) = 0;

    virtual int32_t UnSetCallback() = 0;

    virtual int32_t EnableMetadataType(std::vector<int32_t> metadataTypes) = 0;

    virtual int32_t DisableMetadataType(std::vector<int32_t> metadataTypes) = 0;

    DECLARE_INTERFACE_DESCRIPTOR(u"IStreamMetadata");
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_ISTREAM_METADATA_H
