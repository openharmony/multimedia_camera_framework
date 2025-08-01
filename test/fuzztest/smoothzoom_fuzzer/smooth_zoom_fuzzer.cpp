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

#include "smooth_zoom_fuzzer.h"
#include "camera_timer.h"
#include "hstream_capture.h"
#include "message_parcel.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "accesstoken_kit.h"
#include "camera_metadata_info.h"
#include "metadata_utils.h"
#include <fuzzer/FuzzedDataProvider.h>
#include "test_token.h"

namespace {

const int32_t LIMITSIZE = 12;

}

namespace OHOS {
namespace CameraStandard {

std::shared_ptr<SmoothZoom> SmoothZoomFuzzer::fuzz_{nullptr};
static constexpr int32_t MIN_SIZE_NUM = 4;

void SmoothZoomFuzzer::Test(uint8_t *rawData, size_t size)
{
    FuzzedDataProvider fdp(rawData, size);
    if (fdp.remaining_bytes() < MIN_SIZE_NUM) {
        return;
    }
    CHECK_RETURN_ELOG(!TestToken().GetAllCameraPermission(), "GetPermission error");

    fuzz_ = std::make_shared<SmoothZoom>();
    CHECK_RETURN_ELOG(!fuzz_, "Create fuzz_ Error");
    MessageParcel data;
    data.WriteRawData(rawData, size);

    SmoothZoomType mode = static_cast<SmoothZoomType>(fdp.ConsumeBool());
    auto alg = fuzz_->GetZoomAlgorithm(mode);

    data.RewindRead(0);
    float currentZoom = data.ReadFloat();
    float targetZoom = data.ReadFloat();
    float frameInterval = data.ReadFloat();
    alg->GetZoomArray(currentZoom, targetZoom, frameInterval);
}

} // namespace CameraStandard
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::CameraStandard::SmoothZoomFuzzer::Test(data, size);
    return 0;
}