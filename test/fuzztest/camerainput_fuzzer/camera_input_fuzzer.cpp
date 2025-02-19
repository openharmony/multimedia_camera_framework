/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "camera_input_fuzzer.h"
#include "camera_input.h"
#include "camera_log.h"
#include "input/camera_device.h"
#include "input/camera_manager.h"
#include "message_parcel.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include "token_setproc.h"
#include "nativetoken_kit.h"
#include "accesstoken_kit.h"

namespace OHOS {
namespace CameraStandard {
namespace CameraInputFuzzer {
const int32_t LIMITSIZE = 4;
const int32_t CAM_NUM = 2;
bool g_isCameraDevicePermission = false;
static pid_t g_pid = 0;

void GetPermission()
{
    uint64_t tokenId;
    const char* perms[2];
    perms[0] = "ohos.permission.DISTRIBUTED_DATASYNC";
    perms[1] = "ohos.permission.CAMERA";
    NativeTokenInfoParams infoInstance = {
        .dcapsNum = 0,
        .permsNum = 2,
        .aclsNum = 0,
        .dcaps = NULL,
        .perms = perms,
        .acls = NULL,
        .processName = "native_camera_tdd",
        .aplStr = "system_basic",
    };
    tokenId = GetAccessTokenId(&infoInstance);
    SetSelfTokenID(tokenId);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void Test(uint8_t *rawData, size_t size)
{
    CHECK_ERROR_RETURN(rawData == nullptr || size < LIMITSIZE);
    GetPermission();
    auto manager = CameraManager::GetInstance();
    CHECK_ERROR_RETURN_LOG(!manager, "CameraInputFuzzer: Get CameraManager instance Error");
    auto cameras = manager->GetSupportedCameras();
    CHECK_ERROR_RETURN_LOG(cameras.size() < CAM_NUM, "CameraInputFuzzer: GetSupportedCameras Error");
    MessageParcel data;
    data.WriteRawData(rawData, size);
    auto camera = cameras[data.ReadUint32() % cameras.size()];
    CHECK_ERROR_RETURN_LOG(!camera, "CameraInputFuzzer: Camera is null Error");
    auto input = manager->CreateCameraInput(camera);
    CHECK_ERROR_RETURN_LOG(!input, "CameraInputFuzzer: CreateCameraInput Error");
    std::shared_ptr<CameraOcclusionDetectCallback> cameraOcclusionDetectCallback
        = std::make_shared<CameraOcclusionDetectCallbackTest>();
    input->SetOcclusionDetectCallback(cameraOcclusionDetectCallback);
    std::shared_ptr<CameraDeviceServiceCallback> cameraDeviceServiceCallback =
        std::make_shared<CameraDeviceServiceCallback>(input);
    uint64_t timestamp = 10;
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = nullptr;
    const int32_t defaultItems = 10;
    const int32_t defaultDataLength = 100;
    int32_t count = 1;
    int32_t isOcclusionDetected = 1;
    int32_t isLensDirtyDetected = 1;
    metadata = std::make_shared<OHOS::Camera::CameraMetadata>(defaultItems, defaultDataLength);
    metadata->addEntry(OHOS_STATUS_CAMERA_OCCLUSION_DETECTION, &isOcclusionDetected, count);
    cameraDeviceServiceCallback->OnResult(timestamp, metadata);
    metadata = std::make_shared<OHOS::Camera::CameraMetadata>(defaultItems, defaultDataLength);
    metadata->addEntry(OHOS_STATUS_CAMERA_LENS_DIRTY_DETECTION, &isLensDirtyDetected, count);
    cameraDeviceServiceCallback->OnResult(timestamp, metadata);
    metadata = std::make_shared<OHOS::Camera::CameraMetadata>(defaultItems, defaultDataLength);
    metadata->addEntry(OHOS_STATUS_CAMERA_OCCLUSION_DETECTION, &isOcclusionDetected, count);
    metadata->addEntry(OHOS_STATUS_CAMERA_LENS_DIRTY_DETECTION, &isLensDirtyDetected, count);
    cameraDeviceServiceCallback->OnResult(timestamp, metadata);
    TestInput(input, rawData, size);
}

void TestInput(sptr<CameraInput> input, uint8_t *rawData, size_t size)
{
    MEDIA_INFO_LOG("CameraInputFuzzer: ENTER");
    MessageParcel data;
    data.WriteRawData(rawData, size);
    input->Open();
    input->SetErrorCallback(make_shared<ErrorCallbackMock>());
    input->SetResultCallback(make_shared<ResultCallbackMock>());
    input->GetCameraId();
    input->GetCameraDevice();
    input->GetErrorCallback();
    input->GetResultCallback();
    shared_ptr<OHOS::Camera::CameraMetadata> result;
    data.RewindRead(0);
    input->ProcessCallbackUpdates(data.ReadUint64(), result);
    input->GetCameraSettings();
    data.RewindRead(0);
    input->SetCameraSettings(data.ReadString());
    data.RewindRead(0);
    input->GetMetaSetting(data.ReadUint32());
    std::vector<vendorTag_t> infos;
    input->GetCameraAllVendorTags(infos);
    input->Release();
    input->Close();
    uint64_t secureSeqId;
    data.RewindRead(0);
    input->Open(data.ReadBool(), &secureSeqId);
    input->Release();
    CameraDeviceServiceCallback callback;
    auto meta = make_shared<OHOS::Camera::CameraMetadata>(10, 100);
    callback.OnError(data.ReadInt32(), data.ReadInt32());
    callback.OnResult(data.ReadUint64(), meta);
    input->SetInputUsedAsPosition(CAMERA_POSITION_UNSPECIFIED);
    class CameraOcclusionDetectCallbackMock : public CameraOcclusionDetectCallback {
    public:
        void OnCameraOcclusionDetected(const uint8_t isCameraOcclusion,
            const uint8_t isCameraLensDirty) const override {}
    };
    input->SetOcclusionDetectCallback(make_shared<CameraOcclusionDetectCallbackMock>());
    input->GetOcclusionDetectCallback();
    input->UpdateSetting(meta);
    input->MergeMetadata(meta, meta);
    input->closeDelayed(data.ReadInt32());
    input->CameraServerDied(g_pid);
}

} // namespace StreamRepeatStubFuzzer
} // namespace CameraStandard
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::CameraStandard::CameraInputFuzzer::Test(data, size);
    return 0;
}