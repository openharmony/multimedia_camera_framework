/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "photo_post_processor.h"

#include <sys/mman.h>

#include "auxiliary_picture.h"
#include "buffer_extra_data_impl.h"
#include "camera_dynamic_loader.h"
#include "dp_log.h"
#include "dp_timer.h"
#include "dp_utils.h"
#include "dps_event_report.h"
#include "events_monitor.h"
#include "foundation/multimedia/media_library/interfaces/inner_api/media_library_helper/include/photo_proxy.h"
#include "iproxy_broker.h"
#include "iservmgr_hdi.h"
#include "picture_proxy.h"
#include "securec.h"
#include "v1_3/iimage_process_service.h"
#include "v1_3/iimage_process_callback.h"
#include "v1_3/types.h"

namespace OHOS {
namespace CameraStandard {
namespace DeferredProcessing {
namespace {
    const std::string PHOTO_SERVICE_NAME = "camera_image_process_service";
    constexpr uint32_t MAX_PROC_TIME_MS = 11 * 1000;
    constexpr uint32_t MAX_CONSECUTIVE_TIMEOUT_COUNT = 3;
    constexpr uint32_t MAX_CONSECUTIVE_CRASH_COUNT = 3;
    constexpr int32_t HDI_VERSION_1 = 1;
    constexpr int32_t HDI_VERSION_3 = 3;
}

DpsError MapHdiError(OHOS::HDI::Camera::V1_2::ErrorCode errorCode)
{
    DpsError code = DpsError::DPS_ERROR_IMAGE_PROC_ABNORMAL;
    switch (errorCode) {
        case OHOS::HDI::Camera::V1_2::ErrorCode::ERROR_INVALID_ID:
            code = DpsError::DPS_ERROR_IMAGE_PROC_INVALID_PHOTO_ID;
            break;
        case OHOS::HDI::Camera::V1_2::ErrorCode::ERROR_PROCESS:
            code = DpsError::DPS_ERROR_IMAGE_PROC_FAILED;
            break;
        case OHOS::HDI::Camera::V1_2::ErrorCode::ERROR_TIMEOUT:
            code = DpsError::DPS_ERROR_IMAGE_PROC_TIMEOUT;
            break;
        case OHOS::HDI::Camera::V1_2::ErrorCode::ERROR_HIGH_TEMPERATURE:
            code = DpsError::DPS_ERROR_IMAGE_PROC_HIGH_TEMPERATURE;
            break;
        case OHOS::HDI::Camera::V1_2::ErrorCode::ERROR_ABNORMAL:
            code = DpsError::DPS_ERROR_IMAGE_PROC_ABNORMAL;
            break;
        case OHOS::HDI::Camera::V1_2::ErrorCode::ERROR_ABORT:
            code = DpsError::DPS_ERROR_IMAGE_PROC_INTERRUPTED;
            break;
        default:
            DP_ERR_LOG("unexpected error code: %{public}d.", errorCode);
            break;
    }
    return code;
}

HdiStatus MapHdiStatus(OHOS::HDI::Camera::V1_2::SessionStatus statusCode)
{
    HdiStatus code = HdiStatus::HDI_DISCONNECTED;
    switch (statusCode) {
        case OHOS::HDI::Camera::V1_2::SessionStatus::SESSION_STATUS_READY:
            code = HdiStatus::HDI_READY;
            break;
        case OHOS::HDI::Camera::V1_2::SessionStatus::SESSION_STATUS_READY_SPACE_LIMIT_REACHED:
            code = HdiStatus::HDI_READY_SPACE_LIMIT_REACHED;
            break;
        case OHOS::HDI::Camera::V1_2::SessionStatus::SESSSON_STATUS_NOT_READY_TEMPORARILY:
            code = HdiStatus::HDI_NOT_READY_TEMPORARILY;
            break;
        case OHOS::HDI::Camera::V1_2::SessionStatus::SESSION_STATUS_NOT_READY_OVERHEAT:
            code = HdiStatus::HDI_NOT_READY_OVERHEAT;
            break;
        case OHOS::HDI::Camera::V1_2::SessionStatus::SESSION_STATUS_NOT_READY_PREEMPTED:
            code = HdiStatus::HDI_NOT_READY_PREEMPTED;
            break;
        default:
            DP_ERR_LOG("unexpected error code: %{public}d.", statusCode);
            break;
    }
    return code;
}

OHOS::HDI::Camera::V1_2::ExecutionMode MapToHdiExecutionMode(ExecutionMode executionMode)
{
    auto mode = OHOS::HDI::Camera::V1_2::ExecutionMode::LOW_POWER;
    switch (executionMode) {
        case ExecutionMode::HIGH_PERFORMANCE:
            mode = OHOS::HDI::Camera::V1_2::ExecutionMode::HIGH_PREFORMANCE;
            break;
        case ExecutionMode::LOAD_BALANCE:
            mode = OHOS::HDI::Camera::V1_2::ExecutionMode::BALANCED;
            break;
        case ExecutionMode::LOW_POWER:
            mode = OHOS::HDI::Camera::V1_2::ExecutionMode::LOW_POWER;
            break;
        default:
            DP_ERR_LOG("unexpected error code: %{public}d.", executionMode);
            break;
    }
    return mode;
}

class PhotoPostProcessor::PhotoServiceListener : public HDI::ServiceManager::V1_0::ServStatListenerStub {
public:
    using StatusCallback = std::function<void(const HDI::ServiceManager::V1_0::ServiceStatus&)>;
    explicit PhotoServiceListener(const std::weak_ptr<PhotoPostProcessor>& processor) : processor_(processor)
    {
        DP_DEBUG_LOG("entered.");
    }

    void OnReceive(const HDI::ServiceManager::V1_0::ServiceStatus& status)
    {
        auto process = processor_.lock();
        DP_CHECK_ERROR_RETURN_LOG(process == nullptr, "photo post process is nullptr.");
        process->OnServiceChange(status);
    }

private:
    std::weak_ptr<PhotoPostProcessor> processor_;
};

class PhotoPostProcessor::SessionDeathRecipient : public IRemoteObject::DeathRecipient {
public:
    explicit SessionDeathRecipient(const std::weak_ptr<PhotoProcessResult>& processResult)
        : processResult_(processResult)
    {
        DP_DEBUG_LOG("entered.");
    }

    void OnRemoteDied(const wptr<IRemoteObject> &remote) override
    {
        if (auto processResult = processResult_.lock()) {
            processResult->OnPhotoSessionDied();
        }
    }

private:
    std::weak_ptr<PhotoProcessResult> processResult_;
};

class PhotoPostProcessor::PhotoProcessListener : public OHOS::HDI::Camera::V1_3::IImageProcessCallback {
public:
    explicit PhotoProcessListener(const int32_t userId, const std::weak_ptr<PhotoProcessResult>& processResult)
        : userId_(userId), processResult_(processResult)
    {
        DP_DEBUG_LOG("entered.");
    }

    int32_t OnProcessDone(const std::string& imageId, const OHOS::HDI::Camera::V1_2::ImageBufferInfo& buffer) override;
    int32_t OnProcessDoneExt(const std::string& imageId,
        const OHOS::HDI::Camera::V1_3::ImageBufferInfoExt& buffer) override;
    int32_t OnError(const std::string& imageId,  OHOS::HDI::Camera::V1_2::ErrorCode errorCode) override;
    int32_t OnStatusChanged(OHOS::HDI::Camera::V1_2::SessionStatus status) override;

private:
    void ReportEvent(const std::string& imageId);
    int32_t ProcessBufferInfo(const std::string& imageId, const OHOS::HDI::Camera::V1_2::ImageBufferInfo& buffer);
    int32_t ProcessBufferInfoExt(const std::string& imageId,
        const OHOS::HDI::Camera::V1_3::ImageBufferInfoExt& buffer);
    std::shared_ptr<PictureIntf> AssemblePicture(const OHOS::HDI::Camera::V1_3::ImageBufferInfoExt& buffer);

    const int32_t userId_;
    std::weak_ptr<PhotoProcessResult> processResult_;
};

int32_t PhotoPostProcessor::PhotoProcessListener::OnProcessDone(const std::string& imageId,
    const OHOS::HDI::Camera::V1_2::ImageBufferInfo& buffer)
{
    DP_INFO_LOG("DPS_PHOTO: imageId: %{public}s", imageId.c_str());
    auto ret = ProcessBufferInfo(imageId, buffer);
    if (ret != DP_OK) {
        DP_ERR_LOG("process done failed imageId: %{public}s.", imageId.c_str());
        if (auto processResult = processResult_.lock()) {
            processResult->OnError(imageId, DPS_ERROR_IMAGE_PROC_FAILED);
        }
    }
    return DP_OK;
}

int32_t PhotoPostProcessor::PhotoProcessListener::ProcessBufferInfo(const std::string& imageId,
    const OHOS::HDI::Camera::V1_2::ImageBufferInfo& buffer)
{
    auto bufferHandle = buffer.imageHandle->GetBufferHandle();
    DP_CHECK_ERROR_RETURN_RET_LOG(bufferHandle == nullptr, DPS_ERROR_IMAGE_PROC_FAILED, "bufferHandle is nullptr.");

    int32_t size = bufferHandle->size;
    int32_t isDegradedImage = 0;
    int32_t dataSize = size;
    uint32_t cloudImageEnhanceFlag = 0;
    if (buffer.metadata) {
        int32_t retImageQuality = buffer.metadata->Get("isDegradedImage", isDegradedImage);
        int32_t retDataSize = buffer.metadata->Get("dataSize", dataSize);
        int32_t retCloudImageEnhanceFlag = buffer.metadata->Get("cloudImageEnhanceFlag",
            cloudImageEnhanceFlag);
        DP_DEBUG_LOG("retImageQuality: %{public}d, retDataSize: %{public}d, retCloudImageEnhanceFlag: %{public}d",
            static_cast<int>(retImageQuality), static_cast<int>(retDataSize),
            static_cast<int>(retCloudImageEnhanceFlag));
    }
    DP_INFO_LOG("DPS_PHOTO: bufferHandle param, size: %{public}d, dataSize: %{public}d, isDegradedImage: %{public}d",
        size, static_cast<int>(dataSize), isDegradedImage);
    auto bufferPtr = std::make_shared<SharedBuffer>(dataSize);
    DP_CHECK_ERROR_RETURN_RET_LOG(bufferPtr->Initialize() != DP_OK, DPS_ERROR_IMAGE_PROC_FAILED,
        "failed to initialize shared buffer.");

    auto addr = mmap(nullptr, dataSize, PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0);
    DP_CHECK_ERROR_RETURN_RET_LOG(addr == MAP_FAILED, DPS_ERROR_IMAGE_PROC_FAILED, "failed to mmap shared buffer.");

    if (bufferPtr->CopyFrom(static_cast<uint8_t*>(addr), dataSize) == DP_OK) {
        DP_INFO_LOG("DPS_PHOTO: bufferPtr fd: %{public}d, fd: %{public}d, cloudImageEnhanceFlag: %{public}u",
            bufferHandle->fd, bufferPtr->GetFd(), cloudImageEnhanceFlag);
        std::shared_ptr<BufferInfo> bufferInfo = std::make_shared<BufferInfo>(bufferPtr, dataSize,
            isDegradedImage == 0, cloudImageEnhanceFlag);
        auto processResult = processResult_.lock();
        if (processResult) {
            processResult->OnProcessDone(imageId, bufferInfo);
        }
    }
    munmap(addr, dataSize);
    ReportEvent(imageId);
    return DP_OK;
}

BufferHandle *CloneBufferHandle(const BufferHandle *handle)
{
    if (handle == nullptr) {
        DP_ERR_LOG("%{public}s handle is nullptr", __func__);
        return nullptr;
    }

    BufferHandle *newHandle = AllocateBufferHandle(handle->reserveFds, handle->reserveInts);
    if (newHandle == nullptr) {
        DP_ERR_LOG("%{public}s AllocateBufferHandle failed, newHandle is nullptr", __func__);
        return nullptr;
    }

    if (handle->fd == -1) {
        newHandle->fd = handle->fd;
    } else {
        newHandle->fd = dup(handle->fd);
        if (newHandle->fd == -1) {
            DP_ERR_LOG("CloneBufferHandle dup failed");
            FreeBufferHandle(newHandle);
            return nullptr;
        }
    }
    newHandle->width = handle->width;
    newHandle->stride = handle->stride;
    newHandle->height = handle->height;
    newHandle->size = handle->size;
    newHandle->format = handle->format;
    newHandle->usage = handle->usage;
    newHandle->phyAddr = handle->phyAddr;

    for (uint32_t i = 0; i < newHandle->reserveFds; i++) {
        newHandle->reserve[i] = dup(handle->reserve[i]);
        if (newHandle->reserve[i] == -1) {
            DP_ERR_LOG("CloneBufferHandle dup reserveFds failed");
            FreeBufferHandle(newHandle);
            return nullptr;
        }
    }

    if (handle->reserveInts == 0) {
        DP_ERR_LOG("There is no reserved integer value in old handle, no need to copy");
        return newHandle;
    }

    if (memcpy_s(&newHandle->reserve[newHandle->reserveFds], sizeof(int32_t) * newHandle->reserveInts,
        &handle->reserve[handle->reserveFds], sizeof(int32_t) * handle->reserveInts) != EOK) {
        DP_ERR_LOG("CloneBufferHandle memcpy_s failed");
        FreeBufferHandle(newHandle);
        return nullptr;
    }
    return newHandle;
}

void DpCopyMetaData(sptr<SurfaceBuffer> &inBuffer, sptr<SurfaceBuffer> &outBuffer)
{
    std::vector<uint32_t> keys = {};
    DP_CHECK_ERROR_RETURN_LOG(inBuffer == nullptr, "DpCopyMetaData: inBuffer is nullptr");
    auto ret = inBuffer->ListMetadataKeys(keys);
    DP_CHECK_ERROR_RETURN_LOG(ret != GSError::GSERROR_OK, "DpCopyMetaData: ListMetadataKeys fail!");
    for (uint32_t key : keys) {
        std::vector<uint8_t> values;
        ret = inBuffer->GetMetadata(key, values);
        if (ret != 0) {
            DP_INFO_LOG("GetMetadata fail! key = %{public}d res = %{public}d", key, ret);
            continue;
        }
        ret = outBuffer->SetMetadata(key, values);
        if (ret != 0) {
            DP_INFO_LOG("SetMetadata fail! key = %{public}d res = %{public}d", key, ret);
            continue;
        }
    }
}

sptr<SurfaceBuffer> DpCopyBuffer(sptr<SurfaceBuffer> surfaceBuffer)
{
    DP_DEBUG_LOG("DpCopyBuffer w=%{public}d, h=%{public}d, f=%{public}d",
        surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight(), surfaceBuffer->GetFormat());
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
    DP_DEBUG_LOG("DpCopyBuffer SurfaceBuffer alloc ret: %{public}d", allocErrorCode);
    if (allocErrorCode != GSError::GSERROR_OK) {
        DP_ERR_LOG("DpCopyBuffer Alloc faled!, errCode:%{public}d", static_cast<int32_t>(allocErrorCode));
        return nullptr;
    }
    errno_t errNo = memcpy_s(newSurfaceBuffer->GetVirAddr(), newSurfaceBuffer->GetSize(),
        surfaceBuffer->GetVirAddr(), newSurfaceBuffer->GetSize());
    DP_CHECK_ERROR_RETURN_RET_LOG(errNo != EOK, nullptr, "DpCopyBuffer memcpy_s failed");
    DpCopyMetaData(surfaceBuffer, newSurfaceBuffer);
    DP_DEBUG_LOG("DpCopyBuffer memcpy end");
    return newSurfaceBuffer;
}

sptr<SurfaceBuffer> TransBufferHandleToSurfaceBuffer(BufferHandle *bufferHandle)
{
    DP_DEBUG_LOG("entered");
    DP_CHECK_ERROR_RETURN_RET_LOG(bufferHandle == nullptr, nullptr, "bufferHandle is nullptr.");
    sptr<SurfaceBuffer> surfaceBuffer = SurfaceBuffer::Create();
    surfaceBuffer->SetBufferHandle(CloneBufferHandle(bufferHandle));
    DP_INFO_LOG("TransBufferHandleToSurfaceBuffer w=%{public}d, h=%{public}d, f=%{public}d",
        surfaceBuffer->GetWidth(), surfaceBuffer->GetHeight(), surfaceBuffer->GetFormat());
    return DpCopyBuffer(surfaceBuffer);
}

void SetAuxiliaryPicture(std::shared_ptr<PictureIntf> picture, BufferHandle *bufferHandle,
    CameraAuxiliaryPictureType type)
{
    DP_INFO_LOG("entered, AuxiliaryPictureType type = %{public}d", static_cast<int32_t>(type));
    DP_CHECK_ERROR_RETURN_LOG(picture == nullptr || bufferHandle == nullptr, "bufferHandle is nullptr.");

    auto buffer = TransBufferHandleToSurfaceBuffer(bufferHandle);
    picture->SetAuxiliaryPicture(buffer, type);
}

void AssemleAuxilaryPicture(const OHOS::HDI::Camera::V1_3::ImageBufferInfoExt& buffer,
    std::shared_ptr<PictureIntf>& picture)
{
    if (buffer.isGainMapValid) {
        SetAuxiliaryPicture(picture, buffer.gainMapHandle->GetBufferHandle(),
            CameraAuxiliaryPictureType::GAINMAP);
    }
    if (buffer.isDepthMapValid) {
        SetAuxiliaryPicture(picture, buffer.depthMapHandle->GetBufferHandle(),
            CameraAuxiliaryPictureType::DEPTH_MAP);
    }
    if (buffer.isUnrefocusImageValid) {
        SetAuxiliaryPicture(picture, buffer.unrefocusImageHandle->GetBufferHandle(),
            CameraAuxiliaryPictureType::UNREFOCUS_MAP);
    }
    if (buffer.isHighBitDepthLinearImageValid) {
        SetAuxiliaryPicture(picture, buffer.highBitDepthLinearImageHandle->GetBufferHandle(),
            CameraAuxiliaryPictureType::LINEAR_MAP);
    }
    if (buffer.isMakerInfoValid) {
        auto makerInfoBuffer = TransBufferHandleToSurfaceBuffer(buffer.makerInfoHandle->GetBufferHandle());
        picture->SetMaintenanceData(makerInfoBuffer);
    }
}

std::shared_ptr<PictureIntf> PhotoPostProcessor::PhotoProcessListener::AssemblePicture(
    const OHOS::HDI::Camera::V1_3::ImageBufferInfoExt& buffer)
{
    int32_t exifDataSize = 0;
    if (buffer.metadata) {
        int32_t retExifDataSize = buffer.metadata->Get("exifDataSize", exifDataSize);
        DP_INFO_LOG("AssemblePicture retExifDataSize: %{public}d, exifDataSize: %{public}d",
            retExifDataSize, exifDataSize);
    }
    auto imageBuffer = TransBufferHandleToSurfaceBuffer(buffer.imageHandle->GetBufferHandle());
    DP_CHECK_ERROR_RETURN_RET_LOG(imageBuffer == nullptr, nullptr, "bufferHandle is nullptr.");
    DP_INFO_LOG("AssemblePicture ImageBufferInfoExt valid: gainMap(%{public}d), depthMap(%{public}d), "
        "unrefocusMap(%{public}d), linearMap(%{public}d), exif(%{public}d), makeInfo(%{public}d)",
        buffer.isGainMapValid, buffer.isDepthMapValid, buffer.isUnrefocusImageValid,
        buffer.isHighBitDepthLinearImageValid, buffer.isExifValid, buffer.isMakerInfoValid);
    std::shared_ptr<PictureIntf> picture = PictureProxy::CreatePictureProxy();
    DP_CHECK_ERROR_RETURN_RET_LOG(picture == nullptr, nullptr, "picture is nullptr.");
    picture->Create(imageBuffer);
    if (buffer.isExifValid) {
        auto exifBuffer = TransBufferHandleToSurfaceBuffer(buffer.exifHandle->GetBufferHandle());
        sptr<BufferExtraData> extraData = new BufferExtraDataImpl();
        extraData->ExtraSet("exifDataSize", exifDataSize);
        if (exifBuffer) {
            exifBuffer->SetExtraData(extraData);
        }
        picture->SetExifMetadata(exifBuffer);
    }
    if (picture) {
        AssemleAuxilaryPicture(buffer, picture);
        picture->RotatePicture();
    }
    return picture;
}

int32_t PhotoPostProcessor::PhotoProcessListener::OnProcessDoneExt(const std::string& imageId,
    const OHOS::HDI::Camera::V1_3::ImageBufferInfoExt& buffer)
{
    DP_INFO_LOG("DPS_PHOTO: imageId: %{public}s", imageId.c_str());
    auto ret = ProcessBufferInfoExt(imageId, buffer);
    if (ret != DP_OK) {
        DP_ERR_LOG("process done failed imageId: %{public}s.", imageId.c_str());
        if (auto processResult = processResult_.lock()) {
            processResult->OnError(imageId, DPS_ERROR_IMAGE_PROC_FAILED);
        }
    }
    return DP_OK;
}

int32_t PhotoPostProcessor::PhotoProcessListener::ProcessBufferInfoExt(const std::string& imageId,
    const OHOS::HDI::Camera::V1_3::ImageBufferInfoExt& buffer)
{
    auto bufferHandle = buffer.imageHandle->GetBufferHandle();
    DP_CHECK_ERROR_RETURN_RET_LOG(bufferHandle == nullptr, DPS_ERROR_IMAGE_PROC_FAILED, "bufferHandle is nullptr.");

    int size = bufferHandle->size;
    int32_t isDegradedImage = 0;
    int32_t dataSize = size;
    int32_t deferredImageFormat = 0;
    uint32_t cloudImageEnhanceFlag = 0;
    if (buffer.metadata) {
        int32_t retImageQuality = buffer.metadata->Get("isDegradedImage", isDegradedImage);
        int32_t retDataSize = buffer.metadata->Get("dataSize", dataSize);
        int32_t retFormat = buffer.metadata->Get("deferredImageFormat", deferredImageFormat);
        int32_t retCloudImageEnhanceFlag = buffer.metadata->Get("cloudImageEnhanceFlag",
            cloudImageEnhanceFlag);
        DP_DEBUG_LOG("retImageQuality: %{public}d, retDataSize: %{public}d, retFormat: %{public}d, "
            "retCloudImageEnhanceFlag: %{public}d", retImageQuality, retDataSize, retFormat,
            retCloudImageEnhanceFlag);
    }

    DP_INFO_LOG("bufferHandle param, bufferHandleSize: %{public}d, dataSize: %{public}d, isDegradedImage: %{public}d, "
        "deferredImageFormat: %{public}d, cloudImageEnhanceFlag: %{public}u",
        size, dataSize, isDegradedImage, deferredImageFormat, cloudImageEnhanceFlag);
    auto processResult = processResult_.lock();
    if (deferredImageFormat == static_cast<int32_t>(Media::PhotoFormat::YUV)) {
        std::shared_ptr<PictureIntf> picture = AssemblePicture(buffer);
        DP_CHECK_ERROR_RETURN_RET_LOG(picture == nullptr, DPS_ERROR_IMAGE_PROC_FAILED,
            "failed to AssemblePicture.");

        std::shared_ptr<BufferInfoExt> bufferInfo = std::make_shared<BufferInfoExt>(picture, dataSize,
            isDegradedImage == 0, cloudImageEnhanceFlag);
        if (processResult) {
            processResult->OnProcessDoneExt(imageId, bufferInfo);
        }
    } else {
        auto bufferPtr = std::make_shared<SharedBuffer>(dataSize);
        DP_CHECK_ERROR_RETURN_RET_LOG(bufferPtr->Initialize() != DP_OK, DPS_ERROR_IMAGE_PROC_FAILED,
            "failed to initialize shared buffer.");

        auto addr = mmap(nullptr, dataSize, PROT_READ | PROT_WRITE, MAP_SHARED, bufferHandle->fd, 0);
        DP_CHECK_ERROR_RETURN_RET_LOG(addr == MAP_FAILED,
            DPS_ERROR_IMAGE_PROC_FAILED, "failed to mmap shared buffer.");
        if (bufferPtr->CopyFrom(static_cast<uint8_t*>(addr), dataSize) == DP_OK) {
            DP_INFO_LOG("DPS_PHOTO: bufferPtr fd: %{public}d, fd: %{public}d", bufferHandle->fd, bufferPtr->GetFd());
            std::shared_ptr<BufferInfo> bufferInfo = std::make_shared<BufferInfo>(bufferPtr, dataSize,
                isDegradedImage == 0, cloudImageEnhanceFlag);
            if (processResult) {
                processResult->OnProcessDone(imageId, bufferInfo);
            }
        }
        munmap(addr, dataSize);
    }
    ReportEvent(imageId);
    return DP_OK;
}

int32_t PhotoPostProcessor::PhotoProcessListener::OnError(const std::string& imageId,
    OHOS::HDI::Camera::V1_2::ErrorCode errorCode)
{
    DP_INFO_LOG("DPS_PHOTO: imageId: %{public}s, error: %{public}d", imageId.c_str(), errorCode);
    if (auto processResult = processResult_.lock()) {
        DpsError dpsErrorCode = MapHdiError(errorCode);
        processResult->OnError(imageId, dpsErrorCode);
    }
    return DP_OK;
}

int32_t PhotoPostProcessor::PhotoProcessListener::OnStatusChanged(OHOS::HDI::Camera::V1_2::SessionStatus status)
{
    DP_INFO_LOG("DPS_PHOTO: HdiStatus: %{public}d", status);
    if (auto processResult = processResult_.lock()) {
        HdiStatus hdiStatus = MapHdiStatus(status);
        processResult->OnStateChanged(hdiStatus);
    }
    return DP_OK;
}

void PhotoPostProcessor::PhotoProcessListener::ReportEvent(const std::string& imageId)
{
    DPSEventReport::GetInstance().UpdateProcessDoneTime(imageId, userId_);
}

PhotoPostProcessor::PhotoPostProcessor(const int32_t userId)
    : userId_(userId), serviceListener_(nullptr), processListener_(nullptr), sessionDeathRecipient_(nullptr)
{
    DP_DEBUG_LOG("entered");
}

PhotoPostProcessor::~PhotoPostProcessor()
{
    DP_DEBUG_LOG("entered");
    DisconnectService();
    SetPhotoSession(nullptr);
    runningWork_.clear();
    imageId2CrashCount_.clear();
    removeNeededList_.clear();
}

void PhotoPostProcessor::Initialize()
{
    DP_DEBUG_LOG("entered");
    processResult_ = std::make_shared<PhotoProcessResult>(userId_);
    sessionDeathRecipient_ = sptr<SessionDeathRecipient>::MakeSptr(processResult_);
    processListener_ = sptr<PhotoProcessListener>::MakeSptr(userId_, processResult_);
    ConnectService();
}

int32_t PhotoPostProcessor::GetConcurrency(ExecutionMode mode)
{
    int32_t count = 1;
    auto session = GetPhotoSession();
    DP_CHECK_ERROR_RETURN_RET_LOG(session == nullptr, count, "photo session is nullptr, count: %{public}d", count);

    int32_t ret = session->GetCoucurrency(OHOS::HDI::Camera::V1_2::ExecutionMode::BALANCED, count);
    DP_INFO_LOG("DPS_PHOTO: GetCoucurrency to ive, ret: %{public}d", ret);
    return count;
}

bool PhotoPostProcessor::GetPendingImages(std::vector<std::string>& pendingImages)
{
    auto session = GetPhotoSession();
    DP_CHECK_ERROR_RETURN_RET_LOG(session == nullptr, false, "photo session is nullptr.");

    int32_t ret = session->GetPendingImages(pendingImages);
    DP_INFO_LOG("DPS_PHOTO: GetPendingImages to ive, ret: %{public}d", ret);
    return ret == DP_OK;
}

void PhotoPostProcessor::SetExecutionMode(ExecutionMode executionMode)
{
    auto session = GetPhotoSession();
    DP_CHECK_ERROR_RETURN_LOG(session == nullptr, "photo session is nullptr.");

    int32_t ret = session->SetExecutionMode(MapToHdiExecutionMode(executionMode));
    DP_INFO_LOG("DPS_PHOTO: SetExecutionMode to ive, ret: %{public}d", ret);
}

void PhotoPostProcessor::SetDefaultExecutionMode()
{
    // 采用直接新增方法，不适配1_2 和 1_3 模式的差异点
    auto session = GetPhotoSession();
    DP_CHECK_ERROR_RETURN_LOG(session == nullptr, "photo session is nullptr.");

    int32_t ret = session->SetExecutionMode(
        static_cast<OHOS::HDI::Camera::V1_2::ExecutionMode>(OHOS::HDI::Camera::V1_3::ExecutionMode::DEFAULT));
    DP_INFO_LOG("DPS_PHOTO: SetDefaultExecutionMode to ive, ret: %{public}d", ret);
}

void PhotoPostProcessor::ProcessImage(const std::string& imageId)
{
    auto session = GetPhotoSession();
    if (session == nullptr) {
        DP_ERR_LOG("Failed to process imageId: %{public}s, photo session is nullptr", imageId.c_str());
        DP_CHECK_EXECUTE(processResult_, processResult_->OnError(imageId, DPS_ERROR_SESSION_NOT_READY_TEMPORARILY));
        return;
    }

    StartTimer(imageId);
    int32_t ret = session->ProcessImage(imageId);
    DP_INFO_LOG("DPS_PHOTO: Process photo to ive, imageId: %{public}s, ret: %{public}d", imageId.c_str(), ret);
}

void PhotoPostProcessor::RemoveImage(const std::string& imageId)
{
    auto session = GetPhotoSession();
    if (session == nullptr) {
        DP_ERR_LOG("photo session is nullptr.");
        std::lock_guard<std::mutex> lock(removeMutex_);
        removeNeededList_.emplace_back(imageId);
        return;
    }

    int32_t ret = session->RemoveImage(imageId);
    DP_INFO_LOG("DPS_PHOTO: Remove photo to ive, imageId: %{public}s, ret: %{public}d", imageId.c_str(), ret);
    imageId2CrashCount_.erase(imageId);
    DPSEventReport::GetInstance().UpdateRemoveTime(imageId, userId_);
}

void PhotoPostProcessor::Interrupt()
{
    auto session = GetPhotoSession();
    DP_CHECK_ERROR_RETURN_LOG(session == nullptr, "photo session is nullptr.");

    int32_t ret = session->Interrupt();
    DP_INFO_LOG("DPS_PHOTO: Interrupt photo to ive, ret: %{public}d", ret);
}

void PhotoPostProcessor::Reset()
{
    auto session = GetPhotoSession();
    DP_CHECK_ERROR_RETURN_LOG(session == nullptr, "photo session is nullptr.");

    int32_t ret = session->Reset();
    DP_INFO_LOG("DPS_PHOTO: Reset to ive, ret: %{public}d", ret);
    consecutiveTimeoutCount_ = 0;
}

void PhotoPostProcessor::OnProcessDone(const std::string& imageId, const std::shared_ptr<BufferInfo>& bufferInfo)
{
    DP_INFO_LOG("DPS_PHOTO: imageId: %{public}s, consecutiveTimeoutCount: %{public}d",
        imageId.c_str(), consecutiveTimeoutCount_.load());
    consecutiveTimeoutCount_ = 0;
    StopTimer(imageId);
    if (auto callback = callback_.lock()) {
        callback->OnProcessDone(userId_, imageId, bufferInfo);
    }
}

void PhotoPostProcessor::OnProcessDoneExt(const std::string& imageId, const std::shared_ptr<BufferInfoExt>& bufferInfo)
{
    DP_INFO_LOG("DPS_PHOTO: imageId: %{public}s, consecutiveTimeoutCount: %{public}d",
        imageId.c_str(), consecutiveTimeoutCount_.load());
    consecutiveTimeoutCount_ = 0;
    StopTimer(imageId);
    if (auto callback = callback_.lock()) {
        callback->OnProcessDoneExt(userId_, imageId, bufferInfo);
    }
}

void PhotoPostProcessor::OnError(const std::string& imageId, DpsError errorCode)
{
    DP_INFO_LOG("DPS_PHOTO: imageId: %{public}s, consecutiveTimeoutCount: %{public}d",
        imageId.c_str(), consecutiveTimeoutCount_.load());
    StopTimer(imageId);
    if (errorCode == DpsError::DPS_ERROR_IMAGE_PROC_TIMEOUT) {
        consecutiveTimeoutCount_++;
        if (consecutiveTimeoutCount_ >= static_cast<int>(MAX_CONSECUTIVE_TIMEOUT_COUNT)) {
            Reset();
        }
    } else {
        consecutiveTimeoutCount_ = 0;
    }

    if (auto callback = callback_.lock()) {
        callback->OnError(userId_, imageId, errorCode);
    }
}

void PhotoPostProcessor::OnStateChanged(HdiStatus hdiStatus)
{
    DP_INFO_LOG("DPS_PHOTO: HdiStatus: %{public}d", hdiStatus);
    EventsMonitor::GetInstance().NotifyImageEnhanceStatus(hdiStatus);
}

void PhotoPostProcessor::OnSessionDied()
{
    DP_INFO_LOG("entered, photo session died!");
    SetPhotoSession(nullptr);
    consecutiveTimeoutCount_ = 0;
    OnStateChanged(HdiStatus::HDI_DISCONNECTED);
    for (const auto& item : runningWork_) {
        std::string imageId = item.first;
        DP_INFO_LOG("Failed to process imageId: %{public}s due to connect service failed", item.first.c_str());
        if (imageId2CrashCount_.count(item.first) == 0) {
            imageId2CrashCount_.emplace(item.first, 1);
        } else {
            imageId2CrashCount_[item.first] += 1;
        }
        DpsError code = DPS_ERROR_SESSION_NOT_READY_TEMPORARILY;
        if (imageId2CrashCount_[item.first] >= MAX_CONSECUTIVE_CRASH_COUNT) {
            code = DPS_ERROR_IMAGE_PROC_FAILED;
        }
        DP_CHECK_EXECUTE(processResult_, processResult_->OnError(imageId, code));
    }
}

void PhotoPostProcessor::SetCallback(const std::weak_ptr<IImageProcessCallbacks>& callback)
{
    callback_ = callback;
}

void PhotoPostProcessor::OnTimerOut(const std::string& imageId)
{
    DP_INFO_LOG("DPS_TIMER: Executed imageId: %{public}s", imageId.c_str());
    DP_CHECK_EXECUTE(processResult_, processResult_->OnError(imageId, DPS_ERROR_IMAGE_PROC_TIMEOUT));
}

void PhotoPostProcessor::ConnectService()
{
    auto svcMgr = HDI::ServiceManager::V1_0::IServiceManager::Get();
    DP_CHECK_ERROR_RETURN_LOG(svcMgr == nullptr, "IServiceManager init failed.");
    serviceListener_ = sptr<PhotoServiceListener>::MakeSptr(weak_from_this());
    auto ret  = svcMgr->RegisterServiceStatusListener(serviceListener_, DEVICE_CLASS_DEFAULT);
    DP_CHECK_ERROR_RETURN_LOG(ret != 0, "Register Photo ServiceStatusListener failed.");
}

void PhotoPostProcessor::DisconnectService()
{
    auto session = GetPhotoSession();
    DP_CHECK_ERROR_RETURN_LOG(session == nullptr, "PhotoSession is nullptr.");

    const sptr<IRemoteObject> &remote = OHOS::HDI::hdi_objcast<IImageProcessSession>(session);
    bool result = remote->RemoveDeathRecipient(sessionDeathRecipient_);
    DP_CHECK_ERROR_RETURN_LOG(!result, "Remove DeathRecipient for PhotoProcessSession failed.");
    auto svcMgr = HDI::ServiceManager::V1_0::IServiceManager::Get();
    DP_CHECK_ERROR_RETURN_LOG(svcMgr == nullptr, "IServiceManager init failed.");

    auto ret  = svcMgr->UnregisterServiceStatusListener(serviceListener_);
    DP_CHECK_ERROR_RETURN_LOG(ret != 0, "Unregister Photo ServiceStatusListener failed.");
}

void PhotoPostProcessor::OnServiceChange(const HDI::ServiceManager::V1_0::ServiceStatus& status)
{
    DP_CHECK_RETURN(status.serviceName != PHOTO_SERVICE_NAME);
    DP_CHECK_RETURN_LOG(status.status != HDI::ServiceManager::V1_0::SERVIE_STATUS_START,
        "photo service state: %{public}d", status.status);
    DP_CHECK_RETURN(GetPhotoSession() != nullptr);

    sptr<OHOS::HDI::Camera::V1_2::IImageProcessService> proxyV1_2 =
        OHOS::HDI::Camera::V1_2::IImageProcessService::Get(status.serviceName);
    DP_CHECK_ERROR_RETURN_LOG(proxyV1_2 == nullptr, "get ImageProcessService failed.");

    uint32_t majorVer = 0;
    uint32_t minorVer = 0;
    proxyV1_2->GetVersion(majorVer, minorVer);
    int32_t versionId = GetVersionId(majorVer, minorVer);
    sptr<IImageProcessSession> session;
    sptr<OHOS::HDI::Camera::V1_3::IImageProcessService> proxyV1_3;
    if (versionId >= GetVersionId(HDI_VERSION_1, HDI_VERSION_3)) {
        proxyV1_3 = OHOS::HDI::Camera::V1_3::IImageProcessService::CastFrom(proxyV1_2);
    }
    if (proxyV1_3 != nullptr) {
        DP_INFO_LOG("CreateImageProcessSessionExt version=%{public}d_%{public}d", majorVer, minorVer);
        proxyV1_3->CreateImageProcessSessionExt(userId_, processListener_, session);
    } else {
        DP_INFO_LOG("CreateImageProcessSession version=%{public}d_%{public}d", majorVer, minorVer);
        proxyV1_2->CreateImageProcessSession(userId_, processListener_, session);
    }
    DP_CHECK_ERROR_RETURN_LOG(session == nullptr, "get ImageProcessSession failed.");

    const sptr<IRemoteObject>& remote = OHOS::HDI::hdi_objcast<IImageProcessSession>(session);
    bool result = remote->AddDeathRecipient(sessionDeathRecipient_);
    DP_CHECK_ERROR_RETURN_LOG(!result, "add DeathRecipient for ImageProcessSession failed.");

    RemoveNeedJbo(session);
    SetPhotoSession(session);
    OnStateChanged(HdiStatus::HDI_READY);
}

void PhotoPostProcessor::RemoveNeedJbo(const sptr<IImageProcessSession>& session)
{
    std::lock_guard<std::mutex> lock(removeMutex_);
    for (const auto& imageId : removeNeededList_) {
        int32_t ret = session->RemoveImage(imageId);
        DP_INFO_LOG("DPS_PHOTO: RemoveImage imageId: %{public}s, ret: %{public}d", imageId.c_str(), ret);
    }
    removeNeededList_.clear();
}

void PhotoPostProcessor::StartTimer(const std::string& imageId)
{
    uint32_t timeId = DpsTimer::GetInstance().StartTimer([&, imageId]() {OnTimerOut(imageId);}, MAX_PROC_TIME_MS);
    DP_INFO_LOG("DPS_TIMER: Start imageId: %{public}s, timeId: %{public}u", imageId.c_str(), timeId);
    runningWork_.emplace(imageId, timeId);
}

void PhotoPostProcessor::StopTimer(const std::string& imageId)
{
    auto it = runningWork_.find(imageId);
    DP_CHECK_ERROR_RETURN_LOG(it == runningWork_.end(),
        "Stoptimer failed not find imageId: %{public}s", imageId.c_str());

    DpsTimer::GetInstance().StopTimer(it->second);
    DP_INFO_LOG("DPS_TIMER: Stop imageId: %{public}s, timeId: %{public}u", imageId.c_str(), it->second);
    runningWork_.erase(it);
}
} // namespace DeferredProcessing
} // namespace CameraStandard
} // namespace OHOS