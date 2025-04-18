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

#ifndef AVCODEC_SAMPLE_SAMPLE_INFO_H
#define AVCODEC_SAMPLE_SAMPLE_INFO_H
#include <cstdint>
#include <securec.h>
#include <string>
#include <condition_variable>
#include <queue>
#include "camera_log.h"
#include "native_avcodec_base.h"
#include "native_avbuffer.h"
#include "native_audio_channel_layout.h"
#include <refbase.h>
#include "avcodec_common.h"

namespace OHOS {
namespace CameraStandard {
constexpr std::string_view MIME_VIDEO_AVC = "video/avc";
constexpr std::string_view MIME_VIDEO_HEVC = "video/hevc";

constexpr int32_t BITRATE_10M = 10 * 1000 * 1000; // 10Mbps
constexpr int32_t BITRATE_20M = 20 * 1000 * 1000; // 20Mbps
constexpr int32_t BITRATE_22M = 22 * 1000 * 1000; // 22Mbps
constexpr int32_t BITRATE_30M = 30 * 1000 * 1000; // 30Mbps
constexpr uint32_t DEFAULT_SAMPLERATE = 48000;
constexpr uint32_t SAMPLERATE_32000 = 32000;
constexpr uint64_t DEFAULT_BITRATE = 48000;
constexpr uint32_t DEFAULT_CHANNEL_COUNT = 1;
constexpr uint32_t DEFAULT_PROFILE = 0;
constexpr int32_t AUDIO_ENCODE_EXPIREATION_TIME = 10;
constexpr OH_AudioChannelLayout CHANNEL_LAYOUT = OH_AudioChannelLayout::CH_LAYOUT_MONO;
constexpr OH_BitsPerSample SAMPLE_FORMAT = OH_BitsPerSample::SAMPLE_S16LE;
constexpr int32_t COMPLIANCE_LEVEL = 0;
constexpr OH_BitsPerSample BITS_PER_CODED_SAMPLE = OH_BitsPerSample::SAMPLE_S16LE;
constexpr uint32_t DEFAULT_MAX_INPUT_SIZE = 1024 * DEFAULT_CHANNEL_COUNT * sizeof(short);
constexpr int32_t VIDEO_FRAME_INTERVAL = 33333;
constexpr float VIDEO_FRAME_INTERVAL_MS = 33.33333;
constexpr int32_t AUDIO_FRAME_INTERVAL = 32000;
constexpr double VIDEO_FRAME_RATE = 30.0;
constexpr int32_t CACHE_FRAME_COUNT = 45;
constexpr size_t MAX_AUDIO_FRAME_COUNT = 140;
constexpr int32_t BUFFER_RELEASE_EXPIREATION_TIME = 150;
constexpr int32_t BUFFER_ENCODE_EXPIREATION_TIME = 20;
constexpr int32_t ROTATION_360 = 360;
constexpr OH_AVPixelFormat VIDOE_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV21;
constexpr int32_t IDR_FRAME_COUNT = 2;
constexpr int32_t KEY_FRAME_INTERVAL = 10;
const std::string TIMED_METADATA_TRACK_MIMETYPE = "meta/timed-metadata";
const std::string TIMED_METADATA_KEY = "com.openharmony.timed_metadata.movingphoto";
constexpr int32_t DEFAULT_SIZE = 1920 * 1440;
constexpr float VIDEO_BITRATE_CONSTANT = 0.7;
constexpr float HEVC_TO_AVC_FACTOR = 1.5;
constexpr int64_t NANOSEC_RANGE = 1600000000LL;
constexpr int32_t I32_TWO = 2;
constexpr bool IS_HDR_VIVID = true;

class CodecAVBufferInfo : public RefBase {
public:
    explicit CodecAVBufferInfo(uint32_t argBufferIndex, OH_AVBuffer *argBuffer)
        : bufferIndex(argBufferIndex), buffer(argBuffer)
    {
        // get output buffer attr
        OH_AVBuffer_GetBufferAttr(argBuffer, &attr);
    };
    ~CodecAVBufferInfo() = default;
    uint32_t bufferIndex = 0;
    OH_AVBuffer *buffer = nullptr;
    OH_AVCodecBufferAttr attr = {0, 0, 0, AVCODEC_BUFFER_FLAGS_NONE};

    OH_AVBuffer *GetCopyAVBuffer()
    {
        MEDIA_INFO_LOG("CodecBufferInfo OH_AVBuffer_Create with size: %{public}d", attr.size);
        OH_AVBuffer *destBuffer = OH_AVBuffer_Create(attr.size);
        CHECK_ERROR_RETURN_RET_LOG(destBuffer == nullptr, nullptr, "destBuffer is null");
        auto sourceAddr = OH_AVBuffer_GetAddr(buffer);
        auto destAddr = OH_AVBuffer_GetAddr(destBuffer);
        errno_t cpyRet = memcpy_s(reinterpret_cast<void *>(destAddr), attr.size,
                                  reinterpret_cast<void *>(sourceAddr), attr.size);
        if (cpyRet != 0) {
            MEDIA_ERR_LOG("CodecBufferInfo memcpy_s failed. %{public}d", cpyRet);
        }
        OH_AVErrCode errorCode = OH_AVBuffer_SetBufferAttr(destBuffer, &attr);
        if (errorCode != 0) {
            MEDIA_ERR_LOG("CodecBufferInfo OH_AVBuffer_SetBufferAttr failed. %{public}d", errorCode);
        }
        return destBuffer;
    }

    OH_AVBuffer *AddCopyAVBuffer(OH_AVBuffer *IDRBuffer)
    {
        if (IDRBuffer == nullptr) {
            MEDIA_WARNING_LOG("AddCopyAVBuffer without IDRBuffer!");
            return IDRBuffer;
        }
        OH_AVCodecBufferAttr IDRAttr = {0, 0, 0, AVCODEC_BUFFER_FLAGS_NONE};
        OH_AVCodecBufferAttr destAttr = {0, 0, 0, AVCODEC_BUFFER_FLAGS_NONE};
        OH_AVBuffer_GetBufferAttr(IDRBuffer, &IDRAttr);
        int32_t destBufferSize = IDRAttr.size + attr.size;
        OH_AVBuffer *destBuffer = OH_AVBuffer_Create(destBufferSize);
        CHECK_ERROR_RETURN_RET_LOG(destBuffer == nullptr, nullptr, "destBuffer is null");
        auto destAddr = OH_AVBuffer_GetAddr(destBuffer);
        auto sourceIDRAddr = OH_AVBuffer_GetAddr(IDRBuffer);
        
        errno_t cpyRet = memcpy_s(reinterpret_cast<void *>(destAddr), destBufferSize,
                                  reinterpret_cast<void *>(sourceIDRAddr), IDRAttr.size);
        if (cpyRet != 0) {
            MEDIA_ERR_LOG("CodecBufferInfo memcpy_s IDR frame failed. %{public}d", cpyRet);
        }
        destAddr = destAddr + IDRAttr.size;
        auto sourceAddr = OH_AVBuffer_GetAddr(buffer);
        cpyRet = memcpy_s(reinterpret_cast<void *>(destAddr), attr.size,
                          reinterpret_cast<void *>(sourceAddr), attr.size);
        if (cpyRet != 0) {
            MEDIA_ERR_LOG("CodecBufferInfo memcpy_s I frame failed. %{public}d", cpyRet);
        }
        OH_AVBuffer_Destroy(IDRBuffer);
        destAttr.size = destBufferSize;
        destAttr.flags = IDRAttr.flags | attr.flags;
        MEDIA_INFO_LOG("CodecBufferInfo deep copy with size: %{public}d, %{public}d", destBufferSize, destAttr.flags);
        OH_AVBuffer_SetBufferAttr(destBuffer, &destAttr);
        return destBuffer;
    }
};

class CodecUserData : public RefBase {
public:
    CodecUserData() = default;
    ~CodecUserData()
    {
        inputMutex_.lock();
        while (!inputBufferInfoQueue_.empty()) {
            inputBufferInfoQueue_.pop();
        }
        inputMutex_.unlock();
        outputMutex_.lock();
        while (!outputBufferInfoQueue_.empty()) {
            outputBufferInfoQueue_.pop();
        }
        outputMutex_.unlock();
    };
    uint32_t inputFrameCount_ = 0;
    std::mutex inputMutex_;
    std::condition_variable inputCond_;
    std::queue<sptr<CodecAVBufferInfo>> inputBufferInfoQueue_;

    uint32_t outputFrameCount_ = 0;
    std::mutex outputMutex_;
    std::condition_variable outputCond_;
    std::queue<sptr<CodecAVBufferInfo>> outputBufferInfoQueue_;
};

class VideoCodecAVBufferInfo : public RefBase {
public:
    explicit VideoCodecAVBufferInfo(uint32_t argBufferIndex, std::shared_ptr<OHOS::Media::AVBuffer> argBuffer)
        : bufferIndex(argBufferIndex), buffer(argBuffer)
    {
    }
    ~VideoCodecAVBufferInfo() = default;
    uint32_t bufferIndex = 0;
    std::shared_ptr<OHOS::Media::AVBuffer> buffer = nullptr;

    std::shared_ptr<OHOS::Media::AVBuffer> GetCopyAVBuffer()
    {
        MEDIA_INFO_LOG("CodecBufferInfo OH_AVBuffer_Create with size: %{public}d", buffer->memory_->GetSize());
        auto allocator = Media::AVAllocatorFactory::CreateSharedAllocator(Media::MemoryFlag::MEMORY_READ_WRITE);
        CHECK_ERROR_RETURN_RET_LOG(allocator == nullptr, nullptr, "create allocator failed");
        std::shared_ptr<Media::AVBuffer> destBuffer = Media::AVBuffer::CreateAVBuffer(allocator,
            buffer->memory_->GetCapacity());
        CHECK_ERROR_RETURN_RET_LOG(destBuffer == nullptr, nullptr, "destBuffer is null");
        auto sourceAddr = buffer->memory_->GetAddr();
        auto destAddr = destBuffer->memory_->GetAddr();
        errno_t cpyRet = memcpy_s(reinterpret_cast<void *>(destAddr), buffer->memory_->GetSize(),
                                  reinterpret_cast<void *>(sourceAddr), buffer->memory_->GetSize());
        if (cpyRet != 0) {
            MEDIA_ERR_LOG("CodecBufferInfo memcpy_s failed. %{public}d", cpyRet);
        }
        destBuffer->pts_ = buffer->pts_;
        destBuffer->flag_ = buffer->flag_;
        destBuffer->memory_->SetSize(buffer->memory_->GetSize());
        return destBuffer;
    }

    std::shared_ptr<OHOS::Media::AVBuffer> AddCopyAVBuffer(std::shared_ptr<OHOS::Media::AVBuffer> IDRBuffer)
    {
        if (IDRBuffer == nullptr) {
            MEDIA_WARNING_LOG("AddCopyAVBuffer without IDRBuffer!");
            return IDRBuffer;
        }
        int32_t destBufferSize = IDRBuffer->memory_->GetSize() + buffer->memory_->GetSize();
        auto allocator = Media::AVAllocatorFactory::CreateSharedAllocator(Media::MemoryFlag::MEMORY_READ_WRITE);
        CHECK_ERROR_RETURN_RET_LOG(allocator == nullptr, nullptr, "create allocator failed");
        std::shared_ptr<Media::AVBuffer> destBuffer = Media::AVBuffer::CreateAVBuffer(allocator, destBufferSize);
        CHECK_ERROR_RETURN_RET_LOG(destBuffer == nullptr, nullptr, "destBuffer is null");
        auto destAddr = destBuffer->memory_->GetAddr();
        auto sourceIDRAddr = IDRBuffer->memory_->GetAddr();
        errno_t cpyRet = memcpy_s(reinterpret_cast<void *>(destAddr), destBufferSize,
                                  reinterpret_cast<void *>(sourceIDRAddr), IDRBuffer->memory_->GetSize());
        if (cpyRet != 0) {
            MEDIA_ERR_LOG("CodecBufferInfo memcpy_s IDR frame failed. %{public}d", cpyRet);
        }
        destAddr = destAddr + IDRBuffer->memory_->GetSize();
        auto sourceAddr = buffer->memory_->GetAddr();
        cpyRet = memcpy_s(reinterpret_cast<void *>(destAddr), buffer->memory_->GetSize(),
                          reinterpret_cast<void *>(sourceAddr), buffer->memory_->GetSize());
        if (cpyRet != 0) {
            MEDIA_ERR_LOG("CodecBufferInfo memcpy_s I frame failed. %{public}d", cpyRet);
        }
        destBuffer->memory_->SetSize(destBufferSize);
        destBuffer->flag_ = IDRBuffer->flag_ | buffer->flag_;
        MEDIA_INFO_LOG("CodecBufferInfo copy with size: %{public}d, %{public}d", destBufferSize, destBuffer->flag_);
        return destBuffer;
    }
};

class VideoCodecUserData : public RefBase {
public:
    VideoCodecUserData() = default;
    ~VideoCodecUserData()
    {
        inputMutex_.lock();
        while (!inputBufferInfoQueue_.empty()) {
            inputBufferInfoQueue_.pop();
        }
        inputMutex_.unlock();
        outputMutex_.lock();
        while (!outputBufferInfoQueue_.empty()) {
            outputBufferInfoQueue_.pop();
        }
        outputMutex_.unlock();
    };
    uint32_t inputFrameCount_ = 0;
    std::mutex inputMutex_;
    std::condition_variable inputCond_;
    std::queue<sptr<VideoCodecAVBufferInfo>> inputBufferInfoQueue_;

    uint32_t outputFrameCount_ = 0;
    std::mutex outputMutex_;
    std::condition_variable outputCond_;
    std::queue<sptr<VideoCodecAVBufferInfo>> outputBufferInfoQueue_;
};
} // CameraStandard
} // OHOS
#endif // AVCODEC_SAMPLE_SAMPLE_INFO_H