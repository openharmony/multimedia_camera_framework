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

#include "session/capture_session.h"
#include "camera_util.h"
#include "hcapture_session_callback_stub.h"
#include "input/camera_input.h"
#include "camera_log.h"
#include "output/photo_output.h"
#include "output/preview_output.h"
#include "output/video_output.h"
#include "output/metadata_output.h"

namespace OHOS {
namespace CameraStandard {
namespace {
    constexpr int32_t DEFAULT_ITEMS = 10;
    constexpr int32_t DEFAULT_DATA_LENGTH = 100;
}

const std::unordered_map<camera_focus_state_t, FocusCallback::FocusState> CaptureSession::metaToFwFocusState_ = {
    {OHOS_CAMERA_FOCUS_STATE_SCAN, FocusCallback::SCAN},
    {OHOS_CAMERA_FOCUS_STATE_FOCUSED, FocusCallback::FOCUSED},
    {OHOS_CAMERA_FOCUS_STATE_UNFOCUSED, FocusCallback::UNFOCUSED}
};

const std::unordered_map<camera_exposure_state_t,
        ExposureCallback::ExposureState> CaptureSession::metaToFwExposureState_ = {
    {OHOS_CAMERA_EXPOSURE_STATE_SCAN, ExposureCallback::SCAN},
    {OHOS_CAMERA_EXPOSURE_STATE_CONVERGED, ExposureCallback::CONVERGED}
};

const std::unordered_map<camera_exposure_mode_enum_t, ExposureMode> CaptureSession::metaToFwExposureMode_ = {
    {OHOS_CAMERA_EXPOSURE_MODE_LOCKED, EXPOSURE_MODE_LOCKED},
    {OHOS_CAMERA_EXPOSURE_MODE_AUTO, EXPOSURE_MODE_AUTO},
    {OHOS_CAMERA_EXPOSURE_MODE_CONTINUOUS_AUTO, EXPOSURE_MODE_CONTINUOUS_AUTO}
};

const std::unordered_map<ExposureMode, camera_exposure_mode_enum_t> CaptureSession::fwToMetaExposureMode_ = {
    {EXPOSURE_MODE_LOCKED, OHOS_CAMERA_EXPOSURE_MODE_LOCKED},
    {EXPOSURE_MODE_AUTO, OHOS_CAMERA_EXPOSURE_MODE_AUTO},
    {EXPOSURE_MODE_CONTINUOUS_AUTO, OHOS_CAMERA_EXPOSURE_MODE_CONTINUOUS_AUTO}
};

const std::unordered_map<camera_focus_mode_enum_t, FocusMode> CaptureSession::metaToFwFocusMode_ = {
    {OHOS_CAMERA_FOCUS_MODE_MANUAL, FOCUS_MODE_MANUAL},
    {OHOS_CAMERA_FOCUS_MODE_CONTINUOUS_AUTO, FOCUS_MODE_CONTINUOUS_AUTO},
    {OHOS_CAMERA_FOCUS_MODE_AUTO, FOCUS_MODE_AUTO},
    {OHOS_CAMERA_FOCUS_MODE_LOCKED, FOCUS_MODE_LOCKED}
};

const std::unordered_map<FocusMode, camera_focus_mode_enum_t> CaptureSession::fwToMetaFocusMode_ = {
    {FOCUS_MODE_MANUAL, OHOS_CAMERA_FOCUS_MODE_MANUAL},
    {FOCUS_MODE_CONTINUOUS_AUTO, OHOS_CAMERA_FOCUS_MODE_CONTINUOUS_AUTO},
    {FOCUS_MODE_AUTO, OHOS_CAMERA_FOCUS_MODE_AUTO},
    {FOCUS_MODE_LOCKED, OHOS_CAMERA_FOCUS_MODE_LOCKED}
};

const std::unordered_map<camera_xmage_color_type_t, ColorEffect> CaptureSession::metaToFwColorEffect_ = {
    {CAMERA_CUSTOM_COLOR_NORMAL, COLOR_EFFECT_NORMAL},
    {CAMERA_CUSTOM_COLOR_BRIGHT, COLOR_EFFECT_BRIGHT},
    {CAMERA_CUSTOM_COLOR_SOFT, COLOR_EFFECT_SOFT}
};

const std::unordered_map<ColorEffect, camera_xmage_color_type_t> CaptureSession::fwToMetaColorEffect_ = {
    {COLOR_EFFECT_NORMAL, CAMERA_CUSTOM_COLOR_NORMAL},
    {COLOR_EFFECT_BRIGHT, CAMERA_CUSTOM_COLOR_BRIGHT},
    {COLOR_EFFECT_SOFT, CAMERA_CUSTOM_COLOR_SOFT}
};

const std::unordered_map<camera_flash_mode_enum_t, FlashMode> CaptureSession::metaToFwFlashMode_ = {
    {OHOS_CAMERA_FLASH_MODE_CLOSE, FLASH_MODE_CLOSE},
    {OHOS_CAMERA_FLASH_MODE_OPEN, FLASH_MODE_OPEN},
    {OHOS_CAMERA_FLASH_MODE_AUTO, FLASH_MODE_AUTO},
    {OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN, FLASH_MODE_ALWAYS_OPEN}
};

const std::unordered_map<FlashMode, camera_flash_mode_enum_t> CaptureSession::fwToMetaFlashMode_ = {
    {FLASH_MODE_CLOSE, OHOS_CAMERA_FLASH_MODE_CLOSE},
    {FLASH_MODE_OPEN, OHOS_CAMERA_FLASH_MODE_OPEN},
    {FLASH_MODE_AUTO, OHOS_CAMERA_FLASH_MODE_AUTO},
    {FLASH_MODE_ALWAYS_OPEN, OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN}
};

const std::unordered_map<camera_filter_type_t, FilterType> CaptureSession::metaToFwFilterType_ = {
    {OHOS_CAMERA_FILTER_TYPE_NONE, NONE},
    {OHOS_CAMERA_FILTER_TYPE_CLASSIC, CLASSIC},
    {OHOS_CAMERA_FILTER_TYPE_DAWN, DAWN},
    {OHOS_CAMERA_FILTER_TYPE_PURE, PURE},
    {OHOS_CAMERA_FILTER_TYPE_GREY, GREY},
    {OHOS_CAMERA_FILTER_TYPE_NATURAL, NATURAL},
    {OHOS_CAMERA_FILTER_TYPE_MORI, MORI},
    {OHOS_CAMERA_FILTER_TYPE_FAIR, FAIR},
    {OHOS_CAMERA_FILTER_TYPE_PINK, PINK}
};

const std::unordered_map<FilterType, camera_filter_type_t> CaptureSession::fwToMetaFilterType_ = {
    {NONE, OHOS_CAMERA_FILTER_TYPE_NONE},
    {CLASSIC, OHOS_CAMERA_FILTER_TYPE_CLASSIC},
    {DAWN, OHOS_CAMERA_FILTER_TYPE_DAWN},
    {PURE, OHOS_CAMERA_FILTER_TYPE_PURE},
    {GREY, OHOS_CAMERA_FILTER_TYPE_GREY},
    {NATURAL, OHOS_CAMERA_FILTER_TYPE_NATURAL},
    {MORI, OHOS_CAMERA_FILTER_TYPE_MORI},
    {FAIR, OHOS_CAMERA_FILTER_TYPE_FAIR},
    {PINK, OHOS_CAMERA_FILTER_TYPE_PINK}
};

const std::unordered_map<camera_beauty_type_t, BeautyType> CaptureSession::metaToFwBeautyType_ = {
    {OHOS_CAMERA_BEAUTY_TYPE_AUTO, AUTO_TYPE},
    {OHOS_CAMERA_BEAUTY_TYPE_SKIN_SMOOTH, SKIN_SMOOTH},
    {OHOS_CAMERA_BEAUTY_TYPE_FACE_SLENDER, FACE_SLENDER},
    {OHOS_CAMERA_BEAUTY_TYPE_SKIN_TONE, SKIN_TONE}
};

const std::unordered_map<BeautyType, camera_beauty_type_t> CaptureSession::fwToMetaBeautyType_ = {
    {AUTO_TYPE, OHOS_CAMERA_BEAUTY_TYPE_AUTO},
    {SKIN_SMOOTH, OHOS_CAMERA_BEAUTY_TYPE_SKIN_SMOOTH},
    {FACE_SLENDER, OHOS_CAMERA_BEAUTY_TYPE_FACE_SLENDER},
    {SKIN_TONE, OHOS_CAMERA_BEAUTY_TYPE_SKIN_TONE}
};

const std::unordered_map<BeautyType, camera_device_metadata_tag_t> CaptureSession::fwToMetaBeautyAbility_ = {
    {AUTO_TYPE, OHOS_ABILITY_BEAUTY_AUTO_VALUES},
    {SKIN_SMOOTH, OHOS_ABILITY_BEAUTY_SKIN_SMOOTH_VALUES},
    {FACE_SLENDER, OHOS_ABILITY_BEAUTY_FACE_SLENDER_VALUES},
    {SKIN_TONE, OHOS_ABILITY_BEAUTY_SKIN_TONE_VALUES}
};

const std::unordered_map<BeautyType, camera_device_metadata_tag_t> CaptureSession::fwToMetaBeautyControl_ = {
    {AUTO_TYPE, OHOS_CONTROL_BEAUTY_AUTO_VALUE},
    {SKIN_SMOOTH, OHOS_CONTROL_BEAUTY_SKIN_SMOOTH_VALUE},
    {FACE_SLENDER, OHOS_CONTROL_BEAUTY_FACE_SLENDER_VALUE},
    {SKIN_TONE, OHOS_CONTROL_BEAUTY_SKIN_TONE_VALUE}
};

const std::unordered_map<camera_device_metadata_tag_t, BeautyType> CaptureSession::metaToFwBeautyControl_ = {
    {OHOS_CONTROL_BEAUTY_SKIN_SMOOTH_VALUE, SKIN_SMOOTH},
    {OHOS_CONTROL_BEAUTY_FACE_SLENDER_VALUE, FACE_SLENDER},
    {OHOS_CONTROL_BEAUTY_SKIN_TONE_VALUE, SKIN_TONE}
};

const std::unordered_map<CameraVideoStabilizationMode,
VideoStabilizationMode> CaptureSession::metaToFwVideoStabModes_ = {
    {OHOS_CAMERA_VIDEO_STABILIZATION_OFF, OFF},
    {OHOS_CAMERA_VIDEO_STABILIZATION_LOW, LOW},
    {OHOS_CAMERA_VIDEO_STABILIZATION_MIDDLE, MIDDLE},
    {OHOS_CAMERA_VIDEO_STABILIZATION_HIGH, HIGH},
    {OHOS_CAMERA_VIDEO_STABILIZATION_AUTO, AUTO}
};

const std::unordered_map<VideoStabilizationMode,
CameraVideoStabilizationMode> CaptureSession::fwToMetaVideoStabModes_ = {
    {OFF, OHOS_CAMERA_VIDEO_STABILIZATION_OFF},
    {LOW, OHOS_CAMERA_VIDEO_STABILIZATION_LOW},
    {MIDDLE, OHOS_CAMERA_VIDEO_STABILIZATION_MIDDLE},
    {HIGH, OHOS_CAMERA_VIDEO_STABILIZATION_HIGH},
    {AUTO, OHOS_CAMERA_VIDEO_STABILIZATION_AUTO}
};

int32_t CaptureSessionCallback::OnError(int32_t errorCode)
{
    MEDIA_INFO_LOG("CaptureSessionCallback::OnError() is called!, errorCode: %{public}d",
                   errorCode);
    if (captureSession_ != nullptr && captureSession_->GetApplicationCallback() != nullptr) {
        captureSession_->GetApplicationCallback()->OnError(errorCode);
    } else {
        MEDIA_INFO_LOG("CaptureSessionCallback::ApplicationCallback not set!, Discarding callback");
    }
    return CameraErrorCode::SUCCESS;
}

CaptureSession::CaptureSession(sptr<ICaptureSession> &captureSession)
{
    captureSession_ = captureSession;
    inputDevice_ = nullptr;
    modeName_ = 0;
    metaOutput_ = nullptr;
}

CaptureSession::~CaptureSession()
{
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::~CaptureSession()");
    inputDevice_ = nullptr;
    captureSession_ = nullptr;
    changedMetadata_ = nullptr;
    appCallback_ = nullptr;
    captureSessionCallback_ = nullptr;
    exposureCallback_ = nullptr;
    focusCallback_ = nullptr;
}

int32_t CaptureSession::BeginConfig()
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::BeginConfig");
    if (IsSessionConfiged()) {
        MEDIA_ERR_LOG("CaptureSession::BeginConfig Session is locked");
        return CameraErrorCode::SESSION_CONFIG_LOCKED;
    }
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->BeginConfig();
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to BeginConfig!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::BeginConfig() captureSession_ is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::CommitConfig()
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::CommitConfig");
    if (!IsSessionConfiged()) {
        MEDIA_ERR_LOG("CaptureSession::CommitConfig operation Not allowed!");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->CommitConfig();
        MEDIA_INFO_LOG("CaptureSession::CommitConfig commit mode = %{public}d", GetMode());
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to CommitConfig!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::CommitConfig() captureSession_ is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::CanAddInput(sptr<CaptureInput> &input)
{
    // todo: get Profile passed to createOutput and compare with OutputCapability
    // if present in capability return ok.
    return CameraErrorCode::SUCCESS;
}

int32_t CaptureSession::AddInput(sptr<CaptureInput> &input)
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::AddInput");
    if (!IsSessionConfiged()) {
        MEDIA_ERR_LOG("CaptureSession::AddInput operation Not allowed!");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    if (input == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::AddInput input is null");
        return ServiceToCameraError(CAMERA_INVALID_ARG);
    }
    input->SetSession(this);
    inputDevice_ = input;
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->AddInput(((sptr<CameraInput> &)input)->GetCameraDevice());
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to AddInput!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::AddInput() captureSession_ is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::CanAddOutput(sptr<CaptureOutput> &output)
{
    // todo: get Profile passed to createOutput and compare with OutputCapability
    // if present in capability return ok.
    return CameraErrorCode::SUCCESS;
}

sptr<CaptureOutput> CaptureSession::GetMetaOutput()
{
    MEDIA_DEBUG_LOG("CaptureSession::GetMetadataOutput metaOuput(%{public}d)", metaOutput_ != nullptr);
    return metaOutput_;
}

void CaptureSession::ConfigureOutput(sptr<CaptureOutput> &output)
{
    const int32_t normalMode = 0;
    const int32_t captureMode = 1;
    const int32_t videoMode = 2;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::AddOutput");
    if (output->GetOutputType() == CAPTURE_OUTPUT_TYPE_PREVIEW) {
        MEDIA_INFO_LOG("CaptureSession::AddOutput PreviewOutput");
        previewProfile_ = output->GetPreviewProfile();
        if (GetMode() == normalMode) {
            SetMode(videoMode);
        }
    }
    if (output->GetOutputType() == CAPTURE_OUTPUT_TYPE_PHOTO) {
        MEDIA_INFO_LOG("CaptureSession::AddOutput PhotoOutput");
        photoProfile_ = output->GetPhotoProfile();
        if (GetMode() == normalMode) {
            SetMode(captureMode);
        }
    }
    output->SetSession(this);
    if (output->GetOutputType() == CAPTURE_OUTPUT_TYPE_VIDEO) {
        MEDIA_INFO_LOG("CaptureSession::AddOutput VideoOutput");
        SetFrameRateRange(static_cast<VideoOutput *>(output.GetRefPtr())->GetFrameRateRange());
        if (GetMode() == normalMode) {
            SetMode(videoMode);
        }
    }
}

int32_t CaptureSession::AddOutput(sptr<CaptureOutput> &output)
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::AddOutput");
    if (!IsSessionConfiged()) {
        MEDIA_ERR_LOG("CaptureSession::AddOutput operation Not allowed!");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    if (output == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::AddOutput output is null");
        return ServiceToCameraError(CAMERA_INVALID_ARG);
    }
    ConfigureOutput(output);
    if (output->GetOutputType() == CAPTURE_OUTPUT_TYPE_METADATA) {
        MEDIA_INFO_LOG("CaptureSession::AddOutput MetadataOutput");
        const int32_t portraitMode = 3;
        if (GetMode() == portraitMode) {
            metaOutput_ = output;
            return ServiceToCameraError(CAMERA_OK);
        }
    }
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->AddOutput(output->GetStreamType(), output->GetStream());
        MEDIA_INFO_LOG("CaptureSession::AddOutput StreamType = %{public}d", output->GetStreamType());
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to AddOutput!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::AddOutput() captureSession_ is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::RemoveInput(sptr<CaptureInput> &input)
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::RemoveInput");
    if (!IsSessionConfiged()) {
        MEDIA_ERR_LOG("CaptureSession::RemoveInput operation Not allowed!");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    if (input == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::RemoveInput input is null");
        return ServiceToCameraError(CAMERA_INVALID_ARG);
    }
    if (inputDevice_ != nullptr) {
        inputDevice_ = nullptr;
    }
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->RemoveInput(((sptr<CameraInput> &)input)->GetCameraDevice());
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to RemoveInput!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::RemoveInput() captureSession_ is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::RemoveOutput(sptr<CaptureOutput> &output)
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::RemoveOutput");
    if (!IsSessionConfiged()) {
        MEDIA_ERR_LOG("CaptureSession::RemoveOutput operation Not allowed!");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    if (output == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::RemoveOutput output is null");
        return ServiceToCameraError(CAMERA_INVALID_ARG);
    }
    output->SetSession(nullptr);
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->RemoveOutput(output->GetStreamType(), output->GetStream());
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to RemoveOutput!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::RemoveOutput() captureSession_ is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::Start()
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::Start");
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::Start Session not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->Start();
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to Start capture session!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::Start() captureSession_ is nullptr");
    }
    if (GetMetaOutput()) {
        sptr<MetadataOutput> metaOutput = static_cast<MetadataOutput *>(GetMetaOutput().GetRefPtr());
        if (!metaOutput) {
            MEDIA_INFO_LOG("CaptureSession::metaOutput is null");
            return ServiceToCameraError(errCode);
        }
        std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();
        MEDIA_INFO_LOG("CaptureSession::Start SetCapturingMetadataObjectTypes objectTypes size = %{public}zu",
                       metadataObjectTypes.size());
        metaOutput->SetCapturingMetadataObjectTypes(metadataObjectTypes);
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::Stop()
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::Stop");
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->Stop();
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to Stop capture session!, %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("CaptureSession::Stop() captureSession_ is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t CaptureSession::Release()
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter Into CaptureSession::Release");
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (captureSession_) {
        errCode = captureSession_->Release(0);
        MEDIA_DEBUG_LOG("Release capture session, %{public}d", errCode);
    } else {
        MEDIA_ERR_LOG("CaptureSession::Release() captureSession_ is nullptr");
    }
    inputDevice_ = nullptr;
    captureSession_ = nullptr;
    captureSessionCallback_ = nullptr;
    changedMetadata_ = nullptr;
    appCallback_ = nullptr;
    exposureCallback_ = nullptr;
    focusCallback_ = nullptr;
    return ServiceToCameraError(errCode);
}

void CaptureSession::SetCallback(std::shared_ptr<SessionCallback> callback)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetCallback: Unregistering application callback!");
    }
    int32_t errorCode = CAMERA_OK;

    appCallback_ = callback;
    if (appCallback_ != nullptr && captureSession_ != nullptr) {
        if (captureSessionCallback_ == nullptr) {
            captureSessionCallback_ = new(std::nothrow) CaptureSessionCallback(this);
        }
        if (captureSession_) {
            errorCode = captureSession_->SetCallback(captureSessionCallback_);
            if (errorCode != CAMERA_OK) {
                MEDIA_ERR_LOG("CaptureSession::SetCallback: Failed to register callback, errorCode: %{public}d",
                    errorCode);
                captureSessionCallback_ = nullptr;
                appCallback_ = nullptr;
            }
        }
    }
    return;
}

std::shared_ptr<SessionCallback> CaptureSession::GetApplicationCallback()
{
    return appCallback_;
}

int32_t CaptureSession::UpdateSetting(std::shared_ptr<Camera::CameraMetadata> changedMetadata)
{
    CAMERA_SYNC_TRACE;
    if (!Camera::GetCameraMetadataItemCount(changedMetadata->get())) {
        MEDIA_INFO_LOG("CaptureSession::UpdateSetting No configuration to update");
        return CameraErrorCode::SUCCESS;
    }

    if (!inputDevice_ || !((sptr<CameraInput> &)inputDevice_)->GetCameraDevice()) {
        MEDIA_ERR_LOG("CaptureSession::UpdateSetting Failed inputDevice_ is nullptr");
        return CameraErrorCode::SUCCESS;
    }
    int32_t ret = ((sptr<CameraInput> &)inputDevice_)->GetCameraDevice()->UpdateSetting(changedMetadata);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("CaptureSession::UpdateSetting Failed to update settings, errCode = %{public}d", ret);
        return ServiceToCameraError(ret);
    }

    uint32_t count = changedMetadata->get()->item_count;
    uint8_t* data = Camera::GetMetadataData(changedMetadata->get());
    camera_metadata_item_entry_t* itemEntry = Camera::GetMetadataItems(changedMetadata->get());
    std::shared_ptr<Camera::CameraMetadata> baseMetadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    for (uint32_t i = 0; i < count; i++, itemEntry++) {
        bool status = false;
        camera_metadata_item_t item;
        size_t length = Camera::CalculateCameraMetadataItemDataSize(itemEntry->data_type, itemEntry->count);
        ret = Camera::FindCameraMetadataItem(baseMetadata->get(), itemEntry->item, &item);
        if (ret == CAM_META_SUCCESS) {
            status = baseMetadata->updateEntry(itemEntry->item,
                                               (length == 0) ? itemEntry->data.value : (data + itemEntry->data.offset),
                                               itemEntry->count);
        } else if (ret == CAM_META_ITEM_NOT_FOUND) {
            status = baseMetadata->addEntry(itemEntry->item,
                                            (length == 0) ? itemEntry->data.value : (data + itemEntry->data.offset),
                                            itemEntry->count);
        }
        if (!status) {
            MEDIA_ERR_LOG("CaptureSession::UpdateSetting Failed to add/update metadata item: %{public}d",
                          itemEntry->item);
        }
    }
    return CameraErrorCode::SUCCESS;
}

void CaptureSession::LockForControl()
{
    changeMetaMutex_.lock();
    MEDIA_DEBUG_LOG("CaptureSession::LockForControl Called");
    changedMetadata_ = std::make_shared<Camera::CameraMetadata>(DEFAULT_ITEMS, DEFAULT_DATA_LENGTH);
}

int32_t CaptureSession::UnlockForControl()
{
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::UnlockForControl Need to call LockForControl() before UnlockForControl()");
        return ServiceToCameraError(CAMERA_INVALID_ARG);
    }
    MEDIA_DEBUG_LOG("CaptureSession::UnlockForControl Called");
    UpdateSetting(changedMetadata_);
    changedMetadata_ = nullptr;
    changeMetaMutex_.unlock();
    return CameraErrorCode::SUCCESS;
}

VideoStabilizationMode CaptureSession::GetActiveVideoStabilizationMode()
{
    sptr<CameraDevice> cameraObj_;
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetActiveVideoStabilizationMode camera device is null");
        return OFF;
    }
    cameraObj_ = inputDevice_->GetCameraDeviceInfo();
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_VIDEO_STABILIZATION_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        auto itr = metaToFwVideoStabModes_.find(static_cast<CameraVideoStabilizationMode>(item.data.u8[0]));
        if (itr != metaToFwVideoStabModes_.end()) {
            return itr->second;
        }
    }
    return OFF;
}

int32_t CaptureSession::GetActiveVideoStabilizationMode(VideoStabilizationMode &mode)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetActiveVideoStabilizationMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetActiveVideoStabilizationMode camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    mode = OFF;
    bool isSupported = false;
    sptr<CameraDevice> cameraObj_;
    cameraObj_ = inputDevice_->GetCameraDeviceInfo();
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_VIDEO_STABILIZATION_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        auto itr = metaToFwVideoStabModes_.find(static_cast<CameraVideoStabilizationMode>(item.data.u8[0]));
        if (itr != metaToFwVideoStabModes_.end()) {
            mode = itr->second;
            isSupported = true;
        }
    }
    if (!isSupported || ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetActiveVideoStabilizationMode Failed with return code %{public}d", ret);
    }
    return CameraErrorCode::SUCCESS;
}

int32_t CaptureSession::SetVideoStabilizationMode(VideoStabilizationMode stabilizationMode)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetVideoStabilizationMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    auto itr = fwToMetaVideoStabModes_.find(stabilizationMode);
    if ((itr == fwToMetaVideoStabModes_.end()) || !IsVideoStabilizationModeSupported(stabilizationMode)) {
        MEDIA_ERR_LOG("CaptureSession::SetVideoStabilizationMode Mode: %{public}d not supported", stabilizationMode);
        stabilizationMode = OFF;
    }

    uint32_t count = 1;
    uint8_t stabilizationMode_ = stabilizationMode;

    this->LockForControl();
    MEDIA_DEBUG_LOG("CaptureSession::SetVideoStabilizingMode StabilizationMode : %{public}d", stabilizationMode_);
    if (!(this->changedMetadata_->addEntry(OHOS_CONTROL_VIDEO_STABILIZATION_MODE, &stabilizationMode_, count))) {
        MEDIA_DEBUG_LOG("CaptureSession::SetVideoStabilizingMode Failed to set video stabilization mode");
    }

    int32_t errCode = this->UnlockForControl();
    if (errCode != CameraErrorCode::SUCCESS) {
        MEDIA_DEBUG_LOG("CaptureSession::SetVideoStabilizingMode Failed to set video stabilization mode");
    }
    return CameraErrorCode::SUCCESS;
}

bool CaptureSession::IsVideoStabilizationModeSupported(VideoStabilizationMode stabilizationMode)
{
    std::vector<VideoStabilizationMode> stabilizationModes = GetSupportedStabilizationMode();
    if (std::find(stabilizationModes.begin(), stabilizationModes.end(), stabilizationMode)
       != stabilizationModes.end()) {
        return true;
    }
    return false;
}

int32_t CaptureSession::IsVideoStabilizationModeSupported(VideoStabilizationMode stabilizationMode, bool &isSupported)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::IsVideoStabilizationModeSupported Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    std::vector<VideoStabilizationMode> stabilizationModes = GetSupportedStabilizationMode();
    if (std::find(stabilizationModes.begin(), stabilizationModes.end(), stabilizationMode)
       != stabilizationModes.end()) {
        isSupported = true;
        return CameraErrorCode::SUCCESS;
    }
    isSupported = false;
    return CameraErrorCode::SUCCESS;
}

std::vector<VideoStabilizationMode> CaptureSession::GetSupportedStabilizationMode()
{
    std::vector<VideoStabilizationMode> stabilizationModes;

    sptr<CameraDevice> cameraObj_;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedStabilizationMode Session is not Commited");
        return stabilizationModes;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedStabilizationMode camera device is null");
        return stabilizationModes;
    }
    cameraObj_ = inputDevice_->GetCameraDeviceInfo();
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_VIDEO_STABILIZATION_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupporteStabilizationModes Failed with return code %{public}d", ret);
        return stabilizationModes;
    }

    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwVideoStabModes_.find(static_cast<CameraVideoStabilizationMode>(item.data.u8[i]));
        if (itr != metaToFwVideoStabModes_.end()) {
            stabilizationModes.emplace_back(itr->second);
        }
    }
    return stabilizationModes;
}

int32_t CaptureSession::GetSupportedStabilizationMode(std::vector<VideoStabilizationMode> &stabilizationModes)
{
    sptr<CameraDevice> cameraObj_;
    stabilizationModes.clear();
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedStabilizationMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedStabilizationMode camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    cameraObj_ = inputDevice_->GetCameraDeviceInfo();
    std::shared_ptr<Camera::CameraMetadata> metadata = cameraObj_->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_VIDEO_STABILIZATION_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupporteStabilizationModes Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }

    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwVideoStabModes_.find(static_cast<CameraVideoStabilizationMode>(item.data.u8[i]));
        if (itr != metaToFwVideoStabModes_.end()) {
            stabilizationModes.emplace_back(itr->second);
        }
    }
    return CameraErrorCode::SUCCESS;
}

bool CaptureSession::IsExposureModeSupported(ExposureMode exposureMode)
{
    std::vector<ExposureMode> vecSupportedExposureModeList;
    vecSupportedExposureModeList = this->GetSupportedExposureModes();
    if (find(vecSupportedExposureModeList.begin(), vecSupportedExposureModeList.end(),
        exposureMode) != vecSupportedExposureModeList.end()) {
        return true;
    }

    return false;
}

int32_t CaptureSession::IsExposureModeSupported(ExposureMode exposureMode, bool &isSupported)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::IsExposureModeSupported Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    std::vector<ExposureMode> vecSupportedExposureModeList;
    vecSupportedExposureModeList = this->GetSupportedExposureModes();
    if (find(vecSupportedExposureModeList.begin(), vecSupportedExposureModeList.end(),
        exposureMode) != vecSupportedExposureModeList.end()) {
        isSupported = true;
        return CameraErrorCode::SUCCESS;
    }
    isSupported = false;
    return CameraErrorCode::SUCCESS;
}

std::vector<ExposureMode> CaptureSession::GetSupportedExposureModes()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedExposureModes Session is not Commited");
        return {};
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedExposureModes camera device is null");
        return {};
    }
    std::vector<ExposureMode> supportedExposureModes;
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_EXPOSURE_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedExposureModes Failed with return code %{public}d", ret);
        return supportedExposureModes;
    }

    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwExposureMode_.find(static_cast<camera_exposure_mode_enum_t>(item.data.u8[i]));
        if (itr != metaToFwExposureMode_.end()) {
            supportedExposureModes.emplace_back(itr->second);
        }
    }
    return supportedExposureModes;
}

int32_t CaptureSession::GetSupportedExposureModes(std::vector<ExposureMode> &supportedExposureModes)
{
    supportedExposureModes.clear();
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedExposureModes Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedExposureModes camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_EXPOSURE_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedExposureModes Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }

    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwExposureMode_.find(static_cast<camera_exposure_mode_enum_t>(item.data.u8[i]));
        if (itr != metaToFwExposureMode_.end()) {
            supportedExposureModes.emplace_back(itr->second);
        }
    }
    return CameraErrorCode::SUCCESS;
}

int32_t CaptureSession::SetExposureMode(ExposureMode exposureMode)
{
    CAMERA_SYNC_TRACE;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }

    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureMode Need to call LockForControl() "
                      "before setting camera properties");
        return CameraErrorCode::SUCCESS;
    }
    uint8_t exposure = fwToMetaExposureMode_.at(EXPOSURE_MODE_LOCKED);
    auto itr = fwToMetaExposureMode_.find(exposureMode);
    if (itr == fwToMetaExposureMode_.end()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureMode Unknown exposure mode");
    } else {
        exposure = itr->second;
    }

    bool status = false;
    uint32_t count = 1;
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_EXPOSURE_MODE, &exposure, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_EXPOSURE_MODE, &exposure, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureMode Failed to set exposure mode");
    }

    return CameraErrorCode::SUCCESS;
}

ExposureMode CaptureSession::GetExposureMode()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureMode Session is not Commited");
        return EXPOSURE_MODE_UNSUPPORTED;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureMode camera device is null");
        return EXPOSURE_MODE_UNSUPPORTED;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureMode Failed with return code %{public}d", ret);
        return EXPOSURE_MODE_UNSUPPORTED;
    }
    auto itr = metaToFwExposureMode_.find(static_cast<camera_exposure_mode_enum_t>(item.data.u8[0]));
    if (itr != metaToFwExposureMode_.end()) {
        return itr->second;
    }

    return EXPOSURE_MODE_UNSUPPORTED;
}

int32_t CaptureSession::GetExposureMode(ExposureMode &exposureMode)
{
    exposureMode = EXPOSURE_MODE_UNSUPPORTED;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureMode camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureMode Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    auto itr = metaToFwExposureMode_.find(static_cast<camera_exposure_mode_enum_t>(item.data.u8[0]));
    if (itr != metaToFwExposureMode_.end()) {
        exposureMode = itr->second;
        return CameraErrorCode::SUCCESS;
    }
    return CameraErrorCode::SUCCESS;
}


int32_t CaptureSession::SetMeteringPoint(Point exposurePoint)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetMeteringPoint Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }

    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetExposurePoint Need to call LockForControl() "
            "before setting camera properties");
        return CameraErrorCode::SUCCESS;
    }
    Point exposureVerifyPoint = VerifyFocusCorrectness(exposurePoint);
    Point unifyExposurePoint = CoordinateTransform(exposureVerifyPoint);
    bool status = false;
    float exposureArea[2] = {unifyExposurePoint.x, unifyExposurePoint.y};
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AE_REGIONS, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AE_REGIONS, exposureArea,
            sizeof(exposureArea) / sizeof(exposureArea[0]));
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AE_REGIONS, exposureArea,
            sizeof(exposureArea) / sizeof(exposureArea[0]));
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetExposurePoint Failed to set exposure Area");
    }
    return CameraErrorCode::SUCCESS;
}

Point CaptureSession::GetMeteringPoint()
{
    Point exposurePoint = {0, 0};
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetMeteringPoint Session is not Commited");
        return exposurePoint;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetMeteringPoint camera device is null");
        return exposurePoint;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_REGIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposurePoint Failed with return code %{public}d", ret);
        return exposurePoint;
    }
    exposurePoint.x = item.data.f[0];
    exposurePoint.y = item.data.f[1];
    Point unifyExposurePoint = CoordinateTransform(exposurePoint);
    return unifyExposurePoint;
}

int32_t CaptureSession::GetMeteringPoint(Point &exposurePoint)
{
    exposurePoint.x = 0;
    exposurePoint.y = 0;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetMeteringPoint Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetMeteringPoint camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_REGIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposurePoint Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    exposurePoint.x = item.data.f[0];
    exposurePoint.y = item.data.f[1];
    exposurePoint = CoordinateTransform(exposurePoint);
    return CameraErrorCode::SUCCESS;
}

std::vector<float> CaptureSession::GetExposureBiasRange()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureBiasRange Session is not Commited");
        return {};
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureBiasRange camera device is null");
        return {};
    }
    return inputDevice_->GetCameraDeviceInfo()->GetExposureBiasRange();
}

int32_t CaptureSession::GetExposureBiasRange(std::vector<float> &exposureBiasRange)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureBiasRange Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureBiasRange camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    exposureBiasRange = inputDevice_->GetCameraDeviceInfo()->GetExposureBiasRange();
    return CameraErrorCode::SUCCESS;
}


int32_t CaptureSession::SetExposureBias(float exposureValue)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureBias Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureValue Need to call LockForControl() "
            "before setting camera properties");
        return CameraErrorCode::SUCCESS;
    }
    bool status = false;
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    int32_t count = 1;
    camera_metadata_item_t item;
    MEDIA_DEBUG_LOG("CaptureSession::SetExposureValue exposure compensation: %{public}f", exposureValue);
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureBias camera device is null");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    std::vector<float> biasRange = inputDevice_->GetCameraDeviceInfo()->GetExposureBiasRange();
    if (biasRange.empty()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureValue Bias range is empty");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    if (exposureValue < biasRange[minIndex]) {
        MEDIA_DEBUG_LOG("CaptureSession::SetExposureValue bias value:"
                        "%{public}f is lesser than minimum bias: %{public}f", exposureValue, biasRange[minIndex]);
        exposureValue = biasRange[minIndex];
    } else if (exposureValue > biasRange[maxIndex]) {
        MEDIA_DEBUG_LOG("CaptureSession::SetExposureValue bias value: "
                        "%{public}f is greater than maximum bias: %{public}f", exposureValue, biasRange[maxIndex]);
        exposureValue = biasRange[maxIndex];
    }
    if (std::abs(exposureValue) <= 1e-6) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureValue exposure compensation value no need to change");
        return CameraErrorCode::SUCCESS;
    }

    int32_t exposureCompensation = CalculateExposureValue(exposureValue);

    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &exposureCompensation, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &exposureCompensation, count);
    }
    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureValue Failed to set exposure compensation");
    }
    return CameraErrorCode::SUCCESS;
}

float CaptureSession::GetExposureValue()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue Session is not Commited");
        return 0;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue camera device is null");
        return 0;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue Failed with return code %{public}d", ret);
        return 0;
    }
    int32_t exposureCompensation = item.data.i32[0];

    ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_COMPENSATION_STEP, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue Failed with return code %{public}d", ret);
        return 0;
    }
    int32_t stepNumerator = item.data.r->numerator;
    int32_t stepDenominator = item.data.r->denominator;
    float exposureValue = 0;
    if (stepDenominator != 0) {
        float step = static_cast<float>(stepNumerator) / static_cast<float>(stepDenominator);
        exposureValue = step * exposureCompensation;
    } else {
        MEDIA_ERR_LOG("stepDenominator: %{public}d", stepDenominator);
    }
    MEDIA_DEBUG_LOG("exposureValue: %{public}f", exposureValue);

    return exposureValue;
}

int32_t CaptureSession::GetExposureValue(float &exposureValue)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    int32_t exposureCompensation = item.data.i32[0];

    ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_COMPENSATION_STEP, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetExposureValue Failed with return code %{public}d", ret);
        return 0;
    }
    int32_t stepNumerator = item.data.r->numerator;
    int32_t stepDenominator = item.data.r->denominator;
    float step = static_cast<float>(stepNumerator) / static_cast<float>(stepDenominator);

    exposureValue = step * exposureCompensation;
    MEDIA_DEBUG_LOG("exposureValue: %{public}f", exposureValue);
    return CameraErrorCode::SUCCESS;
}

void CaptureSession::SetExposureCallback(std::shared_ptr<ExposureCallback> exposureCallback)
{
    exposureCallback_ = exposureCallback;
}

void CaptureSession::ProcessAutoExposureUpdates(const std::shared_ptr<Camera::CameraMetadata> &result)
{
    camera_metadata_item_t item;
    common_metadata_header_t* metadata = result->get();

    int ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_EXPOSURE_MODE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_DEBUG_LOG("exposure mode: %{public}d", item.data.u8[0]);
    }

    ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_EXPOSURE_STATE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_INFO_LOG("Exposure state: %{public}d", item.data.u8[0]);
        if (exposureCallback_ != nullptr) {
            auto itr = metaToFwExposureState_.find(static_cast<camera_exposure_state_t>(item.data.u8[0]));
            if (itr != metaToFwExposureState_.end()) {
                exposureCallback_->OnExposureState(itr->second);
            }
        }
    }
}

std::vector<FocusMode> CaptureSession::GetSupportedFocusModes()
{
    std::vector<FocusMode> supportedFocusModes = {};
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureBias Session is not Commited");
        return supportedFocusModes;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFocusModes camera device is null");
        return supportedFocusModes;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FOCUS_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFocusModes Failed with return code %{public}d", ret);
        return supportedFocusModes;
    }
    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwFocusMode_.find(static_cast<camera_focus_mode_enum_t>(item.data.u8[i]));
        if (itr != metaToFwFocusMode_.end()) {
            supportedFocusModes.emplace_back(itr->second);
        }
    }
    return supportedFocusModes;
}

int32_t CaptureSession::GetSupportedFocusModes(std::vector<FocusMode> &supportedFocusModes)
{
    supportedFocusModes.clear();
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureBias Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFocusModes camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FOCUS_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFocusModes Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwFocusMode_.find(static_cast<camera_focus_mode_enum_t>(item.data.u8[i]));
        if (itr != metaToFwFocusMode_.end()) {
            supportedFocusModes.emplace_back(itr->second);
            return CameraErrorCode::SUCCESS;
        }
    }
    return CameraErrorCode::SUCCESS;
}

void CaptureSession::SetFocusCallback(std::shared_ptr<FocusCallback> focusCallback)
{
    focusCallback_ = focusCallback;
    return;
}

bool CaptureSession::IsFocusModeSupported(FocusMode focusMode)
{
    std::vector<FocusMode> vecSupportedFocusModeList;
    vecSupportedFocusModeList = this->GetSupportedFocusModes();
    if (find(vecSupportedFocusModeList.begin(), vecSupportedFocusModeList.end(),
        focusMode) != vecSupportedFocusModeList.end()) {
        return true;
    }

    return false;
}

int32_t CaptureSession::IsFocusModeSupported(FocusMode focusMode, bool &isSupported)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureBias Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    std::vector<FocusMode> vecSupportedFocusModeList;
    vecSupportedFocusModeList = this->GetSupportedFocusModes();
    if (find(vecSupportedFocusModeList.begin(), vecSupportedFocusModeList.end(),
        focusMode) != vecSupportedFocusModeList.end()) {
        isSupported = true;
        return CameraErrorCode::SUCCESS;
    }
    isSupported = false;
    return CameraErrorCode::SUCCESS;
}

int32_t CaptureSession::SetFocusMode(FocusMode focusMode)
{
    CAMERA_SYNC_TRACE;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetFocusMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetFocusMode Need to call LockForControl() before setting camera properties");
        return CameraErrorCode::SUCCESS;
    }
    uint8_t focus = FOCUS_MODE_LOCKED;
    auto itr = fwToMetaFocusMode_.find(focusMode);
    if (itr == fwToMetaFocusMode_.end()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureMode Unknown exposure mode");
    } else {
        focus = itr->second;
    }
    bool status = false;
    int32_t ret;
    uint32_t count = 1;
    camera_metadata_item_t item;

    MEDIA_DEBUG_LOG("CaptureSession::SetFocusMode Focus mode: %{public}d", focusMode);

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_FOCUS_MODE, &focus, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_FOCUS_MODE, &focus, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetFocusMode Failed to set focus mode");
    }
    return CameraErrorCode::SUCCESS;
}

FocusMode CaptureSession::GetFocusMode()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusMode Session is not Commited");
        return FOCUS_MODE_MANUAL;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusMode camera device is null");
        return FOCUS_MODE_MANUAL;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusMode Failed with return code %{public}d", ret);
        return FOCUS_MODE_MANUAL;
    }
    auto itr = metaToFwFocusMode_.find(static_cast<camera_focus_mode_enum_t>(item.data.u8[0]));
    if (itr != metaToFwFocusMode_.end()) {
        return itr->second;
    }
    return FOCUS_MODE_MANUAL;
}

int32_t CaptureSession::GetFocusMode(FocusMode &focusMode)
{
    focusMode = FOCUS_MODE_MANUAL;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusMode camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusMode Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    auto itr = metaToFwFocusMode_.find(static_cast<camera_focus_mode_enum_t>(item.data.u8[0]));
    if (itr != metaToFwFocusMode_.end()) {
        focusMode = itr->second;
        return CameraErrorCode::SUCCESS;
    }
    return CameraErrorCode::SUCCESS;
}

int32_t CaptureSession::SetFocusPoint(Point focusPoint)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetFocusPoint Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetFocusPoint Need to call LockForControl() before setting camera properties");
        return CameraErrorCode::SUCCESS;
    }
    Point focusVerifyPoint = VerifyFocusCorrectness(focusPoint);
    Point unifyFocusPoint = CoordinateTransform(focusVerifyPoint);
    bool status = false;
    float FocusArea[2] = {unifyFocusPoint.x, unifyFocusPoint.y};
    camera_metadata_item_t item;

    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_AF_REGIONS, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_AF_REGIONS, FocusArea,
            sizeof(FocusArea) / sizeof(FocusArea[0]));
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_AF_REGIONS, FocusArea,
            sizeof(FocusArea) / sizeof(FocusArea[0]));
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetFocusPoint Failed to set Focus Area");
    }
    return CameraErrorCode::SUCCESS;
}

Point CaptureSession::CoordinateTransform(Point point)
{
    MEDIA_DEBUG_LOG("CaptureSession::CoordinateTransform begin x: %{public}f, y: %{public}f", point.x, point.y);
    Point unifyPoint = point;
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::CoordinateTransform cameraInput is nullptr");
        return unifyPoint;
    }
    if (inputDevice_->GetCameraDeviceInfo()->GetPosition() == CAMERA_POSITION_FRONT) {
        unifyPoint.x = 1 - unifyPoint.x; // flip horizontally
    }
    MEDIA_DEBUG_LOG("CaptureSession::CoordinateTransform end x: %{public}f, y: %{public}f",
                    unifyPoint.x, unifyPoint.y);
    return unifyPoint;
}

Point CaptureSession::VerifyFocusCorrectness(Point point)
{
    MEDIA_DEBUG_LOG("CaptureSession::VerifyFocusCorrectness begin x: %{public}f, y: %{public}f", point.x, point.y);
    float minPoint = 0.0000001;
    float maxPoint = 1;
    Point VerifyPoint = point;
    if (VerifyPoint.x >= -minPoint && VerifyPoint.x <= minPoint) {
        VerifyPoint.x = minPoint;
    } else if (VerifyPoint.x > maxPoint) {
        VerifyPoint.x = maxPoint;
    }
    if (VerifyPoint.y >= -minPoint && VerifyPoint.y <= minPoint) {
        VerifyPoint.y = minPoint;
    } else if (VerifyPoint.y > maxPoint) {
        VerifyPoint.y = maxPoint;
    }
    MEDIA_DEBUG_LOG("CaptureSession::VerifyFocusCorrectness end x: %{public}f, y: %{public}f",
                    VerifyPoint.x, VerifyPoint.y);
    return VerifyPoint;
}

Point CaptureSession::GetFocusPoint()
{
    Point focusPoint = {0, 0};
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusPoint Session is not Commited");
        return focusPoint;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusPoint camera device is null");
        return focusPoint;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AF_REGIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusPoint Failed with return code %{public}d", ret);
        return focusPoint;
    }
    focusPoint.x = item.data.f[0];
    focusPoint.y = item.data.f[1];
    Point unifyFocusPoint = CoordinateTransform(focusPoint);
    return unifyFocusPoint;
}

int32_t CaptureSession::GetFocusPoint(Point &focusPoint)
{
    focusPoint.x = 0;
    focusPoint.y = 0;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusPoint Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusPoint camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AF_REGIONS, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFocusPoint Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    focusPoint.x = item.data.f[0];
    focusPoint.y = item.data.f[1];
    focusPoint = CoordinateTransform(focusPoint);
    return CameraErrorCode::SUCCESS;
}

float CaptureSession::GetFocalLength()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocalLength Session is not Commited");
        return 0;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocalLength camera device is null");
        return 0;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FOCAL_LENGTH, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFocalLength Failed with return code %{public}d", ret);
        return 0;
    }
    return static_cast<float>(item.data.f[0]);
}

int32_t CaptureSession::GetFocalLength(float &focalLength)
{
    focalLength = 0;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocalLength Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFocalLength camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FOCAL_LENGTH, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFocalLength Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    focalLength = static_cast<float>(item.data.f[0]);
    return CameraErrorCode::SUCCESS;
}

void CaptureSession::ProcessAutoFocusUpdates(const std::shared_ptr<Camera::CameraMetadata> &result)
{
    camera_metadata_item_t item;
    common_metadata_header_t* metadata = result->get();
    int ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_FOCUS_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_DEBUG_LOG("Camera not support Focus mode");
        return;
    }
    MEDIA_DEBUG_LOG("Focus mode: %{public}d", item.data.u8[0]);
    auto it = metaToFwFocusMode_.find(static_cast<camera_focus_mode_enum_t>(item.data.u8[0]));
    // continuous focus mode do not callback focusStateChange
    if (it == metaToFwFocusMode_.end() || it->second != FOCUS_MODE_AUTO) {
        return;
    }
    ret = Camera::FindCameraMetadataItem(metadata, OHOS_CONTROL_FOCUS_STATE, &item);
    if (ret == CAM_META_SUCCESS) {
        MEDIA_DEBUG_LOG("Focus state: %{public}d", item.data.u8[0]);
        if (focusCallback_ != nullptr) {
            auto itr = metaToFwFocusState_.find(static_cast<camera_focus_state_t>(item.data.u8[0]));
            if (itr != metaToFwFocusState_.end() && itr->second != focusCallback_->currentState) {
                focusCallback_->OnFocusState(itr->second);
                focusCallback_->currentState = itr->second;
            }
        }
    }
}

std::vector<FlashMode> CaptureSession::GetSupportedFlashModes()
{
    std::vector<FlashMode> supportedFlashModes = {};
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFlashModes Session is not Commited");
        return supportedFlashModes;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFlashModes camera device is null");
        return supportedFlashModes;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FLASH_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFlashModes Failed with return code %{public}d", ret);
        return supportedFlashModes;
    }
    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwFlashMode_.find(static_cast<camera_flash_mode_enum_t>(item.data.u8[i]));
        if (itr != metaToFwFlashMode_.end()) {
            supportedFlashModes.emplace_back(itr->second);
        }
    }
    return supportedFlashModes;
}

int32_t CaptureSession::GetSupportedFlashModes(std::vector<FlashMode> &supportedFlashModes)
{
    supportedFlashModes.clear();
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFlashModes Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFlashModes camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_FLASH_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFlashModes Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwFlashMode_.find(static_cast<camera_flash_mode_enum_t>(item.data.u8[i]));
        if (itr != metaToFwFlashMode_.end()) {
            supportedFlashModes.emplace_back(itr->second);
        }
    }
    return CameraErrorCode::SUCCESS;
}

FlashMode CaptureSession::GetFlashMode()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFlashMode Session is not Commited");
        return FLASH_MODE_CLOSE;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFlashMode camera device is null");
        return FLASH_MODE_CLOSE;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FLASH_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFlashMode Failed with return code %{public}d", ret);
        return FLASH_MODE_CLOSE;
    }
    auto itr = metaToFwFlashMode_.find(static_cast<camera_flash_mode_enum_t>(item.data.u8[0]));
    if (itr != metaToFwFlashMode_.end()) {
        return itr->second;
    }

    return FLASH_MODE_CLOSE;
}

int32_t CaptureSession::GetFlashMode(FlashMode &flashMode)
{
    flashMode = FLASH_MODE_CLOSE;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetFlashMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFlashMode camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FLASH_MODE, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetFlashMode Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    auto itr = metaToFwFlashMode_.find(static_cast<camera_flash_mode_enum_t>(item.data.u8[0]));
    if (itr != metaToFwFlashMode_.end()) {
        flashMode = itr->second;
        return CameraErrorCode::SUCCESS;
    }

    return CameraErrorCode::SUCCESS;
}

int32_t CaptureSession::SetFlashMode(FlashMode flashMode)
{
    CAMERA_SYNC_TRACE;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetFlashMode Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetFlashMode Need to call LockForControl() before setting camera properties");
        return CameraErrorCode::SUCCESS;
    }
    uint8_t flash = fwToMetaFlashMode_.at(FLASH_MODE_CLOSE);
    auto itr = fwToMetaFlashMode_.find(flashMode);
    if (itr == fwToMetaFlashMode_.end()) {
        MEDIA_ERR_LOG("CaptureSession::SetExposureMode Unknown exposure mode");
    } else {
        flash = itr->second;
    }

    bool status = false;
    uint32_t count = 1;
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_FLASH_MODE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_FLASH_MODE, &flash, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_FLASH_MODE, &flash, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetFlashMode Failed to set flash mode");
        return CameraErrorCode::SUCCESS;
    }

    if (flashMode == FLASH_MODE_CLOSE) {
        POWERMGR_SYSEVENT_FLASH_OFF();
    } else {
        POWERMGR_SYSEVENT_FLASH_ON();
    }
    return CameraErrorCode::SUCCESS;
}

bool CaptureSession::IsFlashModeSupported(FlashMode flashMode)
{
    std::vector<FlashMode> vecSupportedFlashModeList;
    vecSupportedFlashModeList = this->GetSupportedFlashModes();
    if (find(vecSupportedFlashModeList.begin(), vecSupportedFlashModeList.end(), flashMode) !=
        vecSupportedFlashModeList.end()) {
        return true;
    }

    return false;
}

int32_t CaptureSession::IsFlashModeSupported(FlashMode flashMode, bool &isSupported)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::IsFlashModeSupported Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    std::vector<FlashMode> vecSupportedFlashModeList;
    vecSupportedFlashModeList = this->GetSupportedFlashModes();
    if (find(vecSupportedFlashModeList.begin(), vecSupportedFlashModeList.end(), flashMode) !=
        vecSupportedFlashModeList.end()) {
        isSupported = true;
        return CameraErrorCode::SUCCESS;
    }
    isSupported = false;
    return CameraErrorCode::SUCCESS;
}

bool CaptureSession::HasFlash()
{
    std::vector<FlashMode> vecSupportedFlashModeList;
    vecSupportedFlashModeList = this->GetSupportedFlashModes();
    if (vecSupportedFlashModeList.empty()) {
        return false;
    }
    return true;
}

int32_t CaptureSession::HasFlash(bool &hasFlash)
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::HasFlash Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    std::vector<FlashMode> vecSupportedFlashModeList;
    vecSupportedFlashModeList = this->GetSupportedFlashModes();
    if (vecSupportedFlashModeList.empty()) {
        hasFlash = false;
        return CameraErrorCode::SUCCESS;
    }
    hasFlash = true;
    return CameraErrorCode::SUCCESS;
}

std::vector<float> CaptureSession::GetZoomRatioRange()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatioRange Session is not Commited");
        return {};
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatioRange camera device is null");
        return {};
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_SCENE_ZOOM_CAP, &item);
    if (ret != CAM_META_SUCCESS || item.count == 0) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio Failed with return code %{public}d,item.count = %{public}d",
                      ret, item.count);
        return {};
    }
    const uint32_t step = 3;
    int32_t minIndex = 1;
    int32_t maxIndex = 2;
    constexpr float factor = 100.0;
    float minZoom = 0.0;
    float maxZoom = 0.0;
    for (uint32_t i = 0; i < item.count; i += step) {
        MEDIA_INFO_LOG("Scene zoom cap mode: %{public}d, min: %{public}d, max: %{public}d",
                       item.data.i32[i], item.data.i32[i + minIndex], item.data.i32[i + maxIndex]);
        if (GetMode() == item.data.i32[i]) {
            minZoom = item.data.i32[i + minIndex] / factor;
            maxZoom = item.data.i32[i + maxIndex] / factor;
            break;
        }
    }
    return {minZoom, maxZoom};
}

int32_t CaptureSession::GetZoomRatioRange(std::vector<float> &zoomRatioRange)
{
    MEDIA_INFO_LOG("CaptureSession::GetZoomRatioRange is Called");
    zoomRatioRange.clear();
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatioRange Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatioRange camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_SCENE_ZOOM_CAP, &item);
    if (ret != CAM_META_SUCCESS || item.count == 0) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio Failed with return code %{public}d,item.count = %{public}d",
                      ret, item.count);
        return 0;
    }
    const uint32_t step = 3;
    int32_t minIndex = 1;
    int32_t maxIndex = 2;
    constexpr float factor = 100.0;
    float minZoom = 0.0;
    float maxZoom = 0.0;
    for (uint32_t i = 0; i < item.count; i += step) {
        MEDIA_INFO_LOG("Scene zoom cap mode: %{public}d, min: %{public}d, max: %{public}d",
                       item.data.i32[i], item.data.i32[i + minIndex], item.data.i32[i + maxIndex]);
        if (GetMode() == item.data.i32[i]) {
            minZoom = item.data.i32[i + minIndex] / factor;
            maxZoom = item.data.i32[i + maxIndex] / factor;
            break;
        }
    }
    std::vector<float> range = {minZoom, maxZoom};
    zoomRatioRange = range;
    return CameraErrorCode::SUCCESS;
}

float CaptureSession::GetZoomRatio()
{
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio Session is not Commited");
        return 0;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio camera device is null");
        return 0;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio Failed with return code %{public}d", ret);
        return 0;
    }
    return static_cast<float>(item.data.f[0]);
}

int32_t CaptureSession::GetZoomRatio(float &zoomRatio)
{
    zoomRatio = 0;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio camera device is null");
        return CameraErrorCode::SUCCESS;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetZoomRatio Failed with return code %{public}d", ret);
        return CameraErrorCode::SUCCESS;
    }
    zoomRatio = static_cast<float>(item.data.f[0]);
    return CameraErrorCode::SUCCESS;
}

int32_t CaptureSession::SetZoomRatio(float zoomRatio)
{
    CAMERA_SYNC_TRACE;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetZoomRatio Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetZoomRatio Need to call LockForControl() before setting camera properties");
        return CameraErrorCode::SUCCESS;
    }

    bool status = false;
    int32_t ret;
    int32_t minIndex = 0;
    int32_t maxIndex = 1;
    int32_t count = 1;
    camera_metadata_item_t item;
    MEDIA_DEBUG_LOG("CaptureSession::SetZoomRatio Zoom ratio: %{public}f", zoomRatio);
    std::vector<float> zoomRange = GetZoomRatioRange();
    if (zoomRange.empty()) {
        MEDIA_ERR_LOG("CaptureSession::SetZoomRatio Zoom range is empty");
        return CameraErrorCode::SUCCESS;
    }
    if (zoomRatio < zoomRange[minIndex]) {
        MEDIA_DEBUG_LOG("CaptureSession::SetZoomRatio Zoom ratio: %{public}f is lesser than minimum zoom: %{public}f",
                        zoomRatio, zoomRange[minIndex]);
        zoomRatio = zoomRange[minIndex];
    } else if (zoomRatio > zoomRange[maxIndex]) {
        MEDIA_DEBUG_LOG("CaptureSession::SetZoomRatio Zoom ratio: %{public}f is greater than maximum zoom: %{public}f",
                        zoomRatio, zoomRange[maxIndex]);
        zoomRatio = zoomRange[maxIndex];
    }

    if (zoomRatio == 0) {
        MEDIA_ERR_LOG("CaptureSession::SetZoomRatio Invalid zoom ratio");
        return CameraErrorCode::SUCCESS;
    }

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_ZOOM_RATIO, &zoomRatio, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_ZOOM_RATIO, &zoomRatio, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetZoomRatio Failed to set zoom mode");
    }
    return CameraErrorCode::SUCCESS;
}

void CaptureSession::SetCaptureMetadataObjectTypes(std::set<camera_face_detect_mode_t> metadataObjectTypes)
{
    MEDIA_INFO_LOG("CaptureSession SetCaptureMetadataObjectTypes Enter");
    if (inputDevice_ == nullptr) {
        MEDIA_ERR_LOG("SetCaptureMetadataObjectTypes: inputDevice is null");
        return;
    }
    uint32_t count = 1;
    uint8_t objectType = OHOS_CAMERA_FACE_DETECT_MODE_SIMPLE;
    if (!metadataObjectTypes.count(OHOS_CAMERA_FACE_DETECT_MODE_SIMPLE)) {
        objectType = OHOS_CAMERA_FACE_DETECT_MODE_OFF;
        MEDIA_ERR_LOG("CaptureSession SetCaptureMetadataObjectTypes Can not set face detect mode!");
    }
    this->LockForControl();
    if (!this->changedMetadata_->addEntry(OHOS_STATISTICS_FACE_DETECT_SWITCH, &objectType, count)) {
        MEDIA_ERR_LOG("lvxq SetCaptureMetadataObjectTypes: Failed to add detect object types to changed metadata");
    }
    this->UnlockForControl();
}

void CaptureSession::SetMode(int32_t modeName)
{
    modeName_ = modeName;
    MEDIA_INFO_LOG("CaptureSession SetMode modeName = %{public}d", modeName);
}

int32_t CaptureSession::GetMode()
{
    MEDIA_INFO_LOG("CaptureSession GetMode modeName = %{public}d", modeName_);
    return modeName_;
}

int32_t CaptureSession::VerifyAbility(uint32_t ability)
{
    int32_t portraitMode = 3;
    if (GetMode() != portraitMode) {
        MEDIA_ERR_LOG("CaptureSession::VerifyAbility need PortraitMode");
        return CAMERA_INVALID_ARG;
    };
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::VerifyAbility camera device is null");
        return CAMERA_INVALID_ARG;
    }

    std::vector<Profile> photoProfiles = inputDevice_->GetCameraDeviceInfo()->modePhotoProfiles_[portraitMode];
    std::vector<Profile> previewProfiles = inputDevice_->GetCameraDeviceInfo()->modePreviewProfiles_[portraitMode];
    for (auto i : photoProfiles) {
        std::string abilityIds = "";
        for (auto id : i.GetAbilityId()) {
            abilityIds += std::to_string(id) + ",";
        }
        MEDIA_INFO_LOG("photoProfiles f(%{public}d), w(%{public}d), h(%{public}d), ability:(%{public}s)",
                       i.GetCameraFormat(), i.GetSize().width, i.GetSize().height, abilityIds.c_str());
        if (i.GetCameraFormat() == photoProfile_.GetCameraFormat() &&
            i.GetSize().width == photoProfile_.GetSize().width &&
            i.GetSize().height == photoProfile_.GetSize().height) {
                if (i.GetAbilityId().empty()) {
                    MEDIA_INFO_LOG("VerifyAbility::CreatePhotoOutput:: this size'abilityId is not exist");
                }
                photoProfile_.abilityId_ = i.GetAbilityId();
                break;
            }
    }
    for (auto i : previewProfiles) {
        std::string abilityIds = "";
        for (auto id : i.GetAbilityId()) {
            abilityIds += std::to_string(id) + ",";
        }
        MEDIA_INFO_LOG("previewProfiles f(%{public}d), w(%{public}d), h(%{public}d), ability:(%{public}s)",
                       i.GetCameraFormat(), i.GetSize().width, i.GetSize().height, abilityIds.c_str());
        if (i.GetCameraFormat() == previewProfile_.GetCameraFormat() &&
            i.GetSize().width == previewProfile_.GetSize().width &&
            i.GetSize().height == previewProfile_.GetSize().height) {
                if (i.GetAbilityId().empty()) {
                    MEDIA_INFO_LOG("VerifyAbility::CreatePreviewOutput:: this size'abilityId is not exist");
                }
                previewProfile_.abilityId_ = i.GetAbilityId();
                break;
            }
    }
    std::vector<uint32_t> photoAbilityId = previewProfile_.GetAbilityId();
    std::vector<uint32_t> previewAbilityId = previewProfile_.GetAbilityId();

    auto itrPhoto = std::find(photoAbilityId.begin(), photoAbilityId.end(), ability);
    auto itrPreview = std::find(previewAbilityId.begin(), previewAbilityId.end(), ability);
    if (itrPhoto == photoAbilityId.end() || itrPreview == previewAbilityId.end()) {
        MEDIA_ERR_LOG("CaptureSession::VerifyAbility abilityId is NULL");
        return CAMERA_INVALID_ARG;
    }
    return CAMERA_OK;
}

std::vector<FilterType> CaptureSession::GetSupportedFilters()
{
    std::vector<FilterType> supportedFilters = {};
    if (!(IsSessionCommited() || IsSessionConfiged())) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFilters Session is not Commited");
        return supportedFilters;
    }

    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFilters camera device is null");
        return supportedFilters;
    }

    int ret = VerifyAbility(static_cast<uint32_t>(OHOS_ABILITY_SCENE_FILTER_TYPES));
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFilters abilityId is NULL");
        return supportedFilters;
    }

    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_SCENE_FILTER_TYPES, &item);
    if (ret != CAM_META_SUCCESS || item.count == 0) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFilters Failed with return code %{public}d", ret);
        return supportedFilters;
    }
    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwFilterType_.find(static_cast<camera_filter_type_t>(item.data.u8[i]));
        if (itr != metaToFwFilterType_.end()) {
            supportedFilters.emplace_back(itr->second);
        }
    }
    return supportedFilters;
}

FilterType CaptureSession::GetFilter()
{
    if (!(IsSessionCommited() || IsSessionConfiged())) {
        MEDIA_ERR_LOG("CaptureSession::GetFilter Session is not Commited");
        return FilterType::NONE;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetFilter camera device is null");
        return FilterType::NONE;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_FILTER_TYPE, &item);
    if (ret != CAM_META_SUCCESS || item.count == 0) {
        MEDIA_ERR_LOG("CaptureSession::GetFilter Failed with return code %{public}d", ret);
        return FilterType::NONE;
    }
    auto itr = metaToFwFilterType_.find(static_cast<camera_filter_type_t>(item.data.u8[0]));
    if (itr != metaToFwFilterType_.end()) {
        return itr->second;
    }
    return FilterType::NONE;
}

void CaptureSession::SetFilter(FilterType filterType)
{
    CAMERA_SYNC_TRACE;
    if (!(IsSessionCommited() || IsSessionConfiged())) {
        MEDIA_ERR_LOG("CaptureSession::SetFilter Session is not Commited");
        return;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetFilter Need to call LockForControl() before setting camera properties");
        return;
    }

    std::vector<FilterType> supportedFilters = GetSupportedFilters();
    auto itr = std::find(supportedFilters.begin(), supportedFilters.end(), filterType);
    if (itr == supportedFilters.end()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedFilters abilityId is NULL");
        return;
    }
    uint8_t filter = 0;
    for (auto itr2 = fwToMetaFilterType_.cbegin(); itr2 != fwToMetaFilterType_.cend(); itr2++) {
        if (filterType == itr2->first) {
            filter = static_cast<uint8_t>(itr2->second);
            break;
        }
    }
    bool status = false;
    int32_t ret;
    uint32_t count = 1;
    camera_metadata_item_t item;

    MEDIA_DEBUG_LOG("CaptureSession::setFilter: %{public}d", filterType);

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_FILTER_TYPE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_FILTER_TYPE, &filter, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_FILTER_TYPE, &filter, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::setFilter Failed to set filter");
    }
    return;
}

std::vector<BeautyType> CaptureSession::GetSupportedBeautyTypes()
{
    std::vector<BeautyType> supportedBeautyTypes = {};
    if (!(IsSessionCommited() || IsSessionConfiged())) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyTypes Session is not Commited");
        return supportedBeautyTypes;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyTypes camera device is null");
        return supportedBeautyTypes;
    }

    int ret = VerifyAbility(static_cast<uint32_t>(OHOS_ABILITY_SCENE_BEAUTY_TYPES));
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyTypes abilityId is NULL");
        return supportedBeautyTypes;
    }

    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_SCENE_BEAUTY_TYPES, &item);
    if (ret != CAM_META_SUCCESS || item.count == 0) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyTypes Failed with return code %{public}d", ret);
        return supportedBeautyTypes;
    }
    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwBeautyType_.find(static_cast<camera_beauty_type_t>(item.data.u8[i]));
        if (itr != metaToFwBeautyType_.end()) {
            supportedBeautyTypes.emplace_back(itr->second);
        }
    }
    return supportedBeautyTypes;
}

std::vector<int32_t> CaptureSession::GetSupportedBeautyRange(BeautyType beautyType)
{
    int ret  = 0;
    std::vector<int32_t> supportedBeautyRange;
    if (!(IsSessionCommited() || IsSessionConfiged())) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyRange Session is not Commited");
        return supportedBeautyRange;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyRange camera device is null");
        return supportedBeautyRange;
    }

    std::vector<BeautyType> supportedBeautyTypes = GetSupportedBeautyTypes();
    if (std::find(supportedBeautyTypes.begin(), supportedBeautyTypes.end(), beautyType) == supportedBeautyTypes.end()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyRange beautyType is NULL");
        return supportedBeautyRange;
    }

    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;

    MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyRange: %{public}d", beautyType);

    int32_t beautyTypeAbility;
    auto itr = fwToMetaBeautyAbility_.find(beautyType);
    if (itr == fwToMetaBeautyAbility_.end()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyRange Unknown beauty Type");
        return supportedBeautyRange;
    } else {
        beautyTypeAbility = itr->second;
        Camera::FindCameraMetadataItem(metadata->get(), beautyTypeAbility, &item);
    }

    if (ret != CAM_META_SUCCESS || item.count == 0) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyRange Failed with return code %{public}d", ret);
        return supportedBeautyRange;
    }
    int32_t skinToneOff = -1;
    if (beautyType == SKIN_TONE) {
        supportedBeautyRange.push_back(skinToneOff);
    }
    for (uint32_t i = 0; i < item.count; i++) {
        if (beautyTypeAbility == OHOS_ABILITY_BEAUTY_SKIN_TONE_VALUES) {
            supportedBeautyRange.emplace_back(item.data.i32[i]);
        } else {
            supportedBeautyRange.emplace_back(item.data.u8[i]);
        }
    }
    beautyTypeAndRanges_[beautyType] = supportedBeautyRange;
    return supportedBeautyRange;
}

bool CaptureSession::SetBeautyValue(BeautyType beautyType, int32_t beautyLevel)
{
    bool status = false;
    int32_t ret;
    uint32_t count = 1;
    camera_metadata_item_t item;
    camera_device_metadata_tag_t metadata;

    for (auto metaItr = fwToMetaBeautyControl_.cbegin(); metaItr != fwToMetaBeautyControl_.cend(); metaItr++) {
        if (beautyType  == metaItr->first) {
            metadata = metaItr->second;
            break;
        }
    }

    std::vector<int32_t> levelVec = {};
    if (beautyTypeAndRanges_.count(beautyType)) {
        levelVec = beautyTypeAndRanges_[beautyType];
    } else {
        levelVec = GetSupportedBeautyRange(beautyType);
    }

    auto itrType = std::find(levelVec.cbegin(), levelVec.cend(), beautyLevel);
    if (itrType == levelVec.end()) {
        MEDIA_ERR_LOG("CaptureSession::SetBeautyValue: %{public}d not in beautyRanges", beautyLevel);
        return status;
    }
    if (beautyType == BeautyType::SKIN_TONE) {
        int32_t skinToneVal = beautyLevel;
        ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), metadata, &item);
        if (ret == CAM_META_ITEM_NOT_FOUND) {
            status = changedMetadata_->addEntry(metadata, &skinToneVal, count);
        } else if (ret == CAM_META_SUCCESS) {
            status = changedMetadata_->updateEntry(metadata, &skinToneVal, count);
        }
    } else {
        uint8_t beautyVal = static_cast<uint8_t>(beautyLevel);
        ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), metadata, &item);
        if (ret == CAM_META_ITEM_NOT_FOUND) {
            status = changedMetadata_->addEntry(metadata, &beautyVal, count);
        } else if (ret == CAM_META_SUCCESS) {
            status = changedMetadata_->updateEntry(metadata, &beautyVal, count);
        }
    }
    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetBeautyValue Failed to set beauty");
        return status;
    }
    beautyTypeAndLevels_[beautyType] = beautyLevel;
    return status;
}

void CaptureSession::SetBeauty(BeautyType beautyType, int value)
{
    if (!(IsSessionCommited() || IsSessionConfiged())) {
        MEDIA_ERR_LOG("CaptureSession::SetBeauty Session is not Commited");
        return;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetBeauty changedMetadata_ is NULL");
        return;
    }

    std::vector<BeautyType> supportedBeautyTypes = GetSupportedBeautyTypes();
    auto itr = std::find(supportedBeautyTypes.begin(), supportedBeautyTypes.end(), beautyType);
    if (itr == supportedBeautyTypes.end()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedBeautyTypes abilityId is NULL");
        return;
    }
    MEDIA_ERR_LOG("SetBeauty beautyType %{public}d", beautyType);
    bool status = false;
    int32_t ret;
    uint32_t count = 1;
    camera_metadata_item_t item;
    uint8_t beauty = OHOS_CAMERA_BEAUTY_TYPE_OFF;
    if ((beautyType == AUTO_TYPE) && (value == 0)) {
        ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_BEAUTY_TYPE, &item);
        if (ret == CAM_META_ITEM_NOT_FOUND) {
            status = changedMetadata_->addEntry(OHOS_CONTROL_BEAUTY_TYPE, &beauty, count);
        } else if (ret == CAM_META_SUCCESS) {
            status = changedMetadata_->updateEntry(OHOS_CONTROL_BEAUTY_TYPE, &beauty, count);
        }
        MEDIA_INFO_LOG("CaptureSession::SetBeauty Failed, beautyType is AUTO and level is zero");
        return;
    }

    for (auto itrType = fwToMetaBeautyType_.cbegin(); itrType != fwToMetaBeautyType_.cend(); itrType++) {
        if (beautyType == itrType->first) {
            beauty = static_cast<uint8_t>(itrType->second);
            break;
        }
    }

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_BEAUTY_TYPE, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_BEAUTY_TYPE, &beauty, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_BEAUTY_TYPE, &beauty, count);
    }
    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetBeauty Failed to set beautyType control");
    }

    status = SetBeautyValue(beautyType, value);
    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetBeauty Failed to set beauty value");
    }
    return;
}

int32_t CaptureSession::GetBeauty(BeautyType beautyType)
{
    if (!(IsSessionCommited() || IsSessionConfiged())) {
        MEDIA_ERR_LOG("CaptureSession::GetBeauty Session is not Commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetBeauty camera device is null");
        return -1;
    }
    int32_t beautyLevel = 0;
    std::vector<BeautyType> supportedBeautyTypes = GetSupportedBeautyTypes();
    auto itr = std::find(supportedBeautyTypes.begin(), supportedBeautyTypes.end(), beautyType);
    if (itr == supportedBeautyTypes.end()) {
        MEDIA_ERR_LOG("CaptureSession::GetBeauty beautyType is NULL");
        return -1;
    }

    for (auto itrLevel = beautyTypeAndLevels_.cbegin(); itrLevel != beautyTypeAndLevels_.cend(); itrLevel++) {
        if (beautyType == itrLevel->first) {
            beautyLevel = itrLevel->second;
            break;
        }
    }
    return beautyLevel;
}

void CaptureSession::SetFrameRateRange(const std::vector<int32_t>& frameRateRange)
{
    std::vector<int32_t> videoFrameRateRange = frameRateRange;
    this->LockForControl();
    if (!this->changedMetadata_->addEntry(OHOS_CONTROL_FPS_RANGES,
        videoFrameRateRange.data(), videoFrameRateRange.size())) {
        MEDIA_ERR_LOG("Failed to SetFrameRateRange");
    }
    this->UnlockForControl();
}

bool CaptureSession::IsSessionConfiged()
{
    bool isSessionConfiged = false;
    if (captureSession_) {
        CaptureSessionState currentState;
        captureSession_->GetSessionState(currentState);
        isSessionConfiged = (currentState == CaptureSessionState::SESSION_CONFIG_INPROGRESS);
    }
    return isSessionConfiged;
}

bool CaptureSession::IsSessionCommited()
{
    bool isCommitConfig = false;
    if (captureSession_) {
        CaptureSessionState currentState;
        captureSession_->GetSessionState(currentState);
        isCommitConfig = (currentState == CaptureSessionState::SESSION_CONFIG_COMMITTED);
    }
    return isCommitConfig;
}

int32_t CaptureSession::CalculateExposureValue(float exposureValue)
{
    camera_metadata_item_t item;
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_AE_COMPENSATION_STEP, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::Get Ae Compensation step Failed with return code %{public}d", ret);
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }

    int32_t stepNumerator = item.data.r->numerator;
    int32_t stepDenominator = item.data.r->denominator;
    float stepsPerEv = static_cast<float>(stepDenominator) / static_cast<float>(stepNumerator);
    MEDIA_DEBUG_LOG("Exposure step numerator: %{public}d, denominatormax: %{public}d, stepsPerEv: %{public}f",
        stepNumerator, stepDenominator, stepsPerEv);

    int32_t exposureCompensation = static_cast<int32_t>(stepsPerEv * exposureValue);
    MEDIA_DEBUG_LOG("exposureCompensation: %{public}d", exposureCompensation);
    return exposureCompensation;
}

std::vector<ColorEffect> CaptureSession::GetSupportedColorEffects()
{
    std::vector<ColorEffect> supportedColorEffects = {};
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedColorEffects Session is not Commited");
        return supportedColorEffects;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedColorEffects camera device is null");
        return supportedColorEffects;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_SUPPORTED_COLOR_MODES, &item);
    if (ret != CAM_META_SUCCESS) {
        MEDIA_ERR_LOG("CaptureSession::GetSupportedColorEffects Failed with return code %{public}d", ret);
        return supportedColorEffects;
    }
    for (uint32_t i = 0; i < item.count; i++) {
        auto itr = metaToFwColorEffect_.find(static_cast<camera_xmage_color_type_t>(item.data.u8[i]));
        if (itr != metaToFwColorEffect_.end()) {
            supportedColorEffects.emplace_back(itr->second);
        }
    }
    return supportedColorEffects;
}

ColorEffect CaptureSession::GetColorEffect()
{
    ColorEffect colorEffect = ColorEffect::COLOR_EFFECT_NORMAL;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::GetColorEffect Session is not Commited");
        return colorEffect;
    }
    if (!inputDevice_ || !inputDevice_->GetCameraDeviceInfo()) {
        MEDIA_ERR_LOG("CaptureSession::GetColorEffect camera device is null");
        return colorEffect;
    }
    std::shared_ptr<Camera::CameraMetadata> metadata = inputDevice_->GetCameraDeviceInfo()->GetMetadata();
    camera_metadata_item_t item;
    int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_SUPPORTED_COLOR_MODES, &item);
    if (ret != CAM_META_SUCCESS || item.count == 0) {
        MEDIA_ERR_LOG("CaptureSession::GetColorEffect Failed with return code %{public}d", ret);
        return colorEffect;
    }
    auto itr = metaToFwColorEffect_.find(static_cast<camera_xmage_color_type_t>(item.data.u8[0]));
    if (itr != metaToFwColorEffect_.end()) {
        colorEffect = itr->second;
    }
    return colorEffect;
}

void CaptureSession::SetColorEffect(ColorEffect colorEffect)
{
    CAMERA_SYNC_TRACE;
    if (!IsSessionCommited()) {
        MEDIA_ERR_LOG("CaptureSession::SetColorEffect Session is not Commited");
        return;
    }
    if (changedMetadata_ == nullptr) {
        MEDIA_ERR_LOG("CaptureSession::SetColorEffect Need to call LockForControl() before setting camera properties");
        return;
    }
    uint8_t colorEffectTemp = ColorEffect::COLOR_EFFECT_NORMAL;
    auto itr = fwToMetaColorEffect_.find(colorEffect);
    if (itr == fwToMetaColorEffect_.end()) {
        MEDIA_ERR_LOG("CaptureSession::SetColorEffect unknown is color effect");
        return;
    } else {
        colorEffectTemp = itr->second;
    }

    bool status = false;
    int32_t ret;
    uint32_t count = 1;
    camera_metadata_item_t item;

    MEDIA_DEBUG_LOG("CaptureSession::SetColorEffect: %{public}d", colorEffect);

    ret = Camera::FindCameraMetadataItem(changedMetadata_->get(), OHOS_CONTROL_SUPPORTED_COLOR_MODES, &item);
    if (ret == CAM_META_ITEM_NOT_FOUND) {
        status = changedMetadata_->addEntry(OHOS_CONTROL_SUPPORTED_COLOR_MODES, &colorEffectTemp, count);
    } else if (ret == CAM_META_SUCCESS) {
        status = changedMetadata_->updateEntry(OHOS_CONTROL_SUPPORTED_COLOR_MODES, &colorEffectTemp, count);
    }

    if (!status) {
        MEDIA_ERR_LOG("CaptureSession::SetColorEffect Failed to set color effect");
    }
    return;
}
} // CameraStandard
} // OHOS
