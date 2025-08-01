/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#include <cstdint>
#include <vector>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "camera_manager_for_sys.h"
#include "camera_util.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "metadata_utils.h"
#include "nativetoken_kit.h"
#include "night_session_unittest.h"
#include "os_account_manager.h"
#include "sketch_wrapper.h"
#include "surface.h"
#include "test_common.h"
#include "test_token.h"
#include "token_setproc.h"

using namespace testing::ext;

namespace OHOS {
namespace CameraStandard {

sptr<CaptureOutput> CameraNightSessionUnit::CreatePreviewOutput()
{
    previewProfile_ = {};
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetCameraDeviceListFromServer();
    if (!cameraManager_ || cameras.empty()) {
        return nullptr;
    }
    preIsSupportedNighitmode_ = false;
    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager_->GetSupportedModes(camDevice);
        if (find(modes.begin(), modes.end(), SceneMode::NIGHT) != modes.end()) {
            preIsSupportedNighitmode_ = true;
        }

        if (!preIsSupportedNighitmode_) {
            continue;
        }

        auto outputCapability = cameraManager_->GetSupportedOutputCapability(camDevice, static_cast<int32_t>(NIGHT));
        if (!outputCapability) {
            return nullptr;
        }

        previewProfile_ = outputCapability->GetPreviewProfiles();
        if (previewProfile_.empty()) {
            return nullptr;
        }

        sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
        if (surface == nullptr) {
            return nullptr;
        }
        return cameraManager_->CreatePreviewOutput(previewProfile_[0], surface);
    }
    return nullptr;
}

sptr<CaptureOutput> CameraNightSessionUnit::CreatePhotoOutput()
{
    photoProfile_ = {};
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetCameraDeviceListFromServer();
    if (!cameraManager_ || cameras.empty()) {
        return nullptr;
    }
    phoIsSupportedNighitmode_ = false;
    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager_->GetSupportedModes(camDevice);
        if (find(modes.begin(), modes.end(), SceneMode::NIGHT) != modes.end()) {
            phoIsSupportedNighitmode_ = true;
        }

        if (!phoIsSupportedNighitmode_) {
            continue;
        }

        auto outputCapability = cameraManager_->GetSupportedOutputCapability(camDevice, static_cast<int32_t>(NIGHT));
        if (!outputCapability) {
            return nullptr;
        }

        photoProfile_ = outputCapability->GetPhotoProfiles();
        if (photoProfile_.empty()) {
            return nullptr;
        }

        sptr<IConsumerSurface> surface = IConsumerSurface::Create();
        if (surface == nullptr) {
            return nullptr;
        }
        sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
        return cameraManager_->CreatePhotoOutput(photoProfile_[0], surfaceProducer);
    }
    return nullptr;
}

void CameraNightSessionUnit::SetUpTestCase(void)
{
    ASSERT_TRUE(TestToken().GetAllCameraPermission());
}

void CameraNightSessionUnit::TearDownTestCase(void) {}

void CameraNightSessionUnit::SetUp()
{
    cameraManager_ = CameraManager::GetInstance();
    ASSERT_NE(cameraManager_, nullptr);
}

void CameraNightSessionUnit::TearDown()
{
    cameraManager_ = nullptr;
    MEDIA_DEBUG_LOG("CameraPanarmaSessionUnit TearDown");
}

/*
 * Feature: Framework
 * Function: Test NightSession about exposure
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test NightSession about exposure
 */
HWTEST_F(CameraNightSessionUnit, night_session_unittest_001, TestSize.Level1)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetCameraDeviceListFromServer();
    SceneMode mode = NIGHT;
    cameras[0]->supportedModes_.clear();
    cameras[0]->supportedModes_.push_back(NORMAL);
    std::vector<SceneMode> modes = cameraManager_->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
    sptr<CameraOutputCapability> ability = cameraManager_->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    if (!preIsSupportedNighitmode_ || !phoIsSupportedNighitmode_) {
        input->Close();
        GTEST_SKIP();
    }
    ASSERT_NE(preview, nullptr);
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = CameraManagerForSys::GetInstance()->CreateCaptureSessionForSys(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);

    EXPECT_EQ(nightSession->BeginConfig(), 0);
    EXPECT_EQ(nightSession->AddInput(input), 0);
    sptr<CameraDevice> info = captureSession->innerInputDevice_->GetCameraDeviceInfo();
    ASSERT_NE(info, nullptr);
    info->modePreviewProfiles_.emplace(static_cast<int32_t>(NIGHT), previewProfile_);
    info->modePhotoProfiles_.emplace(static_cast<int32_t>(NIGHT), photoProfile_);
    EXPECT_EQ(nightSession->AddOutput(preview), 0);
    EXPECT_EQ(nightSession->AddOutput(photo), 0);
    EXPECT_EQ(nightSession->CommitConfig(), 0);

    std::vector<uint32_t> exposureRange = {};
    uint32_t exposureValue = 0;
    OHOS::Camera::DeleteCameraMetadataItem(info->GetMetadata()->get(), OHOS_ABILITY_NIGHT_MODE_SUPPORTED_EXPOSURE_TIME);
    OHOS::Camera::DeleteCameraMetadataItem(info->GetMetadata()->get(), OHOS_CONTROL_MANUAL_EXPOSURE_TIME);

    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), CameraErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(nightSession->GetExposure(exposureValue), CameraErrorCode::INVALID_ARGUMENT);

    nightSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test NightSession when innerInputDevice_ and output is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test NightSession when innerInputDevice_ and output is nullptr
 */
HWTEST_F(CameraNightSessionUnit, night_session_unittest_002, TestSize.Level1)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetCameraDeviceListFromServer();
    SceneMode mode = NIGHT;
    cameras[0]->supportedModes_.clear();
    cameras[0]->supportedModes_.push_back(NORMAL);
    std::vector<SceneMode> modes = cameraManager_->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
    sptr<CameraOutputCapability> ability = cameraManager_->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    if (!preIsSupportedNighitmode_) {
        input->Close();
        GTEST_SKIP();
    }
    ASSERT_NE(preview, nullptr);
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    if (!phoIsSupportedNighitmode_) {
        input->Close();
        GTEST_SKIP();
    }
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = CameraManagerForSys::GetInstance()->CreateCaptureSessionForSys(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);

    EXPECT_EQ(nightSession->BeginConfig(), 0);
    EXPECT_EQ(nightSession->AddInput(input), 0);
    sptr<CameraDevice> info = captureSession->innerInputDevice_->GetCameraDeviceInfo();
    ASSERT_NE(info, nullptr);
    info->modePreviewProfiles_.emplace(static_cast<int32_t>(NIGHT), previewProfile_);
    info->modePhotoProfiles_.emplace(static_cast<int32_t>(NIGHT), photoProfile_);
    EXPECT_EQ(nightSession->AddOutput(preview), 0);
    EXPECT_EQ(nightSession->AddOutput(photo), 0);
    EXPECT_EQ(nightSession->CommitConfig(), 0);

    nightSession->innerInputDevice_ = nullptr;
    std::vector<uint32_t> exposureRange = {};
    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), CameraErrorCode::INVALID_ARGUMENT);
    sptr<CaptureOutput> output = nullptr;
    EXPECT_EQ(nightSession->CanAddOutput(output), false);

    nightSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test NightSession with SetExposure
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test NightSession with SetExposure
 */
HWTEST_F(CameraNightSessionUnit, night_session_unittest_003, TestSize.Level1)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetCameraDeviceListFromServer();
    SceneMode mode = NIGHT;
    cameras[0]->supportedModes_.clear();
    cameras[0]->supportedModes_.push_back(NORMAL);
    std::vector<SceneMode> modes = cameraManager_->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
    sptr<CameraOutputCapability> ability = cameraManager_->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    if (!preIsSupportedNighitmode_ || !phoIsSupportedNighitmode_) {
        input->Close();
        GTEST_SKIP();
    }
    ASSERT_NE(preview, nullptr);
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = CameraManagerForSys::GetInstance()->CreateCaptureSessionForSys(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);

    EXPECT_EQ(nightSession->BeginConfig(), 0);
    EXPECT_EQ(nightSession->AddInput(input), 0);
    sptr<CameraDevice> info = captureSession->innerInputDevice_->GetCameraDeviceInfo();
    ASSERT_NE(info, nullptr);
    info->modePreviewProfiles_.emplace(static_cast<int32_t>(NIGHT), previewProfile_);
    info->modePhotoProfiles_.emplace(static_cast<int32_t>(NIGHT), photoProfile_);
    EXPECT_EQ(nightSession->AddOutput(preview), 0);
    EXPECT_EQ(nightSession->AddOutput(photo), 0);
    EXPECT_EQ(nightSession->CommitConfig(), 0);

    std::vector<uint32_t> exposureRange = {};
    uint32_t exposureValue = 0;
    OHOS::Camera::DeleteCameraMetadataItem(info->GetMetadata()->get(), OHOS_ABILITY_NIGHT_MODE_SUPPORTED_EXPOSURE_TIME);
    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), CameraErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(nightSession->SetExposure(exposureValue), CameraErrorCode::SUCCESS);

    nightSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test NightSession with SetExposure normal branches
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test NightSession with SetExposure normal branches
 */
HWTEST_F(CameraNightSessionUnit, night_session_unittest_004, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetCameraDeviceListFromServer();
    SceneMode mode = NIGHT;
    cameras[0]->supportedModes_.clear();
    cameras[0]->supportedModes_.push_back(NORMAL);
    std::vector<SceneMode> modes = cameraManager_->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
    sptr<CameraOutputCapability> ability = cameraManager_->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    if (!preIsSupportedNighitmode_) {
        input->Close();
        GTEST_SKIP();
    }
    ASSERT_NE(preview, nullptr);
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    if (!phoIsSupportedNighitmode_) {
        input->Close();
        GTEST_SKIP();
    }
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = CameraManagerForSys::GetInstance()->CreateCaptureSessionForSys(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);

    EXPECT_EQ(nightSession->BeginConfig(), 0);
    EXPECT_EQ(nightSession->AddInput(input), 0);
    sptr<CameraDevice> info = captureSession->innerInputDevice_->GetCameraDeviceInfo();
    ASSERT_NE(info, nullptr);
    info->modePreviewProfiles_.emplace(static_cast<int32_t>(NIGHT), previewProfile_);
    info->modePhotoProfiles_.emplace(static_cast<int32_t>(NIGHT), photoProfile_);
    EXPECT_EQ(nightSession->AddOutput(preview), 0);
    EXPECT_EQ(nightSession->AddOutput(photo), 0);
    EXPECT_EQ(nightSession->CommitConfig(), 0);

    std::vector<uint32_t> exposureRange = {};
    uint32_t exposureValue = 0;
    int32_t count = 1;
    nightSession->LockForControl();
    OHOS::Camera::DeleteCameraMetadataItem(info->GetMetadata()->get(), OHOS_ABILITY_NIGHT_MODE_SUPPORTED_EXPOSURE_TIME);

    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), CameraErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(nightSession->SetExposure(exposureValue), CameraErrorCode::OPERATION_NOT_ALLOWED);

    OHOS::Camera::DeleteCameraMetadataItem(info->GetMetadata()->get(), OHOS_CONTROL_MANUAL_EXPOSURE_TIME);
    EXPECT_EQ(nightSession->SetExposure(exposureValue), CameraErrorCode::OPERATION_NOT_ALLOWED);
    nightSession->changedMetadata_->addEntry(OHOS_CONTROL_MANUAL_EXPOSURE_TIME, &exposureValue, count);
    EXPECT_EQ(nightSession->SetExposure(exposureValue), CameraErrorCode::OPERATION_NOT_ALLOWED);
    nightSession->UnlockForControl();

    nightSession->Release();
    input->Close();
}
}
}