/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "camera_privacy.h"
#include "camera_log.h"
#include "hcamera_device.h"
#include "hcapture_session.h"
#include "types.h"

namespace OHOS {
namespace CameraStandard {
using OHOS::Security::AccessToken::PrivacyKit;
using OHOS::Security::AccessToken::AccessTokenKit;

sptr<HCaptureSession> CastToSession(sptr<IStreamOperatorCallback> streamOpCb)
{
    if (streamOpCb == nullptr) {
        return nullptr;
    }
    return static_cast<HCaptureSession*>(streamOpCb.GetRefPtr());
}

void PermissionStatusChangeCb::PermStateChangeCallback(Security::AccessToken::PermStateChangeInfo& result)
{
    MEDIA_INFO_LOG("enter PermissionStatusChangeNotify permStateChangeType:%{public}d tokenId:%{public}d"
        " permissionName:%{public}s", result.permStateChangeType, result.tokenID, result.permissionName.c_str());
    auto device = cameraDevice_.promote();
    if ((result.permStateChangeType == 0) && (device != nullptr)) {
        auto session = CastToSession(device->GetStreamOperatorCallback());
        if (session) {
            session->ReleaseStreams();
            session->StopMovingPhoto();
        }
        device->CloseDevice();
        device->OnError(DEVICE_PREEMPT, 0);
    }
}

void CameraUseStateChangeCb::StateChangeNotify(Security::AccessToken::AccessTokenID tokenId, bool isShowing)
{
    MEDIA_INFO_LOG("enter CameraUseStateChangeNotify tokenId:%{public}d", tokenId);
    auto device = cameraDevice_.promote();
    if ((isShowing == false) && (device != nullptr)) {
        auto session = CastToSession(device->GetStreamOperatorCallback());
        if (session) {
            session->ReleaseStreams();
            session->StopMovingPhoto();
        }
        device->CloseDevice();
    }
}

CameraPrivacy::~CameraPrivacy()
{
    cameraUseCallbackPtr_ = nullptr;
    permissionCallbackPtr_ = nullptr;
}

bool CameraPrivacy::IsAllowUsingCamera()
{
    return PrivacyKit::IsAllowedUsingPermission(callerToken_, OHOS_PERMISSION_CAMERA);
}

void CameraPrivacy::RegisterPermissionCallback()
{
    Security::AccessToken::PermStateChangeScope scopeInfo;
    scopeInfo.permList = {OHOS_PERMISSION_CAMERA};
    scopeInfo.tokenIDs = {callerToken_};
    permissionCallbackPtr_ = std::make_shared<PermissionStatusChangeCb>(cameraDevice_, scopeInfo);
    MEDIA_DEBUG_LOG("RegisterPermissionCallback tokenId:%{public}d register", callerToken_);
    int32_t res = AccessTokenKit::RegisterPermStateChangeCallback(permissionCallbackPtr_);
    if (res != CAMERA_OK) {
        MEDIA_ERR_LOG("RegisterPermissionCallback failed.");
    }
}

void CameraPrivacy::UnregisterPermissionCallback()
{
    if (permissionCallbackPtr_ == nullptr) {
        MEDIA_ERR_LOG("permissionCallbackPtr_ is null.");
        return;
    }
    MEDIA_DEBUG_LOG("UnregisterPermissionCallback tokenId:%{public}d unregister", callerToken_);
    int32_t res = AccessTokenKit::UnRegisterPermStateChangeCallback(permissionCallbackPtr_);
    if (res != CAMERA_OK) {
        MEDIA_ERR_LOG("UnregisterPermissionCallback failed.");
    }
    permissionCallbackPtr_ = nullptr;
}

void CameraPrivacy::AddCameraPermissionUsedRecord()
{
    int32_t successCout = 1;
    int32_t failCount = 0;
    int32_t ret = PrivacyKit::AddPermissionUsedRecord(callerToken_, OHOS_PERMISSION_CAMERA, successCout, failCount);
    MEDIA_DEBUG_LOG("AddCameraPermissionUsedRecord tokenId:%{public}d", callerToken_);
    if (ret != CAMERA_OK) {
        MEDIA_ERR_LOG("AddCameraPermissionUsedRecord failed.");
    }
}

bool CameraPrivacy::StartUsingPermissionCallback()
{
    CHECK_ERROR_RETURN_RET_LOG(cameraUseCallbackPtr_, true, "has StartUsingPermissionCallback!");
    cameraUseCallbackPtr_ = std::make_shared<CameraUseStateChangeCb>(cameraDevice_);
    int32_t res = PrivacyKit::StartUsingPermission(callerToken_, OHOS_PERMISSION_CAMERA, cameraUseCallbackPtr_, pid_);
    MEDIA_DEBUG_LOG("after StartUsingPermissionCallback tokenId:%{public}d", callerToken_);
    CHECK_ERROR_PRINT_LOG(res != CAMERA_OK, "StartUsingPermissionCallback failed.");
    return res == CAMERA_OK;
}

void CameraPrivacy::StopUsingPermissionCallback()
{
    MEDIA_DEBUG_LOG("enter StopUsingPermissionCallback tokenId:%{public}d", callerToken_);
    int32_t res = PrivacyKit::StopUsingPermission(callerToken_, OHOS_PERMISSION_CAMERA, pid_);
    CHECK_ERROR_PRINT_LOG(res != CAMERA_OK, "StopUsingPermissionCallback failed.");
    cameraUseCallbackPtr_ = nullptr;
}
} // namespace CameraStandard
} // namespace OHOS