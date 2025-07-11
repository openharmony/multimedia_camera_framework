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

#include "test_common.h"
#include <cinttypes>
#include <cstdio>
#include <fcntl.h>
#include <memory>
#include <securec.h>
#include <sys/time.h>
#include <unistd.h>
#include "camera_output_capability.h"
#include "camera_util.h"
#include "camera_log.h"
#include "picture_proxy.h"
namespace OHOS {
namespace CameraStandard {

std::shared_ptr<PictureIntf> GetPictureIntfInstance()
{
    auto pictureProxy = PictureProxy::CreatePictureProxy();
    CHECK_PRINT_ELOG(pictureProxy == nullptr || pictureProxy.use_count() != 1,
        "pictureProxy use count is not 1");
    return pictureProxy;
}

camera_format_t TestUtils::GetCameraMetadataFormat(CameraFormat format)
{
    camera_format_t metaFormat = OHOS_CAMERA_FORMAT_YCRCB_420_SP;
    const std::unordered_map<CameraFormat, camera_format_t> mapToMetadataFormat = {
        {CAMERA_FORMAT_YUV_420_SP, OHOS_CAMERA_FORMAT_YCRCB_420_SP},
        {CAMERA_FORMAT_JPEG, OHOS_CAMERA_FORMAT_JPEG},
        {CAMERA_FORMAT_RGBA_8888, OHOS_CAMERA_FORMAT_RGBA_8888},
        {CAMERA_FORMAT_YCBCR_420_888, OHOS_CAMERA_FORMAT_YCBCR_420_888}
    };
    auto itr = mapToMetadataFormat.find(format);
    if (itr != mapToMetadataFormat.end()) {
        metaFormat = itr->second;
    }
    return metaFormat;
}
uint64_t TestUtils::GetCurrentLocalTimeStamp()
{
    std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp =
        std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    auto tmp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    return tmp.count();
}

int32_t TestUtils::SaveYUV(const char* buffer, int32_t size, SurfaceType type)
{
    char path[PATH_MAX] = {0};
    int32_t retVal;

    CHECK_RETURN_RET_ELOG((buffer == nullptr) || (size == 0), -1, "buffer is null or size is 0");

    MEDIA_DEBUG_LOG("TestUtils::SaveYUV(), type: %{public}d", type);
    if (type == SurfaceType::PREVIEW) {
        (void)system("mkdir -p /data/media/preview");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]), "/data/media/preview/%s_%lld.yuv", "preview",
                           GetCurrentLocalTimeStamp());
        CHECK_RETURN_RET_ELOG(retVal < 0, -1, "Path Assignment failed");
    } else if (type == SurfaceType::PHOTO) {
        (void)system("mkdir -p /data/media/photo");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]), "/data/media/photo/%s_%lld.jpg", "photo",
                           GetCurrentLocalTimeStamp());
        CHECK_RETURN_RET_ELOG(retVal < 0, -1, "Path Assignment failed");
    } else if (type == SurfaceType::SECOND_PREVIEW) {
        (void)system("mkdir -p /data/media/preview2");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]), "/data/media/preview2/%s_%lld.yuv", "preview2",
                           GetCurrentLocalTimeStamp());
        CHECK_RETURN_RET_ELOG(retVal < 0, -1, "Path Assignment failed");
    } else {
        MEDIA_ERR_LOG("Unexpected flow!");
        return -1;
    }

    MEDIA_DEBUG_LOG("%s, saving file to %{private}s", __FUNCTION__, path);
    int imgFd = open(path, O_RDWR | O_CREAT, FILE_PERMISSIONS_FLAG);
    CHECK_RETURN_RET_ELOG(imgFd == -1, -1,
        "%s, open file failed, errno = %{public}s.", __FUNCTION__, strerror(errno));
    fdsan_exchange_owner_tag(imgFd, 0, LOG_DOMAIN);
    int ret = write(imgFd, buffer, size);
    if (ret == -1) {
        MEDIA_ERR_LOG("%s, write file failed, error = %{public}s", __FUNCTION__, strerror(errno));
        fdsan_close_with_tag(imgFd, LOG_DOMAIN);
        return -1;
    }
    fdsan_close_with_tag(imgFd, LOG_DOMAIN);
    return 0;
}

bool TestUtils::IsNumber(const char number[])
{
    for (int i = 0; number[i] != 0; i++) {
        CHECK_RETURN_RET(!std::isdigit(number[i]), false);
    }
    return true;
}

int32_t TestUtils::SaveVideoFile(const char* buffer, int32_t size, VideoSaveMode operationMode, int32_t &fd)
{
    int32_t retVal = 0;

    if (operationMode == VideoSaveMode::CREATE) {
        char path[255] = {0};

        (void)system("mkdir -p /data/media/video");
        retVal = sprintf_s(path, sizeof(path) / sizeof(path[0]),
                           "/data/media/video/%s_%lld.h264", "video", GetCurrentLocalTimeStamp());
        CHECK_RETURN_RET_ELOG(retVal < 0, -1, "Failed to create video file name");
        MEDIA_DEBUG_LOG("%{public}s, save video to file %{private}s", __FUNCTION__, path);
        fd = open(path, O_RDWR | O_CREAT, FILE_PERMISSIONS_FLAG);
        if (fd == -1) {
            std::cout << "open file failed, errno = " << strerror(errno) << std::endl;
            return -1;
        }
        fdsan_exchange_owner_tag(fd, 0, LOG_DOMAIN);
    } else if (operationMode == VideoSaveMode::APPEND && fd != -1) {
        int32_t ret = write(fd, buffer, size);
        if (ret == -1) {
            std::cout << "write file failed, error = " << strerror(errno) << std::endl;
            fdsan_close_with_tag(fd, LOG_DOMAIN);
            fd = -1;
            return fd;
        }
    } else { // VideoSaveMode::CLOSE
        if (fd != -1) {
            fdsan_close_with_tag(fd, LOG_DOMAIN);
            fd = -1;
        }
    }
    return 0;
}

TestCameraMngerCallback::TestCameraMngerCallback(const char* testName) : testName_(testName) {
}

void TestCameraMngerCallback::OnCameraStatusChanged(const CameraStatusInfo &cameraStatusInfo) const
{
    MEDIA_DEBUG_LOG("OnCameraStatusChanged()");
    return;
}

void TestCameraMngerCallback::OnFlashlightStatusChanged(const std::string &cameraID,
                                                        const FlashStatus flashStatus) const
{
    MEDIA_DEBUG_LOG("OnFlashlightStatusChanged(), testName_: %{public}s, cameraID: %{public}s, flashStatus: %{public}d",
                    testName_, cameraID.c_str(), flashStatus);
    return;
}

TestDeviceCallback::TestDeviceCallback(const char* testName) : testName_(testName) {
}

void TestDeviceCallback::OnError(const int32_t errorType, const int32_t errorMsg) const
{
    MEDIA_DEBUG_LOG("TestDeviceCallback::OnError(), testName_: %{public}s, errorType: %{public}d, errorMsg: %{public}d",
                    testName_, errorType, errorMsg);
    return;
}

TestOnResultCallback::TestOnResultCallback(const char* testName) : testName_(testName) {
}

void TestOnResultCallback::OnResult(const uint64_t timestamp,
                                    const std::shared_ptr<Camera::CameraMetadata> &result) const
{
    MEDIA_DEBUG_LOG("TestOnResultCallback::OnResult(), testName_: %{public}s",
                    testName_);
    return;
}


TestPhotoOutputCallback::TestPhotoOutputCallback(const char* testName) : testName_(testName) {
}

void TestPhotoOutputCallback::OnCaptureStarted(const int32_t captureID) const
{
    MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureStarted(), testName_: %{public}s, captureID: %{public}d",
                   testName_, captureID);
}

void TestPhotoOutputCallback::OnCaptureStarted(const int32_t captureID, uint32_t exposureTime) const
{
    MEDIA_INFO_LOG("PhotoOutputCallback:OnCaptureStarted(), testName_: %{public}s, captureID: %{public}d",
                   testName_, captureID);
}

void TestPhotoOutputCallback::OnCaptureEnded(const int32_t captureID, const int32_t frameCount) const
{
    MEDIA_INFO_LOG("TestPhotoOutputCallback:OnCaptureEnded(), testName_: %{public}s, captureID: %{public}d,"
                   " frameCount: %{public}d", testName_, captureID, frameCount);
}

void TestPhotoOutputCallback::OnFrameShutter(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_INFO_LOG("OnFrameShutter(), testName_: %{public}s, captureID: %{public}d", testName_, captureId);
}

void TestPhotoOutputCallback::OnFrameShutterEnd(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_INFO_LOG("OnFrameShutterEnd(), testName_: %{public}s, captureID: %{public}d", testName_, captureId);
}

void TestPhotoOutputCallback::OnCaptureReady(const int32_t captureId, const uint64_t timestamp) const
{
    MEDIA_INFO_LOG("OnCaptureReady(), testName_: %{public}s, captureID: %{public}d", testName_, captureId);
}

void TestPhotoOutputCallback::OnEstimatedCaptureDuration(const int32_t duration) const
{
    MEDIA_INFO_LOG("OnEstimatedCaptureDuration(), duration: %{public}d", duration);
}

void TestPhotoOutputCallback::OnOfflineDeliveryFinished(const int32_t captureId) const
{
    MEDIA_INFO_LOG("OnOfflineDeliveryFinished(), captureId: %{public}d", captureId);
}

void TestPhotoOutputCallback::OnCaptureError(const int32_t captureId, const int32_t errorCode) const
{
    MEDIA_INFO_LOG("OnCaptureError(), testName_: %{public}s, captureID: %{public}d, errorCode: %{public}d",
                   testName_, captureId, errorCode);
}

TestPreviewOutputCallback::TestPreviewOutputCallback(const char* testName) : testName_(testName) {
}

void TestPreviewOutputCallback::OnFrameStarted() const
{
    MEDIA_INFO_LOG("TestPreviewOutputCallback:OnFrameStarted(), testName_: %{public}s", testName_);
}

void TestPreviewOutputCallback::OnFrameEnded(const int32_t frameCount) const
{
    MEDIA_INFO_LOG("TestPreviewOutputCallback:OnFrameEnded(), testName_: %{public}s, frameCount: %{public}d",
                   testName_, frameCount);
}

void TestPreviewOutputCallback::OnError(const int32_t errorCode) const
{
    MEDIA_INFO_LOG("TestPreviewOutputCallback:OnError(), testName_: %{public}s, errorCode: %{public}d",
                   testName_, errorCode);
}

void TestPreviewOutputCallback::OnSketchStatusDataChanged(const SketchStatusData& statusData) const
{
    MEDIA_DEBUG_LOG("TestPreviewOutputCallback::OnSketchStatusDataChanged(), testName_: %{public}s", testName_);
    return;
}

TestVideoOutputCallback::TestVideoOutputCallback(const char* testName) : testName_(testName) {
}

void TestVideoOutputCallback::OnFrameStarted() const
{
    MEDIA_INFO_LOG("TestVideoOutputCallback:OnFrameStarted(), testName_: %{public}s", testName_);
}

void TestVideoOutputCallback::OnFrameEnded(const int32_t frameCount) const
{
    MEDIA_INFO_LOG("TestVideoOutputCallback:OnFrameEnded(), testName_: %{public}s, frameCount: %{public}d",
                   testName_, frameCount);
}

void TestVideoOutputCallback::OnError(const int32_t errorCode) const
{
    MEDIA_INFO_LOG("TestVideoOutputCallback:OnError(), testName_: %{public}s, errorCode: %{public}d",
                   testName_, errorCode);
}

void TestVideoOutputCallback::OnDeferredVideoEnhancementInfo(const CaptureEndedInfoExt info) const
{
    MEDIA_INFO_LOG("TestVideoOutputCallback:OnDeferredVideoEnhancementInfo()");
}

TestMetadataOutputObjectCallback::TestMetadataOutputObjectCallback(const char* testName) : testName_(testName) {
}

void TestMetadataOutputObjectCallback::OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const
{
    MEDIA_INFO_LOG("TestMetadataOutputObjectCallback:OnMetadataObjectsAvailable(), testName_: %{public}s, "
                   "metaObjects size: %{public}zu", testName_, metaObjects.size());
    for (size_t i = 0; i < metaObjects.size(); i++) {
        MEDIA_INFO_LOG("TestMetadataOutputObjectCallback::OnMetadataObjectsAvailable "
                       "metaObjInfo: Type(%{public}d), Rect{x(%{pulic}f),y(%{pulic}f),w(%{pulic}f),d(%{pulic}f)} "
                       "Timestamp: %{public}" PRId64,
                       metaObjects[i]->GetType(),
                       metaObjects[i]->GetBoundingBox().topLeftX, metaObjects[i]->GetBoundingBox().topLeftY,
                       metaObjects[i]->GetBoundingBox().width, metaObjects[i]->GetBoundingBox().height,
                       static_cast<int64_t>(metaObjects[i]->GetTimestamp()));
    }
}

void TestDeferredPhotoProcSessionCallback::OnProcessImageDone(const std::string& imageId,
                                                              const uint8_t* addr,
                                                              const long bytes, uint32_t cloudImageEnhanceFlag)
{
    MEDIA_INFO_LOG("TestDeferredPhotoProcSessionCallback OnProcessImageDone.");
}

void TestDeferredPhotoProcSessionCallback::OnProcessImageDone(const std::string &imageId,
    std::shared_ptr<PictureIntf> picture, uint32_t cloudImageEnhanceFlag)
{
    MEDIA_INFO_LOG("TestDeferredPhotoProcSessionCallback OnProcessImageDone Picture.");
}

void TestDeferredPhotoProcSessionCallback::OnDeliveryLowQualityImage(const std::string &imageId,
    std::shared_ptr<PictureIntf> picture)
{
    MEDIA_INFO_LOG("TestDeferredPhotoProcSessionCallback OnDeliveryLowQualityImage.");
}

void TestDeferredPhotoProcSessionCallback::OnError(const std::string& imageId, const DpsErrorCode errorCode)
{
    MEDIA_INFO_LOG("TestDeferredPhotoProcSessionCallback OnError.");
}

void TestDeferredPhotoProcSessionCallback::OnStateChanged(const DpsStatusCode status)
{
    MEDIA_INFO_LOG("TestDeferredPhotoProcSessionCallback OnStateChanged.");
}

void TestDeferredVideoProcSessionCallback::OnProcessVideoDone(
    const std::string& videoId, const sptr<IPCFileDescriptor> ipcFd)
{
    MEDIA_INFO_LOG("TestDeferredVideoProcSessionCallback OnProcessImageDone Picture.");
}

void TestDeferredVideoProcSessionCallback::OnError(const std::string& videoId, const DpsErrorCode errorCode)
{
    MEDIA_INFO_LOG("TestDeferredVideoProcSessionCallback OnError.");
}

void TestDeferredVideoProcSessionCallback::OnStateChanged(const DpsStatusCode status)
{
    MEDIA_INFO_LOG("TestDeferredVideoProcSessionCallback OnStateChanged.");
}

SurfaceListener::SurfaceListener(const char* testName, SurfaceType type, int32_t &fd, sptr<IConsumerSurface> surface)
    : testName_(testName), surfaceType_(type), fd_(fd), surface_(surface) {
}

void SurfaceListener::OnBufferAvailable()
{
    int32_t flushFence = 0;
    int64_t timestamp = 0;
    OHOS::Rect damage;
    MEDIA_DEBUG_LOG("SurfaceListener::OnBufferAvailable(), testName_: %{public}s, surfaceType_: %{public}d",
                    testName_, surfaceType_);
    OHOS::sptr<OHOS::SurfaceBuffer> buffer = nullptr;
    CHECK_RETURN_ELOG(surface_ == nullptr, "OnBufferAvailable:surface_ is null");
    surface_->AcquireBuffer(buffer, flushFence, timestamp, damage);
    if (buffer != nullptr) {
        char* addr = static_cast<char *>(buffer->GetVirAddr());
        int32_t size = buffer->GetSize();

        switch (surfaceType_) {
            case SurfaceType::PREVIEW:
                if (previewIndex_ % TestUtils::PREVIEW_SKIP_FRAMES == 0
                    && TestUtils::SaveYUV(addr, size, surfaceType_) != CAMERA_OK) {
                    MEDIA_ERR_LOG("Failed to save buffer");
                    previewIndex_ = 0;
                }
                previewIndex_++;
                break;

            case SurfaceType::SECOND_PREVIEW:
                if (secondPreviewIndex_ % TestUtils::PREVIEW_SKIP_FRAMES == 0
                    && TestUtils::SaveYUV(addr, size, surfaceType_) != CAMERA_OK) {
                    MEDIA_ERR_LOG("Failed to save buffer");
                    secondPreviewIndex_ = 0;
                }
                secondPreviewIndex_++;
                break;

            case SurfaceType::PHOTO:
                CHECK_PRINT_ELOG(TestUtils::SaveYUV(addr, size, surfaceType_) != CAMERA_OK,
                    "Failed to save buffer");
                break;

            case SurfaceType::VIDEO:
                CHECK_PRINT_ELOG(fd_ == -1 && (TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CREATE, fd_) !=
                    CAMERA_OK), "Failed to Create video file");
                CHECK_PRINT_ELOG(TestUtils::SaveVideoFile(addr, size, VideoSaveMode::APPEND, fd_) != CAMERA_OK,
                    "Failed to save buffer");
                break;

            default:
                MEDIA_ERR_LOG("Unexpected type");
                break;
        }
        surface_->ReleaseBuffer(buffer, -1);
    } else {
        MEDIA_ERR_LOG("AcquireBuffer failed!");
    }
}
} // namespace CameraStandard
} // namespace OHOS

