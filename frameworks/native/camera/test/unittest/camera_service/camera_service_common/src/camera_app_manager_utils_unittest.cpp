/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "camera_app_manager_utils_unittest.h"
#include "camera_app_manager_utils.h"
#include "camera_log.h"
#include "camera_privacy.h"
#include "capture_scene_const.h"
#include "hcamera_service.h"
#include "ipc_skeleton.h"
#include "nativetoken_kit.h"
#include "os_account_manager.h"
#include "token_setproc.h"
#include "test_token.h"

using namespace testing::ext;
namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Camera::V1_1;
void CameraAppManagerUtilsUnit::SetUpTestCase()
{
    MEDIA_DEBUG_LOG("CameraPrivacyUnitTest::SetUpTestCase started!");
    ASSERT_TRUE(TestToken().GetAllCameraPermission());
}

void CameraAppManagerUtilsUnit::TearDownTestCase()
{
    MEDIA_DEBUG_LOG("CameraPrivacyUnitTest::TearDownTestCase started!");
}

void CameraAppManagerUtilsUnit::SetUp()
{
    MEDIA_DEBUG_LOG("CameraPrivacyUnitTest::SetUp started!");
}

void CameraAppManagerUtilsUnit::TearDown()
{
    MEDIA_DEBUG_LOG("CameraPrivacyUnitTest::TearDown started!");
}

/*
 * Feature: Framework
 * Function: Test CameraAppManagerClient
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: testcase of the CameraAppManagerUtilsUnit
 */
HWTEST_F(CameraAppManagerUtilsUnit, camera_app_manager_utils_unittest, TestSize.Level1)
{
    sptr<OHOS::AppExecFwk::IAppMgr> manager = CameraAppManagerUtils::GetAppManagerInstance();
    ASSERT_NE(manager, nullptr);
    bool ret = CameraAppManagerUtils::IsForegroundApplication(tokenId_);
    EXPECT_EQ(ret, false);
}
} // namespace CameraStandard
} // namespace OHOS