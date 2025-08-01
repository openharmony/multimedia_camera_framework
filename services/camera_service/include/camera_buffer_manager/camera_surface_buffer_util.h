/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_CAMERA_SURFACE_BUFFER_UTIL_H
#define OHOS_CAMERA_SURFACE_BUFFER_UTIL_H

#include "surface.h"
#include "video_key_info.h"
#include "metadata_helper.h"

namespace OHOS {
namespace CameraStandard {

class CameraSurfaceBufferUtil {
static constexpr uint8_t PIXEL_SIZE_HDR_YUV = 3;
static constexpr uint8_t HDR_PIXEL_SIZE = 2;

public:
    static sptr<SurfaceBuffer> DeepCopyBuffer(sptr<SurfaceBuffer> surfaceBuffer)
    {
        CAMERA_SYNC_TRACE;
        CHECK_RETURN_RET_ELOG(surfaceBuffer == nullptr, nullptr, "DeepCopyBuffer surfaceBuffer is null");
        uint32_t bufferSeqNum = surfaceBuffer->GetSeqNum();
        MEDIA_DEBUG_LOG("DeepCopyBuffer E bufferSeqNum:%{public}u", bufferSeqNum);
        DumpSurfaceBuffer(surfaceBuffer);
        int32_t dataStride = GetDataStride(surfaceBuffer) == -1 ? 0x8 : GetDataStride(surfaceBuffer);
        MEDIA_DEBUG_LOG("DeepCopyBuffer dataStride:%{public}d", dataStride);
        // deep copy buffer
        BufferRequestConfig requestConfig = {
            .width = surfaceBuffer->GetWidth(),
            .height = surfaceBuffer->GetHeight(),
            .strideAlignment = 0x8, // default stride is 8 Bytes.
            .format = surfaceBuffer->GetFormat(),
            .usage = surfaceBuffer->GetUsage(),
            .timeout = 0,
            .colorGamut = surfaceBuffer->GetSurfaceBufferColorGamut(),
            .transform = surfaceBuffer->GetSurfaceBufferTransform(),
        };
        sptr<SurfaceBuffer> newSurfaceBuffer = SurfaceBuffer::Create();
        auto allocErrorCode = newSurfaceBuffer->Alloc(requestConfig);
        MEDIA_DEBUG_LOG("DeepCopyBuffer alloc ret: %{public}d", allocErrorCode);
        if (memcpy_s(newSurfaceBuffer->GetVirAddr(), newSurfaceBuffer->GetSize(),
            surfaceBuffer->GetVirAddr(), surfaceBuffer->GetSize()) != EOK) {
            MEDIA_ERR_LOG("DeepCopyBuffer memcpy_s failed");
        }

        // deep copy buffer extData
        MessageParcel extParcl;
        sptr<BufferExtraData> bufferExtraData = surfaceBuffer->GetExtraData();
        GSError gsErr = bufferExtraData->WriteToParcel(extParcl);
        MEDIA_DEBUG_LOG("DeepCopyBuffer WriteToParcel gsErr=%{public}d", gsErr);
        sptr<BufferExtraData> newBufferExtraData = newSurfaceBuffer->GetExtraData();
        gsErr = newBufferExtraData->ReadFromParcel(extParcl);
        MEDIA_DEBUG_LOG("DeepCopyBuffer ReadFromParcel gsErr=%{public}d", gsErr);

        // deep metaData
        CopyMetaData(surfaceBuffer, newSurfaceBuffer);
        DumpSurfaceBuffer(newSurfaceBuffer);
        MEDIA_DEBUG_LOG("DeepCopyBuffer X bufferSeqNum:%{public}u", bufferSeqNum);
        return newSurfaceBuffer;
    }

    static sptr<SurfaceBuffer> DeepCopyThumbnailBuffer(sptr<SurfaceBuffer> surfaceBuffer)
    {
        CAMERA_SYNC_TRACE;
        CHECK_RETURN_RET_ELOG(surfaceBuffer == nullptr, nullptr, "DeepCopyThumbnailBuffer surfaceBuffer is null");
        uint32_t bufferSeqNum = surfaceBuffer->GetSeqNum();
        MEDIA_DEBUG_LOG("DeepCopyThumbnailBuffer E bufferSeqNum:%{public}u", bufferSeqNum);
        DumpSurfaceBuffer(surfaceBuffer);
        int32_t thumbnailStride = GetDataStride(surfaceBuffer);
        int32_t thumbnailHeight = GetDataHeight(surfaceBuffer);
        MEDIA_DEBUG_LOG("DeepCopyThumbnailBuffer thumbnailStride:%{public}d", thumbnailStride);
        // deep copy buffer
        BufferRequestConfig requestConfig = {
            .width = thumbnailStride,
            .height = thumbnailHeight,
            .strideAlignment = thumbnailStride,
            .format = surfaceBuffer->GetFormat(),
            .usage =
                BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_MEM_MMZ_CACHE,
            .timeout = 0,
        };
        sptr<SurfaceBuffer> newSurfaceBuffer = SurfaceBuffer::Create();
        auto allocRet = newSurfaceBuffer->Alloc(requestConfig);
        if (allocRet != 0) {
            MEDIA_ERR_LOG("DeepCopyThumbnailBuffer alloc ret: %{public}d", allocRet);
            return newSurfaceBuffer;
        }
        int32_t colorLength = thumbnailStride * thumbnailHeight * PIXEL_SIZE_HDR_YUV;
        HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType colorSpaceType;
        GSError gsErr = MetadataHelper::GetColorSpaceType(surfaceBuffer, colorSpaceType);
        bool isHdr = colorSpaceType ==  HDI::Display::Graphic::Common::V1_0::CM_ColorSpaceType::CM_BT2020_HLG_FULL;
        MEDIA_ERR_LOG("DeepCopyThumbnailBuffer colorSpaceType:%{public}d isHdr:%{public}d", colorSpaceType, isHdr);
        colorLength = isHdr ? colorLength : colorLength / HDR_PIXEL_SIZE;
        if (memcpy_s(newSurfaceBuffer->GetVirAddr(), newSurfaceBuffer->GetSize(),
            surfaceBuffer->GetVirAddr(), colorLength) != EOK) {
            MEDIA_ERR_LOG("DeepCopyThumbnailBuffer memcpy_s failed");
            return newSurfaceBuffer;
        }

        // deep copy buffer extData
        MessageParcel extParcl;
        sptr<BufferExtraData> bufferExtraData = surfaceBuffer->GetExtraData();
        gsErr = bufferExtraData->WriteToParcel(extParcl);
        MEDIA_DEBUG_LOG("DeepCopyBuffer WriteToParcel gsErr=%{public}d", gsErr);
        sptr<BufferExtraData> newBufferExtraData = newSurfaceBuffer->GetExtraData();
        gsErr = newBufferExtraData->ReadFromParcel(extParcl);
        MEDIA_DEBUG_LOG("DeepCopyBuffer ReadFromParcel gsErr=%{public}d", gsErr);

        // deep metaData
        CopyMetaData(surfaceBuffer, newSurfaceBuffer);
        DumpSurfaceBuffer(newSurfaceBuffer);
        MEDIA_DEBUG_LOG("DeepCopyBuffer X bufferSeqNum:%{public}u", bufferSeqNum);
        return newSurfaceBuffer;
    }

    static int32_t GetDataSize(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t dataSize = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::dataSize, dataSize);
        MEDIA_DEBUG_LOG("GetDataSize:%{public}d", dataSize);
        return dataSize;
    }

    static int32_t GetCaptureId(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t captureId = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::captureId, captureId);
        MEDIA_DEBUG_LOG("GetCaptureId:%{public}d", captureId);
        return captureId;
    }

    static int32_t GetMaskCaptureId(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t captureId;
        int32_t burstSeqId = -1;
        int32_t maskBurstSeqId = 0;
        int32_t invalidSeqenceId = -1;
        int32_t captureIdMask = 0x0000FFFF;
        int32_t captureIdShit = 16;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::burstSequenceId, burstSeqId);
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::captureId, captureId);
        if (burstSeqId != invalidSeqenceId && captureId >= 0) {
            maskBurstSeqId = ((static_cast<uint32_t>(captureId) & static_cast<uint32_t>(captureIdMask)) <<
                static_cast<uint32_t>(captureIdShit)) | static_cast<uint32_t>(burstSeqId);
            MEDIA_DEBUG_LOG("GetMaskCaptureId captureId:%{public}d, burstSeqId:%{public}d, maskBurstSeqId = %{public}d",
                captureId, burstSeqId, maskBurstSeqId);
            return maskBurstSeqId;
        }
        MEDIA_DEBUG_LOG("GetMaskCaptureId captureId:%{public}d, burstSeqId:%{public}d", captureId, burstSeqId);
        return captureId;
    }

    static int32_t GetBurstSequenceId(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t burstSequenceId = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::burstSequenceId, burstSequenceId);
        MEDIA_DEBUG_LOG("GetBurstSequenceId:%{public}d", burstSequenceId);
        return burstSequenceId;
    }

    static int32_t GetImageCount(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t imageCount = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::imageCount, imageCount);
        MEDIA_DEBUG_LOG("GetImageCount:%{public}d", imageCount);
        return imageCount;
    }

    static int64_t GetImageId(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int64_t imageId = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::imageId, imageId);
        MEDIA_DEBUG_LOG("GetImageId:%{public}s", std::to_string(imageId).c_str());
        return imageId;
    }

    static int32_t GetIsDegradedImage(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t isDegradedImage;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::isDegradedImage, isDegradedImage);
        MEDIA_DEBUG_LOG("GetIsDegradedImage:%{public}d", isDegradedImage);
        return isDegradedImage;
    }

    static int32_t GetDeferredProcessingType(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t deferredProcessingType;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::deferredProcessingType, deferredProcessingType);
        MEDIA_DEBUG_LOG("GetDeferredProcessingType:%{public}d", deferredProcessingType);
        return deferredProcessingType;
    }

    static int32_t GetDataWidth(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t dataWidth = 0;
        CHECK_RETURN_RET_ELOG(surfaceBuffer == nullptr, dataWidth, "GetDataWidth: surfaceBuffer is nullptr");
        sptr<BufferExtraData> extraData = surfaceBuffer->GetExtraData();
        CHECK_RETURN_RET_ELOG(extraData  == nullptr, dataWidth, "GetDataWidth: extraData is nullptr");
        extraData->ExtraGet(OHOS::Camera::dataWidth, dataWidth);
        MEDIA_DEBUG_LOG("GetDataWidth:%{public}d", dataWidth);
        return dataWidth;
    }

    static int32_t GetDataHeight(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t dataHeight = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::dataHeight, dataHeight);
        MEDIA_DEBUG_LOG("GetDataHeight:%{public}d", dataHeight);
        return dataHeight;
    }

    static int32_t GetDeferredImageFormat(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t deferredImageFormat = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::deferredImageFormat, deferredImageFormat);
        MEDIA_DEBUG_LOG("GetDeferredImageFormat:%{public}d", deferredImageFormat);
        return deferredImageFormat;
    }

    static int32_t GetDataStride(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t dataStride = -1;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::dataStride, dataStride);
        MEDIA_DEBUG_LOG("GetDataStride:%{public}d", dataStride);
        return dataStride;
    }

    static int32_t GetCloudImageEnhanceFlag(sptr<SurfaceBuffer> surfaceBuffer)
    {
        int32_t cloudImageEnhanceFlag = 0;
        surfaceBuffer->GetExtraData()->ExtraGet(OHOS::Camera::cloudImageEnhanceFlag, cloudImageEnhanceFlag);
        MEDIA_DEBUG_LOG("GetCloudImageEnhanceFlag:%{public}d", cloudImageEnhanceFlag);
        return cloudImageEnhanceFlag;
    }

    static void DumpSurfaceBuffer(sptr<SurfaceBuffer> surfaceBuffer)
    {
        GetCaptureId(surfaceBuffer);
        GetDataWidth(surfaceBuffer);
        GetDataHeight(surfaceBuffer);
        GetDeferredImageFormat(surfaceBuffer);
        GetIsDegradedImage(surfaceBuffer);
        GetImageId(surfaceBuffer);
        GetImageCount(surfaceBuffer);
        GetDeferredProcessingType(surfaceBuffer);
        GetBurstSequenceId(surfaceBuffer);
    }

private:
    static void CopyMetaData(sptr<SurfaceBuffer> &inBuffer, sptr<SurfaceBuffer> &outBuffer)
    {
        std::vector<uint32_t> keys = {};
        CHECK_RETURN_ELOG(inBuffer == nullptr, "CopyMetaData: inBuffer is nullptr");
        auto ret = inBuffer->ListMetadataKeys(keys);
        CHECK_RETURN_ELOG(ret != GSError::GSERROR_OK, "CopyMetaData: ListMetadataKeys fail! res=%{public}d", ret);
        for (uint32_t key : keys) {
            std::vector<uint8_t> values;
            ret = inBuffer->GetMetadata(key, values);
            if (ret != 0) {
                MEDIA_INFO_LOG("GetMetadata fail! key = %{public}d res = %{public}d", key, ret);
                continue;
            }
            ret = outBuffer->SetMetadata(key, values);
            if (ret != 0) {
                MEDIA_INFO_LOG("SetMetadata fail! key = %{public}d res = %{public}d", key, ret);
                continue;
            }
        }
    }
};
}  // namespace CameraStandard
}  // namespace OHOS
#endif
