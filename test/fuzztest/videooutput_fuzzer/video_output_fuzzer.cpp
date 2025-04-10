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

#include "video_output_fuzzer.h"
#include "camera_device.h"
#include "camera_log.h"
#include "camera_output_capability.h"
#include "capture_scene_const.h"
#include <cstdint>
#include "input/camera_manager.h"
#include "message_parcel.h"
#include <cstdint>
#include <memory>
#include "token_setproc.h"
#include "nativetoken_kit.h"
#include "accesstoken_kit.h"
#include "securec.h"

using namespace std;

namespace OHOS {
namespace CameraStandard {
static constexpr int32_t MAX_CODE_LEN  = 512;
static constexpr int32_t MIN_SIZE_NUM = 4;
static const uint8_t* RAW_DATA = nullptr;
const size_t THRESHOLD = 10;
static size_t g_dataSize = 0;
static size_t g_pos;

/*
* describe: get data from outside untrusted data(g_data) which size is according to sizeof(T)
* tips: only support basic type
*/
template<class T>
T GetData()
{
    T object {};
    size_t objectSize = sizeof(object);
    if (RAW_DATA == nullptr || objectSize > g_dataSize - g_pos) {
        return object;
    }
    errno_t ret = memcpy_s(&object, objectSize, RAW_DATA + g_pos, objectSize);
    if (ret != EOK) {
        return {};
    }
    g_pos += objectSize;
    return object;
}

template<class T>
uint32_t GetArrLength(T& arr)
{
    if (arr == nullptr) {
        MEDIA_INFO_LOG("%{public}s: The array length is equal to 0", __func__);
        return 0;
    }
    return sizeof(arr) / sizeof(arr[0]);
}

void VideoOutputFuzzer::VideoOutputFuzzTest()
{
    if ((RAW_DATA == nullptr) || (g_dataSize > MAX_CODE_LEN) || (g_dataSize < MIN_SIZE_NUM)) {
        return;
    }
    auto manager = CameraManager::GetInstance();
    auto cameras = manager->GetSupportedCameras();
    CHECK_ERROR_RETURN_LOG(cameras.empty(), "GetSupportedCameras Error");
    auto s = manager->CreateCaptureSession(SceneMode::VIDEO);
    s->BeginConfig();
    auto cap = s->GetCameraOutputCapabilities(cameras[0]);
    CHECK_ERROR_RETURN_LOG(cap.empty(), "GetCameraOutputCapabilities Error");
    auto vp = cap[0]->GetVideoProfiles();
    CHECK_ERROR_RETURN_LOG(vp.empty(), "GetVideoProfiles Error");
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    auto output = manager->CreateVideoOutput(vp[0], surface);
    output->Start();
    output->Stop();
    output->Resume();
    output->Pause();
    output->CreateStream();
    output->Release();
    output->GetApplicationCallback();
    output->GetFrameRateRange();
    int32_t minFrameRate = GetData<int32_t>();
    int32_t maxFrameRate = GetData<int32_t>();
    output->SetFrameRateRange(minFrameRate, maxFrameRate);
    output->SetOutputFormat(GetData<int32_t>());
    output->SetFrameRate(minFrameRate, maxFrameRate);
    output->GetSupportedFrameRates();
    bool enabled = GetData<bool>();
    output->enableMirror(enabled);
    output->IsMirrorSupported();
    output->GetSupportedVideoMetaTypes();
    output->AttachMetaSurface(surface, VIDEO_META_MAKER_INFO);
    int pid = GetData<int>();
    output->CameraServerDied(pid);
    output->canSetFrameRateRange(minFrameRate, maxFrameRate);
    output->GetVideoRotation(GetData<int32_t>());
    output->IsAutoDeferredVideoEnhancementSupported();
    output->IsAutoDeferredVideoEnhancementEnabled();
    output->EnableAutoDeferredVideoEnhancement(enabled);
    output->IsVideoStarted();
    output->IsRotationSupported(enabled);
    output->SetRotation(GetData<int32_t>());
    output->IsAutoVideoFrameRateSupported();
    output->EnableAutoVideoFrameRate(enabled);
    std::vector<int32_t> supportedRotations;
    output->GetSupportedRotations(supportedRotations);
}

void Test()
{
    auto videoOutput = std::make_unique<VideoOutputFuzzer>();
    if (videoOutput == nullptr) {
        MEDIA_INFO_LOG("videoPostProcessor is null");
        return;
    }
    videoOutput->VideoOutputFuzzTest();
}

typedef void (*TestFuncs[1])();

TestFuncs g_testFuncs = {
    Test,
};

bool FuzzTest(const uint8_t* rawData, size_t size)
{
    // initialize data
    RAW_DATA = rawData;
    g_dataSize = size;
    g_pos = 0;

    uint32_t code = GetData<uint32_t>();
    uint32_t len = GetArrLength(g_testFuncs);
    if (len > 0) {
        g_testFuncs[code % len]();
    } else {
        MEDIA_INFO_LOG("%{public}s: The len length is equal to 0", __func__);
    }

    return true;
}
} // namespace CameraStandard
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    if (size < OHOS::CameraStandard::THRESHOLD) {
        return 0;
    }

    OHOS::CameraStandard::FuzzTest(data, size);
    return 0;
}