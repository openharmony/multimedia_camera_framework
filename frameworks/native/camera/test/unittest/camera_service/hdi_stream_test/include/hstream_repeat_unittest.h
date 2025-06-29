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

#ifndef HSTREAM_REPEAT_UNITTEST_H
#define HSTREAM_REPEAT_UNITTEST_H

#include "gtest/gtest.h"
#include <refbase.h>
#include "hstream_repeat.h"
#include "camera_manager.h"
#include "hstream_common.h"

namespace OHOS {
namespace CameraStandard {
class HStreamRepeatUnit : public testing::Test {
public:
    static const int32_t PHOTO_DEFAULT_WIDTH = 1280;
    static const int32_t PHOTO_DEFAULT_HEIGHT = 960;
    /* SetUpTestCase:The preset action of the test suite is executed before the first TestCase */
    static void SetUpTestCase(void);

    /* TearDownTestCase:The test suite cleanup action is executed after the last TestCase */
    static void TearDownTestCase(void);

    /* SetUp:Execute before each test case */
    void SetUp(void);

    /* TearDown:Execute after each test case */
    void TearDown(void);

    HStreamRepeat* CreateHStreamRepeat(RepeatStreamType type = RepeatStreamType::PREVIEW);
    sptr<CaptureOutput> CreatePhotoOutput(Profile& photoProfile);
};
} // CameraStandard
} // OHOS
#endif