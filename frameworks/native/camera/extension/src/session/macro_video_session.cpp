/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "session/macro_video_session.h"

#include "camera_error_code.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
MacroVideoSession::MacroVideoSession(sptr<ICaptureSession>& captureSession) : CaptureSessionForSys(captureSession) {}

bool MacroVideoSession::CanAddOutput(sptr<CaptureOutput>& output)
{
    MEDIA_DEBUG_LOG("Enter Into MacroVideoSession::CanAddOutput");
    CHECK_RETURN_RET_ELOG(!IsSessionConfiged() || output == nullptr, false,
        "MacroVideoSession::CanAddOutput operation is Not allowed!");
    return CaptureSession::CanAddOutput(output);
}

int32_t MacroVideoSession::CommitConfig()
{
    int32_t ret = CaptureSession::CommitConfig();
    CHECK_RETURN_RET(ret != CameraErrorCode::SUCCESS, ret);
    LockForControl();
    ret = EnableMacro(true);
    UnlockForControl();
    return ret;
}
} // namespace CameraStandard
} // namespace OHOS