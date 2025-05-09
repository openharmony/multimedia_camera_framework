/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

// 闪关灯页面
// 对于图标遍历渲染，后续需要对此进行改进
import cameraDemo from 'libentry.so'
@Component
export struct FlashingLightPage {
  // 页面判断
  @State flashingBol: boolean = true
  // 闪光灯模式
  @State flashingNum: number = 0
  // 返回选中图片
  getImageDefault() {
    if (this.flashingNum == 0) {
      return $r('app.media.ic_camera_public_flash_off')
    }
    if (this.flashingNum == 1) {
      return $r('app.media.ic_camera_public_flash_on')
    }
    if (this.flashingNum == 2) {
      return $r('app.media.ic_camera_public_flash_auto')
    }
    if (this.flashingNum == 3) {
      return $r('app.media.flash_always_on')
    }
  }

  build() {
    Row() {
      if (this.flashingBol) {
        Row() {
          Button() {
            Image(this.getImageDefault())
              .width('60px').height('60px').fillColor('#FFFFFF')
          }
          .width('80px')
          .height('80px')
          .backgroundColor('rgba(255,255,255,0.20)')
          .borderRadius('40px')
          .onClick(() => {
            this.flashingBol = false
          })
        }
      } else {
        Flex({ justifyContent: FlexAlign.SpaceEvenly, alignItems: ItemAlign.Center }) {
          Image($r('app.media.ic_camera_public_flash_auto'))
            .width('60px')
            .height('60px')
            .fillColor(this.flashingNum == 2 ? $r('app.color.theme_color') : '')
            .onClick(() => {
              this.flashingNum = 2
              this.flashingBol = true
              cameraDemo.hasFlash(this.flashingNum)
            })
          Image($r('app.media.ic_camera_public_flash_off'))
            .width('60px')
            .height('60px')
            .fillColor(this.flashingNum == 0 ? $r('app.color.theme_color') : '')
            .onClick(() => {
              this.flashingNum = 0
              this.flashingBol = true
              cameraDemo.hasFlash(this.flashingNum)
            })
          Image($r('app.media.ic_camera_public_flash_on'))
            .width('60px')
            .height('60px')
            .fillColor(this.flashingNum == 1 ? $r('app.color.theme_color') : '')
            .onClick(() => {
              this.flashingNum = 1
              this.flashingBol = true
              cameraDemo.hasFlash(this.flashingNum)
            })
          Image($r('app.media.flash_always_on'))
            .width('50px')
            .height('50px')
            .fillColor(this.flashingNum == 3 ? $r('app.color.theme_color') : '')
            .onClick(() => {
              this.flashingNum = 3
              this.flashingBol = true
              cameraDemo.hasFlash(this.flashingNum)
            })
        }
        .backgroundColor('#FFFFFF')
        .borderRadius('40px')
        .width('300px')
        .height('80px')
        .zIndex(999)
      }

    }
    .position({ x: 30, y: 408 })
  }
}