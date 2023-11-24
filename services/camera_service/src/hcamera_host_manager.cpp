/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "hcamera_host_manager.h"

#include "v1_2/icamera_host_callback.h"
#include "metadata_utils.h"
#include "camera_util.h"
#include "hdf_device_class.h"
#include "iproxy_broker.h"
#include "iservmgr_hdi.h"
#include "camera_log.h"

namespace OHOS {
namespace CameraStandard {
struct HCameraHostManager::CameraDeviceInfo {
    std::string cameraId;
    std::shared_ptr<OHOS::Camera::CameraMetadata> ability;
    std::mutex mutex;

    explicit CameraDeviceInfo(const std::string& cameraId, sptr<ICameraDevice> device = nullptr)
        : cameraId(cameraId), ability(nullptr)
    {
    }

    ~CameraDeviceInfo() = default;
};

class HCameraHostManager::CameraHostInfo : public OHOS::HDI::Camera::V1_2::ICameraHostCallback {
public:
    class CameraHostDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit CameraHostDeathRecipient(const sptr<HCameraHostManager::CameraHostInfo> &hostInfo)
            : cameraHostInfo_(hostInfo)
        {
        }
        virtual ~CameraHostDeathRecipient() = default;
        void OnRemoteDied(const wptr<IRemoteObject> &remote) override
        {
            MEDIA_ERR_LOG("Remote died, do clean works.");
            if (cameraHostInfo_ == nullptr) {
                return;
            }
            cameraHostInfo_->CameraHostDied();
        }

    private:
        sptr<HCameraHostManager::CameraHostInfo> cameraHostInfo_;
    };

    explicit CameraHostInfo(HCameraHostManager* cameraHostManager, std::string name);
    ~CameraHostInfo();
    bool Init();
    void CameraHostDied();
    bool IsCameraSupported(const std::string& cameraId);
    const std::string& GetName();
    int32_t GetCameraHostVersion();
    int32_t GetCameras(std::vector<std::string>& cameraIds);
    int32_t GetCameraAbility(std::string& cameraId, std::shared_ptr<OHOS::Camera::CameraMetadata>& ability);
    int32_t OpenCamera(std::string& cameraId, const sptr<ICameraDeviceCallback>& callback,
                       sptr<OHOS::HDI::Camera::V1_0::ICameraDevice>& pDevice);
    int32_t SetFlashlight(const std::string& cameraId, bool isEnable);
    int32_t SetTorchLevel(float level);
    int32_t Prelaunch(const std::string& cameraId);
    void NotifyDeviceStateChangeInfo(int notifyType, int deviceState);
    bool IsLocalCameraHostInfo();

    // CameraHostCallbackStub
    int32_t OnCameraStatus(const std::string& cameraId, HDI::Camera::V1_0::CameraStatus status) override;
    int32_t OnFlashlightStatus(const std::string& cameraId, FlashlightStatus status) override;
    int32_t OnFlashlightStatusV1_2(FlashlightStatus status) override;
    int32_t OnCameraEvent(const std::string &cameraId, CameraEvent event) override;

private:
    std::shared_ptr<CameraDeviceInfo> FindCameraDeviceInfo(const std::string& cameraId);
    void AddDevice(const std::string& cameraId);
    void RemoveDevice(const std::string& cameraId);

    HCameraHostManager* cameraHostManager_;
    std::string name_;
    uint32_t majorVer_;
    uint32_t minorVer_;
    sptr<OHOS::HDI::Camera::V1_0::ICameraHost> cameraHostProxy_;
    sptr<OHOS::HDI::Camera::V1_1::ICameraHost> cameraHostProxyV1_1_;
    sptr<OHOS::HDI::Camera::V1_2::ICameraHost> cameraHostProxyV1_2_;

    std::mutex mutex_;
    std::vector<std::string> cameraIds_;
    std::vector<std::shared_ptr<CameraDeviceInfo>> devices_;
};

HCameraHostManager::CameraHostInfo::CameraHostInfo(HCameraHostManager* cameraHostManager, std::string name)
    : cameraHostManager_(cameraHostManager), name_(std::move(name)), cameraHostProxy_(nullptr)
{
}

HCameraHostManager::CameraHostInfo::~CameraHostInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_INFO_LOG("CameraHostInfo ~CameraHostInfo");
    cameraHostManager_ = nullptr;
    cameraHostProxy_ = nullptr;
    cameraHostProxyV1_1_ = nullptr;
    cameraHostProxyV1_2_ = nullptr;
    for (unsigned i = 0; i < devices_.size(); i++) {
        devices_.at(i) = nullptr;
    }
    cameraIds_.clear();
    devices_.clear();
}

bool HCameraHostManager::CameraHostInfo::Init()
{
    if (cameraHostProxy_ != nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::Init, no camera host proxy");
        return true;
    }
    cameraHostProxy_ = OHOS::HDI::Camera::V1_0::ICameraHost::Get(name_.c_str(), false);
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("Failed to get ICameraHost");
        return false;
    }
    // Get cameraHost veresion
    cameraHostProxy_->GetVersion(majorVer_, minorVer_);
    MEDIA_INFO_LOG("CameraHostInfo::Init GetVersion majorVer_: %{public}u, minorVers_: %{public}u",
        majorVer_, minorVer_);
    if (GetCameraHostVersion() > GetVersionId(1, 1)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::Init ICameraHost cast to V1_2");
        auto castResult_V1_2 = OHOS::HDI::Camera::V1_2::ICameraHost::CastFrom(cameraHostProxy_);
        if (castResult_V1_2 != nullptr) {
            cameraHostProxyV1_2_ = castResult_V1_2;
        }
    } else if (GetCameraHostVersion() == GetVersionId(1, 1)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::Init ICameraHost cast to V1_1");
        auto castResult_V1_1 = OHOS::HDI::Camera::V1_1::ICameraHost::CastFrom(cameraHostProxy_);
        if (castResult_V1_1 != nullptr) {
            cameraHostProxyV1_1_ = castResult_V1_1;
        }
    }

    if (cameraHostProxyV1_2_ != nullptr && GetCameraHostVersion() > GetVersionId(1, 1)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::Init SetCallback ICameraHost V1_2");
        cameraHostProxyV1_2_->SetCallbackV1_2(this);
    } else {
        MEDIA_DEBUG_LOG("CameraHostInfo::Init SetCallback ICameraHost V1_0");
        cameraHostProxy_->SetCallback(this);
    }

    sptr<CameraHostDeathRecipient> cameraHostDeathRecipient = new CameraHostDeathRecipient(this);
    const sptr<IRemoteObject> &remote = OHOS::HDI::hdi_objcast<ICameraHost>(cameraHostProxy_);
    bool result = remote->AddDeathRecipient(cameraHostDeathRecipient);
    if (!result) {
        MEDIA_ERR_LOG("AddDeathRecipient for CameraHost failed.");
    }

    std::lock_guard<std::mutex> lock(mutex_);
    CamRetCode ret = (CamRetCode)(cameraHostProxy_->GetCameraIds(cameraIds_));
    if (ret != HDI::Camera::V1_0::NO_ERROR) {
        MEDIA_ERR_LOG("Init, GetCameraIds failed, ret = %{public}d", ret);
        return false;
    }
    for (const auto& cameraId : cameraIds_) {
        devices_.push_back(std::make_shared<HCameraHostManager::CameraDeviceInfo>(cameraId));
    }
    return true;
}

void HCameraHostManager::CameraHostInfo::CameraHostDied()
{
    if (cameraHostManager_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::cameraHostManager is null.");
        return;
    }
    cameraHostManager_->RemoveCameraHost(name_);
}

bool HCameraHostManager::CameraHostInfo::IsCameraSupported(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(cameraIds_.begin(), cameraIds_.end(),
                       [&cameraId](const auto& camId) { return camId == cameraId; });
}

const std::string& HCameraHostManager::CameraHostInfo::GetName()
{
    return name_;
}

int32_t HCameraHostManager::CameraHostInfo::GetCameraHostVersion()
{
    MEDIA_INFO_LOG("cameraHostProxy_ GetVersion majorVer_: %{public}u, minorVers_: %{public}u", majorVer_, minorVer_);
    return GetVersionId(majorVer_, minorVer_);
}

int32_t HCameraHostManager::CameraHostInfo::GetCameras(std::vector<std::string>& cameraIds)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cameraIds.insert(cameraIds.end(), cameraIds_.begin(), cameraIds_.end());
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::GetCameraAbility(std::string& cameraId,
    std::shared_ptr<OHOS::Camera::CameraMetadata>& ability)
{
    auto deviceInfo = FindCameraDeviceInfo(cameraId);
    if (deviceInfo == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility deviceInfo is null");
        return CAMERA_UNKNOWN_ERROR;
    }

    std::lock_guard<std::mutex> lock(deviceInfo->mutex);
    if (deviceInfo->ability) {
        ability = deviceInfo->ability;
    } else {
        if (cameraHostProxy_ == nullptr) {
            MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility cameraHostProxy_ is null");
            return CAMERA_UNKNOWN_ERROR;
        }
        if (!deviceInfo->ability) {
            std::vector<uint8_t> cameraAbility;
            CamRetCode rc = (CamRetCode)(cameraHostProxy_->GetCameraAbility(cameraId, cameraAbility));
            if (rc != HDI::Camera::V1_0::NO_ERROR) {
                MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility failed with error Code:%{public}d", rc);
                return HdiToServiceError(rc);
            }
            OHOS::Camera::MetadataUtils::ConvertVecToMetadata(cameraAbility, ability);
            deviceInfo->ability = ability;
        }
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::OpenCamera(std::string& cameraId,
    const sptr<ICameraDeviceCallback>& callback,
    sptr<OHOS::HDI::Camera::V1_0::ICameraDevice>& pDevice)
{
    MEDIA_INFO_LOG("CameraHostInfo::OpenCamera %{public}s", cameraId.c_str());
    auto deviceInfo = FindCameraDeviceInfo(cameraId);
    if (deviceInfo == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::GetCameraAbility deviceInfo is null");
        return CAMERA_UNKNOWN_ERROR;
    }

    std::lock_guard<std::mutex> lock(deviceInfo->mutex);
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::OpenCamera cameraHostProxy_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }
    CamRetCode rc;
    // try to get higher version
    sptr<OHOS::HDI::Camera::V1_1::ICameraDevice> hdiDevice_v1_1;
    sptr<OHOS::HDI::Camera::V1_2::ICameraDevice> hdiDevice_v1_2;
    if (cameraHostProxyV1_2_ != nullptr && GetCameraHostVersion() >= GetVersionId(HDI_VERSION_1, HDI_VERSION_2)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::OpenCamera ICameraDevice V1_2");
        rc = (CamRetCode)(cameraHostProxyV1_2_->OpenCameraV1_2(cameraId, callback, hdiDevice_v1_2));
        pDevice = static_cast<OHOS::HDI::Camera::V1_0::ICameraDevice *>(hdiDevice_v1_2.GetRefPtr());
    } else if (cameraHostProxyV1_1_ != nullptr
        && GetCameraHostVersion() == GetVersionId(HDI_VERSION_1, HDI_VERSION_1)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::OpenCamera ICameraDevice V1_1");
        rc = (CamRetCode)(cameraHostProxyV1_1_->OpenCamera_V1_1(cameraId, callback, hdiDevice_v1_1));
        pDevice = static_cast<OHOS::HDI::Camera::V1_0::ICameraDevice *>(hdiDevice_v1_1.GetRefPtr());
    } else {
        MEDIA_DEBUG_LOG("CameraHostInfo::OpenCamera ICameraDevice V1_0");
        rc = (CamRetCode)(cameraHostProxy_->OpenCamera(cameraId, callback, pDevice));
    }
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        MEDIA_ERR_LOG("CameraHostInfo::OpenCamera failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::SetFlashlight(const std::string& cameraId, bool isEnable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::SetFlashlight cameraHostProxy_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }
    CamRetCode rc = (CamRetCode)(cameraHostProxy_->SetFlashlight(cameraId, isEnable));
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        MEDIA_ERR_LOG("CameraHostInfo::SetFlashlight failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::SetTorchLevel(float level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (cameraHostProxyV1_2_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::SetTorchLevel cameraHostProxyV1_2_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }
    HDI::Camera::V1_2::CamRetCode rc = (HDI::Camera::V1_2::CamRetCode)(cameraHostProxyV1_2_->SetFlashlightV1_2(level));
    if (rc != HDI::Camera::V1_2::NO_ERROR) {
        MEDIA_ERR_LOG("CameraHostInfo::SetTorchLevel failed with error Code:%{public}d", rc);
        return HdiToServiceErrorV1_2(rc);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::Prelaunch(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::Prelaunch cameraHostProxy_ is null");
        return CAMERA_UNKNOWN_ERROR;
    }
    if (GetCameraHostVersion() < GetVersionId(1, 1)) {
        MEDIA_ERR_LOG("CameraHostInfo::Prelaunch not support host V1_0!");
        return CAMERA_UNKNOWN_ERROR;
    }
    MEDIA_INFO_LOG("CameraHostInfo::prelaunch for cameraId %{public}s", cameraId.c_str());
    OHOS::HDI::Camera::V1_1::PrelaunchConfig prelaunchConfig;
    std::vector<uint8_t> settings;
    prelaunchConfig.cameraId = cameraId;
    prelaunchConfig.streamInfos_V1_1 = {};
    prelaunchConfig.setting = settings;
    CamRetCode rc;
    if (cameraHostProxyV1_2_ != nullptr && GetCameraHostVersion() > GetVersionId(1, 1)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::Prelaunch ICameraHost V1_2");
        rc = (CamRetCode)(cameraHostProxyV1_2_->Prelaunch(prelaunchConfig));
    } else if (cameraHostProxyV1_1_ != nullptr && GetCameraHostVersion() == GetVersionId(1, 1)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::Prelaunch ICameraHost V1_1");
        rc = (CamRetCode)(cameraHostProxyV1_1_->Prelaunch(prelaunchConfig));
    } else {
        rc = HDI::Camera::V1_0::NO_ERROR;
    }
    if (rc != HDI::Camera::V1_0::NO_ERROR) {
        MEDIA_ERR_LOG("CameraHostInfo::Prelaunch failed with error Code:%{public}d", rc);
        return HdiToServiceError(rc);
    }
    return CAMERA_OK;
}

void HCameraHostManager::CameraHostInfo::NotifyDeviceStateChangeInfo(int notifyType, int deviceState)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (cameraHostProxy_ == nullptr) {
        MEDIA_ERR_LOG("CameraHostInfo::Prelaunch cameraHostProxy_ is null");
        return;
    }
    MEDIA_DEBUG_LOG("CameraHostInfo::NotifyDeviceStateChangeInfo notifyType = %{public}d, deviceState = %{public}d",
        notifyType, deviceState);
    if (cameraHostProxyV1_2_ != nullptr && GetCameraHostVersion() > GetVersionId(1, 1)) {
        MEDIA_DEBUG_LOG("CameraHostInfo::NotifyDeviceStateChangeInfo ICameraHost V1_2");
        cameraHostProxyV1_2_->NotifyDeviceStateChangeInfo(notifyType, deviceState);
    }
}

int32_t HCameraHostManager::CameraHostInfo::OnCameraStatus(const std::string& cameraId,
                                                           HDI::Camera::V1_0::CameraStatus status)
{
    if ((cameraHostManager_ == nullptr) || (cameraHostManager_->statusCallback_ == nullptr)) {
        MEDIA_WARNING_LOG("CameraHostInfo::OnCameraStatus for %{public}s with status %{public}d "
                          "failed due to no callback",
                          cameraId.c_str(), status);
        return CAMERA_UNKNOWN_ERROR;
    }
    CameraStatus svcStatus = CAMERA_STATUS_UNAVAILABLE;
    switch (status) {
        case UN_AVAILABLE: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraStatus, camera %{public}s unavailable", cameraId.c_str());
            svcStatus = CAMERA_STATUS_UNAVAILABLE;
            break;
        }
        case AVAILABLE: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraStatus, camera %{public}s available", cameraId.c_str());
            svcStatus = CAMERA_STATUS_AVAILABLE;
            AddDevice(cameraId);
            break;
        }
        default:
            MEDIA_ERR_LOG("Unknown camera status: %{public}d", status);
            return CAMERA_UNKNOWN_ERROR;
    }
    cameraHostManager_->statusCallback_->OnCameraStatus(cameraId, svcStatus);
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::OnFlashlightStatus(const std::string& cameraId,
    FlashlightStatus status)
{
    if ((cameraHostManager_ == nullptr) || (cameraHostManager_->statusCallback_ == nullptr)) {
        MEDIA_WARNING_LOG("CameraHostInfo::OnFlashlightStatus for %{public}s with status %{public}d "
                          "failed due to no callback or cameraHostManager_ is null",
                          cameraId.c_str(), status);
        return CAMERA_UNKNOWN_ERROR;
    }
    FlashStatus flashStatus = FLASH_STATUS_OFF;
    switch (status) {
        case FLASHLIGHT_OFF:
            flashStatus = FLASH_STATUS_OFF;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatus, camera %{public}s flash light is off",
                           cameraId.c_str());
            break;

        case FLASHLIGHT_ON:
            flashStatus = FLASH_STATUS_ON;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatus, camera %{public}s flash light is on",
                           cameraId.c_str());
            break;

        case FLASHLIGHT_UNAVAILABLE:
            flashStatus = FLASH_STATUS_UNAVAILABLE;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatus, camera %{public}s flash light is unavailable",
                           cameraId.c_str());
            break;

        default:
            MEDIA_ERR_LOG("Unknown flashlight status: %{public}d for camera %{public}s", status, cameraId.c_str());
            return CAMERA_UNKNOWN_ERROR;
    }
    cameraHostManager_->statusCallback_->OnFlashlightStatus(cameraId, flashStatus);
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::OnFlashlightStatusV1_2(FlashlightStatus status)
{
    if ((cameraHostManager_ == nullptr) || (cameraHostManager_->statusCallback_ == nullptr)) {
        MEDIA_WARNING_LOG("CameraHostInfo::OnFlashlightStatusV1_2 with status %{public}d "
                          "failed due to no callback or cameraHostManager_ is null", status);
        return CAMERA_UNKNOWN_ERROR;
    }
    TorchStatus torchStatus = TORCH_STATUS_OFF;
    switch (status) {
        case FLASHLIGHT_OFF:
            torchStatus = TORCH_STATUS_OFF;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatusV1_2, torch status is off");
            break;

        case FLASHLIGHT_ON:
            torchStatus = TORCH_STATUS_ON;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatusV1_2, torch status is on");
            break;

        case FLASHLIGHT_UNAVAILABLE:
            torchStatus = TORCH_STATUS_UNAVAILABLE;
            MEDIA_INFO_LOG("CameraHostInfo::OnFlashlightStatusV1_2, torch status is unavailable");
            break;

        default:
            MEDIA_ERR_LOG("CameraHostInfo::OnFlashlightStatusV1_2, Unknown flashlight status: %{public}d", status);
            return CAMERA_UNKNOWN_ERROR;
    }
    cameraHostManager_->statusCallback_->OnTorchStatus(torchStatus);
    return CAMERA_OK;
}

int32_t HCameraHostManager::CameraHostInfo::OnCameraEvent(const std::string &cameraId, CameraEvent event)
{
    if ((cameraHostManager_ == nullptr) || (cameraHostManager_->statusCallback_ == nullptr)) {
        MEDIA_WARNING_LOG("CameraHostInfo::OnCameraEvent for %{public}s with status %{public}d "
                          "failed due to no callback or cameraHostManager_ is null",
                          cameraId.c_str(), event);
        return CAMERA_UNKNOWN_ERROR;
    }
    CameraStatus svcStatus = CAMERA_STATUS_UNAVAILABLE;
    switch (event) {
        case CAMERA_EVENT_DEVICE_RMV: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraEvent, camera %{public}s unavailable", cameraId.c_str());
            svcStatus = CAMERA_STATUS_DISAPPEAR;
            RemoveDevice(cameraId);
            break;
        }
        case CAMERA_EVENT_DEVICE_ADD: {
            MEDIA_INFO_LOG("CameraHostInfo::OnCameraEvent camera %{public}s available", cameraId.c_str());
            svcStatus = CAMERA_STATUS_APPEAR;
            AddDevice(cameraId);
            break;
        }
        default:
            MEDIA_ERR_LOG("Unknown camera event: %{public}d", event);
            return CAMERA_UNKNOWN_ERROR;
    }
    cameraHostManager_->statusCallback_->OnCameraStatus(cameraId, svcStatus);
    return CAMERA_OK;
}

std::shared_ptr<HCameraHostManager::CameraDeviceInfo> HCameraHostManager::CameraHostInfo::FindCameraDeviceInfo
   (const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& deviceInfo : devices_) {
        if (deviceInfo->cameraId == cameraId) {
            MEDIA_INFO_LOG("CameraHostInfo::FindCameraDeviceInfo succeed for %{public}s", cameraId.c_str());
            return deviceInfo;
        }
    }
    MEDIA_WARNING_LOG("CameraHostInfo::FindCameraDeviceInfo failed for %{public}s", cameraId.c_str());
    return nullptr;
}

void HCameraHostManager::CameraHostInfo::AddDevice(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::none_of(devices_.begin(), devices_.end(),
                     [&cameraId](auto& devInfo) { return devInfo->cameraId == cameraId; })) {
        cameraIds_.push_back(cameraId);
        devices_.push_back(std::make_shared<HCameraHostManager::CameraDeviceInfo>(cameraId));
        MEDIA_INFO_LOG("CameraHostInfo::AddDevice, camera %{public}s added", cameraId.c_str());
    } else {
        MEDIA_WARNING_LOG("CameraHostInfo::AddDevice, camera %{public}s already exists", cameraId.c_str());
    }
}

void HCameraHostManager::CameraHostInfo::RemoveDevice(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    cameraIds_.erase(std::remove(cameraIds_.begin(), cameraIds_.end(), cameraId), cameraIds_.end());
    devices_.erase(std::remove_if(devices_.begin(), devices_.end(),
        [&cameraId](const auto& devInfo) { return devInfo->cameraId == cameraId; }),
        devices_.end());
}

bool HCameraHostManager::CameraHostInfo::IsLocalCameraHostInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& deviceInfo : devices_) {
        std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility = deviceInfo->ability;
        camera_metadata_item_t item;
        int ret = OHOS::Camera::FindCameraMetadataItem(cameraAbility->get(),
        OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &item);
        if (ret == CAM_META_SUCCESS) {
            if (static_cast<camera_connection_type_t>(item.data.u8[0]) == OHOS_CAMERA_CONNECTION_TYPE_BUILTIN) {
                MEDIA_INFO_LOG("CameraHostInfo::IsLocalCameraHostInfo succeed");
                return true;
            }
        }
    }
    return false;
}

HCameraHostManager::HCameraHostManager(StatusCallback* statusCallback)
    : statusCallback_(statusCallback), cameraHostInfos_()
{
}

HCameraHostManager::~HCameraHostManager()
{
    std::lock_guard<std::mutex> lock(deviceMutex_);
    statusCallback_ = nullptr;
    for (auto it = cameraDevices_.begin(); it != cameraDevices_.end(); it++) {
        if (it->second) {
            it->second = nullptr;
        }
    }
    cameraDevices_.clear();

    for (unsigned i = 0; i < cameraHostInfos_.size(); i++) {
        cameraHostInfos_[i] = nullptr;
    }
    cameraHostInfos_.clear();
}

int32_t HCameraHostManager::Init()
{
    MEDIA_INFO_LOG("HCameraHostManager::Init");
    using namespace OHOS::HDI::ServiceManager::V1_0;
    auto svcMgr = IServiceManager::Get();
    if (svcMgr == nullptr) {
        MEDIA_ERR_LOG("%s: IServiceManager failed!", __func__);
        return CAMERA_UNKNOWN_ERROR;
    }
    auto rt = svcMgr->RegisterServiceStatusListener(this, DEVICE_CLASS_CAMERA);
    if (rt != 0) {
        MEDIA_ERR_LOG("%s: RegisterServiceStatusListener failed!", __func__);
    }
    return rt == 0 ? CAMERA_OK : CAMERA_UNKNOWN_ERROR;
}

void HCameraHostManager::DeInit()
{
    using namespace OHOS::HDI::ServiceManager::V1_0;
    auto svcMgr = IServiceManager::Get();
    if (svcMgr == nullptr) {
        MEDIA_ERR_LOG("%s: IServiceManager failed", __func__);
        return;
    }
    auto rt = svcMgr->UnregisterServiceStatusListener(this);
    if (rt != 0) {
        MEDIA_ERR_LOG("%s: UnregisterServiceStatusListener failed!", __func__);
    }
}

void HCameraHostManager::AddCameraDevice(const std::string& cameraId, sptr<ICameraDeviceService> cameraDevice)
{
    std::lock_guard<std::mutex> lock(deviceMutex_);
    auto it = cameraDevices_.find(cameraId);
    if (it != cameraDevices_.end()) {
        it->second = nullptr;
        cameraDevices_.erase(cameraId);
    }
    cameraDevices_[cameraId] = cameraDevice;
    if (statusCallback_) {
        statusCallback_->OnCameraStatus(cameraId, CAMERA_STATUS_UNAVAILABLE);
    }
}

void HCameraHostManager::RemoveCameraDevice(const std::string& cameraId)
{
    MEDIA_DEBUG_LOG("HCameraHostManager::RemoveCameraDevice start");
    std::lock_guard<std::mutex> lock(deviceMutex_);
    auto it = cameraDevices_.find(cameraId);
    if (it != cameraDevices_.end()) {
        it->second = nullptr;
    }
    cameraDevices_.erase(cameraId);
    if (statusCallback_) {
        statusCallback_->OnCameraStatus(cameraId, CAMERA_STATUS_AVAILABLE);
    }
    MEDIA_DEBUG_LOG("HCameraHostManager::RemoveCameraDevice end");
}

void HCameraHostManager::CloseCameraDevice(const std::string& cameraId)
{
    sptr<ICameraDeviceService> deviceToDisconnect = nullptr;
    {
        std::lock_guard<std::mutex> lock(deviceMutex_);
        auto iter = cameraDevices_.find(cameraId);
        if (iter != cameraDevices_.end()) {
            deviceToDisconnect = iter->second;
        }
    }
    if (deviceToDisconnect) {
        deviceToDisconnect->Close();
    }
}

int32_t HCameraHostManager::GetCameras(std::vector<std::string>& cameraIds)
{
    CAMERA_SYNC_TRACE;
    MEDIA_INFO_LOG("HCameraHostManager::GetCameras");
    if (!IsCameraHostInfoAdded("camera_service")) {
        AddCameraHost("camera_service");
    }
    std::lock_guard<std::mutex> lock(mutex_);
    cameraIds.clear();
    for (const auto& cameraHost : cameraHostInfos_) {
        cameraHost->GetCameras(cameraIds);
    }
    return CAMERA_OK;
}

int32_t HCameraHostManager::GetCameraAbility(std::string &cameraId,
                                             std::shared_ptr<OHOS::Camera::CameraMetadata> &ability)
{
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with invalid device info.");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->GetCameraAbility(cameraId, ability);
}

int32_t HCameraHostManager::GetVersionByCamera(const std::string& cameraId)
{
    MEDIA_INFO_LOG("GetVersionByCamera camera = %{public}s", cameraId.c_str());
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("GetVersionByCamera failed with invalid device info");
        return 0;
    }
    return cameraHostInfo->GetCameraHostVersion();
}

int32_t HCameraHostManager::OpenCameraDevice(std::string &cameraId,
                                             const sptr<ICameraDeviceCallback> &callback,
                                             sptr<OHOS::HDI::Camera::V1_0::ICameraDevice> &pDevice)
{
    MEDIA_INFO_LOG("HCameraHostManager::OpenCameraDevice try to open camera = %{public}s", cameraId.c_str());
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with invalid device info");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->OpenCamera(cameraId, callback, pDevice);
}

int32_t HCameraHostManager::SetTorchLevel(float level)
{
    auto cameraHostInfo = FindLocalCameraHostInfo();
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::SetTorchLevel failed with not exist support device info");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->SetTorchLevel(level);
}

int32_t HCameraHostManager::SetFlashlight(const std::string& cameraId, bool isEnable)
{
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with invalid device info");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->SetFlashlight(cameraId, isEnable);
}

int32_t HCameraHostManager::Prelaunch(const std::string& cameraId)
{
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::OpenCameraDevice failed with invalid device info");
        return CAMERA_INVALID_ARG;
    }
    return cameraHostInfo->Prelaunch(cameraId);
}

void HCameraHostManager::NotifyDeviceStateChangeInfo(const std::string& cameraId, int notifyType, int deviceState)
{
    auto cameraHostInfo = FindCameraHostInfo(cameraId);
    if (cameraHostInfo == nullptr) {
        MEDIA_ERR_LOG("HCameraHostManager::NotifyDeviceStateChangeInfo failed with invalid device info");
        return;
    }
    cameraHostInfo->NotifyDeviceStateChangeInfo(notifyType, deviceState);
}

void HCameraHostManager::OnReceive(const HDI::ServiceManager::V1_0::ServiceStatus& status)
{
    MEDIA_INFO_LOG("HCameraHostManager::OnReceive for camera host %{public}s, status %{public}d",
        status.serviceName.c_str(), status.status);
    if (status.deviceClass != DEVICE_CLASS_CAMERA || status.serviceName != "distributed_camera_service") {
        MEDIA_ERR_LOG("HCameraHostManager::OnReceive invalid device class %{public}d", status.deviceClass);
        return;
    }
    using namespace OHOS::HDI::ServiceManager::V1_0;
    switch (status.status) {
        case SERVIE_STATUS_START:
            AddCameraHost(status.serviceName);
            break;
        case SERVIE_STATUS_STOP:
            RemoveCameraHost(status.serviceName);
            break;
        default:
            MEDIA_ERR_LOG("HCameraHostManager::OnReceive unexpected service status %{public}d", status.status);
    }
}

void HCameraHostManager::AddCameraHost(const std::string& svcName)
{
    MEDIA_INFO_LOG("HCameraHostManager::AddCameraHost camera host %{public}s added", svcName.c_str());
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::any_of(cameraHostInfos_.begin(), cameraHostInfos_.end(),
                    [&svcName](const auto& camHost) { return camHost->GetName() == svcName; })) {
        MEDIA_INFO_LOG("HCameraHostManager::AddCameraHost camera host  %{public}s already exists", svcName.c_str());
        return;
    }
    sptr<HCameraHostManager::CameraHostInfo> cameraHost = new(std::nothrow) HCameraHostManager::CameraHostInfo
                                                          (this, svcName);
    if (!cameraHost->Init()) {
        MEDIA_ERR_LOG("HCameraHostManager::AddCameraHost failed due to init failure");
        return;
    }
    cameraHostInfos_.push_back(cameraHost);
    std::vector<std::string> cameraIds;
    if (statusCallback_ && cameraHost->GetCameras(cameraIds) == CAMERA_OK) {
        for (const auto& cameraId : cameraIds) {
            statusCallback_->OnCameraStatus(cameraId, CAMERA_STATUS_AVAILABLE);
        }
    }
}

void HCameraHostManager::RemoveCameraHost(const std::string& svcName)
{
    std::lock_guard<std::mutex> lock(mutex_);
    MEDIA_INFO_LOG("HCameraHostManager::RemoveCameraHost camera host %{public}s removed", svcName.c_str());
    std::vector<sptr<CameraHostInfo>>::iterator it = std::find_if(cameraHostInfos_.begin(), cameraHostInfos_.end(),
        [&svcName](const auto& camHost) { return camHost->GetName() == svcName; });
    if (it == cameraHostInfos_.end()) {
        MEDIA_WARNING_LOG("HCameraHostManager::RemoveCameraHost camera host %{public}s doesn't exist",
            svcName.c_str());
        return;
    }
    std::vector<std::string> cameraIds;
    if ((*it)->GetCameras(cameraIds) == CAMERA_OK) {
        for (const auto& cameraId : cameraIds) {
            (*it)->OnCameraStatus(cameraId, UN_AVAILABLE);
            CloseCameraDevice(cameraId);
        }
    }
    *it = nullptr;
    cameraHostInfos_.erase(it);
}

sptr<HCameraHostManager::CameraHostInfo> HCameraHostManager::FindCameraHostInfo(const std::string& cameraId)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& cameraHostInfo : cameraHostInfos_) {
        if (cameraHostInfo->IsCameraSupported(cameraId)) {
            return cameraHostInfo;
        }
    }
    return nullptr;
}

sptr<HCameraHostManager::CameraHostInfo> HCameraHostManager::FindLocalCameraHostInfo()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<sptr<CameraHostInfo>>::iterator it = std::find_if(cameraHostInfos_.begin(), cameraHostInfos_.end(),
        [](const auto& cameraHostInfo) { return cameraHostInfo->IsLocalCameraHostInfo(); });
    if (it != cameraHostInfos_.end()) {
        return (*it);
    }
    return nullptr;
}

bool HCameraHostManager::IsCameraHostInfoAdded(const std::string& svcName)
{
    return std::any_of(cameraHostInfos_.begin(), cameraHostInfos_.end(),
                       [&svcName](const auto& camHost) {return camHost->GetName() == svcName; });
}
} // namespace CameraStandard
} // namespace OHOS
