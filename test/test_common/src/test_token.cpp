/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <sstream>
#include <thread>

#include "accesstoken_kit.h"
#include "ipc_skeleton.h"
#include "test_token.h"
#include "token_setproc.h"

namespace OHOS {
namespace CameraStandard {
using namespace OHOS::Security::AccessToken;

uint64_t TestToken::shellTokenID_ = IPCSkeleton::GetSelfTokenID();

uint64_t TestToken::GetMockedTokenId()
{
    return IPCSkeleton::GetSelfTokenID();
}

uint64_t TestToken::GetOriginalTokenId()
{
    return shellTokenID_;
}

void TestToken::AddPermission(const std::string& permission)
{
    permissions_.emplace_back(permission);
}

uint64_t TestToken::GetTokenId(const AtmToolsParamInfo& info)
{
    std::string dumpInfo;
    AccessTokenKit::DumpTokenInfo(info, dumpInfo);
    size_t pos = dumpInfo.find("\"tokenID\": ");
    if (pos == std::string::npos) {
        return 0;
    }
    pos += std::string("\"tokenID\": ").length();
    std::string numStr;
    while (pos < dumpInfo.length() && std::isdigit(dumpInfo[pos])) {
        numStr += dumpInfo[pos];
        ++pos;
    }

    std::istringstream iss(numStr);
    uint64_t tokenID;
    iss >> tokenID;
    return tokenID;
}

uint64_t TestToken::GetTokenIdFromProcess(const std::string& process)
{
    auto tokenId = IPCSkeleton::GetSelfTokenID();
    SetSelfTokenID(shellTokenID_); // only shell can dump tokenid

    AtmToolsParamInfo info;
    info.processName = process;
    auto processId = GetTokenId(info);

    SetSelfTokenID(tokenId);
    return processId;
}

bool TestToken::MockTokenId(const std::string& process)
{
    auto mockTokenId = GetTokenIdFromProcess(process);
    if (mockTokenId == 0) {
        return false;
    }
    if (SetSelfTokenID(mockTokenId) != 0) {
        return false;
    }
    return IPCSkeleton::GetSelfTokenID() != 0;
}

bool TestToken::GetAllCameraPermission()
{
    if (!MockTokenId("foundation")) {
        return false;
    }

    std::vector<std::string> cameraPermissions {
        "ohos.permission.CAMERA",
        "ohos.permission.CAMERA_BACKGROUND",
        "ohos.permission.CAMERA_CONTROL",
        "ohos.permission.DISTRIBUTED_DATASYNC",
        "ohos.permission.GET_SENSITIVE_PERMISSIONS",
        "ohos.permission.MICROPHONE",
        "ohos.permission.PERMISSION_USED_STATS",
        "ohos.permission.READ_IMAGEVIDEO",
        "ohos.permission.READ_MEDIA",
        "ohos.permission.WRITE_IMAGEVIDEO",
        "ohos.permission.WRITE_MEDIA",
    };
    cameraPermissions.insert(cameraPermissions.end(), permissions_.begin(), permissions_.end());
    std::vector<PermissionStateFull> permissionStates;
    for (const auto& permission : cameraPermissions) {
        PermissionStateFull permissionState = { .permissionName = permission,
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { PermissionState::PERMISSION_GRANTED },
            .grantFlags = { PERMISSION_SYSTEM_FIXED } };
        permissionStates.emplace_back(permissionState);
    }
    HapPolicyParams hapPolicyParams = {
        .apl = APL_NORMAL, .domain = "camera_test_setup.domain", .permList = {}, .permStateList = permissionStates
    };

    HapInfoParams hapInfoParams = { .userID = 100,
        .bundleName = "camera_test_setup",
        .instIndex = 0,
        .appIDDesc = "camera_test_setup",
        .apiVersion = 8,
        .isSystemApp = isSystemApp_ };

    AccessTokenIDEx tokenIdEx = { 0 };
    tokenIdEx = AccessTokenKit::AllocHapToken(hapInfoParams, hapPolicyParams);
    auto tokenID = tokenIdEx.tokenIDEx;
    if (!(INVALID_TOKENID != tokenID && 0 == SetSelfTokenID(tokenID) && tokenID == IPCSkeleton::GetSelfTokenID())) {
        return false;
    }
    return true;
}

} // namespace CameraStandard
} // namespace OHOS