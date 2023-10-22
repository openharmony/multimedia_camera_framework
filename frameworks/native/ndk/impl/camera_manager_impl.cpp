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

#include "camera_manager_impl.h"
#include "camera_log.h"
#include "camera_util.h"
#include "surface_utils.h"
#include "image_receiver.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::CameraStandard;

class InnerCameraManagerCallback : public CameraManagerCallback {
public:
    InnerCameraManagerCallback(Camera_Manager* cameraManager, CameraManager_Callbacks* callback)
        : cameraManager_(cameraManager), callback_(*callback) {}
    ~InnerCameraManagerCallback() = default;

    void OnCameraStatusChanged(const CameraStatusInfo &cameraStatusInfo) const override
    {
        MEDIA_DEBUG_LOG("OnFrameStarted is called!");
        Camera_StatusInfo statusInfo;
        statusInfo.camera->cameraId = cameraStatusInfo.cameraDevice->GetID().data();
        statusInfo.camera->cameraPosition = static_cast<Camera_Position>(cameraStatusInfo.cameraDevice->GetPosition());
        statusInfo.camera->cameraType = static_cast<Camera_Type>(cameraStatusInfo.cameraDevice->GetCameraType());
        statusInfo.camera->connectionType =
            static_cast<Camera_Connection>(cameraStatusInfo.cameraDevice->GetConnectionType());
        statusInfo.status = static_cast<Camera_Status>(cameraStatusInfo.cameraStatus);
        if (cameraManager_ != nullptr && callback_.onCameraStatus != nullptr) {
            callback_.onCameraStatus(cameraManager_, &statusInfo);
        }
    }

    void OnFlashlightStatusChanged(const std::string &cameraID, const FlashStatus flashStatus) const override
    {
        MEDIA_DEBUG_LOG("OnFrameStarted is called!");
        (void)cameraID;
        (void)flashStatus;
    }

private:
    Camera_Manager* cameraManager_;
    CameraManager_Callbacks callback_;
};

Camera_Manager::Camera_Manager()
{
    MEDIA_DEBUG_LOG("Camera_Manager Constructor is called");
    cameraManager_ = CameraManager::GetInstance();
}

Camera_Manager::~Camera_Manager()
{
    MEDIA_DEBUG_LOG("~Camera_Manager is called");
    if (cameraManager_) {
        cameraManager_ = nullptr;
    }
}

Camera_ErrorCode Camera_Manager::RegisterCallback(CameraManager_Callbacks* callback)
{
    shared_ptr<InnerCameraManagerCallback> innerCallback =
                make_shared<InnerCameraManagerCallback>(this, callback);
    cameraManager_->SetCallback(innerCallback);
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::UnregisterCallback(CameraManager_Callbacks* callback)
{
    cameraManager_->SetCallback(nullptr);
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::GetSupportedCameras(Camera_Device** cameras, uint32_t* size)
{
    std::vector<sptr<CameraDevice>> cameraObjList = CameraManager::GetInstance()->GetSupportedCameras();
    int cameraSize = cameraObjList.size();
    if (cameraSize <= 0) {
        MEDIA_ERR_LOG("camera size <= 0");
        return CAMERA_INVALID_ARGUMENT;
    }
    Camera_Device *outCameras = new Camera_Device[cameraSize];
    for (int i = 0; i < cameraSize; i++) {
        const char* src = cameraObjList[i]->GetID().c_str();
        size_t dstSize = strlen(src) + 1;
        char* dst = new char[dstSize];
        if (!dst) {
            MEDIA_ERR_LOG("Allocate memory for cameraId failed!");
            return CAMERA_SERVICE_FATAL_ERROR;
        }
        strlcpy(dst, src, dstSize);
        outCameras[i].cameraId = dst;
        outCameras[i].cameraPosition = static_cast<Camera_Position>(cameraObjList[i]->GetPosition());
        outCameras[i].cameraType = static_cast<Camera_Type>(cameraObjList[i]->GetCameraType());
        outCameras[i].connectionType = static_cast<Camera_Connection>(cameraObjList[i]->GetConnectionType());
    }
    *cameras = outCameras;
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::GetSupportedCameraOutputCapability(const Camera_Device* camera,
    Camera_OutputCapability** cameraOutputCapability)
{
    Camera_OutputCapability* outCapability = new Camera_OutputCapability;

    sptr<CameraDevice> cameraDevice = nullptr;
    std::vector<sptr<CameraDevice>> cameraObjList = CameraManager::GetInstance()->GetSupportedCameras();
        MEDIA_ERR_LOG("GetSupportedCameraOutputCapability cameraInfo is null, the cameraObjList size is %{public}zu",
            cameraObjList.size());
        for (size_t i = 0; i < cameraObjList.size(); i++) {
            MEDIA_ERR_LOG("GetSupportedCameraOutputCapability for");
            sptr<CameraDevice> innerCameraDevice = cameraObjList[i];
            if (innerCameraDevice == nullptr) {
                MEDIA_ERR_LOG("GetSupportedCameraOutputCapability innerCameraDevice == null");
                continue;
            }
            if (innerCameraDevice->GetPosition() == static_cast<CameraPosition>(camera->cameraPosition) &&
                innerCameraDevice->GetCameraType() == static_cast<CameraType>(camera->cameraType)) {
                MEDIA_ERR_LOG("GetSupportedCameraOutputCapability position:%{public}d, type:%{public}d ",
                    innerCameraDevice->GetPosition(), innerCameraDevice->GetCameraType());
                cameraDevice = innerCameraDevice;

                break;
            }
        }
    MEDIA_ERR_LOG("GetSupportedCameraOutputCapability  test");
    sptr<CameraOutputCapability> innerCameraOutputCapability =
        CameraManager::GetInstance()->GetSupportedOutputCapability(cameraDevice);
    if (innerCameraOutputCapability == nullptr) {
        MEDIA_ERR_LOG("GetSupportedCameraOutputCapability innerCameraOutputCapability == null");
    }
    MEDIA_ERR_LOG("GetSupportedCameraOutputCapability innerCameraOutputCapability !- null");
    std::vector<Profile> previewProfiles = innerCameraOutputCapability->GetPreviewProfiles();
    std::vector<Profile> photoProfiles = innerCameraOutputCapability->GetPhotoProfiles();
    std::vector<VideoProfile> videoProfiles = innerCameraOutputCapability->GetVideoProfiles();
    int previewOutputNum = previewProfiles.size();
    int photoOutputNum = previewProfiles.size();
    int videoOutputNum = videoProfiles.size();
    if (previewOutputNum <= 0 || photoOutputNum <= 0 || videoOutputNum <= 0) {
        MEDIA_ERR_LOG("alloc size <= 0");
        return CAMERA_INVALID_ARGUMENT;
    }
    Camera_Profile *outPreviewProfiles = new Camera_Profile[previewOutputNum];
    Camera_Profile *outPhotoProfiles = new Camera_Profile[photoOutputNum];
    Camera_VideoProfile *outVideoProfiles = new Camera_VideoProfile[videoOutputNum];
    for (int i = 0; i < previewOutputNum; i++) {
        outPreviewProfiles[i].format = static_cast<Camera_Format>(previewProfiles[i].GetCameraFormat());
        outPreviewProfiles[i].size.width = previewProfiles[i].GetSize().width;
        outPreviewProfiles[i].size.height = previewProfiles[i].GetSize().height;
        MEDIA_ERR_LOG("GetSupportedCameraOutputCapability previewOutputNum enter");
    }
    MEDIA_ERR_LOG("GetSupportedCameraOutputCapability previewOutputNum exit");

    outCapability->previewProfiles = &outPreviewProfiles;

    for (int i = 0; i < photoOutputNum; i++) {
        outPhotoProfiles[i].format = static_cast<Camera_Format>(photoProfiles[i].GetCameraFormat());
        outPhotoProfiles[i].size.width = photoProfiles[i].GetSize().width;
        outPhotoProfiles[i].size.height = photoProfiles[i].GetSize().height;
    }
    outCapability->photoProfiles = &outPhotoProfiles;

    for (int i = 0; i < videoOutputNum; i++) {
        outVideoProfiles[i].format = static_cast<Camera_Format>(videoProfiles[i].GetCameraFormat());
        outVideoProfiles[i].size.width = videoProfiles[i].GetSize().width;
        outVideoProfiles[i].size.height = videoProfiles[i].GetSize().height;
        outVideoProfiles[i].range.min  = videoProfiles[i].framerates_[0];
        outVideoProfiles[i].range.max  = videoProfiles[i].framerates_[1];
    }
    outCapability->videoProfiles = &outVideoProfiles;

    *cameraOutputCapability = outCapability;
    MEDIA_ERR_LOG("GetSupportedCameraOutputCapability previewOutputNum return");
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::IsCameraMuted(bool* isCameraMuted)
{
    MEDIA_ERR_LOG("Camera_Manager IsCameraMuted is called");
    *isCameraMuted = CameraManager::GetInstance()->IsCameraMuted();
    MEDIA_ERR_LOG("IsCameraMuted is %{public}d", *isCameraMuted);
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::CreateCaptureSession(Camera_CaptureSession** captureSession)
{
    sptr<CaptureSession> innerCaptureSession = nullptr;
    int retCode = CameraManager::GetInstance()->CreateCaptureSession(&innerCaptureSession);
    if (retCode != CameraErrorCode::SUCCESS) {
        return CAMERA_SERVICE_FATAL_ERROR;
    }
    Camera_CaptureSession* outSession = new Camera_CaptureSession(innerCaptureSession);
    *captureSession = outSession;
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::CreateCameraInput(const Camera_Device* camera, Camera_Input** cameraInput)
{
    sptr<CameraDevice> cameraDevice = nullptr;
    std::vector<sptr<CameraDevice>> cameraObjList = CameraManager::GetInstance()->GetSupportedCameras();
        MEDIA_DEBUG_LOG("cameraInfo is null, the cameraObjList size is %{public}zu",
            cameraObjList.size());
        for (size_t i = 0; i < cameraObjList.size(); i++) {
            sptr<CameraDevice> innerCameraDevice = cameraObjList[i];
            if (innerCameraDevice == nullptr) {
                continue;
            }
            if (innerCameraDevice->GetPosition() == static_cast<CameraPosition>(camera->cameraPosition) &&
                innerCameraDevice->GetCameraType() == static_cast<CameraType>(camera->cameraType)) {
                cameraDevice = innerCameraDevice;
                break;
            }
        }

    sptr<CameraInput> innerCameraInput = nullptr;
    int retCode = CameraManager::GetInstance()->CreateCameraInput(cameraDevice, &innerCameraInput);
    if (retCode != CameraErrorCode::SUCCESS) {
        return CAMERA_SERVICE_FATAL_ERROR;
    }
    Camera_Input* outInput = new Camera_Input(innerCameraInput);
    *cameraInput = outInput;
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::CreateCameraInputWithPositionAndType(Camera_Position position, Camera_Type type,
    Camera_Input** cameraInput)
{
    MEDIA_ERR_LOG("Camera_Manager CreateCameraInputWithPositionAndType is called");
    sptr<CameraInput> innerCameraInput = nullptr;
    CameraPosition innerPosition = static_cast<CameraPosition>(position);
    CameraType innerType = static_cast<CameraType>(type);

    innerCameraInput = CameraManager::GetInstance()->CreateCameraInput(innerPosition, innerType);
    if (innerCameraInput == nullptr) {
        MEDIA_ERR_LOG("failed to CreateCameraInputWithPositionAndType");
    }

    Camera_Input* outInput = new Camera_Input(innerCameraInput);
    *cameraInput = outInput;
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::CreatePreviewOutput(const Camera_Profile* profile,
    const char* surfaceId, Camera_PreviewOutput** previewOutput)
{
    sptr<PreviewOutput> innerPreviewOutput = nullptr;
    Size size;
    size.width = profile->size.width;
    size.height = profile->size.height;
    Profile innerProfile(static_cast<CameraFormat>(profile->format), size);

    uint64_t iSurfaceId;
    std::istringstream iss(surfaceId);
    iss >> iSurfaceId;
    sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(iSurfaceId);
    if (!surface) {
        surface = Media::ImageReceiver::getSurfaceById(surfaceId);
    }
    if (surface == nullptr) {
        MEDIA_ERR_LOG("failed to get previewOutput surface");
    }

    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(innerProfile.GetCameraFormat()));
    int retCode = CameraManager::GetInstance()->CreatePreviewOutput(innerProfile, surface, &innerPreviewOutput);
    if (retCode != CameraErrorCode::SUCCESS) {
        return CAMERA_SERVICE_FATAL_ERROR;
    }
    Camera_PreviewOutput* out = new Camera_PreviewOutput(innerPreviewOutput);
    *previewOutput = out;
    MEDIA_ERR_LOG("Camera_Manager::CreatePreviewOutput");
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::CreatePhotoOutput(const Camera_Profile* profile,
    const char* surfaceId, Camera_PhotoOutput** photoOutput)
{
    MEDIA_ERR_LOG("Camera_Manager CreatePhotoOutput is called");
    sptr<PhotoOutput> innerPhotoOutput = nullptr;
    Size size;
    size.width = profile->size.width;
    size.height = profile->size.height;
    Profile innerProfile(static_cast<CameraFormat>(profile->format), size);

    sptr<Surface> surface = Media::ImageReceiver::getSurfaceById(surfaceId);
    if (surface == nullptr) {
        MEDIA_ERR_LOG("failed to get photoOutput surface");
    }

    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(innerProfile.GetCameraFormat()));
    sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
    int retCode = CameraManager::GetInstance()->CreatePhotoOutput(innerProfile, surfaceProducer, &innerPhotoOutput);
    if (retCode != CameraErrorCode::SUCCESS) {
        return CAMERA_SERVICE_FATAL_ERROR;
    }
    Camera_PhotoOutput* out = new Camera_PhotoOutput(innerPhotoOutput);
    *photoOutput = out;
    return CAMERA_OK;
}

Camera_ErrorCode Camera_Manager::CreateVideoOutput(const Camera_VideoProfile* profile,
    const char* surfaceId, Camera_VideoOutput** videoOutput)
{
    sptr<VideoOutput> innerVideoOutput = nullptr;
    Size size;
    size.width = profile->size.width;
    size.height = profile->size.height;
    std::vector<int32_t> framerates = {profile->range.min, profile->range.max};
    VideoProfile innerProfile(static_cast<CameraFormat>(profile->format), size, framerates);

    uint64_t iSurfaceId;
    std::istringstream iss(surfaceId);
    iss >> iSurfaceId;
    sptr<Surface> surface = SurfaceUtils::GetInstance()->GetSurface(iSurfaceId);
    if (!surface) {
        surface = Media::ImageReceiver::getSurfaceById(surfaceId);
    }
    if (surface == nullptr) {
        MEDIA_ERR_LOG("failed to get videoOutput surface");
    }

    surface->SetUserData(CameraManager::surfaceFormat, std::to_string(innerProfile.GetCameraFormat()));
    int retCode = CameraManager::GetInstance()->CreateVideoOutput(innerProfile, surface, &innerVideoOutput);
    if (retCode != CameraErrorCode::SUCCESS) {
        return CAMERA_SERVICE_FATAL_ERROR;
    }
    Camera_VideoOutput* out = new Camera_VideoOutput(innerVideoOutput);
    *videoOutput = out;
    return CAMERA_OK;
}


Camera_ErrorCode Camera_Manager::CreateMetadataOutput(const Camera_MetadataObjectType* type,
    Camera_MetadataOutput** metadataOutput)
{
    MEDIA_ERR_LOG("Camera_Manager CreateMetadataOutput is called");
    sptr<MetadataOutput> innerMetadataOutput = nullptr;

    int retCode = CameraManager::GetInstance()->CreateMetadataOutput(&innerMetadataOutput);
    if (retCode != CameraErrorCode::SUCCESS) {
        return CAMERA_SERVICE_FATAL_ERROR;
    }
    Camera_MetadataOutput* out = new Camera_MetadataOutput(innerMetadataOutput);
    *metadataOutput = out;
    return CAMERA_OK;
}