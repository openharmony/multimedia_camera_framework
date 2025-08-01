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

#include "camera_timer_fuzzer.h"
#include "camera_timer.h"
#include "hstream_capture.h"
#include "message_parcel.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "accesstoken_kit.h"
#include "camera_metadata_info.h"
#include "metadata_utils.h"
#include "test_token.h"

namespace {

const int32_t LIMITSIZE = 5;

}

namespace OHOS {
namespace CameraStandard {

std::shared_ptr<CameraTimer> CameraTimerFuzzer::fuzz_{nullptr};

void CameraTimerFuzzer::Test(uint8_t *rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    CHECK_RETURN_ELOG(!TestToken().GetAllCameraPermission(), "GetPermission error");

    fuzz_ = CameraTimer::GetInstance();
    CHECK_RETURN_ELOG(!fuzz_, "Create fuzz_ Error");
    MessageParcel data;
    data.WriteRawData(rawData, size);

    data.RewindRead(0);

    fuzz_->IncreaseUserCount();
    fuzz_->DecreaseUserCount();

    TimerCallback callback = [] {
        // do nothing
    };

    data.RewindRead(0);
    uint32_t interval = data.ReadUint32();

    data.RewindRead(0);
    bool once = data.ReadBool();

    uint32_t timerId = fuzz_->Register(callback, interval, once);
    
    fuzz_->Unregister(timerId);
}

} // namespace CameraStandard
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::CameraStandard::CameraTimerFuzzer::Test(data, size);
    return 0;
}