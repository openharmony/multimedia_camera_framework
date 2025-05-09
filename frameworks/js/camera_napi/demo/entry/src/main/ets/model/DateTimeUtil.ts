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

/**
 * @file 日期工具
 */
export default class DateTimeUtil {

  /**
   * 时分秒
   */
  getTime(): string {
    const DATETIME = new Date();
    return this.concatTime(DATETIME.getHours(), DATETIME.getMinutes(), DATETIME.getSeconds());
  }

  /**
   * 年月日
   */
  getDate(): string {
    const DATETIME = new Date();
    return this.concatDate(DATETIME.getFullYear(), DATETIME.getMonth() + 1, DATETIME.getDate());
  }

  /**
   * 日期不足两位补充0
   * @param value-数据值
   */
  fill(value: number): string {
    let maxNumber = 9;
    return (value > maxNumber ? '' : '0') + value;
  }
  /**
   * 录制时间定时器
   * @param millisecond-数据值
   */
  getVideoTime(millisecond: number): string {
    let millisecond2minute = 60000;
    let millisecond2second = 1000;
    let minute = Math.floor(millisecond / millisecond2minute);
    let second = Math.floor((millisecond - minute * millisecond2minute) / millisecond2second);
    return `${this.fill(minute)} : ${this.fill(second)}`;
  }
  /**
   * 年月日格式修饰
   * @param year
   * @param month
   * @param date
   */
  concatDate(year: number, month: number, date: number): string {
    return `${year}${this.fill(month)}${this.fill(date)}`;
  }

  /**
   * 时分秒格式修饰
   * @param hours
   * @param minutes
   * @param seconds
   */
  concatTime(hours: number, minutes: number, seconds: number): string {
    return `${this.fill(hours)}${this.fill(minutes)}${this.fill(seconds)}`;
  }
}