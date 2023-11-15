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

#include "output/preview_output.h"

#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <variant>

#include "camera_device_ability_items.h"
#include "camera_log.h"
#include "camera_metadata_operator.h"
#include "camera_output_capability.h"
#include "camera_util.h"
#include "hstream_repeat_callback_stub.h"
#include "image_format.h"
#include "metadata_common_utils.h"
#include "mode/mode_manager.h"
#include "pixel_map.h"
#include "sketch_wrapper.h"

namespace OHOS {
namespace CameraStandard {
static constexpr int32_t SKETCH_MIN_WIDTH = 600;
PreviewOutput::PreviewOutput(sptr<IStreamRepeat>& streamRepeat)
    : CaptureOutput(CAPTURE_OUTPUT_TYPE_PREVIEW, StreamType::REPEAT, streamRepeat)
{}

PreviewOutput::~PreviewOutput()
{
    svcCallback_ = nullptr;
    appCallback_ = nullptr;
}

int32_t PreviewOutput::Release()
{
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    MEDIA_DEBUG_LOG("Enter Into PreviewOutput::Release");
    if (GetStream() == nullptr) {
        MEDIA_ERR_LOG("PreviewOutput Failed to Release!, GetStream is nullptr");
        return CameraErrorCode::SERVICE_FATL_ERROR;
    }
    auto itemStream = static_cast<IStreamRepeat*>(GetStream().GetRefPtr());
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (itemStream) {
        errCode = itemStream->Release();
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to release PreviewOutput!, errCode: %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("PreviewOutput::Release() itemStream is nullptr");
    }
    svcCallback_ = nullptr;
    appCallback_ = nullptr;
    CaptureOutput::Release();
    return ServiceToCameraError(errCode);
}

int32_t PreviewOutputCallbackImpl::OnFrameStarted()
{
    CAMERA_SYNC_TRACE;
    if (previewOutput_ != nullptr && previewOutput_->GetApplicationCallback() != nullptr) {
        previewOutput_->GetApplicationCallback()->OnFrameStarted();
    } else {
        MEDIA_INFO_LOG("Discarding PreviewOutputCallbackImpl::OnFrameStarted callback in preview");
    }
    return CAMERA_OK;
}

int32_t PreviewOutputCallbackImpl::OnFrameEnded(int32_t frameCount)
{
    CAMERA_SYNC_TRACE;
    if (previewOutput_ != nullptr && previewOutput_->GetApplicationCallback() != nullptr) {
        previewOutput_->GetApplicationCallback()->OnFrameEnded(frameCount);
    } else {
        MEDIA_INFO_LOG("Discarding PreviewOutputCallbackImpl::OnFrameEnded callback in preview");
    }
    return CAMERA_OK;
}

int32_t PreviewOutputCallbackImpl::OnFrameError(int32_t errorCode)
{
    if (previewOutput_ != nullptr && previewOutput_->GetApplicationCallback() != nullptr) {
        previewOutput_->GetApplicationCallback()->OnError(errorCode);
    } else {
        MEDIA_INFO_LOG("Discarding PreviewOutputCallbackImpl::OnFrameError callback in preview");
    }
    return CAMERA_OK;
}

int32_t PreviewOutputCallbackImpl::OnSketchStatusChanged(SketchStatus status)
{
    if (previewOutput_ != nullptr) {
        previewOutput_->OnSketchStatusChanged(status);
    } else {
        MEDIA_INFO_LOG("Discarding PreviewOutputCallbackImpl::OnFrameError callback in preview");
    }
    return CAMERA_OK;
}

int32_t PreviewOutput::OnSketchStatusChanged(SketchStatus status)
{
    if (sketchWrapper_ == nullptr) {
        return CAMERA_INVALID_STATE;
    }
    auto session = GetSession();
    if (session == nullptr) {
        return CAMERA_INVALID_STATE;
    }
    SketchWrapper* wrapper = static_cast<SketchWrapper*>(sketchWrapper_.get());
    wrapper->OnSketchStatusChanged(status, session->GetFeaturesMode());
    return CAMERA_OK;
}

void PreviewOutput::AddDeferredSurface(sptr<Surface> surface)
{
    MEDIA_INFO_LOG("PreviewOutput::AddDeferredSurface called");
    if (surface == nullptr) {
        MEDIA_ERR_LOG("PreviewOutput::AddDeferredSurface surface is null");
        return;
    }
    auto itemStream = static_cast<IStreamRepeat*>(GetStream().GetRefPtr());
    if (!itemStream) {
        MEDIA_ERR_LOG("PreviewOutput::AddDeferredSurface itemStream is nullptr");
        return;
    }
    itemStream->AddDeferredSurface(surface->GetProducer());
}

int32_t PreviewOutput::Start()
{
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    MEDIA_DEBUG_LOG("Enter Into PreviewOutput::Start");
    auto captureSession = GetSession();
    if (captureSession == nullptr || !captureSession->IsSessionCommited()) {
        MEDIA_ERR_LOG("PreviewOutput Failed to Start!, session not config");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (GetStream() == nullptr) {
        MEDIA_ERR_LOG("PreviewOutput Failed to Start!, GetStream is nullptr");
        return CameraErrorCode::SERVICE_FATL_ERROR;
    }
    auto itemStream = static_cast<IStreamRepeat*>(GetStream().GetRefPtr());
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (itemStream) {
        errCode = itemStream->Start();
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("PreviewOutput Failed to Start!, errCode: %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("PreviewOutput::Start itemStream is nullptr");
    }
    return ServiceToCameraError(errCode);
}

int32_t PreviewOutput::Stop()
{
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    MEDIA_DEBUG_LOG("Enter Into PreviewOutput::Stop");
    if (GetStream() == nullptr) {
        MEDIA_ERR_LOG("PreviewOutput Failed to Stop!, GetStream is nullptr");
        return CameraErrorCode::SERVICE_FATL_ERROR;
    }
    auto itemStream = static_cast<IStreamRepeat*>(GetStream().GetRefPtr());
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    if (itemStream) {
        errCode = itemStream->Stop();
        if (errCode != CAMERA_OK) {
            MEDIA_ERR_LOG("PreviewOutput Failed to Stop!, errCode: %{public}d", errCode);
        }
    } else {
        MEDIA_ERR_LOG("PreviewOutput::Stop itemStream is nullptr");
    }
    return ServiceToCameraError(errCode);
}

bool PreviewOutput::IsSketchSupported()
{
    MEDIA_DEBUG_LOG("Enter Into PreviewOutput::IsSketchSupported");

    Profile profile = GetPreviewProfile();
    CameraFormat cameraFormat = profile.GetCameraFormat();
    if (cameraFormat != CAMERA_FORMAT_YUV_420_SP && cameraFormat != CAMERA_FORMAT_YCBCR_420_888) {
        MEDIA_ERR_LOG("PreviewOutput::IsSketchSupported preview format is illegal:%{public}d",
            static_cast<int32_t>(cameraFormat));
        return false;
    }

    auto sketchSize = FindSketchSize();
    if (sketchSize == nullptr) {
        return false;
    }
    MEDIA_INFO_LOG(
        "IsSketchSupported FindSketchSize Size is %{public}dx%{public}d", sketchSize->width, sketchSize->height);
    auto sketchRatio = GetSketchRatio();
    if (sketchRatio > 0) {
        return true;
    }
    MEDIA_DEBUG_LOG("IsSketchSupported GetSketchRatio failed, %{public}f ", sketchRatio);
    auto session = GetSession();
    if (session == nullptr) {
        return false;
    }
    SketchWrapper::UpdateSketchStaticInfo(GetDeviceMetadata());
    auto subModeNames = session->GetSubFeatureMods();
    for (auto subModeName : subModeNames) {
        float ratio = SketchWrapper::GetSketchEnableRatio(subModeName);
        if (ratio > 0) {
            MEDIA_DEBUG_LOG("IsSketchSupported GetSketchEnableRatio success,mode:%{public}d, ratio:%{public}f ",
                subModeName, ratio);
            return true;
        }
    }
    return false;
}

float PreviewOutput::GetSketchRatio()
{
    MEDIA_DEBUG_LOG("Enter Into PreviewOutput::GetSketchRatio");

    auto session = GetSession();
    if (session == nullptr) {
        MEDIA_WARNING_LOG("PreviewOutput::GetSketchRatio session is null");
        return -1.0f;
    }
    int32_t currentMode = session->GetFeaturesMode();
    SketchWrapper::UpdateSketchStaticInfo(GetDeviceMetadata());
    float ratio = SketchWrapper::GetSketchEnableRatio(currentMode);
    if (ratio <= 0) {
        MEDIA_WARNING_LOG("PreviewOutput::GetSketchRatio %{public}d mode", currentMode);
    }
    return ratio;
}

int32_t PreviewOutput::CreateSketchWrapper(Size sketchSize, sptr<Surface> sketchSurface)
{
    if (sketchSurface == nullptr) {
        MEDIA_ERR_LOG("CreateSketchWrapper sketchSurface is null");
        return ServiceToCameraError(CAMERA_INVALID_ARG);
    }
    auto session = GetSession();
    if (session == nullptr) {
        MEDIA_ERR_LOG("EnableSketch session null");
        return ServiceToCameraError(CAMERA_INVALID_STATE);
    }
    auto wrapper = std::make_shared<SketchWrapper>(GetStream(), sketchSize, sketchSurface);
    if (wrapper == nullptr) {
        return ServiceToCameraError(CAMERA_ALLOC_ERROR);
    }
    sketchWrapper_ = wrapper;
    auto metadata = GetDeviceMetadata();
    int32_t retCode = wrapper->Init(metadata, session->GetFeaturesMode());
    wrapper->SetPreviewStateCallback(appCallback_);
    return ServiceToCameraError(retCode);
}

int32_t PreviewOutput::EnableSketch(bool isEnable, sptr<Surface> sketchSurface)
{
    MEDIA_DEBUG_LOG("Enter Into PreviewOutput::EnableSketch");
    if (!IsSketchSupported()) {
        MEDIA_ERR_LOG("EnableSketch IsSketchSupported is false");
        return CameraErrorCode::OPERATION_NOT_ALLOWED;
    }
    int32_t errCode = CAMERA_UNKNOWN_ERROR;
    std::lock_guard<std::mutex> lock(asyncOpMutex_);

    auto captureSession = GetSession();
    if (captureSession == nullptr || !captureSession->IsSessionConfiged()) {
        MEDIA_ERR_LOG("PreviewOutput Failed EnableSketch!, session not config");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }

    if (isEnable) {
        if (sketchWrapper_ != nullptr) {
            return ServiceToCameraError(CAMERA_OPERATION_NOT_ALLOWED);
        }
        auto sketchSize = FindSketchSize();
        if (sketchSize == nullptr) {
            MEDIA_ERR_LOG("EnableSketch FindSketchSize is null");
            return ServiceToCameraError(errCode);
        }
        MEDIA_INFO_LOG(
            "EnableSketch FindSketchSize Size is %{public}dx%{public}d", sketchSize->width, sketchSize->height);
        return CreateSketchWrapper(*sketchSize, sketchSurface);
    }

    // Disable sketch branch
    if (sketchWrapper_ == nullptr) {
        return ServiceToCameraError(CAMERA_OPERATION_NOT_ALLOWED);
    }
    auto wrapper = static_pointer_cast<SketchWrapper>(sketchWrapper_);
    errCode = wrapper->Destroy();
    sketchWrapper_ = nullptr;
    return ServiceToCameraError(errCode);
}

int32_t PreviewOutput::StartSketch()
{
    int32_t errCode = CAMERA_UNKNOWN_ERROR;

    auto captureSession = GetSession();
    if (captureSession == nullptr || !captureSession->IsSessionCommited()) {
        MEDIA_ERR_LOG("PreviewOutput Failed StartSketch!, session not commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }
    if (sketchWrapper_ != nullptr) {
        errCode = static_pointer_cast<SketchWrapper>(sketchWrapper_)->StartSketchStream();
    }
    return ServiceToCameraError(errCode);
}

int32_t PreviewOutput::StopSketch()
{
    int32_t errCode = CAMERA_UNKNOWN_ERROR;

    auto captureSession = GetSession();
    if (captureSession == nullptr || !captureSession->IsSessionCommited()) {
        MEDIA_ERR_LOG("PreviewOutput Failed StopSketch!, session not commited");
        return CameraErrorCode::SESSION_NOT_CONFIG;
    }

    if (sketchWrapper_ != nullptr) {
        errCode = static_pointer_cast<SketchWrapper>(sketchWrapper_)->StopSketchStream();
    }
    return ServiceToCameraError(errCode);
}

std::shared_ptr<Camera::CameraMetadata> PreviewOutput::GetDeviceMetadata()
{
    auto session = GetSession();
    if (session == nullptr) {
        MEDIA_WARNING_LOG("PreviewOutput::GetDeviceMetadata session is null");
        return nullptr;
    }
    auto inputDevice = session->inputDevice_;
    if (inputDevice == nullptr) {
        MEDIA_WARNING_LOG("PreviewOutput::GetDeviceMetadata inputDevice is null");
        return nullptr;
    }
    auto deviceInfo = inputDevice->GetCameraDeviceInfo();
    if (deviceInfo == nullptr) {
        MEDIA_WARNING_LOG("PreviewOutput::GetDeviceMetadata deviceInfo is null");
        return nullptr;
    }
    auto metaData = deviceInfo->GetMetadata();
    if (metaData == nullptr) {
        MEDIA_WARNING_LOG("PreviewOutput::GetDeviceMetadata metaData is null");
        return nullptr;
    }
    return metaData;
}

std::shared_ptr<Size> PreviewOutput::FindSketchSize()
{
    auto session = GetSession();
    auto metaData = GetDeviceMetadata();
    if (session == nullptr || metaData == nullptr) {
        MEDIA_ERR_LOG("PreviewOutput::FindSketchSize GetDeviceMetadata is fail");
        return nullptr;
    }
    Profile profile = GetPreviewProfile();
    camera_format_t hdi_format = OHOS_CAMERA_FORMAT_IMPLEMENTATION_DEFINED;
    CameraFormat cameraFormat = profile.GetCameraFormat();
    if (cameraFormat == CAMERA_FORMAT_YUV_420_SP) {
        hdi_format = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    } else if (cameraFormat == CAMERA_FORMAT_YCBCR_420_888) {
        hdi_format = OHOS_CAMERA_FORMAT_YCBCR_420_888;
    } else {
        MEDIA_ERR_LOG("PreviewOutput::FindSketchSize preview format is illegal");
        return nullptr;
    }
    auto sizeList = MetadataCommonUtils::GetSupportedPreviewSizeRange(session->GetFeaturesMode(), hdi_format, metaData);
    if (sizeList == nullptr || sizeList->size() == 0) {
        return nullptr;
    }
    Size previewSize = profile.GetSize();
    if (previewSize.width <= 0 || previewSize.height <= 0) {
        return nullptr;
    }
    float ratio = static_cast<float>(previewSize.width) / previewSize.height;
    MEDIA_INFO_LOG("PreviewOutput::FindSketchSize preview size:%{public}dx%{public}d,ratio:%{public}f",
        previewSize.width, previewSize.height, ratio);
    std::shared_ptr<Size> outSize;
    for (auto size : *sizeList.get()) {
        if (size.width >= previewSize.width || size.width < SKETCH_MIN_WIDTH) {
            continue;
        }
        float checkRatio = static_cast<float>(size.width) / size.height;
        MEDIA_DEBUG_LOG("PreviewOutput::FindSketchSize List size:%{public}dx%{public}d,ratio:%{public}f", size.width,
            size.height, checkRatio);
        if (abs(checkRatio - ratio) / ratio <= 0.05f) { // 0.05f is 5% tolerance
            if (outSize == nullptr) {
                outSize = std::make_shared<Size>(size);
            } else if (size.width < outSize->width) {
                outSize = std::make_shared<Size>(size);
            }
        }
    }
    return outSize;
}

void PreviewOutput::SetCallback(std::shared_ptr<PreviewStateCallback> callback)
{
    appCallback_ = callback;
    if (appCallback_ != nullptr) {
        if (svcCallback_ == nullptr) {
            svcCallback_ = new (std::nothrow) PreviewOutputCallbackImpl(this);
            if (svcCallback_ == nullptr) {
                MEDIA_ERR_LOG("new PreviewOutputCallbackImpl Failed to register callback");
                appCallback_ = nullptr;
                return;
            }
        }
        if (GetStream() == nullptr) {
            MEDIA_ERR_LOG("PreviewOutput Failed to SetCallback!, GetStream is nullptr");
            return;
        }
        auto itemStream = static_cast<IStreamRepeat*>(GetStream().GetRefPtr());
        int32_t errorCode = CAMERA_OK;
        if (itemStream) {
            errorCode = itemStream->SetCallback(svcCallback_);
        } else {
            MEDIA_ERR_LOG("PreviewOutput::SetCallback itemStream is nullptr");
        }
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("PreviewOutput::SetCallback: Failed to register callback, errorCode: %{public}d", errorCode);
            svcCallback_ = nullptr;
            appCallback_ = nullptr;
        }
    }
    if (sketchWrapper_ != nullptr) {
        SketchWrapper* wrapper = static_cast<SketchWrapper*>(sketchWrapper_.get());
        wrapper->SetPreviewStateCallback(appCallback_);
    }
    return;
}

std::set<camera_device_metadata_tag_t> PreviewOutput::GetObserverTags() const
{
    return { OHOS_CONTROL_ZOOM_RATIO, OHOS_CONTROL_CAMERA_MACRO };
}

int32_t PreviewOutput::OnMetadataChanged(
    const camera_device_metadata_tag_t tag, const camera_metadata_item_t& metadataItem)
{
    // Don't trust outer public interface passthrough data. Check the legitimacy of the data.
    if (metadataItem.count <= 0) {
        return CAM_META_INVALID_PARAM;
    }
    if (sketchWrapper_ == nullptr) {
        return CAM_META_FAILURE;
    }
    auto session = GetSession();
    if (session == nullptr) {
        return CAM_META_FAILURE;
    }
    std::lock_guard<std::mutex> lock(asyncOpMutex_);
    SketchWrapper* wrapper = reinterpret_cast<SketchWrapper*>(sketchWrapper_.get());
    wrapper->OnControlMetadataDispatch(session->GetFeaturesMode(), tag, metadataItem);
    return CAM_META_SUCCESS;
}

std::shared_ptr<PreviewStateCallback> PreviewOutput::GetApplicationCallback()
{
    return appCallback_;
}

void PreviewOutput::OnNativeRegisterCallback(const std::string& eventString)
{
    if (eventString == "sketchAvailable") {
        std::lock_guard<std::mutex> lock(asyncOpMutex_);
        if (sketchWrapper_ == nullptr) {
            return;
        }
        auto session = GetSession();
        if (session == nullptr) {
            return;
        }
        if (!session->IsSessionCommited()) {
            return;
        }
        auto metadata = GetDeviceMetadata();
        camera_metadata_item_t item;
        int ret = Camera::FindCameraMetadataItem(metadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
        if (ret != CAM_META_SUCCESS || item.count <= 0) {
            return;
        }
        float tagRatio = *item.data.f;
        if (tagRatio <= 0) {
            return;
        }
        auto sketchWrapper = static_pointer_cast<SketchWrapper>(sketchWrapper_);
        float sketchRatio = sketchWrapper->GetSketchEnableRatio(session->GetFeaturesMode());
        MEDIA_DEBUG_LOG("PreviewOutput::OnMetadataChanged OHOS_CONTROL_ZOOM_RATIO >>> tagRatio:%{public}f -- "
                        "sketchRatio:%{public}f",
            tagRatio, sketchRatio);
        sketchWrapper->UpdateSketchRatio(sketchRatio);
        sketchWrapper->UpdateZoomRatio(tagRatio);
    }
}

void PreviewOutput::OnNativeUnregisterCallback(const std::string& eventString)
{
    if (eventString == "sketchAvailable") {
        std::lock_guard<std::mutex> lock(asyncOpMutex_);
        if (sketchWrapper_ == nullptr) {
            return;
        }
        auto sketchWrapper = static_pointer_cast<SketchWrapper>(sketchWrapper_);
        sketchWrapper->StopSketchStream();
    }
}

void PreviewOutput::CameraServerDied(pid_t pid)
{
    MEDIA_ERR_LOG("camera server has died, pid:%{public}d!", pid);
    if (appCallback_ != nullptr) {
        MEDIA_DEBUG_LOG("appCallback not nullptr");
        int32_t serviceErrorType = ServiceToCameraError(CAMERA_INVALID_STATE);
        appCallback_->OnError(serviceErrorType);
    }
    if (GetStream() != nullptr) {
        (void)GetStream()->AsObject()->RemoveDeathRecipient(deathRecipient_);
    }
    deathRecipient_ = nullptr;
}
} // namespace CameraStandard
} // namespace OHOS
