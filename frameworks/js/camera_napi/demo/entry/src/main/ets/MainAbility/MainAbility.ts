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

import Ability from '@ohos.app.ability.UIAbility';
import Window from '@ohos.window';
import Logger from '../model/Logger';

const TAG: string = 'MainAbility';

export default class MainAbility extends Ability {
  onCreate(want, launchParam): void {
    Logger.info(TAG, 'Ability onCreate');
    Logger.info(TAG, 'want param:' + JSON.stringify(want) ?? '');
    Logger.info(TAG, 'launchParam:' + JSON.stringify(launchParam) ?? '');
    globalThis.abilityContext = this.context;
  }

  onDestroy(): void {
    Logger.info(TAG, 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: Window.WindowStage): void {
    // Main window is created, set main page for this ability
    Logger.info(TAG, 'Ability onWindowStageCreate');
    windowStage.getMainWindow().then((win: Window.Window): void => {
      win.setLayoutFullScreen(true).then((): void => {
        win.setSystemBarEnable(['navigation']).then((): void => {
        });
      });
      win.setSystemBarProperties({
        navigationBarColor: '#00000000',
        navigationBarContentColor: '#B3B3B3'
      }).then((): void => {
      });
    })
    this.onLoadContent(windowStage, 'pages/Index');
  }

  onLoadContent(windowStage, page): void {
    windowStage.loadContent(page, (err): void => {
      if (err.code) {
        Logger.error(TAG, 'onLoadContent Failed to load the content. Cause: %{public}s' + JSON.stringify(err) ?? '');
        return;
      }
    });

  }

  onWindowStageDestroy(): void {
    // Main window is destroyed, release UI related resources
    Logger.info(TAG, 'Ability onWindowStageDestroy');
  }

  onForeground(): void {
    // Ability has brought to foreground
    Logger.info(TAG, 'Ability onForeground');
  }

  onBackground(): void {
    // Ability has back to background
    Logger.info(TAG, 'Ability onBackground');
  }
}
