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

#include "sketch_wrapper.h"

#include <cstdint>
#include <mutex>

#include "abilities/sketch_ability.h"
#include "camera_device_ability_items.h"
#include "camera_log.h"
#include "camera_util.h"
#include "capture_scene_const.h"
#include "istream_repeat_callback.h"

namespace OHOS {
namespace CameraStandard {
constexpr uint32_t INVALID_MODE = 0xffffffff;
constexpr float INVALID_MODE_FLOAT = -1.0f;
constexpr float INVALID_ZOOM_RATIO = -1.0f;
constexpr float SKETCH_DIV = 100.0f;
std::mutex SketchWrapper::g_sketchReferenceFovRatioMutex_;
std::map<SceneFeaturesMode, std::vector<SketchReferenceFovRange>> SketchWrapper::g_sketchReferenceFovRatioMap_;
std::mutex SketchWrapper::g_sketchEnableRatioMutex_;
std::map<SceneFeaturesMode, float> SketchWrapper::g_sketchEnableRatioMap_;

SketchWrapper::SketchWrapper(wptr<IStreamCommon> hostStream, const Size size, bool isDynamicNotify)
    : hostStream_(hostStream), sketchSize_(size), isDynamicNotify_(isDynamicNotify)
{}

int32_t SketchWrapper::Init(
    std::shared_ptr<OHOS::Camera::CameraMetadata>& deviceMetadata, const SceneFeaturesMode& sceneFeaturesMode)
{
    sptr<IStreamCommon> hostStream = hostStream_.promote();
    CHECK_RETURN_RET(hostStream == nullptr, CAMERA_INVALID_STATE);
    if (!isDynamicNotify_) {
        UpdateSketchStaticInfo(deviceMetadata);
        sketchEnableRatio_ = GetSketchEnableRatio(sceneFeaturesMode);
        SceneFeaturesMode dumpSceneFeaturesMode = sceneFeaturesMode;
        MEDIA_DEBUG_LOG("SketchWrapper::Init sceneFeaturesMode:%{public}s, sketchEnableRatio_:%{public}f",
            dumpSceneFeaturesMode.Dump().c_str(), sketchEnableRatio_);
    }
    IStreamRepeat* repeatStream = static_cast<IStreamRepeat*>(hostStream.GetRefPtr());
    sptr<IRemoteObject> sketchSream = nullptr;
    int32_t ret = repeatStream->ForkSketchStreamRepeat(
        sketchSize_.width, sketchSize_.height, sketchSream, sketchEnableRatio_);
    CHECK_EXECUTE(sketchSream != nullptr, sketchStream_ = iface_cast<IStreamRepeat>(sketchSream));
    return ret;
}

int32_t SketchWrapper::AttachSketchSurface(sptr<Surface> sketchSurface)
{
    CHECK_RETURN_RET(sketchStream_ == nullptr, CAMERA_INVALID_STATE);
    CHECK_RETURN_RET(sketchSurface == nullptr, CAMERA_INVALID_ARG);
    return sketchStream_->AddDeferredSurface(sketchSurface->GetProducer());
}

int32_t SketchWrapper::UpdateSketchRatio(float sketchRatio)
{
    if (isDynamicNotify_) {
        // Ignore update sketch ratio
        return CAMERA_OK;
    }
    // SketchRatio value could be negative value
    MEDIA_DEBUG_LOG("Enter Into SketchWrapper::UpdateSketchRatio");
    CHECK_RETURN_RET(sketchStream_ == nullptr, CAMERA_INVALID_STATE);
    sptr<IStreamCommon> hostStream = hostStream_.promote();
    CHECK_RETURN_RET_ELOG(hostStream == nullptr, CAMERA_INVALID_STATE,
        "SketchWrapper::UpdateSketchRatio hostStream is null");
    IStreamRepeat* repeatStream = static_cast<IStreamRepeat*>(hostStream.GetRefPtr());
    int32_t ret = repeatStream->UpdateSketchRatio(sketchRatio);
    if (ret == CAMERA_OK) {
        sketchEnableRatio_ = sketchRatio;
    }
    AutoStream();
    return ret;
}

int32_t SketchWrapper::Destroy()
{
    if (sketchStream_ != nullptr) {
        sketchStream_->Stop();
        sketchStream_->Release();
        sketchStream_ = nullptr;
    }
    sptr<IStreamCommon> hostStream = hostStream_.promote();
    CHECK_RETURN_RET(hostStream == nullptr, CAMERA_INVALID_STATE);
    IStreamRepeat* repeatStream = static_cast<IStreamRepeat*>(hostStream.GetRefPtr());
    return repeatStream->RemoveSketchStreamRepeat();
}

SketchWrapper::~SketchWrapper()
{
    Destroy();
}

int32_t SketchWrapper::StartSketchStream()
{
    if (sketchStream_ != nullptr) {
        MEDIA_DEBUG_LOG("Enter Into SketchWrapper::SketchWrapper::StartSketchStream");
        int retCode = sketchStream_->Start();
        return retCode;
    }
    return CAMERA_UNKNOWN_ERROR;
}

int32_t SketchWrapper::StopSketchStream()
{
    if (sketchStream_ != nullptr) {
        MEDIA_DEBUG_LOG("Enter Into SketchWrapper::SketchWrapper::StopSketchStream");
        int retCode = sketchStream_->Stop();
        return retCode;
    }
    return CAMERA_UNKNOWN_ERROR;
}

void SketchWrapper::SetPreviewOutputCallbackManager(wptr<PreviewOutputListenerManager> previewOutputCallbackManager)
{
    previewOutputCallbackManager_ = previewOutputCallbackManager;
}

void SketchWrapper::OnSketchStatusChanged(const SceneFeaturesMode& sceneFeaturesMode)
{
    auto manager = previewOutputCallbackManager_.promote();
    CHECK_RETURN(manager == nullptr);
    // LCOV_EXCL_START
    float sketchReferenceRatio = -1.0f;
    if (isDynamicNotify_) {
        sketchReferenceRatio = sketchDynamicReferenceRatio_;
    } else {
        sketchReferenceRatio = GetSketchReferenceFovRatio(sceneFeaturesMode, currentZoomRatio_);
    }
    std::lock_guard<std::mutex> lock(sketchStatusChangeMutex_);
    if (currentSketchStatusData_.sketchRatio != sketchReferenceRatio ||
        currentSketchStatusData_.offsetx != offset_.x ||
        currentSketchStatusData_.offsety != offset_.y) {
        SketchStatusData statusData;
        statusData.status = currentSketchStatusData_.status;
        statusData.sketchRatio = sketchReferenceRatio;
        statusData.offsetx = offset_.x;
        statusData.offsety = offset_.y;
        MEDIA_DEBUG_LOG("SketchWrapper::OnSketchStatusDataChanged-:%{public}d->%{public}d,%{public}f->%{public}f"
                        ", %{public}f->%{public}f, %{public}f->%{public}f",
            currentSketchStatusData_.status, statusData.status, currentSketchStatusData_.sketchRatio,
            statusData.sketchRatio, currentSketchStatusData_.offsetx, statusData.offsetx,
            currentSketchStatusData_.offsety, statusData.offsety);
        currentSketchStatusData_ = statusData;
        manager->TriggerListener([&](PreviewStateCallback* previewStateCallback) {
            previewStateCallback->OnSketchStatusDataChanged(currentSketchStatusData_);
        });
    }
    // LCOV_EXCL_STOP
}

void SketchWrapper::OnSketchStatusChanged(SketchStatus sketchStatus, const SceneFeaturesMode& sceneFeaturesMode)
{
    auto manager = previewOutputCallbackManager_.promote();
    CHECK_RETURN(manager == nullptr);
    // LCOV_EXCL_START
    float sketchReferenceRatio = -1.0f;
    if (isDynamicNotify_) {
        sketchReferenceRatio = sketchDynamicReferenceRatio_;
    } else {
        sketchReferenceRatio = GetSketchReferenceFovRatio(sceneFeaturesMode, currentZoomRatio_);
    }
    std::lock_guard<std::mutex> lock(sketchStatusChangeMutex_);
    if (currentSketchStatusData_.status != sketchStatus ||
        currentSketchStatusData_.sketchRatio != sketchReferenceRatio) {
        SketchStatusData statusData;
        statusData.status = sketchStatus;
        statusData.sketchRatio = sketchReferenceRatio;
        MEDIA_DEBUG_LOG("SketchWrapper::OnSketchStatusDataChanged:%{public}d->%{public}d,%{public}f->%{public}f",
            currentSketchStatusData_.status, statusData.status, currentSketchStatusData_.sketchRatio,
            statusData.sketchRatio);
        currentSketchStatusData_ = statusData;
        manager->TriggerListener([&](PreviewStateCallback* previewStateCallback) {
            previewStateCallback->OnSketchStatusDataChanged(currentSketchStatusData_);
        });
    }
    // LCOV_EXCL_STOP
}

void SketchWrapper::UpdateSketchStaticInfo(std::shared_ptr<Camera::CameraMetadata> deviceMetadata)
{
    {
        std::lock_guard<std::mutex> lock(g_sketchEnableRatioMutex_);
        g_sketchEnableRatioMap_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(g_sketchReferenceFovRatioMutex_);
        g_sketchReferenceFovRatioMap_.clear();
    }
    UpdateSketchEnableRatio(deviceMetadata);
    UpdateSketchReferenceFovRatio(deviceMetadata);
    UpdateSketchConfigFromMoonCaptureBoostConfig(deviceMetadata);
}

SceneFeaturesMode SketchWrapper::GetSceneFeaturesModeFromModeData(float modeFloatData)
{
    SceneFeaturesMode currentSceneFeaturesMode {};
    auto sceneMode = static_cast<SceneMode>(round(modeFloatData));
    currentSceneFeaturesMode.SetSceneMode(sceneMode);
    return currentSceneFeaturesMode;
}

void SketchWrapper::UpdateSketchEnableRatio(std::shared_ptr<Camera::CameraMetadata>& deviceMetadata)
{
    CHECK_RETURN_ELOG(deviceMetadata == nullptr, "SketchWrapper::UpdateSketchEnableRatio deviceMetadata is null");
    camera_metadata_item_t item;
    int ret = OHOS::Camera::FindCameraMetadataItem(deviceMetadata->get(), OHOS_ABILITY_SKETCH_ENABLE_RATIO, &item);
    CHECK_RETURN_ELOG(ret != CAM_META_SUCCESS,
        "SketchWrapper::UpdateSketchEnableRatio get sketch enable ratio failed");
    // LCOV_EXCL_START
    CHECK_RETURN_ELOG(item.count <= 0, "SketchWrapper::UpdateSketchEnableRatio sketch enable ratio count <= 0");

    constexpr int32_t dataGroupSize = 2; // 2 data as a group: key,value
    constexpr int32_t valueOffset = 1;   // 1 is value offset
    uint32_t groupCount = item.count / dataGroupSize;
    float* dataEntry = item.data.f;
    for (uint32_t i = 0; i < groupCount; i++) {
        float key = dataEntry[i * dataGroupSize];
        float value = dataEntry[i * dataGroupSize + valueOffset];
        MEDIA_DEBUG_LOG("SketchWrapper::UpdateSketchEnableRatio get sketch enable ratio "
                        "%{public}f:%{public}f",
            key, value);
        auto sceneFeaturesMode = GetSceneFeaturesModeFromModeData(key);
        InsertSketchEnableRatioMapValue(sceneFeaturesMode, value);
    }
    // LCOV_EXCL_STOP
}

void SketchWrapper::InsertSketchReferenceFovRatioMapValue(
    SceneFeaturesMode& sceneFeaturesMode, SketchReferenceFovRange& sketchReferenceFovRange)
{
    std::lock_guard<std::mutex> lock(g_sketchReferenceFovRatioMutex_);
    auto it = g_sketchReferenceFovRatioMap_.find(sceneFeaturesMode);
    std::vector<SketchReferenceFovRange> rangeFov;
    // LCOV_EXCL_START
    if (it != g_sketchReferenceFovRatioMap_.end()) {
        rangeFov = std::move(it->second);
    }
    rangeFov.emplace_back(sketchReferenceFovRange);
    if (sceneFeaturesMode.GetSceneMode() == CAPTURE && sceneFeaturesMode.GetFeatures().empty()) {
        g_sketchReferenceFovRatioMap_[{ CAPTURE, { FEATURE_TRIPOD_DETECTION } }] = rangeFov;
    }
    g_sketchReferenceFovRatioMap_[sceneFeaturesMode] = rangeFov;
    if (sceneFeaturesMode.GetSceneMode() == CAPTURE_MACRO) {
        g_sketchReferenceFovRatioMap_[{ CAPTURE, { FEATURE_MACRO } }] = rangeFov;
        g_sketchReferenceFovRatioMap_[{ CAPTURE_MACRO, { FEATURE_MACRO } }] = rangeFov;
    } else if (sceneFeaturesMode.GetSceneMode() == VIDEO_MACRO) {
        g_sketchReferenceFovRatioMap_[{ VIDEO, { FEATURE_MACRO } }] = rangeFov;
        g_sketchReferenceFovRatioMap_[{ VIDEO_MACRO, { FEATURE_MACRO } }] = rangeFov;
    }
    // LCOV_EXCL_STOP
}

void SketchWrapper::InsertSketchEnableRatioMapValue(SceneFeaturesMode& sceneFeaturesMode, float ratioValue)
{
    MEDIA_DEBUG_LOG("SketchWrapper::InsertSketchEnableRatioMapValue %{public}s : %{public}f",
        sceneFeaturesMode.Dump().c_str(), ratioValue);
    std::lock_guard<std::mutex> lock(g_sketchEnableRatioMutex_);
    // LCOV_EXCL_START
    if (sceneFeaturesMode.GetSceneMode() == CAPTURE && sceneFeaturesMode.GetFeatures().empty()) {
        g_sketchEnableRatioMap_[{ CAPTURE, { FEATURE_TRIPOD_DETECTION } }] = ratioValue;
    }
    g_sketchEnableRatioMap_[sceneFeaturesMode] = ratioValue;
    if (sceneFeaturesMode.GetSceneMode() == CAPTURE_MACRO) {
        g_sketchEnableRatioMap_[{ CAPTURE, { FEATURE_MACRO } }] = ratioValue;
        g_sketchEnableRatioMap_[{ CAPTURE_MACRO, { FEATURE_MACRO } }] = ratioValue;
    } else if (sceneFeaturesMode.GetSceneMode() == VIDEO_MACRO) {
        g_sketchEnableRatioMap_[{ VIDEO, { FEATURE_MACRO } }] = ratioValue;
        g_sketchEnableRatioMap_[{ VIDEO_MACRO, { FEATURE_MACRO } }] = ratioValue;
    }
    // LCOV_EXCL_STOP
}

void SketchWrapper::UpdateSketchReferenceFovRatio(std::shared_ptr<Camera::CameraMetadata>& deviceMetadata)
{
    CHECK_RETURN_ELOG(deviceMetadata == nullptr,
        "SketchWrapper::UpdateSketchReferenceFovRatio deviceMetadata is null");
    camera_metadata_item_t item;
    int ret =
        OHOS::Camera::FindCameraMetadataItem(deviceMetadata->get(), OHOS_ABILITY_SKETCH_REFERENCE_FOV_RATIO, &item);
    CHECK_RETURN_ELOG(ret != CAM_META_SUCCESS || item.count <= 0,
        "SketchWrapper::UpdateSketchReferenceFovRatio get sketch reference fov ratio failed");
    UpdateSketchReferenceFovRatio(item);
}

void SketchWrapper::UpdateSketchReferenceFovRatio(camera_metadata_item_t& metadataItem)
{
    uint32_t dataCount = metadataItem.count;
    float currentMode = INVALID_MODE_FLOAT;
    float currentMinRatio = INVALID_ZOOM_RATIO;
    float currentMaxRatio = INVALID_ZOOM_RATIO;
    SceneFeaturesMode currentSceneFeaturesMode;
    // LCOV_EXCL_START
    for (uint32_t i = 0; i < dataCount; i++) {
        if (currentMode == INVALID_MODE_FLOAT) {
            currentMode = metadataItem.data.f[i];
            currentSceneFeaturesMode = GetSceneFeaturesModeFromModeData(metadataItem.data.f[i]);
            continue;
        }
        if (currentMinRatio == INVALID_ZOOM_RATIO) {
            currentMinRatio = metadataItem.data.f[i];
            continue;
        }
        if (currentMaxRatio == INVALID_ZOOM_RATIO) {
            currentMaxRatio = metadataItem.data.f[i];
            continue;
        }
        SketchReferenceFovRange fovRange;
        fovRange.zoomMin = metadataItem.data.f[i];
        fovRange.zoomMax = metadataItem.data.f[i + 1];        // Offset 1 data
        fovRange.referenceValue = metadataItem.data.f[i + 2]; // Offset 2 data
        i = i + 2;                                            // Offset 2 data
        InsertSketchReferenceFovRatioMapValue(currentSceneFeaturesMode, fovRange);
        MEDIA_DEBUG_LOG("SketchWrapper::UpdateSketchReferenceFovRatio get sketch reference fov ratio:mode->%{public}f "
                        "%{public}f-%{public}f value->%{public}f",
            currentMode, fovRange.zoomMin, fovRange.zoomMax, fovRange.referenceValue);
        if (fovRange.zoomMax - currentMaxRatio >= -std::numeric_limits<float>::epsilon()) {
            currentMode = INVALID_MODE_FLOAT;
            currentMinRatio = INVALID_ZOOM_RATIO;
            currentMaxRatio = INVALID_ZOOM_RATIO;
        }
    }
    // LCOV_EXCL_STOP
}

bool SketchWrapper::GetMoonCaptureBoostAbilityItem(
    std::shared_ptr<Camera::CameraMetadata>& deviceMetadata, camera_metadata_item_t& metadataItem)
{
    CHECK_RETURN_RET_ELOG(deviceMetadata == nullptr, false,
        "SketchWrapper::GetMoonCaptureBoostAbilityItem deviceMetadata is null");
    int ret =
        OHOS::Camera::FindCameraMetadataItem(deviceMetadata->get(), OHOS_ABILITY_MOON_CAPTURE_BOOST, &metadataItem);
    CHECK_RETURN_RET_ELOG(ret != CAM_META_SUCCESS || metadataItem.count <= 0, false,
        "SketchWrapper::GetMoonCaptureBoostAbilityItem get OHOS_ABILITY_MOON_CAPTURE_BOOST failed");
    return true;
}

void SketchWrapper::UpdateSketchConfigFromMoonCaptureBoostConfig(
    std::shared_ptr<Camera::CameraMetadata>& deviceMetadata)
{
    camera_metadata_item_t item;
    CHECK_RETURN_ELOG(!GetMoonCaptureBoostAbilityItem(deviceMetadata, item),
        "SketchWrapper::UpdateSketchConfigFromMoonCaptureBoostConfig get GetMoonCaptureBoostAbilityItem failed");
    // LCOV_EXCL_START
    uint32_t currentMode = INVALID_MODE;
    float currentMinRatio = INVALID_ZOOM_RATIO;
    float currentMaxRatio = INVALID_ZOOM_RATIO;
    SceneFeaturesMode currentSceneMode;
    for (uint32_t i = 0; i < item.count; i++) {
        if (currentMode == INVALID_MODE) {
            currentMode = static_cast<SceneMode>(item.data.ui32[i]);
            currentSceneMode =
                SceneFeaturesMode(static_cast<SceneMode>(currentMode), { FEATURE_MOON_CAPTURE_BOOST });
            continue;
        }
        if (currentMinRatio == INVALID_ZOOM_RATIO) {
            currentMinRatio = static_cast<float>(item.data.ui32[i]) / SKETCH_DIV;
            continue;
        }
        if (currentMaxRatio == INVALID_ZOOM_RATIO) {
            currentMaxRatio = static_cast<float>(item.data.ui32[i]) / SKETCH_DIV;
            continue;
        }
        SketchReferenceFovRange fovRange;
        fovRange.zoomMin = static_cast<float>(item.data.ui32[i]) / SKETCH_DIV;
        fovRange.zoomMax = static_cast<float>(item.data.ui32[i + 1]) / SKETCH_DIV;        // Offset 1 data
        fovRange.referenceValue = static_cast<float>(item.data.ui32[i + 2]) / SKETCH_DIV; // Offset 2 data
        i = i + 2;                                                                        // Offset 2 data
        InsertSketchReferenceFovRatioMapValue(currentSceneMode, fovRange);
        MEDIA_DEBUG_LOG("SketchWrapper::UpdateSketchConfigFromMoonCaptureBoostConfig get sketch reference fov "
                        "ratio:mode->%{public}d %{public}f-%{public}f value->%{public}f",
            currentMode, fovRange.zoomMin, fovRange.zoomMax, fovRange.referenceValue);
        if (fovRange.zoomMax - currentMaxRatio >= -std::numeric_limits<float>::epsilon()) {
            InsertSketchEnableRatioMapValue(currentSceneMode, currentMinRatio);
            currentMode = INVALID_MODE;
            currentMinRatio = INVALID_ZOOM_RATIO;
            currentMaxRatio = INVALID_ZOOM_RATIO;
        }
    }
    // LCOV_EXCL_STOP
}

float SketchWrapper::GetSketchReferenceFovRatio(const SceneFeaturesMode& sceneFeaturesMode, float zoomRatio)
{
    float currentZoomRatio = zoomRatio;
    SceneFeaturesMode filteredFeaturesMode = sceneFeaturesMode;
    for (int32_t i = SceneFeature::FEATURE_ENUM_MIN; i < SceneFeature::FEATURE_ENUM_MAX; i++) {
        if (i == SceneFeature::FEATURE_MACRO || i == SceneFeature::FEATURE_MOON_CAPTURE_BOOST) {
            continue;
        }
        filteredFeaturesMode.SwitchFeature(static_cast<SceneFeature>(i), false);
    }
    std::lock_guard<std::mutex> lock(g_sketchReferenceFovRatioMutex_);
    auto it = g_sketchReferenceFovRatioMap_.find(filteredFeaturesMode);
    // LCOV_EXCL_START
    if (it != g_sketchReferenceFovRatioMap_.end()) {
        // only 1 element, just return result;
        CHECK_RETURN_RET(it->second.size() == 1, it->second[0].referenceValue);
        // If zoom ratio out of range, try return min or max range value.
        const auto& minRange = it->second.front();
        CHECK_RETURN_RET(currentZoomRatio - minRange.zoomMin <= std::numeric_limits<float>::epsilon(),
            minRange.referenceValue);
        const auto& maxRange = it->second.back();
        CHECK_RETURN_RET(currentZoomRatio - maxRange.zoomMax >= -std::numeric_limits<float>::epsilon(),
            maxRange.referenceValue);
        auto itRange = std::find_if(it->second.begin(), it->second.end(), [currentZoomRatio](const auto& range) {
            return currentZoomRatio - range.zoomMin >= -std::numeric_limits<float>::epsilon() &&
                   currentZoomRatio - range.zoomMax < -std::numeric_limits<float>::epsilon();
        });
        CHECK_RETURN_RET(itRange != it->second.end(), itRange->referenceValue);
    }
    // LCOV_EXCL_STOP
    return INVALID_ZOOM_RATIO;
}

float SketchWrapper::GetSketchEnableRatio(const SceneFeaturesMode& sceneFeaturesMode)
{
    SceneFeaturesMode filteredFeaturesMode = sceneFeaturesMode;
    for (int32_t i = SceneFeature::FEATURE_ENUM_MIN; i < SceneFeature::FEATURE_ENUM_MAX; i++) {
        if (i == SceneFeature::FEATURE_MACRO || i == SceneFeature::FEATURE_MOON_CAPTURE_BOOST) {
            continue;
        }
        filteredFeaturesMode.SwitchFeature(static_cast<SceneFeature>(i), false);
    }
    std::lock_guard<std::mutex> lock(g_sketchEnableRatioMutex_);
    auto it = g_sketchEnableRatioMap_.find(filteredFeaturesMode);
    CHECK_RETURN_RET(it != g_sketchEnableRatioMap_.end(), it->second);
    return INVALID_ZOOM_RATIO;
}

void SketchWrapper::UpdateZoomRatio(float zoomRatio)
{
    currentZoomRatio_ = zoomRatio;
    AutoStream();
}

void SketchWrapper::AutoStream()
{
    if (currentZoomRatio_ > 0 && sketchEnableRatio_ > 0 &&
        currentZoomRatio_ - sketchEnableRatio_ >= -std::numeric_limits<float>::epsilon()) {
        StartSketchStream();
    } else {
        StopSketchStream();
    }
}

int32_t SketchWrapper::OnMetadataDispatch(const SceneFeaturesMode& sceneFeaturesMode,
    const camera_device_metadata_tag_t tag, const camera_metadata_item_t& metadataItem)
{
    // LCOV_EXCL_START
    if (isDynamicNotify_) {
        if (tag == OHOS_STATUS_SKETCH_STREAM_INFO) {
            MEDIA_DEBUG_LOG("SketchWrapper::OnMetadataDispatch get tag:OHOS_STATUS_SKETCH_STREAM_INFO");
            return OnMetadataChangedSketchDynamicNotify(metadataItem, sceneFeaturesMode);
        } else {
            MEDIA_DEBUG_LOG("SketchWrapper::OnMetadataDispatch isDynamicNotify_ get unhandled tag:%{public}d",
                static_cast<int32_t>(tag));
        }
    } else {
        if (tag == OHOS_CONTROL_ZOOM_RATIO) {
            MEDIA_DEBUG_LOG("SketchWrapper::OnMetadataDispatch get tag:OHOS_CONTROL_ZOOM_RATIO");
            return OnMetadataChangedZoomRatio(sceneFeaturesMode, tag, metadataItem);
        } else if (tag == OHOS_CONTROL_SMOOTH_ZOOM_RATIOS) {
            MEDIA_DEBUG_LOG("SketchWrapper::OnMetadataDispatch get tag:OHOS_CONTROL_SMOOTH_ZOOM_RATIOS");
            return OnMetadataChangedZoomRatio(sceneFeaturesMode, tag, metadataItem);
        } else if (tag == OHOS_CONTROL_CAMERA_MACRO) {
            return OnMetadataChangedMacro(sceneFeaturesMode, tag, metadataItem);
        } else if (tag == OHOS_CONTROL_MOON_CAPTURE_BOOST) {
            return OnMetadataChangedMoonCaptureBoost(sceneFeaturesMode, tag, metadataItem);
        } else {
            MEDIA_DEBUG_LOG(
                "SketchWrapper::OnMetadataDispatch get unhandled tag:%{public}d", static_cast<int32_t>(tag));
        }
    }
    return CAM_META_SUCCESS;
    // LCOV_EXCL_STOP
}

int32_t SketchWrapper::OnMetadataChangedSketchDynamicNotify(
    const camera_metadata_item_t& metadataItem, const SceneFeaturesMode& sceneFeaturesMode)
{
    // LCOV_EXCL_START
    uint32_t dataLen = metadataItem.count;
    if (dataLen != 4) { // data size is 4 ==> [isNeedStartSketch, sketchZoomRatio, sketchPoint_x, sketchPoint_y]
        MEDIA_INFO_LOG("SketchWrapper::OnMetadataChangedSketchDynamicNotify recv error data, size:%{public}d", dataLen);
        return CAM_META_SUCCESS;
    }
    auto infoStatus = static_cast<SketchStreamInfoStatus>(metadataItem.data.f[0]);
    sketchDynamicReferenceRatio_ = metadataItem.data.f[1];
    offset_.x = metadataItem.data.f[2]; // 2 ==> sketchPoint_x
    offset_.y = metadataItem.data.f[3]; // 3 ==> sketchPoint_y
    MEDIA_DEBUG_LOG("SketchWrapper::OnMetadataChangedSketchDynamicNotify infoStatus:%{public}d ratio:%{public}f"
                    "offsetx:%{public}f offsety:%{public}f",
        infoStatus, sketchDynamicReferenceRatio_, offset_.x, offset_.y);
    if (infoStatus == OHOS_CAMERA_SKETCH_STREAM_SUPPORT) {
        StartSketchStream();
    } else if (infoStatus == OHOS_CAMERA_SKETCH_STREAM_UNSUPPORT) {
        StopSketchStream();
    }
    OnSketchStatusChanged(sceneFeaturesMode);
    return CAM_META_SUCCESS;
    // LCOV_EXCL_STOP
}

int32_t SketchWrapper::OnMetadataChangedZoomRatio(const SceneFeaturesMode& sceneFeaturesMode,
    const camera_device_metadata_tag_t tag, const camera_metadata_item_t& metadataItem)
{
    // LCOV_EXCL_START
    float tagRatio = *metadataItem.data.f;
    MEDIA_DEBUG_LOG("SketchWrapper::OnMetadataChangedZoomRatio OHOS_CONTROL_ZOOM_RATIO >>> tagRatio:%{public}f -- "
                    "sketchRatio:%{public}f",
        tagRatio, sketchEnableRatio_);
    UpdateZoomRatio(tagRatio);
    OnSketchStatusChanged(sceneFeaturesMode);
    return CAM_META_SUCCESS;
    // LCOV_EXCL_STOP
}

int32_t SketchWrapper::OnMetadataChangedMacro(const SceneFeaturesMode& sceneFeaturesMode,
    const camera_device_metadata_tag_t tag, const camera_metadata_item_t& metadataItem)
{
    float sketchRatio = GetSketchEnableRatio(sceneFeaturesMode);
    float currentZoomRatio = currentZoomRatio_;
    MEDIA_DEBUG_LOG("SketchWrapper::OnMetadataChangedMacro OHOS_CONTROL_ZOOM_RATIO >>> currentZoomRatio:%{public}f -- "
                    "sketchRatio:%{public}f",
        currentZoomRatio, sketchRatio);
    UpdateSketchRatio(sketchRatio);
    OnSketchStatusChanged(sceneFeaturesMode);
    return CAM_META_SUCCESS;
}

int32_t SketchWrapper::OnMetadataChangedMoonCaptureBoost(const SceneFeaturesMode& sceneFeaturesMode,
    const camera_device_metadata_tag_t tag, const camera_metadata_item_t& metadataItem)
{
    // LCOV_EXCL_START
    float sketchRatio = GetSketchEnableRatio(sceneFeaturesMode);
    float currentZoomRatio = currentZoomRatio_;
    MEDIA_DEBUG_LOG(
        "SketchWrapper::OnMetadataChangedMoonCaptureBoost OHOS_CONTROL_ZOOM_RATIO >>> currentZoomRatio:%{public}f -- "
        "sketchRatio:%{public}f",
        currentZoomRatio, sketchRatio);
    UpdateSketchRatio(sketchRatio);
    OnSketchStatusChanged(sceneFeaturesMode);
    return CAM_META_SUCCESS;
    // LCOV_EXCL_STOP
}
} // namespace CameraStandard
} // namespace OHOS