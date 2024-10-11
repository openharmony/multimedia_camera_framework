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

#include "hcapture_session.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <new>
#include <sched.h>
#include <string>
#include <sync_fence.h>
#include <utility>
#include <vector>

#include "avcodec_task_manager.h"
#include "bundle_mgr_interface.h"
#include "camera_info_dumper.h"
#include "camera_log.h"
#include "camera_report_uitls.h"
#include "camera_server_photo_proxy.h"
#include "camera_service_ipc_interface_code.h"
#include "camera_util.h"
#include "datetime_ex.h"
#include "display/composer/v1_1/display_composer_type.h"
#include "display_manager.h"
#include "errors.h"
#include "hcamera_device_manager.h"
#include "hcamera_restore_param.h"
#include "hstream_capture.h"
#include "hstream_common.h"
#include "hstream_depth_data.h"
#include "hstream_metadata.h"
#include "hstream_repeat.h"
#include "icamera_util.h"
#include "icapture_session.h"
#include "iconsumer_surface.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "istream_common.h"
#include "media_library_manager.h"
#include "metadata_utils.h"
#include "moving_photo/moving_photo_surface_wrapper.h"
#include "moving_photo_video_cache.h"
#include "refbase.h"
#include "smooth_zoom.h"
#include "surface.h"
#include "surface_buffer.h"
#include "system_ability_definition.h"
#include "v1_0/types.h"
#include "parameters.h"

using namespace OHOS::AAFwk;
namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Display::Composer::V1_1;

namespace {
static std::map<pid_t, sptr<HCaptureSession>> g_totalSessions;
static std::mutex g_totalSessionLock;
static size_t TotalSessionSize()
{
    std::lock_guard<std::mutex> lock(g_totalSessionLock);
    return g_totalSessions.size();
}

static const std::map<pid_t, sptr<HCaptureSession>> TotalSessionsCopy()
{
    std::lock_guard<std::mutex> lock(g_totalSessionLock);
    return g_totalSessions;
}

static void TotalSessionsInsert(pid_t pid, sptr<HCaptureSession> session)
{
    std::lock_guard<std::mutex> lock(g_totalSessionLock);
    auto it = g_totalSessions.find(pid);
    if (it != g_totalSessions.end()) {
        MEDIA_ERR_LOG("HCaptureSession TotalSessionsInsert insert session but pid already exist!, memory leak may "
                      "occurred, pid is:%{public}d",
            pid);
        it->second = session;
        return;
    }
    g_totalSessions.emplace(pid, session);
}

static sptr<HCaptureSession> TotalSessionsGet(pid_t pid)
{
    std::lock_guard<std::mutex> lock(g_totalSessionLock);
    auto it = g_totalSessions.find(pid);
    CHECK_AND_RETURN_RET(it == g_totalSessions.end(), it->second);
    return nullptr;
}

static void TotalSessionErase(pid_t pid)
{
    std::lock_guard<std::mutex> lock(g_totalSessionLock);
    g_totalSessions.erase(pid);
}
} // namespace

static const std::map<CaptureSessionState, std::string> SESSION_STATE_STRING_MAP = {
    {CaptureSessionState::SESSION_INIT, "Init"},
    {CaptureSessionState::SESSION_CONFIG_INPROGRESS, "Config_In-progress"},
    {CaptureSessionState::SESSION_CONFIG_COMMITTED, "Committed"},
    {CaptureSessionState::SESSION_RELEASED, "Released"},
    {CaptureSessionState::SESSION_STARTED, "Started"}
};

sptr<HCaptureSession> HCaptureSession::NewInstance(const uint32_t callerToken, int32_t opMode)
{
    sptr<HCaptureSession> session = new HCaptureSession();
    CHECK_AND_RETURN_RET(session->Initialize(callerToken, opMode) != CAMERA_OK, session);
    return nullptr;
}

int32_t HCaptureSession::Initialize(const uint32_t callerToken, int32_t opMode)
{
    pid_ = IPCSkeleton::GetCallingPid();
    uid_ = static_cast<uint32_t>(IPCSkeleton::GetCallingUid());
    MEDIA_DEBUG_LOG("HCaptureSession: camera stub services(%{public}zu) pid(%{public}d).", TotalSessionSize(), pid_);
    if (system::GetParameter("const.camera.multicamera.enable", "false") != "true") {
        auto pidSession = TotalSessionsGet(pid_);
        if (pidSession != nullptr) {
            auto disconnectDevice = pidSession->GetCameraDevice();
            if (disconnectDevice != nullptr) {
                disconnectDevice->OnError(HDI::Camera::V1_0::DEVICE_PREEMPT, 0);
            }
            MEDIA_ERR_LOG("HCaptureSession::HCaptureSession doesn't support multiple sessions per pid");
            pidSession->Release();
        }
    }
    TotalSessionsInsert(pid_, this);
    callerToken_ = callerToken;
    opMode_ = opMode;
    featureMode_ = 0;
    CameraReportUtils::GetInstance().updateModeChangePerfInfo(opMode, CameraReportUtils::GetCallerInfo());
    MEDIA_INFO_LOG(
        "HCaptureSession: camera stub services(%{public}zu). opMode_= %{public}d", TotalSessionSize(), opMode_);
    return CAMERA_OK;
}

HCaptureSession::HCaptureSession()
{
    pid_ = 0;
    uid_ = 0;
    callerToken_ = 0;
    opMode_ = 0;
    featureMode_ = 0;
}

HCaptureSession::HCaptureSession(const uint32_t callingTokenId, int32_t opMode)
{
    Initialize(callingTokenId, opMode);
}

HCaptureSession::~HCaptureSession()
{
    Release(CaptureSessionReleaseType::RELEASE_TYPE_OBJ_DIED);
    if (displayListener_) {
        OHOS::Rosen::DisplayManager::GetInstance().UnregisterDisplayListener(displayListener_);
    }
}

pid_t HCaptureSession::GetPid()
{
    return pid_;
}

int32_t HCaptureSession::GetopMode()
{
    CHECK_AND_RETURN_RET(!featureMode_, featureMode_);
    return opMode_;
}

int32_t HCaptureSession::GetCurrentStreamInfos(std::vector<StreamInfo_V1_1>& streamInfos)
{
    auto streams = streamContainer_.GetAllStreams();
    for (auto& stream : streams) {
        if (stream) {
            StreamInfo_V1_1 curStreamInfo;
            stream->SetStreamInfo(curStreamInfo);
            if (stream->GetStreamType() != StreamType::METADATA) {
                streamInfos.push_back(curStreamInfo);
            }
        }
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::BeginConfig()
{
    CAMERA_SYNC_TRACE;
    int32_t errCode;
    MEDIA_INFO_LOG("HCaptureSession::BeginConfig prepare execute");
    stateMachine_.StateGuard([&errCode, this](const CaptureSessionState state) {
        bool isStateValid = stateMachine_.Transfer(CaptureSessionState::SESSION_CONFIG_INPROGRESS);
        if (!isStateValid) {
            MEDIA_ERR_LOG("HCaptureSession::BeginConfig in invalid state %{public}d", state);
            errCode = CAMERA_INVALID_STATE;
            return;
        }
        UnlinkInputAndOutputs();
        ClearSketchRepeatStream();
        ClearMovingPhotoRepeatStream();
    });
    if (errCode == CAMERA_OK) {
        MEDIA_INFO_LOG("HCaptureSession::BeginConfig execute success");
    } else {
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::BeginConfig", errCode, false, CameraReportUtils::GetCallerInfo());
    }
    return errCode;
}

int32_t HCaptureSession::CanAddInput(sptr<ICameraDeviceService> cameraDevice, bool& result)
{
    CAMERA_SYNC_TRACE;
    int32_t errorCode = CAMERA_OK;
    result = false;
    stateMachine_.StateGuard([this, &errorCode](const CaptureSessionState currentState) {
        if (currentState != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
            MEDIA_ERR_LOG("HCaptureSession::CanAddInput Need to call BeginConfig before adding input");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        if ((GetCameraDevice() != nullptr)) {
            MEDIA_ERR_LOG("HCaptureSession::CanAddInput Only one input is supported");
            errorCode = CAMERA_INVALID_SESSION_CFG;
            return;
        }
    });
    if (errorCode == CAMERA_OK) {
        result = true;
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::CanAddInput"));
    }
    return errorCode;
}

int32_t HCaptureSession::AddInput(sptr<ICameraDeviceService> cameraDevice)
{
    CAMERA_SYNC_TRACE;
    int32_t errorCode = CAMERA_OK;
    if (cameraDevice == nullptr) {
        errorCode = CAMERA_INVALID_ARG;
        MEDIA_ERR_LOG("HCaptureSession::AddInput cameraDevice is null");
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::AddInput", errorCode, false, CameraReportUtils::GetCallerInfo());
        return errorCode;
    }
    MEDIA_INFO_LOG("HCaptureSession::AddInput prepare execute");
    stateMachine_.StateGuard([this, &errorCode, &cameraDevice](const CaptureSessionState currentState) {
        if (currentState != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
            MEDIA_ERR_LOG("HCaptureSession::AddInput Need to call BeginConfig before adding input");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        if ((GetCameraDevice() != nullptr)) {
            MEDIA_ERR_LOG("HCaptureSession::AddInput Only one input is supported");
            errorCode = CAMERA_INVALID_SESSION_CFG;
            return;
        }
        sptr<HCameraDevice> hCameraDevice = static_cast<HCameraDevice*>(cameraDevice.GetRefPtr());
        MEDIA_INFO_LOG("HCaptureSession::AddInput device:%{public}s", hCameraDevice->GetCameraId().c_str());
        hCameraDevice->SetStreamOperatorCallback(this);
        SetCameraDevice(hCameraDevice);
        hCameraDevice->DispatchDefaultSettingToHdi();
    });
    if (errorCode == CAMERA_OK) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddInput"));
    } else {
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::AddInput", errorCode, false, CameraReportUtils::GetCallerInfo());
    }
    MEDIA_INFO_LOG("HCaptureSession::AddInput execute success");
    return errorCode;
}

int32_t HCaptureSession::AddOutputStream(sptr<HStreamCommon> stream)
{
    CHECK_ERROR_RETURN_RET_LOG(stream == nullptr, CAMERA_INVALID_ARG,
        "HCaptureSession::AddOutputStream stream is null");
    MEDIA_INFO_LOG("HCaptureSession::AddOutputStream streamId:%{public}d streamType:%{public}d",
        stream->GetFwkStreamId(), stream->GetStreamType());
    CHECK_ERROR_RETURN_RET_LOG(
        stream->GetFwkStreamId() == STREAM_ID_UNSET && stream->GetStreamType() != StreamType::METADATA,
        CAMERA_INVALID_ARG, "HCaptureSession::AddOutputStream stream is released!");
    bool isAddSuccess = streamContainer_.AddStream(stream);
    CHECK_ERROR_RETURN_RET_LOG(!isAddSuccess, CAMERA_INVALID_SESSION_CFG,
        "HCaptureSession::AddOutputStream add stream fail");
    if (stream->GetStreamType() == StreamType::CAPTURE) {
        auto captureStream = CastStream<HStreamCapture>(stream);
        captureStream->SetMode(opMode_);
        captureStream->SetColorSpace(currCaptureColorSpace_);
    } else {
        stream->SetColorSpace(currColorSpace_);
    }
    return CAMERA_OK;
}

void HCaptureSession::StartMovingPhotoStream()
{
    int32_t errorCode = 0;
    stateMachine_.StateGuard([&errorCode, this](CaptureSessionState currentState) {
        if (currentState != CaptureSessionState::SESSION_CONFIG_COMMITTED) {
            MEDIA_ERR_LOG("EnableMovingPhoto, invalid session state: %{public}d, start after preview", currentState);
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
        bool isPreviewStarted = false;
        for (auto& item : repeatStreams) {
            auto curStreamRepeat = CastStream<HStreamRepeat>(item);
            auto repeatType = curStreamRepeat->GetRepeatStreamType();
            if (repeatType != RepeatStreamType::PREVIEW) {
                continue;
            }
            if (curStreamRepeat->GetPreparedCaptureId() != CAPTURE_ID_UNSET && curStreamRepeat->producer_ != nullptr) {
                isPreviewStarted = true;
                break;
            }
        }
        if (!isPreviewStarted) {
            MEDIA_ERR_LOG("EnableMovingPhoto, preview is not streaming");
            return;
        }
        std::shared_ptr<OHOS::Camera::CameraMetadata> settings = nullptr;
        auto cameraDevice = GetCameraDevice();
        if (cameraDevice != nullptr) {
            settings = cameraDevice->CloneCachedSettings();
            DumpMetadata(settings);
        }
        for (auto& item : repeatStreams) {
            auto curStreamRepeat = CastStream<HStreamRepeat>(item);
            auto repeatType = curStreamRepeat->GetRepeatStreamType();
            if (repeatType != RepeatStreamType::LIVEPHOTO) {
                continue;
            }
            if (isSetMotionPhoto_) {
                errorCode = curStreamRepeat->Start(settings);
                std::lock_guard<std::mutex> lock(movingPhotoStatusLock_);
                audioCapturerSession_ != nullptr && audioCapturerSession_->StartAudioCapture();
            } else {
                errorCode = curStreamRepeat->Stop();
                StopMovingPhoto();
            }
            break;
        }
    });
    MEDIA_INFO_LOG("HCaptureSession::StartMovingPhotoStream result:%{public}d", errorCode);
}

class DisplayRotationListener : public OHOS::Rosen::DisplayManager::IDisplayListener {
public:
    explicit DisplayRotationListener() {};
    virtual ~DisplayRotationListener() = default;
    void OnCreate(OHOS::Rosen::DisplayId) override {}
    void OnDestroy(OHOS::Rosen::DisplayId) override {}
    void OnChange(OHOS::Rosen::DisplayId displayId) override
    {
        sptr<Rosen::Display> display = Rosen::DisplayManager::GetInstance().GetDefaultDisplay();
        if (display == nullptr) {
            MEDIA_INFO_LOG("Get display info failed, display:%{public}" PRIu64"", displayId);
            display = Rosen::DisplayManager::GetInstance().GetDisplayById(0);
            CHECK_ERROR_RETURN_LOG(display == nullptr, "Get display info failed, display is nullptr");
        }
        {
            Rosen::Rotation currentRotation = display->GetRotation();
            std::lock_guard<std::mutex> lock(mStreamManagerLock_);
            for (auto& repeatStream : repeatStreamList_) {
                if (repeatStream) {
                    repeatStream->SetStreamTransform(static_cast<int> (currentRotation));
                }
            }
        }
    }

    void AddHstreamRepeatForListener(sptr<HStreamRepeat> repeatStream)
    {
        std::lock_guard<std::mutex> lock(mStreamManagerLock_);
        if (repeatStream) {
            repeatStreamList_.push_back(repeatStream);
        }
    }

    void RemoveHstreamRepeatForListener(sptr<HStreamRepeat> repeatStream)
    {
        std::lock_guard<std::mutex> lock(mStreamManagerLock_);
        if (repeatStream) {
            repeatStreamList_.erase(std::remove(repeatStreamList_.begin(), repeatStreamList_.end(), repeatStream),
                repeatStreamList_.end());
        }
    }

public:
    std::list<sptr<HStreamRepeat>> repeatStreamList_;
    std::mutex mStreamManagerLock_;
};

void HCaptureSession::RegisterDisplayListener(sptr<HStreamRepeat> repeat)
{
    if (displayListener_ == nullptr) {
        displayListener_ = new DisplayRotationListener();
        OHOS::Rosen::DisplayManager::GetInstance().RegisterDisplayListener(displayListener_);
    }
    displayListener_->AddHstreamRepeatForListener(repeat);
}

void HCaptureSession::UnRegisterDisplayListener(sptr<HStreamRepeat> repeatStream)
{
    if (displayListener_) {
        displayListener_->RemoveHstreamRepeatForListener(repeatStream);
    }
}

int32_t HCaptureSession::SetPreviewRotation(std::string &deviceClass)
{
    enableStreamRotate_ = true;
    deviceClass_ = deviceClass;
    return CAMERA_OK;
}

int32_t HCaptureSession::AddOutput(StreamType streamType, sptr<IStreamCommon> stream)
{
    int32_t errorCode = CAMERA_INVALID_ARG;
    if (stream == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::AddOutput stream is null");
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::AddOutput", errorCode, false, CameraReportUtils::GetCallerInfo());
        return errorCode;
    }
    stateMachine_.StateGuard([this, &errorCode, streamType, &stream](const CaptureSessionState currentState) {
        if (currentState != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
            MEDIA_ERR_LOG("HCaptureSession::AddOutput Need to call BeginConfig before adding output");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        // Temp hack to fix the library linking issue
        sptr<IConsumerSurface> captureSurface = IConsumerSurface::Create();
        if (streamType == StreamType::CAPTURE) {
            errorCode = AddOutputStream(static_cast<HStreamCapture*>(stream.GetRefPtr()));
        } else if (streamType == StreamType::REPEAT) {
            HStreamRepeat* repeatSteam = static_cast<HStreamRepeat*>(stream.GetRefPtr());
            if (enableStreamRotate_ && repeatSteam != nullptr &&
                repeatSteam->GetRepeatStreamType() == RepeatStreamType::PREVIEW) {
                RegisterDisplayListener(repeatSteam);
                repeatSteam->SetPreviewRotation(deviceClass_);
            }
            errorCode = AddOutputStream(repeatSteam);
        } else if (streamType == StreamType::METADATA) {
            errorCode = AddOutputStream(static_cast<HStreamMetadata*>(stream.GetRefPtr()));
        } else if (streamType == StreamType::DEPTH) {
            errorCode = AddOutputStream(static_cast<HStreamDepthData*>(stream.GetRefPtr()));
        }
    });
    if (errorCode == CAMERA_OK) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::AddOutput with %d", streamType));
    } else {
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::AddOutput", errorCode, false, CameraReportUtils::GetCallerInfo());
    }
    MEDIA_INFO_LOG("CaptureSession::AddOutput with with %{public}d, rc = %{public}d", streamType, errorCode);
    return errorCode;
}

int32_t HCaptureSession::RemoveInput(sptr<ICameraDeviceService> cameraDevice)
{
    int32_t errorCode = CAMERA_OK;
    if (cameraDevice == nullptr) {
        errorCode = CAMERA_INVALID_ARG;
        MEDIA_ERR_LOG("HCaptureSession::RemoveInput cameraDevice is null");
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::RemoveInput", errorCode, false, CameraReportUtils::GetCallerInfo());
        return errorCode;
    }
    MEDIA_INFO_LOG("HCaptureSession::RemoveInput prepare execute");
    stateMachine_.StateGuard([this, &errorCode, &cameraDevice](const CaptureSessionState currentState) {
        if (currentState != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
            MEDIA_ERR_LOG("HCaptureSession::RemoveInput Need to call BeginConfig before removing input");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        auto currentDevice = GetCameraDevice();
        if (currentDevice != nullptr && cameraDevice->AsObject() == currentDevice->AsObject()) {
            // Do not close device while remove input!
            MEDIA_INFO_LOG(
                "HCaptureSession::RemoveInput camera id is %{public}s", currentDevice->GetCameraId().c_str());
            currentDevice->ResetDeviceSettings();
            SetCameraDevice(nullptr);
        } else {
            MEDIA_ERR_LOG("HCaptureSession::RemoveInput Invalid camera device");
            errorCode = CAMERA_INVALID_SESSION_CFG;
        }
    });
    if (errorCode == CAMERA_OK) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemoveInput"));
    } else {
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::RemoveInput", errorCode, false, CameraReportUtils::GetCallerInfo());
    }
    MEDIA_INFO_LOG("HCaptureSession::RemoveInput execute success");
    return errorCode;
}

int32_t HCaptureSession::RemoveOutputStream(sptr<HStreamCommon> stream)
{
    CAMERA_SYNC_TRACE;
    CHECK_ERROR_RETURN_RET_LOG(stream == nullptr, CAMERA_INVALID_ARG,
        "HCaptureSession::RemoveOutputStream stream is null");
    MEDIA_INFO_LOG("HCaptureSession::RemoveOutputStream,streamType:%{public}d, streamId:%{public}d",
        stream->GetStreamType(), stream->GetFwkStreamId());
    bool isRemoveSuccess = streamContainer_.RemoveStream(stream);
    CHECK_ERROR_RETURN_RET_LOG(!isRemoveSuccess, CAMERA_INVALID_SESSION_CFG,
        "HCaptureSession::RemoveOutputStream Invalid output");
    return CAMERA_OK;
}

int32_t HCaptureSession::RemoveOutput(StreamType streamType, sptr<IStreamCommon> stream)
{
    int32_t errorCode = CAMERA_INVALID_ARG;
    if (stream == nullptr) {
        MEDIA_ERR_LOG("HCaptureSession::RemoveOutput stream is null");
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::RemoveOutput", errorCode, false, CameraReportUtils::GetCallerInfo());
        return errorCode;
    }
    MEDIA_INFO_LOG("HCaptureSession::RemoveOutput prepare execute");
    stateMachine_.StateGuard([this, &errorCode, streamType, &stream](const CaptureSessionState currentState) {
        if (currentState != CaptureSessionState::SESSION_CONFIG_INPROGRESS) {
            MEDIA_ERR_LOG("HCaptureSession::RemoveOutput Need to call BeginConfig before removing output");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        if (streamType == StreamType::CAPTURE) {
            errorCode = RemoveOutputStream(static_cast<HStreamCapture*>(stream.GetRefPtr()));
        } else if (streamType == StreamType::REPEAT) {
            HStreamRepeat* repeatSteam = static_cast<HStreamRepeat*>(stream.GetRefPtr());
            if (enableStreamRotate_ && repeatSteam != nullptr &&
                repeatSteam->GetRepeatStreamType() == RepeatStreamType::PREVIEW) {
                UnRegisterDisplayListener(repeatSteam);
            }
            errorCode = RemoveOutputStream(repeatSteam);
        } else if (streamType == StreamType::METADATA) {
            errorCode = RemoveOutputStream(static_cast<HStreamMetadata*>(stream.GetRefPtr()));
        }
    });
    if (errorCode == CAMERA_OK) {
        CAMERA_SYSEVENT_STATISTIC(CreateMsg("CaptureSession::RemoveOutput with %d", streamType));
    } else {
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::RemoveOutput", errorCode, false, CameraReportUtils::GetCallerInfo());
    }
    MEDIA_INFO_LOG("HCaptureSession::RemoveOutput execute success");
    return errorCode;
}

int32_t HCaptureSession::ValidateSessionInputs()
{
    CHECK_ERROR_RETURN_RET_LOG(GetCameraDevice() == nullptr, CAMERA_INVALID_SESSION_CFG,
        "HCaptureSession::ValidateSessionInputs No inputs present");
    return CAMERA_OK;
}

int32_t HCaptureSession::ValidateSessionOutputs()
{
    CHECK_ERROR_RETURN_RET_LOG(streamContainer_.Size() == 0, CAMERA_INVALID_SESSION_CFG,
        "HCaptureSession::ValidateSessionOutputs No outputs present");
    return CAMERA_OK;
}

int32_t HCaptureSession::LinkInputAndOutputs()
{
    int32_t rc;
    std::vector<StreamInfo_V1_1> allStreamInfos;
    sptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator;
    auto device = GetCameraDevice();
    MEDIA_INFO_LOG("HCaptureSession::LinkInputAndOutputs prepare execute");
    CHECK_ERROR_RETURN_RET_LOG(device == nullptr, CAMERA_INVALID_SESSION_CFG,
        "HCaptureSession::LinkInputAndOutputs device is null");
    auto settings = device->GetDeviceAbility();
    CHECK_ERROR_RETURN_RET_LOG(settings == nullptr, CAMERA_UNKNOWN_ERROR,
        "HCaptureSession::LinkInputAndOutputs deviceAbility is null");
    streamOperator = device->GetStreamOperator();
    auto allStream = streamContainer_.GetAllStreams();
    MEDIA_INFO_LOG("HCaptureSession::LinkInputAndOutputs allStream size:%{public}zu", allStream.size());
    CHECK_ERROR_RETURN_RET_LOG(!IsValidMode(opMode_, settings), CAMERA_INVALID_SESSION_CFG,
        "HCaptureSession::LinkInputAndOutputs IsValidMode false");
    for (auto& stream : allStream) {
        rc = stream->LinkInput(streamOperator, settings);
        if (rc == CAMERA_OK) {
            stream->SetHdiStreamId(device->GenerateHdiStreamId());
        }
        MEDIA_INFO_LOG(
            "HCaptureSession::LinkInputAndOutputs streamType:%{public}d, streamId:%{public}d ,hdiStreamId:%{public}d",
            stream->GetStreamType(), stream->GetFwkStreamId(), stream->GetHdiStreamId());
        CHECK_ERROR_RETURN_RET_LOG(rc != CAMERA_OK, rc, "HCaptureSession::LinkInputAndOutputs IsValidMode false");
        StreamInfo_V1_1 curStreamInfo;
        stream->SetStreamInfo(curStreamInfo);
        if (stream->GetStreamType() != StreamType::METADATA) {
            allStreamInfos.push_back(curStreamInfo);
        }
    }

    rc = device->CreateAndCommitStreams(allStreamInfos, settings, GetopMode());
    MEDIA_INFO_LOG("HCaptureSession::LinkInputAndOutputs execute success");
    return rc;
}

int32_t HCaptureSession::UnlinkInputAndOutputs()
{
    CAMERA_SYNC_TRACE;
    int32_t rc = CAMERA_UNKNOWN_ERROR;
    std::vector<int32_t> fwkStreamIds;
    std::vector<int32_t> hdiStreamIds;
    auto allStream = streamContainer_.GetAllStreams();
    for (auto& stream : allStream) {
        fwkStreamIds.emplace_back(stream->GetFwkStreamId());
        hdiStreamIds.emplace_back(stream->GetHdiStreamId());
        stream->UnlinkInput();
    }
    MEDIA_INFO_LOG("HCaptureSession::UnlinkInputAndOutputs() streamIds size() = %{public}zu, streamIds:%{public}s, "
                   "hdiStreamIds:%{public}s",
        fwkStreamIds.size(), Container2String(fwkStreamIds.begin(), fwkStreamIds.end()).c_str(),
        Container2String(hdiStreamIds.begin(), hdiStreamIds.end()).c_str());

    // HDI release streams, do not clear streamContainer_
    auto cameraDevice = GetCameraDevice();
    if ((cameraDevice != nullptr)) {
        cameraDevice->ReleaseStreams(hdiStreamIds);
        std::vector<StreamInfo_V1_1> emptyStreams;
        cameraDevice->UpdateStreams(emptyStreams);
        cameraDevice->ResetHdiStreamId();
    }
    return rc;
}

void HCaptureSession::ExpandSketchRepeatStream()
{
    MEDIA_DEBUG_LOG("Enter HCaptureSession::ExpandSketchRepeatStream()");
    std::set<sptr<HStreamCommon>> sketchStreams;
    auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
    for (auto& stream : repeatStreams) {
        if (stream == nullptr) {
            continue;
        }
        auto streamRepeat = CastStream<HStreamRepeat>(stream);
        if (streamRepeat->GetRepeatStreamType() == RepeatStreamType::SKETCH) {
            continue;
        }
        sptr<HStreamRepeat> sketchStream = streamRepeat->GetSketchStream();
        if (sketchStream == nullptr) {
            continue;
        }
        sketchStreams.insert(sketchStream);
    }
    MEDIA_DEBUG_LOG("HCaptureSession::ExpandSketchRepeatStream() sketch size is:%{public}zu", sketchStreams.size());
    for (auto& stream : sketchStreams) {
        AddOutputStream(stream);
    }
    MEDIA_DEBUG_LOG("Exit HCaptureSession::ExpandSketchRepeatStream()");
}

void HCaptureSession::ExpandMovingPhotoRepeatStream()
{
    CAMERA_SYNC_TRACE;
    if (!GetCameraDevice()->CheckMovingPhotoSupported(GetopMode())) {
        MEDIA_DEBUG_LOG("movingPhoto is not supported");
        return;
    }
    MEDIA_DEBUG_LOG("Enter HCaptureSession::ExpandMovingPhotoRepeatStream()");
    auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
    for (auto& stream : repeatStreams) {
        if (stream == nullptr) {
            continue;
        }
        auto streamRepeat = CastStream<HStreamRepeat>(stream);
        if (streamRepeat->GetRepeatStreamType() == RepeatStreamType::PREVIEW) {
            std::lock_guard<std::mutex> lock(movingPhotoStatusLock_);
            auto movingPhotoSurfaceWrapper =
                MovingPhotoSurfaceWrapper::CreateMovingPhotoSurfaceWrapper(streamRepeat->width_, streamRepeat->height_);
            if (movingPhotoSurfaceWrapper == nullptr) {
                MEDIA_ERR_LOG("HCaptureSession::ExpandMovingPhotoRepeatStream CreateMovingPhotoSurfaceWrapper fail.");
                continue;
            }
            auto producer = movingPhotoSurfaceWrapper->GetProducer();
            if (producer == nullptr) {
                MEDIA_ERR_LOG("HCaptureSession::ExpandMovingPhotoRepeatStream get producer fail.");
                continue;
            }
            livephotoListener_ = new (std::nothrow) MovingPhotoListener(movingPhotoSurfaceWrapper);
            if (livephotoListener_ == nullptr) {
                MEDIA_ERR_LOG("failed to new livephotoListener_!");
                continue;
            }
            movingPhotoSurfaceWrapper->SetSurfaceBufferListener(livephotoListener_);
            CreateMovingPhotoStreamRepeat(streamRepeat->format_, streamRepeat->width_, streamRepeat->height_, producer);
            std::lock_guard<std::mutex> streamLock(livePhotoStreamLock_);
            AddOutputStream(livePhotoStreamRepeat_);
            if (!audioCapturerSession_) {
                audioCapturerSession_ = new AudioCapturerSession();
            }
            if (!taskManager_ && audioCapturerSession_) {
                taskManager_ = new AvcodecTaskManager(audioCapturerSession_);
            }
            if (!videoCache_ && taskManager_) {
                videoCache_ = new MovingPhotoVideoCache(taskManager_);
            }
            break;
        }
    }
    MEDIA_DEBUG_LOG("Exit HCaptureSession::ExpandMovingPhotoRepeatStream()");
}

int32_t HCaptureSession::CreateMovingPhotoStreamRepeat(
    int32_t format, int32_t width, int32_t height, sptr<OHOS::IBufferProducer> producer)
{
    CAMERA_SYNC_TRACE;
    std::lock_guard<std::mutex> lock(livePhotoStreamLock_);
    CHECK_ERROR_RETURN_RET_LOG(width <= 0 || height <= 0, CAMERA_INVALID_ARG,
        "HCameraService::CreateLivePhotoStreamRepeat args is illegal");
    if (livePhotoStreamRepeat_ != nullptr) {
        livePhotoStreamRepeat_->Release();
    }
    auto streamRepeat = new (std::nothrow) HStreamRepeat(producer, format, width, height, RepeatStreamType::LIVEPHOTO);
    CHECK_AND_RETURN_RET_LOG(streamRepeat != nullptr, CAMERA_ALLOC_ERROR, "HStreamRepeat allocation failed");
    MEDIA_DEBUG_LOG("para is:%{public}dx%{public}d,%{public}d", width, height, format);
    livePhotoStreamRepeat_ = streamRepeat;
    MEDIA_INFO_LOG("HCameraService::CreateLivePhotoStreamRepeat end");
    return CAMERA_OK;
}

const sptr<HStreamCommon> HCaptureSession::GetStreamByStreamID(int32_t streamId)
{
    auto stream = streamContainer_.GetStream(streamId);
    CHECK_ERROR_PRINT_LOG(stream == nullptr,
        "HCaptureSession::GetStreamByStreamID get stream fail, streamId is:%{public}d", streamId);
    return stream;
}

const sptr<HStreamCommon> HCaptureSession::GetHdiStreamByStreamID(int32_t streamId)
{
    auto stream = streamContainer_.GetHdiStream(streamId);
    CHECK_ERROR_PRINT_LOG(stream == nullptr,
        "HCaptureSession::GetHdiStreamByStreamID get stream fail, streamId is:%{public}d", streamId);
    return stream;
}

void HCaptureSession::ClearSketchRepeatStream()
{
    MEDIA_DEBUG_LOG("Enter HCaptureSession::ClearSketchRepeatStream()");

    // Already added session lock in BeginConfig()
    auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
    for (auto& repeatStream : repeatStreams) {
        if (repeatStream == nullptr) {
            continue;
        }
        auto sketchStream = CastStream<HStreamRepeat>(repeatStream);
        if (sketchStream->GetRepeatStreamType() != RepeatStreamType::SKETCH) {
            continue;
        }
        MEDIA_DEBUG_LOG(
            "HCaptureSession::ClearSketchRepeatStream() stream id is:%{public}d", sketchStream->GetFwkStreamId());
        RemoveOutputStream(repeatStream);
    }
    MEDIA_DEBUG_LOG("Exit HCaptureSession::ClearSketchRepeatStream()");
}

void HCaptureSession::ClearMovingPhotoRepeatStream()
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter HCaptureSession::ClearMovingPhotoRepeatStream()");
    // Already added session lock in BeginConfig()
    auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
    for (auto& repeatStream : repeatStreams) {
        if (repeatStream == nullptr) {
            continue;
        }
        auto movingPhotoStream = CastStream<HStreamRepeat>(repeatStream);
        if (movingPhotoStream->GetRepeatStreamType() != RepeatStreamType::LIVEPHOTO) {
            continue;
        }
        StopMovingPhoto();
        std::lock_guard<std::mutex> lock(movingPhotoStatusLock_);
        livephotoListener_ = nullptr;
        videoCache_ = nullptr;
        taskManager_ = nullptr;
        audioCapturerSession_ = nullptr;
        MEDIA_DEBUG_LOG("HCaptureSession::ClearLivePhotoRepeatStream() stream id is:%{public}d",
            movingPhotoStream->GetFwkStreamId());
        RemoveOutputStream(repeatStream);
    }
    MEDIA_DEBUG_LOG("Exit HCaptureSession::ClearLivePhotoRepeatStream()");
}

void HCaptureSession::StopMovingPhoto() __attribute__((no_sanitize("cfi")))
{
    CAMERA_SYNC_TRACE;
    MEDIA_DEBUG_LOG("Enter HCaptureSession::StopMovingPhoto");
    std::lock_guard<std::mutex> lock(movingPhotoStatusLock_);
    if (livephotoListener_) {
        livephotoListener_->StopDrainOut();
    }
    if (videoCache_) {
        videoCache_->ClearCache();
    }
    if (audioCapturerSession_) {
        audioCapturerSession_->Stop();
    }
    if (taskManager_) {
        taskManager_->Stop();
    }
}

int32_t HCaptureSession::CommitConfig()
{
    CAMERA_SYNC_TRACE;
    MEDIA_INFO_LOG("HCaptureSession::CommitConfig begin");
    int32_t errorCode = CAMERA_OK;
    stateMachine_.StateGuard([&errorCode, this](CaptureSessionState currentState) {
        bool isTransferSupport = stateMachine_.CheckTransfer(CaptureSessionState::SESSION_CONFIG_COMMITTED);
        if (!isTransferSupport) {
            MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Need to call BeginConfig before committing configuration");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        errorCode = ValidateSessionInputs();
        if (errorCode != CAMERA_OK) {
            return;
        }
        errorCode = ValidateSessionOutputs();
        if (errorCode != CAMERA_OK) {
            return;
        }
        // expand moving photo always
        ExpandMovingPhotoRepeatStream();
        ExpandSketchRepeatStream();
        auto device = GetCameraDevice();
        if (device == nullptr) {
            MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Failed to commit config. camera device is null");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        const int32_t secureMode = 15;
        uint64_t secureSeqId = 0L;
        device ->GetSecureCameraSeq(&secureSeqId);
        if (((GetopMode() == secureMode) ^ (secureSeqId != 0))) {
            MEDIA_ERR_LOG("secureCamera is not allowed commit mode = %{public}d.", GetopMode());
            errorCode = CAMERA_OPERATION_NOT_ALLOWED;
            return;
        }

        MEDIA_INFO_LOG("HCaptureSession::CommitConfig secureSeqId = %{public}" PRIu64 "", secureSeqId);
        errorCode = LinkInputAndOutputs();
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::CommitConfig() Failed to commit config. rc: %{public}d", errorCode);
            return;
        }
        stateMachine_.Transfer(CaptureSessionState::SESSION_CONFIG_COMMITTED);
    });
    if (errorCode != CAMERA_OK) {
        CameraReportUtils::ReportCameraError(
            "HCaptureSession::CommitConfig", errorCode, false, CameraReportUtils::GetCallerInfo());
    }
    MEDIA_INFO_LOG("HCaptureSession::CommitConfig end");
    return errorCode;
}

int32_t HCaptureSession::GetActiveColorSpace(ColorSpace& colorSpace)
{
    colorSpace = currColorSpace_;
    return CAMERA_OK;
}

int32_t HCaptureSession::SetColorSpace(ColorSpace colorSpace, ColorSpace captureColorSpace, bool isNeedUpdate)
{
    int32_t result = CAMERA_OK;
    CHECK_ERROR_RETURN_RET_LOG(colorSpace == currColorSpace_ && captureColorSpace == currCaptureColorSpace_, result,
        "HCaptureSession::SetColorSpace() colorSpace no need to update.");
    stateMachine_.StateGuard(
        [&result, this, &colorSpace, &captureColorSpace, &isNeedUpdate](CaptureSessionState currentState) {
            if (!(currentState == CaptureSessionState::SESSION_CONFIG_INPROGRESS ||
                    currentState == CaptureSessionState::SESSION_CONFIG_COMMITTED)) {
                MEDIA_ERR_LOG("HCaptureSession::SetColorSpace(), Invalid session state: %{public}d", currentState);
                result = CAMERA_INVALID_STATE;
                return;
            }

            currColorSpace_ = colorSpace;
            currCaptureColorSpace_ = captureColorSpace;
            MEDIA_INFO_LOG("HCaptureSession::SetColorSpace() colorSpace %{public}d, captureColorSpace %{public}d, "
                "isNeedUpdate %{public}d", colorSpace, captureColorSpace, isNeedUpdate);

            result = CheckIfColorSpaceMatchesFormat(colorSpace);
            if (result != CAMERA_OK && isNeedUpdate) {
                MEDIA_ERR_LOG("HCaptureSession::SetColorSpace() Failed, format and colorSpace not match.");
                return;
            }
            if (result != CAMERA_OK && !isNeedUpdate) {
                MEDIA_ERR_LOG("HCaptureSession::SetColorSpace() %{public}d, format and colorSpace not match.", result);
                currColorSpace_ = ColorSpace::BT709;
            }

            SetColorSpaceForStreams();

            if (isNeedUpdate) {
                result = UpdateStreamInfos();
            }
        });
    return result;
}

void HCaptureSession::SetColorSpaceForStreams()
{
    auto streams = streamContainer_.GetAllStreams();
    for (auto& stream : streams) {
        MEDIA_DEBUG_LOG("HCaptureSession::SetColorSpaceForStreams() streams type %{public}d", stream->GetStreamType());
        if (stream->GetStreamType() == StreamType::CAPTURE) {
            stream->SetColorSpace(currCaptureColorSpace_);
        } else {
            stream->SetColorSpace(currColorSpace_);
        }
    }
}

void HCaptureSession::CancelStreamsAndGetStreamInfos(std::vector<StreamInfo_V1_1>& streamInfos)
{
    MEDIA_INFO_LOG("HCaptureSession::CancelStreamsAndGetStreamInfos enter.");
    StreamInfo_V1_1 curStreamInfo;
    auto streams = streamContainer_.GetAllStreams();
    for (auto& stream : streams) {
        if (stream && stream->GetStreamType() == StreamType::METADATA) {
            continue;
        }
        if (stream && stream->GetStreamType() == StreamType::CAPTURE && isSessionStarted_) {
            static_cast<HStreamCapture*>(stream.GetRefPtr())->CancelCapture();
        } else if (stream && stream->GetStreamType() == StreamType::REPEAT && isSessionStarted_) {
            static_cast<HStreamRepeat*>(stream.GetRefPtr())->Stop();
        }
        if (stream) {
            stream->SetStreamInfo(curStreamInfo);
            streamInfos.push_back(curStreamInfo);
        }
    }
}

void HCaptureSession::RestartStreams()
{
    MEDIA_INFO_LOG("HCaptureSession::RestartStreams() enter.");
    if (!isSessionStarted_) {
        MEDIA_DEBUG_LOG("HCaptureSession::RestartStreams() session is not started yet.");
        return;
    }
    auto cameraDevice = GetCameraDevice();
    if (cameraDevice == nullptr) {
        return;
    }
    auto streams = streamContainer_.GetAllStreams();
    for (auto& stream : streams) {
        if (stream && stream->GetStreamType() == StreamType::REPEAT &&
            CastStream<HStreamRepeat>(stream)->GetRepeatStreamType() == RepeatStreamType::PREVIEW) {
            std::shared_ptr<OHOS::Camera::CameraMetadata> settings = cameraDevice->CloneCachedSettings();
            MEDIA_INFO_LOG("HCaptureSession::RestartStreams() CloneCachedSettings");
            DumpMetadata(settings);
            CastStream<HStreamRepeat>(stream)->Start(settings);
        }
    }
}

int32_t HCaptureSession::UpdateStreamInfos()
{
    std::vector<StreamInfo_V1_1> streamInfos;
    CancelStreamsAndGetStreamInfos(streamInfos);

    auto cameraDevice = GetCameraDevice();
    CHECK_ERROR_RETURN_RET_LOG(cameraDevice == nullptr, CAMERA_UNKNOWN_ERROR,
        "HCaptureSession::UpdateStreamInfos() cameraDevice is null");
    int errorCode = cameraDevice->UpdateStreams(streamInfos);
    if (errorCode == CAMERA_OK) {
        RestartStreams();
    } else {
        MEDIA_DEBUG_LOG("HCaptureSession::UpdateStreamInfos err %{public}d", errorCode);
    }
    return errorCode;
}

int32_t HCaptureSession::CheckIfColorSpaceMatchesFormat(ColorSpace colorSpace)
{
    if (!(colorSpace == ColorSpace::BT2020_HLG || colorSpace == ColorSpace::BT2020_PQ ||
        colorSpace == ColorSpace::BT2020_HLG_LIMIT || colorSpace == ColorSpace::BT2020_PQ_LIMIT)) {
        return CAMERA_OK;
    }

    // 选择BT2020，需要匹配10bit的format；若不匹配，返回error
    auto streams = streamContainer_.GetAllStreams();
    for (auto& curStream : streams) {
        if (!curStream) {
            continue;
        }
        // 当前拍照流不支持BT2020，无需校验format
        if (curStream->GetStreamType() != StreamType::REPEAT) {
            continue;
        }
        StreamInfo_V1_1 curStreamInfo;
        curStream->SetStreamInfo(curStreamInfo);
        MEDIA_INFO_LOG("HCaptureSession::CheckFormat, stream repeatType: %{public}d, format: %{public}d",
            static_cast<HStreamRepeat*>(curStream.GetRefPtr())->GetRepeatStreamType(), curStreamInfo.v1_0.format_);
        if (!(curStreamInfo.v1_0.format_ == OHOS::HDI::Display::Composer::V1_1::PIXEL_FMT_YCBCR_P010 ||
            curStreamInfo.v1_0.format_ == OHOS::HDI::Display::Composer::V1_1::PIXEL_FMT_YCRCB_P010)) {
            MEDIA_ERR_LOG("HCaptureSession::CheckFormat, stream format not match");
            return CAMERA_OPERATION_NOT_ALLOWED;
        }
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::GetSessionState(CaptureSessionState& sessionState)
{
    sessionState = stateMachine_.GetCurrentState();
    return CAMERA_OK;
}

bool HCaptureSession::QueryFpsAndZoomRatio(float& currentFps, float& currentZoomRatio)
{
    auto cameraDevice = GetCameraDevice();
    CHECK_ERROR_RETURN_RET_LOG(cameraDevice == nullptr, false,
        "HCaptureSession::QueryFpsAndZoomRatio() cameraDevice is null");
    int32_t DEFAULT_ITEMS = 2;
    int32_t DEFAULT_DATA_LENGTH = 100;
    std::shared_ptr<OHOS::Camera::CameraMetadata> metaIn =
        std::make_shared<OHOS::Camera::CameraMetadata>(DEFAULT_ITEMS, DEFAULT_DATA_LENGTH);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metaOut =
        std::make_shared<OHOS::Camera::CameraMetadata>(DEFAULT_ITEMS, DEFAULT_DATA_LENGTH);
    uint32_t count = 1;
    uint32_t fps = 30;
    uint32_t zoomRatio = 100;
    metaIn->addEntry(OHOS_STATUS_CAMERA_CURRENT_FPS, &fps, count);
    metaIn->addEntry(OHOS_STATUS_CAMERA_CURRENT_ZOOM_RATIO, &zoomRatio, count);
    cameraDevice->GetStatus(metaIn, metaOut);

    camera_metadata_item_t item;
    int retFindMeta = OHOS::Camera::FindCameraMetadataItem(metaOut->get(),
        OHOS_STATUS_CAMERA_CURRENT_ZOOM_RATIO, &item);
    if (retFindMeta == CAM_META_ITEM_NOT_FOUND) {
        MEDIA_ERR_LOG("HCaptureSession::QueryFpsAndZoomRatio() current zoom not found");
        return false;
    } else if (retFindMeta == CAM_META_SUCCESS) {
        currentZoomRatio = static_cast<float>(item.data.ui32[0]);
        MEDIA_INFO_LOG("HCaptureSession::QueryFpsAndZoomRatio() current zoom %{public}d.", item.data.ui32[0]);
    }
    retFindMeta = OHOS::Camera::FindCameraMetadataItem(metaOut->get(), OHOS_STATUS_CAMERA_CURRENT_FPS, &item);
    if (retFindMeta == CAM_META_ITEM_NOT_FOUND) {
        MEDIA_ERR_LOG("HCaptureSession::QueryFpsAndZoomRatio() current fps not found");
        return false;
    } else if (retFindMeta == CAM_META_SUCCESS) {
        currentFps = static_cast<float>(item.data.ui32[0]);
        MEDIA_INFO_LOG("HCaptureSession::QueryFpsAndZoomRatio() current fps %{public}d.", item.data.ui32[0]);
    }
    return true;
}

bool HCaptureSession::QueryZoomPerformance(std::vector<float>& crossZoomAndTime, int32_t operationMode)
{
    auto cameraDevice = GetCameraDevice();
    CHECK_ERROR_RETURN_RET_LOG(cameraDevice == nullptr, false,
        "HCaptureSession::QueryZoomPerformance() cameraDevice is null");
    // query zoom performance. begin
    std::shared_ptr<OHOS::Camera::CameraMetadata> ability = cameraDevice->GetDeviceAbility();
    camera_metadata_item_t zoomItem;
    int retFindMeta =
        OHOS::Camera::FindCameraMetadataItem(ability->get(), OHOS_ABILITY_CAMERA_ZOOM_PERFORMANCE, &zoomItem);
    if (retFindMeta == CAM_META_ITEM_NOT_FOUND) {
        MEDIA_ERR_LOG("HCaptureSession::QueryZoomPerformance() current zoom not found");
        return false;
    } else if (retFindMeta == CAM_META_SUCCESS) {
        MEDIA_DEBUG_LOG("HCaptureSession::QueryZoomPerformance() zoom performance count %{public}d.", zoomItem.count);
        for (int i = 0; i < static_cast<int>(zoomItem.count); i++) {
            MEDIA_DEBUG_LOG(
                "HCaptureSession::QueryZoomPerformance() zoom performance value %{public}d.", zoomItem.data.ui32[i]);
        }
    }
    int dataLenPerPoint = 3;
    int headLenPerMode = 2;
    MEDIA_DEBUG_LOG("HCaptureSession::QueryZoomPerformance() operationMode %{public}d.",
        static_cast<OHOS::HDI::Camera::V1_3::OperationMode>(operationMode));
    for (int i = 0; i < static_cast<int>(zoomItem.count);) {
        int sceneMode = static_cast<int>(zoomItem.data.ui32[i]);
        int zoomPointsNum = static_cast<int>(zoomItem.data.ui32[i + 1]);
        if (static_cast<OHOS::HDI::Camera::V1_3::OperationMode>(operationMode) == sceneMode) {
            for (int j = 0; j < dataLenPerPoint * zoomPointsNum; j++) {
                crossZoomAndTime.push_back(zoomItem.data.ui32[i + headLenPerMode + j]);
                MEDIA_DEBUG_LOG("HCaptureSession::QueryZoomPerformance()  crossZoomAndTime %{public}d.",
                    static_cast<int>(zoomItem.data.ui32[i + headLenPerMode + j]));
            }
            break;
        } else {
            i = i + 1 + zoomPointsNum * dataLenPerPoint + 1;
        }
    }
    return true;
}

int32_t HCaptureSession::GetSensorOritation()
{
    auto cameraDevice = GetCameraDevice();
    int32_t sensorOrientation = 0;
    CHECK_ERROR_RETURN_RET_LOG(cameraDevice == nullptr, sensorOrientation,
        "HCaptureSession::GetSensorOritation() cameraDevice is null");
    std::shared_ptr<OHOS::Camera::CameraMetadata> ability = cameraDevice->GetDeviceAbility();
    CHECK_AND_RETURN_RET(ability != nullptr, sensorOrientation);
    camera_metadata_item_t item;
    int ret = OHOS::Camera::FindCameraMetadataItem(ability->get(), OHOS_SENSOR_ORIENTATION, &item);
    CHECK_ERROR_RETURN_RET_LOG(ret != CAM_META_SUCCESS, sensorOrientation,
        "HCaptureSession::GetSensorOritation get sensor orientation failed");
    sensorOrientation = item.data.i32[0];
    MEDIA_INFO_LOG("HCaptureSession::GetSensorOritation sensor orientation %{public}d", sensorOrientation);
    return sensorOrientation;
}

int32_t HCaptureSession::SetSmoothZoom(
    int32_t smoothZoomType, int32_t operationMode, float targetZoomRatio, float& duration)
{
    constexpr int32_t ZOOM_RATIO_MULTIPLE = 100;
    auto cameraDevice = GetCameraDevice();
    CHECK_ERROR_RETURN_RET_LOG(cameraDevice == nullptr, CAMERA_UNKNOWN_ERROR,
        "HCaptureSession::SetSmoothZoom device is null");
    float currentFps = 30.0f;
    float currentZoomRatio = 1.0f;
    QueryFpsAndZoomRatio(currentFps, currentZoomRatio);
    std::vector<float> crossZoomAndTime {};
    QueryZoomPerformance(crossZoomAndTime, operationMode);
    float waitTime = 0.0;
    int dataLenPerPoint = 3;
    float frameIntervalMs = 1000.0 / currentFps;
    targetZoomRatio = targetZoomRatio * ZOOM_RATIO_MULTIPLE;
    int indexAdded = targetZoomRatio > currentZoomRatio ? 1 : 2;
    auto zoomAlgorithm = SmoothZoom::GetZoomAlgorithm(static_cast<SmoothZoomType>(smoothZoomType));
    auto array = zoomAlgorithm->GetZoomArray(currentZoomRatio, targetZoomRatio, frameIntervalMs);
    CHECK_ERROR_RETURN_RET_LOG(array.empty(), CAMERA_UNKNOWN_ERROR, "HCaptureSession::SetSmoothZoom array is empty");
    for (int i = 0; i < static_cast<int>(crossZoomAndTime.size()); i = i + dataLenPerPoint) {
        float crossZoom = crossZoomAndTime[i];
        if ((crossZoom - currentZoomRatio) * (crossZoom - targetZoomRatio) > 0) {
            continue;
        }
        float waitMs = crossZoomAndTime[i + indexAdded];
        for (int j = 0; j < static_cast<int>(array.size()); j++) {
            if (static_cast<int>(array[j] - crossZoom) * static_cast<int>(array[0] - crossZoom) < 0) {
                waitTime = fmax(waitMs - frameIntervalMs * j, waitTime);
                break;
            }
        }
    }
    std::vector<uint32_t> zoomAndTimeArray {};
    for (int i = 0; i < static_cast<int>(array.size()); i++) {
        zoomAndTimeArray.push_back(static_cast<uint32_t>(array[i]));
        zoomAndTimeArray.push_back(static_cast<uint32_t>(i * frameIntervalMs + waitTime));
        MEDIA_DEBUG_LOG("HCaptureSession::SetSmoothZoom() zoom %{public}d, waitMs %{public}d.",
            static_cast<uint32_t>(array[i]), static_cast<uint32_t>(i * frameIntervalMs + waitTime));
    }
    duration = (static_cast<int>(array.size()) - 1) * frameIntervalMs + waitTime;
    MEDIA_DEBUG_LOG("HCaptureSession::SetSmoothZoom() duration %{public}f", duration);
    ProcessMetaZoomArray(zoomAndTimeArray, cameraDevice);
    return CAMERA_OK;
}

void HCaptureSession::ProcessMetaZoomArray(
    std::vector<uint32_t>& zoomAndTimeArray, sptr<HCameraDevice>& cameraDevice)
{
    std::shared_ptr<OHOS::Camera::CameraMetadata> metaZoomArray = std::make_shared<OHOS::Camera::CameraMetadata>(1, 1);
    uint32_t zoomCount = static_cast<uint32_t>(zoomAndTimeArray.size());
    MEDIA_INFO_LOG("HCaptureSession::ProcessMetaZoomArray() zoomArray size: %{public}zu, zoomCount: %{public}u",
        zoomAndTimeArray.size(), zoomCount);
    metaZoomArray->addEntry(OHOS_CONTROL_SMOOTH_ZOOM_RATIOS, zoomAndTimeArray.data(), zoomCount);
    cameraDevice->UpdateSettingOnce(metaZoomArray);
}

int32_t HCaptureSession::EnableMovingPhoto(bool isEnable)
{
    isSetMotionPhoto_ = isEnable;
    StartMovingPhotoStream();
    return CAMERA_OK;
}

int32_t HCaptureSession::Start()
{
    CAMERA_SYNC_TRACE;
    int32_t errorCode = CAMERA_OK;
    MEDIA_INFO_LOG("HCaptureSession::Start prepare execute");
    stateMachine_.StateGuard([&errorCode, this](CaptureSessionState currentState) {
        bool isTransferSupport = stateMachine_.CheckTransfer(CaptureSessionState::SESSION_STARTED);
        if (!isTransferSupport) {
            MEDIA_ERR_LOG("HCaptureSession::Start() Need to call after committing configuration");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }

        std::shared_ptr<OHOS::Camera::CameraMetadata> settings = nullptr;
        auto cameraDevice = GetCameraDevice();
        if (cameraDevice != nullptr) {
            settings = cameraDevice->CloneCachedSettings();
            DumpMetadata(settings);
            UpdateMuteSetting(cameraDevice->GetDeviceMuteMode(), settings);
        }
        errorCode = StartPreviewStream(settings);
        if (errorCode == CAMERA_OK) {
            isSessionStarted_ = true;
        }
        stateMachine_.Transfer(CaptureSessionState::SESSION_STARTED);
    });
    MEDIA_INFO_LOG("HCaptureSession::Start execute success");
    return errorCode;
}

void HCaptureSession::UpdateMuteSetting(bool muteMode, std::shared_ptr<OHOS::Camera::CameraMetadata> &settings)
{
    int32_t count = 1;
    uint8_t mode = muteMode? OHOS_CAMERA_MUTE_MODE_SOLID_COLOR_BLACK:OHOS_CAMERA_MUTE_MODE_OFF;
    settings->addEntry(OHOS_CONTROL_MUTE_MODE, &mode, count);
}

void HCaptureSession::StartMovingPhoto(sptr<HStreamRepeat>& curStreamRepeat)
{
    auto thisPtr = wptr<HCaptureSession>(this);
    curStreamRepeat->SetMovingPhotoStartCallback([thisPtr]() {
        auto sessionPtr = thisPtr.promote();
        if (sessionPtr != nullptr) {
            MEDIA_INFO_LOG("StartMovingPhotoStream when addDeferedSurface");
            sessionPtr->StartMovingPhotoStream();
        }
    });
}

int32_t HCaptureSession::StartPreviewStream(const std::shared_ptr<OHOS::Camera::CameraMetadata>& settings)
{
    int32_t errorCode = CAMERA_OK;
    auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
    bool hasDerferedPreview = false;
    // start preview
    for (auto& item : repeatStreams) {
        auto curStreamRepeat = CastStream<HStreamRepeat>(item);
        auto repeatType = curStreamRepeat->GetRepeatStreamType();
        if (repeatType != RepeatStreamType::PREVIEW) {
            continue;
        }
        errorCode = curStreamRepeat->Start(settings);
        hasDerferedPreview = curStreamRepeat->producer_ == nullptr;
        if (isSetMotionPhoto_ && hasDerferedPreview) {
            StartMovingPhoto(curStreamRepeat);
        }
        if (errorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("HCaptureSession::Start(), Failed to start preview, rc: %{public}d", errorCode);
            break;
        }
    }
    // start movingPhoto
    for (auto& item : repeatStreams) {
        auto curStreamRepeat = CastStream<HStreamRepeat>(item);
        auto repeatType = curStreamRepeat->GetRepeatStreamType();
        if (repeatType != RepeatStreamType::LIVEPHOTO) {
            continue;
        }
        int32_t movingPhotoErrorCode = CAMERA_OK;
        if (isSetMotionPhoto_ && !hasDerferedPreview) {
            movingPhotoErrorCode = curStreamRepeat->Start(settings);
            std::lock_guard<std::mutex> lock(movingPhotoStatusLock_);
            audioCapturerSession_ != nullptr && audioCapturerSession_->StartAudioCapture();
        }
        if (movingPhotoErrorCode != CAMERA_OK) {
            MEDIA_ERR_LOG("Failed to start movingPhoto, rc: %{public}d", movingPhotoErrorCode);
            break;
        }
    }
    return errorCode;
}

int32_t HCaptureSession::Stop()
{
    CAMERA_SYNC_TRACE;
    int32_t errorCode = CAMERA_OK;
    MEDIA_INFO_LOG("HCaptureSession::Stop prepare execute");
    stateMachine_.StateGuard([&errorCode, this](CaptureSessionState currentState) {
        bool isTransferSupport = stateMachine_.CheckTransfer(CaptureSessionState::SESSION_CONFIG_COMMITTED);
        if (!isTransferSupport) {
            MEDIA_ERR_LOG("HCaptureSession::Stop() Need to call after Start");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        auto allStreams = streamContainer_.GetAllStreams();
        for (auto& item : allStreams) {
            if (item->GetStreamType() == StreamType::REPEAT) {
                auto repeatStream = CastStream<HStreamRepeat>(item);
                if (repeatStream->GetRepeatStreamType() == RepeatStreamType::PREVIEW) {
                    errorCode = repeatStream->Stop();
                } else if (repeatStream->GetRepeatStreamType() == RepeatStreamType::LIVEPHOTO) {
                    repeatStream->Stop();
                    StopMovingPhoto();
                } else {
                    repeatStream->Stop();
                }
            } else if (item->GetStreamType() == StreamType::METADATA) {
                CastStream<HStreamMetadata>(item)->Stop();
            } else if (item->GetStreamType() == StreamType::CAPTURE) {
                CastStream<HStreamCapture>(item)->CancelCapture();
            } else if (item->GetStreamType() == StreamType::DEPTH) {
                CastStream<HStreamDepthData>(item)->Stop();
            } else {
                MEDIA_ERR_LOG("HCaptureSession::Stop(), get unknow stream, streamType: %{public}d, streamId:%{public}d",
                    item->GetStreamType(), item->GetFwkStreamId());
            }
            if (errorCode != CAMERA_OK) {
                MEDIA_ERR_LOG("HCaptureSession::Stop(), Failed to stop stream, rc: %{public}d, streamId:%{public}d",
                    errorCode, item->GetFwkStreamId());
            }
        }
        if (errorCode == CAMERA_OK) {
            isSessionStarted_ = false;
        }
        stateMachine_.Transfer(CaptureSessionState::SESSION_CONFIG_COMMITTED);
    });
    MEDIA_INFO_LOG("HCaptureSession::Stop execute success");
    return errorCode;
}

void HCaptureSession::ReleaseStreams()
{
    CAMERA_SYNC_TRACE;
    std::vector<int32_t> fwkStreamIds;
    std::vector<int32_t> hdiStreamIds;
    auto allStream = streamContainer_.GetAllStreams();
    for (auto& stream : allStream) {
        auto fwkStreamId = stream->GetFwkStreamId();
        if (fwkStreamId != STREAM_ID_UNSET) {
            fwkStreamIds.emplace_back(fwkStreamId);
        }
        auto hdiStreamId = stream->GetHdiStreamId();
        if (hdiStreamId != STREAM_ID_UNSET) {
            hdiStreamIds.emplace_back(hdiStreamId);
        }
        stream->ReleaseStream(true);
    }
    streamContainer_.Clear();
    MEDIA_INFO_LOG("HCaptureSession::ReleaseStreams() streamIds size() = %{public}zu, fwkStreamIds:%{public}s, "
        "hdiStreamIds:%{public}s,",
        fwkStreamIds.size(), Container2String(fwkStreamIds.begin(), fwkStreamIds.end()).c_str(),
        Container2String(hdiStreamIds.begin(), hdiStreamIds.end()).c_str());
    auto cameraDevice = GetCameraDevice();
    if ((cameraDevice != nullptr) && !hdiStreamIds.empty()) {
        cameraDevice->ReleaseStreams(hdiStreamIds);
    }
}

int32_t HCaptureSession::Release(CaptureSessionReleaseType type)
{
    CAMERA_SYNC_TRACE;
    int32_t errorCode = CAMERA_OK;
    MEDIA_INFO_LOG("HCaptureSession::Release prepare execute, release type is:%{public}d", type);
    stateMachine_.StateGuard([&errorCode, this, type](CaptureSessionState currentState) {
        MEDIA_INFO_LOG("HCaptureSession::Release pid(%{public}d). release type is:%{public}d", pid_, type);
        bool isTransferSupport = stateMachine_.CheckTransfer(CaptureSessionState::SESSION_RELEASED);
        if (!isTransferSupport) {
            MEDIA_ERR_LOG("HCaptureSession::Release error, this session is already released!");
            errorCode = CAMERA_INVALID_STATE;
            return;
        }
        // stop movingPhoto
        StopMovingPhoto();

        // Clear outputs
        ReleaseStreams();

        // Clear inputs
        auto cameraDevice = GetCameraDevice();
        if (cameraDevice != nullptr) {
            cameraDevice->Release();
            SetCameraDevice(nullptr);
        }

        // Clear current session
        MEDIA_DEBUG_LOG(
            "ClearCaptureSession: camera stub services(%{public}zu) pid(%{public}d).", TotalSessionSize(), pid_);
        TotalSessionErase(pid_);
        MEDIA_DEBUG_LOG("ClearCaptureSession: camera stub services(%{public}zu).", TotalSessionSize());

        sptr<ICaptureSessionCallback> emptyCallback = nullptr;
        SetCallback(emptyCallback);
        stateMachine_.Transfer(CaptureSessionState::SESSION_RELEASED);
        isSessionStarted_ = false;
        std::lock_guard<std::mutex> lock(movingPhotoStatusLock_);
        livephotoListener_ = nullptr;
        videoCache_ = nullptr;
        taskManager_ = nullptr;
        audioCapturerSession_ = nullptr;
    });
    MEDIA_INFO_LOG("HCaptureSession::Release execute success");
    return errorCode;
}

int32_t HCaptureSession::Release()
{
    MEDIA_INFO_LOG("HCaptureSession::Release()");
    CameraReportUtils::GetInstance().SetModeChangePerfStartInfo(opMode_, CameraReportUtils::GetCallerInfo());
    return Release(CaptureSessionReleaseType::RELEASE_TYPE_CLIENT);
}

int32_t HCaptureSession::OperatePermissionCheck(uint32_t interfaceCode)
{
    if (stateMachine_.GetCurrentState() == CaptureSessionState::SESSION_RELEASED) {
        MEDIA_ERR_LOG("HCaptureSession::OperatePermissionCheck session is released");
        return CAMERA_INVALID_STATE;
    }
    switch (static_cast<CaptureSessionInterfaceCode>(interfaceCode)) {
        case CAMERA_CAPTURE_SESSION_START: {
            auto callerToken = IPCSkeleton::GetCallingTokenID();
            if (callerToken_ != callerToken) {
                MEDIA_ERR_LOG("HCaptureSession::OperatePermissionCheck fail, callerToken_ is : %{public}d, now token "
                              "is %{public}d",
                    callerToken_, callerToken);
                return CAMERA_OPERATION_NOT_ALLOWED;
            }
            break;
        }
        default:
            break;
    }
    return CAMERA_OK;
}

void HCaptureSession::DestroyStubObjectForPid(pid_t pid)
{
    MEDIA_DEBUG_LOG("camera stub services(%{public}zu) pid(%{public}d).", TotalSessionSize(), pid);
    sptr<HCaptureSession> session = TotalSessionsGet(pid);
    if (session != nullptr) {
        session->Release(CaptureSessionReleaseType::RELEASE_TYPE_CLIENT_DIED);
    }
    MEDIA_DEBUG_LOG("camera stub services(%{public}zu).", TotalSessionSize());
}

int32_t HCaptureSession::SetCallback(sptr<ICaptureSessionCallback>& callback)
{
    if (callback == nullptr) {
        MEDIA_WARNING_LOG("HCaptureSession::SetCallback callback is null, we should clear the callback");
    }

    // Not implement yet.
    return CAMERA_OK;
}

std::string HCaptureSession::GetSessionState()
{
    auto currentState = stateMachine_.GetCurrentState();
    std::map<CaptureSessionState, std::string>::const_iterator iter = SESSION_STATE_STRING_MAP.find(currentState);
    if (iter != SESSION_STATE_STRING_MAP.end()) {
        return iter->second;
    }
    return std::to_string(static_cast<uint32_t>(currentState));
}

void HCaptureSession::DumpCameraSessionSummary(CameraInfoDumper& infoDumper)
{
    infoDumper.Msg("Number of Camera clients:[" + std::to_string(TotalSessionSize()) + "]");
}

void HCaptureSession::DumpSessions(CameraInfoDumper& infoDumper)
{
    auto totalSession = TotalSessionsCopy();
    uint32_t index = 0;
    for (auto it = totalSession.begin(); it != totalSession.end(); it++) {
        if (it->second != nullptr) {
            sptr<HCaptureSession> session = it->second;
            infoDumper.Title("Camera Sessions[" + std::to_string(index++) + "] Info:");
            session->DumpSessionInfo(infoDumper);
        }
    }
}

void HCaptureSession::DumpSessionInfo(CameraInfoDumper& infoDumper)
{
    infoDumper.Msg("Client pid:[" + std::to_string(pid_)+ "]    Client uid:[" + std::to_string(uid_) + "]");
    infoDumper.Msg("session state:[" + GetSessionState() + "]");
    for (auto& stream : streamContainer_.GetAllStreams()) {
        infoDumper.Push();
        stream->DumpStreamInfo(infoDumper);
        infoDumper.Pop();
    }
}

int32_t HCaptureSession::StartMovingPhotoCapture(bool isMirror, int32_t rotation)
{
    if (!isSetMotionPhoto_) {
        return CAMERA_OK;
    }
    if (isMirror != isMovingPhotoMirror_) {
        auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
        for (auto& stream : repeatStreams) {
            if (stream == nullptr) {
                continue;
            }
            auto streamRepeat = CastStream<HStreamRepeat>(stream);
            if (streamRepeat->GetRepeatStreamType() == RepeatStreamType::LIVEPHOTO) {
                MEDIA_INFO_LOG("restart movingphoto stream.");
                std::lock_guard<std::mutex> lock(movingPhotoStatusLock_);
                streamRepeat->SetMirrorForLivePhoto(isMirror, opMode_);
                streamRepeat->Stop();
                streamRepeat->Start();
                break;
            }
        }
        isMovingPhotoMirror_ = isMirror;
        // clear cache frame
        livephotoListener_->ClearCache();
    }
    return CAMERA_OK;
}

void HCaptureSession::GetOutputStatus(int32_t &status)
{
    auto repeatStreams = streamContainer_.GetStreams(StreamType::REPEAT);
    for (auto& stream : repeatStreams) {
        if (stream == nullptr) {
            continue;
        }
        auto streamRepeat = CastStream<HStreamRepeat>(stream);
        if (streamRepeat->GetRepeatStreamType() == RepeatStreamType::VIDEO) {
            if (streamRepeat->GetPreparedCaptureId() != CAPTURE_ID_UNSET) {
                const int32_t videoStartStatus = 2;
                status = videoStartStatus;
            }
        }
    }
}

void HCaptureSession::StartMovingPhotoEncode(int32_t rotation, uint64_t timestamp)
{
    if (!isSetMotionPhoto_) {
        return;
    }
    int32_t realRotation = GetSensorOritation() + rotation;
    realRotation = realRotation > ROTATION_360 ? realRotation - ROTATION_360 : realRotation;
    StartRecord(timestamp, realRotation);
}

std::string HCaptureSession::CreateDisplayName()
{
    struct tm currentTime;
    std::string formattedTime = "";
    if (GetSystemCurrentTime(&currentTime)) {
        std::stringstream ss;
        ss << prefix << std::setw(yearWidth) << std::setfill(placeholder) << currentTime.tm_year + startYear
           << std::setw(otherWidth) << std::setfill(placeholder) << (currentTime.tm_mon + 1)
           << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_mday
           << connector << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_hour
           << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_min
           << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_sec;
        formattedTime = ss.str();
    } else {
        MEDIA_ERR_LOG("Failed to get current time.");
    }
    if (lastDisplayName_ == formattedTime) {
        saveIndex++;
        formattedTime = formattedTime + connector + std::to_string(saveIndex);
        MEDIA_INFO_LOG("CreateDisplayName is %{private}s", formattedTime.c_str());
        return formattedTime;
    }
    lastDisplayName_ = formattedTime;
    saveIndex = 0;
    MEDIA_INFO_LOG("CreateDisplayName is %{private}s", formattedTime.c_str());
    return formattedTime;
}

std::string HCaptureSession::CreateBurstDisplayName(int32_t seqId)
{
    struct tm currentTime;
    std::string formattedTime = "";
    std::stringstream ss;
    if (seqId == 1) {
        if (!GetSystemCurrentTime(&currentTime)) {
            MEDIA_ERR_LOG("Failed to get current time.");
            return formattedTime;
        }
        ss << prefix << std::setw(yearWidth) << std::setfill(placeholder) << currentTime.tm_year + startYear
            << std::setw(otherWidth) << std::setfill(placeholder) << (currentTime.tm_mon + 1)
            << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_mday << connector
            << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_hour
            << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_min
            << std::setw(otherWidth) << std::setfill(placeholder) << currentTime.tm_sec
            << connector << burstTag;
        lastBurstPrefix_ = ss.str();
        MEDIA_DEBUG_LOG("burst prefix is %{private}s", lastBurstPrefix_.c_str());
        ss  << std::setw(burstWidth) << std::setfill(placeholder) << seqId
            << coverTag;
    } else {
        ss << lastBurstPrefix_ << std::setw(burstWidth) << std::setfill(placeholder) << seqId;
    }
    formattedTime = ss.str();
    MEDIA_INFO_LOG("CreateBurstDisplayName is %{private}s", formattedTime.c_str());
    return formattedTime;
}

int32_t HCaptureSession::CreateMediaLibrary(sptr<CameraPhotoProxy> &photoProxy,
    std::string &uri, int32_t &cameraShotType, std::string &burstKey, int64_t timestamp)
{
    CAMERA_SYNC_TRACE;
    sptr<IRemoteObject> object = nullptr;
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_ERROR_RETURN_RET_LOG(samgr == nullptr, CAMERA_UNKNOWN_ERROR, "Failed to get System ability manager");
    object = samgr->GetSystemAbility(CAMERA_SERVICE_ID);
    CHECK_ERROR_RETURN_RET_LOG(object == nullptr, CAMERA_UNKNOWN_ERROR, "object is null");
    auto mediaLibraryManager = Media::MediaLibraryManager::GetMediaLibraryManager();
    CHECK_ERROR_RETURN_RET_LOG(mediaLibraryManager == nullptr, CAMERA_UNKNOWN_ERROR,
        "Error to init mediaLibraryManager");
    mediaLibraryManager->InitMediaLibraryManager(object);
    const static int32_t INVALID_UID = -1;
    const static int32_t BASE_USER_RANGE = 200000;
    int uid = IPCSkeleton::GetCallingUid();
    if (uid <= INVALID_UID) {
        MEDIA_ERR_LOG("Get INVALID_UID UID %{public}d", uid);
    }
    int32_t userId = uid / BASE_USER_RANGE;
    MEDIA_DEBUG_LOG("get uid:%{public}d, userId:%{public}d, tokenId:%{public}d", uid, userId,
        IPCSkeleton::GetCallingTokenID());
    auto type = isSetMotionPhoto_ ? CameraShotType::MOVING_PHOTO : CameraShotType::IMAGE;
    cameraShotType = static_cast<int32_t>(type);
    MessageParcel data;
    photoProxy->WriteToParcel(data);
    photoProxy->CameraFreeBufferHandle();
    sptr<CameraServerPhotoProxy> cameraServerPhotoProxy = new CameraServerPhotoProxy();
    cameraServerPhotoProxy->ReadFromParcel(data);
    cameraServerPhotoProxy->SetDisplayName(CreateDisplayName());
    cameraServerPhotoProxy->SetShootingMode(opMode_);
    int32_t captureId = cameraServerPhotoProxy->GetCaptureId();
    std::string imageId = cameraServerPhotoProxy->GetPhotoId();
    bool isBursting = false;
    bool isCoverPhoto = false;
    auto captureStreams = streamContainer_.GetStreams(StreamType::CAPTURE);
    for (auto& stream : captureStreams) {
        MEDIA_INFO_LOG("for captureStreams");
        if (stream == nullptr) {
            continue;
        }
        MEDIA_INFO_LOG("CreateMediaLibrary get captureStream");
        auto streamCapture = CastStream<HStreamCapture>(stream);
        isBursting = streamCapture->IsBurstCapture(captureId);
        if (isBursting) {
            burstKey = streamCapture->GetBurstKey(captureId);
            streamCapture->SetBurstImages(captureId, imageId);
            isCoverPhoto = streamCapture->IsBurstCover(captureId);
            int32_t imageSeq = streamCapture->GetCurBurstSeq(captureId);
            cameraServerPhotoProxy->SetDisplayName(CreateBurstDisplayName(imageSeq));
            streamCapture->CheckResetBurstKey(captureId);
            MEDIA_INFO_LOG("CreateMediaLibrary isBursting burstKey:%{public}s isCoverPhoto:%{public}d",
                burstKey.c_str(), isCoverPhoto);
            type = CameraShotType::BURST;
            cameraShotType = static_cast<int32_t>(CameraShotType::BURST);
            cameraServerPhotoProxy->SetBurstInfo(burstKey, isCoverPhoto);
            break;
        }
        MEDIA_INFO_LOG("CreateMediaLibrary not Bursting");
    }

    if (cameraServerPhotoProxy->GetPhotoQuality() == Media::PhotoQuality::HIGH) {
        MEDIA_INFO_LOG("CreateMediaLibrary Media::PhotoQuality::HIGH");
    }
    if (cameraServerPhotoProxy->GetFormat() == Media::PhotoFormat::JPG) {
        MEDIA_INFO_LOG("CreateMediaLibrary Media::PhotoFormat::JPG");
    }

    auto photoAssetProxy = mediaLibraryManager->CreatePhotoAssetProxy(type, uid, userId);
    photoAssetProxy->AddPhotoProxy((sptr<PhotoProxy>&)cameraServerPhotoProxy);
    uri = photoAssetProxy->GetPhotoAssetUri();
    if (!isBursting && isSetMotionPhoto_ && taskManager_) {
        MEDIA_INFO_LOG("taskManager setVideoFd start");
        taskManager_->SetVideoFd(timestamp, photoAssetProxy);
    }
    return CAMERA_OK;
}

int32_t HCaptureSession::SetFeatureMode(int32_t featureMode)
{
    MEDIA_INFO_LOG("SetFeatureMode is called!");
    featureMode_ = featureMode;
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnCaptureStarted(int32_t captureId, const std::vector<int32_t>& streamIds)
{
    MEDIA_INFO_LOG("StreamOperatorCallback::OnCaptureStarted captureId:%{public}d, streamIds:%{public}s", captureId,
        Container2String(streamIds.begin(), streamIds.end()).c_str());
    std::lock_guard<std::mutex> lock(cbMutex_);
    for (auto& streamId : streamIds) {
        sptr<HStreamCommon> curStream = GetHdiStreamByStreamID(streamId);
        if (curStream == nullptr) {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureStarted StreamId: %{public}d not found", streamId);
            return CAMERA_INVALID_ARG;
        } else if (curStream->GetStreamType() == StreamType::REPEAT) {
            CastStream<HStreamRepeat>(curStream)->OnFrameStarted();
            CameraReportUtils::GetInstance().SetOpenCamPerfEndInfo();
            CameraReportUtils::GetInstance().SetModeChangePerfEndInfo();
            CameraReportUtils::GetInstance().SetSwitchCamPerfEndInfo();
        } else if (curStream->GetStreamType() == StreamType::CAPTURE) {
            CastStream<HStreamCapture>(curStream)->OnCaptureStarted(captureId);
        }
    }
    return CAMERA_OK;
}

void HCaptureSession::StartRecord(uint64_t timestamp, int32_t rotation)
{
    if (isSetMotionPhoto_) {
        taskManager_->SubmitTask([this, timestamp, rotation]() {
            this->StartOnceRecord(timestamp, rotation);
        });
    }
}

SessionDrainImageCallback::SessionDrainImageCallback(std::vector<sptr<FrameRecord>>& frameCacheList,
                                                     wptr<MovingPhotoListener> listener,
                                                     wptr<MovingPhotoVideoCache> cache,
                                                     uint64_t timestamp,
                                                     int32_t rotation)
    : frameCacheList_(frameCacheList), listener_(listener), videoCache_(cache), timestamp_(timestamp),
      rotation_(rotation)
{
}

SessionDrainImageCallback::~SessionDrainImageCallback()
{
    MEDIA_INFO_LOG("~SessionDrainImageCallback enter");
}

void SessionDrainImageCallback::OnDrainImage(sptr<FrameRecord> frame)
{
    MEDIA_INFO_LOG("OnDrainImage enter");
    {
        std::lock_guard<std::mutex> lock(mutex_);
        frameCacheList_.push_back(frame);
    }
    auto videoCache = videoCache_.promote();
    if (frame->IsIdle() && videoCache) {
        videoCache_->CacheFrame(frame);
    } else if (frame->IsFinishCache() && videoCache) {
        videoCache_->OnImageEncoded(frame, true);
    } else {
        MEDIA_INFO_LOG("videoCache and frame is not useful");
    }
}

void SessionDrainImageCallback::OnDrainImageFinish(bool isFinished)
{
    MEDIA_INFO_LOG("OnDrainImageFinish enter");
    auto videoCache = videoCache_.promote();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        videoCache_->GetFrameCachedResult(
            frameCacheList_,
            [videoCache](const std::vector<sptr<FrameRecord>>& frameRecords,
                        uint64_t timestamp,
                        int32_t rotation) { videoCache->DoMuxerVideo(frameRecords, timestamp, rotation); },
            timestamp_,
            rotation_);
    }
    auto listener = listener_.promote();
    if (listener && isFinished) {
        listener->RemoveDrainImageManager(this);
    }
}

void HCaptureSession::StartOnceRecord(uint64_t timestamp, int32_t rotation)
{
    MEDIA_INFO_LOG("StartOnceRecord enter");
    // frameCacheList only used by now thread
    std::vector<sptr<FrameRecord>> frameCacheList;
    sptr<SessionDrainImageCallback> imageCallback = new SessionDrainImageCallback(frameCacheList,
        livephotoListener_, videoCache_, timestamp, rotation);
    livephotoListener_->DrainOutImage(imageCallback);
    MEDIA_INFO_LOG("StartOnceRecord end");
}

int32_t StreamOperatorCallback::OnCaptureStarted_V1_2(
    int32_t captureId, const std::vector<OHOS::HDI::Camera::V1_2::CaptureStartedInfo>& infos)
{
    MEDIA_INFO_LOG("StreamOperatorCallback::OnCaptureStarted_V1_2 captureId:%{public}d", captureId);
    std::lock_guard<std::mutex> lock(cbMutex_);
    for (auto& captureInfo : infos) {
        sptr<HStreamCommon> curStream = GetHdiStreamByStreamID(captureInfo.streamId_);
        if (curStream == nullptr) {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureStarted_V1_2 StreamId: %{public}d not found."
                          " exposureTime: %{public}u",
                captureInfo.streamId_, captureInfo.exposureTime_);
            return CAMERA_INVALID_ARG;
        } else if (curStream->GetStreamType() == StreamType::CAPTURE) {
            MEDIA_DEBUG_LOG("StreamOperatorCallback::OnCaptureStarted_V1_2 StreamId: %{public}d."
                            " exposureTime: %{public}u",
                captureInfo.streamId_, captureInfo.exposureTime_);
            CastStream<HStreamCapture>(curStream)->OnCaptureStarted(captureId, captureInfo.exposureTime_);
        }
    }
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnCaptureEnded(int32_t captureId, const std::vector<CaptureEndedInfo>& infos)
{
    MEDIA_INFO_LOG("StreamOperatorCallback::OnCaptureEnded");
    std::lock_guard<std::mutex> lock(cbMutex_);
    for (auto& captureInfo : infos) {
        sptr<HStreamCommon> curStream = GetHdiStreamByStreamID(captureInfo.streamId_);
        if (curStream == nullptr) {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureEnded StreamId: %{public}d not found."
                          " Framecount: %{public}d",
                captureInfo.streamId_, captureInfo.frameCount_);
            return CAMERA_INVALID_ARG;
        } else if (curStream->GetStreamType() == StreamType::REPEAT) {
            CastStream<HStreamRepeat>(curStream)->OnFrameEnded(captureInfo.frameCount_);
        } else if (curStream->GetStreamType() == StreamType::CAPTURE) {
            CastStream<HStreamCapture>(curStream)->OnCaptureEnded(captureId, captureInfo.frameCount_);
        }
    }
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnCaptureEndedExt(int32_t captureId,
    const std::vector<OHOS::HDI::Camera::V1_3::CaptureEndedInfoExt>& infos)
{
    MEDIA_INFO_LOG("StreamOperatorCallback::OnCaptureEndedExt");
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnCaptureError(int32_t captureId, const std::vector<CaptureErrorInfo>& infos)
{
    MEDIA_INFO_LOG("StreamOperatorCallback::OnCaptureError");
    std::lock_guard<std::mutex> lock(cbMutex_);
    for (auto& errInfo : infos) {
        sptr<HStreamCommon> curStream = GetHdiStreamByStreamID(errInfo.streamId_);
        if (curStream == nullptr) {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureError StreamId: %{public}d not found."
                          " Error: %{public}d",
                errInfo.streamId_, errInfo.error_);
            return CAMERA_INVALID_ARG;
        } else if (curStream->GetStreamType() == StreamType::REPEAT) {
            CastStream<HStreamRepeat>(curStream)->OnFrameError(errInfo.error_);
        } else if (curStream->GetStreamType() == StreamType::CAPTURE) {
            auto captureStream = CastStream<HStreamCapture>(curStream);
            captureStream->rotationMap_.Erase(captureId);
            captureStream->OnCaptureError(captureId, errInfo.error_);
        }
    }
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnFrameShutter(
    int32_t captureId, const std::vector<int32_t>& streamIds, uint64_t timestamp)
{
    MEDIA_INFO_LOG("StreamOperatorCallback::OnFrameShutter ts is:%{public}" PRId64, timestamp);
    std::lock_guard<std::mutex> lock(cbMutex_);
    for (auto& streamId : streamIds) {
        sptr<HStreamCommon> curStream = GetHdiStreamByStreamID(streamId);
        if ((curStream != nullptr) && (curStream->GetStreamType() == StreamType::CAPTURE)) {
            auto captureStream = CastStream<HStreamCapture>(curStream);
            int32_t rotation = 0;
            captureStream->rotationMap_.Find(captureId, rotation);
            StartMovingPhotoEncode(rotation, timestamp);
            captureStream->OnFrameShutter(captureId, timestamp);
        } else {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnFrameShutter StreamId: %{public}d not found", streamId);
            return CAMERA_INVALID_ARG;
        }
    }
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnFrameShutterEnd(
    int32_t captureId, const std::vector<int32_t>& streamIds, uint64_t timestamp)
{
    MEDIA_INFO_LOG("StreamOperatorCallback::OnFrameShutterEnd ts is:%{public}" PRId64, timestamp);
    std::lock_guard<std::mutex> lock(cbMutex_);
    for (auto& streamId : streamIds) {
        sptr<HStreamCommon> curStream = GetHdiStreamByStreamID(streamId);
        if ((curStream != nullptr) && (curStream->GetStreamType() == StreamType::CAPTURE)) {
            auto captureStream = CastStream<HStreamCapture>(curStream);
            captureStream->rotationMap_.Erase(captureId);
            captureStream->OnFrameShutterEnd(captureId, timestamp);
        } else {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnFrameShutterEnd StreamId: %{public}d not found", streamId);
            return CAMERA_INVALID_ARG;
        }
    }
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnCaptureReady(
    int32_t captureId, const std::vector<int32_t>& streamIds, uint64_t timestamp)
{
    MEDIA_DEBUG_LOG("StreamOperatorCallback::OnCaptureReady");
    std::lock_guard<std::mutex> lock(cbMutex_);
    for (auto& streamId : streamIds) {
        sptr<HStreamCommon> curStream = GetHdiStreamByStreamID(streamId);
        if ((curStream != nullptr) && (curStream->GetStreamType() == StreamType::CAPTURE)) {
            CastStream<HStreamCapture>(curStream)->OnCaptureReady(captureId, timestamp);
        } else {
            MEDIA_ERR_LOG("StreamOperatorCallback::OnCaptureReady StreamId: %{public}d not found", streamId);
            return CAMERA_INVALID_ARG;
        }
    }
    return CAMERA_OK;
}

int32_t StreamOperatorCallback::OnResult(int32_t streamId, const std::vector<uint8_t>& result)
{
    MEDIA_DEBUG_LOG("StreamOperatorCallback::OnResult");
    sptr<HStreamCommon> curStream;
    const int32_t metaStreamId = -1;
    if (streamId == metaStreamId) {
        curStream = GetStreamByStreamID(streamId);
    } else {
        curStream = GetHdiStreamByStreamID(streamId);
    }
    if ((curStream != nullptr) && (curStream->GetStreamType() == StreamType::METADATA)) {
        CastStream<HStreamMetadata>(curStream)->OnMetaResult(streamId, result);
    } else {
        MEDIA_ERR_LOG("StreamOperatorCallback::OnResult StreamId: %{public}d is null or not Not adapted", streamId);
        return CAMERA_INVALID_ARG;
    }
    return CAMERA_OK;
}

StateMachine::StateMachine()
{
    stateTransferMap_[static_cast<uint32_t>(CaptureSessionState::SESSION_INIT)] = {
        CaptureSessionState::SESSION_CONFIG_INPROGRESS, CaptureSessionState::SESSION_RELEASED
    };

    stateTransferMap_[static_cast<uint32_t>(CaptureSessionState::SESSION_CONFIG_INPROGRESS)] = {
        CaptureSessionState::SESSION_CONFIG_COMMITTED, CaptureSessionState::SESSION_RELEASED
    };

    stateTransferMap_[static_cast<uint32_t>(CaptureSessionState::SESSION_CONFIG_COMMITTED)] = {
        CaptureSessionState::SESSION_CONFIG_INPROGRESS, CaptureSessionState::SESSION_STARTED,
        CaptureSessionState::SESSION_RELEASED
    };
    
    stateTransferMap_[static_cast<uint32_t>(CaptureSessionState::SESSION_STARTED)] = {
        CaptureSessionState::SESSION_CONFIG_INPROGRESS, CaptureSessionState::SESSION_CONFIG_COMMITTED,
        CaptureSessionState::SESSION_RELEASED
    };
    
    stateTransferMap_[static_cast<uint32_t>(CaptureSessionState::SESSION_RELEASED)] = {};
}

bool StateMachine::CheckTransfer(CaptureSessionState targetState)
{
    std::lock_guard<std::recursive_mutex> lock(sessionStateLock_);
    return any_of(stateTransferMap_[static_cast<uint32_t>(currentState_)].begin(),
        stateTransferMap_[static_cast<uint32_t>(currentState_)].end(),
        [&targetState](const auto& state) {return state == targetState; });
}

bool StateMachine::Transfer(CaptureSessionState targetState)
{
    std::lock_guard<std::recursive_mutex> lock(sessionStateLock_);
    if (CheckTransfer(targetState)) {
        currentState_ = targetState;
        return true;
    }
    return false;
}

bool StreamContainer::AddStream(sptr<HStreamCommon> stream)
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    auto& list = streams_[stream->GetStreamType()];
    auto it = std::find_if(list.begin(), list.end(), [stream](auto item) { return item == stream; });
    if (it == list.end()) {
        list.emplace_back(stream);
        return true;
    }
    return false;
}

bool StreamContainer::RemoveStream(sptr<HStreamCommon> stream)
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    auto& list = streams_[stream->GetStreamType()];
    auto it = std::find_if(list.begin(), list.end(), [stream](auto item) { return item == stream; });
    if (it == list.end()) {
        return false;
    }
    list.erase(it);
    return true;
}

sptr<HStreamCommon> StreamContainer::GetStream(int32_t streamId)
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    for (auto& pair : streams_) {
        for (auto& stream : pair.second) {
            if (stream->GetFwkStreamId() == streamId) {
                return stream;
            }
        }
    }
    return nullptr;
}

sptr<HStreamCommon> StreamContainer::GetHdiStream(int32_t streamId)
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    for (auto& pair : streams_) {
        for (auto& stream : pair.second) {
            if (stream->GetHdiStreamId() == streamId) {
                return stream;
            }
        }
    }
    return nullptr;
}

void StreamContainer::Clear()
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    streams_.clear();
}

size_t StreamContainer::Size()
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    size_t totalSize = 0;
    for (auto& pair : streams_) {
        totalSize += pair.second.size();
    }
    return totalSize;
}

std::list<sptr<HStreamCommon>> StreamContainer::GetStreams(const StreamType streamType)
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    std::list<sptr<HStreamCommon>> totalOrderedStreams;
    for (auto& stream : streams_[streamType]) {
        auto insertPos = std::find_if(totalOrderedStreams.begin(), totalOrderedStreams.end(),
            [&stream](auto& it) { return stream->GetFwkStreamId() <= it->GetFwkStreamId(); });
        totalOrderedStreams.emplace(insertPos, stream);
    }
    return totalOrderedStreams;
}

std::list<sptr<HStreamCommon>> StreamContainer::GetAllStreams()
{
    std::lock_guard<std::mutex> lock(streamsLock_);
    std::list<sptr<HStreamCommon>> totalOrderedStreams;
    for (auto& pair : streams_) {
        for (auto& stream : pair.second) {
            auto insertPos = std::find_if(totalOrderedStreams.begin(), totalOrderedStreams.end(),
                [&stream](auto& it) { return stream->GetFwkStreamId() <= it->GetFwkStreamId(); });
            totalOrderedStreams.emplace(insertPos, stream);
        }
    }
    return totalOrderedStreams;
}

MovingPhotoListener::MovingPhotoListener(sptr<MovingPhotoSurfaceWrapper> surfaceWrapper)
    : movingPhotoSurfaceWrapper_(surfaceWrapper), recorderBufferQueue_("videoBuffer", CACHE_FRAME_COUNT)
{}

MovingPhotoListener::~MovingPhotoListener()
{
    while (!recorderBufferQueue_.Empty()) {
        MEDIA_ERR_LOG("surface_ release surface buffer");
        sptr<FrameRecord> popFrame= recorderBufferQueue_.Pop();
        popFrame->ReleaseSurfaceBuffer(nullptr);
    }
    recorderBufferQueue_.SetActive(false);
    recorderBufferQueue_.Clear();
    MEDIA_ERR_LOG("HStreamRepeat::LivePhotoListener ~ end");
}

void MovingPhotoListener::RemoveDrainImageManager(sptr<SessionDrainImageCallback> callback)
{
    callbackMap_.Erase(callback);
    MEDIA_INFO_LOG("RemoveDrainImageManager drainImageManagerVec_ Start %d", callbackMap_.Size());
}

void MovingPhotoListener::ClearCache()
{
    MEDIA_INFO_LOG("ClearCache enter");
    while (!recorderBufferQueue_.Empty()) {
        MEDIA_ERR_LOG("surface_ release surface buffer");
        sptr<FrameRecord> popFrame = recorderBufferQueue_.Pop();
        popFrame->ReleaseSurfaceBuffer(movingPhotoSurfaceWrapper_);
    }
    recorderBufferQueue_.Clear();
}

void MovingPhotoListener::StopDrainOut()
{
    MEDIA_INFO_LOG("StopDrainOut drainImageManagerVec_ Start %d", callbackMap_.Size());
    callbackMap_.Iterate([](const sptr<SessionDrainImageCallback> callback, sptr<DrainImageManager> manager) {
        manager->DrainFinish(false);
    });
    callbackMap_.Clear();
}

void MovingPhotoListener::OnBufferArrival(sptr<SurfaceBuffer> buffer, int64_t timestamp, GraphicTransformType transform)
{
    if (recorderBufferQueue_.Full()) {
        MEDIA_DEBUG_LOG("surface_ release surface buffer");
        sptr<FrameRecord> popFrame = recorderBufferQueue_.Pop();
        popFrame->ReleaseSurfaceBuffer(movingPhotoSurfaceWrapper_);
        MEDIA_DEBUG_LOG("surface_ release surface buffer: %{public}s, refCount: %{public}d",
            popFrame->GetFrameId().c_str(), popFrame->GetSptrRefCount());
    }
    MEDIA_DEBUG_LOG("surface_ push buffer %{public}d x %{public}d, stride is %{public}d",
        buffer->GetSurfaceBufferWidth(), buffer->GetSurfaceBufferHeight(), buffer->GetStride());
    sptr<FrameRecord> frameRecord = new (std::nothrow) FrameRecord(buffer, timestamp, transform);
    if (frameRecord == nullptr) {
        MEDIA_ERR_LOG("MovingPhotoListener::OnBufferAvailable create FrameRecord fail!");
        return;
    }
    recorderBufferQueue_.Push(frameRecord);
    vector<sptr<SessionDrainImageCallback>> callbacks;
    callbackMap_.Iterate([frameRecord, &callbacks](const sptr<SessionDrainImageCallback> callback,
        sptr<DrainImageManager> manager) {
        callbacks.push_back(callback);
    });
    for (sptr<SessionDrainImageCallback> drainImageCallback : callbacks) {
        sptr<DrainImageManager> drainImageManager;
        if (callbackMap_.Find(drainImageCallback, drainImageManager)) {
            drainImageManager->DrainImage(frameRecord);
        }
    }
}

void MovingPhotoListener::DrainOutImage(sptr<SessionDrainImageCallback> drainImageCallback)
{
    sptr<DrainImageManager> drainImageManager =
        new DrainImageManager(drainImageCallback, recorderBufferQueue_.Size() + CACHE_FRAME_COUNT);
    {
        MEDIA_INFO_LOG("DrainOutImage enter %{public}zu", recorderBufferQueue_.Size());
        callbackMap_.Insert(drainImageCallback, drainImageManager);
    }
    // Convert recorderBufferQueue_ to a vector
    std::vector<sptr<FrameRecord>> frameList = recorderBufferQueue_.GetAllElements();
    if (!frameList.empty()) {
        frameList.back()->SetCoverFrame();
    }
    for (const auto& frame : frameList) {
        MEDIA_DEBUG_LOG("DrainOutImage enter DrainImage");
        drainImageManager->DrainImage(frame);
    }
}
} // namespace CameraStandard
} // namespace OHOS
