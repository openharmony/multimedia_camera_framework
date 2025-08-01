/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "audio_encoder.h"
#include "native_avbuffer.h"
#include "sample_callback.h"
#include "utils/camera_log.h"

namespace OHOS {
namespace CameraStandard {

AudioEncoder::~AudioEncoder()
{
    MEDIA_INFO_LOG("~AudioEncoder enter");
    Release();
}

AudioEncoder::AudioEncoder()
{
    MEDIA_INFO_LOG("AudioEncoder Constructor enter");
    context_ = new CodecUserData;
}

int32_t AudioEncoder::Create(const std::string &codecMime)
{
    std::lock_guard<std::mutex> lock(encoderMutex_);
    encoder_ = OH_AudioCodec_CreateByMime(codecMime.data(), true);
    CHECK_RETURN_RET_ELOG(encoder_ == nullptr, 1, "Create failed");
    return 0;
}

int32_t AudioEncoder::Config()
{
    std::lock_guard<std::mutex> lock(encoderMutex_);
    CHECK_RETURN_RET_ELOG(encoder_ == nullptr, 1, "Encoder is null");
    // Configure audio encoder
    int32_t ret = Configure();
    CHECK_RETURN_RET_ELOG(ret != 0, 1, "Configure failed");
    // SetCallback for audio encoder
    CHECK_RETURN_RET_ELOG(context_ == nullptr, 1, "AudioEncoder has been released");
    ret = SetCallback(context_);
    CHECK_RETURN_RET_ELOG(ret != 0, 1, "Set callback failed");
    // Prepare audio encoder
    ret = OH_AudioCodec_Prepare(encoder_);
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1, "Prepare failed, ret: %{public}d", ret);
    return 0;
}

int32_t AudioEncoder::Start()
{
    std::lock_guard<std::mutex> lock(encoderMutex_);
    CHECK_RETURN_RET_ELOG(encoder_ == nullptr, 1, "Encoder is null");
    int ret = OH_AudioCodec_Start(encoder_);
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1, "Start failed, ret: %{public}d", ret);
    isStarted_ = true;
    return 0;
}

int32_t AudioEncoder::PushInputData(sptr<CodecAVBufferInfo> info)
{
    // LCOV_EXCL_START
    CHECK_RETURN_RET_ELOG(encoder_ == nullptr, 1, "Encoder is null");
    CHECK_RETURN_RET_ELOG(!isStarted_, 1, "Encoder is not started");
    int32_t ret = AV_ERR_OK;
    ret = OH_AVBuffer_SetBufferAttr(info->buffer, &info->attr);
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1, "Set avbuffer attr failed, ret: %{public}d", ret);
    ret = OH_AudioCodec_PushInputBuffer(encoder_, info->bufferIndex);
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1, "Push input data failed, ret: %{public}d", ret);
    return 0;
    // LCOV_EXCL_STOP
}

int32_t AudioEncoder::FreeOutputData(uint32_t bufferIndex)
{
    // LCOV_EXCL_START
    CHECK_RETURN_RET_ELOG(encoder_ == nullptr, 1, "Encoder is null");
    MEDIA_INFO_LOG("FreeOutputData bufferIndex: %{public}u", bufferIndex);
    int32_t ret = OH_AudioCodec_FreeOutputBuffer(encoder_, bufferIndex);
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1,
        "Free output data failed, ret: %{public}d", ret);
    return 0;
    // LCOV_EXCL_STOP
}

int32_t AudioEncoder::Stop()
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(encoderMutex_);
    CHECK_RETURN_RET_ELOG(isEncoding_.load(), 1, "Is Encoding.");
    CHECK_RETURN_RET_ELOG(encoder_ == nullptr, 1, "Encoder is null");
    int ret = OH_AudioCodec_Stop(encoder_);
    context_->Release();
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1, "Stop failed, ret: %{public}d", ret);
    isStarted_ = false;
    return 0;
}

int32_t AudioEncoder::Release()
{
    CAMERA_SYNC_TRACE;
    {
        std::lock_guard<std::mutex> lock(encoderMutex_);
        if (encoder_ != nullptr) {
            OH_AudioCodec_Destroy(encoder_);
            encoder_ = nullptr;
        }
        context_->Release();
    }
    isStarted_ = false;
    return 0;
}

void AudioEncoder::RestartAudioCodec()
{
    CAMERA_SYNC_TRACE;
    Release();
    Create(OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    Config();
    Start();
}

bool AudioEncoder::EnqueueBuffer(sptr<AudioRecord> audioRecord)
{
    // LCOV_EXCL_START
    CAMERA_SYNC_TRACE;
    uint8_t* buffer = audioRecord->GetAudioBuffer();
    CHECK_RETURN_RET_ELOG(buffer == nullptr, false, "Enqueue audio buffer is empty");
    int enqueueRetryCount = 2;
    while (enqueueRetryCount > 0) {
        std::unique_lock<std::mutex> encoderLock(encoderMutex_);
        enqueueRetryCount--;
        CHECK_RETURN_RET_ELOG(context_ == nullptr, false, "AudioEncoder has been released");
        std::unique_lock<std::mutex> lock(context_->inputMutex_);
        if (context_->inputBufferInfoQueue_.empty()) {
            bool condRet = context_->inputCond_.wait_for(lock,
                std::chrono::milliseconds(AUDIO_ENCODE_EXPIREATION_TIME),
                [this]() { return !isStarted_ || !context_->inputBufferInfoQueue_.empty(); });
        CHECK_CONTINUE_WLOG(context_->inputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);
        }
        sptr<CodecAVBufferInfo> bufferInfo = context_->inputBufferInfoQueue_.front();
        context_->inputBufferInfoQueue_.pop();
        context_->inputFrameCount_++;
        bufferInfo->attr.pts = audioRecord->GetTimeStamp();
        bufferInfo->attr.size = DEFAULT_MAX_INPUT_SIZE;
        bufferInfo->attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
        CHECK_RETURN_RET_ELOG(!isStarted_, false, "EnqueueBuffer while encoder is not started");
        auto bufferAddr = OH_AVBuffer_GetAddr(bufferInfo->buffer);
        int32_t bufferCap = OH_AVBuffer_GetCapacity(bufferInfo->buffer);
        CHECK_RETURN_RET_ELOG(bufferCap <= DEFAULT_MAX_INPUT_SIZE, false,
            "Destination buffer capacity is insufficient to hold the data.");
        errno_t cpyRet = memcpy_s(bufferAddr, bufferCap, buffer, DEFAULT_MAX_INPUT_SIZE);
        CHECK_RETURN_RET_ELOG(cpyRet != 0, false, "encoder memcpy_s failed. %{public}d", cpyRet);
        int32_t ret = PushInputData(bufferInfo);
        CHECK_RETURN_RET_ELOG(ret != 0, false, "Push data failed");
        MEDIA_DEBUG_LOG("Success frame id is : %{public}s", audioRecord->GetFrameId().c_str());
        return true;
    }
    MEDIA_ERR_LOG("Failed frame id is : %{public}s", audioRecord->GetFrameId().c_str());
    return false;
    // LCOV_EXCL_STOP
}

bool AudioEncoder::EncodeAudioBuffer(sptr<AudioRecord> audioRecord)
{
    // LCOV_EXCL_START
    CAMERA_SYNC_TRACE;
    {
        std::lock_guard<std::mutex> lock(encoderMutex_);
        CHECK_RETURN_RET(encoder_ == nullptr, false);
    }
    audioRecord->SetStatusReadyConvertStatus();
    CHECK_RETURN_RET(!EnqueueBuffer(audioRecord), false);
    int retryCount = 3;
    while (retryCount > 0) {
        std::unique_lock<std::mutex> encoderLock(encoderMutex_);
        retryCount--;
        CHECK_RETURN_RET_ELOG(context_ == nullptr, false, "AudioEncoder has been released");
        std::unique_lock<std::mutex> lock(context_->outputMutex_);
        if (context_->outputBufferInfoQueue_.empty()) {
            bool condRet = context_->outputCond_.wait_for(lock,
                std::chrono::milliseconds(AUDIO_ENCODE_EXPIREATION_TIME),
                [this]() { return !isStarted_ || !context_->outputBufferInfoQueue_.empty(); });
        CHECK_CONTINUE_WLOG(context_->outputBufferInfoQueue_.empty(),
            "Buffer queue is empty, continue, cond ret: %{public}d", condRet);
        }
        sptr<CodecAVBufferInfo> bufferInfo = context_->outputBufferInfoQueue_.front();
        context_->outputBufferInfoQueue_.pop();
        context_->outputFrameCount_++;
        MEDIA_DEBUG_LOG("Out buffer count: %{public}u, size: %{public}d, flag: %{public}u, pts:%{public}" PRIu64,
            context_->outputFrameCount_, bufferInfo->attr.size, bufferInfo->attr.flags, bufferInfo->attr.pts);
        lock.unlock();
        OH_AVBuffer *audioBuffer = bufferInfo->GetCopyAVBuffer();
        audioRecord->CacheEncodedBuffer(audioBuffer);
        int32_t ret = FreeOutputData(bufferInfo->bufferIndex);
        CHECK_BREAK_WLOG(ret != 0, "FreeOutputData failed");
        MEDIA_DEBUG_LOG("Success frame id is : %{public}s", audioRecord->GetFrameId().c_str());
        audioRecord->SetEncodedResult(true);
        return true;
    }
    MEDIA_ERR_LOG("Failed frame id is : %{public}s", audioRecord->GetFrameId().c_str());
    return false;
    // LCOV_EXCL_STOP
}

bool AudioEncoder::EncodeAudioBuffer(vector<sptr<AudioRecord>> audioRecords)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(serialMutex_);
    MEDIA_INFO_LOG("EncodeAudioBuffer enter");
    CHECK_EXECUTE(!isStarted_.load(), RestartAudioCodec());
    bool isSuccess = true;
    isEncoding_.store(true);
    // LCOV_EXCL_START
    for (sptr<AudioRecord> audioRecord : audioRecords) {
        if (!audioRecord->IsEncoded() && !EncodeAudioBuffer(audioRecord)) {
            isSuccess = false;
            MEDIA_ERR_LOG("Failed frame id is : %{public}s", audioRecord->GetFrameId().c_str());
        }
    }
    // LCOV_EXCL_STOP
    isEncoding_.store(false);
    return isSuccess;
}


int32_t AudioEncoder::SetCallback(sptr<CodecUserData> codecUserData)
{
    int32_t ret = OH_AudioCodec_RegisterCallback(encoder_,
        {SampleCallback::OnCodecError, SampleCallback::OnOutputFormatChanged,
        SampleCallback::OnInputBufferAvailable, SampleCallback::OnOutputBufferAvailable},
        static_cast<void *>(codecUserData));
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1, "Set callback failed, ret: %{public}d", ret);
    return 0;
}

int32_t AudioEncoder::Configure()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    CHECK_RETURN_RET_ELOG(format == nullptr, 1, "AVFormat create failed");

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, DEFAULT_CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLERATE_32000);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, DEFAULT_MAX_INPUT_SIZE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, DEFAULT_PROFILE);
    int ret = OH_AudioCodec_Configure(encoder_, format);
    OH_AVFormat_Destroy(format);
    format = nullptr;
    CHECK_RETURN_RET_ELOG(ret != AV_ERR_OK, 1, "Config failed, ret: %{public}d", ret);
    return 0;
}
} // CameraStandard
} // OHOS