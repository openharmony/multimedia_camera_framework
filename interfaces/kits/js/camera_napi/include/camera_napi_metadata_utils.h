/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef CAMERA_NAPI_METADATA_UTILS_H
#define CAMERA_NAPI_METADATA_UTILS_H

#include <cstdint>

#include "istream_metadata.h"
#include "camera_types.h"

namespace OHOS {
namespace CameraStandard {
namespace CameraNapiMetadataUtils {
int32_t MapMetadataObjSupportedTypesEnum(MetadataObjectType nativeMetadataObjType);
MetadataObjectType MapMetadataObjSupportedTypesEnumFromJS(int32_t jsMetadataObjType);
} // namespace CameraNapiMetadataUtils
} // namespace CameraStandard
} // namespace OHOS
#endif /* CAMERA_NAPI_METADATA_UTILS_H */
