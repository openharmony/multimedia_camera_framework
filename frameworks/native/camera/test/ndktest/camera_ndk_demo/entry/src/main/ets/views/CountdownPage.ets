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
 
// 倒计时页面
@Component
export struct CountdownPage {
  @Link countdownNum: number // 倒计时的值
  private countdownListData: Array<string> = ['2', '5', '10'] // 循环渲染
  @State countdownBol: boolean = true

  build() {
    Row() {
      if (this.countdownBol && !this.countdownNum) {
        Row() {
          Button() {
            Image($r('app.media.icon_camera_setting_timer'))
              .width('60px').height('60px')
          }
          .width('80px')
          .height('80px')
          .backgroundColor('rgba(255,255,255,0.20)')
          .borderRadius('40px')
          .onClick(() => {
            this.countdownBol = false
          })
        }
      }
      if (this.countdownNum && this.countdownBol) {
        Row() {
          Image($r('app.media.icon_camera_setting_timer_on')).width('60px').height('60px').margin({ left: 5 })
          Text(this.countdownNum + '').fontSize(21).fontWeight(500).margin({ left: 5 }).fontColor(Color.White)
        }
        .backgroundColor('rgba(255,255,255,0.20)')
        .borderRadius('40px')
        .width('140px')
        .height('80px')
        .onClick(() => {
          this.countdownBol = false
        })
      }
      if (!this.countdownBol) {
        Row() {
          Image($r('app.media.icon_camera_setting_timer_on_balk')).width('60px').height('60px').margin({ left: 5 })
          ForEach(this.countdownListData, (item) => {
            Text(item).fontSize(21).fontWeight(500).margin({
              left: 10
            })
              .fontColor(this.countdownNum == item ? $r('app.color.theme_color') : '#182431').onClick(() => {
              this.countdownNum = item
              this.countdownBol = true
            })
          }, item => item)
          Text('OFF').fontSize(21).fontWeight(500).margin({
            left: 10
          })
            .fontColor(this.countdownNum == 0 ? $r('app.color.theme_color') : '#182431').onClick(() => {
            this.countdownNum = 0
            this.countdownBol = true
          })
        }
        .backgroundColor('#FFFFFF')
        .borderRadius('40px')
        .width('360px')
        .height('80px')
        .zIndex(999)
      }
    }
    .position({ x: 30, y: 352 })
  }
}