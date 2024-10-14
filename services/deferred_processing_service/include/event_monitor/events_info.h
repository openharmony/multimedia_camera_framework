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

#ifndef OHOS_CAMERA_DPS_EVENTS_INFO_H
#define OHOS_CAMERA_DPS_EVENTS_INFO_H

#include "basic_definitions.h"
#include "singleton.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
class EventsInfo : public Singleton<EventsInfo> {
    DECLARE_SINGLETON(EventsInfo);
    
public:
    void Initialize();
    ScreenStatus GetScreenState();
    BatteryStatus GetBatteryState();
    ChargingStatus GetChargingState();
    BatteryLevel GetBatteryLevel();
    SystemPressureLevel GetPhotoThermalLevel();
    ThermalLevel GetThermalLevel();

private:
    ScreenStatus screenState_ {SCREEN_OFF};
    BatteryStatus batteryState_ {BATTERY_LOW};
    ChargingStatus chargingState_ {DISCHARGING};
    BatteryLevel batteryLevel_ {BATTERY_LEVEL_LOW};
    SystemPressureLevel photoThermalLevel_ {SEVERE};
    ThermalLevel thermalLevel_ {LEVEL_0};
};
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS
#endif // OHOS_CAMERA_DPS_EVENTS_INFO_H