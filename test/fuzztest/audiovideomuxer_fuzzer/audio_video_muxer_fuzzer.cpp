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

#include "audio_video_muxer_fuzzer.h"
#include "message_parcel.h"
#include "securec.h"
#include "camera_log.h"
#include "avmuxer.h"
#include "avmuxer_impl.h"

namespace OHOS {
namespace CameraStandard {
static constexpr int32_t MAX_CODE_LEN  = 512;
static constexpr int32_t MIN_SIZE_NUM = 4;
static constexpr int32_t NUM_1 = 1;
static const uint8_t* RAW_DATA = nullptr;
const size_t THRESHOLD = 10;
static size_t g_dataSize = 0;
static size_t g_pos;
std::shared_ptr<AudioVideoMuxer> AudioVideoMuxerFuzzer::fuzz_{nullptr};

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

void AudioVideoMuxerFuzzer::AudioVideoMuxerFuzzTest()
{
    if ((RAW_DATA == nullptr) || (g_dataSize > MAX_CODE_LEN) || (g_dataSize < MIN_SIZE_NUM)) {
        return;
    }
    fuzz_ = std::make_shared<AudioVideoMuxer>();
    CHECK_ERROR_RETURN_LOG(!fuzz_, "Create fuzz_ Error");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_MPEG_4;
    fuzz_->Create(outputFormat, nullptr);
    std::shared_ptr<OHOS::Media::AVBuffer> sample = std::make_shared<OHOS::Media::AVBuffer>();
    constexpr int32_t executionModeCount = static_cast<int32_t>(TrackType::META_TRACK) + NUM_1;
    TrackType type = static_cast<TrackType>(GetData<uint8_t>() % executionModeCount);
    std::shared_ptr<AVMuxerImpl> impl = std::make_shared<AVMuxerImpl>();
    fuzz_->muxer_ = impl;
    int trackId = GetData<int>();
    fuzz_->WriteSampleBuffer(sample, type);
    std::shared_ptr<Format> format = std::make_shared<Format>();
    fuzz_->AddTrack(trackId, format, type);
}

void Test()
{
    auto audioVideoMuxer = std::make_unique<AudioVideoMuxerFuzzer>();
    if (audioVideoMuxer == nullptr) {
        MEDIA_INFO_LOG("audioVideoMuxer is null");
        return;
    }
    audioVideoMuxer->AudioVideoMuxerFuzzTest();
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