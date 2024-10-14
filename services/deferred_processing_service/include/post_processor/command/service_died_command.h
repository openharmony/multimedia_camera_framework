/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
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

#ifndef OHOS_CAMERA_DPS_SERVICE_DIED_COMMAND_H
#define OHOS_CAMERA_DPS_SERVICE_DIED_COMMAND_H

#include "command.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class ServiceDiedCommand : public Command {
    DECLARE_CMD_CLASS(ServiceDiedCommand);
public:
    ServiceDiedCommand(const int32_t userId);
    ~ServiceDiedCommand() override;

protected:
    int32_t Executing() override;

    const int32_t userId_;
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_SERVICE_DIED_COMMAND_H