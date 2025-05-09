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

#include "hcamera_host_manager_fuzzer.h"

#include "camera_log.h"
#include "message_parcel.h"
#include "token_setproc.h"
#include "nativetoken_kit.h"
#include "securec.h"
#include "portrait_session.h"
#include "os_account_manager.h"
#include "output/sketch_wrapper.h"
#include "hcamera_service.h"
#include "ipc_skeleton.h"

using namespace OHOS::CameraStandard::DeferredProcessing;
namespace OHOS {
namespace CameraStandard {
using OHOS::HDI::Camera::V1_0::ICameraDeviceCallback;
static constexpr int32_t MAX_CODE_LEN = 512;
static constexpr int32_t MIN_SIZE_NUM = 4;
static const uint8_t* RAW_DATA = nullptr;
const size_t THRESHOLD = 10;
static size_t g_dataSize = 0;
static size_t g_pos;
std::shared_ptr<HCameraHostManager> HCameraHostManagerFuzzer::fuzz_{nullptr};
std::shared_ptr<HCameraHostManager::CameraHostInfo> HCameraHostManagerFuzzer::hostInfo{nullptr};

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

void HCameraHostManagerFuzzer::HCameraHostManagerFuzzTest1()
{
    if ((RAW_DATA == nullptr) || (g_dataSize > MAX_CODE_LEN) || (g_dataSize < MIN_SIZE_NUM)) {
        return;
    }
    wptr<HCameraHostManager> hostManager =nullptr;
    std::shared_ptr<HCameraHostManager::StatusCallback> statusCallback = std::make_shared<StatusCallbackDemo>();
    fuzz_ = std::make_shared<HCameraHostManager>(statusCallback);
    CHECK_ERROR_RETURN_LOG(!fuzz_, "Create fuzz_ Error");
    uint8_t randomNum = GetData<uint8_t>();
    std::vector<std::string> testStrings = {"test1", "test2"};
    std::string cameraId(testStrings[randomNum % testStrings.size()]);
    bool isEnable = GetData<bool>();
    std::shared_ptr<OHOS::Camera::CameraMetadata> ability;
    fuzz_->GetCameraAbility(cameraId, ability);
    fuzz_->SetFlashlight(cameraId, isEnable);
}

void Test()
{
    auto hCameraHostManagerFuzzer = std::make_unique<HCameraHostManagerFuzzer>();
    if (hCameraHostManagerFuzzer == nullptr) {
        MEDIA_INFO_LOG("hCameraHostManagerFuzzer is null");
        return;
    }
    hCameraHostManagerFuzzer->HCameraHostManagerFuzzTest1();
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