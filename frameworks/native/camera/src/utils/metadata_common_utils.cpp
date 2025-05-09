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

#include "metadata_common_utils.h"
#include "camera_metadata_operator.h"
#include "camera_util.h"
#include <cstdint>
#include <memory>

#include "camera_log.h"
#include "capture_session.h"

namespace OHOS {
namespace CameraStandard {
namespace {
void FillSizeListFromStreamInfo(
    vector<Size>& sizeList, const StreamInfo& streamInfo, const camera_format_t targetFormat)
{
    for (const auto &detail : streamInfo.detailInfos) {
        camera_format_t hdi_format = static_cast<camera_format_t>(detail.format);
        if (hdi_format != targetFormat) {
            continue;
        }
        Size size { .width = detail.width, .height = detail.height };
        sizeList.emplace_back(size);
    }
}

void FillSizeListFromStreamInfo(
    vector<Size>& sizeList, const StreamRelatedInfo& streamInfo, const camera_format_t targetFormat)
{
    for (const auto &detail : streamInfo.detailInfo) {
        camera_format_t hdi_format = static_cast<camera_format_t>(detail.format);
        if (hdi_format != targetFormat) {
            continue;
        }
        Size size{.width = detail.width, .height = detail.height};
        sizeList.emplace_back(size);
    }
}

std::shared_ptr<vector<Size>> GetSupportedPreviewSizeRangeFromProfileLevel(
    const int32_t modeName, camera_format_t targetFormat, const std::shared_ptr<OHOS::Camera::CameraMetadata> metadata)
{
    CHECK_ERROR_RETURN_RET(metadata == nullptr, nullptr);
    camera_metadata_item_t item;
    int32_t retCode = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_AVAILABLE_PROFILE_LEVEL, &item);
    CHECK_ERROR_RETURN_RET(retCode != CAM_META_SUCCESS || item.count == 0, nullptr);
    std::shared_ptr<vector<Size>> sizeList = std::make_shared<vector<Size>>();
    std::vector<SpecInfo> specInfos;
    ProfileLevelInfo modeInfo = {};
    CameraAbilityParseUtil::GetModeInfo(modeName, item, modeInfo);
    specInfos.insert(specInfos.end(), modeInfo.specInfos.begin(), modeInfo.specInfos.end());
    for (SpecInfo& specInfo : specInfos) {
        for (StreamInfo& streamInfo : specInfo.streamInfos) {
            CHECK_EXECUTE(streamInfo.streamType == 0,
                FillSizeListFromStreamInfo(*sizeList.get(), streamInfo, targetFormat));
        }
    }
    MEDIA_INFO_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRangeFromProfileLevel listSize: %{public}d",
        static_cast<int>(sizeList->size()));
    return sizeList;
}

std::shared_ptr<vector<Size>> GetSupportedPreviewSizeRangeFromExtendConfig(
    const int32_t modeName, camera_format_t targetFormat, const std::shared_ptr<OHOS::Camera::CameraMetadata> metadata)
{
    auto item = MetadataCommonUtils::GetCapabilityEntry(metadata, OHOS_ABILITY_STREAM_AVAILABLE_EXTEND_CONFIGURATIONS);
    CHECK_ERROR_RETURN_RET(item == nullptr, nullptr);
    std::shared_ptr<vector<Size>> sizeList = std::make_shared<vector<Size>>();

    ExtendInfo extendInfo = {};
    std::shared_ptr<CameraStreamInfoParse> modeStreamParse = std::make_shared<CameraStreamInfoParse>();
    modeStreamParse->getModeInfo(item->data.i32, item->count, extendInfo); // 解析tag中带的数据信息意义
    for (uint32_t modeIndex = 0; modeIndex < extendInfo.modeCount; modeIndex++) {
        auto modeInfo = std::move(extendInfo.modeInfo[modeIndex]);
        MEDIA_DEBUG_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRangeFromExtendConfig check modeName: %{public}d",
            modeInfo.modeName);
        if (modeName != modeInfo.modeName) {
            continue;
        }
        for (uint32_t streamIndex = 0; streamIndex < modeInfo.streamTypeCount; streamIndex++) {
            auto streamInfo = std::move(modeInfo.streamInfo[streamIndex]);
            int32_t streamType = streamInfo.streamType;
            MEDIA_DEBUG_LOG(
                "MetadataCommonUtils::GetSupportedPreviewSizeRangeFromExtendConfig check streamType: %{public}d",
                streamType);
            if (streamType != 0) { // Stremp type 0 is preview type.
                continue;
            }
            FillSizeListFromStreamInfo(*sizeList.get(), streamInfo, targetFormat);
        }
    }
    MEDIA_INFO_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRangeFromExtendConfig listSize: %{public}d",
        static_cast<int>(sizeList->size()));
    return sizeList;
}

std::shared_ptr<vector<Size>> GetSupportedPreviewSizeRangeFromBasicConfig(
    camera_format_t targetFormat, const std::shared_ptr<OHOS::Camera::CameraMetadata> metadata)
{
    auto item = MetadataCommonUtils::GetCapabilityEntry(metadata, OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS);
    CHECK_ERROR_RETURN_RET(item == nullptr, nullptr);
    std::shared_ptr<vector<Size>> sizeList = std::make_shared<vector<Size>>();

    const uint32_t widthOffset = 1;
    const uint32_t heightOffset = 2;
    const uint32_t unitStep = 3;

    for (uint32_t i = 0; i < item->count; i += unitStep) {
        camera_format_t hdi_format = static_cast<camera_format_t>(item->data.i32[i]);
        if (hdi_format != targetFormat) {
            continue;
        }
        Size size;
        size.width = static_cast<uint32_t>(item->data.i32[i + widthOffset]);
        size.height = static_cast<uint32_t>(item->data.i32[i + heightOffset]);
        sizeList->emplace_back(size);
    }
    MEDIA_INFO_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRangeFromBasicConfig listSize: %{public}d",
        static_cast<int>(sizeList->size()));
    return sizeList;
}
} // namespace

std::shared_ptr<camera_metadata_item_t> MetadataCommonUtils::GetCapabilityEntry(
    const std::shared_ptr<Camera::CameraMetadata> metadata, uint32_t metadataTag)
{
    CHECK_ERROR_RETURN_RET(metadata == nullptr, nullptr);
    std::shared_ptr<camera_metadata_item_t> item = std::make_shared<camera_metadata_item_t>();
    int32_t retCode = Camera::FindCameraMetadataItem(metadata->get(), metadataTag, item.get());
    CHECK_ERROR_RETURN_RET(retCode != CAM_META_SUCCESS || item->count == 0, nullptr);
    return item;
}

std::shared_ptr<vector<Size>> MetadataCommonUtils::GetSupportedPreviewSizeRange(
    const int32_t modeName, camera_format_t targetFormat, const std::shared_ptr<OHOS::Camera::CameraMetadata> metadata)
{
    MEDIA_DEBUG_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRange modeName: %{public}d, targetFormat:%{public}d",
        modeName, targetFormat);
    std::shared_ptr<vector<Size>> sizeList = std::make_shared<vector<Size>>();
    auto levelList = GetSupportedPreviewSizeRangeFromProfileLevel(modeName, targetFormat, metadata);
    if (levelList && levelList->empty() && (modeName == SceneMode::CAPTURE || modeName == SceneMode::VIDEO)) {
        levelList = GetSupportedPreviewSizeRangeFromProfileLevel(SceneMode::NORMAL, targetFormat, metadata);
    }
    if (levelList && !levelList->empty()) {
        for (auto& size : *levelList) {
            MEDIA_DEBUG_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRange level info:%{public}dx%{public}d",
                size.width, size.height);
        }
        sizeList->insert(sizeList->end(), levelList->begin(), levelList->end());
        return sizeList;
    }

    auto extendList = GetSupportedPreviewSizeRangeFromExtendConfig(modeName, targetFormat, metadata);
    if (extendList != nullptr && extendList->empty() &&
        (modeName == SceneMode::CAPTURE || modeName == SceneMode::VIDEO)) {
        extendList = GetSupportedPreviewSizeRangeFromExtendConfig(SceneMode::NORMAL, targetFormat, metadata);
    }
    if (extendList != nullptr && !extendList->empty()) {
        for (auto& size : *extendList) {
            MEDIA_DEBUG_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRange extend info:%{public}dx%{public}d",
                size.width, size.height);
        }
        sizeList->insert(sizeList->end(), extendList->begin(), extendList->end());
        return sizeList;
    }
    auto basicList = GetSupportedPreviewSizeRangeFromBasicConfig(targetFormat, metadata);
    if (basicList != nullptr && !basicList->empty()) {
        for (auto& size : *basicList) {
            MEDIA_DEBUG_LOG("MetadataCommonUtils::GetSupportedPreviewSizeRange basic info:%{public}dx%{public}d",
                size.width, size.height);
        }
        sizeList->insert(sizeList->end(), basicList->begin(), basicList->end());
    }
    return sizeList;
}

std::shared_ptr<OHOS::Camera::CameraMetadata> MetadataCommonUtils::CopyMetadata(
    const std::shared_ptr<OHOS::Camera::CameraMetadata> srcMetadata)
{
    CHECK_ERROR_RETURN_RET_LOG(srcMetadata == nullptr, nullptr, "CopyMetadata fail,src is null");
    auto oldMetadata = srcMetadata->get();
    CHECK_ERROR_RETURN_RET(oldMetadata == nullptr, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> result =
        std::make_shared<OHOS::Camera::CameraMetadata>(oldMetadata->item_capacity, oldMetadata->data_capacity);
    auto newMetadata = result->get();
    int32_t ret = OHOS::Camera::CopyCameraMetadataItems(newMetadata, oldMetadata);
    CHECK_ERROR_PRINT_LOG(ret != CAM_META_SUCCESS, "CopyCameraMetadataItems failed ret:%{public}d", ret);
    return result;
}

std::vector<float> ParsePhysicalApertureRangeByMode(const camera_metadata_item_t &item, const int32_t modeName)
{
    const float factor = 20.0;
    std::vector<float> allRange = {};
    for (uint32_t i = 0; i < item.count; i++) {
        allRange.push_back(item.data.f[i] * factor);
    }
    MEDIA_DEBUG_LOG("ParsePhysicalApertureRangeByMode allRange=%{public}s",
                    Container2String(allRange.begin(), allRange.end()).c_str());
    float npos = -1.0;
    std::vector<std::vector<float>> modeRanges = {};
    std::vector<float> modeRange = {};
    for (uint32_t i = 0; i < item.count - 1; i++) {
        if (item.data.f[i] == npos && item.data.f[i + 1] == npos) {
            modeRange.emplace_back(npos);
            MEDIA_DEBUG_LOG("ParsePhysicalApertureRangeByMode mode %{public}d, modeRange=%{public}s",
                            modeName, Container2String(modeRange.begin(), modeRange.end()).c_str());
            modeRanges.emplace_back(std::move(modeRange));
            modeRange.clear();
            i++;
            continue;
        }
        modeRange.emplace_back(item.data.f[i]);
    }
    float currentMode = static_cast<float>(modeName);
    auto it = std::find_if(modeRanges.begin(), modeRanges.end(),
        [currentMode](auto value) -> bool {
            return currentMode == value[0];
        });
    CHECK_ERROR_RETURN_RET_LOG(it == modeRanges.end(), {},
        "ParsePhysicalApertureRangeByMode Failed meta not support mode:%{public}d", modeName);

    return *it;
}

std::shared_ptr<camera_metadata_item_t> GetMetadataItem(const common_metadata_header_t* src, uint32_t tag)
{
    CHECK_ERROR_RETURN_RET(src == nullptr, nullptr);
    auto item = std::make_shared<camera_metadata_item_t>();
    int32_t ret = OHOS::Camera::CameraMetadata::FindCameraMetadataItem(src, tag, item.get());
    CHECK_ERROR_RETURN_RET(ret != CAM_META_SUCCESS, nullptr);
    return item;
}
} // namespace CameraStandard
} // namespace OHOS
