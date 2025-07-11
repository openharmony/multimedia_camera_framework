/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "output/deferred_photo_proxy.h"

#include <sys/mman.h>
#include <unistd.h>

#include "camera_buffer_handle_utils.h"
#include "camera_log.h"
#include "output/deferred_photo_proxy.h"
#include "photo_proxy.h"

namespace OHOS {
namespace CameraStandard {

DeferredPhotoProxy::DeferredPhotoProxy()
{
    photoId_ = "";
    deferredProcType_ = 0;
    photoWidth_ = 0;
    photoHeight_ = 0;
    bufferHandle_ = nullptr;
    fileDataAddr_ = nullptr;
    fileSize_ = 0;
    isMmaped_ = false;
    buffer_ = nullptr;
}

DeferredPhotoProxy::DeferredPhotoProxy(const BufferHandle* bufferHandle,
    std::string imageId, int32_t deferredProcType)
{
    MEDIA_INFO_LOG("DeferredPhotoProxy");
    photoId_ = imageId;
    deferredProcType_ = deferredProcType;
    bufferHandle_ = bufferHandle;
    fileDataAddr_ = nullptr;
    photoWidth_ = 0;
    photoHeight_ = 0;
    fileSize_ = 0;
    isMmaped_ = false;
    buffer_ = nullptr;
    MEDIA_INFO_LOG("DeferredPhotoProxy imageId: = %{public}s, deferredProcType = %{public}d",
        imageId.c_str(), deferredProcType);
}

DeferredPhotoProxy::DeferredPhotoProxy(const BufferHandle* bufferHandle,
    std::string imageId, int32_t deferredProcType, int32_t photoWidth, int32_t photoHeight)
{
    MEDIA_INFO_LOG("DeferredPhotoProxy");
    photoId_ = imageId;
    deferredProcType_ = deferredProcType;
    photoWidth_ = photoWidth;
    photoHeight_ = photoHeight;
    bufferHandle_ = bufferHandle;
    fileDataAddr_ = nullptr;
    fileSize_ = 0;
    isMmaped_ = false;
    buffer_ = nullptr;
    MEDIA_INFO_LOG("imageId: = %{public}s, deferredProcType = %{public}d, width = %{public}d, height = %{public}d",
        imageId.c_str(), deferredProcType, photoWidth, photoHeight);
}

DeferredPhotoProxy::DeferredPhotoProxy(const BufferHandle* bufferHandle,
    std::string imageId, int32_t deferredProcType, uint8_t* buffer, size_t fileSize)
{
    MEDIA_INFO_LOG("DeferredPhotoProxy");
    photoId_ = imageId;
    deferredProcType_ = deferredProcType;
    photoWidth_ = 0;
    photoHeight_ = 0;
    bufferHandle_ = bufferHandle;
    fileDataAddr_ = nullptr;
    fileSize_ = fileSize;
    isMmaped_ = false;
    buffer_ = buffer;
    MEDIA_INFO_LOG("DeferredPhotoProxy imageId: = %{public}s, deferredProcType = %{public}d",
        imageId.c_str(), deferredProcType);
}

DeferredPhotoProxy::~DeferredPhotoProxy()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_INFO_LOG("~DeferredPhotoProxy");
    CHECK_EXECUTE(isMmaped_, munmap(fileDataAddr_, fileSize_));
    CameraFreeBufferHandle(const_cast<BufferHandle*>(bufferHandle_));
    fileDataAddr_ = nullptr;
    fileSize_ = 0;
    delete [] buffer_;
    buffer_ = nullptr;
}

void DeferredPhotoProxy::ReadFromParcel(MessageParcel &parcel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    photoId_ = parcel.ReadString();
    deferredProcType_ = parcel.ReadInt32();
    photoWidth_ = parcel.ReadInt32();
    photoHeight_ = parcel.ReadInt32();
    bufferHandle_ = ReadBufferHandle(parcel);
    MEDIA_INFO_LOG("DeferredPhotoProxy::ReadFromParcel");
}

void DeferredPhotoProxy::WriteToParcel(MessageParcel &parcel)
{
    std::lock_guard<std::mutex> lock(mutex_);
    parcel.WriteString(photoId_);
    parcel.WriteInt32(deferredProcType_);
    parcel.WriteInt32(photoWidth_);
    parcel.WriteInt32(photoHeight_);
    WriteBufferHandle(parcel, *bufferHandle_);
    MEDIA_INFO_LOG("DeferredPhotoProxy::WriteToParcel");
}

std::string DeferredPhotoProxy::GetPhotoId()
{
    MEDIA_INFO_LOG("DeferredPhotoProxy::GetPhotoId photoId: = %{public}s", photoId_.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    return photoId_;
}

Media::DeferredProcType DeferredPhotoProxy::GetDeferredProcType()
{
    MEDIA_INFO_LOG("DeferredPhotoProxy::GetDeferredProcType");
    std::lock_guard<std::mutex> lock(mutex_);
    if (deferredProcType_ == 0) {
        return Media::DeferredProcType::BACKGROUND;
    } else {
        return Media::DeferredProcType::OFFLINE;
    }
}

void* DeferredPhotoProxy::GetFileDataAddr()
{
    MEDIA_INFO_LOG("DeferredPhotoProxy::GetFileDataAddr");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_RETURN_RET_ELOG(buffer_ != nullptr, buffer_, "DeferredPhotoProxy::GetFileDataAddr get addr temp!");

    CHECK_RETURN_RET_ELOG(isMmaped_ == true, fileDataAddr_, "DeferredPhotoProxy::GetFileDataAddr mmap failed");
    if (!isMmaped_) {
        MEDIA_INFO_LOG("DeferredPhotoProxy::GetFileDataAddr mmap");
        fileDataAddr_ = mmap(nullptr, bufferHandle_->size, PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle_->fd, 0);
        CHECK_RETURN_RET_ELOG(
            fileDataAddr_ == MAP_FAILED, fileDataAddr_, "DeferredPhotoProxy::GetFileDataAddr mmap failed");
        isMmaped_ = true;
    }
    return fileDataAddr_;
}

Media::PhotoFormat DeferredPhotoProxy::GetFormat()
{
    return Media::PhotoFormat::RGBA;
}

Media::PhotoQuality DeferredPhotoProxy::GetPhotoQuality()
{
    return Media::PhotoQuality::LOW;
}

size_t DeferredPhotoProxy::GetFileSize()
{
    MEDIA_INFO_LOG("DeferredPhotoProxy::GetFileSize");
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_RETURN_RET_ELOG(buffer_ != nullptr, fileSize_,
        "DeferredPhotoProxy::GetFileSize temp!");
    fileSize_ = bufferHandle_->size;
    return fileSize_;
}

int32_t DeferredPhotoProxy::GetWidth()
{
    return photoWidth_;
}

int32_t DeferredPhotoProxy::GetHeight()
{
    return photoHeight_;
}

void DeferredPhotoProxy::Release()
{
    MEDIA_INFO_LOG("DeferredPhotoProxy release start");
}

std::string DeferredPhotoProxy::GetTitle()
{
    return "";
}

std::string DeferredPhotoProxy::GetExtension()
{
    return "";
}
double DeferredPhotoProxy::GetLatitude()
{
    return 0;
}
double DeferredPhotoProxy::GetLongitude()
{
    return 0;
}
std::string DeferredPhotoProxy::GetBurstKey()
{
    return "";
}
bool DeferredPhotoProxy::IsCoverPhoto()
{
    return false;
}
int32_t DeferredPhotoProxy::GetShootingMode()
{
    return 0;
}

uint32_t DeferredPhotoProxy::GetCloudImageEnhanceFlag()
{
    return 0;
}

} // namespace CameraStandard
} // namespace OHOS
