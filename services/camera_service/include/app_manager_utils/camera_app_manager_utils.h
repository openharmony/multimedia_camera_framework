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

#ifndef OHOS_CAMERA_APP_MANAGER_UTILS_H
#define OHOS_CAMERA_APP_MANAGER_UTILS_H

#include <string>
#include <vector>

#include "app_mgr_interface.h"

namespace OHOS {
namespace CameraStandard {
class CameraAppManagerUtils {
public:
    static sptr<OHOS::AppExecFwk::IAppMgr> GetAppManagerInstance();
    static void GetForegroundApplications(std::vector<OHOS::AppExecFwk::AppStateData>& appsData);
    static bool IsForegroundApplication(const uint32_t tokenId);

private:
    static void OnRemoveInstance();

    class CameraAppManagerUtilsDeathRecipient;
    static sptr<OHOS::AppExecFwk::IAppMgr> appManagerInstance_;
};

} // namespace PowerMgr
} // namespace OHOS

#endif // OHOS_CAMERA_APP_MANAGER_UTILS_H