/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "video_output_unittest.h"

#include "gtest/gtest.h"
#include <cstdint>
#include <vector>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "camera_log.h"
#include "camera_manager.h"
#include "camera_util.h"
#include "capture_output.h"
#include "capture_session.h"
#include "gmock/gmock.h"
#include "input/camera_input.h"
#include "ipc_skeleton.h"
#include "nativetoken_kit.h"
#include "surface.h"
#include "test_common.h"
#include "token_setproc.h"
#include "os_account_manager.h"

using namespace testing::ext;
using ::testing::A;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Camera::V1_1;

void CameraVedioOutputUnit::SetUpTestCase(void) {}

void CameraVedioOutputUnit::TearDownTestCase(void) {}

void CameraVedioOutputUnit::SetUp()
{
    NativeAuthorization();
    cameraManager_ = CameraManager::GetInstance();
}

void CameraVedioOutputUnit::TearDown()
{
    cameraManager_ = nullptr;
}

void CameraVedioOutputUnit::NativeAuthorization()
{
    const char *perms[2];
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
    tokenId_ = GetAccessTokenId(&infoInstance);
    uid_ = IPCSkeleton::GetCallingUid();
    AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(uid_, userId_);
    MEDIA_DEBUG_LOG("CameraVedioOutputUnit::NativeAuthorization g_uid:%{public}d", uid_);
    SetSelfTokenID(tokenId_);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

/*
 * Feature: Framework
 * Function: Test videooutput when videoOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test videooutput when videoOutput
 */
HWTEST_F(CameraVedioOutputUnit, video_output_unittest_001, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    ASSERT_FALSE(cameras.empty());
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    camInput->GetCameraDevice()->Open();

    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = VIDEO_DEFAULT_WIDTH;
    videoSize.height = VIDEO_DEFAULT_HEIGHT;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile profile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager_->CreateVideoOutput(profile, surface);
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> output = video;
    sptr<CaptureSession> session = cameraManager_->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();
    session->AddInput(input);
    session->AddOutput(output);
    session->CommitConfig();
    session->Start();

    video->stream_ = nullptr;
    int32_t ret = video->CreateStream();
    EXPECT_EQ(ret, CameraErrorCode::SUCCESS);
    ret = video->Start();
    EXPECT_NE(ret, CameraErrorCode::CONFLICT_CAMERA);
    ret = video->Pause();
    EXPECT_EQ(ret, CAMERA_INVALID_STATE);
    ret = video->Resume();
    EXPECT_EQ(ret, CameraErrorCode::SERVICE_FATL_ERROR);
    ret = video->Stop();
    EXPECT_EQ(ret, CameraErrorCode::SERVICE_FATL_ERROR);
    ret = video->Release();
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(input->Release(), 0);
    session->Stop();
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test start in videoOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test start in videoOutput
 */
HWTEST_F(CameraVedioOutputUnit, video_output_unittest_002, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    ASSERT_FALSE(cameras.empty());
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    camInput->GetCameraDevice()->Open();

    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = VIDEO_DEFAULT_WIDTH;
    videoSize.height = VIDEO_DEFAULT_HEIGHT;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile profile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager_->CreateVideoOutput(profile, surface);
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> output = video;
    sptr<CaptureSession> session = cameraManager_->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();
    session->AddInput(input);
    session->AddOutput(output);
    session->CommitConfig();
    session->Start();

    video->stream_ = nullptr;
    int32_t ret = video->CreateStream();
    EXPECT_EQ(ret, CameraErrorCode::SUCCESS);

    wptr<IStreamCommon> stream_;
    sptr<IStreamCommon> stream = stream_.promote();
    video->SetStream(stream);

    ret = video->Start();
    EXPECT_NE(ret, CameraErrorCode::CONFLICT_CAMERA);

    int32_t minFrameRate = 200;
    int32_t maxFrameRate = 300;
    video->SetFrameRateRange(minFrameRate, maxFrameRate);
    ret = video->Start();
    EXPECT_NE(ret, CameraErrorCode::CONFLICT_CAMERA);

    minFrameRate = 100;
    video->SetFrameRateRange(minFrameRate, maxFrameRate);
    ret = video->Start();
    EXPECT_NE(ret, CameraErrorCode::CONFLICT_CAMERA);

    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test stop in videoOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test stop in videoOutput
 */
HWTEST_F(CameraVedioOutputUnit, video_output_unittest_003, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    ASSERT_FALSE(cameras.empty());
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    camInput->GetCameraDevice()->Open();

    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = VIDEO_DEFAULT_WIDTH;
    videoSize.height = VIDEO_DEFAULT_HEIGHT;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile profile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager_->CreateVideoOutput(profile, surface);
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> output = video;
    sptr<CaptureSession> session = cameraManager_->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();
    session->AddInput(input);
    session->AddOutput(output);
    session->CommitConfig();
    session->Start();

    video->stream_ = nullptr;
    int32_t ret = video->CreateStream();
    EXPECT_EQ(ret, CameraErrorCode::SUCCESS);

    wptr<IStreamCommon> stream_;
    sptr<IStreamCommon> stream = stream_.promote();
    video->SetStream(stream);

    ret = video->Start();
    EXPECT_NE(ret, CameraErrorCode::CONFLICT_CAMERA);
    ret = video->Stop();
    EXPECT_EQ(ret, CameraErrorCode::SERVICE_FATL_ERROR);

    int32_t minFrameRate = 200;
    int32_t maxFrameRate = 300;
    video->SetFrameRateRange(minFrameRate, maxFrameRate);
    ret = video->Stop();
    EXPECT_EQ(ret, CameraErrorCode::SERVICE_FATL_ERROR);

    minFrameRate = 100;
    video->SetFrameRateRange(minFrameRate, maxFrameRate);
    ret = video->Stop();
    EXPECT_EQ(ret, CameraErrorCode::SERVICE_FATL_ERROR);

    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test IsMirrorSupported in videoOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test IsMirrorSupported in videoOutput
 */
HWTEST_F(CameraVedioOutputUnit, video_output_unittest_004, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    ASSERT_FALSE(cameras.empty());
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    camInput->GetCameraDevice()->Open();

    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = VIDEO_DEFAULT_WIDTH;
    videoSize.height = VIDEO_DEFAULT_HEIGHT;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile profile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager_->CreateVideoOutput(profile, surface);
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> output = video;
    sptr<CaptureSession> session = cameraManager_->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();
    session->AddInput(input);
    session->AddOutput(output);
    session->CommitConfig();
    session->Start();

    bool ret = video->IsMirrorSupported();
    EXPECT_EQ(ret, false);

    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test GetSupportedVideoMetaTypes in videoOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetSupportedVideoMetaTypes in videoOutput
 */
HWTEST_F(CameraVedioOutputUnit, video_output_unittest_005, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    ASSERT_FALSE(cameras.empty());
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    camInput->GetCameraDevice()->Open();

    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = VIDEO_DEFAULT_WIDTH;
    videoSize.height = VIDEO_DEFAULT_HEIGHT;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile profile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager_->CreateVideoOutput(profile, surface);
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> output = video;
    sptr<CaptureSession> session = cameraManager_->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    std::vector<VideoMetaType> supportedVideoMetaTypes = video->GetSupportedVideoMetaTypes();
    EXPECT_EQ(supportedVideoMetaTypes.size(), 0);

    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test CameraServerDied in videoOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CameraServerDied in videoOutput
 */
HWTEST_F(CameraVedioOutputUnit, video_output_unittest_006, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    ASSERT_FALSE(cameras.empty());
    sptr<CaptureInput> input = cameraManager_->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    camInput->GetCameraDevice()->Open();

    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = VIDEO_DEFAULT_WIDTH;
    videoSize.height = VIDEO_DEFAULT_HEIGHT;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile profile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager_->CreateVideoOutput(profile, surface);
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> output = video;
    sptr<CaptureSession> session = cameraManager_->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();
    session->AddInput(input);
    session->AddOutput(output);
    session->CommitConfig();
    session->Start();

    pid_t pid = 0;
    video->CameraServerDied(pid);
    video->appCallback_ = nullptr;
    video->CameraServerDied(pid);

    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

}
}
