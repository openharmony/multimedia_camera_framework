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

#ifndef VIDEO_SESSION_UNITTEST_H
#define VIDEO_SESSION_UNITTEST_H

#include "gtest/gtest.h"
#include "video_session.h"
#include "video_session_for_sys.h"
#include "camera_manager.h"
#include "camera_manager_for_sys.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {

class TestFocusTrackingCallback : public FocusTrackingCallback {
    public:
        void OnFocusTrackingInfoAvailable(FocusTrackingInfo &focusTrackingInfo) const override {}
};

class CameraVideoSessionUnitTest : public testing::Test {
public:
    /* SetUpTestCase:The preset action of the test suite is executed before the first TestCase */
    static void SetUpTestCase(void);
    /* TearDownTestCase:The test suite cleanup action is executed after the last TestCase */
    static void TearDownTestCase(void);
    /* SetUp:Execute before each test case */
    void SetUp(void);
    /* TearDown:Execute after each test case */
    void TearDown(void);
    void TestVideoSessionForSysCallback();
    void TestVideoSessionForSysPreconfig(sptr<CaptureInput>& input,
        PreconfigType preconfigType, ProfileSizeRatio profileSizeRatio);
    void TestVideoSessionPreconfig(sptr<CaptureInput>& input,
        PreconfigType preconfigType, ProfileSizeRatio profileSizeRatio);
private:
    uint64_t tokenId_ = 0;
    int32_t uid_ = 0;
    int32_t userId_ = 0;
    sptr<CameraManager> cameraManager_ = nullptr;
    sptr<CameraManagerForSys> cameraManagerForSys_ = nullptr;
};

class TestLightStatusCallback : public LightStatusCallback {
public:
    TestLightStatusCallback() {}
    ~TestLightStatusCallback() {}
    void OnLightStatusChanged(LightStatus &status)  override {}
};
}
}

#endif