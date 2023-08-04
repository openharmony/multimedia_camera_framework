/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "stream_capture_fuzzer.h"
using namespace std;

namespace OHOS {
namespace CameraStandard {
constexpr int32_t OFFSET = 4;
const std::u16string FORMMGR_INTERFACE_TOKEN = u"ICameraDeviceService";
const int32_t LIMITSIZE = 4;
const int32_t SHIFT_LEFT_8 = 8;
const int32_t SHIFT_LEFT_16 = 16;
const int32_t SHIFT_LEFT_24 = 24;
static int32_t cnt = 0;
const int32_t photoWidth = 1280;
const int32_t photoHeight = 960;
const int32_t photoFormat = CAMERA_FORMAT_JPEG;

uint32_t Convert2Uint32(const uint8_t *ptr)
{
    if (ptr == nullptr) {
        return 0;
    }
    /* Move the 0th digit to the left by 24 bits, the 1st digit to the left by 16 bits,
       the 2nd digit to the left by 8 bits, and the 3rd digit not to the left */
    return (ptr[0] << SHIFT_LEFT_24) | (ptr[1] << SHIFT_LEFT_16) | (ptr[2] << SHIFT_LEFT_8) | (ptr[3]);
}
void StreamCaptureFuzzTest(uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    cout<<"StreamCaptureFuzzTest begin--------------------------------------- cnt = "<<++cnt<<endl;
    uint32_t code = Convert2Uint32(rawData);
    rawData = rawData + OFFSET;
    size = size - OFFSET;

    MessageParcel data;
    data.WriteInterfaceToken(FORMMGR_INTERFACE_TOKEN);
    data.WriteBuffer(rawData, size);
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;

    sptr<IConsumerSurface> photoSurface = IConsumerSurface::Create();
    if (photoSurface == nullptr) {
        return 0;
    }
    sptr<IBufferProducer> producer = photoSurface->GetProducer();
    std::shared_ptr<HStreamCapture> streamcapture =
        std::make_shared<HStreamCapture>(producer, photoFormat, photoWidth, photoHeight);
    streamcapture->OnRemoteRequest(code, data, reply, option);
    cout<<"StreamCaptureFuzzTest begin--------------------------------------- cnt = "<<++cnt<<endl;
}
} // namespace CameraStandard
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    cout<<"StreamCaptureFuzzTest begin+++++++++++++++++++++++++++++++++"<<endl;
    OHOS::CameraStandard::StreamCaptureFuzzTest(data, size);
    cout<<"StreamCaptureFuzzTest end+++++++++++++++++++++++++++++++++++"<<endl;
    return 0;
}

