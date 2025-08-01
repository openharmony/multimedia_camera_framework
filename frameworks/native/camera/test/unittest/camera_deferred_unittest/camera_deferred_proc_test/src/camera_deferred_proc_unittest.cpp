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

#include "camera_deferred_proc_unittest.h"
#include "deferred_video_proc_session.h"

#include "dp_log.h"
#include "picture_proxy.h"
#include "pixel_map.h"

#include "access_token.h"
#include "accesstoken_kit.h"

#include "gtest/gtest.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "nativetoken_kit.h"
#include "os_account_manager.h"

#include "token_setproc.h"
#include "picture_interface.h"
#include "test_token.h"

namespace OHOS {
namespace CameraStandard {
using namespace testing::ext;
using namespace DeferredProcessing;

constexpr int32_t SIZE_WIDTH = 2;
constexpr int32_t SIZE_HEIGHT = 3;
constexpr int32_t BUFFER_LENGTH = 8;
constexpr int VIDEO_REQUEST_FD_ID = 1;

class TestDeferredPhotoProcSessionCallback : public IDeferredPhotoProcSessionCallback {
public:
    void OnProcessImageDone(const std::string &imageId, std::shared_ptr<PictureIntf> picture,
        uint32_t cloudImageEnhanceFlag) {}
    void OnDeliveryLowQualityImage(const std::string &imageId, std::shared_ptr<PictureIntf> picture) {}
    void OnProcessImageDone(const std::string& imageId, const uint8_t* addr, const long bytes,
        uint32_t cloudImageEnhanceFlag) {}
    void OnError(const std::string& imageId, const DpsErrorCode errorCode) {}
    void OnStateChanged(const DpsStatusCode status) {}
};

class TestDeferredVideoProcSessionCallback : public IDeferredVideoProcSessionCallback {
public:
    void OnProcessVideoDone(const std::string& videoId, const sptr<IPCFileDescriptor> ipcFd) {}
    void OnError(const std::string& videoId, const DpsErrorCode errorCode) {}
    void OnStateChanged(const DpsStatusCode status) {}
};

std::shared_ptr<PictureIntf> GetPictureIntfInstance()
{
    auto pictureProxy = PictureProxy::CreatePictureProxy();
    DP_CHECK_ERROR_PRINT_LOG(pictureProxy == nullptr || pictureProxy.use_count() != 1,
        "pictureProxy use count is not 1");
    return pictureProxy;
}

void DeferredProcUnitTest::SetUpTestCase(void)
{
    ASSERT_TRUE(TestToken().GetAllCameraPermission());
}

void DeferredProcUnitTest::TearDownTestCase(void)
{
    DP_DEBUG_LOG("DeferredProcUnitTest::TearDownTestCase started!");
}

void DeferredProcUnitTest::SetUp()
{
    const uint32_t color[BUFFER_LENGTH] = { 0x80, 0x02, 0x04, 0x08, 0x40, 0x02, 0x04, 0x08 };
    Media::InitializationOptions options;
    options.size.width = SIZE_WIDTH;
    options.size.height = SIZE_HEIGHT;
    options.srcPixelFormat = Media::PixelFormat::UNKNOWN;
    options.pixelFormat = Media::PixelFormat::UNKNOWN;
    options.alphaType = Media::AlphaType::IMAGE_ALPHA_TYPE_OPAQUE;
    std::shared_ptr<Media::PixelMap> pixelmap = Media::PixelMap::Create(color, BUFFER_LENGTH, options);
    EXPECT_NE(pixelmap, nullptr);
    auto picture = GetPictureIntfInstance();
    if (picture == nullptr) {
        picture_ = nullptr;
    } else {
        picture_ = picture;
    }
}

void DeferredProcUnitTest::TearDown()
{
    DP_DEBUG_LOG("DeferredProcUnitTest::TearDown started!");
}

/*
 * Feature: Framework
 * Function: Test is that the class calls the function function correctly and functions properly.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test is that the class calls the function function correctly and functions properly.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_001, TestSize.Level0)
{
    sptr<DeferredPhotoProcSession> deferredProcSession =
        new(std::nothrow) DeferredPhotoProcSession(userId_, std::make_shared<TestDeferredPhotoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    deferredProcSession->BeginSynchronize();

    std::string imageId = "testImageId";
    DpsMetadata metadata;
    bool discardable = true;
    deferredProcSession->AddImage(imageId, metadata, discardable);

    bool restorable = true;
    deferredProcSession->RemoveImage(imageId, restorable);

    deferredProcSession->RestoreImage(imageId);

    std::string appName = "com.cameraFwk.ut";
    deferredProcSession->ProcessImage(appName, imageId);

    bool ret = deferredProcSession->CancelProcessImage(imageId);
    EXPECT_TRUE(ret);

    deferredProcSession->EndSynchronize();

    pid_t pid = 0;
    deferredProcSession->CameraServerDied(pid);

    auto callback = deferredProcSession->GetCallback();
    EXPECT_NE(callback, nullptr);
}

/*
 * Feature: Framework
 * Function: Test exception constructs the function that calls the function properly.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test exception constructs the function that calls the function properly.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_002, TestSize.Level0)
{
    sptr<DeferredPhotoProcSession> deferredProcSession =
        new(std::nothrow) DeferredPhotoProcSession(userId_, std::make_shared<TestDeferredPhotoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    EXPECT_EQ(deferredProcSession->remoteSession_, nullptr);
    deferredProcSession->BeginSynchronize();

    std::string imageId = "testImageId";
    DpsMetadata metadata;
    bool discardable = true;
    deferredProcSession->AddImage(imageId, metadata, discardable);

    bool restorable = true;
    deferredProcSession->RemoveImage(imageId, restorable);

    deferredProcSession->RestoreImage(imageId);

    std::string appName = "com.cameraFwk.ut";
    deferredProcSession->ProcessImage(appName, imageId);

    bool ret = deferredProcSession->CancelProcessImage(imageId);
    EXPECT_TRUE(!ret);

    deferredProcSession->EndSynchronize();

    pid_t pid = 0;
    deferredProcSession->CameraServerDied(pid);
    EXPECT_NE(deferredProcSession->deathRecipient_, nullptr);
}

/*
 * Feature: Framework
 * Function: Test the construction class calls the listener function, and the function functions normally.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the construction class calls the listener function, and the function functions normally.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_003, TestSize.Level0)
{
    sptr<DeferredPhotoProcSession> deferredProcSession =
        new(std::nothrow) DeferredPhotoProcSession(userId_, std::make_shared<TestDeferredPhotoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    sptr<DeferredPhotoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredPhotoProcessingSessionCallback(deferredProcSession);
    ASSERT_NE(remoteCallback, nullptr);

    std::string imageId = "testImageId";
    bool restorable = true;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessImageDone(imageId, nullptr, restorable);
    EXPECT_EQ(ret, 0);

    ret = remoteCallback->OnProcessImageDone(imageId, picture_, restorable);
    EXPECT_EQ(ret, 0);

    sptr<IPCFileDescriptor> ipcFd = sptr<IPCFileDescriptor>::MakeSptr(dup(VIDEO_REQUEST_FD_ID));
    EXPECT_NE(ipcFd, nullptr);
    long bytes = sizeof(ipcFd);
    ret = remoteCallback->OnProcessImageDone(imageId, ipcFd, bytes, restorable);
    EXPECT_EQ(ret, 0);

    ret = remoteCallback->OnDeliveryLowQualityImage(imageId, picture_);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::ErrorCode errorCode = DeferredProcessing::ErrorCode::ERROR_SESSION_SYNC_NEEDED;
    ret = remoteCallback->OnError(imageId, errorCode);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::StatusCode status = DeferredProcessing::StatusCode::SESSION_STATE_IDLE;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: When the callback is not implemented, the callback function is called normally.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: When the callback is not implemented, the callback function is called normally.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_004, TestSize.Level0)
{
    sptr<DeferredPhotoProcSession> deferredProcSession = new(std::nothrow) DeferredPhotoProcSession(userId_, nullptr);
    ASSERT_NE(deferredProcSession, nullptr);
    sptr<DeferredPhotoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredPhotoProcessingSessionCallback(deferredProcSession);
    ASSERT_NE(remoteCallback, nullptr);

    std::string imageId = "testImageId";
    bool restorable = true;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessImageDone(imageId, picture_, restorable);
    EXPECT_EQ(ret, 0);

    sptr<IPCFileDescriptor> ipcFd = sptr<IPCFileDescriptor>::MakeSptr(dup(VIDEO_REQUEST_FD_ID));
    EXPECT_NE(ipcFd, nullptr);
    long bytes = sizeof(ipcFd);
    ret = remoteCallback->OnProcessImageDone(imageId, ipcFd, bytes, restorable);
    EXPECT_EQ(ret, 0);

    ret = remoteCallback->OnDeliveryLowQualityImage(imageId, nullptr);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::ErrorCode errorCode = DeferredProcessing::ErrorCode::ERROR_SESSION_SYNC_NEEDED;
    ret = remoteCallback->OnError(imageId, errorCode);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::StatusCode status = DeferredProcessing::StatusCode::SESSION_STATE_IDLE;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: When the test does not implement the callback, the function call is normal.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: When the test does not implement the callback, the function call is normal.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_005, TestSize.Level0)
{
    sptr<DeferredPhotoProcSession> deferredProcSession =
        new(std::nothrow) DeferredPhotoProcSession(userId_, std::make_shared<TestDeferredPhotoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    sptr<DeferredPhotoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredPhotoProcessingSessionCallback();
    ASSERT_NE(remoteCallback, nullptr);

    std::string imageId = "testImageId";
    bool restorable = true;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessImageDone(imageId, picture_, restorable);
    EXPECT_EQ(ret, 0);

    sptr<IPCFileDescriptor> ipcFd = sptr<IPCFileDescriptor>::MakeSptr(dup(VIDEO_REQUEST_FD_ID));
    EXPECT_NE(ipcFd, nullptr);
    long bytes = sizeof(ipcFd);
    ret = remoteCallback->OnProcessImageDone(imageId, ipcFd, bytes, restorable);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::ErrorCode errorCode = DeferredProcessing::ErrorCode::ERROR_SESSION_SYNC_NEEDED;
    ret = remoteCallback->OnError(imageId, errorCode);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::StatusCode status = DeferredProcessing::StatusCode::SESSION_STATE_IDLE;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: The test function is returned abnormally.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: The test function is returned abnormally.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_006, TestSize.Level0)
{
    sptr<DeferredPhotoProcSession> deferredProcSession = new(std::nothrow) DeferredPhotoProcSession(userId_, nullptr);
    ASSERT_NE(deferredProcSession, nullptr);
    sptr<DeferredPhotoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredPhotoProcessingSessionCallback();
    ASSERT_NE(remoteCallback, nullptr);

    std::string imageId = "testImageId";
    bool restorable = true;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessImageDone(imageId, picture_, restorable);
    EXPECT_EQ(ret, 0);

    sptr<IPCFileDescriptor> ipcFd = sptr<IPCFileDescriptor>::MakeSptr(dup(VIDEO_REQUEST_FD_ID));
    EXPECT_NE(ipcFd, nullptr);
    long bytes = sizeof(ipcFd);
    ret = remoteCallback->OnProcessImageDone(imageId, ipcFd, bytes, restorable);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::ErrorCode errorCode = DeferredProcessing::ErrorCode::ERROR_SESSION_SYNC_NEEDED;
    ret = remoteCallback->OnError(imageId, errorCode);
    EXPECT_EQ(ret, 0);

    DeferredProcessing::StatusCode status = DeferredProcessing::StatusCode::SESSION_STATE_IDLE;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test is that the class calls the function function correctly and functions properly.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test is that the class calls the function function correctly and functions properly.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_007, TestSize.Level0)
{
    sptr<DeferredVideoProcSession> deferredProcSession =
        new(std::nothrow) DeferredVideoProcSession(userId_, std::make_shared<TestDeferredVideoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    deferredProcSession->BeginSynchronize();

    std::string videoId = "testVideoId";
    sptr<IPCFileDescriptor> srcFd;
    const sptr<IPCFileDescriptor> dstFd;
    deferredProcSession->AddVideo(videoId, srcFd, dstFd);

    bool restorable = true;
    deferredProcSession->RemoveVideo(videoId, restorable);

    deferredProcSession->RestoreVideo(videoId);

    deferredProcSession->EndSynchronize();

    pid_t pid = 0;
    deferredProcSession->CameraServerDied(pid);

    auto callback = deferredProcSession->GetCallback();
    EXPECT_NE(callback, nullptr);
}

/*
 * Feature: Framework
 * Function: Test exception constructs the function that calls the function properly.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test exception constructs the function that calls the function properly.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_008, TestSize.Level0)
{
    sptr<DeferredVideoProcSession> deferredProcSession =
        new(std::nothrow) DeferredVideoProcSession(userId_, std::make_shared<TestDeferredVideoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    EXPECT_EQ(deferredProcSession->remoteSession_, nullptr);

    deferredProcSession->BeginSynchronize();

    std::string videoId = "testVideoId";
    sptr<IPCFileDescriptor> srcFd;
    const sptr<IPCFileDescriptor> dstFd;
    deferredProcSession->AddVideo(videoId, srcFd, dstFd);

    bool restorable = true;
    deferredProcSession->RemoveVideo(videoId, restorable);

    deferredProcSession->RestoreVideo(videoId);

    deferredProcSession->EndSynchronize();

    pid_t pid = 0;
    deferredProcSession->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test the construction class calls the listener function, and the function functions normally.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the construction class calls the listener function, and the function functions normally.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_009, TestSize.Level0)
{
    sptr<DeferredVideoProcSession> deferredProcSession =
        new(std::nothrow) DeferredVideoProcSession(userId_, std::make_shared<TestDeferredVideoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);

    sptr<DeferredVideoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredVideoProcessingSessionCallback(deferredProcSession);
    ASSERT_NE(remoteCallback, nullptr);
    std::string videoId = "testVideoId";
    sptr<IPCFileDescriptor> ipcFileDescriptor;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessVideoDone(videoId, ipcFileDescriptor);
    EXPECT_EQ(ret, 0);

    int32_t errorCode = 0;
    ret = remoteCallback->OnError(videoId, errorCode);
    EXPECT_EQ(ret, 0);

    int32_t status = 0;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: When the callback is not implemented, the callback function is called normally.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: When the callback is not implemented, the callback function is called normally.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_010, TestSize.Level0)
{
    sptr<DeferredVideoProcSession> deferredProcSession =
        new(std::nothrow) DeferredVideoProcSession(userId_, std::make_shared<TestDeferredVideoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    sptr<DeferredVideoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredVideoProcessingSessionCallback();
    ASSERT_NE(remoteCallback, nullptr);
    std::string videoId = "testVideoId";
    sptr<IPCFileDescriptor> ipcFileDescriptor;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessVideoDone(videoId, ipcFileDescriptor);
    EXPECT_EQ(ret, 0);

    int32_t errorCode = 0;
    ret = remoteCallback->OnError(videoId, errorCode);
    EXPECT_EQ(ret, 0);

    int32_t status = 0;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: When the test does not implement the callback, the function call is normal.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: When the test does not implement the callback, the function call is normal.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_011, TestSize.Level0)
{
    sptr<DeferredVideoProcSession> deferredProcSession = new(std::nothrow) DeferredVideoProcSession(userId_, nullptr);
    ASSERT_NE(deferredProcSession, nullptr);

    sptr<DeferredVideoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredVideoProcessingSessionCallback(deferredProcSession);
    ASSERT_NE(remoteCallback, nullptr);
    std::string videoId = "testVideoId";
    sptr<IPCFileDescriptor> ipcFileDescriptor;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessVideoDone(videoId, ipcFileDescriptor);
    EXPECT_EQ(ret, 0);

    int32_t errorCode = 0;
    ret = remoteCallback->OnError(videoId, errorCode);
    EXPECT_EQ(ret, 0);

    int32_t status = 0;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: The test function is returned abnormally.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: The test function is returned abnormally.
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_012, TestSize.Level0)
{
    sptr<DeferredVideoProcSession> deferredProcSession = new(std::nothrow) DeferredVideoProcSession(userId_, nullptr);
    ASSERT_NE(deferredProcSession, nullptr);

    sptr<DeferredVideoProcessingSessionCallback> remoteCallback =
        new(std::nothrow) DeferredVideoProcessingSessionCallback();
    ASSERT_NE(remoteCallback, nullptr);
    std::string videoId = "testVideoId";
    sptr<IPCFileDescriptor> ipcFileDescriptor;
    int32_t ret = 0;
    ret = remoteCallback->OnProcessVideoDone(videoId, ipcFileDescriptor);
    EXPECT_EQ(ret, 0);

    int32_t errorCode = 0;
    ret = remoteCallback->OnError(videoId, errorCode);
    EXPECT_EQ(ret, 0);

    int32_t status = 0;
    ret = remoteCallback->OnStateChanged(status);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test DeferredPhotoProcessingSessionCallback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test DeferredPhotoProcessingSessionCallback
 */
HWTEST_F(DeferredProcUnitTest, camera_deferred_proc_unittest_013, TestSize.Level0)
{
    std::shared_ptr<DeferredPhotoProcessingSessionCallback> deferredPhotoSessionCb1 =
        std::make_shared<DeferredPhotoProcessingSessionCallback>();
    ASSERT_NE(deferredPhotoSessionCb1, nullptr);
    sptr<DeferredPhotoProcSession> deferredProcSession =
        new(std::nothrow) DeferredPhotoProcSession(userId_, std::make_shared<TestDeferredPhotoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
    std::shared_ptr<DeferredPhotoProcessingSessionCallback> deferredPhotoSessionCb2 =
        std::make_shared<DeferredPhotoProcessingSessionCallback>(deferredProcSession);
    ASSERT_NE(deferredPhotoSessionCb2, nullptr);
}
} // CameraStandard
} // OHOS
