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

#include "camera_device_unittest.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <vector>
#include "access_token.h"
#include "accesstoken_kit.h"
#include "camera_log.h"
#include "gmock/gmock.h"
#include "hap_token_info.h"
#include "ipc_skeleton.h"
#include "metadata_utils.h"
#include "nativetoken_kit.h"
#include "surface.h"
#include "test_common.h"
#include "token_setproc.h"
#include "os_account_manager.h"

using namespace testing::ext;

namespace OHOS {
namespace CameraStandard {

void CameraDeviceUnitTest::SetUpTestCase(void) {}

void CameraDeviceUnitTest::TearDownTestCase(void) {}

void CameraDeviceUnitTest::SetUp()
{
    NativeAuthorization();
    cameraManager_ = CameraManager::GetInstance();
    ASSERT_NE(cameraManager_, nullptr);
}

void CameraDeviceUnitTest::TearDown()
{
    cameraManager_ = nullptr;
}

void CameraDeviceUnitTest::NativeAuthorization()
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
    MEDIA_DEBUG_LOG("CameraDeviceUnitTest::NativeAuthorization uid:%{public}d", uid_);
    SetSelfTokenID(tokenId_);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

/*
 * Feature: Framework
 * Function: Test cameradevice with GetPosition and GetZoomRatioRange
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetPosition for invoke and GetZoomRatioRange for zoomRatioRange_ not empty
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_001, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();

    cameras[0]->GetPosition();
    cameras[0]->zoomRatioRange_ = {1.1, 2.1};
    EXPECT_EQ(cameras[0]->GetZoomRatioRange(), cameras[0]->zoomRatioRange_);
}

/*
 * Feature: Framework
 * Function: Test cameradevice with SetProfile
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetProfile for capability is nullptr
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_002, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();

    sptr<CameraOutputCapability> capabilityRet = nullptr;
    int32_t modeNameRet = 0;
    cameras[0]->SetProfile(capabilityRet);
    EXPECT_FALSE(cameras[0]->modePreviewProfiles_.empty());
    cameras[0]->SetProfile(capabilityRet, modeNameRet);
    EXPECT_FALSE(cameras[0]->modePreviewProfiles_.empty());
}

/*
 * Feature: Framework
 * Function: Test cameradevice with SetCameraDeviceUsedAsPosition and GetSupportedFoldStatus
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetCameraDeviceUsedAsPosition for invoke and GetSupportedFoldStatus for invoke
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_004, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();

    CameraPosition usedAsPositionRet = CAMERA_POSITION_BACK;
    cameras[0]->SetCameraDeviceUsedAsPosition(usedAsPositionRet);
    EXPECT_EQ(cameras[0]->usedAsCameraPosition_, CAMERA_POSITION_BACK);

    EXPECT_EQ(cameras[0]->GetSupportedFoldStatus(), 0);
}

/*
 * Feature: Framework
 * Function: Test cameradevice isFindModuleTypeTag
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test isFindModuleTypeTag
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_005, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();

    uint32_t tag = 0;
    bool ret = cameras[0]->isFindModuleTypeTag(tag);

    EXPECT_EQ(ret, true);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetID()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetID()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_006, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    string ret = cameras[0]->GetID();
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetMetadata()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetMetadata()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_007, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    cameras[0]->GetMetadata();
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetMetadata()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetMetadata()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_008, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraPtr = cameras[0]->GetCameraAbility();
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetPosition()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetPosition()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_009, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    int ret = cameras[0]->GetPosition();

    EXPECT_EQ(ret, 2);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetUsedAsPosition()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetUsedAsPosition()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_010, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    int ret = cameras[0]->GetUsedAsPosition();

    EXPECT_EQ(ret, 1);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetCameraType()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetCameraType()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_011, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    int ret = cameras[0]->GetCameraType();

    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetConnectionType()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetConnectionType()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_012, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    int ret = cameras[0]->GetConnectionType();

    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetCameraFoldScreenType()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetCameraFoldScreenType()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_013, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    int ret = cameras[0]->GetCameraFoldScreenType();

    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetCameraFoldScreenType()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetCameraFoldScreenType()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_014, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    int ret = cameras[0]->GetCameraOrientation();

    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetCameraFoldScreenType()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetCameraFoldScreenType()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_015, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    int ret = cameras[0]->GetModuleType();

    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test cameradevice SetProfile(sptr<CameraOutputCapability> capability) with nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetProfile(sptr<CameraOutputCapability> capability) with nullptr
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_016, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    sptr<CameraOutputCapability> capabilityRet = nullptr;
    
    cameras[0]->SetProfile(capabilityRet);
    EXPECT_FALSE(cameras[0]->modePreviewProfiles_.empty());
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetExposureBiasRange()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureBiasRange()
 */
HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_017, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    vector<float>cameDev_ExpBiaRange = cameras[0]->GetExposureBiasRange();

    EXPECT_EQ(cameDev_ExpBiaRange.size(), 2);
}

/*
 * Feature: Framework
 * Function: Test cameradevice SetCameraDeviceUsedAsPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetCameraDeviceUsedAsPosition
 */
 HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_018, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    cameras[0]->SetCameraDeviceUsedAsPosition(CameraPosition::CAMERA_POSITION_FRONT);

    EXPECT_EQ(cameras[0]->GetUsedAsPosition(), CameraPosition::CAMERA_POSITION_FRONT);
}

/*
 * Feature: Framework
 * Function: Test cameradevice GetSupportedFoldStatus()
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetSupportedFoldStatus()
 */
 HWTEST_F(CameraDeviceUnitTest, camera_device_unittest_019, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager_->GetSupportedCameras();
    uint32_t ret = cameras[0]->GetSupportedFoldStatus();

    EXPECT_EQ(ret, 0);
}
}
}