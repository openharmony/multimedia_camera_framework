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

#ifndef OHOS_CAMERA_CAMERA_INFO_H
#define OHOS_CAMERA_CAMERA_INFO_H

#include <iostream>
#include <refbase.h>
#include <vector>

#include "camera_metadata_info.h"

namespace OHOS {
namespace CameraStandard {
class CameraInfo : public RefBase {
public:
    CameraInfo() = default;
    CameraInfo(std::string cameraID, std::shared_ptr<OHOS::Camera::CameraMetadata> metadata);
    ~CameraInfo();
    /**
    * @brief Get the camera Id.
    *
    * @return Returns the camera Id.
    */
    std::string GetID();

    /**
    * @brief Get the metadata corresponding to current camera object.
    *
    * @return Returns the metadata corresponding to current object.
    */
    std::shared_ptr<OHOS::Camera::CameraMetadata> GetMetadata();

    /**
    * @brief Set the metadata to current camera object.
    *
    * @param Metadat to set.
    */
    void SetMetadata(std::shared_ptr<OHOS::Camera::CameraMetadata> metadata);

    /**
    * @brief Get the position of the camera.
    *
    * @return Returns the position of the camera.
    */
    camera_position_enum_t GetPosition();

    /**
    * @brief Get the Camera type of the camera.
    *
    * @return Returns the Camera type of the camera.
    */
    camera_type_enum_t GetCameraType();

    /**
    * @brief Get the Camera connection type.
    *
    * @return Returns the Camera type of the camera.
    */
    camera_connection_type_t GetConnectionType();

    /**
    * @brief Get the facing for foldScreen device.
    *
    * @return Returns the Camera type of the camera.
    */
    camera_foldscreen_enum_t GetCameraFoldScreenType();

    /**
    * @brief Get the supported Zoom Ratio range.
    *
    * @return Returns vector<float> of supported Zoom ratio range.
    */
    std::vector<float> GetZoomRatioRange();

    /**
    * @brief Get the supported exposure compensation range.
    *
    * @return Returns vector<int32_t> of supported exposure compensation range.
    */
    std::vector<float> GetExposureBiasRange();

private:
    std::string cameraID_;
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata_;
    camera_position_enum_t cameraPosition_ = OHOS_CAMERA_POSITION_OTHER;
    camera_type_enum_t cameraType_ = OHOS_CAMERA_TYPE_UNSPECIFIED;
    camera_connection_type_t connectionType_ = OHOS_CAMERA_CONNECTION_TYPE_BUILTIN;
    camera_foldscreen_enum_t foldScreenType_ = OHOS_CAMERA_FOLDSCREEN_OTHER;
    std::vector<float> zoomRatioRange_;
    std::vector<float> exposureBiasRange_;

    void init(common_metadata_header_t* metadataHeader);
    std::vector<float> CalculateZoomRange();
};
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_CAMERA_INFO_H
