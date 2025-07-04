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

#ifndef CAMERA_PROFESSION_SESSION_UNITTEST_H
#define CAMERA_PROFESSION_SESSION_UNITTEST_H

#include "gtest/gtest.h"
#include "session/profession_session.h"
#include "input/camera_manager.h"

namespace OHOS {
namespace CameraStandard {

class ProfessionSessionUnitTest : public testing::Test {
public:
    /* SetUpTestCase:The preset action of the test suite is executed before the first TestCase */
    static void SetUpTestCase(void);

    /* TearDownTestCase:The test suite cleanup action is executed after the last TestCase */
    static void TearDownTestCase(void);

    /* SetUp:Execute before each test case */
    void SetUp();

    /* TearDown:Execute after each test case */
    void TearDown();

    bool IsSupportMode(SceneMode mode);

    sptr<CaptureOutput> CreatePreviewOutput(Profile& profile);
    sptr<CaptureOutput> CreateVideoOutput(VideoProfile& videoProfile);
    void Init();

    sptr<CameraManager> manager_;
    std::vector<sptr<CameraDevice>> cameras_;
    sptr<ProfessionSession> session_;
    sptr<CaptureInput> input_;
    SceneMode sceneMode_;
};
} // CameraStandard
} // OHOS
#endif // CAMERA_PROFESSION_SESSION_UNITTEST_H
