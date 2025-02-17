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

#include "avcodev_task_manager_fuzzer.h"
#include "message_parcel.h"
#include "securec.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
using namespace DeferredProcessing;
static constexpr int32_t MAX_CODE_LEN  = 512;
static constexpr int32_t MIN_SIZE_NUM = 4;
static const uint8_t* RAW_DATA = nullptr;
const size_t THRESHOLD = 10;
static size_t g_dataSize = 0;
static size_t g_pos;
std::shared_ptr<AvcodecTaskManager> AvcodecTaskManagerFuzzer::fuzz_{nullptr};

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

void AvcodecTaskManagerFuzzer::AvcodecTaskManagerFuzzTest()
{
    if ((RAW_DATA == nullptr) || (g_dataSize > MAX_CODE_LEN) || (g_dataSize < MIN_SIZE_NUM)) {
        return;
    }
    sptr<AudioCapturerSession> session = new AudioCapturerSession();
    VideoCodecType type = VideoCodecType::VIDEO_ENCODE_TYPE_AVC;
    if (fuzz_ == nullptr) {
        fuzz_ = std::make_shared<AvcodecTaskManager>(session, type);
    }
    fuzz_->GetTaskManager();
    fuzz_->GetEncoderManager();
    int64_t timestamp = GetData<int64_t>();
    GraphicTransformType type_ = GRAPHIC_ROTATE_90;
    sptr<SurfaceBuffer> videoBuffer = SurfaceBuffer::Create();
    sptr<FrameRecord> frameRecord =
        new(std::nothrow) FrameRecord(videoBuffer, timestamp, type_);
    function<void()> task = []() {};
    fuzz_->SubmitTask(task);
    std::shared_ptr<PhotoAssetIntf> photoAssetProxy = nullptr;
    int32_t captureId = GetData<int32_t>();
    int32_t captureRotation = GetData<int32_t>();
    uint64_t taskName = GetData<uint64_t>();
    fuzz_->SetVideoFd(timestamp, photoAssetProxy, captureId);
    vector<sptr<FrameRecord>> frameRecords;
    fuzz_->DoMuxerVideo(frameRecords, taskName, captureRotation, captureId);
    vector<sptr<FrameRecord>> choosedBuffer;
    int64_t shutterTime = GetData<int64_t>();
    fuzz_->ChooseVideoBuffer(frameRecords, choosedBuffer, shutterTime, captureId);
    vector<sptr<AudioRecord>> audioRecordVec;
    sptr<AudioVideoMuxer> muxer;
    fuzz_->CollectAudioBuffer(audioRecordVec, muxer);
    fuzz_->videoEncoder_ = nullptr;
    fuzz_->audioEncoder_ = make_unique<AudioEncoder>();
    fuzz_->Stop();
}

void Test()
{
    auto avcodecTaskManager = std::make_unique<AvcodecTaskManagerFuzzer>();
    if (avcodecTaskManager == nullptr) {
        MEDIA_INFO_LOG("avcodecTaskManager is null");
        return;
    }
    avcodecTaskManager->AvcodecTaskManagerFuzzTest();
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