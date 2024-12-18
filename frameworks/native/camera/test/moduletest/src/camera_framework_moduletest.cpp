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

#include "camera_framework_moduletest.h"

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <memory>
#include <vector>
#include <thread>

#include "accesstoken_kit.h"
#include "aperture_video_session.h"
#include "camera_error_code.h"
#include "camera_log.h"
#include "camera_metadata_operator.h"
#include "camera_output_capability.h"
#include "camera_util.h"
#include "capture_scene_const.h"
#include "capture_session.h"
#include "gtest/gtest.h"
#include "hap_token_info.h"
#include "hcamera_device.h"
#include "hcamera_device_callback_proxy.h"
#include "hcamera_device_proxy.h"
#include "hcamera_service.h"
#include "hcamera_service_stub.h"
#include "input/camera_input.h"
#include "input/camera_manager.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "light_painting_session.h"
#include "nativetoken_kit.h"
#include "night_session.h"
#include "parameter.h"
#include "quick_shot_photo_session.h"
#include "scan_session.h"
#include "session/panorama_session.h"
#include "session/photo_session.h"
#include "session/profession_session.h"
#include "session/photo_session.h"
#include "session/video_session.h"
#include "session/portrait_session.h"
#include "session/secure_camera_session.h"
#include "session/time_lapse_photo_session.h"
#include "session/slow_motion_session.h"
#include "surface.h"
#include "system_ability_definition.h"
#include "test_common.h"
#include "token_setproc.h"
#include "video_session.h"

using namespace testing::ext;
using namespace OHOS::HDI::Camera::V1_0;

namespace OHOS {
namespace CameraStandard {

bool g_camInputOnError = false;
bool g_sessionclosed = false;
bool g_brightnessStatusChanged = false;
bool g_slowMotionStatusChanged = false;
std::shared_ptr<OHOS::Camera::CameraMetadata> g_metaResult = nullptr;
int32_t g_videoFd = -1;
int32_t g_previewFd = -1;
int32_t g_sketchFd = -1;
std::bitset<static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_MAX_EVENT)> g_photoEvents;
std::bitset<static_cast<unsigned int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_MAX_EVENT)> g_previewEvents;
std::bitset<static_cast<unsigned int>(CAM_VIDEO_EVENTS::CAM_VIDEO_MAX_EVENT)> g_videoEvents;
std::bitset<static_cast<unsigned int>(CAM_MACRO_DETECT_EVENTS::CAM_MACRO_EVENT_MAX_EVENT)> g_macroEvents;
std::bitset<static_cast<unsigned int>(CAM_MOON_CAPTURE_BOOST_EVENTS::CAM_MOON_CAPTURE_BOOST_EVENT_MAX_EVENT)>
    g_moonCaptureBoostEvents;
std::list<int32_t> g_sketchStatus;
std::unordered_map<std::string, int> g_camStatusMap;
std::unordered_map<std::string, bool> g_camFlashMap;
TorchStatusInfo g_torchInfo;

void AppCallback::OnCameraStatusChanged(const CameraStatusInfo& cameraDeviceInfo) const
{
    const std::string cameraID = cameraDeviceInfo.cameraDevice->GetID();
    const CameraStatus cameraStatus = cameraDeviceInfo.cameraStatus;
    EXPECT_TRUE(true)<<"OnCameraStatusChanged:CameraId="<<cameraID<<",CameraStatus="<<cameraDeviceInfo.cameraStatus
        <<",bundleName="<<cameraDeviceInfo.bundleName<<endl;
    switch (cameraStatus) {
        case CAMERA_STATUS_UNAVAILABLE: {
        MEDIA_DEBUG_LOG(
                "AppCallback::OnCameraStatusChanged %{public}s: CAMERA_STATUS_UNAVAILABLE", cameraID.c_str());
            g_camStatusMap[cameraID] = CAMERA_STATUS_UNAVAILABLE;
            break;
        }
        case CAMERA_STATUS_AVAILABLE: {
            MEDIA_DEBUG_LOG(
                "AppCallback::OnCameraStatusChanged %{public}s: CAMERA_STATUS_AVAILABLE", cameraID.c_str());
            g_camStatusMap[cameraID] = CAMERA_STATUS_AVAILABLE;
            break;
        }
        default: {
            MEDIA_DEBUG_LOG("AppCallback::OnCameraStatusChanged %{public}s: unknown", cameraID.c_str());
            EXPECT_TRUE(false);
        }
    }
    return;
}

void AppCallback::OnFlashlightStatusChanged(const std::string& cameraID, const FlashStatus flashStatus) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnFlashlightStatusChanged() is called!");
    switch (flashStatus) {
        case FLASH_STATUS_OFF: {
            MEDIA_DEBUG_LOG(
                "AppCallback::OnFlashlightStatusChanged %{public}s: FLASH_STATUS_OFF", cameraID.c_str());
            g_camFlashMap[cameraID] = false;
            break;
        }
        case FLASH_STATUS_ON: {
            MEDIA_DEBUG_LOG("AppCallback::OnFlashlightStatusChanged %{public}s: FLASH_STATUS_ON", cameraID.c_str());
            g_camFlashMap[cameraID] = true;
            break;
        }
        case FLASH_STATUS_UNAVAILABLE: {
            MEDIA_DEBUG_LOG(
                "AppCallback::OnFlashlightStatusChanged %{public}s: FLASH_STATUS_UNAVAILABLE", cameraID.c_str());
            g_camFlashMap.erase(cameraID);
            break;
        }
        default: {
            MEDIA_DEBUG_LOG("AppCallback::OnFlashlightStatusChanged %{public}s: unknown", cameraID.c_str());
            EXPECT_TRUE(false);
        }
    }
    return;
}

void AppCallback::OnTorchStatusChange(const TorchStatusInfo &torchStatusInfo) const
{
    MEDIA_DEBUG_LOG("TorchListener::OnTorchStatusChange called %{public}d %{public}d %{public}f",
        torchStatusInfo.isTorchAvailable, torchStatusInfo.isTorchActive, torchStatusInfo.torchLevel);
    g_torchInfo = torchStatusInfo;
    return;
}

void AppCallback::OnError(const int32_t errorType, const int32_t errorMsg) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnError errorType: %{public}d, errorMsg: %{public}d", errorType, errorMsg);
    g_camInputOnError = true;
    if (errorType == CAMERA_DEVICE_PREEMPTED) {
        g_sessionclosed = true;
    }
    return;
}

void AppCallback::OnResult(const uint64_t timestamp, const std::shared_ptr<OHOS::Camera::CameraMetadata>& result) const
{
    MEDIA_INFO_LOG("CameraDeviceServiceCallback::OnResult() is called!");

    if (result != nullptr) {
        g_metaResult = result;
        common_metadata_header_t* data = result->get();
        std::vector<int32_t> fpsRange;
        camera_metadata_item_t entry;
        int ret = OHOS::Camera::FindCameraMetadataItem(data, OHOS_ABILITY_FPS_RANGES, &entry);
        MEDIA_INFO_LOG("CameraDeviceServiceCallback::FindCameraMetadataItem() %{public}d", ret);
        if (ret != 0) {
            MEDIA_INFO_LOG("demo test: get OHOS_ABILITY_FPS_RANGES error");
        }
        uint32_t count = entry.count;
        MEDIA_INFO_LOG("demo test: fpsRange count  %{public}d", count);
        for (uint32_t i = 0; i < count; i++) {
            fpsRange.push_back(*(entry.data.i32 + i));
        }
        for (auto it = fpsRange.begin(); it != fpsRange.end(); it++) {
            MEDIA_INFO_LOG("demo test: fpsRange %{public}d ", *it);
        }
    }
}

void AppCallback::OnCaptureStarted(const int32_t captureId) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnCaptureStarted captureId: %{public}d", captureId);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] = 1;
    return;
}

void AppCallback::OnCaptureStarted(const int32_t captureId, uint32_t exposureTime) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnCaptureStarted captureId: %{public}d", captureId);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] = 1;
    return;
}

void AppCallback::OnCaptureEnded(const int32_t captureId, const int32_t frameCount) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnCaptureEnded captureId: %{public}d, frameCount: %{public}d",
                    captureId, frameCount);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_END)] = 1;
    return;
}

void AppCallback::OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_DEBUG_LOG(
        "AppCallback::OnFrameShutter captureId: %{public}d, timestamp: %{public}" PRIu64, captureId, timestamp);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_FRAME_SHUTTER)] = 1;
    return;
}

void AppCallback::OnFrameShutterEnd(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_DEBUG_LOG(
        "AppCallback::OnFrameShutterEnd captureId: %{public}d, timestamp: %{public}" PRIu64, captureId, timestamp);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_FRAME_SHUTTER_END)] = 1;
    return;
}

void AppCallback::OnCaptureReady(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_DEBUG_LOG(
        "AppCallback::OnCaptureReady captureId: %{public}d, timestamp: %{public}" PRIu64, captureId, timestamp);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_READY)] = 1;
    return;
}

void AppCallback::OnEstimatedCaptureDuration(const int32_t duration) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnEstimatedCaptureDuration duration: %{public}d", duration);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_ESTIMATED_CAPTURE_DURATION)] = 1;
    return;
}

void AppCallback::OnCaptureError(const int32_t captureId, const int32_t errorCode) const
{
    MEDIA_DEBUG_LOG(
        "AppCallback::OnCaptureError captureId: %{public}d, errorCode: %{public}d", captureId, errorCode);
    g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_ERR)] = 1;
    return;
}

void AppCallback::OnFrameStarted() const
{
    MEDIA_DEBUG_LOG("AppCallback::OnFrameStarted");
    g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_START)] = 1;
    return;
}
void AppCallback::OnFrameEnded(const int32_t frameCount) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnFrameEnded frameCount: %{public}d", frameCount);
    g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_END)] = 1;
    return;
}
void AppCallback::OnError(const int32_t errorCode) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnError errorCode: %{public}d", errorCode);
    g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_ERR)] = 1;
    return;
}
void AppCallback::OnSketchStatusDataChanged(const SketchStatusData& statusData) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnSketchStatusDataChanged");
    g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)] = 1;
    g_sketchStatus.push_back(static_cast<int32_t>(statusData.status));
    return;
}
void AppCallback::OnMacroStatusChanged(MacroStatus status)
{
    MEDIA_DEBUG_LOG("AppCallback::OnMacroStatusChanged");
    if (status == MacroStatus::IDLE) {
        g_macroEvents[static_cast<int>(CAM_MACRO_DETECT_EVENTS::CAM_MACRO_EVENT_IDLE)] = 1;
        g_macroEvents[static_cast<int>(CAM_MACRO_DETECT_EVENTS::CAM_MACRO_EVENT_ACTIVE)] = 0;
    } else if (status == MacroStatus::ACTIVE) {
        g_macroEvents[static_cast<int>(CAM_MACRO_DETECT_EVENTS::CAM_MACRO_EVENT_ACTIVE)] = 1;
        g_macroEvents[static_cast<int>(CAM_MACRO_DETECT_EVENTS::CAM_MACRO_EVENT_IDLE)] = 0;
    }
    return;
}

void AppCallback::OnFeatureDetectionStatusChanged(SceneFeature feature, FeatureDetectionStatus status)
{
    MEDIA_DEBUG_LOG("AppCallback::OnFeatureDetectionStatusChanged");
    if (feature == SceneFeature::FEATURE_MOON_CAPTURE_BOOST) {
        if (status == FeatureDetectionStatus::IDLE) {
            g_moonCaptureBoostEvents[static_cast<int>(
                CAM_MOON_CAPTURE_BOOST_EVENTS::CAM_MOON_CAPTURE_BOOST_EVENT_IDLE)] = 1;
            g_moonCaptureBoostEvents[static_cast<int>(
                CAM_MOON_CAPTURE_BOOST_EVENTS::CAM_MOON_CAPTURE_BOOST_EVENT_ACTIVE)] = 0;
        } else if (status == FeatureDetectionStatus::ACTIVE) {
            g_moonCaptureBoostEvents[static_cast<int>(
                CAM_MOON_CAPTURE_BOOST_EVENTS::CAM_MOON_CAPTURE_BOOST_EVENT_ACTIVE)] = 1;
            g_moonCaptureBoostEvents[static_cast<int>(
                CAM_MOON_CAPTURE_BOOST_EVENTS::CAM_MOON_CAPTURE_BOOST_EVENT_IDLE)] = 0;
        }
    }
}

bool AppCallback::IsFeatureSubscribed(SceneFeature feature)
{
    return true;
}

void AppCallback::OnBrightnessStatusChanged(bool state)
{
    MEDIA_DEBUG_LOG("AppCallback::OnBrightnessStatusChanged");
    g_brightnessStatusChanged = true;
}

void AppCallback::OnSlowMotionState(const SlowMotionState state)
{
    MEDIA_DEBUG_LOG("AppCallback::OnSlowMotionState");
    g_slowMotionStatusChanged = true;
}
void AppCallback::OnFoldStatusChanged(const FoldStatusInfo &foldStatusInfo) const
{
    MEDIA_DEBUG_LOG("AppCallback::OnFoldStatusChanged");
    return;
}
void AppCallback::OnLcdFlashStatusChanged(LcdFlashStatusInfo lcdFlashStatusInfo)
{
    MEDIA_DEBUG_LOG("AppCallback::OnLcdFlashStatusChanged");
}

void AppVideoCallback::OnFrameStarted() const
{
    MEDIA_DEBUG_LOG("AppVideoCallback::OnFrameStarted");
    g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_START)] = 1;
    return;
}
void AppVideoCallback::OnFrameEnded(const int32_t frameCount) const
{
    MEDIA_DEBUG_LOG("AppVideoCallback::OnFrameEnded frameCount: %{public}d", frameCount);
    g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_END)] = 1;
    return;
}
void AppVideoCallback::OnError(const int32_t errorCode) const
{
    MEDIA_DEBUG_LOG("AppVideoCallback::OnError errorCode: %{public}d", errorCode);
    g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_ERR)] = 1;
    return;
}
void AppVideoCallback::OnDeferredVideoEnhancementInfo(const CaptureEndedInfoExt info) const
{
    MEDIA_DEBUG_LOG("AppVideoCallback::OnDeferredVideoEnhancementInfo");
    return;
}

void AppMetadataCallback::OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const
{
    MEDIA_DEBUG_LOG("AppMetadataCallback::OnMetadataObjectsAvailable received");
}
void AppMetadataCallback::OnError(int32_t errorCode) const
{
    MEDIA_DEBUG_LOG("AppMetadataCallback::OnError %{public}d", errorCode);
}

void AppSessionCallback::OnError(int32_t errorCode)
{
    MEDIA_DEBUG_LOG("AppMetadataCallback::OnError %{public}d", errorCode);
    return;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePhotoOutput(int32_t width, int32_t height)
{
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    CameraFormat photoFormat = photoFormat_;
    Size photoSize;
    photoSize.width = width;
    photoSize.height = height;
    Profile photoProfile = Profile(photoFormat, photoSize);
    sptr<CaptureOutput> photoOutput = nullptr;
    sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
    manager_->CreatePhotoOutput(surfaceProducer);
    photoOutput = manager_->CreatePhotoOutput(photoProfile, surfaceProducer);
    return photoOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePhotoOutput()
{
    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(photoWidth_, photoHeight_);
    return photoOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePreviewOutput(int32_t width, int32_t height)
{
    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<SurfaceListener> listener = new SurfaceListener("Preview", SurfaceType::PREVIEW, g_previewFd, previewSurface);
    previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)listener);
    Size previewSize;
    previewSize.width = previewProfiles[0].GetSize().width;
    previewSize.height = previewProfiles[0].GetSize().height;
    previewSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(previewProfiles[0].GetCameraFormat()));
    previewSurface->SetDefaultWidthAndHeight(previewSize.width, previewSize.height);

    sptr<IBufferProducer> bp = previewSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(bp);

    sptr<CaptureOutput> previewOutput = nullptr;
    previewOutput = manager_->CreatePreviewOutput(previewProfiles[0], pSurface);
    return previewOutput;
}

std::shared_ptr<Profile> CameraFrameworkModuleTest::GetSketchPreviewProfile()
{
    std::shared_ptr<Profile> returnProfile;
    Size size720P { 1280, 720 };
    Size size1080P { 1920, 1080 };
    for (const auto& profile : previewProfiles) {
        if (profile.format_ != CAMERA_FORMAT_YUV_420_SP) {
            continue;
        }
        if (profile.size_.width == size720P.width && profile.size_.height == size720P.height) {
            returnProfile = std::make_shared<Profile>(profile);
            return returnProfile;
        }
        if (profile.size_.width == size1080P.width && profile.size_.height == size1080P.height) {
            returnProfile = std::make_shared<Profile>(profile);
            return returnProfile;
        }
        if (profile.size_.height < SKETCH_PREVIEW_MIN_HEIGHT || profile.size_.width > SKETCH_PREVIEW_MAX_WIDTH) {
            continue;
        }
        if (returnProfile == nullptr) {
            returnProfile = std::make_shared<Profile>(profile);
            continue;
        }
        if (profile.size_.width > returnProfile->size_.width) {
            returnProfile = std::make_shared<Profile>(profile);
        }
    }
    return returnProfile;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePreviewOutput(Profile& profile)
{
    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<SurfaceListener> listener = new SurfaceListener("Preview", SurfaceType::PREVIEW, g_previewFd, previewSurface);
    previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)listener);
    Size previewSize = profile.GetSize();
    previewSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(profile.GetCameraFormat()));
    previewSurface->SetDefaultWidthAndHeight(previewSize.width, previewSize.height);

    sptr<IBufferProducer> bp = previewSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(bp);

    sptr<CaptureOutput> previewOutput = nullptr;
    previewOutput = manager_->CreatePreviewOutput(profile, pSurface);
    return previewOutput;
}

sptr<Surface> CameraFrameworkModuleTest::CreateSketchSurface(CameraFormat cameraFormat)
{
    sptr<IConsumerSurface> sketchSurface = IConsumerSurface::Create();
    sptr<SurfaceListener> listener = new SurfaceListener("Sketch", SurfaceType::SKETCH, g_sketchFd, sketchSurface);
    sketchSurface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)listener);
    sketchSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(cameraFormat));
    sketchSurface->SetDefaultWidthAndHeight(SKETCH_DEFAULT_WIDTH, SKETCH_DEFAULT_HEIGHT);
    sptr<IBufferProducer> bp = sketchSurface->GetProducer();
    return Surface::CreateSurfaceAsProducer(bp);
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePreviewOutput()
{
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(previewWidth_, previewHeight_);
    return previewOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreateVideoOutput(int32_t width, int32_t height)
{
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    sptr<SurfaceListener> videoSurfaceListener =
        new (std::nothrow) SurfaceListener("Video", SurfaceType::VIDEO, g_videoFd, surface);
    if (videoSurfaceListener == nullptr) {
        MEDIA_ERR_LOG("Failed to create new SurfaceListener");
        return nullptr;
    }
    surface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)videoSurfaceListener);
    sptr<IBufferProducer> videoProducer = surface->GetProducer();
    sptr<Surface> videoSurface = Surface::CreateSurfaceAsProducer(videoProducer);
    VideoProfile videoProfile = videoProfiles[0];
    sptr<CaptureOutput> videoOutput = nullptr;
    videoOutput = manager_->CreateVideoOutput(videoProfile, videoSurface);
    return videoOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreateVideoOutput(VideoProfile& videoProfile)
{
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    sptr<SurfaceListener> videoSurfaceListener =
        new (std::nothrow) SurfaceListener("Video", SurfaceType::VIDEO, g_videoFd, surface);
    if (videoSurfaceListener == nullptr) {
        MEDIA_ERR_LOG("Failed to create new SurfaceListener");
        return nullptr;
    }
    surface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)videoSurfaceListener);
    sptr<IBufferProducer> videoProducer = surface->GetProducer();
    sptr<Surface> videoSurface = Surface::CreateSurfaceAsProducer(videoProducer);
    sptr<CaptureOutput> videoOutput = nullptr;
    videoOutput = manager_->CreateVideoOutput(videoProfile, videoSurface);
    return videoOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreateVideoOutput()
{
    sptr<CaptureOutput> videoOutput = CreateVideoOutput(videoWidth_, videoHeight_);
    return videoOutput;
}

sptr<CaptureOutput> CameraFrameworkModuleTest::CreatePhotoOutput(Profile profile)
{
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    Size photoSize;
    photoSize.width = profile.GetSize().width;
    photoSize.height = profile.GetSize().height;
    sptr<CaptureOutput> photoOutput = nullptr;
    sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
    photoOutput = manager_->CreatePhotoOutput(profile, surfaceProducer);
    return photoOutput;
}

void CameraFrameworkModuleTest::ConfigScanSession(sptr<CaptureOutput> &previewOutput_1,
                                                  sptr<CaptureOutput> &previewOutput_2)
{
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
    }
    scanSession_ = manager_ -> CreateCaptureSession(SceneMode::SCAN);
    ASSERT_NE(scanSession_, nullptr);

    int32_t intResult = scanSession_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = scanSession_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    previewOutput_1 = CreatePreviewOutput();
    ASSERT_NE(previewOutput_1, nullptr);

    intResult = scanSession_->AddOutput(previewOutput_1);
    EXPECT_EQ(intResult, 0);

    previewOutput_2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput_2, nullptr);

    intResult = scanSession_->AddOutput(previewOutput_2);
    EXPECT_EQ(intResult, 0);

    intResult = scanSession_->CommitConfig();
    EXPECT_EQ(intResult, 0);
}

void CameraFrameworkModuleTest::ConfigHighResSession(sptr<CaptureOutput> &previewOutput,
                                                     sptr<CaptureOutput> &photoOutput)
{
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
        input_->Close();
    }

    cameras_ = manager_->GetSupportedCameras();
    ASSERT_TRUE(cameras_.size() != 0);

    std::cout<<std::endl;

    sptr<CameraDevice> device;
    for (auto deviceEach : cameras_) {
        if (deviceEach->GetPosition() == 1 && deviceEach->GetCameraType() == CameraType::CAMERA_TYPE_WIDE_ANGLE) {
            device = deviceEach;
            break;
        }
    }
    sptr<CaptureInput> input = manager_->CreateCameraInput(device);
    ASSERT_NE(input, nullptr);

    int32_t intResult = input->Open();
    EXPECT_EQ(intResult, 0);

    highResSession_ = manager_ -> CreateCaptureSession(SceneMode::HIGH_RES_PHOTO);
    ASSERT_NE(highResSession_, nullptr);

    intResult = highResSession_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = highResSession_->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CameraOutputCapability> capability =
        manager_->GetSupportedOutputCapability(device, SceneMode::HIGH_RES_PHOTO);

    CreateHighResPhotoOutput(previewOutput, photoOutput,
                             capability->previewProfiles_[0], capability->photoProfiles_[0]);
    ASSERT_NE(previewOutput, nullptr);
    ASSERT_NE(photoOutput, nullptr);

    intResult = highResSession_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = highResSession_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = highResSession_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::vector<float> zoomRatioRange = highResSession_->GetZoomRatioRange();
    ASSERT_NE(zoomRatioRange.size(), 0);
}

void CameraFrameworkModuleTest::ConfigSlowMotionSession(sptr<CaptureOutput> &previewOutput_frame,
                                                        sptr<CaptureOutput> &videoOutput_frame)
{
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
    }
    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::SLOW_MOTION);
    slowMotionSession_ = static_cast<SlowMotionSession*>(captureSession.GetRefPtr());
    slowMotionSession_->SetCallback(std::make_shared<AppCallback>());
    ASSERT_NE(slowMotionSession_, nullptr);

    int32_t intResult = slowMotionSession_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = slowMotionSession_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    Profile previewProfile = previewProfiles[0];
    const int32_t sizeOfWidth = 1920;
    const int32_t sizeOfHeight = 1080;
    for (auto item : previewProfiles) {
        if (item.GetSize().width == sizeOfWidth && item.GetSize().height == sizeOfHeight) {
            previewProfile = item;
            break;
        }
    }
    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<SurfaceListener> listener = new SurfaceListener("Preview", SurfaceType::PREVIEW, g_previewFd, previewSurface);
    previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)listener);
    Size previewSize;
    previewSize.width = previewProfile.GetSize().width;
    previewSize.height = previewProfile.GetSize().height;
    previewSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(previewProfile.GetCameraFormat()));
    previewSurface->SetDefaultWidthAndHeight(previewSize.width, previewSize.height);
    sptr<IBufferProducer> bp = previewSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(bp);
    sptr<CaptureOutput> previewOutput = nullptr;
    previewOutput_frame = manager_->CreatePreviewOutput(previewProfile, pSurface);
    ASSERT_NE(previewOutput_frame, nullptr);

    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    sptr<SurfaceListener> videoSurfaceListener =
        new (std::nothrow) SurfaceListener("Video", SurfaceType::VIDEO, g_videoFd, surface);
    surface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)videoSurfaceListener);
    ASSERT_NE(videoSurfaceListener, nullptr);

    sptr<IBufferProducer> videoProducer = surface->GetProducer();
    sptr<Surface> videoSurface = Surface::CreateSurfaceAsProducer(videoProducer);
    std::vector<int32_t> fps = {120, 120};
    Size size = {.width = 1920, .height = 1080};
    VideoProfile videoProfile = VideoProfile(CameraFormat::CAMERA_FORMAT_YUV_420_SP, size, fps);
    videoOutput_frame = manager_->CreateVideoOutput(videoProfile, videoSurface);
    ASSERT_NE(videoOutput_frame, nullptr);
}

void CameraFrameworkModuleTest::CreateHighResPhotoOutput(sptr<CaptureOutput> &previewOutput,
                                                         sptr<CaptureOutput> &photoOutput,
                                                         Profile previewProfile,
                                                         Profile photoProfile)
{
    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<SurfaceListener> listener = new SurfaceListener("Preview", SurfaceType::PREVIEW, g_previewFd, previewSurface);
    previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)listener);
    Size previewSize;
    previewSize.width = previewProfile.GetSize().width;
    previewSize.height = previewProfile.GetSize().height;
    previewSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(previewProfile.GetCameraFormat()));
    previewSurface->SetDefaultWidthAndHeight(previewSize.width, previewSize.height);

    sptr<IBufferProducer> bp = previewSurface->GetProducer();
    sptr<Surface> consumerInPreview = Surface::CreateSurfaceAsProducer(bp);
    previewOutput = manager_->CreatePreviewOutput(previewProfile, consumerInPreview);

    sptr<IConsumerSurface> photoSurface = IConsumerSurface::Create();
    Size photoSize;
    photoSize.width = photoProfile.GetSize().width;
    photoSize.height = photoProfile.GetSize().height;
    sptr<IBufferProducer> surfaceProducerOfPhoto = photoSurface->GetProducer();
    photoOutput = manager_->CreatePhotoOutput(photoProfile, surfaceProducerOfPhoto);
}

void CameraFrameworkModuleTest::ConfigVideoSession(sptr<CaptureOutput> &previewOutput_frame,
                                                   sptr<CaptureOutput> &videoOutput_frame)
{
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
    }
    videoSession_ = manager_ -> CreateCaptureSession(SceneMode::VIDEO);
    ASSERT_NE(videoSession_, nullptr);

    int32_t intResult = videoSession_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = videoSession_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    Profile previewProfile = previewProfiles[0];
    const int32_t sizeOfWidth = 1920;
    const int32_t sizeOfHeight = 1080;
    for (auto item : previewProfiles) {
        if (item.GetSize().width == sizeOfWidth && item.GetSize().height == sizeOfHeight) {
            previewProfile = item;
            break;
        }
    }
    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<SurfaceListener> listener = new SurfaceListener("Preview", SurfaceType::PREVIEW, g_previewFd, previewSurface);
    previewSurface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)listener);
    Size previewSize;
    previewSize.width = previewProfile.GetSize().width;
    previewSize.height = previewProfile.GetSize().height;
    previewSurface->SetUserData(CameraManager::surfaceFormat, std::to_string(previewProfile.GetCameraFormat()));
    previewSurface->SetDefaultWidthAndHeight(previewSize.width, previewSize.height);
    sptr<IBufferProducer> bp = previewSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(bp);
    sptr<CaptureOutput> previewOutput = nullptr;
    previewOutput_frame = manager_->CreatePreviewOutput(previewProfile, pSurface);
    ASSERT_NE(previewOutput_frame, nullptr);

    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    sptr<SurfaceListener> videoSurfaceListener =
        new (std::nothrow) SurfaceListener("Video", SurfaceType::VIDEO, g_videoFd, surface);
    ASSERT_NE(videoSurfaceListener, nullptr);
    surface->RegisterConsumerListener((sptr<IBufferConsumerListener>&)videoSurfaceListener);

    sptr<IBufferProducer> videoProducer = surface->GetProducer();
    sptr<Surface> videoSurface = Surface::CreateSurfaceAsProducer(videoProducer);
    VideoProfile videoProfile = videoProfiles[0];
    for (auto item : videoProfiles) {
        if (item.GetSize().width == sizeOfWidth && item.GetSize().height == sizeOfHeight) {
            videoProfile = item;
            break;
        }
    }
    videoOutput_frame = manager_->CreateVideoOutput(videoProfile, videoSurface);
    ASSERT_NE(videoOutput_frame, nullptr);
}

void CameraFrameworkModuleTest::GetSupportedOutputCapability()
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraDevice>> cameraObjList = camManagerObj->GetSupportedCameras();
    ASSERT_GE(cameraObjList.size(), CAMERA_NUMBER);
    sptr<CameraOutputCapability> outputcapability = camManagerObj->GetSupportedOutputCapability(cameraObjList[1]);
    previewProfiles = outputcapability->GetPreviewProfiles();
    ASSERT_TRUE(!previewProfiles.empty());

    photoProfiles = outputcapability->GetPhotoProfiles();
    ASSERT_TRUE(!photoProfiles.empty());

    videoProfiles = outputcapability->GetVideoProfiles();
    ASSERT_TRUE(!videoProfiles.empty());
    return;
}

Profile CameraFrameworkModuleTest::SelectProfileByRatioAndFormat(sptr<CameraOutputCapability>& modeAbility,
                                                                 camera_rational_t ratio, CameraFormat format)
{
    uint32_t width;
    uint32_t height;
    Profile profile;
    std::vector<Profile> profiles;

    if (format == photoFormat_) {
        profiles = modeAbility->GetPhotoProfiles();
    } else if (format == previewFormat_) {
        profiles = modeAbility->GetPreviewProfiles();
    }

    for (int i = 0; i < profiles.size(); ++i) {
        width = profiles[i].GetSize().width;
        height = profiles[i].GetSize().height;
        if ((width % ratio.numerator == 0) && (height % ratio.denominator == 0)) {
            profile = profiles[i];
            break;
        }
    }
    MEDIA_ERR_LOG("SelectProfileByRatioAndFormat format:%{public}d width:%{public}d height:%{public}d",
        profile.format_, profile.size_.width, profile.size_.height);
    return profile;
}

SelectProfiles CameraFrameworkModuleTest::SelectWantedProfiles(
    sptr<CameraOutputCapability>& modeAbility, const SelectProfiles wanted)
{
    SelectProfiles ret;
    ret.preview.format_ = CAMERA_FORMAT_INVALID;
    ret.photo.format_ = CAMERA_FORMAT_INVALID;
    ret.video.format_ = CAMERA_FORMAT_INVALID;
    vector<Profile> previewProfiles = modeAbility->GetPreviewProfiles();
    vector<Profile> photoProfiles = modeAbility->GetPhotoProfiles();
    vector<VideoProfile> videoProfiles = modeAbility->GetVideoProfiles();
    const auto& preview = std::find_if(previewProfiles.begin(),
                                       previewProfiles.end(),
                                       [&wanted](auto& profile) { return profile == wanted.preview; });
    if (preview != previewProfiles.end()) {
        ret.preview = *preview;
    } else {
        MEDIA_ERR_LOG("preview format:%{public}d width:%{public}d height:%{public}d not support",
            wanted.preview.format_, wanted.preview.size_.width, wanted.preview.size_.height);
    }
    const auto& photo = std::find_if(photoProfiles.begin(), photoProfiles.end(),
                                     [&wanted](auto& profile) { return profile == wanted.photo; });
    if (photo != photoProfiles.end()) {
        ret.photo = *photo;
    } else {
        MEDIA_ERR_LOG("photo format:%{public}d width:%{public}d height:%{public}d not support",
            wanted.photo.format_, wanted.photo.size_.width, wanted.photo.size_.height);
    }
    const auto& video = std::find_if(videoProfiles.begin(), videoProfiles.end(),
                                     [&wanted](auto& profile) { return profile == wanted.video; });
    if (video != videoProfiles.end()) {
        ret.video = *video;
    } else {
        MEDIA_ERR_LOG("video format:%{public}d width:%{public}d height:%{public}d not support",
            wanted.video.format_, wanted.video.size_.width, wanted.video.size_.height);
    }
    return ret;
}

void CameraFrameworkModuleTest::ReleaseInput()
{
    if (input_) {
        sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
        camInput->Close();
        input_->Release();
    }
    return;
}

void CameraFrameworkModuleTest::SetCameraParameters(sptr<CaptureSession>& session, bool video)
{
    session->LockForControl();

    std::vector<float> zoomRatioRange = session->GetZoomRatioRange();
    if (!zoomRatioRange.empty()) {
        session->SetZoomRatio(zoomRatioRange[0]);
    }

    // GetExposureBiasRange
    std::vector<float> exposureBiasRange = session->GetExposureBiasRange();
    if (!exposureBiasRange.empty()) {
        session->SetExposureBias(exposureBiasRange[0]);
    }

    // Get/Set Exposurepoint
    Point exposurePoint = { 1, 2 };
    session->SetMeteringPoint(exposurePoint);

    // GetFocalLength
    float focalLength = session->GetFocalLength();
    EXPECT_NE(focalLength, 0);

    // Get/Set focuspoint
    Point focusPoint = { 1, 2 };
    session->SetFocusPoint(focusPoint);

    FlashMode flash = FLASH_MODE_OPEN;
    if (video) {
        flash = FLASH_MODE_ALWAYS_OPEN;
    }
    session->SetFlashMode(flash);

    FocusMode focus = FOCUS_MODE_AUTO;
    session->SetFocusMode(focus);

    ExposureMode exposure = EXPOSURE_MODE_AUTO;
    session->SetExposureMode(exposure);

    session->UnlockForControl();

    Point exposurePointGet = session->GetMeteringPoint();
    EXPECT_EQ(exposurePointGet.x, exposurePoint.x > 1 ? 1 : exposurePoint.x);
    EXPECT_EQ(exposurePointGet.y, exposurePoint.y > 1 ? 1 : exposurePoint.y);

    Point focusPointGet = session->GetFocusPoint();
    EXPECT_EQ(focusPointGet.x, focusPoint.x > 1 ? 1 : focusPoint.x);
    EXPECT_EQ(focusPointGet.y, focusPoint.y > 1 ? 1 : focusPoint.y);

    // exposureBiasRange
    if (!exposureBiasRange.empty()) {
        EXPECT_EQ(session->GetExposureValue(), exposureBiasRange[0]);
    }

    EXPECT_EQ(session->GetFlashMode(), flash);
    EXPECT_EQ(session->GetFocusMode(), focus);
    EXPECT_EQ(session->GetExposureMode(), exposure);
}

void CameraFrameworkModuleTest::TestCallbacksSession(sptr<CaptureOutput> photoOutput, sptr<CaptureOutput> videoOutput)
{
    int32_t intResult;

    if (videoOutput != nullptr) {
        intResult = session_->Start();
        EXPECT_EQ(intResult, 0);

        intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
        EXPECT_EQ(intResult, 0);
        sleep(WAIT_TIME_AFTER_START);
    }

    if (photoOutput != nullptr) {
        intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
        EXPECT_EQ(intResult, 0);
    }

    if (videoOutput != nullptr) {
        intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
        EXPECT_EQ(intResult, 0);
    }

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

void CameraFrameworkModuleTest::TestCallbacks(sptr<CameraDevice>& cameraInfo, bool video)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    // Register error callback
    RegisterErrorCallback();

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = nullptr;
    sptr<CaptureOutput> videoOutput = nullptr;
    if (!video) {
        photoOutput = CreatePhotoOutput();
        ASSERT_NE(photoOutput, nullptr);

        // Register photo callback
        ((sptr<PhotoOutput>&)photoOutput)->SetCallback(std::make_shared<AppCallback>());
        intResult = session_->AddOutput(photoOutput);
    } else {
        videoOutput = CreateVideoOutput();
        ASSERT_NE(videoOutput, nullptr);

        // Register video callback
        ((sptr<VideoOutput>&)videoOutput)->SetCallback(std::make_shared<AppVideoCallback>());
        intResult = session_->AddOutput(videoOutput);
    }

    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    // Register preview callback
    ((sptr<PreviewOutput>&)previewOutput)->SetCallback(std::make_shared<AppCallback>());
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    SetCameraParameters(session_, video);

    /* In case of wagner device, once commit config is done with flash on
    it is not giving the flash status callback, removing it */
    EXPECT_TRUE(g_photoEvents.none());
    EXPECT_TRUE(g_previewEvents.none());
    EXPECT_TRUE(g_videoEvents.none());

    TestCallbacksSession(photoOutput, videoOutput);

    if (photoOutput != nullptr) {
        if (IsSupportNow()) {
            EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] == 0);
        } else {
            EXPECT_TRUE(g_photoEvents[static_cast<int>(CAM_PHOTO_EVENTS::CAM_PHOTO_CAPTURE_START)] == 1);
        }
        ((sptr<PhotoOutput>&)photoOutput)->Release();
    }

    if (videoOutput != nullptr) {
        EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_FRAME_START)], 1);

        TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

        EXPECT_EQ(g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_START)], 1);
        EXPECT_EQ(g_videoEvents[static_cast<int>(CAM_VIDEO_EVENTS::CAM_VIDEO_FRAME_END)], 1);

        ((sptr<VideoOutput>&)videoOutput)->Release();
    }

    ((sptr<PreviewOutput>&)previewOutput)->Release();
}

void CameraFrameworkModuleTest::RegisterErrorCallback()
{
    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->SetErrorCallback(callback);

    EXPECT_EQ(g_camInputOnError, false);
}

bool CameraFrameworkModuleTest::IsSupportNow()
{
    const char* deviveTypeString = GetDeviceType();
    std::string deviveType = std::string(deviveTypeString);
    if (deviveType.compare("default") == 0 ||
        (cameras_[0] != nullptr && cameras_[0]->GetConnectionType() == CAMERA_CONNECTION_USB_PLUGIN)) {
        return false;
    }
    return true;
}

bool CameraFrameworkModuleTest::IsSupportMode(SceneMode mode)
{
    std::vector<SceneMode> modes = manager_->GetSupportedModes(cameras_[0]);
    if (modes.size() == 0) {
        MEDIA_ERR_LOG("IsSupportMode: modes.size is null");
        return false;
    }
    bool supportMode = false;
    for (auto &it : modes) {
        if (it == mode) {
            supportMode = true;
            break;
        }
    }
    return supportMode;
}

void CameraFrameworkModuleTest::SetUpTestCase(void)
{
    MEDIA_INFO_LOG("SetUpTestCase of camera test case!");
    // set native token
    SetNativeToken();
    // set hap token please use SetHapToken();
}

void CameraFrameworkModuleTest::TearDownTestCase(void)
{
    MEDIA_INFO_LOG("TearDownTestCase of camera test case!");
}

void CameraFrameworkModuleTest::SetUpInit()
{
    MEDIA_INFO_LOG("SetUpInit of camera test case!");
    g_photoEvents.reset();
    g_previewEvents.reset();
    g_videoEvents.reset();
    g_moonCaptureBoostEvents.reset();
    g_macroEvents.reset();
    g_camStatusMap.clear();
    g_camFlashMap.clear();
    g_sketchStatus.clear();
    g_camInputOnError = false;
    g_videoFd = -1;
    g_previewFd = -1;
    previewFormat_ = CAMERA_FORMAT_YUV_420_SP;
    videoFormat_ = CAMERA_FORMAT_YUV_420_SP;
    photoFormat_ = CAMERA_FORMAT_JPEG;
    previewWidth_ = PREVIEW_DEFAULT_WIDTH;
    previewHeight_ = PREVIEW_DEFAULT_HEIGHT;
    photoWidth_ = PHOTO_DEFAULT_WIDTH;
    photoHeight_ = PHOTO_DEFAULT_HEIGHT;
    videoWidth_ = VIDEO_DEFAULT_WIDTH;
    videoHeight_ = VIDEO_DEFAULT_HEIGHT;
}

void CameraFrameworkModuleTest::SetUp()
{
    MEDIA_INFO_LOG("SetUp");
    SetUpInit();
    // set hap token please use SetHapToken();
    manager_ = CameraManager::GetInstance();
    ASSERT_NE(manager_, nullptr);
    manager_->SetCallback(std::make_shared<AppCallback>());

    cameras_ = manager_->GetSupportedCameras();
    ASSERT_TRUE(cameras_.size() != 0);

    input_ = manager_->CreateCameraInput(cameras_[0]);
    ASSERT_NE(input_, nullptr);

    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    std::vector<sptr<CameraDevice>> cameraObjList = camManagerObj->GetSupportedCameras();
    sptr<CameraOutputCapability> outputcapability = camManagerObj->GetSupportedOutputCapability(cameraObjList[0]);

    ProcessPreviewProfiles(outputcapability);
    ASSERT_TRUE(!previewFormats_.empty());
    ASSERT_TRUE(!previewSizes_.empty());
    if (std::find(previewFormats_.begin(), previewFormats_.end(), CAMERA_FORMAT_YUV_420_SP) != previewFormats_.end()) {
        previewFormat_ = CAMERA_FORMAT_YUV_420_SP;
    } else {
        previewFormat_ = previewFormats_[0];
    }
    photoProfiles = outputcapability->GetPhotoProfiles();

    for (auto i : photoProfiles) {
        photoFormats_.push_back(i.GetCameraFormat());
        photoSizes_.push_back(i.GetSize());
    }
    ASSERT_TRUE(!photoFormats_.empty());
    ASSERT_TRUE(!photoSizes_.empty());
    photoFormat_ = photoFormats_[0];
    videoProfiles = outputcapability->GetVideoProfiles();

    for (auto i : videoProfiles) {
        videoFormats_.push_back(i.GetCameraFormat());
        videoSizes_.push_back(i.GetSize());
        videoFrameRates_ = i.GetFrameRates();
    }
    ASSERT_TRUE(!videoFormats_.empty());
    ASSERT_TRUE(!videoSizes_.empty());
    ASSERT_TRUE(!videoFrameRates_.empty());
    if (std::find(videoFormats_.begin(), videoFormats_.end(), CAMERA_FORMAT_YUV_420_SP) != videoFormats_.end()) {
        videoFormat_ = CAMERA_FORMAT_YUV_420_SP;
    } else {
        videoFormat_ = videoFormats_[0];
    }
    ProcessSize();
}

void CameraFrameworkModuleTest::TearDown()
{
    MEDIA_INFO_LOG("TearDown start");
    if (session_) {
        session_->Release();
    }
    if (scanSession_) {
        scanSession_->Release();
    }
    if (input_) {
        sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
        camInput->Close();
        input_->Release();
    }
    MEDIA_INFO_LOG("TearDown end");
}

void CameraFrameworkModuleTest::SetNativeToken()
{
    uint64_t tokenId;
    const char* perms[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.CAMERA";
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = NULL,
        .perms = perms,
        .acls = NULL,
        .processName = "native_camera_tdd",
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void CameraFrameworkModuleTest::SetHapToken()
{
    static Security::AccessToken::PermissionStateFull cameraPermission = {
        .permissionName = "ohos.permission.CAMERA",
        .isGeneral = true,
        .resDeviceID = {"local"},
        .grantStatus = {Security::AccessToken::PermissionState::PERMISSION_GRANTED},
        .grantFlags = {1}
    };

    static Security::AccessToken::PermissionStateFull distributePermission = {
        .permissionName = "ohos.permission.DISTRIBUTED_DATASYNC",
        .isGeneral = true,
        .resDeviceID = {"local"},
        .grantStatus = {Security::AccessToken::PermissionState::PERMISSION_GRANTED},
        .grantFlags = {1}
    };

    static Security::AccessToken::HapPolicyParams cameraFwkMstPolicy = {
        .apl = Security::AccessToken::APL_SYSTEM_BASIC,
        .domain = "test.domain",
        // 权限定义，为空即可
        .permList = {},
        // 要授予的权限
        .permStateList = { cameraPermission, distributePermission },
    };

    // 模拟应用的参数
    Security::AccessToken::HapInfoParams hapInfo = {
    .userID = 0,
    .bundleName = "com.ohos.cameraFwk.Mst",
    .instIndex = 0,
    .appIDDesc = "com.ohos.cameraFwk.Mst",
    .apiVersion = 0,
    .isSystemApp = true
};
    Security::AccessToken::AccessTokenIDEx tokenIdEx = {0};
    // 获取模拟hap的tokenID
    tokenIdEx = Security::AccessToken::AccessTokenKit::AllocHapToken(hapInfo, cameraFwkMstPolicy);
    // 将模拟的应用tokenID设置给本测试进程
    SetSelfTokenID(tokenIdEx.tokenIDEx);
}

void CameraFrameworkModuleTest::ProcessPreviewProfiles(sptr<CameraOutputCapability>& outputcapability)
{
    previewProfiles.clear();
    std::vector<Profile> tempPreviewProfiles = outputcapability->GetPreviewProfiles();
    for (const auto& profile : tempPreviewProfiles) {
        if ((profile.size_.width == PRVIEW_WIDTH_176 && profile.size_.height == PRVIEW_HEIGHT_144) ||
            (profile.size_.width == PRVIEW_WIDTH_480 && profile.size_.height == PRVIEW_HEIGHT_480) ||
            (profile.size_.width == PRVIEW_WIDTH_640 && profile.size_.height == PRVIEW_WIDTH_640) ||
            (profile.size_.width == PRVIEW_WIDTH_4096 && profile.size_.height == PRVIEW_HEIGHT_3072) ||
            (profile.size_.width == PRVIEW_WIDTH_4160 && profile.size_.height == PRVIEW_HEIGHT_3120) ||
            (profile.size_.width == PRVIEW_WIDTH_8192 && profile.size_.height == PRVIEW_HEIGHT_6144)) {
            MEDIA_DEBUG_LOG("SetUp skip previewProfile width:%{public}d height:%{public}d",
                profile.size_.width, profile.size_.height);
            continue;
        }
        previewProfiles.push_back(profile);
    }

    for (auto i : previewProfiles) {
        MEDIA_DEBUG_LOG("SetUp previewProfiles width:%{public}d height:%{public}d", i.size_.width, i.size_.height);
        previewFormats_.push_back(i.GetCameraFormat());
        previewSizes_.push_back(i.GetSize());
    }
}

void CameraFrameworkModuleTest::ProcessSize()
{
    Size size = previewSizes_.back();
    previewWidth_ = size.width;
    previewHeight_ = size.height;
    size = photoSizes_.back();
    photoWidth_ = size.width;
    photoHeight_ = size.height;
    size = videoSizes_.back();
    videoWidth_ = size.width;
    videoHeight_ = size.height;

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->Open();
    session_ = manager_->CreateCaptureSession();
    ASSERT_NE(session_, nullptr);
}

void CameraFrameworkModuleTest::ProcessPortraitSession(sptr<PortraitSession>& portraitSession,
    sptr<CaptureOutput>& previewOutput)
{
    int32_t intResult = portraitSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    portraitSession->LockForControl();

    std::vector<PortraitEffect> effects = portraitSession->GetSupportedPortraitEffects();
    if (!effects.empty()) {
        portraitSession->SetPortraitEffect(effects[0]);
    }

    portraitSession->UnlockForControl();

    if (!effects.empty()) {
        EXPECT_EQ(portraitSession->GetPortraitEffect(), effects[0]);
    }
}

sptr<CameraDevice> CameraFrameworkModuleTest::ChooseCamerasByPositionAndType(CameraPosition position, CameraType type)
{
    sptr<CameraDevice> choosedCamera = nullptr;
    for (auto it : cameras_) {
        if (it->GetPosition() == position && it->GetCameraType() == type) {
            choosedCamera = it;
            break;
        }
    }
    return choosedCamera;
}

/*
 * Feature: Framework
 * Function: Test get distributed camera hostname
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test get distributed camera hostname
 */
HWTEST_F(CameraFrameworkModuleTest, Camera_fwInfoManager_moduletest_001, TestSize.Level0)
{
    std::string hostName;
    for (size_t i = 0; i < cameras_.size(); i++) {
        hostName = cameras_[i]->GetHostName();
        std::string cameraId = cameras_[i]->GetID();
        std::string networkId = cameras_[i]->GetNetWorkId();
        if (networkId != "") {
            ASSERT_NE(hostName, "");
        } else {
            ASSERT_EQ(hostName, "");
        }
    }
}
/*
 * Feature: Framework
 * Function: Test get DeviceType
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test get DeviceType
 */
HWTEST_F(CameraFrameworkModuleTest, Camera_fwInfoManager_moduletest_002, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = manager_->GetSupportedCameras();
    auto judgeDeviceType = [&cameras]() -> bool {
        bool isOk = false;
        for (size_t i = 0; i < cameras.size(); i++) {
            uint16_t deviceType = cameras[i]->GetDeviceType();
            switch (deviceType) {
                case HostDeviceType::UNKNOWN:
                case HostDeviceType::PHONE:
                case HostDeviceType::TABLET:
                    isOk = true;
                    break;
                default:
                    isOk = false;
                    break;
            }
            if (isOk == false) {
                break;
            }
        }
        return isOk;
    };
    ASSERT_NE(judgeDeviceType(), false);
}

/*
 * Feature: Framework
 * Function: Test Capture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_001, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    ((sptr<PhotoOutput>&)photoOutput)->Release();
}
/*
 * Feature: Framework
 * Function: Test Capture + Preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture + Preview
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_002, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Preview + Video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview + Video
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_003, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test camera status, flash, camera input, photo output and preview output callbacks
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test callbacks
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_004, TestSize.Level0)
{
    TestCallbacks(cameras_[0], false);
}

/*
 * Feature: Framework
 * Function: Test camera status, flash, camera input, preview output and video output callbacks
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test callbacks
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_005, TestSize.Level0)
{
    TestCallbacks(cameras_[0], true);
}

/*
 * Feature: Framework
 * Function: Test Preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_006, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Video
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_007, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    // Video mode without preview is not supported
}

/*
 * Feature: Framework
 * Function: Test Custom Preview with invalid resolutions
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Custom Preview with invalid resolution(0 * 0)
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_011, TestSize.Level0)
{
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
}

/*
 * Feature: Framework
 * Function: Test capture session with commit config multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with commit config multiple times
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_017, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session add input with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add input with invalid value
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_018, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input1 = nullptr;
    intResult = session_->AddInput(input1);
    EXPECT_NE(intResult, 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session add output with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add output with invalid value
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_019, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = nullptr;
    intResult = session_->AddOutput(previewOutput);
    EXPECT_NE(intResult, 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding input
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_020, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 7400201);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding output
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_021, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop without adding preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop without adding preview output
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_022, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session without begin config
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session without begin config
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_023, TestSize.Level0)
{
    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    int32_t intResult = session_->AddInput(input_);
    EXPECT_NE(intResult, 0);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_NE(intResult, 0);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_NE(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_NE(intResult, 0);

    intResult = session_->Start();
    EXPECT_NE(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_NE(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_NE(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple photo outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple photo outputs
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_024, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput1 = CreatePhotoOutput();
    ASSERT_NE(photoOutput1, nullptr);

    intResult = session_->AddOutput(photoOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput2 = CreatePhotoOutput();
    ASSERT_NE(photoOutput2, nullptr);

    intResult = session_->AddOutput(photoOutput2);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput1)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<PhotoOutput>&)photoOutput2)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session_->Stop();

    ((sptr<PhotoOutput>&)photoOutput1)->Release();
    ((sptr<PhotoOutput>&)photoOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple preview outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple preview ouputs
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_025, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput1 = CreatePreviewOutput();
    ASSERT_NE(previewOutput1, nullptr);

    intResult = session_->AddOutput(previewOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput2, nullptr);

    intResult = session_->AddOutput(previewOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput1)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput2)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    ((sptr<PreviewOutput>&)previewOutput1)->Release();
    ((sptr<PreviewOutput>&)previewOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple video outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple video ouputs
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_026, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput1 = CreateVideoOutput();
    ASSERT_NE(videoOutput1, nullptr);

    intResult = session_->AddOutput(videoOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput2 = CreateVideoOutput();
    ASSERT_NE(videoOutput2, nullptr);

    intResult = session_->AddOutput(videoOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<VideoOutput>&)videoOutput1)->Start();
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<VideoOutput>&)videoOutput2)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput1)->Stop();
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<VideoOutput>&)videoOutput2)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session_->Stop();

    ((sptr<VideoOutput>&)videoOutput1)->Release();
    ((sptr<VideoOutput>&)videoOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop preview multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop preview multiple times
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_027, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();

    sleep(WAIT_TIME_AFTER_START);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop video multiple times
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop video multiple times
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_028, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test remove video output and commit when preview + video outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove video output and commit when preview + video outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_029, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test remove video output, add photo output and commit when preview + video outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove video output, add photo output and commit when preview + video outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_030, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test remove photo output and commit when preview + photo outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove photo output and commit when preview + photo outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_031, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test remove photo output, add video output and commit when preview + photo outputs were committed
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test remove photo output, add video output and commit when preview + photo outputs were committed
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_032, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session remove output with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove output with null
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_033, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> output = nullptr;
    intResult = session_->RemoveOutput(output);
    EXPECT_NE(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test capture session remove input with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove input with null
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_034, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = nullptr;
    intResult = session_->RemoveInput(input);
    EXPECT_NE(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test Capture with location setting [lat:1 ,long:1 ,alt:1]
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_035, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::shared_ptr<Location> location = std::make_shared<Location>();
    location->latitude = 1;
    location->longitude = 1;
    location->altitude = 1;

    photoSetting->SetLocation(location);
    photoSetting->SetLocation(location);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Capture with location setting [lat:0.0 ,long:0.0 ,alt:0.0]
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_036, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::shared_ptr<Location> location = std::make_shared<Location>();
    location->latitude = 0.0;
    location->longitude = 0.0;
    location->altitude = 0.0;

    photoSetting->SetLocation(location);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Capture with location setting [lat:-1 ,long:-1 ,alt:-1]
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Capture with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_037, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::shared_ptr<Location> location = std::make_shared<Location>();
    location->latitude = -1;
    location->longitude = -1;
    location->altitude = -1;

    photoSetting->SetLocation(location);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test snapshot
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test snapshot
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_038, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test snapshot with location setting
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test snapshot with location setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_039, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::shared_ptr<Location> location = std::make_shared<Location>();
    location->latitude = 12.972442;
    location->longitude = 77.580643;
    location->altitude = 0;

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test snapshot with mirror setting
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test snapshot with mirror setting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_040, TestSize.Level0)
{
    ReleaseInput();
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetMirror(true);
    photoSetting->SetMirror(true);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    if (cameras_.size() < 2) {
        return;
    }

    sptr<CaptureInput> input = manager_->CreateCameraInput(cameras_[1]);
    ASSERT_NE(input, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input;
    camInput->Open();

    intResult = session_->AddInput(input);
    EXPECT_EQ(intResult, 0);

    GetSupportedOutputCapability();

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(photoProfiles[0]);
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    if (!(((sptr<PhotoOutput>&)photoOutput)->IsMirrorSupported())) {
        return;
    }
    
    SelectProfiles wanted;
    wanted.preview.size_ = {PRVIEW_WIDTH_640, PRVIEW_HEIGHT_480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(wanted.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);

    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session with color effect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with color effect
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_042, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::vector<ColorEffect> colorEffects = session_->GetSupportedColorEffects();
    if (colorEffects.empty()) {
        return;
    }
    ASSERT_EQ(colorEffects.empty(), false);

    ColorEffect colorEffect = colorEffects.back();
    session_->LockForControl();
    session_->SetColorEffect(colorEffect);
    session_->UnlockForControl();
    EXPECT_EQ(session_->GetColorEffect(), colorEffect);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test Preview + Metadata
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Preview + Metadata
 * @tc.require: SR000GVK5P SR000GVO5O
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_043, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;
    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();
    if (metadataObjectTypes.size() == 0) {
        return;
    }

    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> { MetadataObjectType::FACE });

    std::shared_ptr<MetadataObjectCallback> metadataObjectCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataObjectCallback);
    std::shared_ptr<MetadataStateCallback> metadataStateCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataStateCallback);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->Stop();
    metaOutput->Release();
}

/* Feature: Framework
 * Function: Test preview/capture with portrait session's portraitEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with portrait session's portraitEffect
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_045, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    if (!IsSupportMode(portraitMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> modes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(modes.size() != 0);


    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], portraitMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(portraitMode);

    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = static_cast<PortraitSession*>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    int32_t intResult = portraitSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    camera_rational_t ratio = {
        .numerator = 16,
        .denominator=9
    };

    Profile profile = SelectProfileByRatioAndFormat(modeAbility, ratio, photoFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(profile);
    ASSERT_NE(photoOutput, nullptr);

    intResult = portraitSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    profile = SelectProfileByRatioAndFormat(modeAbility, ratio, previewFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profile);
    ASSERT_NE(previewOutput, nullptr);

    ProcessPortraitSession(portraitSession, previewOutput);

    intResult = portraitSession->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    portraitSession->Stop();
}

/* Feature: Framework
 * Function: Test preview/capture with portrait session's filter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with portrait session's filter
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_046, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    if (!IsSupportMode(portraitMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> modes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], portraitMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(portraitMode);

    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = static_cast<PortraitSession*>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    int32_t intResult = portraitSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    camera_rational_t ratio = {
        .numerator = 4,
        .denominator=3
    };

    Profile profile = SelectProfileByRatioAndFormat(modeAbility, ratio, photoFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(profile);
    ASSERT_NE(photoOutput, nullptr);

    intResult = portraitSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    profile = SelectProfileByRatioAndFormat(modeAbility, ratio, previewFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profile);
    ASSERT_NE(previewOutput, nullptr);

    intResult = portraitSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    portraitSession->LockForControl();

    std::vector<FilterType> filterLists = portraitSession->GetSupportedFilters();
    if (!filterLists.empty()) {
        portraitSession->SetFilter(filterLists[0]);
    }

    portraitSession->UnlockForControl();

    if (!filterLists.empty()) {
        EXPECT_EQ(portraitSession->GetFilter(), filterLists[0]);
    }

    intResult = portraitSession->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    portraitSession->Stop();
}

/* Feature: Framework
 * Function: Test preview/capture with portrait session's beauty
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with portrait session's beauty
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_047, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    if (!IsSupportMode(portraitMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> modes = cameraManagerObj->GetSupportedModes(cameras_[1]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[1], portraitMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(portraitMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = static_cast<PortraitSession*>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    EXPECT_EQ(portraitSession->BeginConfig(), 0);

    if (input_) {
        input_->Close();
        input_->Release();
    }
    input_ = cameraManagerObj->CreateCameraInput(cameras_[1]);
    int32_t intResult = input_->Open();
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(portraitSession->AddInput(input_), 0);

    camera_rational_t ratio = {
        .numerator = 16,
        .denominator=9
    };

    Profile profile = SelectProfileByRatioAndFormat(modeAbility, ratio, photoFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(profile);
    ASSERT_NE(photoOutput, nullptr);

    EXPECT_EQ(portraitSession->AddOutput(photoOutput), 0);

    profile = SelectProfileByRatioAndFormat(modeAbility, ratio, previewFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profile);
    ASSERT_NE(previewOutput, nullptr);

    EXPECT_EQ(portraitSession->AddOutput(previewOutput), 0);

    EXPECT_EQ(portraitSession->CommitConfig(), 0);

    portraitSession->LockForControl();

    std::vector<BeautyType> beautyLists = portraitSession->GetSupportedBeautyTypes();
    EXPECT_GE(beautyLists.size(), 3);

    std::vector<int32_t> rangeLists = {};
    if (beautyLists.size() >= 3) {
        rangeLists = portraitSession->GetSupportedBeautyRange(beautyLists[3]);
    }

    if (beautyLists.size() >= 3) {
        portraitSession->SetBeauty(beautyLists[3], rangeLists[0]);
    }

    portraitSession->UnlockForControl();

    if (beautyLists.size() >= 3) {
        EXPECT_EQ(portraitSession->GetBeauty(beautyLists[3]), rangeLists[0]);
    }

    EXPECT_EQ(portraitSession->Start(), 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    portraitSession->Stop();
}

/* Feature: Framework
 * Function: Test preview/capture with portrait session
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with portrait session
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_048, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    if (!IsSupportMode(portraitMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> modes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], portraitMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(portraitMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = static_cast<PortraitSession*>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    int32_t intResult = portraitSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    camera_rational_t ratio = {
        .numerator = 4,
        .denominator=3
    };

    Profile profile = SelectProfileByRatioAndFormat(modeAbility, ratio, photoFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(profile);
    ASSERT_NE(photoOutput, nullptr);

    intResult = portraitSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    profile = SelectProfileByRatioAndFormat(modeAbility, ratio, previewFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profile);
    ASSERT_NE(previewOutput, nullptr);

    intResult = portraitSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    portraitSession->Stop();
}

/* Feature: Framework
 * Function: Test preview/video with profession session
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/video with profession session
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_071, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);


    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_RECORDING);
    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    session->Stop();
}

/* Feature: Framework
 * Function: Test profession session metering mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test profession session metering mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_072, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    MeteringMode meteringMode = METERING_MODE_CENTER_WEIGHTED;
    bool isSupported;
    intResult = session->IsMeteringModeSupported(meteringMode, isSupported);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(isSupported, true);

    std::vector<MeteringMode> modes = {};
    intResult = session->GetSupportedMeteringModes(modes);
    EXPECT_EQ(intResult, 0);
    EXPECT_NE(modes.size(), 0);

    session->LockForControl();
    intResult = session->SetMeteringMode(modes[0]);
    session->UnlockForControl();
    EXPECT_EQ(intResult, 0);
    intResult = session->GetMeteringMode(meteringMode);
    EXPECT_EQ(intResult, 0);

    EXPECT_EQ(meteringMode, modes[0]);
    intResult = session->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_RECORDING);
    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    session->Stop();
}

/* Feature: Framework
 * Function: Test profession session focus Assist flash mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test profession session focus Assist flash mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_073, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    FocusAssistFlashMode mode = FocusAssistFlashMode::FOCUS_ASSIST_FLASH_MODE_DEFAULT;
    bool isSupported;
    intResult = session->IsFocusAssistFlashModeSupported(mode, isSupported);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(isSupported, true);

    std::vector<FocusAssistFlashMode> modes = {};
    intResult = session->GetSupportedFocusAssistFlashModes(modes);
    EXPECT_EQ(intResult, 0);
    EXPECT_NE(modes.size(), 0);

    session->LockForControl();
    intResult = session->SetFocusAssistFlashMode(modes[0]);
    session->UnlockForControl();
    EXPECT_EQ(intResult, 0);
    intResult = session->GetFocusAssistFlashMode(mode);
    EXPECT_EQ(intResult, 0);

    EXPECT_EQ(mode, modes[0]);
    intResult = session->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_RECORDING);
    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    session->Stop();
}

/* Feature: Framework
 * Function: Test profession session exposure hint mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test profession session exposure hint mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_074, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);


    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);
    std::vector<ExposureHintMode> modes = {};
    intResult = session->GetSupportedExposureHintModes(modes);
    EXPECT_EQ(intResult, 0);
    EXPECT_NE(modes.size(), 0);

    session->LockForControl();
    intResult = session->SetExposureHintMode(modes[0]);
    EXPECT_EQ(intResult, 0);
    session->UnlockForControl();

    ExposureHintMode mode;
    intResult = session->GetExposureHintMode(mode);
    EXPECT_EQ(intResult, 0);

    EXPECT_EQ(mode, modes[0]);
    intResult = session->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_RECORDING);
    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    session->Stop();
}
/*
* Feature: Framework
* Function: Test manual_iso_props && auto_awb_props && manual_awb_props
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: test manual_iso_props && auto_awb_props && manual_awb_props
*/
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_075, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = modeManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        modeManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);

    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isSupported = session->IsManualIsoSupported();
    if (isSupported) {
        std::vector<int32_t> isoRange;
        session->GetIsoRange(isoRange);
        ASSERT_EQ(isoRange.empty(), false);
        session->LockForControl();
        intResult = session->SetISO(isoRange[1] + 1);
        EXPECT_NE(intResult, 0);
        session->UnlockForControl();

        session->LockForControl();
        intResult = session->SetISO(isoRange[1]);
        EXPECT_EQ(intResult, 0);
        session->UnlockForControl();

        int32_t iso;
        session->GetISO(iso);
        EXPECT_EQ(isoRange[1], iso);
    }

    std::vector<WhiteBalanceMode> supportedWhiteBalanceModes;
    session->GetSupportedWhiteBalanceModes(supportedWhiteBalanceModes);
    if (!supportedWhiteBalanceModes.empty()) {
        session->IsWhiteBalanceModeSupported(supportedWhiteBalanceModes[0], isSupported);
        ASSERT_EQ(isSupported, true);
        session->LockForControl();
        intResult = session->SetWhiteBalanceMode(supportedWhiteBalanceModes[0]);
        ASSERT_EQ(isSupported, 0);
        session->UnlockForControl();
        WhiteBalanceMode currentMode;
        session->GetWhiteBalanceMode(currentMode);
        ASSERT_EQ(currentMode, supportedWhiteBalanceModes[0]);
    }

    session->IsManualWhiteBalanceSupported(isSupported);
    std::vector<int32_t> whiteBalanceRange;
    if (isSupported) {
        session->GetManualWhiteBalanceRange(whiteBalanceRange);
        ASSERT_EQ(whiteBalanceRange.size() == 2, true);

        session->LockForControl();
        intResult = session->SetManualWhiteBalance(whiteBalanceRange[0] - 1);
        session->UnlockForControl();

        int32_t wbValue;
        session->GetManualWhiteBalance(wbValue);
        ASSERT_EQ(wbValue, whiteBalanceRange[0]);
    } else {
        session->GetManualWhiteBalanceRange(whiteBalanceRange);
        ASSERT_EQ(whiteBalanceRange.size() < 2, true);
    }
}
/* Feature: Framework
 * Function: Test profession session focus mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test profession session focus mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_076, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    FocusMode focusMode = FocusMode::FOCUS_MODE_MANUAL;
    bool isSupported;
    intResult = session->IsFocusModeSupported(focusMode, isSupported);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(isSupported, true);

    std::vector<FocusMode> modes = {};
    intResult = session->GetSupportedFocusModes(modes);
    EXPECT_EQ(intResult, 0);
    EXPECT_NE(modes.size(), 0);

    session->LockForControl();
    intResult = session->SetFocusMode(modes[0]);
    session->UnlockForControl();
    EXPECT_EQ(intResult, 0);
    intResult = session->GetFocusMode(focusMode);
    EXPECT_EQ(intResult, 0);

    EXPECT_EQ(focusMode, modes[0]);
    intResult = session->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_RECORDING);
    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    session->Stop();
}

/* Feature: Framework
 * Function: Test profession session FlashMode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test profession session FlashMode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_077, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isSupported = false;
    intResult = session->HasFlash(isSupported);
    EXPECT_EQ(isSupported, true);

    std::vector<FlashMode> modes = {};
    intResult = session->GetSupportedFlashModes(modes);
    EXPECT_EQ(intResult, 0);
    EXPECT_NE(modes.size(), 0);

    intResult = session->IsFlashModeSupported(modes[0], isSupported);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(isSupported, true);

    session->LockForControl();
    intResult = session->SetFlashMode(modes[0]);
    EXPECT_EQ(intResult, 0);
    session->UnlockForControl();

    FlashMode mode;
    intResult = session->GetFlashMode(mode);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(mode, modes[0]);
}

/* Feature: Framework
 * Function: Test profession session ColorEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test profession session ColorEffect
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_profession_078, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PROFESSIONAL_VIDEO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.photo.format_, CAMERA_FORMAT_INVALID);
    ASSERT_NE(profiles.video.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ProfessionSession> session = static_cast<ProfessionSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(profiles.video);
    ASSERT_NE(videoOutput, nullptr);

    intResult = session->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);


    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);
    std::vector<ColorEffect> colorEffects = {};
    intResult = session->GetSupportedColorEffects(colorEffects);
    EXPECT_EQ(intResult, 0);
    EXPECT_NE(colorEffects.size(), 0);

    session->LockForControl();
    intResult = session->SetColorEffect(colorEffects[0]);
    EXPECT_EQ(intResult, 0);
    session->UnlockForControl();

    ColorEffect colorEffect;
    intResult = session->GetColorEffect(colorEffect);
    EXPECT_EQ(intResult, 0);

    EXPECT_EQ(colorEffect, colorEffects[0]);
    intResult = session->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_RECORDING);
    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    session->Stop();
}

/*
 * Feature: Framework
 * Function: Test Scan Session add output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test scan session with two preview outputs
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_scan_049, TestSize.Level0)
{
    if (!IsSupportNow())
    {
        return;
    }
    MEDIA_INFO_LOG("teset049 begin");
    sptr<CaptureOutput> previewOutput_1;
    sptr<CaptureOutput> previewOutput_2;
    ConfigScanSession(previewOutput_1, previewOutput_2);

    int32_t intResult = scanSession_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = scanSession_->Stop();
    EXPECT_EQ(intResult, 0);

    ((sptr<PreviewOutput> &) previewOutput_1)->Release();
    ((sptr<PreviewOutput> &) previewOutput_2)->Release();
    MEDIA_INFO_LOG("teset049 end");
}

/*
 * Feature: Framework
 * Function: Test Scan Session add output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test scan session with Capture and video output
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_scan_050, TestSize.Level0)
{
    if (!IsSupportNow())
    {
        return;
    }
    MEDIA_INFO_LOG("teset050 begin");
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
    }
    scanSession_ = manager_ -> CreateCaptureSession(SceneMode::SCAN);
    ASSERT_NE(scanSession_, nullptr);

    int32_t intResult = scanSession_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = scanSession_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> phtotOutput = CreatePhotoOutput();
    ASSERT_NE(phtotOutput, nullptr);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(phtotOutput, nullptr);

    intResult = scanSession_->AddOutput(videoOutput);
    EXPECT_NE(intResult, 0);

    intResult = scanSession_->AddOutput(phtotOutput);
    EXPECT_NE(intResult, 0);

    intResult = scanSession_->CommitConfig();
    EXPECT_NE(intResult, 0);

    ((sptr<PhotoOutput> &) phtotOutput)->Release();
    ((sptr<VideoOutput> &) videoOutput)->Release();
    MEDIA_INFO_LOG("teset050 end");
}

/*
 * Feature: Framework
 * Function: Test Scan Session outputcapability and supported mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test scan session print outputcapability and supported mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_scan_051, TestSize.Level0)
{
    MEDIA_INFO_LOG("teset051 start");
    if (!IsSupportNow()) {
        return;
    }
    sptr<CameraDevice> scanModeDevice = cameras_[0];
    vector<SceneMode> modeVec = manager_ -> GetSupportedModes(scanModeDevice);
    EXPECT_TRUE(find(modeVec.begin(), modeVec.end(), SceneMode::SCAN) != modeVec.end());

    for (auto iter : modeVec) {
        MEDIA_INFO_LOG("get supportedMode : %{public}d", iter);
    }

    sptr<CameraOutputCapability> scanCapability =
        manager_ -> GetSupportedOutputCapability(scanModeDevice, SceneMode::SCAN);
    EXPECT_EQ(scanCapability -> GetPhotoProfiles().size(), 0);
    EXPECT_NE(scanCapability -> GetPreviewProfiles().size(), 0);
    EXPECT_EQ(scanCapability -> GetVideoProfiles().size(), 0);

    MEDIA_INFO_LOG("photoProfile/previewProfile/videoProfiles size : %{public}zu, %{public}zu, %{public}zu",
                    scanCapability -> GetPhotoProfiles().size(),
                    scanCapability -> GetPreviewProfiles().size(),
                    scanCapability -> GetVideoProfiles().size());
    MEDIA_INFO_LOG("teset051 end");
}

/*
 * Feature: Framework
 * Function: Test scan session set Focus mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test scan session set Focus mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_scan_052, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureOutput> previewOutput_1;
    sptr<CaptureOutput> previewOutput_2;
    ConfigScanSession(previewOutput_1, previewOutput_2);

    scanSession_->LockForControl();
    int32_t setFocusMode = scanSession_->SetFocusMode(FOCUS_MODE_AUTO);
    EXPECT_EQ(setFocusMode, 0);
    scanSession_->UnlockForControl();

    FocusMode focusMode = scanSession_->GetFocusMode();
    EXPECT_NE(focusMode, FOCUS_MODE_MANUAL);
    EXPECT_NE(focusMode, FOCUS_MODE_CONTINUOUS_AUTO);
    EXPECT_EQ(focusMode, FOCUS_MODE_AUTO);
    EXPECT_NE(focusMode, FOCUS_MODE_LOCKED);

    int32_t getFoucusMode = scanSession_->GetFocusMode(focusMode);
    EXPECT_EQ(getFoucusMode, 0);
    EXPECT_EQ(focusMode, FOCUS_MODE_AUTO);

    bool isSupported = scanSession_->IsFocusModeSupported(focusMode);
    EXPECT_EQ(isSupported, true);

    int32_t isFocusSupported = scanSession_->IsFocusModeSupported(focusMode, isSupported);
    EXPECT_EQ(isFocusSupported, 0);
    EXPECT_EQ(isSupported, true);

    std::vector<FocusMode> supportedFocusModes = scanSession_->GetSupportedFocusModes();
    EXPECT_EQ(supportedFocusModes.empty(), false);

    int32_t getSupportedFocusModes = scanSession_->GetSupportedFocusModes(supportedFocusModes);
    EXPECT_EQ(supportedFocusModes.empty(), false);
    EXPECT_EQ(getSupportedFocusModes, 0);

    ((sptr<PreviewOutput> &) previewOutput_1)->Release();
    ((sptr<PreviewOutput> &) previewOutput_2)->Release();
}

/*
 * Feature: Framework
 * Function: Test scan session set Focus Point
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test scan session set Focus Point
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_scan_053, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureOutput> previewOutput_1;
    sptr<CaptureOutput> previewOutput_2;
    ConfigScanSession(previewOutput_1, previewOutput_2);

    Point point = { 1, 1 };
    scanSession_->LockForControl();
    int32_t setFocusMode = scanSession_->SetFocusPoint(point);
    EXPECT_EQ(setFocusMode, 0);
    scanSession_->UnlockForControl();

    Point focusPointGet = scanSession_->GetFocusPoint();
    EXPECT_EQ(focusPointGet.x, 1);
    EXPECT_EQ(focusPointGet.y, 1);

    float focalLength;
    int32_t focalLengthGet = scanSession_->GetFocalLength(focalLength);
    EXPECT_EQ(focalLengthGet, 0);

    ((sptr<PreviewOutput> &) previewOutput_1)->Release();
    ((sptr<PreviewOutput> &) previewOutput_2)->Release();
}

/*
 * Feature: Framework
 * Function: Test scan session set Focus Point
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test scan session set Zoom Ratio
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_scan_054, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureOutput> previewOutput_1;
    sptr<CaptureOutput> previewOutput_2;
    ConfigScanSession(previewOutput_1, previewOutput_2);

    scanSession_->LockForControl();
    int32_t zoomRatioSet = scanSession_->SetZoomRatio(100);
    EXPECT_EQ(zoomRatioSet, 0);
    scanSession_->UnlockForControl();

    std::vector<float> zoomRatioRange = scanSession_->GetZoomRatioRange();
    EXPECT_EQ(zoomRatioRange.empty(), false);

    int32_t zoomRatioRangeGet = scanSession_->GetZoomRatioRange(zoomRatioRange);
    EXPECT_EQ(zoomRatioRange.empty(), false);
    EXPECT_EQ(zoomRatioRangeGet, 0);

    float zoomRatio = scanSession_->GetZoomRatio();
    int32_t zoomRatioGet = scanSession_->GetZoomRatio(zoomRatio);
    EXPECT_EQ(zoomRatioGet, 0);

    ((sptr<PreviewOutput> &) previewOutput_1)->Release();
    ((sptr<PreviewOutput> &) previewOutput_2)->Release();
}

/*
 * Feature: Framework
 * Function: Test scan session report brightness status
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test scan session report brightness status
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_scan_055, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureOutput> previewOutput_1;
    sptr<CaptureOutput> previewOutput_2;
    ConfigScanSession(previewOutput_1, previewOutput_2);

    auto scanSession = static_cast<ScanSession*>(scanSession_.GetRefPtr());
    bool isSupported = scanSession->IsBrightnessStatusSupported();
    EXPECT_EQ(isSupported, true);

    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    scanSession->RegisterBrightnessStatusCallback(callback);

    int32_t intResult = scanSession->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    EXPECT_EQ(g_brightnessStatusChanged, true);

    scanSession->UnRegisterBrightnessStatusCallback();
    intResult = scanSession->Stop();
    EXPECT_EQ(intResult, 0);

    ((sptr<PreviewOutput> &) previewOutput_1)->Release();
    ((sptr<PreviewOutput> &) previewOutput_2)->Release();
}

/*
 * Feature: Framework
 * Function: Test photo ability function.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo ability function.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_ability001, TestSize.Level0)
{
    SceneMode photoMode = SceneMode::CAPTURE;
    if (!IsSupportMode(photoMode)) {
        return;
    }
    sptr<CameraManager> cameraMgr = CameraManager::GetInstance();
    ASSERT_NE(cameraMgr, nullptr);

    sptr<CameraOutputCapability> capability = cameraMgr->GetSupportedOutputCapability(cameras_[0], photoMode);
    ASSERT_NE(capability, nullptr);

    sptr<CaptureSession> captureSession = cameraMgr->CreateCaptureSession(photoMode);
    ASSERT_NE(captureSession, nullptr);

    sptr<PhotoSession> photoSession = static_cast<PhotoSession *>(captureSession.GetRefPtr());
    ASSERT_NE(photoSession, nullptr);

    std::vector<sptr<CameraOutputCapability>> cocList = photoSession->GetCameraOutputCapabilities(cameras_[0]);
    ASSERT_TRUE(cocList.size() != 0);
    auto previewProfiles = cocList[0]->GetPreviewProfiles();
    auto photoProfiles = cocList[0]->GetPhotoProfiles();
    auto videoProfiles = cocList[0]->GetVideoProfiles();

    int32_t intResult = photoSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = photoSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);
    intResult = photoSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = photoSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = photoSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    auto photoFunctionsList = photoSession->GetSessionFunctions(previewProfiles, photoProfiles, videoProfiles, true);
    ASSERT_TRUE(photoFunctionsList.size() != 0);
    auto photoFunctions = photoFunctionsList[0];
    photoFunctions->HasFlash();
    photoFunctions->GetSupportedFocusModes();
    photoFunctions->GetSupportedBeautyTypes();
    photoFunctions->GetSupportedBeautyRange(BeautyType::AUTO_TYPE);
    photoFunctions->GetSupportedColorEffects();
    photoFunctions->GetSupportedColorSpaces();
    photoFunctions->IsFocusModeSupported(FocusMode::FOCUS_MODE_MANUAL);
    std::vector<sptr<CameraAbility>> photoConflictFunctionsList = photoSession->GetSessionConflictFunctions();

    std::vector<float> zoomRatioRange = photoSession->GetZoomRatioRange();
    ASSERT_NE(zoomRatioRange.size(), 0);

    photoSession->LockForControl();
    photoSession->EnableMacro(true);
    photoSession->UnlockForControl();

    zoomRatioRange = photoSession->GetZoomRatioRange();
    ASSERT_NE(zoomRatioRange.size(), 0);

    photoSession->LockForControl();
    photoSession->EnableMacro(false);
    photoSession->UnlockForControl();

    photoSession->IsMacroSupported();

    photoSession->LockForControl();
    photoSession->SetZoomRatio(zoomRatioRange[1]);
    photoSession->UnlockForControl();

    photoSession->IsMacroSupported();
}

/*
 * Feature: Framework
 * Function: Test portrait photo ability function.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test portrait photo ability function.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_ability002, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    if (!IsSupportMode(portraitMode)) {
        return;
    }

    sptr<CameraManager> cameraMgr = CameraManager::GetInstance();
    ASSERT_NE(cameraMgr, nullptr);

    sptr<CameraOutputCapability> capability = cameraMgr->GetSupportedOutputCapability(cameras_[0], portraitMode);
    ASSERT_NE(capability, nullptr);

    sptr<CaptureSession> captureSession = cameraMgr->CreateCaptureSession(portraitMode);
    ASSERT_NE(captureSession, nullptr);

    sptr<PortraitSession> portraitSession = static_cast<PortraitSession *>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    std::vector<sptr<CameraOutputCapability>> cocList = portraitSession->GetCameraOutputCapabilities(cameras_[0]);
    ASSERT_TRUE(cocList.size() != 0);
    auto previewProfiles = cocList[0]->GetPreviewProfiles();
    auto photoProfiles = cocList[0]->GetPhotoProfiles();
    auto videoProfiles = cocList[0]->GetVideoProfiles();

    int32_t intResult = portraitSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);
    intResult = portraitSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = portraitSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    auto portraitFunctionsList = portraitSession->GetSessionFunctions(previewProfiles, photoProfiles, videoProfiles, true);
    ASSERT_TRUE(portraitFunctionsList.size() != 0);
    auto portraitFunctions = portraitFunctionsList[0];
    portraitFunctions->GetSupportedPortraitEffects();
    portraitFunctions->GetSupportedVirtualApertures();
    portraitFunctions->GetSupportedPhysicalApertures();
    std::vector<sptr<CameraAbility>> portraitConflictFunctionsList = portraitSession->GetSessionConflictFunctions();
}

/*
 * Feature: Framework
 * Function: Test video ability function.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test video ability function.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_ability003, TestSize.Level0)
{
    SceneMode videoMode = SceneMode::VIDEO;
    if (!IsSupportMode(videoMode)) {
        return;
    }
    sptr<CameraManager> cameraMgr = CameraManager::GetInstance();
    ASSERT_NE(cameraMgr, nullptr);

    sptr<CameraOutputCapability> capability = cameraMgr->GetSupportedOutputCapability(cameras_[0], videoMode);
    ASSERT_NE(capability, nullptr);

    sptr<CaptureSession> captureSession = cameraMgr->CreateCaptureSession(videoMode);
    ASSERT_NE(captureSession, nullptr);

    sptr<VideoSession> videoSession = static_cast<VideoSession *>(captureSession.GetRefPtr());
    ASSERT_NE(videoSession, nullptr);

    std::vector<sptr<CameraOutputCapability>> cocList = videoSession->GetCameraOutputCapabilities(cameras_[0]);
    ASSERT_TRUE(cocList.size() != 0);
    auto previewProfiles = cocList[0]->GetPreviewProfiles();
    auto photoProfiles = cocList[0]->GetPhotoProfiles();
    auto videoProfiles = cocList[0]->GetVideoProfiles();

    int32_t intResult = videoSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = videoSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);
    intResult = videoSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = videoSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = videoSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    auto videoFunctionsList = videoSession->GetSessionFunctions(previewProfiles, photoProfiles, videoProfiles, true);
    ASSERT_TRUE(videoFunctionsList.size() != 0);
    auto videoFunctions = videoFunctionsList[0];
    videoFunctions->GetSupportedStabilizationMode();
    videoFunctions->IsVideoStabilizationModeSupported(VideoStabilizationMode::HIGH);
    std::vector<sptr<CameraAbility>> videoConflictFunctionsList = videoSession->GetSessionConflictFunctions();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session video stabilization mode with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_001, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    VideoStabilizationMode stabilizationMode = camSession->GetActiveVideoStabilizationMode();
    EXPECT_EQ(stabilizationMode, OFF);

    int32_t actVideoStaMode = camSession->SetVideoStabilizationMode(stabilizationMode);
    EXPECT_EQ(actVideoStaMode, 7400103);

    actVideoStaMode = camSession->GetActiveVideoStabilizationMode(stabilizationMode);
    EXPECT_EQ(actVideoStaMode, 7400103);

    bool isSupported = true;

    int32_t videoStaModeSupported = camSession->IsVideoStabilizationModeSupported(stabilizationMode, isSupported);
    EXPECT_EQ(videoStaModeSupported, 7400103);

    std::vector<VideoStabilizationMode> videoStabilizationMode = camSession->GetSupportedStabilizationMode();
    EXPECT_EQ(videoStabilizationMode.empty(), true);

    int32_t supportedVideoStaMode = camSession->GetSupportedStabilizationMode(videoStabilizationMode);
    EXPECT_EQ(supportedVideoStaMode, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = camSession->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    videoStaModeSupported = camSession->IsVideoStabilizationModeSupported(stabilizationMode, isSupported);
    EXPECT_EQ(videoStaModeSupported, 0);

    actVideoStaMode = camSession->GetActiveVideoStabilizationMode(stabilizationMode);
    EXPECT_EQ(actVideoStaMode, 0);

    supportedVideoStaMode = camSession->GetSupportedStabilizationMode(videoStabilizationMode);
    EXPECT_EQ(supportedVideoStaMode, 0);

    ReleaseInput();

    stabilizationMode = camSession->GetActiveVideoStabilizationMode();
    EXPECT_EQ(stabilizationMode, OFF);

    actVideoStaMode = camSession->GetActiveVideoStabilizationMode(stabilizationMode);
    EXPECT_EQ(actVideoStaMode, 0);

    videoStabilizationMode = camSession->GetSupportedStabilizationMode();
    EXPECT_EQ(videoStabilizationMode.empty(), true);

    supportedVideoStaMode = camSession->GetSupportedStabilizationMode(videoStabilizationMode);
    EXPECT_EQ(supportedVideoStaMode, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session exposure mode with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_002, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    ExposureMode exposureMode = camSession->GetExposureMode();
    EXPECT_EQ(exposureMode, EXPOSURE_MODE_UNSUPPORTED);

    int32_t getExposureMode = camSession->GetExposureMode(exposureMode);
    EXPECT_EQ(getExposureMode, 7400103);

    int32_t setExposureMode = camSession->SetExposureMode(exposureMode);
    EXPECT_EQ(setExposureMode, 7400103);

    bool isExposureModeSupported = camSession->IsExposureModeSupported(exposureMode);
    EXPECT_EQ(isExposureModeSupported, false);

    bool isSupported = true;

    int32_t exposureModeSupported = camSession->IsExposureModeSupported(exposureMode, isSupported);
    EXPECT_EQ(exposureModeSupported, 7400103);

    std::vector<ExposureMode> getSupportedExpModes_1 = camSession->GetSupportedExposureModes();
    EXPECT_EQ(getSupportedExpModes_1.empty(), true);

    int32_t getSupportedExpModes_2 = camSession->GetSupportedExposureModes(getSupportedExpModes_1);
    EXPECT_EQ(getSupportedExpModes_2, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    setExposureMode = camSession->SetExposureMode(exposureMode);
    EXPECT_EQ(setExposureMode, 0);

    getExposureMode = camSession->GetExposureMode(exposureMode);
    EXPECT_EQ(getExposureMode, 0);

    exposureModeSupported = camSession->IsExposureModeSupported(exposureMode, isSupported);
    EXPECT_EQ(exposureModeSupported, 0);

    getSupportedExpModes_1 = camSession->GetSupportedExposureModes();
    EXPECT_EQ(getSupportedExpModes_1.empty(), false);

    getSupportedExpModes_2 = camSession->GetSupportedExposureModes(getSupportedExpModes_1);
    EXPECT_EQ(getSupportedExpModes_2, 0);

    ReleaseInput();

    getSupportedExpModes_1 = camSession->GetSupportedExposureModes();
    EXPECT_EQ(getSupportedExpModes_1.empty(), true);

    getSupportedExpModes_2 = camSession->GetSupportedExposureModes(getSupportedExpModes_1);
    EXPECT_EQ(getSupportedExpModes_2, 0);

    exposureMode = camSession->GetExposureMode();
    EXPECT_EQ(exposureMode, EXPOSURE_MODE_UNSUPPORTED);

    getExposureMode = camSession->GetExposureMode(exposureMode);
    EXPECT_EQ(getExposureMode, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session metering point with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_003, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    Point exposurePointGet = camSession->GetMeteringPoint();
    EXPECT_EQ(exposurePointGet.x, 0);
    EXPECT_EQ(exposurePointGet.y, 0);

    int32_t setMeteringPoint = camSession->SetMeteringPoint(exposurePointGet);
    EXPECT_EQ(setMeteringPoint, 7400103);

    int32_t getMeteringPoint = camSession->GetMeteringPoint(exposurePointGet);
    EXPECT_EQ(getMeteringPoint, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    setMeteringPoint = camSession->SetMeteringPoint(exposurePointGet);
    EXPECT_EQ(setMeteringPoint, 0);

    getMeteringPoint = camSession->GetMeteringPoint(exposurePointGet);
    EXPECT_EQ(getMeteringPoint, 0);

    ReleaseInput();

    exposurePointGet = camSession->GetMeteringPoint();
    EXPECT_EQ(exposurePointGet.x, 0);
    EXPECT_EQ(exposurePointGet.y, 0);

    getMeteringPoint = camSession->GetMeteringPoint(exposurePointGet);
    EXPECT_EQ(getMeteringPoint, 0);

    camSession->LockForControl();

    setMeteringPoint = camSession->SetMeteringPoint(exposurePointGet);
    EXPECT_EQ(setMeteringPoint, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session exposure value and range with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_004, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras_[0]->GetMetadata();
    ASSERT_NE(metadata, nullptr);

    camSession->ProcessAutoExposureUpdates(metadata);

    float exposureValue = camSession->GetExposureValue();
    EXPECT_EQ(exposureValue, 0);

    int32_t exposureValueGet = camSession->GetExposureValue(exposureValue);
    EXPECT_EQ(exposureValueGet, 7400103);

    int32_t setExposureBias = camSession->SetExposureBias(exposureValue);
    EXPECT_EQ(setExposureBias, 7400103);

    std::vector<float> getExposureBiasRange = camSession->GetExposureBiasRange();
    EXPECT_EQ(getExposureBiasRange.empty(), true);

    int32_t exposureBiasRangeGet = camSession->GetExposureBiasRange(getExposureBiasRange);
    EXPECT_EQ(exposureBiasRangeGet, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    setExposureBias = camSession->SetExposureBias(exposureValue);
    EXPECT_EQ(setExposureBias, 0);

    exposureValueGet = camSession->GetExposureValue(exposureValue);
    EXPECT_EQ(exposureValueGet, 0);

    exposureBiasRangeGet = camSession->GetExposureBiasRange(getExposureBiasRange);
    EXPECT_EQ(exposureBiasRangeGet, 0);

    ReleaseInput();

    getExposureBiasRange = camSession->GetExposureBiasRange();
    EXPECT_EQ(getExposureBiasRange.empty(), true);

    exposureBiasRangeGet = camSession->GetExposureBiasRange(getExposureBiasRange);
    EXPECT_EQ(exposureBiasRangeGet, 0);

    exposureValue = camSession->GetExposureValue();
    EXPECT_EQ(exposureValue, 0);

    exposureValueGet = camSession->GetExposureValue(exposureValue);
    EXPECT_EQ(exposureValueGet, 0);

    camSession->LockForControl();

    setExposureBias = camSession->SetExposureBias(exposureValue);
    EXPECT_EQ(setExposureBias, 7400102);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session focus mode with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_005, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    FocusMode focusMode = camSession->GetFocusMode();
    EXPECT_EQ(focusMode, FOCUS_MODE_MANUAL);

    int32_t getFocusMode = camSession->GetFocusMode(focusMode);
    EXPECT_EQ(getFocusMode, 7400103);

    int32_t setFocusMode = camSession->SetFocusMode(focusMode);
    EXPECT_EQ(setFocusMode, 7400103);

    bool isFocusModeSupported = camSession->IsFocusModeSupported(focusMode);
    EXPECT_EQ(isFocusModeSupported, false);

    bool isSupported = true;

    int32_t focusModeSupported = camSession->IsFocusModeSupported(focusMode, isSupported);
    EXPECT_EQ(focusModeSupported, 7400103);

    std::vector<FocusMode> getSupportedFocusModes = camSession->GetSupportedFocusModes();
    EXPECT_EQ(getSupportedFocusModes.empty(), true);

    int32_t supportedFocusModesGet = camSession->GetSupportedFocusModes(getSupportedFocusModes);
    EXPECT_EQ(supportedFocusModesGet, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    setFocusMode = camSession->SetFocusMode(focusMode);
    EXPECT_EQ(setFocusMode, 0);

    getFocusMode = camSession->GetFocusMode(focusMode);
    EXPECT_EQ(getFocusMode, 0);

    focusModeSupported = camSession->IsFocusModeSupported(focusMode, isSupported);
    EXPECT_EQ(focusModeSupported, 0);

    getSupportedFocusModes = camSession->GetSupportedFocusModes();
    EXPECT_EQ(getSupportedFocusModes.empty(), false);

    supportedFocusModesGet = camSession->GetSupportedFocusModes(getSupportedFocusModes);
    EXPECT_EQ(supportedFocusModesGet, 0);

    ReleaseInput();

    getSupportedFocusModes = camSession->GetSupportedFocusModes();
    EXPECT_EQ(getSupportedFocusModes.empty(), true);

    supportedFocusModesGet = camSession->GetSupportedFocusModes(getSupportedFocusModes);
    EXPECT_EQ(supportedFocusModesGet, 0);

    focusMode = camSession->GetFocusMode();
    EXPECT_EQ(focusMode, FOCUS_MODE_MANUAL);

    getFocusMode = camSession->GetFocusMode(focusMode);
    EXPECT_EQ(getFocusMode, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session focus point with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_006, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    Point focusPoint = camSession->GetFocusPoint();
    EXPECT_EQ(focusPoint.x, 0);
    EXPECT_EQ(focusPoint.y, 0);

    float focalLength = camSession->GetFocalLength();
    EXPECT_EQ(focalLength, 0);

    int32_t focalLengthGet = camSession->GetFocalLength(focalLength);
    EXPECT_EQ(focalLengthGet, 7400103);

    int32_t focusPointGet = camSession->GetFocusPoint(focusPoint);
    EXPECT_EQ(focusPointGet, 7400103);

    int32_t setFocusPoint = camSession->SetFocusPoint(focusPoint);
    EXPECT_EQ(setFocusPoint, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    setFocusPoint = camSession->SetFocusPoint(focusPoint);
    EXPECT_EQ(setFocusPoint, 0);

    camSession->LockForControl();

    Point point = { 0, 0 };
    setFocusPoint = camSession->SetFocusPoint(point);
    EXPECT_EQ(setFocusPoint, 0);

    point.x = CameraErrorCode::SESSION_NOT_CONFIG;
    setFocusPoint = camSession->SetFocusPoint(point);
    EXPECT_EQ(setFocusPoint, 0);

    focusPointGet = camSession->GetFocusPoint(focusPoint);
    EXPECT_EQ(focusPointGet, 0);

    focalLengthGet = camSession->GetFocalLength(focalLength);
    EXPECT_EQ(focalLengthGet, 0);

    ReleaseInput();

    focusPoint = camSession->GetFocusPoint();
    EXPECT_EQ(focusPoint.x, 0);
    EXPECT_EQ(focusPoint.y, 0);

    focusPointGet = camSession->GetFocusPoint(focusPoint);
    EXPECT_EQ(focusPointGet, 0);

    focalLength = camSession->GetFocalLength();
    EXPECT_EQ(focalLength, 0);

    focalLengthGet = camSession->GetFocalLength(focalLength);
    EXPECT_EQ(focalLengthGet, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session flash mode with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_007, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    bool hasFlash = camSession->HasFlash();
    EXPECT_EQ(hasFlash, false);

    int32_t isFlash = camSession->HasFlash(hasFlash);
    EXPECT_EQ(isFlash, 7400103);

    FlashMode flashMode = camSession->GetFlashMode();
    EXPECT_EQ(flashMode, FLASH_MODE_CLOSE);

    int32_t flashModeGet = camSession->GetFlashMode(flashMode);
    EXPECT_EQ(flashModeGet, 7400103);

    int32_t setFlashMode = camSession->SetFlashMode(flashMode);
    EXPECT_EQ(setFlashMode, 7400103);

    bool isFlashModeSupported = camSession->IsFlashModeSupported(flashMode);
    EXPECT_EQ(isFlashModeSupported, false);

    bool isSupported = true;

    int32_t flashModeSupported = camSession->IsFlashModeSupported(flashMode, isSupported);
    EXPECT_EQ(flashModeSupported, 7400103);

    std::vector<FlashMode> getSupportedFlashModes = camSession->GetSupportedFlashModes();
    EXPECT_EQ(getSupportedFlashModes.empty(), true);

    int32_t getSupportedFlashMode = camSession->GetSupportedFlashModes(getSupportedFlashModes);
    EXPECT_EQ(getSupportedFlashMode, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    isFlash = camSession->HasFlash(hasFlash);
    EXPECT_EQ(isFlash, 0);

    setFlashMode = camSession->SetFlashMode(flashMode);
    EXPECT_EQ(setFlashMode, 0);

    flashModeGet = camSession->GetFlashMode(flashMode);
    EXPECT_EQ(flashModeGet, 0);

    flashModeSupported = camSession->IsFlashModeSupported(flashMode, isSupported);
    EXPECT_EQ(setFlashMode, 0);

    getSupportedFlashModes = camSession->GetSupportedFlashModes();
    EXPECT_EQ(getSupportedFlashModes.empty(), false);

    getSupportedFlashMode = camSession->GetSupportedFlashModes(getSupportedFlashModes);
    EXPECT_EQ(getSupportedFlashMode, 0);

    camSession->LockForControl();
    setFlashMode = camSession->SetFlashMode(flashMode);
    EXPECT_EQ(setFlashMode, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test flash mode with release input.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_008, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    ReleaseInput();

    std::vector<FlashMode> getSupportedFlashModes = camSession->GetSupportedFlashModes();
    EXPECT_EQ(getSupportedFlashModes.empty(), true);

    int32_t getSupportedFlashMode = camSession->GetSupportedFlashModes(getSupportedFlashModes);
    EXPECT_EQ(getSupportedFlashMode, 0);

    FlashMode flashMode = camSession->GetFlashMode();
    EXPECT_EQ(flashMode, FLASH_MODE_CLOSE);

    int32_t flashModeGet = camSession->GetFlashMode(flashMode);
    EXPECT_EQ(flashModeGet, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session zoom ratio with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_009, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    float zoomRatio = camSession->GetZoomRatio();
    EXPECT_EQ(zoomRatio, 0);

    std::vector<float> zoomRatioRange = camSession->GetZoomRatioRange();
    EXPECT_EQ(zoomRatioRange.empty(), true);

    int32_t zoomRatioRangeGet = camSession->GetZoomRatioRange(zoomRatioRange);
    EXPECT_EQ(zoomRatioRangeGet, 7400103);

    int32_t zoomRatioGet = camSession->GetZoomRatio(zoomRatio);
    EXPECT_EQ(zoomRatioGet, 7400103);

    int32_t setZoomRatio = camSession->SetZoomRatio(zoomRatio);
    EXPECT_EQ(setZoomRatio, 7400103);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    zoomRatioRangeGet = camSession->GetZoomRatioRange(zoomRatioRange);
    EXPECT_EQ(zoomRatioRangeGet, 0);

    zoomRatioGet = camSession->GetZoomRatio(zoomRatio);
    if (zoomRatioRange.empty()) {
        EXPECT_EQ(zoomRatioGet, 7400201);
    } else {
        EXPECT_EQ(zoomRatioGet, 0);
    }

    setZoomRatio = camSession->SetZoomRatio(zoomRatio);
    EXPECT_EQ(setZoomRatio, 0);

    ReleaseInput();

    zoomRatioRange = camSession->GetZoomRatioRange();
    EXPECT_EQ(zoomRatioRange.empty(), true);

    zoomRatioRangeGet = camSession->GetZoomRatioRange(zoomRatioRange);
    EXPECT_EQ(zoomRatioRangeGet, 0);

    zoomRatio = camSession->GetZoomRatio();
    EXPECT_EQ(zoomRatio, 0);

    zoomRatioGet = camSession->GetZoomRatio(zoomRatio);
    EXPECT_EQ(zoomRatioGet, 0);

    camSession->LockForControl();

    setZoomRatio = camSession->SetZoomRatio(zoomRatio);
    EXPECT_EQ(setZoomRatio, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with set metadata Object type anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_010, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    std::set<camera_face_detect_mode_t> metadataObjectTypes;
    camSession->SetCaptureMetadataObjectTypes(metadataObjectTypes);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataOutput with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_011, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;

    sptr<MetadataObjectListener> metaObjListener = new (std::nothrow) MetadataObjectListener(metaOutput);
    ASSERT_NE(metaObjListener, nullptr);

    metaObjListener->OnBufferAvailable();

    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();

    metaOutput->SetCapturingMetadataObjectTypes(metadataObjectTypes);
    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> { MetadataObjectType::FACE });

    metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();

    std::shared_ptr<MetadataObjectCallback> metadataObjectCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataObjectCallback);
    std::shared_ptr<MetadataStateCallback> metadataStateCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataStateCallback);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = metaOutput->Release();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test capture session callback error with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_012, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    sptr<CaptureSessionCallback> capSessionCallback = new (std::nothrow) CaptureSessionCallback();
    ASSERT_NE(capSessionCallback, nullptr);
    int32_t onError = capSessionCallback->OnError(CAMERA_DEVICE_PREEMPTED);
    EXPECT_EQ(onError, 0);
    capSessionCallback = nullptr;
    capSessionCallback = new (std::nothrow) CaptureSessionCallback(camSession);
    ASSERT_NE(capSessionCallback, nullptr);
    onError = capSessionCallback->OnError(CAMERA_DEVICE_PREEMPTED);
    EXPECT_EQ(onError, 0);

    std::shared_ptr<SessionCallback> callback = nullptr;
    camSession->SetCallback(callback);

    callback = std::make_shared<AppSessionCallback>();
    camSession->SetCallback(callback);
    capSessionCallback = nullptr;
    capSessionCallback = new (std::nothrow) CaptureSessionCallback(camSession);
    ASSERT_NE(capSessionCallback, nullptr);
    onError = capSessionCallback->OnError(CAMERA_DEVICE_PREEMPTED);
    EXPECT_EQ(onError, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test remove input with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_013, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    sptr<CaptureInput> input = nullptr;
    int32_t intResult = camSession->RemoveInput(input);
    EXPECT_EQ(intResult, 7400102);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->RemoveInput(input);
    EXPECT_EQ(intResult, 7400201);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->RemoveInput(input);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test remove output with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_014, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    sptr<CaptureOutput> output = nullptr;
    int32_t intResult = camSession->RemoveOutput(output);
    EXPECT_EQ(intResult, 7400102);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    output = CreatePreviewOutput();
    ASSERT_NE(output, nullptr);

    intResult = camSession->RemoveOutput(output);
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test stop/release session and preview with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_015, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);
    camSession->Release();

    int32_t intResult = camSession->Stop();
    EXPECT_EQ(intResult, 7400201);

    intResult = camSession->Release();
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataOutput with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_016, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;

    sptr<MetadataObjectListener> metaObjListener = new (std::nothrow) MetadataObjectListener(metaOutput);
    ASSERT_NE(metaObjListener, nullptr);

    metaObjListener->OnBufferAvailable();

    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> { MetadataObjectType::FACE });

    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();

    sptr<CaptureOutput> metadatOutput_2 = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput_2, nullptr);

    intResult = metaOutput->Release();
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput_2 = (sptr<MetadataOutput>&)metadatOutput_2;

    metaOutput_2->Release();

    metadataObjectTypes = metaOutput_2->GetSupportedMetadataObjectTypes();
    EXPECT_EQ(metadataObjectTypes.empty(), true);

    metaObjListener->OnBufferAvailable();

    sptr<MetadataOutput> metaOutput_3 = nullptr;

    sptr<MetadataObjectListener> metaObjListener_2 = new (std::nothrow) MetadataObjectListener(metaOutput_3);
    ASSERT_NE(metaObjListener_2, nullptr);

    metaObjListener_2->OnBufferAvailable();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photoOutput with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_017, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    PhotoCaptureSetting::QualityLevel quality = photoSetting->GetQuality();
    EXPECT_EQ(quality, PhotoCaptureSetting::QualityLevel::QUALITY_LEVEL_MEDIUM);

    photoSetting->SetQuality(PhotoCaptureSetting::QualityLevel::QUALITY_LEVEL_HIGH);
    photoSetting->SetQuality(quality);

    PhotoCaptureSetting::RotationConfig rotation = photoSetting->GetRotation();
    EXPECT_EQ(rotation, PhotoCaptureSetting::RotationConfig::Rotation_0);

    photoSetting->SetRotation(rotation);
    photoSetting->SetRotation(rotation);

    std::shared_ptr<Location> location = std::make_shared<Location>();
    location->latitude = 12.972442;
    location->longitude = 77.580643;

    photoSetting->SetGpsLocation(location->latitude, location->longitude);

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    intResult = photoOutput_1->Capture(photoSetting);
    EXPECT_EQ(intResult, 7400104);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    int32_t capture = photoOutput_1->Capture();
    EXPECT_EQ(capture, 0);

    int32_t cancelCapture = photoOutput_1->CancelCapture();
    EXPECT_EQ(cancelCapture, 0);

    intResult = photoOutput_1->Release();
    EXPECT_EQ(intResult, 0);

    intResult = photoOutput_1->Release();
    EXPECT_EQ(intResult, 7400201);

    cancelCapture = photoOutput_1->CancelCapture();
    EXPECT_EQ(cancelCapture, 7400104);

    bool isMirrorSupported = photoOutput_1->IsMirrorSupported();
    EXPECT_EQ(isMirrorSupported, false);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test captureCallback with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_018, TestSize.Level0)
{
    std::shared_ptr<HStreamCaptureCallbackImpl> captureCallback = std::make_shared<HStreamCaptureCallbackImpl>(nullptr);

    int32_t captureId = 2001;
    int32_t intResult = captureCallback->OnCaptureStarted(captureId);
    EXPECT_EQ(intResult, 0);

    int32_t frameCount = 0;
    intResult = captureCallback->OnCaptureEnded(captureId, frameCount);
    EXPECT_EQ(intResult, 0);

    int32_t errorCode = 0;
    intResult = captureCallback->OnCaptureError(captureId, errorCode);
    EXPECT_EQ(intResult, 0);

    uint64_t timestamp = 10;
    intResult = captureCallback->OnFrameShutter(captureId, timestamp);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<AppCallback> callback = nullptr;
    photoOutput_1->SetCallback(callback);

    callback = std::make_shared<AppCallback>();
    photoOutput_1->SetCallback(callback);
    photoOutput_1->SetCallback(callback);

    sptr<HStreamCaptureCallbackImpl> captureCallback_2 = new (std::nothrow) HStreamCaptureCallbackImpl(photoOutput_1);
    ASSERT_NE(captureCallback_2, nullptr);

    intResult = captureCallback_2->OnCaptureEnded(captureId, frameCount);
    EXPECT_EQ(intResult, 0);

    intResult = captureCallback_2->OnCaptureError(captureId, errorCode);
    EXPECT_EQ(intResult, 0);

    intResult = captureCallback_2->OnFrameShutter(captureId, timestamp);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test preview output repeat callback with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_019, TestSize.Level0)
{
    std::shared_ptr<PreviewOutputCallbackImpl> repeatCallback = std::make_shared<PreviewOutputCallbackImpl>();

    int32_t intResult = repeatCallback->OnFrameStarted();
    EXPECT_EQ(intResult, 0);

    int32_t frameCount = 0;
    intResult = repeatCallback->OnFrameEnded(frameCount);
    EXPECT_EQ(intResult, 0);

    int32_t errorCode = 0;
    intResult = repeatCallback->OnFrameError(errorCode);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput_1 = (sptr<PreviewOutput>&)previewOutput;

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<AppCallback> callback = nullptr;
    previewOutput_1->SetCallback(callback);

    callback = std::make_shared<AppCallback>();
    previewOutput_1->SetCallback(callback);
    previewOutput_1->SetCallback(callback);

    sptr<PreviewOutputCallbackImpl> repeatCallback_2 = new (std::nothrow) PreviewOutputCallbackImpl(previewOutput_1);
    ASSERT_NE(repeatCallback_2, nullptr);

    intResult = repeatCallback_2->OnFrameEnded(frameCount);
    EXPECT_EQ(intResult, 0);

    intResult = repeatCallback_2->OnFrameError(errorCode);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test preview output with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_020, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput_1 = (sptr<PreviewOutput>&)previewOutput;

    EXPECT_EQ(previewOutput_1->Start(), 7400103);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<Surface> surface = nullptr;

    previewOutput_1->AddDeferredSurface(surface);

    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<IBufferProducer> previewProducer = previewSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(previewProducer);

    previewOutput_1->AddDeferredSurface(pSurface);

    intResult = previewOutput_1->CaptureOutput::Release();
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    previewOutput_1->SetCallback(callback);

    intResult = previewOutput_1->Stop();
    EXPECT_EQ(intResult, 7400201);

    intResult = previewOutput_1->Release();
    EXPECT_EQ(intResult, 7400201);

    previewOutput_1->AddDeferredSurface(pSurface);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test video output with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_021, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = session_->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<VideoOutput> videoOutput_1 = (sptr<VideoOutput>&)videoOutput;

    std::shared_ptr<AppVideoCallback> callback = std::make_shared<AppVideoCallback>();

    intResult = videoOutput_1->Start();
    EXPECT_EQ(intResult, 7400103);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutput_1->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutput_1->Pause();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutput_1->Resume();
    EXPECT_EQ(intResult, 0);

    intResult = videoOutput_1->Stop();
    EXPECT_EQ(intResult, 0);

    intResult = videoOutput_1->CaptureOutput::Release();
    EXPECT_EQ(intResult, 0);

    videoOutput_1->SetCallback(callback);

    intResult = videoOutput_1->Pause();
    EXPECT_EQ(intResult, 7400201);

    intResult = videoOutput_1->Resume();
    EXPECT_EQ(intResult, 7400201);

    intResult = videoOutput_1->Stop();
    EXPECT_EQ(intResult, 7400201);

    intResult = videoOutput_1->Release();
    EXPECT_EQ(intResult, 7400201);

    session_->Release();
    EXPECT_EQ(videoOutput_1->Start(), 7400103);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test video output repeat callback with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_022, TestSize.Level0)
{
    std::shared_ptr<VideoOutputCallbackImpl> repeatCallback = std::make_shared<VideoOutputCallbackImpl>();

    int32_t intResult = repeatCallback->OnFrameStarted();
    EXPECT_EQ(intResult, 0);

    int32_t frameCount = 0;
    intResult = repeatCallback->OnFrameEnded(frameCount);
    EXPECT_EQ(intResult, 0);

    int32_t errorCode = 0;
    intResult = repeatCallback->OnFrameError(errorCode);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = camSession->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<VideoOutput> videoOutput_1 = (sptr<VideoOutput>&)videoOutput;

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<AppVideoCallback> callback = nullptr;
    videoOutput_1->SetCallback(callback);

    callback = std::make_shared<AppVideoCallback>();
    videoOutput_1->SetCallback(callback);
    videoOutput_1->SetCallback(callback);

    sptr<VideoOutputCallbackImpl> repeatCallback_2 = new (std::nothrow) VideoOutputCallbackImpl(videoOutput_1);
    ASSERT_NE(repeatCallback_2, nullptr);

    intResult = repeatCallback->OnFrameStarted();
    EXPECT_EQ(intResult, 0);

    intResult = repeatCallback->OnFrameEnded(frameCount);
    EXPECT_EQ(intResult, 0);

    intResult = repeatCallback_2->OnFrameError(errorCode);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HCameraDeviceProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_023, TestSize.Level0)
{
    sptr<CameraInput> input_1 = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceService> deviceObj = input_1->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    sptr<ICameraDeviceServiceCallback> callback = nullptr;
    int32_t intResult = deviceObj->SetCallback(callback);
    EXPECT_EQ(intResult, 200);

    std::shared_ptr<OHOS::Camera::CameraMetadata> settings = nullptr;
    intResult = deviceObj->UpdateSetting(settings);
    EXPECT_EQ(intResult, 200);

    std::vector<int32_t> results = {};
    intResult = deviceObj->GetEnabledResults(results);
    EXPECT_EQ(intResult, 0);

    deviceObj->EnableResult(results);
    deviceObj->DisableResult(results);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HCameraServiceProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_024, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    sptr<ICameraService> serviceProxy = iface_cast<ICameraService>(object);
    ASSERT_NE(serviceProxy, nullptr);

    sptr<ICameraServiceCallback> callback = nullptr;
    int32_t intResult = serviceProxy->SetCameraCallback(callback);
    EXPECT_EQ(intResult, 200);

    sptr<ICameraMuteServiceCallback> callback_2 = nullptr;
    intResult = serviceProxy->SetMuteCallback(callback_2);
    EXPECT_EQ(intResult, 200);

    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    sptr<IStreamCapture> output = nullptr;
    sptr<IBufferProducer> Producer = nullptr;
    intResult = serviceProxy->CreatePhotoOutput(Producer, format, width, height, output);
    EXPECT_EQ(intResult, 200);

    sptr<IStreamRepeat> output_2 = nullptr;
    intResult = serviceProxy->CreatePreviewOutput(Producer, format, width, height, output_2);
    EXPECT_EQ(intResult, 200);

    intResult = serviceProxy->CreateDeferredPreviewOutput(format, width, height, output_2);
    EXPECT_EQ(intResult, 200);

    sptr<IStreamMetadata> output_3 = nullptr;
    intResult = serviceProxy->CreateMetadataOutput(Producer, format, {1}, output_3);
    EXPECT_EQ(intResult, 200);

    intResult = serviceProxy->CreateVideoOutput(Producer, format, width, height, output_2);
    EXPECT_EQ(intResult, 200);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HCameraServiceProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_025, TestSize.Level0)
{
    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    std::string cameraId = "";
    std::vector<std::string> cameraIds = {};
    std::vector<std::shared_ptr<OHOS::Camera::CameraMetadata>> cameraAbilityList = {};

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> Producer = Surface->GetProducer();

    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    sptr<ICameraService> serviceProxy = iface_cast<ICameraService>(object);
    ASSERT_NE(serviceProxy, nullptr);

    sptr<ICameraServiceCallback> callback = new (std::nothrow) CameraStatusServiceCallback(manager_);
    ASSERT_NE(callback, nullptr);
    int32_t intResult = serviceProxy->SetCameraCallback(callback);
    EXPECT_EQ(intResult, -1);

    sptr<ICameraMuteServiceCallback> callback_2 = new (std::nothrow) CameraMuteServiceCallback(manager_);
    ASSERT_NE(callback_2, nullptr);
    serviceProxy->SetMuteCallback(callback_2);
    EXPECT_EQ(intResult, -1);

    intResult = serviceProxy->GetCameras(cameraIds, cameraAbilityList);
    EXPECT_EQ(intResult, -1);

    sptr<ICaptureSession> session = nullptr;
    intResult = serviceProxy->CreateCaptureSession(session);
    EXPECT_EQ(intResult, -1);

    sptr<IStreamCapture> output = nullptr;
    intResult = serviceProxy->CreatePhotoOutput(Producer, format, width, height, output);
    EXPECT_EQ(intResult, -1);

    width = PREVIEW_DEFAULT_WIDTH;
    height = PREVIEW_DEFAULT_HEIGHT;
    sptr<IStreamRepeat> output_2 = nullptr;
    intResult = serviceProxy->CreatePreviewOutput(Producer, format, width, height, output_2);
    EXPECT_EQ(intResult, -1);

    intResult = serviceProxy->CreateDeferredPreviewOutput(format, width, height, output_2);
    EXPECT_EQ(intResult, -1);

    sptr<IStreamMetadata> output_3 = nullptr;
    intResult = serviceProxy->CreateMetadataOutput(Producer, format, {1}, output_3);
    EXPECT_EQ(intResult, -1);

    intResult = serviceProxy->CreateVideoOutput(Producer, format, width, height, output_2);
    EXPECT_EQ(intResult, -1);

    intResult = serviceProxy->SetListenerObject(object);
    EXPECT_EQ(intResult, 200);

    bool muteMode = true;
    intResult = serviceProxy->IsCameraMuted(muteMode);
    EXPECT_EQ(intResult, -1);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HCameraDeviceProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_026, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);

    sptr<ICameraDeviceService> deviceObj = iface_cast<ICameraDeviceService>(object);
    ASSERT_NE(deviceObj, nullptr);

    int32_t intResult = deviceObj->Open();
    EXPECT_EQ(intResult, -1);

    intResult = deviceObj->Close();
    EXPECT_EQ(intResult, -1);

    intResult = deviceObj->Release();
    EXPECT_EQ(intResult, -1);

    sptr<CameraInput> input = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceServiceCallback> callback = new (std::nothrow) CameraDeviceServiceCallback(input);
    ASSERT_NE(callback, nullptr);

    intResult = deviceObj->SetCallback(callback);
    EXPECT_EQ(intResult, -1);

    std::shared_ptr<OHOS::Camera::CameraMetadata> settings = cameras_[0]->GetMetadata();
    ASSERT_NE(settings, nullptr);

    intResult = deviceObj->UpdateSetting(settings);
    EXPECT_EQ(intResult, -1);

    std::vector<int32_t> results = {};
    intResult = deviceObj->GetEnabledResults(results);
    EXPECT_EQ(intResult, 200);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HCaptureSessionProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_027, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);

    sptr<ICaptureSession> captureSession = iface_cast<ICaptureSession>(object);
    ASSERT_NE(captureSession, nullptr);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    sptr<IStreamCommon> stream = nullptr;

    int32_t intResult = captureSession->AddOutput(previewOutput->GetStreamType(), stream);
    EXPECT_EQ(intResult, 200);

    sptr<ICameraDeviceService> cameraDevice = nullptr;
    intResult = captureSession->RemoveInput(cameraDevice);
    EXPECT_EQ(intResult, 200);

    intResult = captureSession->RemoveOutput(previewOutput->GetStreamType(), stream);
    EXPECT_EQ(intResult, 200);

    sptr<ICaptureSessionCallback> callback = nullptr;
    EXPECT_EQ(intResult, 200);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HCaptureSessionProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_028, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);

    sptr<ICaptureSession> captureSession = iface_cast<ICaptureSession>(object);
    ASSERT_NE(captureSession, nullptr);

    int32_t intResult = captureSession->BeginConfig();
    EXPECT_EQ(intResult, -1);

    sptr<CameraInput> input_1 = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceService> deviceObj = input_1->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    intResult = captureSession->AddInput(deviceObj);
    EXPECT_EQ(intResult, -1);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = captureSession->AddOutput(previewOutput->GetStreamType(), previewOutput->GetStream());
    EXPECT_EQ(intResult, -1);

    intResult = captureSession->Start();
    EXPECT_EQ(intResult, -1);

    intResult = captureSession->Release();
    EXPECT_EQ(intResult, -1);

    sptr<ICaptureSessionCallback> callback = new (std::nothrow) CaptureSessionCallback(session_);
    ASSERT_NE(callback, nullptr);

    intResult = captureSession->SetCallback(callback);
    EXPECT_EQ(intResult, -1);

    CaptureSessionState currentState = CaptureSessionState::SESSION_CONFIG_INPROGRESS;
    intResult = captureSession->GetSessionState(currentState);
    EXPECT_EQ(intResult, -1);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HStreamCaptureProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_029, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);

    sptr<IStreamCapture> capture = iface_cast<IStreamCapture>(object);
    ASSERT_NE(capture, nullptr);

    sptr<IStreamCaptureCallback> callback = nullptr;
    int32_t intResult = capture->SetCallback(callback);
    EXPECT_EQ(intResult, 200);

    bool isEnabled = false;
    sptr<OHOS::IBufferProducer> producer = nullptr;
    intResult = capture->SetThumbnail(isEnabled, producer);
    EXPECT_EQ(intResult, 200);

    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    producer = previewSurface->GetProducer();

    intResult = capture->SetThumbnail(isEnabled, producer);
    EXPECT_EQ(intResult, -1);

    std::shared_ptr<OHOS::Camera::CameraMetadata> captureSettings = nullptr;
    intResult = capture->Capture(captureSettings);
    EXPECT_EQ(intResult, 200);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HStreamCaptureProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_030, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);

    sptr<IStreamCapture> capture = iface_cast<IStreamCapture>(object);
    ASSERT_NE(capture, nullptr);

    std::shared_ptr<OHOS::Camera::CameraMetadata> captureSettings = cameras_[0]->GetMetadata();
    int32_t intResult = capture->Capture(captureSettings);
    EXPECT_EQ(intResult, -1);

    intResult = capture->CancelCapture();
    EXPECT_EQ(intResult, -1);

    intResult = capture->Release();
    EXPECT_EQ(intResult, -1);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;
    sptr<IStreamCaptureCallback> callback = new (std::nothrow) HStreamCaptureCallbackImpl(photoOutput_1);
    ASSERT_NE(callback, nullptr);

    intResult = capture->SetCallback(callback);
    EXPECT_EQ(intResult, -1);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HStreamMetadataProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_031, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);

    sptr<IStreamMetadata> metadata = iface_cast<IStreamMetadata>(object);
    ASSERT_NE(metadata, nullptr);

    int32_t intResult = metadata->Start();
    EXPECT_EQ(intResult, -1);

    intResult = metadata->Release();
    EXPECT_EQ(intResult, -1);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HStreamRepeatProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_032, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);

    sptr<IStreamRepeat> repeat = iface_cast<IStreamRepeat>(object);
    ASSERT_NE(repeat, nullptr);

    sptr<IStreamRepeatCallback> callback = nullptr;
    int32_t intResult = repeat->SetCallback(callback);
    EXPECT_EQ(intResult, 200);

    sptr<OHOS::IBufferProducer> producer = nullptr;
    intResult = repeat->AddDeferredSurface(producer);
    EXPECT_EQ(intResult, 200);

    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);

    repeat = iface_cast<IStreamRepeat>(object);
    ASSERT_NE(repeat, nullptr);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    sptr<PreviewOutput> previewOutput_1 = (sptr<PreviewOutput>&)previewOutput;
    callback = new (std::nothrow) PreviewOutputCallbackImpl(previewOutput_1);
    ASSERT_NE(callback, nullptr);

    intResult = repeat->SetCallback(callback);
    EXPECT_EQ(intResult, -1);

    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    producer = previewSurface->GetProducer();

    intResult = repeat->AddDeferredSurface(producer);
    EXPECT_EQ(intResult, -1);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test set the Thumbnail with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_033, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<SurfaceListener> listener = new SurfaceListener("Preview", SurfaceType::PREVIEW, g_previewFd, previewSurface);
    sptr<IBufferConsumerListener> listener_1 = (sptr<IBufferConsumerListener>&)listener;

    photoOutput_1->SetThumbnailListener(listener_1);

    intResult = photoOutput_1->IsQuickThumbnailSupported();

    if (!IsSupportNow()) {
        EXPECT_EQ(intResult, -1);
    } else {
        EXPECT_EQ(intResult, 0);
    }

    bool isEnabled = false;
    intResult = photoOutput_1->SetThumbnail(isEnabled);
    EXPECT_EQ(intResult, 0);

    photoOutput_1->SetThumbnailListener(listener_1);

    intResult = photoOutput_1->Release();
    EXPECT_EQ(intResult, 0);

    intResult = photoOutput_1->IsQuickThumbnailSupported();
    EXPECT_EQ(intResult, 7400104);

    intResult = photoOutput_1->SetThumbnail(isEnabled);
    EXPECT_EQ(intResult, 7400104);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test submit device control setting with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_034, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);
    camSession->Release();

    int32_t intResult = camSession->UnlockForControl();
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test MuteCamera with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_035, TestSize.Level0)
{
    bool cameraMuted = manager_->IsCameraMuted();
    EXPECT_EQ(cameraMuted, false);

    bool cameraMuteSupported = manager_->IsCameraMuteSupported();
    if (!cameraMuteSupported) {
        return;
    }

    manager_->MuteCamera(cameraMuted);

    sptr<CameraMuteServiceCallback> muteService = new (std::nothrow) CameraMuteServiceCallback(nullptr);
    ASSERT_NE(muteService, nullptr);

    int32_t muteCameraState = muteService->OnCameraMute(cameraMuted);
    EXPECT_EQ(muteCameraState, 0);

    muteService = new (std::nothrow) CameraMuteServiceCallback(manager_);
    ASSERT_NE(muteService, nullptr);

    muteCameraState = muteService->OnCameraMute(cameraMuted);
    EXPECT_EQ(muteCameraState, 0);

    std::shared_ptr<CameraMuteListener> listener = nullptr;
    manager_->RegisterCameraMuteListener(listener);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test service callback with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_036, TestSize.Level0)
{
    std::string cameraIdtest = "";
    FlashStatus status = FLASH_STATUS_OFF;

    sptr<CameraStatusServiceCallback> camServiceCallback = new (std::nothrow) CameraStatusServiceCallback(nullptr);
    ASSERT_NE(camServiceCallback, nullptr);
    int32_t cameraStatusChanged = camServiceCallback->OnFlashlightStatusChanged(cameraIdtest, status);
    EXPECT_EQ(cameraStatusChanged, 0);

    sptr<CameraDeviceServiceCallback> camDeviceSvcCallback = new (std::nothrow) CameraDeviceServiceCallback();
    ASSERT_NE(camDeviceSvcCallback, nullptr);
    int32_t onError = camDeviceSvcCallback->OnError(CAMERA_DEVICE_PREEMPTED, 0);
    EXPECT_EQ(onError, 0);

    std::shared_ptr<OHOS::Camera::CameraMetadata> result = nullptr;
    int32_t onResult = camDeviceSvcCallback->OnResult(0, result);
    EXPECT_EQ(onResult, CAMERA_INVALID_ARG);

    sptr<CameraInput> input = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceService> deviceObj = input->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    sptr<CameraDevice> camdeviceObj = nullptr;
    sptr<CameraInput> camInput_1 = new (std::nothrow) CameraInput(deviceObj, camdeviceObj);
    ASSERT_NE(camInput_1, nullptr);

    camDeviceSvcCallback = nullptr;
    camDeviceSvcCallback = new (std::nothrow) CameraDeviceServiceCallback(camInput_1);
    ASSERT_NE(camDeviceSvcCallback, nullptr);
    onResult = camDeviceSvcCallback->OnResult(0, result);
    EXPECT_EQ(onResult, CAMERA_INVALID_ARG);

    sptr<CameraInput> camInput_2 = new (std::nothrow) CameraInput(deviceObj, cameras_[0]);
    ASSERT_NE(camInput_2, nullptr);
    camDeviceSvcCallback = nullptr;
    camDeviceSvcCallback = new (std::nothrow) CameraDeviceServiceCallback(camInput_2);
    ASSERT_NE(camDeviceSvcCallback, nullptr);
    onError = camDeviceSvcCallback->OnError(CAMERA_DEVICE_PREEMPTED, 0);
    EXPECT_EQ(onError, 0);

    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    camInput_2->SetErrorCallback(callback);

    camDeviceSvcCallback = nullptr;
    camDeviceSvcCallback = new (std::nothrow) CameraDeviceServiceCallback(camInput_2);
    ASSERT_NE(camDeviceSvcCallback, nullptr);
    onError = camDeviceSvcCallback->OnError(CAMERA_DEVICE_PREEMPTED, 0);
    EXPECT_EQ(onError, 0);

    callback = nullptr;
    camInput_2->SetErrorCallback(callback);
    camInput_2->SetResultCallback(callback);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test create camera input instance with provided camera position and type anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_037, TestSize.Level0)
{
    CameraPosition cameraPosition = cameras_[0]->GetPosition();
    CameraType cameraType = cameras_[0]->GetCameraType();
    sptr<CaptureInput> camInputtest = manager_->CreateCameraInput(cameraPosition, cameraType);
    ASSERT_NE(camInputtest, nullptr);

    cameraType = CAMERA_TYPE_UNSUPPORTED;
    cameraPosition = CAMERA_POSITION_UNSPECIFIED;
    camInputtest = manager_->CreateCameraInput(cameraPosition, cameraType);
    EXPECT_EQ(camInputtest, nullptr);

    cameraType = CAMERA_TYPE_UNSUPPORTED;
    cameraPosition = cameras_[0]->GetPosition();
    camInputtest = manager_->CreateCameraInput(cameraPosition, cameraType);
    EXPECT_EQ(camInputtest, nullptr);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test the supported exposure compensation range and Zoom Ratio range with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_038, TestSize.Level0)
{
    std::string cameraId = "";
    dmDeviceInfo deviceInfo = {};
    auto metaData = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    sptr<CameraDevice> camdeviceObj = new (std::nothrow) CameraDevice(cameraId, metaData, deviceInfo);
    ASSERT_NE(camdeviceObj, nullptr);

    std::vector<float> zoomRatioRange = camdeviceObj->GetZoomRatioRange();
    EXPECT_EQ(zoomRatioRange.empty(), true);

    std::vector<float> exposureBiasRange = camdeviceObj->GetExposureBiasRange();
    EXPECT_EQ(exposureBiasRange.empty(), true);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test session with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_039, TestSize.Level0)
{
    sptr<CameraInput> input = (sptr<CameraInput>&)input_;
    input->Close();
    input->SetMetadataResultProcessor(session_->GetMetadataResultProcessor());

    sptr<CaptureInput> input_2 = (sptr<CaptureInput>&)input;

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 7400105);

    intResult = session_->AddInput(input_2);
    EXPECT_EQ(intResult, 7400201);

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);
    camSession->Release();

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test create preview output with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_040, TestSize.Level0)
{
    sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
    sptr<IBufferProducer> previewProducer = previewSurface->GetProducer();
    sptr<Surface> pSurface = Surface::CreateSurfaceAsProducer(previewProducer);

    sptr<Surface> pSurface_1 = nullptr;

    sptr<CaptureOutput> previewOutput = manager_->CreatePreviewOutput(previewProfiles[0], pSurface_1);
    EXPECT_EQ(previewOutput, nullptr);

    sptr<CaptureOutput> previewOutput_1 = manager_->CreateDeferredPreviewOutput(previewProfiles[0]);
    ASSERT_NE(previewOutput_1, nullptr);

    // height is zero
    CameraFormat previewFormat = previewProfiles[0].GetCameraFormat();
    Size previewSize;
    previewSize.width = previewProfiles[0].GetSize().width;
    previewSize.height = 0;
    previewProfiles[0] = Profile(previewFormat, previewSize);

    previewOutput = manager_->CreatePreviewOutput(previewProfiles[0], pSurface);
    EXPECT_EQ(previewOutput, nullptr);

    previewOutput_1 = manager_->CreateDeferredPreviewOutput(previewProfiles[0]);
    EXPECT_EQ(previewOutput_1, nullptr);

    sptr<CaptureOutput> previewOutput_2 = manager_->CreatePreviewOutput(previewProducer, previewFormat);
    EXPECT_EQ(previewOutput_2, nullptr);

    sptr<CaptureOutput> previewOutput_3 = nullptr;
    previewOutput_3 = manager_->CreateCustomPreviewOutput(pSurface, previewSize.width, previewSize.height);
    EXPECT_EQ(previewOutput_3, nullptr);

    // width is zero
    previewSize.width = 0;
    previewSize.height = PREVIEW_DEFAULT_HEIGHT;
    previewProfiles[0] = Profile(previewFormat, previewSize);

    previewOutput = manager_->CreatePreviewOutput(previewProfiles[0], pSurface);
    EXPECT_EQ(previewOutput, nullptr);

    previewOutput_1 = manager_->CreateDeferredPreviewOutput(previewProfiles[0]);
    EXPECT_EQ(previewOutput_1, nullptr);

    // format is CAMERA_FORMAT_INVALID
    previewFormat = CAMERA_FORMAT_INVALID;
    previewSize.width = PREVIEW_DEFAULT_WIDTH;
    previewProfiles[0] = Profile(previewFormat, previewSize);

    previewOutput = manager_->CreatePreviewOutput(previewProfiles[0], pSurface);
    EXPECT_EQ(previewOutput, nullptr);

    previewOutput_1 = manager_->CreateDeferredPreviewOutput(previewProfiles[0]);
    EXPECT_EQ(previewOutput_1, nullptr);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test create photo output with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_041, TestSize.Level0)
{
    sptr<IConsumerSurface> photosurface = IConsumerSurface::Create();
    sptr<IBufferProducer> surfaceProducer_1 = photosurface->GetProducer();

    sptr<IBufferProducer> surfaceProducer_2 = nullptr;

    sptr<CaptureOutput> photoOutput = manager_->CreatePhotoOutput(photoProfiles[0], surfaceProducer_2);
    EXPECT_EQ(photoOutput, nullptr);

    // height is zero
    CameraFormat photoFormat = photoProfiles[0].GetCameraFormat();
    Size photoSize;
    photoSize.width = photoProfiles[0].GetSize().width;
    photoSize.height = 0;
    photoProfiles[0] = Profile(photoFormat, photoSize);

    photoOutput = manager_->CreatePhotoOutput(photoProfiles[0], surfaceProducer_1);
    EXPECT_EQ(photoOutput, nullptr);

    // width is zero
    photoSize.width = 0;
    photoSize.height = PHOTO_DEFAULT_HEIGHT;
    photoProfiles[0] = Profile(photoFormat, photoSize);

    photoOutput = manager_->CreatePhotoOutput(photoProfiles[0], surfaceProducer_1);
    EXPECT_EQ(photoOutput, nullptr);

    // format is CAMERA_FORMAT_INVALID
    photoFormat = CAMERA_FORMAT_INVALID;
    photoSize.width = PHOTO_DEFAULT_WIDTH;
    photoProfiles[0] = Profile(photoFormat, photoSize);

    photoOutput = manager_->CreatePhotoOutput(photoProfiles[0], surfaceProducer_1);
    EXPECT_EQ(photoOutput, nullptr);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test create video output with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_042, TestSize.Level0)
{
    sptr<IConsumerSurface> videoSurface = IConsumerSurface::Create();
    sptr<IBufferProducer> videoProducer = videoSurface->GetProducer();
    sptr<Surface> pSurface_1 = Surface::CreateSurfaceAsProducer(videoProducer);

    sptr<Surface> pSurface_2 = nullptr;

    sptr<CaptureOutput> videoOutput = manager_->CreateVideoOutput(videoProfiles[0], pSurface_2);
    EXPECT_EQ(videoOutput, nullptr);

    sptr<CaptureOutput> videoOutput_1 = manager_->CreateVideoOutput(pSurface_2);
    EXPECT_EQ(videoOutput_1, nullptr);

    // height is zero
    std::vector<int32_t> framerates;
    CameraFormat videoFormat = videoProfiles[0].GetCameraFormat();
    Size videoSize;
    videoSize.width = videoProfiles[0].GetSize().width;
    videoSize.height = 0;
    VideoProfile videoProfile_1 = VideoProfile(videoFormat, videoSize, framerates);

    videoOutput = manager_->CreateVideoOutput(videoProfile_1, pSurface_1);
    EXPECT_EQ(videoOutput, nullptr);

    // width is zero
    videoSize.width = 0;
    videoSize.height = VIDEO_DEFAULT_HEIGHT;
    videoProfile_1 = VideoProfile(videoFormat, videoSize, framerates);

    videoOutput = manager_->CreateVideoOutput(videoProfile_1, pSurface_1);
    EXPECT_EQ(videoOutput, nullptr);

    // format is CAMERA_FORMAT_INVALID
    videoFormat = CAMERA_FORMAT_INVALID;
    videoSize.width = VIDEO_DEFAULT_WIDTH;
    videoProfile_1 = VideoProfile(videoFormat, videoSize, framerates);

    videoOutput = manager_->CreateVideoOutput(videoProfile_1, pSurface_1);
    EXPECT_EQ(videoOutput, nullptr);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test camera settings with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_043, TestSize.Level0)
{
    sptr<CameraInput> input = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceService> deviceObj_2 = input->GetCameraDevice();
    ASSERT_NE(deviceObj_2, nullptr);

    sptr<CameraDevice> camdeviceObj = nullptr;
    sptr<CameraInput> camInput_2 = new (std::nothrow) CameraInput(deviceObj_2, camdeviceObj);
    ASSERT_NE(camInput_2, nullptr);

    std::string cameraId = input->GetCameraId();

    std::string getCameraSettings_1 = input->GetCameraSettings();
    int32_t intResult = input->SetCameraSettings(getCameraSettings_1);
    EXPECT_EQ(intResult, 0);

    std::string getCameraSettings_2 = "";

    intResult = camInput_2->SetCameraSettings(getCameraSettings_2);
    EXPECT_EQ(intResult, 2);

    sptr<CameraInput> camInput_3 = (sptr<CameraInput>&)input_;
    getCameraSettings_2 = camInput_3->GetCameraSettings();
    intResult = camInput_3->SetCameraSettings(getCameraSettings_2);
    EXPECT_EQ(intResult, 0);

    sptr<CameraInput> input_1 = (sptr<CameraInput>&)input_;
    input_1->Close();
    input_1->SetMetadataResultProcessor(session_->GetMetadataResultProcessor());

    sptr<CaptureInput> input_2 = (sptr<CaptureInput>&)input_1;

    intResult = input_2->Open();
    EXPECT_EQ(intResult, 7400201);

    intResult = camInput_2->Release();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test submit device control setting with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_044, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    camSession->LockForControl();

    std::vector<float> zoomRatioRange = camSession->GetZoomRatioRange();
    if (!zoomRatioRange.empty()) {
        camSession->SetZoomRatio(zoomRatioRange[0]);
    }

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->Close();

    intResult = camSession->UnlockForControl();
    EXPECT_EQ(intResult, 0);

    camSession->LockForControl();

    intResult = camSession->UnlockForControl();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test abnormal branches with empty inputDevice
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_045, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    camSession->innerInputDevice_ = nullptr;
    VideoStabilizationMode mode = MIDDLE;

    intResult = camSession->GetActiveVideoStabilizationMode(mode);
    EXPECT_EQ(intResult, 0);

    std::vector<VideoStabilizationMode> videoStabilizationMode = camSession->GetSupportedStabilizationMode();
    EXPECT_EQ(videoStabilizationMode.empty(), true);

    intResult = camSession->GetSupportedStabilizationMode(videoStabilizationMode);
    EXPECT_EQ(intResult, 0);

    std::vector<ExposureMode> getSupportedExpModes = camSession->GetSupportedExposureModes();
    EXPECT_EQ(getSupportedExpModes.empty(), true);

    intResult = camSession->GetSupportedExposureModes(getSupportedExpModes);
    EXPECT_EQ(intResult, 0);

    ExposureMode exposureMode = camSession->GetExposureMode();
    EXPECT_EQ(exposureMode, EXPOSURE_MODE_UNSUPPORTED);

    intResult = camSession->GetExposureMode(exposureMode);
    EXPECT_EQ(intResult, 0);

    Point exposurePointGet = camSession->GetMeteringPoint();
    EXPECT_EQ(exposurePointGet.x, 0);
    EXPECT_EQ(exposurePointGet.y, 0);

    intResult = camSession->GetMeteringPoint(exposurePointGet);
    EXPECT_EQ(intResult, 0);

    std::vector<float> getExposureBiasRange = camSession->GetExposureBiasRange();
    EXPECT_EQ(getExposureBiasRange.empty(), true);

    intResult = camSession->GetExposureBiasRange(getExposureBiasRange);
    EXPECT_EQ(intResult, 0);

    float exposureValue = camSession->GetExposureValue();
    EXPECT_EQ(exposureValue, 0);

    intResult = camSession->GetExposureValue(exposureValue);
    EXPECT_EQ(intResult, 0);

    camSession->LockForControl();

    intResult = camSession->SetExposureBias(exposureValue);
    EXPECT_EQ(intResult, 7400102);

    std::vector<FocusMode> getSupportedFocusModes = camSession->GetSupportedFocusModes();
    EXPECT_EQ(getSupportedFocusModes.empty(), true);

    intResult = camSession->GetSupportedFocusModes(getSupportedFocusModes);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test abnormal branches with empty innerInputDevice_
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_046, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    camSession->innerInputDevice_ = nullptr;

    FocusMode focusMode = camSession->GetFocusMode();
    EXPECT_EQ(focusMode, FOCUS_MODE_MANUAL);

    intResult = camSession->GetFocusMode(focusMode);
    EXPECT_EQ(intResult, 0);

    Point focusPoint = camSession->GetFocusPoint();
    EXPECT_EQ(focusPoint.x, 0);
    EXPECT_EQ(focusPoint.y, 0);

    intResult = camSession->GetFocusPoint(focusPoint);
    EXPECT_EQ(intResult, 0);

    float focalLength = camSession->GetFocalLength();
    EXPECT_EQ(focalLength, 0);

    intResult = camSession->GetFocalLength(focalLength);
    EXPECT_EQ(intResult, 0);

    std::vector<FlashMode> getSupportedFlashModes = camSession->GetSupportedFlashModes();
    EXPECT_EQ(getSupportedFlashModes.empty(), true);

    intResult = camSession->GetSupportedFlashModes(getSupportedFlashModes);
    EXPECT_EQ(intResult, 0);

    FlashMode flashMode = camSession->GetFlashMode();
    EXPECT_EQ(flashMode, FLASH_MODE_CLOSE);

    intResult = camSession->GetFlashMode(flashMode);
    EXPECT_EQ(intResult, 0);

    std::vector<float> zoomRatioRange = camSession->GetZoomRatioRange();
    EXPECT_EQ(zoomRatioRange.empty(), true);

    intResult = camSession->GetZoomRatioRange(zoomRatioRange);
    EXPECT_EQ(intResult, 0);

    float zoomRatio = camSession->GetZoomRatio();
    EXPECT_EQ(zoomRatio, 0);

    intResult = camSession->GetZoomRatio(zoomRatio);
    EXPECT_EQ(intResult, 0);

    camSession->LockForControl();

    intResult = camSession->SetZoomRatio(zoomRatio);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->SetMeteringPoint(focusPoint);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test submit device control setting with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_047, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    camSession->LockForControl();

    std::vector<float> exposureBiasRange = camSession->GetExposureBiasRange();
    if (!exposureBiasRange.empty()) {
        camSession->SetExposureBias(exposureBiasRange[0]);
    }

    camSession->innerInputDevice_ = nullptr;

    intResult = camSession->UnlockForControl();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test stream_ null with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_048, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = camSession->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    previewOutput->Release();
    previewOutput->SetSession(camSession);

    photoOutput->Release();
    photoOutput->SetSession(camSession);

    videoOutput->Release();
    videoOutput->SetSession(camSession);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;
    sptr<PreviewOutput> previewOutput_1 = (sptr<PreviewOutput>&)previewOutput;
    sptr<VideoOutput> videoOutput_1 = (sptr<VideoOutput>&)videoOutput;

    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    photoOutput_1->SetCallback(callback);

    intResult = previewOutput_1->Start();
    EXPECT_EQ(intResult, 7400201);

    intResult = photoOutput_1->Capture(photoSetting);
    EXPECT_EQ(intResult, 7400201);

    intResult = photoOutput_1->Capture();
    EXPECT_EQ(intResult, 7400201);

    intResult = photoOutput_1->CancelCapture();
    EXPECT_EQ(intResult, 7400201);

    intResult = videoOutput_1->Start();
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test stream_ null with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_049, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = camSession->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    metadatOutput->Release();
    metadatOutput->SetSession(camSession);

    sptr<MetadataOutput> metadatOutput_1 = (sptr<MetadataOutput>&)metadatOutput;

    intResult = metadatOutput_1->Start();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test cameraObj null with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_050, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    ReleaseInput();
    photoOutput->SetSession(camSession);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    bool isMirrorSupported = photoOutput_1->IsMirrorSupported();
    EXPECT_EQ(isMirrorSupported, false);

    intResult = photoOutput_1->IsQuickThumbnailSupported();
    EXPECT_EQ(intResult, 7400104);

    bool isEnabled = false;
    intResult = photoOutput_1->SetThumbnail(isEnabled);
    EXPECT_EQ(intResult, 7400104);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test CallbackProxy_cpp with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_052, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);

    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);

    sptr<CameraInput> input = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceServiceCallback> callback = new (std::nothrow) CameraDeviceServiceCallback(input);
    ASSERT_NE(callback, nullptr);

    sptr<HCameraDeviceCallbackProxy> deviceCallback = (sptr<HCameraDeviceCallbackProxy>&)callback;
    deviceCallback = new (std::nothrow) HCameraDeviceCallbackProxy(object);
    ASSERT_NE(deviceCallback, nullptr);

    uint64_t timestamp = 10;
    std::shared_ptr<OHOS::Camera::CameraMetadata> result = nullptr;
    int32_t intResult = deviceCallback->OnResult(timestamp, result);
    EXPECT_EQ(intResult, 200);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test prelaunch the camera with serviceProxy_ null anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_058, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    std::string cameraId = camInput->GetCameraId();
    int activeTime = 0;
    EffectParam effectParam = {0, 0, 0};

    int32_t intResult = camManagerObj->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::TRANSIENT_ACTIVE_PARAM_OHOS,
        activeTime, effectParam);
    if (!IsSupportNow()) {
        EXPECT_EQ(intResult, 7400201);
    } else {
        EXPECT_EQ(intResult, 0);
    }

    intResult = camManagerObj->PrelaunchCamera();
    EXPECT_EQ(intResult, 0);
    // CameraManager instance has been changed, need recover
    camManagerObj->SetServiceProxy(nullptr);

    intResult = camManagerObj->PrelaunchCamera();
    EXPECT_EQ(intResult, 7400201);

    bool isPreLaunchSupported = camManagerObj->IsPrelaunchSupported(cameras_[0]);
    if (!IsSupportNow()) {
        EXPECT_EQ(isPreLaunchSupported, false);
    } else {
        EXPECT_EQ(isPreLaunchSupported, true);
    }

    intResult = camManagerObj->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::TRANSIENT_ACTIVE_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(intResult, 7400201);
    // CameraManager recovered
    camManagerObj->InitCameraManager();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test serviceProxy_ null with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_059, TestSize.Level0)
{
    sptr<Surface> pSurface = nullptr;
    sptr<IBufferProducer> surfaceProducer = nullptr;

    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    // CameraManager instance has been changed, need recover
    camManagerObj->ClearCameraDeviceListCache();

    std::vector<sptr<CameraDevice>> camdeviceObj_1 = camManagerObj->GetSupportedCameras();
    ASSERT_TRUE(camdeviceObj_1.size() != 0);

    camManagerObj->SetServiceProxy(nullptr);

    sptr<CaptureSession> captureSession = camManagerObj->CreateCaptureSession();
    EXPECT_EQ(captureSession, nullptr);

    sptr<CaptureOutput> metadataOutput = camManagerObj->CreateMetadataOutput();
    EXPECT_EQ(metadataOutput, nullptr);

    sptr<CaptureOutput> previewOutput = camManagerObj->CreatePreviewOutput(previewProfiles[0], pSurface);
    EXPECT_EQ(previewOutput, nullptr);

    previewOutput = camManagerObj->CreateDeferredPreviewOutput(previewProfiles[0]);
    EXPECT_EQ(previewOutput, nullptr);

    sptr<CaptureOutput> photoOutput = camManagerObj->CreatePhotoOutput(photoProfiles[0], surfaceProducer);
    EXPECT_EQ(photoOutput, nullptr);

    sptr<CaptureOutput> videoOutput = camManagerObj->CreateVideoOutput(videoProfiles[0], pSurface);
    EXPECT_EQ(videoOutput, nullptr);
    // CameraManager recovered
    camManagerObj->InitCameraManager();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test serviceProxy_ null with muteCamera anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_060, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    // CameraManager instance has been changed, need recover
    camManagerObj->SetServiceProxy(nullptr);

    bool cameraMuted = camManagerObj->IsCameraMuted();
    EXPECT_EQ(cameraMuted, false);

    camManagerObj->ClearCameraDeviceListCache();
    bool cameraMuteSupported = camManagerObj->IsCameraMuteSupported();
    EXPECT_EQ(cameraMuteSupported, true);

    camManagerObj->MuteCamera(cameraMuted);
    // CameraManager recovered
    camManagerObj->InitCameraManager();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test create camera input and construct device object with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_061, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    // CameraManager instance has been changed, need recover
    camManagerObj->SetServiceProxy(nullptr);

    std::string cameraId = "";
    dmDeviceInfo deviceInfo = {};
    auto metaData = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    sptr<CameraDevice> camdeviceObj_1 = new (std::nothrow) CameraDevice(cameraId, metaData, deviceInfo);
    ASSERT_NE(camdeviceObj_1, nullptr);

    cameraId = cameras_[0]->GetID();
    sptr<CameraDevice> camdeviceObj_2 = new (std::nothrow) CameraDevice(cameraId, metaData, deviceInfo);
    ASSERT_NE(camdeviceObj_2, nullptr);

    sptr<CameraDevice> camdeviceObj_3 = nullptr;
    sptr<CaptureInput> camInput = camManagerObj->CreateCameraInput(camdeviceObj_3);
    EXPECT_EQ(camInput, nullptr);

    camInput = camManagerObj->CreateCameraInput(camdeviceObj_1);
    EXPECT_EQ(camInput, nullptr);

    sptr<CameraInfo> camera = nullptr;
    camInput = camManagerObj->CreateCameraInput(camera);
    EXPECT_EQ(camInput, nullptr);

    CameraPosition cameraPosition = cameras_[0]->GetPosition();
    CameraType cameraType = cameras_[0]->GetCameraType();
    camInput = camManagerObj->CreateCameraInput(cameraPosition, cameraType);
    EXPECT_EQ(camInput, nullptr);

    sptr<CameraInput> input_2 = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceService> deviceObj_2 = input_2->GetCameraDevice();
    ASSERT_NE(deviceObj_2, nullptr);

    input_2 = new (std::nothrow) CameraInput(deviceObj_2, camdeviceObj_3);
    ASSERT_NE(input_2, nullptr);
    // CameraManager recovered
    camManagerObj->InitCameraManager();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test abnormal branches with get device from empty id
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_062, TestSize.Level0)
{
    std::string cameraId = "";

    sptr<CameraDevice> camDevice = manager_->GetCameraDeviceFromId(cameraId);
    EXPECT_EQ(camDevice, nullptr);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test IsSessionCommited with CaptureSession null
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_063, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    std::shared_ptr<CameraManagerCallback> camMnagerCallback = std::make_shared<AppCallback>();
    camMnagerCallback = nullptr;
    manager_->SetCallback(camMnagerCallback);

    int32_t intResult = camSession->Release();
    EXPECT_EQ(intResult, 0);

    bool isSessionCommited = camSession->IsSessionCommited();
    EXPECT_EQ(isSessionCommited, false);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataOutput with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_064, TestSize.Level0)
{
    EXPECT_EQ(session_->BeginConfig(), 0);
    EXPECT_EQ(session_->AddInput(input_), 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(previewOutput), 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(metadatOutput), 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;

    EXPECT_EQ(session_->CommitConfig(), 0);
    EXPECT_EQ(session_->Start(), 0);

    sleep(WAIT_TIME_AFTER_START);

    session_->innerInputDevice_ = nullptr;
    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();
    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> { MetadataObjectType::FACE });

    session_->Release();

    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> { MetadataObjectType::FACE });

    metaOutput->Release();
    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> { MetadataObjectType::FACE });
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test PhotoOutput with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_065, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    EXPECT_EQ(session_->BeginConfig(), 0);
    EXPECT_EQ(session_->AddInput(input_), 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(previewOutput), 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(photoOutput), 0);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    EXPECT_EQ(session_->CommitConfig(), 0);
    EXPECT_EQ(session_->Start(), 0);

    sleep(WAIT_TIME_AFTER_START);

    EXPECT_EQ(((sptr<PhotoOutput>&)photoOutput)->Capture(), 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    session_->innerInputDevice_ = nullptr;
    EXPECT_EQ(photoOutput_1->IsQuickThumbnailSupported(), 7400104);
    EXPECT_EQ(photoOutput_1->IsMirrorSupported(), false);
    EXPECT_EQ(photoOutput_1->SetThumbnail(false), 7400104);

    photoOutput_1->Release();
    EXPECT_EQ(photoOutput_1->Capture(photoSetting), 7400104);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataOutput Getstream branch with CaptureOutput nullptr.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_066, TestSize.Level0)
{
    EXPECT_EQ(session_->BeginConfig(), 0);
    EXPECT_EQ(session_->AddInput(input_), 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(previewOutput), 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(metadatOutput), 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;

    EXPECT_EQ(session_->CommitConfig(), 0);
    EXPECT_EQ(session_->Start(), 0);

    sleep(WAIT_TIME_AFTER_START);

    metadatOutput->Release();
    metaOutput->SetSession(session_);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test PreviewOutput/PhotoOutput/VideoOutput Getstream branch with CaptureOutput nullptr.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_067, TestSize.Level0)
{
    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    std::shared_ptr<AppVideoCallback> callback_2 = std::make_shared<AppVideoCallback>();

    EXPECT_EQ(session_->BeginConfig(), 0);
    EXPECT_EQ(session_->AddInput(input_), 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(previewOutput), 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(photoOutput), 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    EXPECT_EQ(session_->AddOutput(videoOutput), 0);

    sptr<VideoOutput> videoOutput_1 = (sptr<VideoOutput>&)videoOutput;
    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;
    sptr<PreviewOutput> previewOutput_1 = (sptr<PreviewOutput>&)previewOutput;

    EXPECT_EQ(session_->CommitConfig(), 0);
    EXPECT_EQ(session_->Start(), 0);

    photoOutput->Release();
    photoOutput_1->SetSession(session_);
    EXPECT_EQ(photoOutput_1->Capture(photoSetting), 7400201);
    EXPECT_EQ(photoOutput_1->Capture(), 7400201);
    EXPECT_EQ(photoOutput_1->CancelCapture(), 7400201);

    previewOutput->Release();
    previewOutput_1->SetSession(session_);
    EXPECT_EQ(previewOutput_1->Start(), 7400201);
    EXPECT_EQ(previewOutput_1->Stop(), 7400201);
    previewOutput_1->SetCallback(callback);

    videoOutput->Release();
    videoOutput_1->SetSession(session_);
    EXPECT_EQ(videoOutput_1->Start(), 7400201);
    EXPECT_EQ(videoOutput_1->Stop(), 7400201);
    EXPECT_EQ(videoOutput_1->Resume(), 7400201);
    EXPECT_EQ(videoOutput_1->Pause(), 7400201);
    videoOutput_1->SetCallback(callback_2);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test IsSessionCommited branch with capturesession object null.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_068, TestSize.Level0)
{
    session_->~CaptureSession();
    EXPECT_EQ(session_->Start(), 7400103);

    VideoStabilizationMode mode;
    EXPECT_EQ(session_->GetActiveVideoStabilizationMode(mode), 7400103);
    EXPECT_EQ(session_->SetVideoStabilizationMode(OFF), 7400103);

    bool isSupported;
    EXPECT_EQ(session_->IsVideoStabilizationModeSupported(OFF, isSupported), 7400103);
    EXPECT_EQ((session_->GetSupportedStabilizationMode()).empty(), true);

    std::vector<VideoStabilizationMode> stabilizationModes = {};
    EXPECT_EQ(session_->GetSupportedStabilizationMode(stabilizationModes), 7400103);
    EXPECT_EQ(session_->IsExposureModeSupported(EXPOSURE_MODE_AUTO, isSupported), 7400103);
    EXPECT_EQ((session_->GetSupportedExposureModes()).empty(), true);

    std::vector<ExposureMode> supportedExposureModes = {};
    EXPECT_EQ(session_->GetSupportedExposureModes(supportedExposureModes), 7400103);
    EXPECT_EQ(session_->SetExposureMode(EXPOSURE_MODE_AUTO), 7400103);
    EXPECT_EQ(session_->GetExposureMode(), EXPOSURE_MODE_UNSUPPORTED);

    ExposureMode exposureMode;
    EXPECT_EQ(session_->GetExposureMode(exposureMode), 7400103);

    Point exposurePointGet = session_->GetMeteringPoint();
    EXPECT_EQ(session_->SetMeteringPoint(exposurePointGet), 7400103);

    EXPECT_EQ((session_->GetMeteringPoint()).x, 0);
    EXPECT_EQ((session_->GetMeteringPoint()).y, 0);

    EXPECT_EQ(session_->GetMeteringPoint(exposurePointGet), 7400103);
    EXPECT_EQ((session_->GetExposureBiasRange()).empty(), true);

    std::vector<float> exposureBiasRange = {};
    EXPECT_EQ(session_->GetExposureBiasRange(exposureBiasRange), 7400103);
    EXPECT_EQ(session_->SetExposureBias(0), 7400103);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test IsSessionCommited branch with capturesession object null.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_069, TestSize.Level0)
{
    float exposureValue;
    EXPECT_EQ(session_->GetExposureValue(), 0);
    EXPECT_EQ(session_->GetExposureValue(exposureValue), 7400103);
    EXPECT_EQ((session_->GetSupportedFocusModes()).empty(), true);

    std::vector<FocusMode> supportedFocusModes;
    EXPECT_EQ(session_->GetSupportedFocusModes(supportedFocusModes), 7400103);

    bool isSupported;
    EXPECT_EQ(session_->IsFocusModeSupported(FOCUS_MODE_AUTO, isSupported), 7400103);
    EXPECT_EQ(session_->SetFocusMode(FOCUS_MODE_AUTO), 7400103);
    EXPECT_EQ(session_->GetFocusMode(), FOCUS_MODE_MANUAL);

    FocusMode focusMode;
    EXPECT_EQ(session_->GetFocusMode(focusMode), 7400103);

    Point exposurePointGet = session_->GetMeteringPoint();
    EXPECT_EQ(session_->SetFocusPoint(exposurePointGet), 7400103);

    EXPECT_EQ((session_->GetFocusPoint()).x, 0);
    EXPECT_EQ((session_->GetFocusPoint()).y, 0);

    Point focusPoint;
    EXPECT_EQ(session_->GetFocusPoint(focusPoint), 7400103);
    EXPECT_EQ(session_->GetFocalLength(), 0);

    float focalLength;
    EXPECT_EQ(session_->GetFocalLength(focalLength), 7400103);
    EXPECT_EQ((session_->GetSupportedFlashModes()).empty(), true);

    std::vector<FlashMode> supportedFlashModes;
    EXPECT_EQ(session_->GetSupportedFlashModes(supportedFlashModes), 7400103);
    EXPECT_EQ(session_->GetFlashMode(), FLASH_MODE_CLOSE);

    FlashMode flashMode;
    EXPECT_EQ(session_->GetFlashMode(flashMode), 7400103);
    EXPECT_EQ(session_->SetFlashMode(FLASH_MODE_CLOSE), 7400103);
    EXPECT_EQ(session_->IsFlashModeSupported(FLASH_MODE_CLOSE, isSupported), 7400103);
    EXPECT_EQ(session_->HasFlash(isSupported), 7400103);
    EXPECT_EQ((session_->GetZoomRatioRange()).empty(), true);

    std::vector<float> exposureBiasRange = {};
    EXPECT_EQ(session_->GetZoomRatioRange(exposureBiasRange), 7400103);
    EXPECT_EQ(session_->GetZoomRatio(), 0);

    float zoomRatio;
    EXPECT_EQ(session_->GetZoomRatio(zoomRatio), 7400103);
    EXPECT_EQ(session_->SetZoomRatio(0), 7400103);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test IsSessionConfiged branch with innerCaptureSession object null.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_070, TestSize.Level0)
{
    session_->innerCaptureSession_ = nullptr;

    EXPECT_EQ(session_->CommitConfig(), 7400102);

    sptr<CaptureInput> input;
    EXPECT_EQ(session_->AddInput(input), 7400102);

    sptr<CaptureOutput> output;
    EXPECT_EQ(session_->RemoveInput(input), 7400102);
    EXPECT_EQ(session_->RemoveOutput(output), 7400102);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeat
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HStreamRepeat with producer is null
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_071, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);

    sptr<IStreamRepeatCallback> repeatCallback = iface_cast<IStreamRepeatCallback>(object);
    ASSERT_NE(repeatCallback, nullptr);

    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    sptr<HStreamRepeat> streamRepeat =
        new (std::nothrow) HStreamRepeat(nullptr, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    EXPECT_EQ(streamRepeat->SetCallback(repeatCallback), CAMERA_OK);
    EXPECT_EQ(streamRepeat->OnFrameError(BUFFER_LOST), CAMERA_OK);
    EXPECT_EQ(streamRepeat->OnFrameError(0), CAMERA_OK);
}

/*
 * Feature: Framework
 * Function: Test HStreamCapture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HStreamCapture with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_072, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);

    sptr<IStreamCaptureCallback> captureCallback = iface_cast<IStreamCaptureCallback>(object);
    ASSERT_NE(captureCallback, nullptr);

    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t captureId = 0;
    int32_t frameCount = 0;
    uint64_t timestamp = 0;
    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    sptr<HStreamCapture> streamCapture = new (std::nothrow) HStreamCapture(producer, format, width, height);
    ASSERT_NE(streamCapture, nullptr);
    EXPECT_EQ(streamCapture->SetCallback(captureCallback), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnCaptureEnded(captureId, frameCount), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnCaptureError(captureId, frameCount), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnCaptureError(captureId, BUFFER_LOST), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnFrameShutter(captureId, timestamp), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnFrameShutterEnd(captureId, timestamp), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnCaptureReady(captureId, timestamp), CAMERA_OK);
}


/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in CaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_078, TestSize.Level0)
{
    EXPECT_EQ((session_->GetSupportedColorEffects()).empty(), true);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(session_->GetFilter(), FilterType::NONE);
    EXPECT_EQ(session_->GetSupportedFilters().empty(), true);
    EXPECT_EQ(session_->GetSupportedBeautyTypes().empty(), true);
    session_->SetBeauty(AUTO_TYPE, 0);
    EXPECT_EQ(session_->GetBeauty(AUTO_TYPE), -1);
    session_->SetFilter(NONE);
    std::vector<ColorSpace> supportedColorSpaces = session_->GetSupportedColorSpaces();
    auto it = std::find(supportedColorSpaces.begin(), supportedColorSpaces.end(), COLOR_SPACE_UNKNOWN);
    bool isSupportSet = (it != supportedColorSpaces.end());
    EXPECT_EQ(session_->SetColorSpace(COLOR_SPACE_UNKNOWN), isSupportSet ? CAMERA_OK : INVALID_ARGUMENT);
    EXPECT_EQ(session_->VerifyAbility(0), CAMERA_INVALID_ARG);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    session_->SetBeauty(AUTO_TYPE, 0);
    EXPECT_EQ(session_->GetBeauty(AUTO_TYPE), -1);
    session_->SetFilter(NONE);
    EXPECT_EQ(session_->SetColorSpace(COLOR_SPACE_UNKNOWN), isSupportSet ? CAMERA_OK : INVALID_ARGUMENT);
    EXPECT_EQ((session_->GetSupportedColorEffects()).empty(), false);
    intResult = session_->Release();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in CaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_079, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    session_->innerInputDevice_ = nullptr;

    EXPECT_EQ(session_->GetSupportedFilters().empty(), true);
    EXPECT_EQ(session_->GetFilter(), FilterType::NONE);
    EXPECT_EQ(session_->GetSupportedBeautyTypes().empty(), true);

    BeautyType beautyType = AUTO_TYPE;
    EXPECT_EQ(session_->GetSupportedBeautyRange(beautyType).empty(), true);
    EXPECT_EQ(session_->GetBeauty(beautyType), -1);
    EXPECT_EQ(session_->GetColorEffect(), ColorEffect::COLOR_EFFECT_NORMAL);
    EXPECT_EQ(session_->GetSupportedColorEffects().empty(), true);

    intResult = session_->Release();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in CaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_080, TestSize.Level0)
{
    EXPECT_EQ(session_->GetSupportedFilters().empty(), true);
    EXPECT_EQ(session_->GetFilter(), FilterType::NONE);
    EXPECT_EQ(session_->GetSupportedBeautyTypes().empty(), true);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(session_->GetSupportedFilters().empty(), true);
    EXPECT_EQ(session_->GetFilter(), FilterType::NONE);
    EXPECT_EQ(session_->GetSupportedBeautyTypes().empty(), true);
    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    session_->innerInputDevice_ = nullptr;
    EXPECT_EQ(session_->GetSupportedFilters().empty(), true);
    EXPECT_EQ(session_->GetFilter(), FilterType::NONE);
    EXPECT_EQ(session_->GetSupportedBeautyTypes().empty(), true);
    EXPECT_EQ(session_->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test Commited() || Configed() with abnormal branches in CaptureSession in CaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_081, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);
    ColorSpace colorSpace = COLOR_SPACE_UNKNOWN;
    camSession->~CaptureSession();
    EXPECT_EQ(camSession->GetSupportedFilters().empty(), true);
    EXPECT_EQ(camSession->GetFilter(), FilterType::NONE);
    EXPECT_EQ(camSession->GetSupportedBeautyTypes().empty(), true);
    BeautyType beautyType = AUTO_TYPE;
    EXPECT_EQ(camSession->GetSupportedBeautyRange(beautyType).empty(), true);
    EXPECT_EQ(camSession->GetBeauty(AUTO_TYPE), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(camSession->GetColorEffect(), ColorEffect::COLOR_EFFECT_NORMAL);
    EXPECT_EQ(camSession->GetSupportedColorEffects().empty(), true);
    camSession->SetFilter(NONE);
    camSession->SetBeauty(beautyType, 0);
    EXPECT_EQ(camSession->GetSupportedColorSpaces().empty(), true);
    EXPECT_EQ(camSession->SetColorSpace(COLOR_SPACE_UNKNOWN), CameraErrorCode::SESSION_NOT_CONFIG);
    camSession->SetMode(SceneMode::NORMAL);
    EXPECT_EQ(camSession->VerifyAbility(0), CAMERA_INVALID_ARG);
    EXPECT_EQ(camSession->GetActiveColorSpace(colorSpace), CameraErrorCode::SESSION_NOT_CONFIG);
    camSession->SetColorEffect(COLOR_EFFECT_NORMAL);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in CaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_082, TestSize.Level0)
{
    ColorSpace colorSpace = COLOR_SPACE_UNKNOWN;
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);
    session_->SetMode(SceneMode::PORTRAIT);
    EXPECT_EQ(session_->VerifyAbility(0), CAMERA_INVALID_ARG);
    EXPECT_EQ(session_->GetActiveColorSpace(colorSpace), CAMERA_OK);
    session_->SetColorEffect(COLOR_EFFECT_NORMAL);
    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    if (session_->IsMacroSupported()) {
        session_->LockForControl();
        intResult = session_->EnableMacro(false);
        session_->UnlockForControl();
        EXPECT_EQ(intResult, 0);
    }
    EXPECT_EQ(session_->GetActiveColorSpace(colorSpace), CAMERA_OK);
    session_->SetColorEffect(COLOR_EFFECT_NORMAL);
    session_->innerInputDevice_ = nullptr;
    EXPECT_EQ(session_->VerifyAbility(0), CAMERA_INVALID_ARG);
    EXPECT_EQ(session_->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in NightSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_083, TestSize.Level0)
{
    SceneMode nightMode = SceneMode::NIGHT;
    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(nightMode);
    auto nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);
    int32_t intResult = nightSession->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = nightSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    std::vector<uint32_t> exposureRange;
    uint32_t exposureValue = 0;
    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), SESSION_NOT_CONFIG);
    EXPECT_EQ(nightSession->SetExposure(0), SESSION_NOT_CONFIG);
    EXPECT_EQ(nightSession->GetExposure(exposureValue), SESSION_NOT_CONFIG);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in NightSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_084, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);
    SceneMode nightMode = SceneMode::NIGHT;
    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(nightMode);
    auto nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    nightSession->~NightSession();
    uint32_t exposureValue = 0;
    std::vector<uint32_t> exposureRange;
    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), SESSION_NOT_CONFIG);
    EXPECT_EQ(nightSession->GetExposure(exposureValue), SESSION_NOT_CONFIG);
    EXPECT_EQ(nightSession->SetExposure(0), SESSION_NOT_CONFIG);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in PortraitSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_087, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(portraitMode);
    auto portraitSession = static_cast<PortraitSession*>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);
    EXPECT_EQ(portraitSession->GetSupportedPortraitEffects().empty(), true);
    EXPECT_EQ(portraitSession->GetPortraitEffect(), OFF_EFFECT);
    portraitSession->SetPortraitEffect(OFF_EFFECT);
    int32_t intResult = portraitSession->BeginConfig();
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(portraitSession->GetSupportedPortraitEffects().empty(), true);
    EXPECT_EQ(portraitSession->GetPortraitEffect(), OFF_EFFECT);
    portraitSession->SetPortraitEffect(OFF_EFFECT);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with PortraitSession is nullptr
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_088, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(portraitMode);
    auto portraitSession = static_cast<PortraitSession*>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);
    portraitSession->~PortraitSession();
    EXPECT_EQ(portraitSession->GetSupportedPortraitEffects().empty(), true);
    EXPECT_EQ(portraitSession->GetPortraitEffect(), OFF_EFFECT);
    portraitSession->SetPortraitEffect(OFF_EFFECT);
}

/*
 * Feature: Framework
 * Function: Test OnSketchStatusChanged
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test PreviewOutputCallbackImpl:OnSketchStatusChanged with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_089, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);
    sptr<PreviewOutput> previewOutput_1 = (sptr<PreviewOutput>&)previewOutput;
    ASSERT_NE(previewOutput_1, nullptr);
    auto previewOutputCallbackImpl = new (std::nothrow) PreviewOutputCallbackImpl(previewOutput_1.GetRefPtr());
    ASSERT_NE(previewOutputCallbackImpl, nullptr);
    EXPECT_EQ(previewOutputCallbackImpl->OnSketchStatusChanged(SketchStatus::STOPED), CAMERA_OK);
    auto previewOutputCallbackImpl1 = new (std::nothrow) PreviewOutputCallbackImpl();
    ASSERT_NE(previewOutputCallbackImpl1, nullptr);
    EXPECT_EQ(previewOutputCallbackImpl1->OnSketchStatusChanged(SketchStatus::STOPED), CAMERA_OK);
    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    session_->innerInputDevice_ = nullptr;
    EXPECT_EQ(session_->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test !IsSessionCommited() && !IsSessionConfiged() with abnormal branches in CaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_090, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    BeautyType beautyType = AUTO_TYPE;
    EXPECT_EQ(session_->GetSupportedBeautyRange(beautyType).empty(), true);
    EXPECT_EQ(session_->GetBeauty(beautyType), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(session_->GetColorEffect(), ColorEffect::COLOR_EFFECT_NORMAL);
    EXPECT_EQ(session_->GetSupportedColorEffects().empty(), true);
    EXPECT_EQ(session_->GetSupportedColorSpaces().empty(), true);
    session_->SetBeauty(beautyType, 0);
    session_->SetFilter(NONE);
    EXPECT_EQ(session_->SetColorSpace(COLOR_SPACE_UNKNOWN), SESSION_NOT_CONFIG);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(session_->GetSupportedBeautyRange(beautyType).empty(), true);
    EXPECT_EQ(session_->GetBeauty(beautyType), -1);
    EXPECT_EQ(session_->GetColorEffect(), COLOR_EFFECT_NORMAL);
    session_->SetBeauty(beautyType, 0);
    session_->SetFilter(NONE);
    EXPECT_EQ(session_->SetColorSpace(COLOR_SPACE_UNKNOWN), 7400101);
    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    session_->innerInputDevice_ = nullptr;
    EXPECT_EQ(session_->GetSupportedBeautyRange(beautyType).empty(), true);
    EXPECT_EQ(session_->GetBeauty(beautyType), -1);
    EXPECT_EQ(session_->GetColorEffect(), COLOR_EFFECT_NORMAL);
    EXPECT_EQ(session_->GetSupportedColorEffects().empty(), true);
    EXPECT_EQ(session_->GetSupportedColorSpaces().empty(), true);
    session_->SetBeauty(beautyType, 0);
    session_->SetFilter(NONE);
    EXPECT_EQ(session_->SetColorSpace(COLOR_SPACE_UNKNOWN), CAMERA_OK);
    EXPECT_EQ(session_->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_091, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    HCameraServiceProxy *hCameraServiceProxy = new (std::nothrow) HCameraServiceProxy(object);
    ASSERT_NE(hCameraServiceProxy, nullptr);
    EffectParam effectParam = {0, 0, 0};

    sptr<ICameraDeviceService> device = nullptr;
    sptr<ITorchServiceCallback> torchSvcCallback = nullptr;
    bool canOpenCamera = true;
    EXPECT_EQ(hCameraServiceProxy->CreateCameraDevice(cameras_[0]->GetID(), device), -1);
    hCameraServiceProxy->SetTorchCallback(torchSvcCallback);
    torchSvcCallback = new(std::nothrow) TorchServiceCallback(nullptr);
    ASSERT_NE(torchSvcCallback, nullptr);
    hCameraServiceProxy->SetTorchCallback(torchSvcCallback);
    hCameraServiceProxy->MuteCamera(true);
    hCameraServiceProxy->MuteCamera(false);
    hCameraServiceProxy->PrelaunchCamera();
    hCameraServiceProxy->SetPrelaunchConfig(cameras_[0]->GetID(), NO_NEED_RESTORE_PARAM_OHOS, 0, effectParam);
    hCameraServiceProxy->SetTorchLevel(0);
    hCameraServiceProxy->AllowOpenByOHSide(cameras_[0]->GetID(), 0, canOpenCamera);
    hCameraServiceProxy->NotifyCameraState(cameras_[0]->GetID(), 0);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_092, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    HCameraServiceCallbackProxy *hCameraServiceCallbackProxy = new (std::nothrow) HCameraServiceCallbackProxy(object);
    ASSERT_NE(hCameraServiceCallbackProxy, nullptr);
    auto hCameraMuteServiceCallbackProxy = new (std::nothrow) HCameraMuteServiceCallbackProxy(object);
    ASSERT_NE(hCameraMuteServiceCallbackProxy, nullptr);
    HTorchServiceCallbackProxy *hTorchServiceCallbackProxy = new (std::nothrow) HTorchServiceCallbackProxy(object);
    ASSERT_NE(hTorchServiceCallbackProxy, nullptr);

    hCameraServiceCallbackProxy->OnFlashlightStatusChanged(cameras_[0]->GetID(), FLASH_STATUS_OFF);
    hCameraMuteServiceCallbackProxy->OnCameraMute(true);
    hCameraMuteServiceCallbackProxy->OnCameraMute(false);
    hTorchServiceCallbackProxy->OnTorchStatusChange(TORCH_STATUS_OFF);
}

/*
 * Feature: Framework
 * Function: Test errorCode, width and height, if sketchRatio > SKETCH_RATIO_MAX_VALUE
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode, width and height, if sketchRatio > SKETCH_RATIO_MAX_VALUE with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_093, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    sptr<IStreamRepeat> repeat = iface_cast<IStreamRepeat>(object);
    ASSERT_NE(repeat, nullptr);
    HStreamRepeatProxy *hStreamRepeatProxy = new (std::nothrow) HStreamRepeatProxy(object);
    ASSERT_NE(hStreamRepeatProxy, nullptr);

    hStreamRepeatProxy->ForkSketchStreamRepeat(0, 0, repeat, 0);
    hStreamRepeatProxy->ForkSketchStreamRepeat(1, 0, repeat, 0);
    hStreamRepeatProxy->ForkSketchStreamRepeat(1, 1, repeat, 0);
    hStreamRepeatProxy->UpdateSketchRatio(0.0f);
    hStreamRepeatProxy->UpdateSketchRatio(200.0f);
    hStreamRepeatProxy->UpdateSketchRatio(90.0f);
    hStreamRepeatProxy->RemoveSketchStreamRepeat();
}

/*
 * Feature: Framework
 * Function: Test errorCode, width and height, if sketchRatio > SKETCH_RATIO_MAX_VALUE
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode, width and height, if sketchRatio > SKETCH_RATIO_MAX_VALUE with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_094, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    sptr<IStreamRepeat> repeat = iface_cast<IStreamRepeat>(object);
    ASSERT_NE(repeat, nullptr);
    HStreamRepeatProxy *hStreamRepeatProxy = new (std::nothrow) HStreamRepeatProxy(object);
    ASSERT_NE(hStreamRepeatProxy, nullptr);

    hStreamRepeatProxy->UpdateSketchRatio(0.0f);
    hStreamRepeatProxy->UpdateSketchRatio(200.0f);
    hStreamRepeatProxy->UpdateSketchRatio(90.0f);
    hStreamRepeatProxy->RemoveSketchStreamRepeat();
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_095, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    HStreamCaptureCallbackProxy *hStreamCaptureCallbackProxy = new (std::nothrow) HStreamCaptureCallbackProxy(object);
    ASSERT_NE(hStreamCaptureCallbackProxy, nullptr);

    hStreamCaptureCallbackProxy->OnCaptureStarted(0);
    hStreamCaptureCallbackProxy->OnCaptureStarted(0, 0);
    hStreamCaptureCallbackProxy->OnCaptureEnded(0, 0);
    hStreamCaptureCallbackProxy->OnCaptureError(0, 0);
    hStreamCaptureCallbackProxy->OnFrameShutter(0, 0);
    hStreamCaptureCallbackProxy->OnFrameShutterEnd(0, 0);
    hStreamCaptureCallbackProxy->OnCaptureReady(0, 0);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_096, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    HStreamRepeatCallbackProxy *hStreamRepeatCallbackProxy = new (std::nothrow) HStreamRepeatCallbackProxy(object);
    ASSERT_NE(hStreamRepeatCallbackProxy, nullptr);

    hStreamRepeatCallbackProxy->OnSketchStatusChanged(SketchStatus::STOPED);
    hStreamRepeatCallbackProxy->OnFrameStarted();
    hStreamRepeatCallbackProxy->OnFrameEnded(0);
    hStreamRepeatCallbackProxy->OnFrameError(0);
}


/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_097, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    auto hCaptureSessionCallbackProxy = new (std::nothrow) HCaptureSessionCallbackProxy(object);
    ASSERT_NE(hCaptureSessionCallbackProxy, nullptr);
    hCaptureSessionCallbackProxy->OnError(0);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_098, TestSize.Level0)
{
    auto photoOutput1 = CreatePhotoOutput();
    auto photoOutput = static_cast<PhotoOutput*>(photoOutput1.GetRefPtr());
    photoOutput->ConfirmCapture();
    photoOutput->SetSession(nullptr);
    photoOutput->ConfirmCapture();
    photoOutput->SetSession(session_);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);
    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    photoOutput->ConfirmCapture();
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_099, TestSize.Level0)
{
    auto photoOutput1 = CreatePhotoOutput();
    auto photoOutput = static_cast<PhotoOutput*>(photoOutput1.GetRefPtr());
    photoOutput->Release();
    photoOutput->ConfirmCapture();
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_100, TestSize.Level0)
{
    std::shared_ptr<HStreamCaptureCallbackImpl> captureCallback = std::make_shared<HStreamCaptureCallbackImpl>(nullptr);
    int32_t captureId = 2001;
    int32_t intResult = captureCallback->OnCaptureStarted(captureId, 0);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetMetaSetting with existing metaTag.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_101, TestSize.Level0)
{
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;

    std::shared_ptr<camera_metadata_item_t> metadataItem = nullptr;
    metadataItem = camInput->GetMetaSetting(OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS);
    ASSERT_NE(metadataItem, nullptr);

    std::vector<vendorTag_t> infos = {};
    camInput->GetCameraAllVendorTags(infos);

    sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    sptr<CameraDevice> camdeviceObj = nullptr;
    sptr<CameraInput> camInput_1 = new (std::nothrow) CameraInput(deviceObj, camdeviceObj);
    ASSERT_NE(camInput_1, nullptr);

    metadataItem = camInput_1->GetMetaSetting(OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS);
    EXPECT_EQ(metadataItem, nullptr);

    metadataItem = camInput->GetMetaSetting(-1);
    EXPECT_EQ(metadataItem, nullptr);

    if (!IsSupportNow()) {
        return;
    }

    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras_[1]->GetMetadata();

    std::string cameraId = cameras_[1]->GetID();
    sptr<CameraDevice> camdeviceObj_2 = new (std::nothrow) CameraDevice(cameraId, metadata);
    ASSERT_NE(camdeviceObj_2, nullptr);

    EXPECT_EQ(camdeviceObj_2->GetPosition(), CAMERA_POSITION_FRONT);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Torch with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_102, TestSize.Level0)
{
    EXPECT_EQ(input_->Close(), 0);
    sleep(WAIT_TIME_AFTER_CLOSE);

    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();

    if (!(manager_->IsTorchSupported())) {
        return;
    }

    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    camManagerObj->RegisterTorchListener(callback);
    EXPECT_EQ(camManagerObj->IsTorchModeSupported(TORCH_MODE_AUTO), false);

    int32_t intResult = camManagerObj->SetTorchMode(TORCH_MODE_AUTO);
    EXPECT_EQ(intResult, 7400102);

    EXPECT_EQ(camManagerObj->IsTorchModeSupported(TORCH_MODE_ON), true);
    intResult = camManagerObj->SetTorchMode(TORCH_MODE_ON);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_CALLBACK);

    EXPECT_EQ(camManagerObj->IsTorchModeSupported(TORCH_MODE_OFF), true);

    intResult = camManagerObj->SetTorchMode(TORCH_MODE_OFF);
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_CALLBACK);

    // CameraManager instance has been changed, need recover
    camManagerObj->SetServiceProxy(nullptr);
    intResult = camManagerObj->SetTorchMode(TORCH_MODE_OFF);
    EXPECT_EQ(intResult, 7400201);
    // CameraManager recovered
    camManagerObj->InitCameraManager();
}

/*
 * Feature: Framework
 * Function: Test Metadata
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Metadata
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_103, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;
    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();
    if (metadataObjectTypes.size() == 0) {
        return;
    }

    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> { MetadataObjectType::FACE });

    std::shared_ptr<MetadataObjectCallback> metadataObjectCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataObjectCallback);
    std::shared_ptr<MetadataStateCallback> metadataStateCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataStateCallback);

    pid_t pid = 0;
    metaOutput->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test Metadata
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Metadata
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_104, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;
    std::vector<MetadataObjectType> metadataObjectTypes = metaOutput->GetSupportedMetadataObjectTypes();
    if (metadataObjectTypes.size() == 0) {
        return;
    }

    metaOutput->SetCapturingMetadataObjectTypes(std::vector<MetadataObjectType> {});
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_105, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    HCameraServiceProxy *hCameraServiceProxy = new (std::nothrow) HCameraServiceProxy(object);
    ASSERT_NE(hCameraServiceProxy, nullptr);
    EffectParam effectParam = {0, 0, 0};

    sptr<ICameraDeviceService> device = nullptr;
    sptr<ITorchServiceCallback> torchSvcCallback = nullptr;
    hCameraServiceProxy->CreateCameraDevice(cameras_[0]->GetID(), device);
    hCameraServiceProxy->SetTorchCallback(torchSvcCallback);
    torchSvcCallback = new(std::nothrow) TorchServiceCallback(nullptr);
    ASSERT_NE(torchSvcCallback, nullptr);
    hCameraServiceProxy->SetTorchCallback(torchSvcCallback);
    hCameraServiceProxy->MuteCamera(true);
    hCameraServiceProxy->MuteCamera(false);
    hCameraServiceProxy->PrelaunchCamera();
    hCameraServiceProxy->SetPrelaunchConfig(cameras_[0]->GetID(), NO_NEED_RESTORE_PARAM_OHOS, 0, effectParam);
    hCameraServiceProxy->SetTorchLevel(0);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_106, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    HCameraServiceCallbackProxy *hCameraServiceCallbackProxy = new (std::nothrow) HCameraServiceCallbackProxy(object);
    ASSERT_NE(hCameraServiceCallbackProxy, nullptr);
    HCameraMuteServiceCallbackProxy *hCameraMuteServiceCallbackProxy =
        new (std::nothrow) HCameraMuteServiceCallbackProxy(object);
    ASSERT_NE(hCameraMuteServiceCallbackProxy, nullptr);
    HTorchServiceCallbackProxy *hTorchServiceCallbackProxy = new (std::nothrow) HTorchServiceCallbackProxy(object);
    ASSERT_NE(hTorchServiceCallbackProxy, nullptr);

    hCameraServiceCallbackProxy->OnFlashlightStatusChanged(cameras_[0]->GetID(), FLASH_STATUS_OFF);
    hCameraMuteServiceCallbackProxy->OnCameraMute(true);
    hCameraMuteServiceCallbackProxy->OnCameraMute(false);
    hTorchServiceCallbackProxy->OnTorchStatusChange(TORCH_STATUS_OFF);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_107, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    HStreamCaptureCallbackProxy *hStreamCaptureCallbackProxy = new (std::nothrow) HStreamCaptureCallbackProxy(object);
    ASSERT_NE(hStreamCaptureCallbackProxy, nullptr);

    hStreamCaptureCallbackProxy->OnCaptureStarted(0);
    hStreamCaptureCallbackProxy->OnCaptureStarted(0, 0);
    hStreamCaptureCallbackProxy->OnCaptureEnded(0, 0);
    hStreamCaptureCallbackProxy->OnCaptureError(0, 0);
    hStreamCaptureCallbackProxy->OnFrameShutter(0, 0);
    hStreamCaptureCallbackProxy->OnFrameShutterEnd(0, 0);
    hStreamCaptureCallbackProxy->OnCaptureReady(0, 0);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_108, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    HStreamRepeatCallbackProxy *hStreamRepeatCallbackProxy = new (std::nothrow) HStreamRepeatCallbackProxy(object);
    ASSERT_NE(hStreamRepeatCallbackProxy, nullptr);

    hStreamRepeatCallbackProxy->OnSketchStatusChanged(SketchStatus::STOPED);
    hStreamRepeatCallbackProxy->OnFrameStarted();
    hStreamRepeatCallbackProxy->OnFrameEnded(0);
    hStreamRepeatCallbackProxy->OnFrameError(0);
}

/*
 * Feature: Framework
 * Function: Test errorCode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test errorCode with abnormal branches
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_109, TestSize.Level0)
{
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    ASSERT_NE(samgr, nullptr);
    object = samgr->GetSystemAbility(AUDIO_POLICY_SERVICE_ID);
    HCaptureSessionCallbackProxy *hCaptureSessionCallbackProxy =
        new (std::nothrow) HCaptureSessionCallbackProxy(object);
    ASSERT_NE(hCaptureSessionCallbackProxy, nullptr);
    hCaptureSessionCallbackProxy->OnError(0);
}

/* Feature: Framework
 * Function: Test CreateCaptureSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CreateCaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_110, TestSize.Level0)
{
    SceneMode mode = SceneMode::CAPTURE;
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
}

/* Feature: Framework
 * Function: Test CreateCaptureSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CreateCaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_111, TestSize.Level0)
{
    SceneMode mode = SceneMode::SCAN;
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
}

/* Feature: Framework
 * Function: Test CreateCaptureSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CreateCaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_112, TestSize.Level0)
{
    SceneMode mode = SceneMode::CAPTURE_MACRO;
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
}

/* Feature: Framework
 * Function: Test CreateCaptureSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CreateCaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_113, TestSize.Level0)
{
    SceneMode mode = SceneMode::CAPTURE_MACRO;
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
}

/* Feature: Framework
 * Function: Test CreateCaptureSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CreateCaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_114, TestSize.Level0)
{
    SceneMode mode = SceneMode::VIDEO;
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
}

/*
* Feature: Framework
* Function: Test SetExposureBias && GetFeaturesMode && SetBeautyValue && GetSubFeatureMods
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: test SetExposureBias && GetFeaturesMode && SetBeautyValue && GetSubFeatureMods
*/
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_115, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    int32_t intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    camSession->LockForControl();
    EXPECT_EQ(camSession->SetExposureBias(100.0f), CameraErrorCode::SUCCESS);
    intResult = camSession->UnlockForControl();

    camSession->isSetMacroEnable_ = true;
    camSession->currentMode_ = SceneMode::VIDEO;

    SceneFeaturesMode videoMacroMode(SceneMode::VIDEO, { SceneFeature::FEATURE_MACRO });
    bool isMatchSubFeatureMode = false;
    auto vec = camSession->GetSubFeatureMods();
    for (auto& sceneFeaturesMode : vec) {
        if (sceneFeaturesMode == videoMacroMode) {
            isMatchSubFeatureMode = true;
            break;
        }
    }
    EXPECT_TRUE(isMatchSubFeatureMode);

    auto mode = camSession->GetFeaturesMode();
    EXPECT_TRUE(mode == videoMacroMode);

    camSession->currentMode_ = SceneMode::CAPTURE;
    mode = camSession->GetFeaturesMode();
    SceneFeaturesMode captureMacroMode(SceneMode::CAPTURE, { SceneFeature::FEATURE_MACRO });
    EXPECT_TRUE(mode == captureMacroMode);

    isMatchSubFeatureMode = false;
    vec = camSession->GetSubFeatureMods();
    for (auto& sceneFeaturesMode : vec) {
        if (sceneFeaturesMode == captureMacroMode) {
            isMatchSubFeatureMode = true;
            break;
        }
    }
    EXPECT_TRUE(isMatchSubFeatureMode);

    bool boolResult = camSession->SetBeautyValue(BeautyType::SKIN_TONE, 0);
    EXPECT_FALSE(boolResult);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test captureCallback with anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_116, TestSize.Level0)
{
    std::shared_ptr<HStreamCaptureCallbackImpl> captureCallback = std::make_shared<HStreamCaptureCallbackImpl>(nullptr);

    int32_t captureId = 2001;
    uint64_t timestamp = 10;

    int32_t intResult = captureCallback->OnFrameShutterEnd(captureId, timestamp);
    EXPECT_EQ(intResult, 0);

    intResult = captureCallback->OnCaptureReady(captureId, timestamp);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    photoOutput_1->SetCallback(callback);

    sptr<HStreamCaptureCallbackImpl> captureCallback_2 = new (std::nothrow) HStreamCaptureCallbackImpl(photoOutput_1);
    ASSERT_NE(captureCallback_2, nullptr);

    intResult = captureCallback_2->OnFrameShutterEnd(captureId, timestamp);
    EXPECT_EQ(intResult, 0);

    intResult = captureCallback_2->OnCaptureReady(captureId, timestamp);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test prelaunch the camera with PERSISTENT_DEFAULT_PARAM_OHOS anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_117, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    std::string cameraId = camInput->GetCameraId();
    int activeTime = 0;
    EffectParam effectParam = {0, 0, 0};

    int32_t intResult = camManagerObj->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::PERSISTENT_DEFAULT_PARAM_OHOS,
        activeTime, effectParam);
    if (!IsSupportNow()) {
        EXPECT_EQ(intResult, 7400201);
    } else {
        EXPECT_EQ(intResult, 0);
    }

    intResult = camManagerObj->PrelaunchCamera();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test prelaunch the camera with NO_NEED_RESTORE_PARAM_OHOS anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_118, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    std::string cameraId = camInput->GetCameraId();
    int activeTime = 15;
    EffectParam effectParam = {5, 0, 0};

    int32_t intResult = camManagerObj->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::NO_NEED_RESTORE_PARAM_OHOS,
        activeTime, effectParam);
    if (!IsSupportNow()) {
        EXPECT_EQ(intResult, 7400201);
    } else {
        EXPECT_EQ(intResult, 0);
    }
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test prelaunch the camera with abnormal cameraid branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_119, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    std::string cameraId = camInput->GetCameraId();
    int activeTime = 15;
    EffectParam effectParam = {0, 0, 0};

    cameraId = "";
    int32_t intResult = camManagerObj->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::TRANSIENT_ACTIVE_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test prelaunch the camera with abnormal setting branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_moduletest_120, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    ASSERT_NE(camManagerObj, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    std::string cameraId = camInput->GetCameraId();
    int activeTime = 15;
    EffectParam effectParam = {-1, -1, -1};
    int32_t intResult = camManagerObj->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::NO_NEED_RESTORE_PARAM_OHOS,
        activeTime, effectParam);
    if (!IsSupportNow()) {
        EXPECT_EQ(intResult, 7400201);
    } else {
        EXPECT_EQ(intResult, 0);
    }
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test low light boost
 */
HWTEST_F(CameraFrameworkModuleTest, camera_fwcoverage_scenefeature_test_001, TestSize.Level0)
{

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    bool isLowLightBoostSupported = session_->IsFeatureSupported(FEATURE_LOW_LIGHT_BOOST);

    if (isLowLightBoostSupported) {
        intResult = session_->EnableFeature(FEATURE_LOW_LIGHT_BOOST, true);
        EXPECT_EQ(intResult, 7400103);
    }

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    isLowLightBoostSupported = session_->IsFeatureSupported(FEATURE_LOW_LIGHT_BOOST);
    if (isLowLightBoostSupported) {
        session_->SetFeatureDetectionStatusCallback(std::make_shared<AppCallback>());
        session_->LockForControl();
        intResult = session_->EnableLowLightDetection(true);
        session_->UnlockForControl();
        EXPECT_EQ(intResult, 0);
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test camera preempted.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test camera preempted.
 * @tc.require: SR000GVTU0
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_044, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    sptr<CameraInput> camInput_2 = (sptr<CameraInput>&)input_;
    camInput_2->Open();

    camInput_2->SetErrorCallback(callback);

    sptr<CaptureSession> session_2 = manager_->CreateCaptureSession();
    ASSERT_NE(session_2, nullptr);

    int32_t intResult = session_2->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_2->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput_2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput_2, nullptr);

    intResult = session_2->AddOutput(previewOutput_2);
    EXPECT_EQ(intResult, 0);

    intResult = session_2->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_2->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    camInput_2->Close();

    if (cameras_.size() < 2) {
        return;
    }

    sptr<CaptureInput> input_3 = manager_->CreateCameraInput(cameras_[1]);
    ASSERT_NE(input_3, nullptr);

    sptr<CameraInput> camInput_3 = (sptr<CameraInput>&)input_3;
    camInput_3->Open();

    sptr<CaptureSession> session_3 = manager_->CreateCaptureSession();
    ASSERT_NE(session_3, nullptr);

    intResult = session_3->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_3->AddInput(input_3);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput_3 = CreatePreviewOutput();
    ASSERT_NE(previewOutput_3, nullptr);

    intResult = session_3->AddOutput(previewOutput_3);
    EXPECT_EQ(intResult, 0);

    intResult = session_3->CommitConfig();
    EXPECT_EQ(intResult, 0);

    camInput_3->Close();
    session_3->Stop();
}

/*
 * Feature: Framework
 * Function: Test capture session with Video Stabilization Mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with Video Stabilization Mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_049, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::vector<VideoStabilizationMode> stabilizationmodes = session_->GetSupportedStabilizationMode();
    if (stabilizationmodes.empty()) {
        return;
    }
    ASSERT_EQ(stabilizationmodes.empty(), false);

    VideoStabilizationMode stabilizationMode = stabilizationmodes.back();
    if (session_->IsVideoStabilizationModeSupported(stabilizationMode)) {
        session_->SetVideoStabilizationMode(stabilizationMode);
        EXPECT_EQ(session_->GetActiveVideoStabilizationMode(), stabilizationMode);
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<VideoOutput>&)videoOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
    session_->Stop();
}

/*
 * Feature: Framework
 * Function: Test sketch functions.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketch functions.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_050, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    bool isSketchSupport = previewOutput->IsSketchSupported();
    if (!isSketchSupport) {
        return;
    }

    int sketchEnableRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchEnableRatio, 0);

    intResult = previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test sketch functions callback.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketch functions.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_051, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    bool isSketchSupport = previewOutput->IsSketchSupported();
    if (!isSketchSupport) {
        return;
    }

    previewOutput->SetCallback(std::make_shared<AppCallback>());

    int sketchEnableRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchEnableRatio, 0);

    intResult = previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 0);

    session_->LockForControl();
    intResult = session_->SetZoomRatio(30.0f);
    EXPECT_EQ(intResult, 0);
    session_->UnlockForControl();

    sleep(WAIT_TIME_AFTER_START);

    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 1);
    g_previewEvents.reset();

    sleep(WAIT_TIME_AFTER_START);
    auto statusSize = g_sketchStatus.size();
    EXPECT_GT(statusSize, 0);
    if (statusSize == 2) {
        EXPECT_EQ(g_sketchStatus.front(), 3);
        g_sketchStatus.pop_front();
        EXPECT_EQ(g_sketchStatus.front(), 1);
        g_sketchStatus.pop_front();
    }

    sleep(1);
    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 0);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test sketch anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketch anomalous branch.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_052, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    ASSERT_NE(output, nullptr);

    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    bool isSketchSupport = previewOutput->IsSketchSupported();
    ASSERT_FALSE(isSketchSupport);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    isSketchSupport = previewOutput->IsSketchSupported();
    if (!isSketchSupport) {
        return;
    }

    session_->SetMode(SceneMode::PORTRAIT);
    int sketchEnableRatio = previewOutput->GetSketchRatio();
    EXPECT_LT(sketchEnableRatio, 0);

    session_->SetMode(SceneMode::CAPTURE);
    sketchEnableRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchEnableRatio, 0);

    auto sketchSurface = CreateSketchSurface(previewProfile->GetCameraFormat());

    intResult = previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->AttachSketchSurface(sketchSurface);
    EXPECT_EQ(intResult, 7400103);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->AttachSketchSurface(sketchSurface);
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 7400103);

    sleep(WAIT_TIME_AFTER_START);

    intResult = previewOutput->AttachSketchSurface(sketchSurface);
    EXPECT_EQ(intResult, 0);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test sketch functions auto start sketch while zoom set.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketch functions auto start sketch while zoom set.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_053, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    previewOutput->SetCallback(std::make_shared<AppCallback>());
    bool isSketchSupport = previewOutput->IsSketchSupported();
    if (!isSketchSupport) {
        return;
    }

    int sketchEnableRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchEnableRatio, 0);

    intResult = previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    session_->LockForControl();
    intResult = session_->SetZoomRatio(30.0f);
    EXPECT_EQ(intResult, 0);
    session_->UnlockForControl();

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 1);
    g_previewEvents.reset();

    auto statusSize = g_sketchStatus.size();
    EXPECT_EQ(statusSize, 3);
    if (statusSize == 3) {
        EXPECT_EQ(g_sketchStatus.front(), 0);
        g_sketchStatus.pop_front();
        EXPECT_EQ(g_sketchStatus.front(), 3);
        g_sketchStatus.pop_front();
        EXPECT_EQ(g_sketchStatus.front(), 1);
        g_sketchStatus.pop_front();
    }

    sleep(WAIT_TIME_AFTER_START);
    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 0);
    g_previewEvents.reset();

    session_->LockForControl();
    intResult = session_->SetZoomRatio(1.0f);
    EXPECT_EQ(intResult, 0);
    session_->UnlockForControl();

    sleep(WAIT_TIME_AFTER_START);
    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 1);
    g_previewEvents.reset();

    statusSize = g_sketchStatus.size();
    EXPECT_EQ(statusSize, 2);
    if (statusSize == 2) {
        EXPECT_EQ(g_sketchStatus.front(), 2);
        g_sketchStatus.pop_front();
        EXPECT_EQ(g_sketchStatus.front(), 0);
        g_sketchStatus.pop_front();
    }

    sleep(WAIT_TIME_AFTER_START);
    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 0);
    g_previewEvents.reset();

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test macro function.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test macro function.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_054, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    session_->SetMacroStatusCallback(std::make_shared<AppCallback>());

    g_macroEvents.reset();
    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    bool isMacroSupported = session_->IsMacroSupported();
    if (isMacroSupported) {
        EXPECT_EQ(g_macroEvents.count(), 1);
        if (g_macroEvents[static_cast<int>(CAM_MACRO_DETECT_EVENTS::CAM_MACRO_EVENT_ACTIVE)] == 1) {
            session_->LockForControl();
            intResult = session_->EnableMacro(true);
            EXPECT_EQ(intResult, 0);
            session_->UnlockForControl();
        }
        if (g_macroEvents[static_cast<int>(CAM_MACRO_DETECT_EVENTS::CAM_MACRO_EVENT_IDLE)] == 1) {
            session_->LockForControl();
            intResult = session_->EnableMacro(false);
            EXPECT_EQ(intResult, 0);
            session_->UnlockForControl();
        }
    }

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test macro function anomalous branch..
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test macro function anomalous branch..
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_055, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    bool isMacroSupported = session_->IsMacroSupported();
    EXPECT_FALSE(isMacroSupported);

    intResult = session_->EnableMacro(true);
    EXPECT_EQ(intResult, 7400102);

    intResult = session_->EnableMacro(false);
    EXPECT_EQ(intResult, 7400102);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    isMacroSupported = session_->IsMacroSupported();
    if (!isMacroSupported) {
        intResult = session_->EnableMacro(false);
        EXPECT_EQ(intResult, 7400102);
    }
    session_->SetMacroStatusCallback(std::make_shared<AppCallback>());

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    g_macroEvents.reset();

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Device preemption
 * Function: Test device open twice
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test device open twice
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_056, TestSize.Level0)
{
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->Open();

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/* Feature: Framework
 * Function: Test preview/capture with portrait session's beauty
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with portrait session's beauty
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_057, TestSize.Level0)
{
    SceneMode portraitMode = SceneMode::PORTRAIT;
    if (!IsSupportMode(portraitMode)) {
        return;
    }
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
    }
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    std::vector<SceneMode> modes = modeManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        modeManagerObj->GetSupportedOutputCapability(cameras_[0], portraitMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(portraitMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = static_cast<PortraitSession*>(captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    int32_t intResult = portraitSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    camera_rational_t ratio = {
        .numerator = 16,
        .denominator=9
    };

    Profile profile = SelectProfileByRatioAndFormat(modeAbility, ratio, photoFormat_);
    ASSERT_NE(profile.format_, -1);
    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(profile);
    ASSERT_NE(photoOutput, nullptr);
    intResult = portraitSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    profile = SelectProfileByRatioAndFormat(modeAbility, ratio, previewFormat_);
    ASSERT_NE(profile.format_, -1);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profile);
    ASSERT_NE(previewOutput, nullptr);
    intResult = portraitSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = portraitSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    portraitSession->LockForControl();

    std::vector<BeautyType> beautyLists = portraitSession->GetSupportedBeautyTypes();
    EXPECT_GE(beautyLists.size(), 0);

    if (beautyLists.size() == 1) {
        std::vector<int32_t> rangeLists = portraitSession->GetSupportedBeautyRange(beautyLists[0]);
        EXPECT_NE(rangeLists.size(), 0);
        bool boolResult = portraitSession->SetBeautyValue(beautyLists[0], rangeLists[0]);
        EXPECT_TRUE(boolResult);
    }

    portraitSession->UnlockForControl();
}

/* Feature: Framework
 * Function: Test preview/capture with portrait session's beauty
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with portrait session's beauty
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_058, TestSize.Level0)
{
    SceneMode nightMode = SceneMode::NIGHT;
    if (!IsSupportMode(nightMode)) {
        return;
    }
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    std::vector<SceneMode> modes = modeManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        modeManagerObj->GetSupportedOutputCapability(cameras_[0], nightMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(nightMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);

    int32_t intResult = nightSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = nightSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = nightSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = nightSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::vector<uint32_t> exposureRange = {};

    intResult = nightSession->GetExposureRange(exposureRange);
    EXPECT_EQ(intResult, 0);

    EXPECT_NE(exposureRange.size(), 0);

    if (!exposureRange.empty()) {
        nightSession->LockForControl();
        EXPECT_EQ(nightSession->SetExposure(exposureRange[0]), 0);
        nightSession->UnlockForControl();
        uint32_t manulExposure = 0;
        EXPECT_EQ(nightSession->GetExposure(manulExposure), 0);
        EXPECT_EQ(manulExposure, exposureRange[0]);
    }
}

/*
 * Feature: Framework
 * Function: Test set default color space
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set default color space
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_059, TestSize.Level0)
{
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->Open();

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::vector<ColorSpace> colorSpaceLists = session_->GetSupportedColorSpaces();
    if (colorSpaceLists.size() != 0) {
        ColorSpace colorSpace;
        intResult = session_->GetActiveColorSpace(colorSpace);
        EXPECT_EQ(intResult, 0);
        EXPECT_EQ(colorSpaceLists[0], colorSpace);
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test set color space before commitConfig
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set default color space
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_060, TestSize.Level0)
{
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->Open();

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    std::vector<ColorSpace> colorSpaceLists = session_->GetSupportedColorSpaces();
    if (colorSpaceLists.size() != 0) {
        intResult = session_->SetColorSpace(colorSpaceLists[1]);
        EXPECT_EQ(intResult, 0);
        ColorSpace colorSpace;
        intResult = session_->GetActiveColorSpace(colorSpace);
        EXPECT_EQ(intResult, 0);
        EXPECT_EQ(colorSpaceLists[1], colorSpace);
    }

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test set color space after commitConfig
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set default color space
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_061, TestSize.Level0)
{
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->Open();

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);


    std::vector<ColorSpace> colorSpaceLists = session_->GetSupportedColorSpaces();
    if (colorSpaceLists.size() != 0) {
        intResult = session_->SetColorSpace(colorSpaceLists[1]);
        EXPECT_EQ(intResult, 0);
        ColorSpace colorSpace;
        intResult = session_->GetActiveColorSpace(colorSpace);
        EXPECT_EQ(intResult, 0);
        EXPECT_EQ(colorSpaceLists[1], colorSpace);
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test set color space after session start
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set default color space
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_062, TestSize.Level0)
{
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
    camInput->Open();

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    std::vector<ColorSpace> colorSpaceLists = session_->GetSupportedColorSpaces();
    if (colorSpaceLists.size() != 0) {
        intResult = session_->SetColorSpace(colorSpaceLists[1]);
        EXPECT_EQ(intResult, 0);
        ColorSpace colorSpace;
        intResult = session_->GetActiveColorSpace(colorSpace);
        EXPECT_EQ(intResult, 0);
        EXPECT_EQ(colorSpaceLists[1], colorSpace);
    }

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test macro and sketch function.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test macro and sketch function.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_063, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::VIDEO);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    bool isSketchSupported = previewOutput->IsSketchSupported();
    if (!isSketchSupported) {
        return;
    }

    previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isMacroSupported = session_->IsMacroSupported();
    if (!isMacroSupported) {
        return;
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    // Video mode don't support sketch, but MacroVideo mode supported.
    float sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_LT(sketchRatio, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    session_->LockForControl();
    intResult = session_->EnableMacro(true);
    session_->UnlockForControl();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    // Now we are in MacroVideo mode
    sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchRatio, 0);

    session_->LockForControl();
    intResult = session_->EnableMacro(false);
    session_->UnlockForControl();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    // Now we are in Video mode
    sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_LT(sketchRatio, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Camera pre_switch test
 * Function: Test preSwitch interface
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preSwitch interface
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_064, TestSize.Level0)
{
    if (cameras_.size() <= 1) {
        EXPECT_LT(cameras_.size(), 2);
        return;
    }

    manager_->PreSwitchCamera(cameras_[1]->GetID());

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Camera sketch add output output with guess mode
 * Function: Test sketch add output with guess mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketch add output with guess mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_065, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    ASSERT_NE(output, nullptr);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    bool isSketchSupport = previewOutput->IsSketchSupported();
    if (!isSketchSupport) {
        return;
    }

    int sketchEnableRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchEnableRatio, 0);

    intResult = previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Test add released output
 * Function: Test add released output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test add released output
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_066, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto previewOutput = CreatePreviewOutput(*previewProfile);
    ASSERT_NE(previewOutput, nullptr);

    auto captureOutput = CreatePhotoOutput();
    EXPECT_NE(captureOutput, nullptr);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(captureOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(captureOutput);
    EXPECT_EQ(intResult, 0);

    intResult = captureOutput->Release();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(captureOutput);
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test smooth zoom and sketch functions callback.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test smooth zoom and sketch functions.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_067, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    bool isSketchSupport = previewOutput->IsSketchSupported();
    if (!isSketchSupport) {
        return;
    }

    previewOutput->SetCallback(std::make_shared<AppCallback>());

    int sketchEnableRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchEnableRatio, 0);

    intResult = previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 0);

    session_->LockForControl();
    intResult = session_->SetSmoothZoom(30.0f, 0);
    EXPECT_EQ(intResult, 0);
    session_->UnlockForControl();

    sleep(WAIT_TIME_AFTER_START);

    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 1);
    g_previewEvents.reset();

    sleep(WAIT_TIME_AFTER_START);
    auto statusSize = g_sketchStatus.size();
    EXPECT_EQ(statusSize, 2);
    if (statusSize == 2) {
        EXPECT_EQ(g_sketchStatus.front(), 3);
        g_sketchStatus.pop_front();
        EXPECT_EQ(g_sketchStatus.front(), 1);
        g_sketchStatus.pop_front();
    }

    sleep(1);
    EXPECT_EQ(g_previewEvents[static_cast<int>(CAM_PREVIEW_EVENTS::CAM_PREVIEW_SKETCH_STATUS_CHANGED)], 0);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}


/*
 * Feature: Framework
 * Function: Test moon capture boost function.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test moon capture boost function.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_068, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::CAPTURE);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    session_->SetFeatureDetectionStatusCallback(std::make_shared<AppCallback>());

    g_moonCaptureBoostEvents.reset();
    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    bool isMoonCaptureBoostSupported = session_->IsFeatureSupported(FEATURE_MOON_CAPTURE_BOOST);
    if (isMoonCaptureBoostSupported) {
        EXPECT_EQ(g_moonCaptureBoostEvents.count(), 1);
        if (g_moonCaptureBoostEvents[static_cast<int>(
                CAM_MOON_CAPTURE_BOOST_EVENTS::CAM_MOON_CAPTURE_BOOST_EVENT_ACTIVE)] == 1) {
            intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, true);
            EXPECT_EQ(intResult, 0);
        }
        if (g_moonCaptureBoostEvents[static_cast<int>(
                CAM_MOON_CAPTURE_BOOST_EVENTS::CAM_MOON_CAPTURE_BOOST_EVENT_IDLE)] == 1) {
            intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, false);
            EXPECT_EQ(intResult, 0);
        }
    }

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test moon capture boost function anomalous branch..
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test moon capture boost function anomalous branch..
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_069, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::VIDEO);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    bool isMoonCaptureBoostSupported = session_->IsFeatureSupported(FEATURE_MOON_CAPTURE_BOOST);
    EXPECT_FALSE(isMoonCaptureBoostSupported);

    intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, true);
    EXPECT_EQ(intResult, 7400102);

    intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, false);
    EXPECT_EQ(intResult, 7400102);

    session_->SetMode(SceneMode::CAPTURE);
    isMoonCaptureBoostSupported = session_->IsFeatureSupported(FEATURE_MOON_CAPTURE_BOOST);
    if (isMoonCaptureBoostSupported) {
        intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, true);
        EXPECT_EQ(intResult, 7400103);
    }

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    isMoonCaptureBoostSupported = session_->IsFeatureSupported(FEATURE_MOON_CAPTURE_BOOST);
    if (!isMoonCaptureBoostSupported) {
        intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, true);
        EXPECT_EQ(intResult, 7400102);
    }
    session_->SetFeatureDetectionStatusCallback(std::make_shared<AppCallback>());

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test moon capture boost and sketch function.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test moon capture boost and sketch function.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_070, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    session_->SetMode(SceneMode::NORMAL);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    bool isSketchSupported = previewOutput->IsSketchSupported();
    if (!isSketchSupported) {
        return;
    }

    previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isMoonCaptureBoostSupported = session_->IsFeatureSupported(FEATURE_MOON_CAPTURE_BOOST);
    if (!isMoonCaptureBoostSupported) {
        return;
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    float sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchRatio, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, true);
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchRatio, 0);

    intResult = session_->EnableFeature(FEATURE_MOON_CAPTURE_BOOST, false);
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchRatio, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test super macro photo session.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test super macro photo session.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_071, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    auto output = CreatePreviewOutput(*previewProfile);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    session_ = manager_->CreateCaptureSession(SceneMode::CAPTURE_MACRO);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    bool isSketchSupported = previewOutput->IsSketchSupported();
    EXPECT_TRUE(isSketchSupported);
    if (!isSketchSupported) {
        return;
    }

    previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    float sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchRatio, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test super macro video session.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test super macro video session.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_072, TestSize.Level0)
{
    auto previewProfile = GetSketchPreviewProfile();
    if (previewProfile == nullptr) {
        EXPECT_EQ(previewProfile.get(), nullptr);
        return;
    }
    SelectProfiles wanted;
    wanted.preview.size_ = {SKETCH_DEFAULT_WIDTH, SKETCH_DEFAULT_HEIGHT};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.size_ = {SKETCH_DEFAULT_WIDTH, SKETCH_DEFAULT_HEIGHT};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {MAX_FRAME_RATE, MAX_FRAME_RATE};
    sptr<CaptureOutput> output = CreatePreviewOutput(wanted.preview);
    sptr<PreviewOutput> previewOutput = (sptr<PreviewOutput>&)output;
    ASSERT_NE(output, nullptr);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(wanted.video);
    ASSERT_NE(videoOutput, nullptr);

    session_ = manager_->CreateCaptureSession(SceneMode::VIDEO_MACRO);
    int32_t intResult = session_->BeginConfig();

    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(output);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    bool isSketchSupported = previewOutput->IsSketchSupported();
    EXPECT_TRUE(isSketchSupported);
    if (!isSketchSupported) {
        return;
    }

    previewOutput->EnableSketch(true);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    float sketchRatio = previewOutput->GetSketchRatio();
    EXPECT_GT(sketchRatio, 0);

    intResult = previewOutput->AttachSketchSurface(CreateSketchSurface(previewProfile->GetCameraFormat()));
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test high-res photo session.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test high-res photo session capture.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_073, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> photoOutput;
    ConfigHighResSession(previewOutput, photoOutput);

    int32_t intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PreviewOutput>&)previewOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    intResult = highResSession_->Release();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test high-res photo session.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test high-res photo session setFocusPoint.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_074, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> photoOutput;
    ConfigHighResSession(previewOutput, photoOutput);

    Point point = { 1, 1 };
    highResSession_->LockForControl();
    int32_t setFocusMode = highResSession_->SetFocusPoint(point);
    EXPECT_EQ(setFocusMode, 0);
    highResSession_->UnlockForControl();

    Point focusPointGet = highResSession_->GetFocusPoint();
    EXPECT_EQ(focusPointGet.x, 1);
    EXPECT_EQ(focusPointGet.y, 1);

    int32_t intResult = ((sptr<PreviewOutput>&)previewOutput)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PreviewOutput>&)previewOutput)->Stop();
    EXPECT_EQ(intResult, 0);

    intResult = highResSession_->Release();
    EXPECT_EQ(intResult, 0);
}

/* Feature: Framework
 * Function: Test AR Mode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test AR Mode
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_075, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    uint32_t moduleType;
    intResult = session_->GetModuleType(moduleType);
    EXPECT_EQ(intResult, 0);

    uint32_t cam0ModuleType = cameras_[0]->GetModuleType();
    EXPECT_EQ(moduleType, cam0ModuleType);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    float minimumFocusDistance = session_->GetMinimumFocusDistance();
    MEDIA_INFO_LOG("minimumFocusDistance=%{public}f", minimumFocusDistance);

    std::vector<uint32_t> exposureTimeRange;
    intResult = session_->GetSensorExposureTimeRange(exposureTimeRange);
    EXPECT_EQ(intResult, 0);

    std::vector<int32_t> sensitivityRange;
    intResult = session_->GetSensorSensitivityRange(sensitivityRange);
    EXPECT_EQ(intResult, 0);

    session_->LockForControl();

    intResult = session_->SetARMode(true);
    EXPECT_EQ(intResult, 0);

    float num = 1.0f;
    intResult = session_->SetFocusDistance(num);
    EXPECT_EQ(intResult, 0);

    intResult = session_->SetSensorSensitivity(sensitivityRange[0]);
    EXPECT_EQ(intResult, 0);

    intResult = session_->SetSensorExposureTime(exposureTimeRange[0]);
    EXPECT_EQ(intResult, 0);

    session_->UnlockForControl();

    float recnum;
    intResult = session_->GetFocusDistance(recnum);
    EXPECT_EQ(intResult, 0);

    uint32_t recSensorExposureTime;
    intResult = session_->GetSensorExposureTime(recSensorExposureTime);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test set preview frame rate range dynamical
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set preview frame rate range dynamical
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_076, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> videoOutput;
    ConfigVideoSession(previewOutput, videoOutput);
    ASSERT_NE(previewOutput, nullptr);
    ASSERT_NE(videoOutput, nullptr);

    int32_t intResult = videoSession_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = videoSession_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutputTrans = ((sptr<PreviewOutput>&)previewOutput);
    intResult = previewOutputTrans->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    std::vector<std::vector<int32_t>> supportedFrameRateArray = previewOutputTrans->GetSupportedFrameRates();
    ASSERT_NE(supportedFrameRateArray.size(), 0);
    int32_t maxFpsTobeSet = 0;
    for (auto item : supportedFrameRateArray) {
        if (item[1] != 0) {
            maxFpsTobeSet = item[1];
        }
        cout << "supported: " << item[0] << item[1] <<endl;
    }

    std::vector<int32_t> activeFrameRateRange = previewOutputTrans->GetFrameRateRange();
    ASSERT_NE(activeFrameRateRange.size(), 0);
    EXPECT_EQ(activeFrameRateRange[0], 0);
    EXPECT_EQ(activeFrameRateRange[1], 0);

    intResult = previewOutputTrans->SetFrameRate(maxFpsTobeSet, maxFpsTobeSet);
    EXPECT_EQ(intResult, 0);

    std::cout<< "set: "<<maxFpsTobeSet<<maxFpsTobeSet<<std::endl;
    std::vector<int32_t> currentFrameRateRange = previewOutputTrans->GetFrameRateRange();
    EXPECT_EQ(currentFrameRateRange[0], maxFpsTobeSet);
    EXPECT_EQ(currentFrameRateRange[1], maxFpsTobeSet);
    sleep(WAIT_TIME_AFTER_START);

    // test set same frame rate
    intResult = previewOutputTrans->SetFrameRate(maxFpsTobeSet, maxFpsTobeSet);
    EXPECT_EQ(intResult, 7400101);
    sleep(WAIT_TIME_AFTER_START);

    intResult = previewOutputTrans->Release();
    EXPECT_EQ(intResult, 0);
}

/*
* Feature: Framework
* Function: Test normal branch for secure camera
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test normal branch for secure camera
*/
HWTEST_F(CameraFrameworkModuleTest, camera_framework_securecamera_moduleTest_001, TestSize.Level0)
{
    SceneMode secureMode = SceneMode::SECURE;
    if (!IsSupportMode(secureMode)) {
        return;
    }
    if (session_) {
        session_->Release();
    }
    if (scanSession_) {
        scanSession_->Release();
    }
    if (input_) {
        sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
        camInput->Close();
        input_->Release();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::vector<sptr<CameraDevice>> cameras = manager_->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = manager_->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = manager_->
            GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);

            sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);
            sptr<CaptureInput> inputSecure = manager_->CreateCameraInput(camDevice);
            ASSERT_NE(inputSecure, nullptr);

            sptr<CameraInput> inputSecure_ = (sptr<CameraInput>&)inputSecure;
            uint64_t secureSeqId = 0;
            int intResult = inputSecure_->Open(true, &secureSeqId);
            EXPECT_EQ(intResult, 0);
            EXPECT_NE(secureSeqId, 0);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            sptr<CaptureOutput> preview = CreatePreviewOutput();
            ASSERT_NE(preview, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(inputSecure), 0);
            EXPECT_EQ(secureSession->AddOutput(preview), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview), 0);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            secureSession->Release();
            inputSecure_->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test abnormal branch that is non-securemode while opening securecamera
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test abnormal branch that is non-securemode while opening securecamera
*/
HWTEST_F(CameraFrameworkModuleTest, camera_framework_securecamera_moduleTest_002, TestSize.Level0)
{
    SceneMode secureMode = SceneMode::SECURE;
    if (!IsSupportMode(secureMode)) {
        return;
    }
    if (session_) {
        session_->Release();
    }
    if (scanSession_) {
        scanSession_->Release();
    }
    if (input_) {
        sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
        camInput->Close();
        input_->Release();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::vector<sptr<CameraDevice>> cameras = manager_->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = manager_->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = manager_->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);

            sptr<CaptureInput> inputSecure = manager_->CreateCameraInput(camDevice);
            ASSERT_NE(inputSecure, nullptr);
            sptr<CameraInput> inputSecure_ = (sptr<CameraInput>&)inputSecure;
            uint64_t secureSeqId = 0;
            int intResult = inputSecure_->Open(true, &secureSeqId);
            EXPECT_EQ(intResult, 0);
            EXPECT_NE(secureSeqId, 0);

            sptr<CaptureOutput> preview = CreatePreviewOutput();
            ASSERT_NE(preview, nullptr);
            sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::NORMAL);
            ASSERT_NE(captureSession, nullptr);

            EXPECT_EQ(captureSession->BeginConfig(), 0);
            EXPECT_EQ(captureSession->AddInput(inputSecure), 0);
            EXPECT_EQ(captureSession->AddOutput(preview), 0);
            EXPECT_EQ(captureSession->CommitConfig(), 7400102);
            captureSession->Release();
            inputSecure_->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test abnormal branch that is add secure output flag twice
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test abnormal branch that is add secure output flag twice
*/
HWTEST_F(CameraFrameworkModuleTest, camera_framework_securecamera_moduleTest_003, TestSize.Level0)
{
    SceneMode secureMode = SceneMode::SECURE;
    if (!IsSupportMode(secureMode)) {
        return;
    }
    if (session_) {
        session_->Release();
    }
    if (scanSession_) {
        scanSession_->Release();
    }
    if (input_) {
        sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
        camInput->Close();
        input_->Release();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::vector<sptr<CameraDevice>> cameras = manager_->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = manager_->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = manager_->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);

            sptr<CaptureInput> inputSecure = manager_->CreateCameraInput(camDevice);
            ASSERT_NE(inputSecure, nullptr);
            sptr<CameraInput> inputSecure_ = (sptr<CameraInput>&)inputSecure;
            uint64_t secureSeqId = 0;
            int intResult = inputSecure_->Open(true, &secureSeqId);
            EXPECT_EQ(intResult, 0);
            EXPECT_NE(secureSeqId, 0);

            sptr<CaptureOutput> preview1 = CreatePreviewOutput();
            ASSERT_NE(preview1, nullptr);
            sptr<CaptureOutput> preview2 = CreatePreviewOutput();
            ASSERT_NE(preview2, nullptr);
            sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(inputSecure), 0);
            EXPECT_EQ(secureSession->AddOutput(preview1), 0);
            EXPECT_EQ(secureSession->AddOutput(preview2), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview1), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview2), CAMERA_OPERATION_NOT_ALLOWED);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            secureSession->Release();
            inputSecure_->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test abnormal branch that secure output is added after commiting
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test abnormal branch that secure output is added after commiting
*/
HWTEST_F(CameraFrameworkModuleTest, camera_framework_securecamera_moduleTest_004, TestSize.Level0)
{
    SceneMode secureMode = SceneMode::SECURE;
    if (!IsSupportMode(secureMode)) {
        return;
    }
    if (session_) {
        session_->Release();
    }
    if (scanSession_) {
        scanSession_->Release();
    }
    if (input_) {
        sptr<CameraInput> camInput = (sptr<CameraInput>&)input_;
        camInput->Close();
        input_->Release();
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::vector<sptr<CameraDevice>> cameras = manager_->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = manager_->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = manager_->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);

            sptr<CaptureInput> inputSecure = manager_->CreateCameraInput(camDevice);
            ASSERT_NE(inputSecure, nullptr);
            sptr<CameraInput> inputSecure_ = (sptr<CameraInput>&)inputSecure;
            uint64_t secureSeqId = 0;
            int intResult = inputSecure_->Open(true, &secureSeqId);
            EXPECT_EQ(intResult, 0);
            EXPECT_NE(secureSeqId, 0);

            sptr<CaptureOutput> preview = CreatePreviewOutput();
            ASSERT_NE(preview, nullptr);
            sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(inputSecure), 0);
            EXPECT_EQ(secureSession->AddOutput(preview), 0);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview), CAMERA_OPERATION_NOT_ALLOWED);
            secureSession->Release();
            inputSecure_->Close();
            break;
        }
    }
}

HWTEST_F(CameraFrameworkModuleTest, deferred_photo_enable, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->IsDeferredImageDeliverySupported(
        DeferredDeliveryImageType::DELIVERY_PHOTO);
    if (intResult != 0) {
        MEDIA_DEBUG_LOG("device not support deferred_photo");
        return;
    }

    ((sptr<PhotoOutput>&)photoOutput)->DeferImageDeliveryFor(DeferredDeliveryImageType::DELIVERY_PHOTO);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->IsDeferredImageDeliveryEnabled(
        DeferredDeliveryImageType::DELIVERY_PHOTO);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_DEFERRED_CAPTURE);

    ((sptr<PhotoOutput>&)photoOutput)->Release();
}

/*
 * Feature: Framework
 * Function: Test master ai
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test master ai
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_077, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    if (session_->IsEffectSuggestionSupported()) {

        std::vector<EffectSuggestionType> supportedTypeList = session_->GetSupportedEffectSuggestionType();
        EXPECT_NE(supportedTypeList.size(), 0);

        session_->LockForControl();

        intResult = session_->EnableEffectSuggestion(true);
        EXPECT_EQ(intResult, 0);

        std::vector<EffectSuggestionStatus> effectSuggestionStatusList =
            {{EffectSuggestionType::EFFECT_SUGGESTION_NONE, false},
             {EffectSuggestionType::EFFECT_SUGGESTION_PORTRAIT, true},
             {EffectSuggestionType::EFFECT_SUGGESTION_FOOD, false},
             {EffectSuggestionType::EFFECT_SUGGESTION_SKY, false},
             {EffectSuggestionType::EFFECT_SUGGESTION_SUNRISE_SUNSET, false}};
        intResult = session_->SetEffectSuggestionStatus(effectSuggestionStatusList);
        EXPECT_EQ(intResult, 0);

        intResult = session_->UpdateEffectSuggestion(EffectSuggestionType::EFFECT_SUGGESTION_PORTRAIT, false);
        EXPECT_EQ(intResult, 0);

        session_->UnlockForControl();
    }
}

/*
 * Feature: Framework
 * Function: Test quick shot photo session create.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test quick shot session photo create.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_078, TestSize.Level0)
{
    auto modes = manager_->GetSupportedModes(cameras_[0]);
    auto it = std::find_if(modes.begin(), modes.end(), [](auto& mode) { return mode == SceneMode::QUICK_SHOT_PHOTO; });
    if (it == modes.end()) {
        ASSERT_TRUE(true);
        return;
    }

    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::QUICK_SHOT_PHOTO);
    auto quickShotPhotoSession = static_cast<QuickShotPhotoSession*>(captureSession.GetRefPtr());
    ASSERT_NE(quickShotPhotoSession, nullptr);
    int32_t intResult = quickShotPhotoSession->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = quickShotPhotoSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = quickShotPhotoSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = quickShotPhotoSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = quickShotPhotoSession->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = quickShotPhotoSession->Stop();
    EXPECT_EQ(intResult, 0);
}
/*
 * Feature: Framework
 * Function: Test cameraStatus with bundleName
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test master ai
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_079, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    constexpr int32_t waitForCallbackTime = 5;
    sleep(waitForCallbackTime);
}

/*
 * Feature: Framework
 * Function: Test aperture video session create.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test aperture video session create.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_080, TestSize.Level0)
{
    auto modes = manager_->GetSupportedModes(cameras_[0]);
    auto it = std::find_if(modes.begin(), modes.end(), [](auto& mode) { return mode == SceneMode::APERTURE_VIDEO; });
    if (it == modes.end()) {
        ASSERT_TRUE(true);
        return;
    }
    auto outputCapability = manager_->GetSupportedOutputCapability(cameras_[0], SceneMode::APERTURE_VIDEO);
    ASSERT_NE(outputCapability, nullptr);
    auto previewProfiles = outputCapability->GetPreviewProfiles();
    ASSERT_TRUE(!previewProfiles.empty());

    auto videoProfiles = outputCapability->GetVideoProfiles();
    ASSERT_TRUE(!videoProfiles.empty());

    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::APERTURE_VIDEO);
    auto apertureVideoSession = static_cast<ApertureVideoSession*>(captureSession.GetRefPtr());
    ASSERT_NE(apertureVideoSession, nullptr);
    int32_t intResult = apertureVideoSession->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = apertureVideoSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(previewProfiles[0]);
    ASSERT_NE(previewOutput, nullptr);

    intResult = apertureVideoSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput = CreateVideoOutput(videoProfiles[0]);
    ASSERT_NE(videoOutput, nullptr);

    intResult = apertureVideoSession->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = apertureVideoSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = apertureVideoSession->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = apertureVideoSession->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test photo session preconfig.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photoSession CanPreconfig interface.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_081, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    PreconfigType preconfigTypes[] = { PRECONFIG_720P, PRECONFIG_1080P, PRECONFIG_4K, PRECONFIG_HIGH_QUALITY };
    ProfileSizeRatio preconfigRatios[] = { UNSPECIFIED, RATIO_1_1, RATIO_4_3, RATIO_16_9 };
    int ret;
    for (auto type : preconfigTypes) {
        for (auto ratio : preconfigRatios) {
            sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::CAPTURE);
            auto photoSession = static_cast<PhotoSession*>(captureSession.GetRefPtr());
            ASSERT_NE(photoSession, nullptr);
            if (photoSession->CanPreconfig(type, ratio)) {
                ret = photoSession->Preconfig(type, ratio);
                EXPECT_EQ(ret, 0);

                ret = photoSession->BeginConfig();
                EXPECT_EQ(ret, 0);

                ret = photoSession->AddInput(input_);
                EXPECT_EQ(ret, 0);

                sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
                sptr<IBufferProducer> previewProducer = previewSurface->GetProducer();
                sptr<Surface> producerPreviewSurface = Surface::CreateSurfaceAsProducer(previewProducer);
                sptr<PreviewOutput> previewOutput = nullptr;
                ret = manager_->CreatePreviewOutputWithoutProfile(producerPreviewSurface, &previewOutput);
                EXPECT_EQ(ret, 0);
                sptr<CaptureOutput> capturePreviewOutput = previewOutput;
                ret = photoSession->AddOutput(capturePreviewOutput);
                EXPECT_EQ(ret, 0);

                sptr<IConsumerSurface> photoSurface = IConsumerSurface::Create();
                sptr<IBufferProducer> photoProducer = photoSurface->GetProducer();
                sptr<PhotoOutput> photoOutput = nullptr;
                ret = manager_->CreatePhotoOutputWithoutProfile(photoProducer, &photoOutput);
                EXPECT_EQ(ret, 0);
                sptr<CaptureOutput> capturePhotoOutput = photoOutput;
                ret = photoSession->AddOutput(capturePhotoOutput);
                EXPECT_EQ(ret, 0);

                ret = photoSession->CommitConfig();
                EXPECT_EQ(ret, 0);

                ret = photoSession->BeginConfig();
                EXPECT_EQ(ret, 0);

                ret = photoSession->RemoveInput(input_);
                EXPECT_EQ(ret, 0);
            }
            ret = photoSession->Release();
            EXPECT_EQ(ret, 0);
        }
    }
}

/*
 * Feature: Framework
 * Function: Test video session preconfig.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test videoSession CanPreconfig interface.
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_082, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    PreconfigType preconfigTypes[] = { PRECONFIG_720P, PRECONFIG_1080P, PRECONFIG_4K, PRECONFIG_HIGH_QUALITY };
    ProfileSizeRatio preconfigRatios[] = { UNSPECIFIED, RATIO_1_1, RATIO_4_3, RATIO_16_9 };
    int ret;
    for (auto type : preconfigTypes) {
        for (auto ratio : preconfigRatios) {
            sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::VIDEO);
            auto videoSession = static_cast<VideoSession*>(captureSession.GetRefPtr());
            ASSERT_NE(videoSession, nullptr);
            if (videoSession->CanPreconfig(type, ratio)) {
                ret = videoSession->Preconfig(type, ratio);
                ASSERT_EQ(ret, 0);

                ret = videoSession->BeginConfig();
                EXPECT_EQ(ret, 0);

                ret = videoSession->AddInput(input_);
                EXPECT_EQ(ret, 0);

                sptr<IConsumerSurface> previewSurface = IConsumerSurface::Create();
                sptr<IBufferProducer> previewProducer = previewSurface->GetProducer();
                sptr<Surface> producerPreviewSurface = Surface::CreateSurfaceAsProducer(previewProducer);
                sptr<PreviewOutput> previewOutput = nullptr;
                ret = manager_->CreatePreviewOutputWithoutProfile(producerPreviewSurface, &previewOutput);
                EXPECT_EQ(ret, 0);
                sptr<CaptureOutput> capturePreviewOutput = previewOutput;
                ret = videoSession->AddOutput(capturePreviewOutput);
                EXPECT_EQ(ret, 0);

                sptr<IConsumerSurface> photoSurface = IConsumerSurface::Create();
                sptr<IBufferProducer> photoProducer = photoSurface->GetProducer();
                sptr<PhotoOutput> photoOutput = nullptr;
                ret = manager_->CreatePhotoOutputWithoutProfile(photoProducer, &photoOutput);
                EXPECT_EQ(ret, 0);
                sptr<CaptureOutput> capturePhotoOutput = photoOutput;
                ret = videoSession->AddOutput(capturePhotoOutput);
                EXPECT_EQ(ret, 0);

                sptr<IConsumerSurface> videoSurface = IConsumerSurface::Create();
                sptr<IBufferProducer> videoProducer = previewSurface->GetProducer();
                sptr<Surface> captureVideoSurface = Surface::CreateSurfaceAsProducer(videoProducer);
                sptr<VideoOutput> videoOutput = nullptr;
                ret = manager_->CreateVideoOutputWithoutProfile(captureVideoSurface, &videoOutput);
                EXPECT_EQ(ret, 0);
                sptr<CaptureOutput> captureVideoOutput = videoOutput;
                ret = videoSession->AddOutput(captureVideoOutput);
                EXPECT_EQ(ret, 0);

                ret = videoSession->CommitConfig();
                EXPECT_EQ(ret, 0);

                ret = videoSession->BeginConfig();
                EXPECT_EQ(ret, 0);

                ret = videoSession->RemoveInput(input_);
                EXPECT_EQ(ret, 0);
            }
            ret = videoSession->Release();
            EXPECT_EQ(ret, 0);
        }
    }
}

/*
 * Feature: Framework
 * Function: Test Callback for Occlusion Detection
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Callback for Occlusion Detection
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_083, TestSize.Level0)
{
    sptr<CameraInput> input = (sptr<CameraInput>&)input_;
    sptr<ICameraDeviceService> deviceObj = input->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    sptr<CameraDevice> camdeviceObj = nullptr;
    sptr<CameraInput> camInput_1 = new CameraInput(deviceObj, camdeviceObj);
    ASSERT_NE(camInput_1, nullptr);

    sptr<CameraDeviceServiceCallback> callback = new CameraDeviceServiceCallback(camInput_1);

    uint64_t timestamp = 10;
    const int32_t DEFAULT_ITEMS = 10;
    const int32_t DEFAULT_DATA_LENGTH = 100;
    int32_t count = 1;
    int32_t isOcclusionDetected = 1;
    std::shared_ptr<OHOS::Camera::CameraMetadata> result =
        std::make_shared<OHOS::Camera::CameraMetadata>(DEFAULT_ITEMS, DEFAULT_DATA_LENGTH);
    bool status = result->addEntry(OHOS_STATUS_CAMERA_OCCLUSION_DETECTION, &isOcclusionDetected, count);
    ASSERT_TRUE(status);

    int32_t intResult = callback->OnResult(timestamp, result);
    sleep(WAIT_TIME_AFTER_START);
    EXPECT_EQ(intResult, 0);

    result = std::make_shared<OHOS::Camera::CameraMetadata>(DEFAULT_ITEMS, DEFAULT_DATA_LENGTH);
    intResult = callback->OnResult(timestamp, result);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test video frame rate
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test video frame rate with normal setting branch
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_084, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::LIGHT_PAINTING;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles wanted;
    wanted.preview.size_ = {1920, 1080};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;

    SelectProfiles profiles = SelectWantedProfiles(modeAbility, wanted);
    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<LightPaintingSession> session = static_cast<LightPaintingSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    std::vector<LightPaintingType> supportedType;
    session->GetSupportedLightPaintings(supportedType);
    EXPECT_NE(supportedType.size(), 0);

    LightPaintingType setType = LightPaintingType::LIGHT;
    session->LockForControl();
    intResult = session->SetLightPainting(setType);
    session->UnlockForControl();
    EXPECT_EQ(intResult, 0);

    LightPaintingType defaultType;
    intResult = session->GetLightPainting(defaultType);
    EXPECT_EQ(defaultType, setType);

    session->LockForControl();
    intResult = session->TriggerLighting();
    session->UnlockForControl();
    EXPECT_EQ(intResult, 0);
}

/* Feature: Framework
 * Function: Test preview/capture with night session's beauty
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with night session's beauty
 */
HWTEST_F(CameraFrameworkModuleTest, test_night_mode_beauty, TestSize.Level0)
{
    SceneMode nightMode = SceneMode::NIGHT;
    if (!IsSupportMode(nightMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> modes = cameraManagerObj->GetSupportedModes(cameras_[1]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[1], nightMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(nightMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);
    int32_t intResult = nightSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = nightSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    camera_rational_t ratio = {
        .numerator = 4,
        .denominator=3
    };
    Profile profile = SelectProfileByRatioAndFormat(modeAbility, ratio, photoFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(profile);
    ASSERT_NE(photoOutput, nullptr);

    intResult = nightSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    profile = SelectProfileByRatioAndFormat(modeAbility, ratio, previewFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profile);
    ASSERT_NE(previewOutput, nullptr);

    intResult = nightSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    EXPECT_EQ(nightSession->CommitConfig(), 0);

    nightSession->LockForControl();

    std::vector<BeautyType> beautyLists = nightSession->GetSupportedBeautyTypes();
    EXPECT_GE(beautyLists.size(), 1);

    std::vector<int32_t> rangeLists = {};
    if (beautyLists.size() >= 1) {
        rangeLists = nightSession->GetSupportedBeautyRange(beautyLists[0]);
    }

    if (beautyLists.size() >= 1) {
        nightSession->SetBeauty(beautyLists[0], rangeLists[0]);
    }

    nightSession->UnlockForControl();

    if (beautyLists.size() >= 1) {
        EXPECT_EQ(nightSession->GetBeauty(beautyLists[0]), rangeLists[0]);
    }

    EXPECT_EQ(nightSession->Start(), 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    nightSession->Stop();
}

HWTEST_F(CameraFrameworkModuleTest, test_moving_photo, TestSize.Level0)
{
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
    }
    session_ = manager_->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(session_, nullptr);

    bool isSupported = session_->IsMovingPhotoSupported();
    EXPECT_EQ(isSupported, false);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    isSupported = session_->IsMovingPhotoSupported();
    EXPECT_EQ(isSupported, true);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    session_->LockForControl();
    intResult = session_->EnableMovingPhoto(true);
    EXPECT_EQ(intResult, 0);
    session_->UnlockForControl();

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test IsSessionCommited() || IsSessionConfiged() with abnormal branches in CaptureSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_086, TestSize.Level0)
{
    if (!IsSupportNow()) {
        return;
    }
    sptr<CaptureSession> captureSession = manager_->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(captureSession, nullptr);
    std::vector<float> virtualApertures = {};
    float aperture;
    EXPECT_EQ(captureSession->GetSupportedVirtualApertures(virtualApertures), SESSION_NOT_CONFIG);
    EXPECT_EQ(captureSession->GetVirtualAperture(aperture), SESSION_NOT_CONFIG);
    EXPECT_EQ(captureSession->SetVirtualAperture(aperture), SESSION_NOT_CONFIG);  
}

/* Feature: Framework
 * Function: Test preview/capture with capture session's VirtualAperture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview/capture with capture session's VirtualAperture
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_087, TestSize.Level0)
{
    SceneMode captureMode = SceneMode::CAPTURE;
    if (!IsSupportMode(captureMode)) {
        return;
    }
    if (session_) {
        MEDIA_INFO_LOG("old session exist, need release");
        session_->Release();
    }
    sptr<CameraManager> modeManagerObj = CameraManager::GetInstance();
    ASSERT_NE(modeManagerObj, nullptr);

    std::vector<SceneMode> modes = modeManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(modes.size() != 0);


    sptr<CameraOutputCapability> modeAbility =
        modeManagerObj->GetSupportedOutputCapability(cameras_[0], captureMode);
    ASSERT_NE(modeAbility, nullptr);

    sptr<CaptureSession> captureSession = modeManagerObj->CreateCaptureSession(captureMode);
    ASSERT_NE(captureSession, nullptr);

    int32_t intResult = captureSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = captureSession->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    camera_rational_t ratio = {
        .numerator = 16,
        .denominator=9
    };

    Profile profile = SelectProfileByRatioAndFormat(modeAbility, ratio, photoFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(profile);
    ASSERT_NE(photoOutput, nullptr);

    intResult = captureSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    profile = SelectProfileByRatioAndFormat(modeAbility, ratio, previewFormat_);
    ASSERT_NE(profile.format_, -1);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profile);
    ASSERT_NE(previewOutput, nullptr);

    intResult = captureSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = captureSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    captureSession->LockForControl();

    std::vector<float> virtualApertures = {};
    EXPECT_EQ(captureSession->GetSupportedVirtualApertures(virtualApertures), 0);
    EXPECT_EQ(captureSession->SetVirtualAperture(virtualApertures[0]), 0);

    captureSession->UnlockForControl();
    float aperture;
    EXPECT_EQ(captureSession->GetVirtualAperture(aperture), 0);
    EXPECT_EQ(aperture, virtualApertures[0]);

    intResult = captureSession->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    captureSession->Stop();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test abnormal branches with empty inputDevice and empty metadata
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_088, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);
    float getAperture = 0.0f;
    int32_t intResult = camSession->GetVirtualAperture(getAperture);
    EXPECT_EQ(intResult, 7400103);
    EXPECT_EQ(getAperture, 0.0f);

    camSession->LockForControl();
    float setAperture = 1.0f;
    intResult = camSession->SetVirtualAperture(setAperture);
    EXPECT_EQ(intResult, 7400103);
    camSession->UnlockForControl();

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = camSession->GetVirtualAperture(getAperture);
    EXPECT_EQ(intResult, 0);
    EXPECT_EQ(getAperture, 0.95f);

    camSession->LockForControl();
    intResult = camSession->SetVirtualAperture(setAperture);
    EXPECT_EQ(intResult, 0);
    camSession->UnlockForControl();
    intResult = camSession->GetVirtualAperture(getAperture);
    EXPECT_EQ(getAperture, 0.95f);

    ReleaseInput();

    intResult = camSession->GetVirtualAperture(getAperture);
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test set video frame rate range dynamical
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set video frame rate range dynamical
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_089, TestSize.Level0)
{
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> videoOutput;
    ConfigVideoSession(previewOutput, videoOutput);
    ASSERT_NE(previewOutput, nullptr);
    ASSERT_NE(videoOutput, nullptr);

    int32_t intResult = videoSession_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = videoSession_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<VideoOutput> videoOutputTrans = ((sptr<VideoOutput>&)videoOutput);
    intResult = videoOutputTrans->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    std::vector<std::vector<int32_t>> supportedFrameRateArray = videoOutputTrans->GetSupportedFrameRates();
    ASSERT_NE(supportedFrameRateArray.size(), 0);
    int32_t maxFpsTobeSet = 0;
    for (auto item : supportedFrameRateArray) {
        if (item[1] != 0) {
            maxFpsTobeSet = item[1];
        }
        cout << "supported: " << item[0] << item[1] <<endl;
    }

    std::vector<int32_t> activeFrameRateRange = videoOutputTrans->GetFrameRateRange();
    ASSERT_NE(activeFrameRateRange.size(), 0);
    EXPECT_EQ(activeFrameRateRange[0], 1);
    EXPECT_EQ(activeFrameRateRange[1], 30);

    intResult = videoOutputTrans->SetFrameRate(maxFpsTobeSet, maxFpsTobeSet);
    EXPECT_EQ(intResult, 0);

    std::cout<< "set: " << maxFpsTobeSet << maxFpsTobeSet << std::endl;
    std::vector<int32_t> currentFrameRateRange = videoOutputTrans->GetFrameRateRange();
    EXPECT_EQ(currentFrameRateRange[0], maxFpsTobeSet);
    EXPECT_EQ(currentFrameRateRange[1], maxFpsTobeSet);
    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutputTrans->SetFrameRate(MIN_FRAME_RATE, MAX_FRAME_RATE);
    EXPECT_EQ(intResult, 0);

    std::cout<< "set: "<< MIN_FRAME_RATE << MAX_FRAME_RATE << std::endl;
    currentFrameRateRange = videoOutputTrans->GetFrameRateRange();
    EXPECT_EQ(currentFrameRateRange[0], MIN_FRAME_RATE);
    EXPECT_EQ(currentFrameRateRange[1], MAX_FRAME_RATE);
    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutputTrans->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
}

/*
* Feature: Framework
* Function: Test TimeLapsePhoto Init Session
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test TimeLapsePhoto Init Session
*/
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_timelapsephoto_001, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::TIMELAPSE_PHOTO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    if (session_) {
        session_->Release();
        session_.clear();
    }
    sptr<CameraManager> cameraManager = CameraManager::GetInstance();
    ASSERT_NE(cameraManager, nullptr);
    auto cameras = cameraManager->GetSupportedCameras();
    ASSERT_NE(cameras.size(), 0);
    sptr<CameraDevice> camera = cameras[0];
    ASSERT_NE(camera, nullptr);
    sptr<CameraOutputCapability> capability = cameraManager->GetSupportedOutputCapability(camera, sceneMode);
    ASSERT_NE(capability, nullptr);
    SelectProfiles wanted;
    wanted.preview.size_ = {640, 480};
    wanted.preview.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.photo.size_ = {640, 480};
    wanted.photo.format_ = CAMERA_FORMAT_JPEG;
    wanted.video.size_ = {640, 480};
    wanted.video.format_ = CAMERA_FORMAT_YUV_420_SP;
    wanted.video.framerates_ = {30, 30};
    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<TimeLapsePhotoSession> session = reinterpret_cast<TimeLapsePhotoSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);
    int32_t status = session->BeginConfig();
    EXPECT_EQ(status, 0);
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(camera);
    ASSERT_NE(input, nullptr);
    status = input->Open();
    EXPECT_EQ(status, 0);
    status = session->AddInput(input);
    EXPECT_EQ(status, 0);
    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(wanted.preview);
    ASSERT_NE(previewOutput, nullptr);
    status = session->AddOutput(previewOutput);
    EXPECT_EQ(status, 0);
    sptr<CaptureOutput> photoOutput = CreatePhotoOutput(wanted.photo);
    ASSERT_NE(photoOutput, nullptr);
    status = session->AddOutput(photoOutput);
    EXPECT_EQ(status, 0);
    status = session->CommitConfig();
    EXPECT_EQ(status, 0);
    class ExposureInfoCallbackMock : public ExposureInfoCallback {
    public:
        void OnExposureInfoChanged(ExposureInfo info) override
        {
            EXPECT_EQ(info.exposureDurationValue, 1);
        }
    };
    session->SetExposureInfoCallback(make_shared<ExposureInfoCallbackMock>());
    static const camera_rational_t r = {
        .denominator = 1000000,
        .numerator = 1,
    };
    auto meta = make_shared<OHOS::Camera::CameraMetadata>(10, 100);
    EXPECT_EQ(meta->addEntry(OHOS_STATUS_SENSOR_EXPOSURE_TIME, &r, 1), true);
    session->ProcessExposureChange(meta);
 
    class IsoInfoCallbackMock : public IsoInfoCallback {
    public:
        void OnIsoInfoChanged(IsoInfo info) override
        {
            EXPECT_EQ(info.isoValue, 1);
        }
    };
    session->SetIsoInfoCallback(make_shared<IsoInfoCallbackMock>());
    static const uint32_t iso = 1;
    EXPECT_EQ(meta->addEntry(OHOS_STATUS_ISO_VALUE, &iso, 1), true);
    session->ProcessIsoInfoChange(meta);
 
    static const uint32_t lumination = 256;
    class LuminationInfoCallbackMock : public LuminationInfoCallback {
    public:
        void OnLuminationInfoChanged(LuminationInfo info) override
        {
            EXPECT_EQ((int32_t)info.luminationValue, 1);
        }
    };
    session->SetLuminationInfoCallback(make_shared<LuminationInfoCallbackMock>());
    EXPECT_EQ(meta->addEntry(OHOS_STATUS_ALGO_MEAN_Y, &lumination, 1), true);
    session->ProcessLuminationChange(meta);
 
    static const int32_t value = 1;
    class TryAEInfoCallbackMock : public TryAEInfoCallback {
    public:
        void OnTryAEInfoChanged(TryAEInfo info) override
        {
            EXPECT_EQ(info.isTryAEDone, true);
            EXPECT_EQ(info.isTryAEHintNeeded, true);
            EXPECT_EQ(info.previewType, TimeLapsePreviewType::DARK);
            EXPECT_EQ(info.captureInterval, 1);
        }
    };
    session->SetTryAEInfoCallback(make_shared<TryAEInfoCallbackMock>());
    EXPECT_EQ(meta->addEntry(OHOS_STATUS_TIME_LAPSE_TRYAE_DONE, &value, 1), true);
    EXPECT_EQ(meta->addEntry(OHOS_STATUS_TIME_LAPSE_TRYAE_HINT, &value, 1), true);
    EXPECT_EQ(meta->addEntry(OHOS_STATUS_TIME_LAPSE_PREVIEW_TYPE, &value, 1), true);
    EXPECT_EQ(meta->addEntry(OHOS_STATUS_TIME_LAPSE_CAPTURE_INTERVAL, &value, 1), true);
    session->ProcessSetTryAEChange(meta);
 
    EXPECT_EQ(meta->addEntry(OHOS_CAMERA_MACRO_STATUS, &value, 1), true);
    session->ProcessPhysicalCameraSwitch(meta);
 
    bool isTryAENeeded;
    status = session->IsTryAENeeded(isTryAENeeded);
    EXPECT_EQ(status, 0);
    if (isTryAENeeded) {
        session->LockForControl();
        status = session->StartTryAE();
        session->UnlockForControl();
        EXPECT_EQ(status, 0);
        session->LockForControl();
        status = session->StopTryAE();
        session->UnlockForControl();
        EXPECT_EQ(status, 0);
    }
    vector<int32_t> range;
    status = session->GetSupportedTimeLapseIntervalRange(range);
    EXPECT_EQ(status, 0);
    if (!range.empty()) {
        session->LockForControl();
        status = session->SetTimeLapseInterval(range[0]);
        session->UnlockForControl();
        EXPECT_EQ(status, 0);
        int32_t interval;
        status = session->GetTimeLapseInterval(interval);
        EXPECT_EQ(status, 0);
        EXPECT_EQ(interval, range[0]);
    }
    session->LockForControl();
    status = session->SetTimeLapseRecordState(TimeLapseRecordState::RECORDING);
    session->UnlockForControl();
    EXPECT_EQ(status, 0);
    session->LockForControl();
    status = session->SetTimeLapsePreviewType(TimeLapsePreviewType::DARK);
    session->UnlockForControl();
    EXPECT_EQ(status, 0);
    session->LockForControl();
    status = session->SetExposureHintMode(ExposureHintMode::EXPOSURE_HINT_MODE_OFF);
    session->UnlockForControl();
    EXPECT_EQ(status, 0);
    // ManualExposure
    vector<uint32_t> exposureRange;
    status = session->GetSupportedExposureRange(exposureRange);
    EXPECT_EQ(status, 0);
    if (!exposureRange.empty()) {
        session->LockForControl();
        status = session->SetExposure(exposureRange[0]);
        session->UnlockForControl();
        EXPECT_EQ(status, 0);
        uint32_t exposure;
        status = session->GetExposure(exposure);
        EXPECT_EQ(status, 0);
        EXPECT_EQ(exposure, exposureRange[0]);
    }
    vector<MeteringMode> modes;
    status = session->GetSupportedMeteringModes(modes);
    EXPECT_EQ(status, 0);
    if (!modes.empty()) {
        bool supported;
        int32_t i = METERING_MODE_CENTER_WEIGHTED;
        for (;i <= METERING_MODE_SPOT; i++) {
            status = session->IsExposureMeteringModeSupported(METERING_MODE_CENTER_WEIGHTED, supported);
            EXPECT_EQ(status, 0);
            if (status == 0 && supported) {
                break;
            }
        }
        if (supported) {
            session->LockForControl();
            status = session->SetExposureMeteringMode(static_cast<MeteringMode>(i));
            session->UnlockForControl();
            EXPECT_EQ(status, 0);
            MeteringMode mode;
            status = session->GetExposureMeteringMode(mode);
            EXPECT_EQ(status, 0);
            EXPECT_EQ(mode, i);
        }
    }
    // ManualIso
    bool isManualIsoSupported;
    status = session->IsManualIsoSupported(isManualIsoSupported);
    EXPECT_EQ(status, 0);
    if (isManualIsoSupported) {
        vector<int32_t> isoRange;
        status = session->GetIsoRange(isoRange);
        EXPECT_EQ(status, 0);
        if (!isoRange.empty()) {
            session->LockForControl();
            status = session->SetIso(isoRange[0]);
            session->UnlockForControl();
            EXPECT_EQ(status, 0);
            int32_t iso;
            status = session->GetIso(iso);
            EXPECT_EQ(status, 0);
            EXPECT_EQ(iso, isoRange[0]);
        }
    }
    // WhiteBalance
    vector<WhiteBalanceMode> wbModes;
    status = session->GetSupportedWhiteBalanceModes(wbModes);
    EXPECT_EQ(status, 0);
    if (!wbModes.empty()) {
        bool isWhiteBalanceModeSupported;
        status = session->IsWhiteBalanceModeSupported(wbModes[0], isWhiteBalanceModeSupported);
        EXPECT_EQ(status, 0);
        if (isWhiteBalanceModeSupported) {
            session->LockForControl();
            status = session->SetWhiteBalanceMode(wbModes[0]);
            session->UnlockForControl();
            EXPECT_EQ(status, 0);
            WhiteBalanceMode mode;
            status = session->GetWhiteBalanceMode(mode);
            EXPECT_EQ(status, 0);
            EXPECT_EQ(mode, wbModes[0]);
        }
    }
    vector<int32_t> wbRange;
    status = session->GetWhiteBalanceRange(wbRange);
    EXPECT_EQ(status, 0);
    if (!wbRange.empty()) {
        session->LockForControl();
        status = session->SetWhiteBalance(wbRange[0]);
        session->UnlockForControl();
        EXPECT_EQ(status, 0);
        int32_t wb;
        status = session->GetWhiteBalance(wb);
        EXPECT_EQ(status, 0);
        EXPECT_EQ(wb, wbRange[0]);
    }
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test Init PanoramaSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Init PanoramaSession
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_panorama, TestSize.Level0)
{
    SceneMode sceneMode = SceneMode::PANORAMA_PHOTO;
    if (!IsSupportMode(sceneMode)) {
        return;
    }
    sptr<CameraManager> cameraManagerObj = CameraManager::GetInstance();
    ASSERT_NE(cameraManagerObj, nullptr);

    std::vector<SceneMode> sceneModes = cameraManagerObj->GetSupportedModes(cameras_[0]);
    ASSERT_TRUE(sceneModes.size() != 0);

    sptr<CameraOutputCapability> modeAbility =
        cameraManagerObj->GetSupportedOutputCapability(cameras_[0], sceneMode);
    ASSERT_NE(modeAbility, nullptr);

    SelectProfiles profiles;
    profiles.preview.size_ = {1920, 1080};
    profiles.preview.format_ = CAMERA_FORMAT_YUV_420_SP;

    ASSERT_NE(profiles.preview.format_, CAMERA_FORMAT_INVALID);

    sptr<CaptureSession> captureSession = cameraManagerObj->CreateCaptureSession(sceneMode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PanoramaSession> session = static_cast<PanoramaSession*>(captureSession.GetRefPtr());
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput(profiles.preview);
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->Start();
    EXPECT_EQ(intResult, 0);

    intResult = session->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test dynamic remove preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test dynamic remove preview output
 */
HWTEST_F(CameraFrameworkModuleTest, test_dynamic_remove_previewoutput, TestSize.Level0)
{
    auto previewOutput1 = CreatePreviewOutput();
    ASSERT_NE(previewOutput1, nullptr);

    auto previewOutput2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput2, nullptr);

    auto captureOutput = CreatePhotoOutput();
    ASSERT_NE(captureOutput, nullptr);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(previewOutput1);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(previewOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(captureOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->RemoveOutput(previewOutput2);
    EXPECT_EQ(intResult, 0);

    ((sptr<PreviewOutput>&)previewOutput2)->Release();

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test dynamic add preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test dynamic add preview output
 */
HWTEST_F(CameraFrameworkModuleTest, test_dynamic_add_previewoutput, TestSize.Level0)
{
    auto previewOutput1 = CreatePreviewOutput();
    ASSERT_NE(previewOutput1, nullptr);

    auto previewOutput2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput2, nullptr);

    auto captureOutput = CreatePhotoOutput();
    ASSERT_NE(captureOutput, nullptr);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(previewOutput1);
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(captureOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddOutput(previewOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test HighQualityPhoto with anomalous branch
 */
HWTEST_F(CameraFrameworkModuleTest, test_high_quality_Photo_enable, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    bool isEnabled = false;
    int32_t intResult = camSession->EnableAutoHighQualityPhoto(isEnabled);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    int32_t isAutoHighQualityPhotoSupported;
    intResult = photoOutput_1->IsAutoHighQualityPhotoSupported(isAutoHighQualityPhotoSupported);

    if (!IsSupportNow()) {
        EXPECT_EQ(intResult, -1);
    } else {
        EXPECT_EQ(intResult, 0);
    }

    intResult = photoOutput_1->EnableAutoHighQualityPhoto(isEnabled);
    EXPECT_EQ(intResult, 7400101);

    intResult = camSession->EnableAutoHighQualityPhoto(isEnabled);
    EXPECT_EQ(intResult, 0);

    intResult = photoOutput_1->Release();
    EXPECT_EQ(intResult, 0);

    intResult = photoOutput_1->IsAutoHighQualityPhotoSupported(isAutoHighQualityPhotoSupported);
    EXPECT_EQ(intResult, 7400104);

    intResult = photoOutput_1->EnableAutoHighQualityPhoto(isEnabled);
    EXPECT_EQ(intResult, 7400104);
}

/*
 * Feature: Framework
 * Function: Test slow motion callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test slow motion callback
 */
HWTEST_F(CameraFrameworkModuleTest, test_slow_motion_callback, TestSize.Level0)
{
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> videoOutput;
    ConfigSlowMotionSession(previewOutput, videoOutput);
    ASSERT_NE(previewOutput, nullptr);
    ASSERT_NE(videoOutput, nullptr);

    int32_t intResult = slowMotionSession_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = slowMotionSession_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);
    intResult = slowMotionSession_->Start();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test video frame rate
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test video frame rate with abnormal setting branch
 */
HWTEST_F(CameraFrameworkModuleTest, test_video_frame_rate, TestSize.Level0)
{
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> videoOutput;
    ConfigVideoSession(previewOutput, videoOutput);
    ASSERT_NE(previewOutput, nullptr);
    ASSERT_NE(videoOutput, nullptr);

    int32_t intResult = videoSession_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = videoSession_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<VideoOutput> videoOutputTrans = ((sptr<VideoOutput>&)videoOutput);
    intResult = videoOutputTrans->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    std::vector<std::vector<int32_t>> supportedFrameRateArray = videoOutputTrans->GetSupportedFrameRates();
    ASSERT_NE(supportedFrameRateArray.size(), 0);
    int32_t maxFpsTobeSet = 0;
    for (auto item : supportedFrameRateArray) {
        if (item[1] != 0) {
            maxFpsTobeSet = item[1];
        }
        cout << "supported: " << item[0] << item[1] <<endl;
    }

    std::vector<int32_t> activeFrameRateRange = videoOutputTrans->GetFrameRateRange();
    ASSERT_NE(activeFrameRateRange.size(), 0);
    EXPECT_EQ(activeFrameRateRange[0], 1);
    EXPECT_EQ(activeFrameRateRange[1], 30);

    intResult = videoOutputTrans->SetFrameRate(maxFpsTobeSet, maxFpsTobeSet);
    EXPECT_EQ(intResult, 0);

    std::cout<< "set: "<<maxFpsTobeSet<<maxFpsTobeSet<<std::endl;
    std::vector<int32_t> currentFrameRateRange = videoOutputTrans->GetFrameRateRange();
    EXPECT_EQ(currentFrameRateRange[0], maxFpsTobeSet);
    EXPECT_EQ(currentFrameRateRange[1], maxFpsTobeSet);
    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutputTrans->SetFrameRate(15, 15);
    EXPECT_EQ(intResult, 0);

    std::cout<< "set: "<<15<<15<<std::endl;
    currentFrameRateRange = videoOutputTrans->GetFrameRateRange();
    EXPECT_EQ(currentFrameRateRange[0], 15);
    EXPECT_EQ(currentFrameRateRange[1], 15);
    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutputTrans->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test burst capture
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test burst capture
 */
HWTEST_F(CameraFrameworkModuleTest, test_burst_capture, TestSize.Level0)
{
    ASSERT_NE(session_, nullptr);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetBurstCaptureState(1);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->Capture(photoSetting);
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_CAPTURE);

    intResult = ((sptr<PhotoOutput>&)photoOutput)->ConfirmCapture();
    EXPECT_EQ(intResult, 0);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Testing folded state callbacks
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Testing folded state callbacks
 */
HWTEST_F(CameraFrameworkModuleTest, test_folded_state_callback, TestSize.Level0)
{
    sptr<CameraManager> camManagerObj = CameraManager::GetInstance();
    camManagerObj->OnCameraServerAlive();
    std::shared_ptr<AppCallback> callback = std::make_shared<AppCallback>();
    camManagerObj->RegisterFoldListener(callback);
    EXPECT_EQ((camManagerObj->GetFoldListenerMap()).Size(), 1);
}

/*
 * Feature: Framework
 * Function: Test time machine dotting
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test time machine dotting
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_time_machine, TestSize.Level0)
{
    sptr<CaptureOutput> previewOutput;
    sptr<CaptureOutput> videoOutput;
    ConfigVideoSession(previewOutput, videoOutput);
    ASSERT_NE(previewOutput, nullptr);
    ASSERT_NE(videoOutput, nullptr);

    int32_t intResult = videoSession_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<VideoOutput> videoOutputTrans = ((sptr<VideoOutput>&)videoOutput);
    std::vector<VideoMetaType> supportedVideoMetaTypes = videoOutputTrans->GetSupportedVideoMetaTypes();
    ASSERT_NE(supportedVideoMetaTypes.size(), 0);

    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    ASSERT_NE(surface, nullptr);
    videoOutputTrans->AttachMetaSurface(surface, supportedVideoMetaTypes[0]);

    intResult = videoSession_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = videoOutputTrans->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = videoOutputTrans->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    sleep(WAIT_TIME_BEFORE_STOP);
}

/*
 * Feature: Framework
 * Function: Test camera rotation func.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test camera rotation func.
 */
HWTEST_F(CameraFrameworkModuleTest, test_camera_rotation_func, TestSize.Level0)
{
    auto previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);
    auto photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);
    auto videoOutput = CreateVideoOutput();
    ASSERT_NE(videoOutput, nullptr);

    session_->SetMode(SceneMode::NORMAL);
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);
    intResult = session_->AddOutput(videoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PreviewOutput> previewOutput_1 = (sptr<PreviewOutput>&)previewOutput;
    int32_t previewRotation = previewOutput_1->GetPreviewRotation(PhotoCaptureSetting::RotationConfig::Rotation_0);
    EXPECT_EQ(previewRotation, PhotoCaptureSetting::RotationConfig::Rotation_90);
    sptr<VideoOutput> videoOutput_1 = (sptr<VideoOutput>&)videoOutput;
    int32_t videoRotation = videoOutput_1->GetVideoRotation(PhotoCaptureSetting::RotationConfig::Rotation_90);
    EXPECT_EQ(videoRotation, PhotoCaptureSetting::RotationConfig::Rotation_180);
    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;
    int32_t photoRotation = photoOutput_1->GetPhotoRotation(PhotoCaptureSetting::RotationConfig::Rotation_180);
    EXPECT_EQ(photoRotation, PhotoCaptureSetting::RotationConfig::Rotation_180);
    intResult = previewOutput_1->SetPreviewRotation(previewRotation, false);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);
    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);
    sleep(WAIT_TIME_AFTER_START);
    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test lcd flash
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test lcd flash
 */
HWTEST_F(CameraFrameworkModuleTest, test_lcd_flash01, TestSize.Level0)
{
    session_ = manager_->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(session_, nullptr);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isSupported = session_->IsLcdFlashSupported();
    EXPECT_EQ(isSupported, false);
}

/*
 * Feature: Framework
 * Function: Test lcd flash
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test lcd flash
 */
HWTEST_F(CameraFrameworkModuleTest, test_lcd_flash02, TestSize.Level0)
{
    if (session_) {
        session_->Release();
        session_.clear();
        input_->Close();
    }
    session_ = manager_->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(session_, nullptr);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    input_ = manager_->CreateCameraInput(cameras_[1]);
    ASSERT_NE(input_, nullptr);
    EXPECT_EQ(input_->Open(), 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isSupported = session_->IsLcdFlashSupported();
    EXPECT_EQ(isSupported, true);

    if (isSupported) {
        session_->SetLcdFlashStatusCallback(std::make_shared<AppCallback>());
        session_->LockForControl();
        intResult = session_->EnableLcdFlashDetection(true);
        session_->UnlockForControl();
        EXPECT_EQ(intResult, 0);
    }

    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test tripod
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test tripod
 */
HWTEST_F(CameraFrameworkModuleTest, test_tripod_01, TestSize.Level0)
{
    session_ = manager_->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(session_, nullptr);

    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isSupported = session_->IsTripodDetectionSupported();
    EXPECT_EQ(isSupported, true);

    if (isSupported) {
        session_->SetFeatureDetectionStatusCallback(std::make_shared<AppCallback>());
        session_->EnableFeature(FEATURE_TRIPOD_DETECTION, true);
        EXPECT_EQ(intResult, 0);
    }
    intResult = session_->Start();
    EXPECT_EQ(intResult, 0);

    sleep(WAIT_TIME_AFTER_START);

    intResult = session_->Stop();
    EXPECT_EQ(intResult, 0);
}

/*
 * Feature: Framework
 * Function: Test cloud enhance
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: test CloudImageEnhance enable
 */
HWTEST_F(CameraFrameworkModuleTest, test_cloud_image_enhance_enable, TestSize.Level0)
{
    sptr<CaptureSession> camSession = manager_->CreateCaptureSession();
    ASSERT_NE(camSession, nullptr);

    bool isEnabled = false;
    int32_t intResult = camSession->EnableAutoCloudImageEnhancement(isEnabled);
    EXPECT_EQ(intResult, 0);

    intResult = camSession->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = (sptr<CaptureInput>&)input_;
    ASSERT_NE(input, nullptr);

    intResult = camSession->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = camSession->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = camSession->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    sptr<PhotoOutput> photoOutput_1 = (sptr<PhotoOutput>&)photoOutput;

    intResult = camSession->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isAutoCloudImageEnhanceSupported;
    intResult = photoOutput_1->IsAutoCloudImageEnhancementSupported(isAutoCloudImageEnhanceSupported);
    EXPECT_EQ(isAutoCloudImageEnhanceSupported, true);

    intResult = photoOutput_1->EnableAutoCloudImageEnhancement(isEnabled);
    EXPECT_EQ(intResult, 0);

    intResult = photoOutput_1->Release();
    EXPECT_EQ(intResult, 0);

    intResult = photoOutput_1->IsAutoCloudImageEnhancementSupported(isAutoCloudImageEnhanceSupported);
    EXPECT_EQ(intResult, 7400201);

    intResult = photoOutput_1->EnableAutoCloudImageEnhancement(isEnabled);
    EXPECT_EQ(intResult, 7400201);
}

/*
 * Feature: Framework
 * Function: Test abnormal branches Requirements for Meta Detection Types
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test abnormal branches Requirements for Meta Detection Types
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_meta_abnormal, TestSize.Level0)
{
    SetHapToken();
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;
    std::vector<MetadataObjectType> typeToAdd = { MetadataObjectType::FACE, MetadataObjectType::HUMAN_BODY,
                                                  MetadataObjectType::CAT_FACE };
    intResult = metaOutput->AddMetadataObjectTypes(typeToAdd);
    EXPECT_EQ(intResult, CameraErrorCode::SESSION_NOT_CONFIG);

    intResult = metaOutput->RemoveMetadataObjectTypes(std::vector<MetadataObjectType> {MetadataObjectType::CAT_FACE});
    EXPECT_EQ(intResult, CameraErrorCode::SESSION_NOT_CONFIG);
    SetNativeToken();
}

/*
 * Feature: Framework
 * Function: Test Requirements for Meta Detection Types
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Requirements for Meta Detection Types
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_meta, TestSize.Level0)
{
    SetHapToken();
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;
    std::vector<MetadataObjectType> typeToAdd = { MetadataObjectType::FACE, MetadataObjectType::HUMAN_BODY,
                                                  MetadataObjectType::CAT_FACE };
    intResult = metaOutput->AddMetadataObjectTypes(typeToAdd);
    EXPECT_EQ(intResult, 0);

    intResult = metaOutput->RemoveMetadataObjectTypes(std::vector<MetadataObjectType> {MetadataObjectType::CAT_FACE});
    EXPECT_EQ(intResult, 0);

    intResult = metaOutput->Start();
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<MetadataObjectCallback> metadataObjectCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataObjectCallback);
    std::shared_ptr<MetadataStateCallback> metadataStateCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataStateCallback);

    intResult = metaOutput->Stop();
    EXPECT_EQ(intResult, 0);
    SetNativeToken();
}

/*
 * Feature: Framework
 * Function: Test callback for Meta Detection Types
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test callback for Meta Detection Types
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_moduletest_meta_callback, TestSize.Level0)
{
    SetHapToken();
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> metadatOutput = manager_->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    intResult = session_->AddOutput(metadatOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<MetadataOutput> metaOutput = (sptr<MetadataOutput>&)metadatOutput;
    std::vector<MetadataObjectType> typeToAdd = { MetadataObjectType::FACE, MetadataObjectType::HUMAN_BODY,
                                                  MetadataObjectType::CAT_FACE };
    intResult = metaOutput->AddMetadataObjectTypes(typeToAdd);
    EXPECT_EQ(intResult, 0);

    intResult = metaOutput->RemoveMetadataObjectTypes(std::vector<MetadataObjectType> {MetadataObjectType::CAT_FACE});
    EXPECT_EQ(intResult, 0);

    std::shared_ptr<MetadataObjectCallback> metadataObjectCallback = std::make_shared<AppMetadataCallback>();
    metaOutput->SetCallback(metadataObjectCallback);

    sptr<HStreamMetadataCallbackImpl> cameraMetadataCallback_ =
        new HStreamMetadataCallbackImpl(metaOutput.GetRefPtr());

    const int32_t defaultItems = 10;
    const int32_t defaultDataLength = 100;

    std::shared_ptr<OHOS::Camera::CameraMetadata> mockMetaFromHal =
        std::make_shared<OHOS::Camera::CameraMetadata>(defaultItems, defaultDataLength);
    std::vector<uint8_t> mockHumanFaceTagfromHal = {
        0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<uint8_t> mockCatFaceTagFromHal = {2, 2, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<uint8_t> mockDogFaceTagFromHal = {4, 1, 0, 0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool status = mockMetaFromHal->addEntry(OHOS_STATISTICS_DETECT_HUMAN_FACE_INFOS, mockHumanFaceTagfromHal.data(), mockHumanFaceTagfromHal.size());
    ASSERT_TRUE(status);
    status = mockMetaFromHal->addEntry(OHOS_STATISTICS_DETECT_CAT_FACE_INFOS, mockCatFaceTagFromHal.data(), mockCatFaceTagFromHal.size());
    ASSERT_TRUE(status);
    status = mockMetaFromHal->addEntry(OHOS_STATISTICS_DETECT_DOG_FACE_INFOS, mockDogFaceTagFromHal.data(), mockDogFaceTagFromHal.size());
    ASSERT_TRUE(status);

    intResult = cameraMetadataCallback_->OnMetadataResult(0, mockMetaFromHal);
    EXPECT_EQ(intResult, 0);
    SetNativeToken();
}
/*
 * Feature: Framework
 * Function: Test setMovingPhotoVideoCodecType Methodology
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test setMovingPhotoVideoCodecType Methodology
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_module_set_moving_photo_video_codec_type, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    int32_t videoCodecType = 0;
    intResult = ((sptr<PhotoOutput>&)photoOutput)->SetMovingPhotoVideoCodecType(videoCodecType);
    EXPECT_EQ(intResult, 0);
}
/*
 * Feature: Framework
 * Function: Test EnableRawDelivery Methodology
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test EnableRawDelivery Methodology
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_module_enable_raw_delivery, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool enable = true;
    intResult = ((sptr<PhotoOutput>&)photoOutput)->EnableRawDelivery(enable);
    EXPECT_EQ(intResult, 0);
}
/*
 * Feature: Framework
 * Function: Test non professional IsRawDeliverySupported Methodology
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test non professional IsRawDeliverySupported Methodology
 */
HWTEST_F(CameraFrameworkModuleTest, camera_framework_module_is_raw_delivery_supported, TestSize.Level0)
{
    int32_t intResult = session_->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session_->AddInput(input_);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session_->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    intResult = session_->AddOutput(photoOutput);
    EXPECT_EQ(intResult, 0);

    intResult = session_->CommitConfig();
    EXPECT_EQ(intResult, 0);

    bool isSupported;
    intResult = ((sptr<PhotoOutput>&)photoOutput)->IsRawDeliverySupported(isSupported);
    EXPECT_EQ(intResult, CAMERA_OK);
    EXPECT_EQ(isSupported, false);
}
} // namespace CameraStandard
} // namespace OHOS
