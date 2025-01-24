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

#include "hstream_capture.h"
#include <cstdint>
#include <memory>
#include <mutex>
#include <uuid.h>

#include "camera_log.h"
#include "camera_server_photo_proxy.h"
#include "camera_service_ipc_interface_code.h"
#include "camera_util.h"
#include "hstream_common.h"
#include "ipc_skeleton.h"
#include "metadata_utils.h"
#include "camera_report_uitls.h"
#include "media_library/photo_asset_interface.h"
#include "media_library/photo_asset_proxy.h"
#include "camera_report_dfx_uitls.h"
#include "bms_adapter.h"

namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Camera::V1_0;
static const int32_t CAPTURE_ROTATE_360 = 360;
static const std::string BURST_UUID_BEGIN = "";
static std::string g_currentBurstUuid = BURST_UUID_BEGIN;

static std::string GenerateBurstUuid()
{
    MEDIA_INFO_LOG("HStreamCapture::GenerateBurstUuid");
    uuid_t uuid;
    char str[37] = {}; // UUIDs are 36 characters plus the null terminator
    uuid_generate(uuid);
    uuid_unparse(uuid, str); // Convert the UUID to a string
    std::string burstUuid(str);
    g_currentBurstUuid = burstUuid;
    return burstUuid;
}

HStreamCapture::HStreamCapture(sptr<OHOS::IBufferProducer> producer, int32_t format, int32_t width, int32_t height)
    : HStreamCommon(StreamType::CAPTURE, producer, format, width, height)
{
    MEDIA_INFO_LOG(
        "HStreamCapture::HStreamCapture construct, format:%{public}d size:%{public}dx%{public}d streamId:%{public}d",
        format, width, height, GetFwkStreamId());
    thumbnailSwitch_ = 0;
    rawDeliverySwitch_ = 0;
    modeName_ = 0;
    deferredPhotoSwitch_ = 0;
    deferredVideoSwitch_ = 0;
    burstNum_ = 0;
}

HStreamCapture::~HStreamCapture()
{
    rotationMap_.Clear();
    MEDIA_INFO_LOG(
        "HStreamCapture::~HStreamCapture deconstruct, format:%{public}d size:%{public}dx%{public}d streamId:%{public}d",
        format_, width_, height_, GetFwkStreamId());
}

int32_t HStreamCapture::LinkInput(sptr<HDI::Camera::V1_0::IStreamOperator> streamOperator,
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility)
{
    MEDIA_INFO_LOG("HStreamCapture::LinkInput streamId:%{public}d", GetFwkStreamId());
    return HStreamCommon::LinkInput(streamOperator, cameraAbility);
}

void HStreamCapture::FullfillPictureExtendStreamInfos(StreamInfo_V1_1 &streamInfo, int32_t format)
{
    HDI::Camera::V1_1::ExtendedStreamInfo gainmapExtendedStreamInfo = {
        .type = static_cast<HDI::Camera::V1_1::ExtendedStreamInfoType>(HDI::Camera::V1_3::EXTENDED_STREAM_INFO_GAINMAP),
        .width = width_,
        .height = height_,
        .format = format,
        .bufferQueue = gainmapBufferQueue_,
    };
    HDI::Camera::V1_1::ExtendedStreamInfo deepExtendedStreamInfo = {
        .type = static_cast<HDI::Camera::V1_1::ExtendedStreamInfoType>(HDI::Camera::V1_3::EXTENDED_STREAM_INFO_DEPTH),
        .width = width_,
        .height = height_,
        .format = format,
        .bufferQueue = deepBufferQueue_,
    };
    HDI::Camera::V1_1::ExtendedStreamInfo exifExtendedStreamInfo = {
        .type = static_cast<HDI::Camera::V1_1::ExtendedStreamInfoType>(HDI::Camera::V1_3::EXTENDED_STREAM_INFO_EXIF),
        .width = width_,
        .height = height_,
        .format = format,
        .bufferQueue = exifBufferQueue_,
    };
    HDI::Camera::V1_1::ExtendedStreamInfo debugExtendedStreamInfo = {
        .type =
            static_cast<HDI::Camera::V1_1::ExtendedStreamInfoType>(HDI::Camera::V1_3::EXTENDED_STREAM_INFO_MAKER_INFO),
        .width = width_,
        .height = height_,
        .format = format,
        .bufferQueue = debugBufferQueue_,
    };
    std::vector<HDI::Camera::V1_1::ExtendedStreamInfo> extendedStreams = { gainmapExtendedStreamInfo,
        deepExtendedStreamInfo, exifExtendedStreamInfo, debugExtendedStreamInfo };
    streamInfo.extendedStreamInfos.insert(streamInfo.extendedStreamInfos.end(),
        extendedStreams.begin(), extendedStreams.end());
}

void HStreamCapture::SetStreamInfo(StreamInfo_V1_1 &streamInfo)
{
    HStreamCommon::SetStreamInfo(streamInfo);
    streamInfo.v1_0.intent_ = STILL_CAPTURE;
    if (format_ == OHOS_CAMERA_FORMAT_HEIC) {
        streamInfo.v1_0.encodeType_ =
            static_cast<HDI::Camera::V1_0::EncodeType>(HDI::Camera::V1_3::ENCODE_TYPE_HEIC);
        streamInfo.v1_0.format_ = GRAPHIC_PIXEL_FMT_BLOB;
    } else if (format_ == OHOS_CAMERA_FORMAT_YCRCB_420_SP) { // NV21
        streamInfo.v1_0.encodeType_ = ENCODE_TYPE_NULL;
        streamInfo.v1_0.format_ = GRAPHIC_PIXEL_FMT_YCRCB_420_SP; // NV21
        FullfillPictureExtendStreamInfos(streamInfo, GRAPHIC_PIXEL_FMT_YCRCB_420_SP);
    } else {
        streamInfo.v1_0.encodeType_ = ENCODE_TYPE_JPEG;
    }
    if (rawDeliverySwitch_) {
        MEDIA_INFO_LOG("HStreamCapture::SetStreamInfo Set DNG info, streamId:%{public}d", GetFwkStreamId());
        HDI::Camera::V1_1::ExtendedStreamInfo extendedStreamInfo = {
            .type =
                static_cast<HDI::Camera::V1_1::ExtendedStreamInfoType>(HDI::Camera::V1_3::EXTENDED_STREAM_INFO_RAW),
            .width = width_,
            .height = height_,
            .format = streamInfo.v1_0.format_,
            .dataspace = 0,
            .bufferQueue = rawBufferQueue_,
        };
        streamInfo.extendedStreamInfos.push_back(extendedStreamInfo);
    }
    if (thumbnailSwitch_) {
        HDI::Camera::V1_1::ExtendedStreamInfo extendedStreamInfo = {
            .type = HDI::Camera::V1_1::EXTENDED_STREAM_INFO_QUICK_THUMBNAIL,
            .width = 0,
            .height = 0,
            .format = 0,
            .dataspace = 0,
            .bufferQueue = thumbnailBufferQueue_,
        };
        streamInfo.extendedStreamInfos.push_back(extendedStreamInfo);
    }
}

int32_t HStreamCapture::SetThumbnail(bool isEnabled, const sptr<OHOS::IBufferProducer> &producer)
{
    if (isEnabled && producer != nullptr) {
        thumbnailSwitch_ = 1;
        thumbnailBufferQueue_ = new BufferProducerSequenceable(producer);
    } else {
        thumbnailSwitch_ = 0;
        thumbnailBufferQueue_ = nullptr;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::EnableRawDelivery(bool enabled)
{
    if (enabled) {
        rawDeliverySwitch_ = 1;
    } else {
        rawDeliverySwitch_ = 0;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::EnableMovingPhoto(bool enabled)
{
    if (enabled) {
        movingPhotoSwitch_ = 1;
    } else {
        movingPhotoSwitch_ = 0;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::SetBufferProducerInfo(const std::string bufName, const sptr<OHOS::IBufferProducer> &producer)
{
    std::string resStr = "";
    if (bufName == "rawImage") {
        if (producer != nullptr) {
            rawBufferQueue_ = new BufferProducerSequenceable(producer);
        } else {
            rawBufferQueue_ = nullptr;
            resStr += bufName + ",";
        }
    }
    if (bufName == "gainmapImage") {
        if (producer != nullptr) {
            gainmapBufferQueue_ = new BufferProducerSequenceable(producer);
        } else {
            gainmapBufferQueue_ = nullptr;
            resStr += bufName + ",";
        }
    }
    if (bufName == "deepImage") {
        if (producer != nullptr) {
            deepBufferQueue_ = new BufferProducerSequenceable(producer);
        } else {
            deepBufferQueue_ = nullptr;
            resStr += bufName + ",";
        }
    }
    if (bufName == "exifImage") {
        if (producer != nullptr) {
            exifBufferQueue_ = new BufferProducerSequenceable(producer);
        } else {
            exifBufferQueue_ = nullptr;
            resStr += bufName + ",";
        }
    }
    if (bufName == "debugImage") {
        if (producer != nullptr) {
            debugBufferQueue_ = new BufferProducerSequenceable(producer);
        } else {
            debugBufferQueue_ = nullptr;
            resStr += bufName + ",";
        }
    }
    MEDIA_INFO_LOG("HStreamCapture::SetBufferProducerInfo bufferQueue whether is nullptr: %{public}s", resStr.c_str());
    return CAMERA_OK;
}

int32_t HStreamCapture::DeferImageDeliveryFor(int32_t type)
{
    MEDIA_INFO_LOG("HStreamCapture::DeferImageDeliveryFor type: %{public}d", type);
    if (type == OHOS::HDI::Camera::V1_2::STILL_IMAGE) {
        MEDIA_INFO_LOG("HStreamCapture STILL_IMAGE");
        deferredPhotoSwitch_ = 1;
    } else if (type == OHOS::HDI::Camera::V1_2::MOVING_IMAGE) {
        MEDIA_INFO_LOG("HStreamCapture MOVING_IMAGE");
        deferredVideoSwitch_ = 1;
    } else {
        MEDIA_INFO_LOG("HStreamCapture NONE");
        deferredPhotoSwitch_ = 0;
        deferredVideoSwitch_ = 0;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::PrepareBurst(int32_t captureId)
{
    MEDIA_INFO_LOG("HStreamCapture::PrepareBurst captureId:%{public}d", captureId);
    isBursting_ = true;
    std::lock_guard<std::mutex> lock(burstLock_);
    curBurstKey_ = GenerateBurstUuid();
    burstkeyMap_.emplace(captureId, curBurstKey_);
    std::vector<std::string> imageList = {};
    burstImagesMap_.emplace(captureId, imageList);
    burstNumMap_.emplace(captureId, 0);
    burstNum_ = 0;
    return CAMERA_OK;
}

void HStreamCapture::ResetBurst()
{
    MEDIA_INFO_LOG("HStreamCapture::ResetBurst");
    curBurstKey_ = BURST_UUID_UNSET;
    isBursting_ = false;
}

void HStreamCapture::ResetBurstKey(int32_t captureId)
{
    if (burstkeyMap_.erase(captureId) > 0 &&
        burstImagesMap_.erase(captureId) > 0 &&
        burstNumMap_.erase(captureId) > 0) {
        MEDIA_INFO_LOG("HStreamCapture::ResetBurstKey captureId:%{public}d", captureId);
    } else {
        MEDIA_DEBUG_LOG("HStreamCapture::ResetBurstKey captureId not found");
    }
}

std::string HStreamCapture::GetBurstKey(int32_t captureId) const
{
    MEDIA_DEBUG_LOG("HStreamCapture::GetBurstKey captureId:%{public}d", captureId);
    std::string burstKey = BURST_UUID_UNSET;
    std::lock_guard<std::mutex> lock(burstLock_);
    auto iter = burstkeyMap_.find(captureId);
    if (iter != burstkeyMap_.end()) {
        burstKey = iter->second;
        MEDIA_DEBUG_LOG("HStreamCapture::GetBurstKey %{public}s", burstKey.c_str());
    } else {
        MEDIA_DEBUG_LOG("HStreamCapture::GetBurstKey not found");
    }
    return burstKey;
}

bool HStreamCapture::IsBurstCapture(int32_t captureId) const
{
    MEDIA_DEBUG_LOG("HStreamCapture::captureId:%{public}d", captureId);
    auto iter = burstkeyMap_.find(captureId);
    return iter != burstkeyMap_.end();
}

bool HStreamCapture::IsBurstCover(int32_t captureId) const
{
    MEDIA_DEBUG_LOG("HStreamCapture::IsBurstCover for captureId: %d", captureId);
    std::lock_guard<std::mutex> lock(burstLock_);
    auto iter = burstImagesMap_.find(captureId);
    return (iter != burstImagesMap_.end()) ? (iter->second.size() == 1) : false;
}

int32_t HStreamCapture::GetCurBurstSeq(int32_t captureId) const
{
    MEDIA_DEBUG_LOG("HStreamCapture::GetCurBurstSeq for captureId: %d", captureId);
    std::lock_guard<std::mutex> lock(burstLock_);
    auto iter = burstImagesMap_.find(captureId);
    CHECK_ERROR_RETURN_RET(iter != burstImagesMap_.end(), iter->second.size());
    return -1;
}

void HStreamCapture::SetBurstImages(int32_t captureId, std::string imageId)
{
    MEDIA_DEBUG_LOG("HStreamCapture::SetBurstImages captureId:%{public}d imageId:%{public}s",
        captureId, imageId.c_str());
    std::lock_guard<std::mutex> lock(burstLock_);
    auto iter = burstImagesMap_.find(captureId);
    if (iter != burstImagesMap_.end()) {
        iter->second.emplace_back(imageId);
        MEDIA_DEBUG_LOG("HStreamCapture::SetBurstImages success");
    } else {
        MEDIA_ERR_LOG("HStreamCapture::SetBurstImages error");
    }
}

void HStreamCapture::CheckResetBurstKey(int32_t captureId)
{
    MEDIA_DEBUG_LOG("HStreamCapture::CheckResetBurstKey captureId:%{public}d", captureId);
    std::lock_guard<std::mutex> lock(burstLock_);
    auto numIter = burstNumMap_.find(captureId);
    auto imageIter = burstImagesMap_.find(captureId);
    if (numIter != burstNumMap_.end() && imageIter != burstImagesMap_.end()) {
        int32_t burstSum = numIter->second;
        size_t curBurstSum = imageIter->second.size();
        MEDIA_DEBUG_LOG("CheckResetBurstKey: burstSum=%d, curBurstSum=%zu", burstSum, curBurstSum);
        CHECK_EXECUTE(static_cast<size_t>(burstSum) == curBurstSum, ResetBurstKey(captureId));
    } else {
        MEDIA_DEBUG_LOG("CheckResetBurstKey: captureId %d not found in one or both maps", captureId);
    }
}


int32_t HStreamCapture::CheckBurstCapture(const std::shared_ptr<OHOS::Camera::CameraMetadata>& captureSettings,
                                          const int32_t &preparedCaptureId)
{
    MEDIA_INFO_LOG("CheckBurstCapture start!");
    camera_metadata_item_t item;
    CHECK_ERROR_RETURN_RET_LOG(captureSettings == nullptr, CAMERA_INVALID_STATE, "captureSettings is nullptr");
    int32_t result = OHOS::Camera::FindCameraMetadataItem(captureSettings->get(), OHOS_CONTROL_BURST_CAPTURE, &item);
    if (result == CAM_META_SUCCESS && item.count > 0) {
        CameraBurstCaptureEnum burstState = static_cast<CameraBurstCaptureEnum>(item.data.u8[0]);
        MEDIA_INFO_LOG("CheckBurstCapture get burstState:%{public}d", item.data.u8[0]);
        if (burstState) {
            std::string burstUuid = GetBurstKey(preparedCaptureId);
            CHECK_ERROR_RETURN_RET_LOG(burstUuid != BURST_UUID_UNSET || isBursting_, CAMERA_INVALID_STATE,
                "CheckBurstCapture faild!");
            PrepareBurst(preparedCaptureId);
            MEDIA_INFO_LOG("CheckBurstCapture ready!");
        }
    }
    return CAM_META_SUCCESS;
}

int32_t HStreamCapture::CreateMediaLibraryPhotoAssetProxy(int32_t captureId)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(photoAssetLock_);
    MEDIA_DEBUG_LOG("CreateMediaLibraryPhotoAssetProxy E captureId:%{public}d", captureId);
    if (photoAssetProxy_.find(captureId) != photoAssetProxy_.end()) {
        return CAMERA_OK;
    }
    constexpr int32_t imageShotType = 0;
    constexpr int32_t movingPhotoShotType = 2;
    constexpr int32_t burstShotType = 3;
    int32_t cameraShotType = imageShotType;
    if (movingPhotoSwitch_) {
        cameraShotType = movingPhotoShotType;
    } else if (isBursting_) {
        cameraShotType = burstShotType;
    }
    auto photoAssetProxy = PhotoAssetProxy::GetPhotoAssetProxy(cameraShotType, IPCSkeleton::GetCallingUid());
    CHECK_ERROR_RETURN_RET_LOG(photoAssetProxy == nullptr, CAMERA_ALLOC_ERROR,
        "HStreamCapture::CreateMediaLibraryPhotoAssetProxy get photoAssetProxy fail");
    photoAssetProxy_.emplace(std::pair<int32_t, std::shared_ptr<PhotoAssetIntf>>(captureId, photoAssetProxy));
    MEDIA_DEBUG_LOG("CreateMediaLibraryPhotoAssetProxy X captureId:%{public}d", captureId);
    return CAMERA_OK;
}

std::shared_ptr<PhotoAssetIntf> HStreamCapture::GetPhotoAssetInstance(int32_t captureId)
{
    std::lock_guard<std::mutex> lock(photoAssetLock_);
    auto it = photoAssetProxy_.find(captureId);
    if (it == photoAssetProxy_.end()) {
        return nullptr;
    }
    std::shared_ptr<PhotoAssetIntf> proxy = it->second;
    photoAssetProxy_.erase(it);
    return proxy;
}

void HStreamCapture::EnableAddPhotoProxy(bool enabled)
{
    std::lock_guard<mutex> lock(enableAddPhotoProxyLock_);
    enableAddPhotoProxy_ = enabled;
}

bool HStreamCapture::GetAddPhotoProxyEnabled()
{
    return enableAddPhotoProxy_;
}

int32_t HStreamCapture::AcquireBufferToPrepareProxy(int32_t captureId)
{
    MEDIA_DEBUG_LOG("HStreamCapture::AcquireBufferToPrepareProxy start");
    CameraReportDfxUtils::GetInstance()->SetFirstBufferEndInfo(captureId);
    CameraReportDfxUtils::GetInstance()->SetPrepareProxyStartInfo(captureId);
    MEDIA_DEBUG_LOG("HStreamCapture::AcquireBufferToPrepareProxy end");
    return CAMERA_OK;
}

int32_t HStreamCapture::Capture(const std::shared_ptr<OHOS::Camera::CameraMetadata>& captureSettings)
{
    CAMERA_SYNC_TRACE;
    MEDIA_INFO_LOG("HStreamCapture::Capture Entry, streamId:%{public}d", GetFwkStreamId());
    auto streamOperator = GetStreamOperator();
    CHECK_ERROR_RETURN_RET(streamOperator == nullptr, CAMERA_INVALID_STATE);
    CHECK_ERROR_RETURN_RET_LOG(isCaptureReady_ == false, CAMERA_OPERATION_NOT_ALLOWED,
        "HStreamCapture::Capture failed due to capture not ready");
    auto preparedCaptureId = GetPreparedCaptureId();
    CHECK_ERROR_RETURN_RET_LOG(preparedCaptureId != CAPTURE_ID_UNSET, CAMERA_INVALID_STATE,
        "HStreamCapture::Capture, Already started with captureID: %{public}d", preparedCaptureId);
    int32_t ret = PrepareCaptureId();
    preparedCaptureId = GetPreparedCaptureId();
    CHECK_ERROR_RETURN_RET_LOG(ret != CAMERA_OK || preparedCaptureId == CAPTURE_ID_UNSET, ret,
        "HStreamCapture::Capture Failed to allocate a captureId");
    ret = CheckBurstCapture(captureSettings, preparedCaptureId);
    CHECK_ERROR_RETURN_RET_LOG(ret != CAMERA_OK, ret, "HStreamCapture::Capture Failed with burst state error");

    CaptureDfxInfo captureDfxInfo;
    captureDfxInfo.captureId = preparedCaptureId;
    captureDfxInfo.isSystemApp = CheckSystemApp();
    captureDfxInfo.bundleName = BmsAdapter::GetInstance()->GetBundleName(IPCSkeleton::GetCallingUid());
    CameraReportDfxUtils::GetInstance()->SetFirstBufferStartInfo(captureDfxInfo);

    CaptureInfo captureInfoPhoto;
    captureInfoPhoto.streamIds_ = { GetHdiStreamId() };
    ProcessCaptureInfoPhoto(captureInfoPhoto, captureSettings, preparedCaptureId);

    auto callingTokenId = IPCSkeleton::GetCallingTokenID();
    const std::string permissionName = "ohos.permission.CAMERA";
    AddCameraPermissionUsedRecord(callingTokenId, permissionName);

    // report capture performance dfx
    std::shared_ptr<OHOS::Camera::CameraMetadata> captureMetadataSetting_ = nullptr;
    OHOS::Camera::MetadataUtils::ConvertVecToMetadata(captureInfoPhoto.captureSetting_, captureMetadataSetting_);
    if (captureMetadataSetting_ == nullptr) {
        captureMetadataSetting_ = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    }
    DfxCaptureInfo captureInfo;
    captureInfo.captureId = preparedCaptureId;
    captureInfo.caller = CameraReportUtils::GetCallerInfo();
    CameraReportUtils::GetInstance().SetCapturePerfStartInfo(captureInfo);
    MEDIA_INFO_LOG("HStreamCapture::Capture Starting photo capture with capture ID: %{public}d", preparedCaptureId);
    HStreamCommon::PrintCaptureDebugLog(captureMetadataSetting_);
    CamRetCode rc = (CamRetCode)(streamOperator->Capture(preparedCaptureId, captureInfoPhoto, isBursting_));
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        ResetCaptureId();
        captureIdForConfirmCapture_ = CAPTURE_ID_UNSET;
        MEDIA_ERR_LOG("HStreamCapture::Capture failed with error Code: %{public}d", rc);
        CameraReportUtils::ReportCameraError(
            "HStreamCapture::Capture", rc, true, CameraReportUtils::GetCallerInfo());
        ret = HdiToServiceError(rc);
    }
    camera_metadata_item_t item;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    {
        std::lock_guard<std::mutex> lock(cameraAbilityLock_);
        CHECK_ERROR_RETURN_RET_LOG(cameraAbility_ == nullptr, CAMERA_INVALID_STATE,
            "HStreamCapture::cameraAbility_ is null");
        int32_t result = OHOS::Camera::FindCameraMetadataItem(cameraAbility_->get(), OHOS_ABILITY_CAMERA_POSITION,
                                                              &item);
        if (result == CAM_META_SUCCESS && item.count > 0) {
            cameraPosition = static_cast<camera_position_enum_t>(item.data.u8[0]);
        }
    }

    int32_t NightMode = 4;
    CHECK_ERROR_RETURN_RET(GetMode() == NightMode && cameraPosition == OHOS_CAMERA_POSITION_BACK, ret);
    ResetCaptureId();

    uint32_t major;
    uint32_t minor;
    streamOperator->GetVersion(major, minor);
    MEDIA_INFO_LOG("streamOperator GetVersion major:%{public}d, minor:%{public}d", major, minor);
    // intercept when streamOperatorCallback support onCaptureReady
    if (major >= HDI_VERSION_1 && minor >= HDI_VERSION_2 && !isBursting_) {
        MEDIA_INFO_LOG("HStreamCapture::Capture set capture not ready");
        isCaptureReady_ = false;
    }
    if (!isBursting_) {
        MEDIA_DEBUG_LOG("HStreamCapture::Capture CreateMediaLibraryPhotoAssetProxy E");
        CHECK_ERROR_PRINT_LOG(CreateMediaLibraryPhotoAssetProxy(preparedCaptureId) != CAMERA_OK,
            "HStreamCapture::Capture Failed with CreateMediaLibraryPhotoAssetProxy");
        MEDIA_DEBUG_LOG("HStreamCapture::Capture CreateMediaLibraryPhotoAssetProxy X");
    }
    return ret;
}

void HStreamCapture::ProcessCaptureInfoPhoto(CaptureInfo& captureInfoPhoto,
    const std::shared_ptr<OHOS::Camera::CameraMetadata>& captureSettings, int32_t captureId)
{
    if (!OHOS::Camera::GetCameraMetadataItemCount(captureSettings->get())) {
        std::lock_guard<std::mutex> lock(cameraAbilityLock_);
        OHOS::Camera::MetadataUtils::ConvertMetadataToVec(cameraAbility_, captureInfoPhoto.captureSetting_);
    } else {
        OHOS::Camera::MetadataUtils::ConvertMetadataToVec(captureSettings, captureInfoPhoto.captureSetting_);
    }
    captureInfoPhoto.enableShutterCallback_ = true;
    std::shared_ptr<OHOS::Camera::CameraMetadata> captureMetadataSetting_ = nullptr;
    OHOS::Camera::MetadataUtils::ConvertVecToMetadata(captureInfoPhoto.captureSetting_, captureMetadataSetting_);
    if (captureMetadataSetting_ != nullptr) {
        // convert rotation with application set rotation
        SetRotation(captureMetadataSetting_, captureId);

        // update settings
        std::vector<uint8_t> finalSetting;
        OHOS::Camera::MetadataUtils::ConvertMetadataToVec(captureMetadataSetting_, finalSetting);
        captureInfoPhoto.captureSetting_ = finalSetting;
    }
}

void HStreamCapture::SetRotation(const std::shared_ptr<OHOS::Camera::CameraMetadata> &captureMetadataSetting_,
    int32_t captureId)
{
    // set orientation for capture
    // sensor orientation, counter-clockwise rotation
    int32_t sensorOrientation = 0;
    int result;
    camera_metadata_item_t item;
    {
        std::lock_guard<std::mutex> lock(cameraAbilityLock_);
        CHECK_ERROR_RETURN(cameraAbility_ == nullptr);
        result = OHOS::Camera::FindCameraMetadataItem(cameraAbility_->get(), OHOS_SENSOR_ORIENTATION, &item);
        if (result == CAM_META_SUCCESS && item.count > 0) {
            sensorOrientation = item.data.i32[0];
        }
        MEDIA_INFO_LOG("set rotation sensor orientation %{public}d", sensorOrientation);

        camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
        result = OHOS::Camera::FindCameraMetadataItem(cameraAbility_->get(), OHOS_ABILITY_CAMERA_POSITION, &item);
        if (result == CAM_META_SUCCESS && item.count > 0) {
            cameraPosition = static_cast<camera_position_enum_t>(item.data.u8[0]);
        }
        MEDIA_INFO_LOG("set rotation camera position %{public}d", cameraPosition);
    }

    // rotation from application
    int32_t rotationValue = 0;
    int32_t rotation = 0;
    result = OHOS::Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_JPEG_ORIENTATION, &item);
    if (result == CAM_META_SUCCESS && item.count > 0) {
        rotationValue = item.data.i32[0];
    }
    MEDIA_INFO_LOG("set rotation app rotationValue %{public}d", rotationValue);
    rotationMap_.EnsureInsert(captureId, rotationValue);
    // real rotation
    if (enableCameraPhotoRotation_) {
        rotation = rotationValue;
    } else {
        rotation = sensorOrientation + rotationValue;
        if (rotation >= CAPTURE_ROTATE_360) {
            rotation = rotation - CAPTURE_ROTATE_360;
        }
    }
    {
        uint8_t connectType = 0;
        std::lock_guard<std::mutex> lock(cameraAbilityLock_);
        CHECK_ERROR_RETURN(cameraAbility_ == nullptr);
        int ret = OHOS::Camera::FindCameraMetadataItem(
            cameraAbility_->get(), OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
        if (ret == CAM_META_SUCCESS && item.count > 0) {
            connectType = item.data.u8[0];
        }
        if (connectType == OHOS_CAMERA_CONNECTION_TYPE_REMOTE) {
            rotation = rotationValue;
        }
        MEDIA_INFO_LOG("set rotation camera real rotation %{public}d", rotation);
    }

    bool status = false;
    if (result == CAM_META_ITEM_NOT_FOUND) {
        status = captureMetadataSetting_->addEntry(OHOS_JPEG_ORIENTATION, &rotation, 1);
    } else if (result == CAM_META_SUCCESS) {
        status = captureMetadataSetting_->updateEntry(OHOS_JPEG_ORIENTATION, &rotation, 1);
    }
    result = OHOS::Camera::FindCameraMetadataItem(captureMetadataSetting_->get(), OHOS_JPEG_ORIENTATION, &item);
    CHECK_ERROR_PRINT_LOG(result != CAM_META_SUCCESS, "set rotation Failed to find OHOS_JPEG_ORIENTATION tag");
    CHECK_ERROR_PRINT_LOG(!status, "set rotation Failed to set Rotation");
}

int32_t HStreamCapture::CancelCapture()
{
    CAMERA_SYNC_TRACE;
    // Cancel capture is dummy till continuous/burst mode is supported
    StopStream();
    return CAMERA_OK;
}

void HStreamCapture::SetMode(int32_t modeName)
{
    modeName_ = modeName;
    MEDIA_DEBUG_LOG("HStreamCapture SetMode modeName = %{public}d", modeName);
}

int32_t HStreamCapture::GetMode()
{
    MEDIA_INFO_LOG("HStreamCapture GetMode modeName = %{public}d", modeName_);
    return modeName_;
}

int32_t HStreamCapture::ConfirmCapture()
{
    CAMERA_SYNC_TRACE;
    auto streamOperator = GetStreamOperator();
    CHECK_ERROR_RETURN_RET(streamOperator == nullptr, CAMERA_INVALID_STATE);
    int32_t ret = 0;

    // end burst capture
    if (isBursting_) {
        MEDIA_INFO_LOG("HStreamCapture::ConfirmCapture when burst capture");
        std::vector<uint8_t> settingVector;
        std::shared_ptr<OHOS::Camera::CameraMetadata> burstCaptureSettings = nullptr;
        OHOS::Camera::MetadataUtils::ConvertMetadataToVec(cameraAbility_, settingVector);
        OHOS::Camera::MetadataUtils::ConvertVecToMetadata(settingVector, burstCaptureSettings);
        if (burstCaptureSettings == nullptr) {
            burstCaptureSettings = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
        }
        EndBurstCapture(burstCaptureSettings);
        ret = Capture(burstCaptureSettings);
        CHECK_ERROR_PRINT_LOG(ret != CAMERA_OK, "HStreamCapture::ConfirmCapture end burst faild!");
        return ret;
    }

    auto preparedCaptureId = captureIdForConfirmCapture_;
    MEDIA_INFO_LOG("HStreamCapture::ConfirmCapture with capture ID: %{public}d", preparedCaptureId);
    sptr<HDI::Camera::V1_2::IStreamOperator> streamOperatorV1_2 =
        OHOS::HDI::Camera::V1_2::IStreamOperator::CastFrom(streamOperator);
    CHECK_ERROR_RETURN_RET_LOG(streamOperatorV1_2 == nullptr, CAMERA_UNKNOWN_ERROR,
        "HStreamCapture::ConfirmCapture streamOperatorV1_2 castFrom failed!");
    OHOS::HDI::Camera::V1_2::CamRetCode rc =
        (HDI::Camera::V1_2::CamRetCode)(streamOperatorV1_2->ConfirmCapture(preparedCaptureId));
    if (rc != HDI::Camera::V1_2::NO_ERROR) {
        MEDIA_ERR_LOG("HStreamCapture::ConfirmCapture failed with error Code: %{public}d", rc);
        ret = HdiToServiceErrorV1_2(rc);
    }
    ResetCaptureId();
    captureIdForConfirmCapture_ = CAPTURE_ID_UNSET;
    return ret;
}

void HStreamCapture::EndBurstCapture(const std::shared_ptr<OHOS::Camera::CameraMetadata>& captureMetadataSetting)
{
    CHECK_ERROR_RETURN(captureMetadataSetting == nullptr);
    MEDIA_INFO_LOG("HStreamCapture::EndBurstCapture");
    camera_metadata_item_t item;
    bool status = false;
    int result = OHOS::Camera::FindCameraMetadataItem(captureMetadataSetting->get(), OHOS_CONTROL_BURST_CAPTURE, &item);
    uint8_t burstState = 0;
    if (result == CAM_META_ITEM_NOT_FOUND) {
        status = captureMetadataSetting->addEntry(OHOS_CONTROL_BURST_CAPTURE, &burstState, 1);
    } else if (result == CAM_META_SUCCESS) {
        status = captureMetadataSetting->updateEntry(OHOS_CONTROL_BURST_CAPTURE, &burstState, 1);
    }

    CHECK_ERROR_PRINT_LOG(!status, "HStreamCapture::EndBurstCapture Failed");
}

int32_t HStreamCapture::Release()
{
    return ReleaseStream(false);
}

int32_t HStreamCapture::ReleaseStream(bool isDelay)
{
    {
        std::lock_guard<std::mutex> lock(callbackLock_);
        streamCaptureCallback_ = nullptr;
    }
    return HStreamCommon::ReleaseStream(isDelay);
}

int32_t HStreamCapture::SetCallback(sptr<IStreamCaptureCallback> &callback)
{
    CHECK_ERROR_RETURN_RET_LOG(callback == nullptr, CAMERA_INVALID_ARG, "HStreamCapture::SetCallback input is null");
    std::lock_guard<std::mutex> lock(callbackLock_);
    streamCaptureCallback_ = callback;
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureStarted(int32_t captureId)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(callbackLock_);
    CHECK_EXECUTE(streamCaptureCallback_ != nullptr, streamCaptureCallback_->OnCaptureStarted(captureId));
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureStarted(int32_t captureId, uint32_t exposureTime)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(callbackLock_);
    CHECK_EXECUTE(streamCaptureCallback_ != nullptr,
        streamCaptureCallback_->OnCaptureStarted(captureId, exposureTime));
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureEnded(int32_t captureId, int32_t frameCount)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(callbackLock_);
    CHECK_EXECUTE(streamCaptureCallback_ != nullptr, streamCaptureCallback_->OnCaptureEnded(captureId, frameCount));
    CameraReportUtils::GetInstance().SetCapturePerfEndInfo(captureId);
    auto preparedCaptureId = GetPreparedCaptureId();
    if (preparedCaptureId != CAPTURE_ID_UNSET) {
        MEDIA_INFO_LOG("HStreamCapture::OnCaptureEnded capturId = %{public}d already used, need release",
                       preparedCaptureId);
        ResetCaptureId();
        captureIdForConfirmCapture_ = CAPTURE_ID_UNSET;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::OnCaptureError(int32_t captureId, int32_t errorCode)
{
    std::lock_guard<std::mutex> lock(callbackLock_);
    if (streamCaptureCallback_ != nullptr) {
        int32_t captureErrorCode;
        if (errorCode == BUFFER_LOST) {
            captureErrorCode = CAMERA_STREAM_BUFFER_LOST;
        } else {
            captureErrorCode = CAMERA_UNKNOWN_ERROR;
        }
        CAMERA_SYSEVENT_FAULT(CreateMsg("Photo OnCaptureError! captureId:%d & "
                                        "errorCode:%{public}d", captureId, captureErrorCode));
        streamCaptureCallback_->OnCaptureError(captureId, captureErrorCode);
    }
    auto preparedCaptureId = GetPreparedCaptureId();
    if (preparedCaptureId != CAPTURE_ID_UNSET) {
        MEDIA_INFO_LOG("HStreamCapture::OnCaptureError capturId = %{public}d already used, need release",
                       preparedCaptureId);
        ResetCaptureId();
        captureIdForConfirmCapture_ = CAPTURE_ID_UNSET;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::OnFrameShutter(int32_t captureId, uint64_t timestamp)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(callbackLock_);
    CHECK_EXECUTE(streamCaptureCallback_ != nullptr,
        streamCaptureCallback_->OnFrameShutter(captureId, timestamp));
    return CAMERA_OK;
}

int32_t HStreamCapture::OnFrameShutterEnd(int32_t captureId, uint64_t timestamp)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(callbackLock_);
    CHECK_EXECUTE(streamCaptureCallback_ != nullptr,
        streamCaptureCallback_->OnFrameShutterEnd(captureId, timestamp));
    if (isBursting_) {
        burstNum_++;
        MEDIA_DEBUG_LOG("HStreamCapture::OnFrameShutterEnd burstNum:%{public}d", burstNum_);
    }
    return CAMERA_OK;
}


int32_t HStreamCapture::OnCaptureReady(int32_t captureId, uint64_t timestamp)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(callbackLock_);
    MEDIA_INFO_LOG("HStreamCapture::Capture, notify OnCaptureReady with capture ID: %{public}d", captureId);
    isCaptureReady_ = true;
    CHECK_EXECUTE(streamCaptureCallback_ != nullptr,
        streamCaptureCallback_->OnCaptureReady(captureId, timestamp));
    std::lock_guard<std::mutex> burstLock(burstLock_);
    if (IsBurstCapture(captureId)) {
        burstNumMap_[captureId] = burstNum_;
        ResetBurst();
    }
    return CAMERA_OK;
}

void HStreamCapture::DumpStreamInfo(CameraInfoDumper& infoDumper)
{
    infoDumper.Title("capture stream");
    infoDumper.Msg("ThumbnailSwitch:[" + std::to_string(thumbnailSwitch_) + "]");
    infoDumper.Msg("RawDeliverSwitch:[" + std::to_string(rawDeliverySwitch_) + "]");
    if (thumbnailBufferQueue_) {
        infoDumper.Msg(
            "ThumbnailBuffer producer Id:[" + std::to_string(thumbnailBufferQueue_->producer_->GetUniqueId()) + "]");
    }
    HStreamCommon::DumpStreamInfo(infoDumper);
}

int32_t HStreamCapture::OperatePermissionCheck(uint32_t interfaceCode)
{
    switch (static_cast<StreamCaptureInterfaceCode>(interfaceCode)) {
        case CAMERA_STREAM_CAPTURE_START: {
            auto callerToken = IPCSkeleton::GetCallingTokenID();
            CHECK_ERROR_RETURN_RET_LOG(callerToken_ != callerToken, CAMERA_OPERATION_NOT_ALLOWED,
                "HStreamCapture::OperatePermissionCheck fail, callerToken_ is : %{public}d, now token "
                "is %{public}d", callerToken_, callerToken);
            break;
        }
        default:
            break;
    }
    return CAMERA_OK;
}

int32_t HStreamCapture::IsDeferredPhotoEnabled()
{
    MEDIA_INFO_LOG("HStreamCapture IsDeferredPhotoEnabled  deferredPhotoSwitch_: %{public}d", deferredPhotoSwitch_);
    CHECK_ERROR_RETURN_RET(deferredPhotoSwitch_ == 1, 1);
    MEDIA_INFO_LOG("HStreamCapture IsDeferredPhotoEnabled return 0");
    return 0;
}

int32_t HStreamCapture::IsDeferredVideoEnabled()
{
    MEDIA_INFO_LOG("HStreamCapture IsDeferredVideoEnabled  deferredVideoSwitch_: %{public}d", deferredVideoSwitch_);
    CHECK_ERROR_RETURN_RET(deferredVideoSwitch_ == 1, 1);
    return 0;
}

int32_t HStreamCapture::GetMovingPhotoVideoCodecType()
{
    MEDIA_INFO_LOG("HStreamCapture GetMovingPhotoVideoCodecType videoCodecType_: %{public}d", videoCodecType_);
    return videoCodecType_;
}

int32_t HStreamCapture::SetMovingPhotoVideoCodecType(int32_t videoCodecType)
{
    MEDIA_INFO_LOG("HStreamCapture SetMovingPhotoVideoCodecType videoCodecType_: %{public}d", videoCodecType);
    videoCodecType_ = videoCodecType;
    return 0;
}

int32_t HStreamCapture::SetCameraPhotoRotation(bool isEnable)
{
    enableCameraPhotoRotation_ = isEnable;
    return 0;
}

void HStreamCapture::SetCameraPhotoProxyInfo(sptr<CameraServerPhotoProxy> cameraPhotoProxy)
{
    MEDIA_INFO_LOG("SetCameraPhotoProxyInfo get captureStream");
    cameraPhotoProxy->SetDisplayName(CreateDisplayName());
    cameraPhotoProxy->SetShootingMode(GetMode());
    MEDIA_INFO_LOG("SetCameraPhotoProxyInfo quality:%{public}d, format:%{public}d",
        cameraPhotoProxy->GetPhotoQuality(), cameraPhotoProxy->GetFormat());
}

int32_t HStreamCapture::UpdateMediaLibraryPhotoAssetProxy(sptr<CameraPhotoProxy> photoProxy)
{
    if (isBursting_) {
        EnableAddPhotoProxy(false);
        return CAMERA_UNSUPPORTED;
    }
    std::lock_guard<std::mutex> lock(photoAssetLock_);
    MEDIA_DEBUG_LOG("HStreamCapture UpdateMediaLibraryPhotoAssetProxy E captureId(%{public}d)", photoProxy->captureId_);
    auto iter = photoAssetProxy_.find(photoProxy->captureId_);
    if (iter == photoAssetProxy_.end()) {
        MEDIA_ERR_LOG("HStreamCapture UpdateMediaLibraryPhotoAssetProxy failed, no iter captureId(%{public}d)",
            photoProxy->captureId_);
        return CAMERA_INVALID_ARG;
    }
    auto photoAssetProxy = iter->second;
    CHECK_ERROR_RETURN_RET_LOG(photoAssetProxy == nullptr, CAMERA_UNKNOWN_ERROR,
        "HStreamCapture UpdateMediaLibraryPhotoAssetProxy failed");
    MessageParcel data;
    photoProxy->WriteToParcel(data);
    photoProxy->CameraFreeBufferHandle();
    sptr<CameraServerPhotoProxy> cameraPhotoProxy = new CameraServerPhotoProxy();
    cameraPhotoProxy->ReadFromParcel(data);
    SetCameraPhotoProxyInfo(cameraPhotoProxy);
    int32_t captureId = cameraPhotoProxy->GetCaptureId();
    string pictureId = cameraPhotoProxy->GetTitle() + "." + cameraPhotoProxy->GetExtension();
    CameraReportDfxUtils::GetInstance()->SetPictureId(captureId, pictureId);
    MEDIA_DEBUG_LOG("HStreamCapture AddPhotoProxy E");
    photoAssetProxy->AddPhotoProxy(cameraPhotoProxy);
    MEDIA_DEBUG_LOG("HStreamCapture AddPhotoProxy X");
    EnableAddPhotoProxy(true);
    MEDIA_DEBUG_LOG("HStreamCapture UpdateMediaLibraryPhotoAssetProxy X captureId(%{public}d)", photoProxy->captureId_);
    return CAMERA_OK;
}
} // namespace CameraStandard
} // namespace OHOS
