/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_DPS_SESSION_INFO_H
#define OHOS_CAMERA_DPS_SESSION_INFO_H

#include "ideferred_photo_processing_session_callback.h"
#include "deferred_photo_processing_session.h"
#include "deferred_photo_processor.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class SessionManager;

class SessionInfo : public RefBase {
public:
    SessionInfo(const int32_t userId, const sptr<IDeferredPhotoProcessingSessionCallback>& callback,
        SessionManager* sessionManager);
    virtual ~SessionInfo();

    sptr<IDeferredPhotoProcessingSession> CreateDeferredPhotoProcessingSession(const int32_t userId,
        std::shared_ptr<DeferredPhotoProcessor> processor, TaskManager* taskManager,
        sptr<IDeferredPhotoProcessingSessionCallback> callback);
    sptr<IDeferredPhotoProcessingSession> GetDeferredPhotoProcessingSession();
    sptr<IDeferredPhotoProcessingSessionCallback> GetRemoteCallback();
    void OnCallbackDied();
    void SetCallback(const sptr<IDeferredPhotoProcessingSessionCallback>& callback);

private:
    class CallbackDeathRecipient;
    const int32_t userId_;
    sptr<IDeferredPhotoProcessingSessionCallback> callback_;
    SessionManager* sessionManager_;
    sptr<IDeferredPhotoProcessingSession> session_;
    sptr<CallbackDeathRecipient> callbackDeathRecipient_;
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_SESSION_INFO_H