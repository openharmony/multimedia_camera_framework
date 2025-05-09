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

#ifndef AVCODEC_AUDIO_DEFERRED_PROCESS_H
#define AVCODEC_AUDIO_DEFERRED_PROCESS_H

#include "refbase.h"
#include "audio_capturer.h"
#include "audio_record.h"
#include "audio_session_manager.h"
#include "offline_audio_effect_manager.h"
#include <cstdint>
#include <mutex>

namespace OHOS {
namespace CameraStandard {
using namespace AudioStandard;
class AudioDeferredProcess : public RefBase {
public:
    static constexpr int32_t ONE_THOUSAND = 1000;
    static constexpr int32_t DURATION_EACH_AUDIO_FRAME = 32;
    static constexpr int32_t PROCESS_BATCH_SIZE = 5;
    static constexpr int32_t MAX_UNPROCESSED_SIZE = 12288;
    static constexpr int32_t MAX_PROCESSED_SIZE = 2048;

    explicit AudioDeferredProcess();
    ~AudioDeferredProcess();

    void StoreOptions(const AudioStreamInfo& inputOptions, const AudioStreamInfo& outputOptions);
    int32_t ConfigOfflineAudioEffectChain();
    int32_t PrepareOfflineAudioEffectChain();
    int32_t GetMaxBufferSize(const AudioStreamInfo& inputOption, const AudioStreamInfo& outputOption);
    int32_t GetOfflineEffectChain();
    uint32_t GetOneUnprocessedSize();
    void FadeOneBatch(std::array<uint8_t, MAX_PROCESSED_SIZE * PROCESS_BATCH_SIZE>& processedArr);
    void EffectChainProcess(std::array<uint8_t, MAX_UNPROCESSED_SIZE * PROCESS_BATCH_SIZE>& rawArr,
        std::array<uint8_t, MAX_PROCESSED_SIZE * PROCESS_BATCH_SIZE>& processedArr);
    void ReturnToRecords(std::array<uint8_t, MAX_PROCESSED_SIZE * PROCESS_BATCH_SIZE>& processedArr,
        vector<sptr<AudioRecord>>& processedRecords, uint32_t i, uint32_t batchSize);
    int32_t Process(vector<sptr<AudioRecord>>& audioRecords, vector<sptr<AudioRecord>>& processedRecords);
    void Release();

private:
    std::string chainName_ = "offline_record_algo";
    std::mutex mutex_;
    std::unique_ptr<OfflineAudioEffectManager> offlineAudioEffectManager_ = nullptr;
    std::unique_ptr<OfflineAudioEffectChain> offlineEffectChain_ = nullptr;
    AudioStreamInfo inputOptions_;
    AudioStreamInfo outputOptions_;
    uint32_t oneUnprocessedSize_ = 0;
    uint32_t oneProcessedSize_ = 0;
};
} // CameraStandard
} // OHOS
#endif // AVCODEC_AUDIO_PRE_PROCESS_H