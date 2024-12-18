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

#include "camera_framework_unittest.h"

#include "gtest/gtest.h"
#include <cstdint>
#include <vector>

#include "access_token.h"
#include "accesstoken_kit.h"
#include "camera_log.h"
#include "camera_manager.h"
#include "camera_output_capability.h"
#include "camera_util.h"
#include "capture_output.h"
#include "capture_scene_const.h"
#include "capture_session.h"
#include "gmock/gmock.h"
#include "hap_token_info.h"
#include "input/camera_input.h"
#include "ipc_skeleton.h"
#include "metadata_utils.h"
#include "nativetoken_kit.h"
#include "night_session.h"
#include "panorama_session.h"
#include "photo_output.h"
#include "photo_session.h"
#include "preview_output.h"
#include "scan_session.h"
#include "secure_camera_session.h"
#include "slow_motion_session.h"
#include "surface.h"
#include "test_common.h"
#include "token_setproc.h"
#include "uv.h"
#include "video_session.h"
#include "os_account_manager.h"
#include "output/metadata_output.h"

using namespace testing::ext;
using ::testing::A;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;

bool g_mockFlagWithoutAbt = false;
bool g_getCameraAbilityerror = false;
bool g_openCameraDevicerror = false;
int g_num = 0;

namespace OHOS {
namespace CameraStandard {
using namespace OHOS::HDI::Camera::V1_1;
constexpr static int32_t ARRAY_IDX = 2;
const int32_t SLEEP_TIME_FOR_UNIT = 2;
int32_t g_videoFd = -1;
class MockStreamOperator : public OHOS::HDI::Camera::V1_1::IStreamOperator {
public:
    MockStreamOperator()
    {
        ON_CALL(*this, CreateStreams(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, CreateStreams_V1_1(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, ReleaseStreams(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, CommitStreams(_, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, CommitStreams_V1_1(_, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, Capture(_, _, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, CancelCapture(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported(_, _, A<const std::shared_ptr<OHOS::HDI::Camera::V1_0::StreamInfo> &>(), _))
            .WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported(_, _, A<const std::vector<OHOS::HDI::Camera::V1_0::StreamInfo> &>(), _))
            .WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported_V1_1(_, _, A<const std::shared_ptr<StreamInfo> &>(), _))
            .WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, IsStreamsSupported_V1_1(_, _, A<const std::vector<StreamInfo_V1_1> &>(), _))
            .WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, GetStreamAttributes(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, AttachBufferQueue(_, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, DetachBufferQueue(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, ChangeToOfflineStream(_, _, _)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
    }
    ~MockStreamOperator() {}
    MOCK_METHOD1(CreateStreams, int32_t(
        const std::vector<OHOS::HDI::Camera::V1_0::StreamInfo>& streamInfos));
    MOCK_METHOD1(CreateStreams_V1_1, int32_t(
        const std::vector<StreamInfo_V1_1>& streamInfos));
    MOCK_METHOD1(ReleaseStreams, int32_t(const std::vector<int32_t>& streamIds));
    MOCK_METHOD1(CancelCapture, int32_t(int32_t captureId));
    MOCK_METHOD1(GetStreamAttributes, int32_t(
        std::vector<StreamAttribute>& attributes));
    MOCK_METHOD1(DetachBufferQueue, int32_t(int32_t streamId));
    MOCK_METHOD2(CommitStreams, int32_t(OperationMode mode, const std::vector<uint8_t>& modeSetting));
    MOCK_METHOD2(CommitStreams_V1_1, int32_t(OHOS::HDI::Camera::V1_1::OperationMode_V1_1 mode,
        const std::vector<uint8_t>& modeSetting));
    MOCK_METHOD2(AttachBufferQueue, int32_t(int32_t streamId,
        const sptr<BufferProducerSequenceable>& bufferProducer));
    MOCK_METHOD3(Capture, int32_t(int32_t captureId, const CaptureInfo& info, bool isStreaming));
    MOCK_METHOD3(ChangeToOfflineStream, int32_t(const std::vector<int32_t>& streamIds,
                                            const sptr<HDI::Camera::V1_0::IStreamOperatorCallback>& callbackObj,
                                            sptr<HDI::Camera::V1_0::IOfflineStreamOperator>& offlineOperator));
    MOCK_METHOD4(IsStreamsSupported, int32_t(OperationMode mode,
        const std::shared_ptr<OHOS::Camera::CameraMetadata> &modeSetting,
        const std::shared_ptr<OHOS::HDI::Camera::V1_0::StreamInfo> &info, StreamSupportType &type));
    MOCK_METHOD4(IsStreamsSupported, int32_t(OperationMode mode, const std::vector<uint8_t>& modeSetting,
        const std::vector<OHOS::HDI::Camera::V1_0::StreamInfo>& infos, StreamSupportType& type));
    MOCK_METHOD4(IsStreamsSupported_V1_1, int32_t(OHOS::HDI::Camera::V1_1::OperationMode_V1_1 mode,
        const std::shared_ptr<OHOS::Camera::CameraMetadata> &modeSetting,
        const std::shared_ptr<StreamInfo> &info, StreamSupportType &type));
    MOCK_METHOD4(IsStreamsSupported_V1_1, int32_t(OHOS::HDI::Camera::V1_1::OperationMode_V1_1 mode,
        const std::vector<uint8_t>& modeSetting,
        const std::vector<StreamInfo_V1_1>& infos, StreamSupportType& type));
};

class MockCameraDevice : public OHOS::HDI::Camera::V1_1::ICameraDevice {
public:
    MockCameraDevice()
    {
        streamOperator = new MockStreamOperator();
        ON_CALL(*this, GetStreamOperator_V1_1).WillByDefault([this](
            const OHOS::sptr<HDI::Camera::V1_0::IStreamOperatorCallback> &callback,
            sptr<OHOS::HDI::Camera::V1_1::IStreamOperator>& pStreamOperator) {
            pStreamOperator = streamOperator;
            return HDI::Camera::V1_0::NO_ERROR;
        });
        ON_CALL(*this, GetStreamOperator).WillByDefault([this](
            const OHOS::sptr<HDI::Camera::V1_0::IStreamOperatorCallback> &callback,
            sptr<OHOS::HDI::Camera::V1_0::IStreamOperator>& pStreamOperator) {
            pStreamOperator = streamOperator;
            return HDI::Camera::V1_0::NO_ERROR;
        });
        ON_CALL(*this, UpdateSettings(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, SetResultMode(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, GetEnabledResults(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, EnableResult(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, DisableResult(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, Open()).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, Close()).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
        ON_CALL(*this, GetDefaultSettings(_)).WillByDefault(Return(HDI::Camera::V1_0::NO_ERROR));
    }
    ~MockCameraDevice() {}
    MOCK_METHOD0(Open, int32_t());
    MOCK_METHOD0(Close, int32_t());
    MOCK_METHOD1(GetDefaultSettings, int32_t(std::vector<uint8_t> &settings));
    MOCK_METHOD1(UpdateSettings, int32_t(const std::vector<uint8_t>& settings));
    MOCK_METHOD1(SetResultMode, int32_t(ResultCallbackMode mode));
    MOCK_METHOD1(GetEnabledResults, int32_t(std::vector<int32_t>& results));
    MOCK_METHOD1(EnableResult, int32_t(const std::vector<int32_t>& results));
    MOCK_METHOD1(DisableResult, int32_t(const std::vector<int32_t>& results));
    MOCK_METHOD2(GetStreamOperator, int32_t(const sptr<HDI::Camera::V1_0::IStreamOperatorCallback>& callbackObj,
                                        sptr<OHOS::HDI::Camera::V1_0::IStreamOperator>& streamOperator));
    MOCK_METHOD2(GetStreamOperator_V1_1, int32_t(const sptr<HDI::Camera::V1_0::IStreamOperatorCallback>& callbackObj,
                                             sptr<OHOS::HDI::Camera::V1_1::IStreamOperator>& streamOperator));
    sptr<MockStreamOperator> streamOperator;
};

class MockHCameraHostManager : public HCameraHostManager {
public:
    struct StreamFormatConfig {
        camera_format_t cameraFormat;
        int32_t streamWidth;
        int32_t streamHeight;
        int32_t fixedFrameRate;
        int32_t minFrameRate;
        int32_t maxFrameRate;
        std::vector<int32_t> streamAbilities = {};
        void IntoVector(std::vector<int32_t>& vector) const
        {
            static const int32_t ABILITY_FINISH = -1;
            vector.emplace_back(cameraFormat);
            vector.emplace_back(streamWidth);
            vector.emplace_back(streamHeight);
            vector.emplace_back(fixedFrameRate);
            vector.emplace_back(minFrameRate);
            vector.emplace_back(maxFrameRate);
            vector.insert(vector.end(), streamAbilities.begin(), streamAbilities.end());
            vector.emplace_back(ABILITY_FINISH);
        }
    };

    struct StreamConfig {
        int32_t streamType;
        std::vector<StreamFormatConfig> streamFormatConfigs;

        void IntoVector(std::vector<int32_t>& vector) const
        {
            static const int32_t STREAM_FINISH = -1;
            vector.emplace_back(streamType);
            for (auto& formatConfig : streamFormatConfigs) {
                formatConfig.IntoVector(vector);
            }
            vector.emplace_back(STREAM_FINISH);
        }
    };

    struct ModeConfig {
        int32_t modeName;
        std::vector<StreamConfig> streams = {};
        void IntoVector(std::vector<int32_t>& vector) const
        {
            static const int32_t MODE_FINISH = -1;
            vector.emplace_back(modeName);
            for (auto& stream : streams) {
                stream.IntoVector(vector);
            }
            vector.emplace_back(MODE_FINISH);
        }
    };

    struct ModeConfigs {
        std::vector<ModeConfig> configs = {};
        explicit ModeConfigs(std::vector<ModeConfig> modeConfigs) : configs(std::move(modeConfigs)) {}

        std::vector<uint8_t> GetModes() const
        {
            std::vector<uint8_t> modes = {};
            for (auto& config : configs) {
                modes.emplace_back(config.modeName);
            }
            return modes;
        }

        std::vector<int32_t> GetDatas() const
        {
            std::vector<int32_t> datas = {};
            for (auto& config : configs) {
                config.IntoVector(datas);
            }
            return datas;
        }
    };

    const ModeConfig DEFAULT_MODE_CONFIG = { CameraFrameworkUnitTest::DEFAULT_MODE, {
        { CameraFrameworkUnitTest::PREVIEW_STREAM, { { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 480, 0, 0, 0 } } },
        { CameraFrameworkUnitTest::VIDEO_STREAM, { { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 360, 30, 0, 0 } } },
        { CameraFrameworkUnitTest::PHOTO_STREAM, { { OHOS_CAMERA_FORMAT_JPEG, 1280, 960, 0, 0, 0 } } } } };

    const ModeConfig PHOTO_MODE_CONFIG = { CameraFrameworkUnitTest::PHOTO_MODE,
        {
            { CameraFrameworkUnitTest::PREVIEW_STREAM,
                {
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 480, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 720, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1080, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1440, 1440, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 960, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1440, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1920, 1440, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1280, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1920, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 2560, 1440, 0, 0, 0 },
                } },
            { CameraFrameworkUnitTest::PHOTO_STREAM,
                {
                    { OHOS_CAMERA_FORMAT_JPEG, 720, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 1080, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 2160, 2160, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 3072, 3072, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_JPEG, 960, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 1440, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 2880, 2160, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 4096, 3072, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_JPEG, 1280, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 1920, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 3840, 2160, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 4096, 2304, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_JPEG, 1280, 960, 0, 0, 0 },
                } },
        } };

    const ModeConfig VIDEO_MODE_CONFIG = { CameraFrameworkUnitTest::VIDEO_MODE,
        {
            { CameraFrameworkUnitTest::PREVIEW_STREAM,
                {
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 720, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1080, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_P010, 1080, 1080, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 960, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1440, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_P010, 1440, 1080, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1280, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1920, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_YCRCB_P010, 1920, 1080, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 480, 0, 0, 0 },
                } },
            { CameraFrameworkUnitTest::VIDEO_STREAM,
                {
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 720, 720, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1080, 1080, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 2160, 2160, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_P010, 2160, 2160, 30, 24, 30 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 960, 720, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1440, 1080, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 2880, 2160, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_P010, 2880, 2160, 30, 24, 30 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1280, 720, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1920, 1080, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 3840, 2160, 30, 24, 30 },
                    { OHOS_CAMERA_FORMAT_YCRCB_P010, 3840, 2160, 30, 24, 30 },

                    { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 360, 30, 24, 30 },
                } },
            { CameraFrameworkUnitTest::PHOTO_STREAM,
                {
                    { OHOS_CAMERA_FORMAT_JPEG, 2304, 2304, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 3072, 2304, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 4096, 2304, 0, 0, 0 },

                    { OHOS_CAMERA_FORMAT_JPEG, 1280, 960, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 1280, 720, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 1920, 1080, 0, 0, 0 },
                    { OHOS_CAMERA_FORMAT_JPEG, 3840, 2160, 0, 0, 0 },
                } },
        } };

    const ModeConfig PORTRAIT_MODE_CONFIG = { CameraFrameworkUnitTest::PORTRAIT_MODE, {
        { CameraFrameworkUnitTest::PREVIEW_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 480, 0, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } },
        { CameraFrameworkUnitTest::VIDEO_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 360, 30, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } },
        { CameraFrameworkUnitTest::PHOTO_STREAM, {
            { OHOS_CAMERA_FORMAT_JPEG, 1280, 960, 0, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } } } };

    const ModeConfig NIGHT_MODE_CONFIG = { CameraFrameworkUnitTest::NIGHT_MODE, {
        { CameraFrameworkUnitTest::PREVIEW_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 480, 0, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } },
        { CameraFrameworkUnitTest::VIDEO_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 360, 30, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } },
        { CameraFrameworkUnitTest::PHOTO_STREAM, {
            { OHOS_CAMERA_FORMAT_JPEG, 1280, 960, 0, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } } } };

    const ModeConfig SCAN_MODE_CONFIG = { CameraFrameworkUnitTest::SCAN_MODE, {
        { CameraFrameworkUnitTest::PREVIEW_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 480, 0, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } },
        { CameraFrameworkUnitTest::VIDEO_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 640, 360, 30, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR } } } },
        { CameraFrameworkUnitTest::PHOTO_STREAM, {
            { OHOS_CAMERA_FORMAT_JPEG, 1280, 960, 0, 0, 0, {
                CameraFrameworkUnitTest::ABILITY_ID_ONE,
                CameraFrameworkUnitTest::ABILITY_ID_TWO,
                CameraFrameworkUnitTest::ABILITY_ID_THREE,
                CameraFrameworkUnitTest::ABILITY_ID_FOUR, } } } } } };

    const ModeConfig SLOW_MODE_CONFIG = { CameraFrameworkUnitTest::SLOW_MODE, {
        { CameraFrameworkUnitTest::PREVIEW_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1920, 1080, 0, 0, 0} } },
        { CameraFrameworkUnitTest::VIDEO_STREAM, {
            { OHOS_CAMERA_FORMAT_YCRCB_420_SP, 1920, 1080, 0, 0, 0} } }}};

    const ModeConfigs ABILITY_MODE_CONFIGS = ModeConfigs(std::vector<ModeConfig> { DEFAULT_MODE_CONFIG,
        PHOTO_MODE_CONFIG, VIDEO_MODE_CONFIG, PORTRAIT_MODE_CONFIG, NIGHT_MODE_CONFIG, SCAN_MODE_CONFIG,
        SLOW_MODE_CONFIG });

    uint8_t filterAbility[2] = {0, 1};
    int32_t filterControl[2] = {0, 1};
    uint8_t beautyAbility[5] = {OHOS_CAMERA_BEAUTY_TYPE_OFF, OHOS_CAMERA_BEAUTY_TYPE_AUTO,
                                OHOS_CAMERA_BEAUTY_TYPE_SKIN_SMOOTH, OHOS_CAMERA_BEAUTY_TYPE_FACE_SLENDER,
                                OHOS_CAMERA_BEAUTY_TYPE_SKIN_TONE};
    uint8_t faceSlenderBeautyRange[2] = {2, 3};
    int32_t faceSlenderBeautyControl[2] = {2, 3};
    int32_t effectAbility[2] = {0, 1};
    int32_t effectControl[2] = {0, 1};
    int32_t photoFormats[2] = {OHOS_CAMERA_FORMAT_YCRCB_420_SP, OHOS_CAMERA_FORMAT_JPEG};
    class MockStatusCallback : public StatusCallback {
    public:
        void OnCameraStatus(const std::string& cameraId, CameraStatus status, CallbackInvoker invoker) override
        {
            MEDIA_DEBUG_LOG("MockStatusCallback::OnCameraStatus");
            return;
        }
        void OnFlashlightStatus(const std::string& cameraId, FlashStatus status) override
        {
            MEDIA_DEBUG_LOG("MockStatusCallback::OnFlashlightStatus");
            return;
        }
        void OnTorchStatus(TorchStatus status) override
        {
            MEDIA_DEBUG_LOG("MockStatusCallback::OnTorchStatus");
            return;
        }
    };

    explicit MockHCameraHostManager(std::shared_ptr<StatusCallback> statusCallback) : HCameraHostManager(statusCallback)
    {
        cameraDevice = new MockCameraDevice();
        ON_CALL(*this, GetCameras).WillByDefault([](std::vector<std::string> &cameraIds) {
            cameraIds.emplace_back("cam0");
            return CAMERA_OK;
        });
        ON_CALL(*this, GetVersionByCamera).WillByDefault([](const std::string& cameraId) {
            const uint32_t offset = 8;
            uint32_t major = 1;
            uint32_t minor = 1;
            return static_cast<int32_t>((major << offset) | minor);
        });
        ON_CALL(*this, GetCameraAbility).WillByDefault([this](std::string &cameraId,
                                                            std::shared_ptr<OHOS::Camera::CameraMetadata> &ability) {
            int32_t itemCount = 10;
            int32_t dataSize = 100;
            auto streamsInfo = ABILITY_MODE_CONFIGS.GetDatas();
            ability = std::make_shared<OHOS::Camera::CameraMetadata>(itemCount, dataSize);
            ability->addEntry(
                OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, streamsInfo.data(), streamsInfo.size());

            ability->addEntry(
                OHOS_ABILITY_STREAM_AVAILABLE_EXTEND_CONFIGURATIONS, streamsInfo.data(), streamsInfo.size());

            if (g_mockFlagWithoutAbt) {
                return CAMERA_OK;
            }
            if (g_getCameraAbilityerror) {
                g_getCameraAbilityerror = false;
                return CAMERA_ALLOC_ERROR;
            }

            int32_t compensationRange[2] = {0, 0};
            ability->addEntry(OHOS_ABILITY_AE_COMPENSATION_RANGE, compensationRange,
                              sizeof(compensationRange) / sizeof(compensationRange[0]));
            float focalLength = 1.5;
            ability->addEntry(OHOS_ABILITY_FOCAL_LENGTH, &focalLength, sizeof(float));

            int32_t sensorOrientation = 0;
            ability->addEntry(OHOS_SENSOR_ORIENTATION, &sensorOrientation, sizeof(int32_t));

            int32_t cameraPosition = 0;
            ability->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, sizeof(int32_t));

            const camera_rational_t aeCompensationStep[] = {{0, 1}};
            ability->addEntry(OHOS_ABILITY_AE_COMPENSATION_STEP, &aeCompensationStep,
                              sizeof(aeCompensationStep) / sizeof(aeCompensationStep[0]));

            int32_t zoomCap[2] = {0, 10};
            ability->addEntry(OHOS_ABILITY_ZOOM_CAP, &zoomCap,
                              sizeof(zoomCap) / sizeof(zoomCap[0]));

            int32_t sceneZoomCap[2] = {1, 10};
            ability->addEntry(OHOS_ABILITY_SCENE_ZOOM_CAP, &sceneZoomCap,
                              sizeof(zoomCap) / sizeof(zoomCap[0]));

            float zoomRatio = 1.0;
            ability->addEntry(OHOS_CONTROL_ZOOM_RATIO, &zoomRatio, sizeof(float));

            uint8_t flaMode = OHOS_CAMERA_FLASH_MODE_ALWAYS_OPEN;
            ability->addEntry(OHOS_CONTROL_FLASH_MODE, &flaMode, sizeof(uint8_t));

            uint8_t flashstate = OHOS_CAMERA_FLASH_STATE_READY;
            ability->addEntry(OHOS_CONTROL_FLASH_STATE, &flashstate, sizeof(uint8_t));

            uint8_t fMode = FOCUS_MODE_AUTO;
            ability->addEntry(OHOS_CONTROL_FOCUS_MODE, &fMode, sizeof(uint8_t));

            uint8_t focusState = OHOS_CAMERA_FOCUS_STATE_SCAN;
            ability->addEntry(OHOS_CONTROL_FOCUS_STATE, &focusState, sizeof(uint8_t));

            uint8_t exMode = 0;
            ability->addEntry(OHOS_CONTROL_EXPOSURE_MODE, &exMode, sizeof(uint8_t));

            uint8_t muteMode = OHOS_CAMERA_MUTE_MODE_OFF;
            ability->addEntry(OHOS_CONTROL_MUTE_MODE, &muteMode, sizeof(uint8_t));

            int32_t activeArraySize[] = {
                0, 0, CameraFrameworkUnitTest::PREVIEW_DEFAULT_WIDTH, CameraFrameworkUnitTest::PREVIEW_DEFAULT_HEIGHT
            };
            ability->addEntry(OHOS_SENSOR_INFO_ACTIVE_ARRAY_SIZE, &activeArraySize, sizeof(int32_t));

            uint8_t videoStabilizationMode = OHOS_CAMERA_VIDEO_STABILIZATION_OFF;
            ability->addEntry(OHOS_CONTROL_VIDEO_STABILIZATION_MODE, &videoStabilizationMode, sizeof(uint8_t));

            uint8_t faceDetectMode = OHOS_CAMERA_FACE_DETECT_MODE_SIMPLE;
            ability->addEntry(OHOS_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, sizeof(uint8_t));

            uint8_t type = 5;
            ability->addEntry(OHOS_ABILITY_CAMERA_TYPE, &type, sizeof(uint8_t));
            uint8_t value_u8 = 0;
            ability->addEntry(OHOS_ABILITY_CAMERA_CONNECTION_TYPE, &value_u8, sizeof(uint8_t));
            ability->addEntry(OHOS_ABILITY_FLASH_MODES, &value_u8, sizeof(uint8_t));
            ability->addEntry(OHOS_ABILITY_FOCUS_MODES, &value_u8, sizeof(uint8_t));
            ability->addEntry(OHOS_ABILITY_EXPOSURE_MODES, &value_u8, sizeof(uint8_t));
            ability->addEntry(OHOS_ABILITY_VIDEO_STABILIZATION_MODES, &value_u8, sizeof(uint8_t));
            ability->addEntry(OHOS_ABILITY_PRELAUNCH_AVAILABLE, &value_u8, sizeof(uint8_t));
            ability->addEntry(OHOS_ABILITY_STREAM_QUICK_THUMBNAIL_AVAILABLE, &value_u8, sizeof(uint8_t));
            ability->addEntry(OHOS_ABILITY_MUTE_MODES, &value_u8, sizeof(uint8_t));

            uint32_t value_u32[2] = {0, 10};
            ability->addEntry(OHOS_ABILITY_FPS_RANGES, &value_u32, sizeof(value_u32) / sizeof(value_u32[0]));

            float value_f[1] = {1.0};
            ability->addEntry(OHOS_ABILITY_ZOOM_RATIO_RANGE, &value_f, sizeof(value_f) / sizeof(value_f[0]));

            int32_t aeRegions = 0;
            ability->addEntry(OHOS_CONTROL_AE_REGIONS, &aeRegions, sizeof(int32_t));

            int32_t aeExposureCompensation = 0;
            ability->addEntry(OHOS_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExposureCompensation, sizeof(int32_t));

            uint8_t exposureState = 0;
            ability->addEntry(OHOS_CONTROL_EXPOSURE_STATE, &exposureState, sizeof(uint8_t));

            uint8_t mirror = OHOS_CAMERA_MIRROR_OFF;
            ability->addEntry(OHOS_CONTROL_CAPTURE_MIRROR, &mirror, sizeof(uint8_t));

            int32_t orientation = OHOS_CAMERA_JPEG_ROTATION_0;
            ability->addEntry(OHOS_JPEG_ORIENTATION, &orientation, sizeof(int32_t));

            std::vector<uint8_t> modes = ABILITY_MODE_CONFIGS.GetModes();
            ability->addEntry(OHOS_ABILITY_CAMERA_MODES, modes.data(), modes.size());

            ability->addEntry(OHOS_ABILITY_SCENE_FILTER_TYPES, &filterAbility,
                              sizeof(filterAbility) / sizeof(filterAbility[0]));

            ability->addEntry(OHOS_CONTROL_FILTER_TYPE, &filterControl,
                              sizeof(filterControl) / sizeof(filterControl[0]));

            ability->addEntry(OHOS_ABILITY_SCENE_BEAUTY_TYPES, beautyAbility,
                              sizeof(beautyAbility) / sizeof(beautyAbility[0]));

            ability->addEntry(OHOS_ABILITY_BEAUTY_FACE_SLENDER_VALUES, &faceSlenderBeautyRange,
                              sizeof(faceSlenderBeautyRange) / sizeof(faceSlenderBeautyRange[0]));

            ability->addEntry(OHOS_CONTROL_BEAUTY_FACE_SLENDER_VALUE, &faceSlenderBeautyControl,
                              sizeof(faceSlenderBeautyControl) / sizeof(faceSlenderBeautyControl[0]));

            ability->addEntry(OHOS_ABILITY_SCENE_PORTRAIT_EFFECT_TYPES, &effectAbility,
                              sizeof(effectAbility) / sizeof(effectAbility[0]));

            ability->addEntry(OHOS_CONTROL_PORTRAIT_EFFECT_TYPE, &effectControl,
                              sizeof(effectControl) / sizeof(effectControl[0]));
            ability->addEntry(OHOS_STREAM_AVAILABLE_FORMATS, &photoFormats,
                              sizeof(photoFormats) / sizeof(photoFormats[0]));
                        return CAMERA_OK;
        });
        ON_CALL(*this, OpenCameraDevice).WillByDefault([this](std::string &cameraId,
                                                            const sptr<ICameraDeviceCallback> &callback,
                                                            sptr<OHOS::HDI::Camera::V1_0::ICameraDevice> &pDevice,
                                                            bool isEnableSecCam) {
            pDevice = cameraDevice;
            return CAMERA_OK;
        });
        ON_CALL(*this, SetFlashlight(_, _)).WillByDefault(Return(CAMERA_OK));
        ON_CALL(*this, SetCallback(_)).WillByDefault(Return(CAMERA_OK));
    }
    ~MockHCameraHostManager() {}
    MOCK_METHOD1(GetVersionByCamera, int32_t(const std::string& cameraId));
    MOCK_METHOD1(GetCameras, int32_t(std::vector<std::string> &cameraIds));
    MOCK_METHOD1(SetCallback, int32_t(sptr<ICameraHostCallback> &callback));
    MOCK_METHOD2(GetCameraAbility, int32_t(std::string &cameraId,
        std::shared_ptr<OHOS::Camera::CameraMetadata> &ability));
    MOCK_METHOD2(SetFlashlight, int32_t(const std::string &cameraId, bool isEnable));
    MOCK_METHOD4(OpenCameraDevice, int32_t(std::string &cameraId,
        const sptr<ICameraDeviceCallback> &callback, sptr<OHOS::HDI::Camera::V1_0::ICameraDevice> &pDevice,
        bool isEnableSecCam));
    sptr<MockCameraDevice> cameraDevice;
};

class MockCameraManager : public CameraManager {
public:
    MOCK_METHOD0(GetIsFoldable, bool());
    MOCK_METHOD0(GetFoldStatus, FoldStatus());
    ~MockCameraManager() {}
};

class FakeHCameraService : public HCameraService {
public:
    explicit FakeHCameraService(sptr<HCameraHostManager> hostManager) : HCameraService(hostManager)
    {
        SetServiceStatus(CameraServiceStatus::SERVICE_READY);
    }
    ~FakeHCameraService() {}
};

class FakeCameraManager : public CameraManager {
public:
    explicit FakeCameraManager(sptr<HCameraService> service) : CameraManager(service) {}
    ~FakeCameraManager() {}
};

class CallbackListener : public FocusCallback, public ExposureCallback {
public:
    void OnFocusState(FocusState state) override
    {
        MEDIA_DEBUG_LOG("CallbackListener::OnFocusState ");
        return;
    }

    void OnExposureState(ExposureState state) override
    {
        MEDIA_DEBUG_LOG("CallbackListener::OnExposureState ");
        return;
    }
};

class AppSessionCallback : public SessionCallback {
public:
    void OnError(int32_t errorCode)
    {
        MEDIA_DEBUG_LOG("AppMetadataCallback::OnError %{public}d", errorCode);
        return;
    }
};

class AppMacroStatusCallback : public MacroStatusCallback {
public:
    void OnMacroStatusChanged(MacroStatus status)
    {
        MEDIA_DEBUG_LOG("AppMacroStatusCallback");
    }
};

class AppMetadataCallback : public MetadataObjectCallback, public MetadataStateCallback {
public:
    void OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const
    {
        MEDIA_DEBUG_LOG("AppMetadataCallback::OnMetadataObjectsAvailable received");
    }
    void OnError(int32_t errorCode) const
    {
        MEDIA_DEBUG_LOG("AppMetadataCallback::OnError %{public}d", errorCode);
    }
};

class TestSlowMotionStateCallback : public SlowMotionStateCallback {
public:
    void OnSlowMotionState(const SlowMotionState state)
    {
        MEDIA_INFO_LOG("TestSlowMotionStateCallback OnSlowMotionState.");
    }
};

sptr<CaptureOutput> CameraFrameworkUnitTest::CreatePhotoOutput(int32_t width, int32_t height)
{
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    if (surface == nullptr) {
        return nullptr;
    }
    CameraFormat photoFormat = CAMERA_FORMAT_JPEG;
    Size photoSize;
    photoSize.width = width;
    photoSize.height = height;
    Profile photoProfile = Profile(photoFormat, photoSize);
    sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
    return cameraManager->CreatePhotoOutput(photoProfile, surfaceProducer);
}

sptr<CaptureOutput> CameraFrameworkUnitTest::CreatePreviewOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    if (surface == nullptr) {
        return nullptr;
    }
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = width;
    previewSize.height = height;
    Profile previewProfile = Profile(previewFormat, previewSize);
    return cameraManager->CreatePreviewOutput(previewProfile, surface);
}

sptr<CaptureOutput> CameraFrameworkUnitTest::CreateVideoOutput(int32_t width, int32_t height)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    if (surface == nullptr) {
        return nullptr;
    }
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = width;
    videoSize.height = height;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    return cameraManager->CreateVideoOutput(videoProfile, surface);
}

void CameraFrameworkUnitTest::SessionCommit(sptr<CaptureSession> session)
{
    int32_t ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);
    int32_t captureId = 3;
    EXPECT_CALL(*mockStreamOperator, Capture(captureId, _, true));
    ret = session->Start();
    EXPECT_EQ(ret, 0);
}

void CameraFrameworkUnitTest::SessionControlParams(sptr<CaptureSession> session)
{
    session->LockForControl();

    std::vector<float> zoomRatioRange = session->GetZoomRatioRange();
    if (!zoomRatioRange.empty()) {
        session->SetZoomRatio(zoomRatioRange[0]);
    }

    std::vector<float> exposurebiasRange = session->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        session->SetExposureBias(exposurebiasRange[0]);
    }

    FlashMode flash = FLASH_MODE_ALWAYS_OPEN;
    bool flashSupported = session->IsFlashModeSupported(flash);
    if (flashSupported) {
        session->SetFlashMode(flash);
    }

    FocusMode focus = FOCUS_MODE_AUTO;
    bool focusSupported = session->IsFocusModeSupported(focus);
    if (focusSupported) {
        session->SetFocusMode(focus);
    }

    ExposureMode exposure = EXPOSURE_MODE_AUTO;
    bool exposureSupported = session->IsExposureModeSupported(exposure);
    if (exposureSupported) {
        session->SetExposureMode(exposure);
    }

    session->UnlockForControl();

    if (!exposurebiasRange.empty()) {
        EXPECT_EQ(session->GetExposureValue(), exposurebiasRange[0]);
    }

    if (flashSupported) {
        EXPECT_EQ(session->GetFlashMode(), flash);
    }

    if (focusSupported) {
        EXPECT_EQ(session->GetFocusMode(), focus);
    }

    if (exposureSupported) {
        EXPECT_EQ(session->GetExposureMode(), exposure);
    }
}

void CameraFrameworkUnitTest::PortraitSessionControlParams(sptr<PortraitSession> portraitSession)
{
    portraitSession->LockForControl();

    std::vector<PortraitEffect> effects= portraitSession->GetSupportedPortraitEffects();
    ASSERT_TRUE(effects.size() != 0);

    if (!effects.empty()) {
        portraitSession->SetPortraitEffect(effects[0]);
    }

    std::vector<FilterType> filterLists= portraitSession->GetSupportedFilters();
    ASSERT_TRUE(filterLists.size() != 0);

    if (!filterLists.empty()) {
        portraitSession->SetFilter(filterLists[0]);
    }

    std::vector<BeautyType> beautyLists= portraitSession->GetSupportedBeautyTypes();
    ASSERT_TRUE(beautyLists.size() != 0);

    std::vector<int32_t> rangeLists= portraitSession->GetSupportedBeautyRange(beautyLists[ARRAY_IDX]);
    ASSERT_TRUE(rangeLists.size() != 0);

    if (!beautyLists.empty()) {
        portraitSession->SetBeauty(beautyLists[ARRAY_IDX], rangeLists[0]);
    }

    portraitSession->UnlockForControl();
    ASSERT_EQ(portraitSession->GetPortraitEffect(), effects[0]);
    ASSERT_EQ(portraitSession->GetFilter(), filterLists[0]);
    ASSERT_EQ(portraitSession->GetBeauty(beautyLists[ARRAY_IDX]), rangeLists[0]);
}

void CameraFrameworkUnitTest::PortraitSessionEffectParams(sptr<PortraitSession> portraitSession)
{
    std::vector<PortraitEffect> effects= portraitSession->GetSupportedPortraitEffects();
    ASSERT_TRUE(effects.size() != 0);

    if (!effects.empty()) {
        portraitSession->LockForControl();
        portraitSession->SetPortraitEffect(effects[0]);
        portraitSession->UnlockForControl();
    }
    ASSERT_EQ(portraitSession->GetPortraitEffect(), effects[0]);
}

void CameraFrameworkUnitTest::PortraitSessionFilterParams(sptr<PortraitSession> portraitSession)
{
    std::vector<FilterType> filterLists= portraitSession->GetSupportedFilters();
    ASSERT_TRUE(filterLists.size() != 0);

    if (!filterLists.empty()) {
        portraitSession->LockForControl();
        portraitSession->SetFilter(filterLists[0]);
        portraitSession->UnlockForControl();
    }
    ASSERT_EQ(portraitSession->GetFilter(), filterLists[0]);
}

void CameraFrameworkUnitTest::PortraitSessionBeautyParams(sptr<PortraitSession> portraitSession)
{
    std::vector<BeautyType> beautyLists= portraitSession->GetSupportedBeautyTypes();
    ASSERT_TRUE(beautyLists.size() != 0);

    std::vector<int32_t> rangeLists= portraitSession->GetSupportedBeautyRange(beautyLists[ARRAY_IDX]);
    ASSERT_TRUE(rangeLists.size() != 0);

    if (!beautyLists.empty()) {
        portraitSession->LockForControl();
        portraitSession->SetBeauty(beautyLists[ARRAY_IDX], rangeLists[0]);
        portraitSession->UnlockForControl();
    }
    ASSERT_EQ(portraitSession->GetBeauty(beautyLists[ARRAY_IDX]), rangeLists[0]);
}

void CameraFrameworkUnitTest::SetUpTestCase(void) {}

void CameraFrameworkUnitTest::TearDownTestCase(void)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    camSession->Release();
}

void CameraFrameworkUnitTest::SetUp()
{
    // set native token
    g_num++;
    MEDIA_DEBUG_LOG("CameraFrameworkUnitTest::SetUp num:%{public}d", g_num);
    // MEDIA_DEBUG_LOG("SetUp testName:%{public}s",
    //     ::testing::UnitTest::GetInstance()->current_test_info()->name());
    NativeAuthorization();
    g_mockFlagWithoutAbt = false;
    mockCameraHostManager = new MockHCameraHostManager(nullptr);
    mockCameraDevice = mockCameraHostManager->cameraDevice;
    mockStreamOperator = mockCameraDevice->streamOperator;
    cameraManager = new FakeCameraManager(new FakeHCameraService(mockCameraHostManager));
    mockCameraManager = new MockCameraManager();
}

void CameraFrameworkUnitTest::NativeAuthorization()
{
    const char *perms[2];
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
    g_tokenId_ = GetAccessTokenId(&infoInstance);
    g_uid_ = IPCSkeleton::GetCallingUid();
    AccountSA::OsAccountManager::GetOsAccountLocalIdFromUid(g_uid_, g_userId_);
    MEDIA_DEBUG_LOG("CameraFrameworkUnitTest::NativeAuthorization g_uid:%{public}d", g_uid_);
    SetSelfTokenID(g_tokenId_);
    OHOS::Security::AccessToken::AccessTokenKit::ReloadNativeTokenInfo();
}

void CameraFrameworkUnitTest::TearDown()
{
    Mock::AllowLeak(mockCameraHostManager);
    Mock::AllowLeak(mockCameraDevice);
    Mock::AllowLeak(mockStreamOperator);
    Mock::AllowLeak(mockCameraManager);
    MEDIA_DEBUG_LOG("CameraFrameworkUnitTest::TearDown num:%{public}d", g_num);
}

MATCHER_P(matchCaptureSetting, captureSetting, "Match Capture Setting")
{
    std::vector<uint8_t> result;
    OHOS::Camera::MetadataUtils::ConvertMetadataToVec(captureSetting, result);
    return (arg.captureSetting_ == result);
}

/*
 * Feature: Framework
 * Function: Test get HostName
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test get HostName
 */
HWTEST_F(CameraFrameworkUnitTest, Camera_fwInfoManager_unittest_001, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    ASSERT_NE(static_cast<int>(cameras.size()), 0);
    std::string hostName = cameras[0]->GetHostName();
    ASSERT_EQ(hostName, "");
}

/*
 * Feature: Framework
 * Function: Test get DeviceType
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test get DeviceType
 */
HWTEST_F(CameraFrameworkUnitTest, Camera_fwInfoManager_unittest_002, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    ASSERT_NE(static_cast<int>(cameras.size()), 0);
    auto judgeDeviceType = [&cameras] () -> bool {
        bool isOk = false;
        uint16_t deviceType = cameras[0]->GetDeviceType();
        switch (deviceType) {
            case UNKNOWN:
            case PHONE:
            case TABLET:
                isOk = true;
                break;
            default:
                isOk = false;
                break;
        }
        return isOk;
    };
    ASSERT_NE(judgeDeviceType(), false);
}

/*
 * Feature: Framework
 * Function: Test result callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test result callback
 */
HWTEST_F(CameraFrameworkUnitTest, Camera_ResultCallback_unittest, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    std::shared_ptr<TestOnResultCallback> setResultCallback =
    std::make_shared<TestOnResultCallback>("OnResultCallback");

    input->SetResultCallback(setResultCallback);
    std::shared_ptr<ResultCallback> getResultCallback = input->GetResultCallback();
    ASSERT_EQ(getResultCallback, setResultCallback);
}

/*
 * Feature: Framework
 * Function: Test get cameras
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test get cameras
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_001, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    ASSERT_TRUE(cameras.size() != 0);
}

/*
 * Feature: Framework
 * Function: Test create input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create input
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_002, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    ASSERT_TRUE(cameras.size() != 0);
    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    ASSERT_NE(input->GetCameraDevice(), nullptr);

    input->Release();
}

/*
 * Feature: Framework
 * Function: Test create input with null camera
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create input with null camera
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_003, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    ASSERT_TRUE(cameras.size() != 0);
    cameras[0] = nullptr;
    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    EXPECT_EQ(input, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create session
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create session
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_004, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test create session with SceneMode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create session with SceneMode
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_005, TestSize.Level0)
{
    SceneMode mode = NORMAL;
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = CAPTURE;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = VIDEO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = PORTRAIT;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = NIGHT;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = PROFESSIONAL;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = SLOW_MOTION;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = SCAN;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = CAPTURE_MACRO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = VIDEO_MACRO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = PROFESSIONAL_PHOTO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = PROFESSIONAL_VIDEO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = HIGH_FRAME_RATE;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = HIGH_RES_PHOTO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = SECURE;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = LIGHT_PAINTING;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = FLUORESCENCE_PHOTO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = PANORAMA_PHOTO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = TIMELAPSE_PHOTO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
    mode = APERTURE_VIDEO;
    session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test create session with sptr<CaptureSession>*
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create session with sptr<CaptureSession>*
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_006, TestSize.Level0)
{
    sptr<CaptureSession> *session = new (std::nothrow) sptr<CaptureSession>;
    cameraManager->CreateCaptureSession(session);
    ASSERT_NE(*session, nullptr);
    sptr<CaptureSession>psession = *session;
    EXPECT_EQ(psession->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test create two sessions with same cameraManager
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create two sessions with same cameraManager
 *(After the second session is created, the first session is not left empty.
 * Waiting for subsequent development)
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_007, TestSize.Level0)
{
    //  sptr<CaptureSession> session1 = cameraManager->CreateCaptureSession(); Not supported
    // ASSERT_NE(session1, nullptr); Not supported
    sptr<CaptureSession> session2 = cameraManager->CreateCaptureSession();
    // ASSERT_NE(session1, nullptr); Not supported
    ASSERT_NE(session2, nullptr);
    // EXPECT_EQ(session1->Release(), 0); Not supported
    EXPECT_EQ(session2->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test create preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create preview output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_008, TestSize.Level0)
{
    int32_t width = PREVIEW_DEFAULT_WIDTH;
    int32_t height = PREVIEW_DEFAULT_HEIGHT;
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = width;
    previewSize.height = height;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(preview, nullptr);
    preview->Release();
}

/*
 * Feature: Framework
 * Function: Test create preview output with surface as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create preview output with surface as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_009, TestSize.Level0)
{
    int32_t width = PREVIEW_DEFAULT_WIDTH;
    int32_t height = PREVIEW_DEFAULT_HEIGHT;
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = width;
    previewSize.height = height;
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<Surface> surface = nullptr;
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_EQ(preview, nullptr);
}

/*
 * Feature: Framework
 * Function: Test two create preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test two create preview output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_010, TestSize.Level0)
{
    int32_t width = PREVIEW_DEFAULT_WIDTH;
    int32_t height = PREVIEW_DEFAULT_HEIGHT;
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = width;
    previewSize.height = height;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> preview1 = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(preview1, nullptr);
    sptr<CaptureOutput> preview2 = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(preview2, nullptr);
    preview1->Release();
    preview2->Release();
}

/*
 * Feature: Framework
 * Function: Test create photo output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create photo output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_011, TestSize.Level0)
{
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    CameraFormat photoFormat = CAMERA_FORMAT_JPEG;
    Size photoSize;
    photoSize.width = width;
    photoSize.height = height;
    Profile photoProfile = Profile(photoFormat, photoSize);
    sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
    sptr<PhotoOutput> photo = cameraManager->CreatePhotoOutput(photoProfile, surfaceProducer);
    ASSERT_NE(photo, nullptr);
    photo->Release();
}

/*
 * Feature: Framework
 * Function: Test create photo output with surface as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create photo output with surface as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_012, TestSize.Level0)
{
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;
    CameraFormat photoFormat = CAMERA_FORMAT_JPEG;
    Size photoSize;
    photoSize.width = width;
    photoSize.height = height;
    Profile photoProfile = Profile(photoFormat, photoSize);
    sptr<IBufferProducer> surfaceProducer = nullptr;
    sptr<PhotoOutput> photo = cameraManager->CreatePhotoOutput(photoProfile, surfaceProducer);
    ASSERT_EQ(photo, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create photo output with only surface
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create photo output with only surface
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_013, TestSize.Level0)
{
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
    sptr<PhotoOutput> photo = cameraManager->CreatePhotoOutput(surfaceProducer);
    ASSERT_EQ(photo, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create video output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create video output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_015, TestSize.Level0)
{
    int32_t width = VIDEO_DEFAULT_WIDTH;
    int32_t height = VIDEO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = width;
    videoSize.height = height;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(videoProfile, surface);
    ASSERT_NE(video, nullptr);
    video->Release();
}

/*
 * Feature: Framework
 * Function: Test create video output with surface as null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create video output with surface as null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_016, TestSize.Level0)
{
    int32_t width = VIDEO_DEFAULT_WIDTH;
    int32_t height = VIDEO_DEFAULT_HEIGHT;
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    sptr<Surface> surface = nullptr;
    Size videoSize;
    videoSize.width = width;
    videoSize.height = height;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(videoProfile, surface);
    ASSERT_EQ(video, nullptr);
}

/*
 * Feature: Framework
 * Function: Test create video output only surface
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test create video output only surface
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_017, TestSize.Level0)
{
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(surface);
    ASSERT_EQ(video, nullptr);
}

/*
 * Feature: Framework
 * Function: Test manager callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test manager callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_019, TestSize.Level0)
{
    std::shared_ptr<TestCameraMngerCallback> setCallback = std::make_shared<TestCameraMngerCallback>("MgrCallback");
    cameraManager->SetCallback(setCallback);
    std::shared_ptr<CameraManagerCallback> getCallback = cameraManager->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test set camera parameters
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test set camera parameters zoom, focus, flash & exposure
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_020, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);
    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    SessionControlParams(session);

    session->RemoveOutput(photo);
    session->RemoveInput(input);
    photo->Release();
    input->Release();
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test input callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test input callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_021, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    std::shared_ptr<TestDeviceCallback> setCallback = std::make_shared<TestDeviceCallback>("InputCallback");
    input->SetErrorCallback(setCallback);
    std::shared_ptr<ErrorCallback> getCallback = input->GetErrorCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test preview callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preview callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_022, TestSize.Level0)
{
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    std::shared_ptr<PreviewStateCallback> setCallback =
        std::make_shared<TestPreviewOutputCallback>("PreviewStateCallback");
    ((sptr<PreviewOutput> &)preview)->SetCallback(setCallback);
    std::shared_ptr<PreviewStateCallback> getCallback = ((sptr<PreviewOutput> &)preview)->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test photo callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_023, TestSize.Level0)
{
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    std::shared_ptr<PhotoStateCallback> setCallback = std::make_shared<TestPhotoOutputCallback>("PhotoStateCallback");
    ((sptr<PhotoOutput> &)photo)->SetCallback(setCallback);
    std::shared_ptr<PhotoStateCallback> getCallback = ((sptr<PhotoOutput> &)photo)->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test video callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test video callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_024, TestSize.Level0)
{
    sptr<CaptureOutput> video = CreateVideoOutput();
    ASSERT_NE(video, nullptr);

    std::shared_ptr<VideoStateCallback> setCallback = std::make_shared<TestVideoOutputCallback>("VideoStateCallback");
    ((sptr<VideoOutput> &)video)->SetCallback(setCallback);
    std::shared_ptr<VideoStateCallback> getCallback = ((sptr<VideoOutput> &)video)->GetApplicationCallback();
    ASSERT_EQ(setCallback, getCallback);
}

/*
 * Feature: Framework
 * Function: Test capture session add input with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add input with invalid value
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_025, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureInput> input = nullptr;
    ret = session->AddInput(input);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test capture session add output with invalid value
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session add output with invalid value
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_026, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureOutput> preview = nullptr;
    ret = session->AddOutput(preview);
    EXPECT_NE(ret, 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding input
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_027, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 7400201);

    ret = session->CommitConfig();
    EXPECT_NE(ret, 0);
    session->RemoveOutput(preview);
    preview->Release();
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test capture session commit config without adding output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session commit config without adding output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_028, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_NE(ret, 0);
    session->RemoveInput(input);
    input->Release();
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test capture session without begin config
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session without begin config
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_029, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->AddInput(input);
    EXPECT_NE(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_NE(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_NE(ret, 0);

    ret = session->CommitConfig();
    EXPECT_NE(ret, 0);

    ret = session->Start();
    EXPECT_NE(ret, 0);

    ret = ((sptr<PreviewOutput> &)preview)->Start();
    EXPECT_NE(ret, 0);

    ret = ((sptr<PhotoOutput> &)photo)->Capture();
    EXPECT_NE(ret, 0);

    ret = ((sptr<PreviewOutput> &)preview)->Stop();
    EXPECT_NE(ret, 0);

    ret = session->Stop();
    EXPECT_NE(ret, 0);
    session->RemoveInput(input);
    session->RemoveOutput(preview);
    session->RemoveOutput(photo);
    preview->Release();
    photo->Release();
    input->Release();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session start and stop without adding preview output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session start and stop without adding preview output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_030, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(_, _, true)).Times(0);
    ret = session->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(_)).Times(0);

    ret = session->Stop();
    EXPECT_EQ(ret, 0);

    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + photo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + photo
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_031, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(1, _, true));
    ret = session->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, Capture(2, _, false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(1));
    ret = ((sptr<PreviewOutput> &)preview)->Stop();
    EXPECT_EQ(ret, 0);

    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + photo with camera configuration
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + photo with camera configuration
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_032, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    std::vector<float> zoomRatioRange = session->GetZoomRatioRange();
    if (!zoomRatioRange.empty()) {
        session->LockForControl();
        session->SetZoomRatio(zoomRatioRange[0]);
        session->UnlockForControl();
    }

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    if (!zoomRatioRange.empty()) {
        EXPECT_CALL(*mockCameraDevice, UpdateSettings(_));
    }
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + video
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + video
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_033, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> video = CreateVideoOutput();
    ASSERT_NE(video, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(video);
    EXPECT_EQ(ret, 0);

    SessionCommit(session);

    EXPECT_CALL(*mockStreamOperator, Capture(4, _, true));
    ret = ((sptr<VideoOutput> &)video)->Start();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(4));
    ret = ((sptr<VideoOutput> &)video)->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(3));
    ret = ((sptr<PreviewOutput> &)preview)->Stop();
    EXPECT_EQ(ret, 0);

    ((sptr<VideoOutput> &)video)->Release();
    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test session with preview + video + photo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test session with preview + video + photo
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_034, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> video = CreateVideoOutput();
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(video);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);
    EXPECT_CALL(*mockStreamOperator, Capture(_, _, _));
    ret = session->Start();

    EXPECT_CALL(*mockStreamOperator, Capture(_, _, _));
    ret = ((sptr<VideoOutput> &)video)->Start();
    EXPECT_EQ(ret, 0);
    
    EXPECT_CALL(*mockStreamOperator, Capture(_, _, _));
    ret = ((sptr<PhotoOutput> &)photo)->Capture();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(_));
    ret = ((sptr<VideoOutput> &)video)->Stop();
    EXPECT_EQ(ret, 0);

    EXPECT_CALL(*mockStreamOperator, CancelCapture(_));
    ret = ((sptr<PreviewOutput> &)preview)->Stop();
    EXPECT_EQ(ret, 0);

    ((sptr<VideoOutput> &)video)->Release();
    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session remove output with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove output with null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_035, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureOutput> output = nullptr;
    ret = session->RemoveOutput(output);
    EXPECT_NE(ret, 0);
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session remove output
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove output
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_036, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> video = CreateVideoOutput();
    ASSERT_NE(video, nullptr);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(video);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->RemoveOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->RemoveOutput(video);
    EXPECT_EQ(ret, 0);

    ret = session->RemoveOutput(photo);
    EXPECT_EQ(ret, 0);

    input->Release();
    video->Release();
    preview->Release();
    photo->Release();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session remove input with null
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove input with null
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_037, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    sptr<CaptureInput> input = nullptr;
    ret = session->RemoveInput(input);
    EXPECT_NE(ret, 0);
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session remove input
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session remove input
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_038, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->RemoveInput(input);
    EXPECT_EQ(ret, 0);
    input->Release();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple preview outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple preview ouputs
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_039, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    intResult = session->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput1 = CreatePreviewOutput();
    ASSERT_NE(previewOutput1, nullptr);

    intResult = session->AddOutput(previewOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput2 = CreatePreviewOutput();
    ASSERT_NE(previewOutput2, nullptr);

    intResult = session->AddOutput(previewOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sleep(SLEEP_TIME_FOR_UNIT);
    intResult = ((sptr<PreviewOutput>&)previewOutput1)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(SLEEP_TIME_FOR_UNIT);
    intResult = ((sptr<PreviewOutput>&)previewOutput2)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(SLEEP_TIME_FOR_UNIT);

    ((sptr<PreviewOutput>&)previewOutput1)->Release();
    ((sptr<PreviewOutput>&)previewOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test capture session with multiple video outputs
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test capture session with multiple video ouputs
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_040, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    intResult = session->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutput = CreatePreviewOutput();
    ASSERT_NE(previewOutput, nullptr);

    intResult = session->AddOutput(previewOutput);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput1 = CreateVideoOutput();
    ASSERT_NE(videoOutput1, nullptr);

    intResult = session->AddOutput(videoOutput1);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutput2 = CreateVideoOutput();
    ASSERT_NE(videoOutput2, nullptr);

    intResult = session->AddOutput(videoOutput2);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->Start();
    EXPECT_EQ(intResult, 0);
    sleep(SLEEP_TIME_FOR_UNIT);
    intResult = ((sptr<VideoOutput>&)videoOutput1)->Start();
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<VideoOutput>&)videoOutput2)->Start();
    EXPECT_EQ(intResult, 0);

    sleep(SLEEP_TIME_FOR_UNIT);

    intResult = ((sptr<VideoOutput>&)videoOutput1)->Stop();
    EXPECT_EQ(intResult, 0);

    intResult = ((sptr<VideoOutput>&)videoOutput2)->Stop();
    EXPECT_EQ(intResult, 0);

    TestUtils::SaveVideoFile(nullptr, 0, VideoSaveMode::CLOSE, g_videoFd);

    ((sptr<PreviewOutput>&)previewOutput)->Stop();
    session->Stop();

    ((sptr<VideoOutput>&)videoOutput1)->Release();
    ((sptr<VideoOutput>&)videoOutput2)->Release();
}

/*
 * Feature: Framework
 * Function: Test photo capture with photo settings
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo capture with photo settings
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_041, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);
    photoSetting->SetQuality(PhotoCaptureSetting::QUALITY_LEVEL_MEDIUM);
    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_90);
    EXPECT_EQ(photoSetting->GetQuality(), PhotoCaptureSetting::QUALITY_LEVEL_MEDIUM);

    EXPECT_CALL(*mockStreamOperator, Capture(_,
        matchCaptureSetting(photoSetting->GetCaptureMetadataSetting()), false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture(photoSetting);
    EXPECT_EQ(ret, 0);

    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test photo capture with SetRotation twice
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo capture with SetRotation twice
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_042, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);
    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_90);

    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_180);
    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_180);

    EXPECT_CALL(*mockStreamOperator, Capture(_,
        matchCaptureSetting(photoSetting->GetCaptureMetadataSetting()), false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture(photoSetting);
    EXPECT_EQ(ret, 0);

    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test photo capture with SetQuality QUALITY_LEVEL_HIGH
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo capture with SetQuality QUALITY_LEVEL_HIGH
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_043, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();

    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);

    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_90);

    photoSetting->SetQuality(PhotoCaptureSetting::QUALITY_LEVEL_HIGH);

    EXPECT_EQ(photoSetting->GetQuality(), PhotoCaptureSetting::QUALITY_LEVEL_HIGH);

    EXPECT_CALL(*mockStreamOperator, Capture(_,
        matchCaptureSetting(photoSetting->GetCaptureMetadataSetting()), false));
    
    ret = ((sptr<PhotoOutput> &)photo)->Capture(photoSetting);
    EXPECT_EQ(ret, 0);

    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test photo capture with SetQuality QUALITY_LEVEL_LOW
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photo capture with SetQuality QUALITY_LEVEL_LOW
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_044, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();

    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);
    
    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_90);

    photoSetting->SetQuality(PhotoCaptureSetting::QUALITY_LEVEL_LOW);

    EXPECT_EQ(photoSetting->GetQuality(), PhotoCaptureSetting::QUALITY_LEVEL_LOW);

    EXPECT_CALL(*mockStreamOperator, Capture(_,
        matchCaptureSetting(photoSetting->GetCaptureMetadataSetting()), false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture(photoSetting);
    EXPECT_EQ(ret, 0);

    session->Release();
    input->Release();
}

/*
 * Feature: Framework
 * Function: Test GetFocalLength
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetFocalLength
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_045, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    double focalLength = session->GetFocalLength();
    ASSERT_EQ(focalLength, 1.5);

    session->RemoveInput(input);
    session->RemoveOutput(photo);
    photo->Release();
    input->Release();
    session->Release();
}


/*
 * Feature: Framework
 * Function: Test SetMeteringPoint & GetMeteringPoint
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetMeteringPoint & GetMeteringPoint
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_046, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    Point exposurePoint = {1.0, 2.0};
    session->LockForControl();
    session->SetMeteringPoint(exposurePoint);
    session->UnlockForControl();
    ASSERT_EQ((session->GetMeteringPoint().x), exposurePoint.x > 1 ? 1 : exposurePoint.x);
    ASSERT_EQ((session->GetMeteringPoint().y), exposurePoint.y > 1 ? 1 : exposurePoint.y);

    session->RemoveInput(input);
    session->RemoveOutput(photo);
    photo->Release();
    input->Release();
    session->Release();
}


/*
 * Feature: Framework
 * Function: Test SetFocusPoint & GetFousPoint
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetFocusPoint & GetFousPoint
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_047, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    Point FocusPoint = {1.0, 2.0};
    session->LockForControl();
    session->SetFocusPoint(FocusPoint);
    session->UnlockForControl();
    ASSERT_EQ((session->GetFocusPoint().x), FocusPoint.x > 1 ? 1 : FocusPoint.x);
    ASSERT_EQ((session->GetFocusPoint().y), FocusPoint.y > 1 ? 1 : FocusPoint.y);

    session->RemoveInput(input);
    session->RemoveOutput(photo);
    photo->Release();
    input->Release();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test GetExposureValue and SetExposureBias
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureValue and SetExposureBias with value less then the range
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_048, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::vector<float> exposurebiasRange = session->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        session->LockForControl();
        session->SetExposureBias(exposurebiasRange[0]-1.0);
        session->UnlockForControl();
        ASSERT_EQ(session->GetExposureValue(), exposurebiasRange[0]);
    }

    session->RemoveInput(input);
    session->RemoveOutput(photo);
    photo->Release();
    input->Release();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test GetExposureValue and SetExposureBias
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureValue and SetExposureBias with value between the range
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_049, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::vector<float> exposurebiasRange = session->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        session->LockForControl();
        session->SetExposureBias(exposurebiasRange[0]+1.0);
        session->UnlockForControl();
        EXPECT_TRUE((session->GetExposureValue()>=exposurebiasRange[0] &&
                session->GetExposureValue()<=exposurebiasRange[1]));
    }
    session->RemoveInput(input);
    session->RemoveOutput(photo);
    photo->Release();
    input->Release();
    session->Release();
}


/*
 * Feature: Framework
 * Function: Test GetExposureValue and SetExposureBias
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureValue and SetExposureBias with value more then the range
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_050, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);
    SceneMode mode = CAPTURE;
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::vector<float> exposurebiasRange = session->GetExposureBiasRange();
    if (!exposurebiasRange.empty()) {
        session->LockForControl();
        session->SetExposureBias(exposurebiasRange[1]+1.0);
        session->UnlockForControl();
    }
    ASSERT_EQ(session->GetExposureValue(), exposurebiasRange[1]);
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test cameraManager and portrait session with beauty/filter/portrait effects
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameraManager and portrait session with beauty/filter/portrait effects
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_051, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();
    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    int32_t ret = portraitSession->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->CommitConfig();
    EXPECT_EQ(ret, 0);

    PortraitSessionControlParams(portraitSession);

    portraitSession->Release();
}

/*
 * Feature: Framework
 * Function: Test cameraManager with GetSupportedModes
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetSupportedModes to get modes
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_052, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
}

/*
 * Feature: Framework
 * Function: Test cameraManager with GetSupportedOutputCapability
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetSupportedOutputCapability to get capability
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_053, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
}

/*
 * Feature: Framework
 * Function: Test cameraManager with CreateCaptureSession and CommitConfig
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameraManager to CreateCaptureSession
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_054, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);


    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);


    int32_t ret = portraitSession->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->CommitConfig();
    EXPECT_EQ(ret, 0);
    portraitSession->Release();
}

/*
 * Feature: Framework
 * Function: Test GetSupportedPortraitEffects / GetPortraitEffect / SetPortraitEffect
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetPortraitEffect and SetPortraitEffect with value
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_055, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();


    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    int32_t ret = portraitSession->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->CommitConfig();
    EXPECT_EQ(ret, 0);

    PortraitSessionEffectParams(portraitSession);

    portraitSession->Release();
}

/*
 * Feature: Framework
 * Function: Test GetSupportedFilters / GetFilter / SetFilter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetFilter and SetFilter with value
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_056, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    int32_t ret = portraitSession->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->CommitConfig();
    EXPECT_EQ(ret, 0);

    PortraitSessionFilterParams(portraitSession);

    portraitSession->Release();
}

/*
 * Feature: Framework
 * Function: Test GetSupportedBeautyTypes / GetSupportedBeautyRange / GetBeauty / SetBeauty
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetBeauty and SetBeauty with value
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_057, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();


    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    int32_t ret = portraitSession->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = portraitSession->CommitConfig();
    EXPECT_EQ(ret, 0);

    PortraitSessionBeautyParams(portraitSession);

    portraitSession->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice with anomalous branch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_001, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow) HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);

    int32_t ret = camDevice->HCameraDevice::Open();
    EXPECT_EQ(ret, 0);

    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);

    ret = camDevice->HCameraDevice::EnableResult(result);
    EXPECT_EQ(ret, 0);

    ret = camDevice->HCameraDevice::DisableResult(result);
    EXPECT_EQ(ret, 0);

    sptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator = camDevice->HCameraDevice::GetStreamOperator();
    EXPECT_TRUE(streamOperator != nullptr);

    ret = camDevice->HCameraDevice::OnError(REQUEST_TIMEOUT, 0);
    EXPECT_EQ(ret, 0);

    ret = camDevice->HCameraDevice::Close();
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test dump with args empty
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_002, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> camService = (sptr<HCameraService> &)mockHCameraService;
    ASSERT_NE(camService, nullptr);

    std::vector<std::string> cameraIds = {};
    std::vector<std::shared_ptr<OHOS::Camera::CameraMetadata>> cameraAbilityList = {};
    int32_t ret = camService->GetCameras(cameraIds, cameraAbilityList);

    int fd = 0;
    std::vector<std::u16string> args = {};
    ret = camService->Dump(fd, args);
    EXPECT_EQ(ret, 0);

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession with anomalous branch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_003, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    sptr<StreamOperatorCallback> streamOperatorCb = nullptr;
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    sptr<ICameraDeviceService> cameraDevice = camInput->GetCameraDevice();
    int32_t ret = camSession->AddInput(cameraDevice);
    EXPECT_EQ(ret, 10);

    ret = camSession->RemoveInput(cameraDevice);
    EXPECT_EQ(ret, 10);

    ret = camSession->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = camSession->BeginConfig();
    EXPECT_EQ(ret, 10);

    cameraDevice = nullptr;
    ret = camSession->AddInput(cameraDevice);
    EXPECT_EQ(ret, 2);

    ret = camSession->RemoveInput(cameraDevice);
    EXPECT_EQ(ret, 2);

    sptr<IStreamCommon> stream_2 = nullptr;
    ret = camSession->AddOutput(StreamType::CAPTURE, stream_2);
    EXPECT_EQ(ret, 2);

    ret = camSession->RemoveOutput(StreamType::CAPTURE, stream_2);
    EXPECT_EQ(ret, 2);

    input->Close();
    camSession->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraService with anomalous branch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_004, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> cameraService = (sptr<HCameraService> &)mockHCameraService;
    ASSERT_NE(cameraService, nullptr);

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> Producer = Surface->GetProducer();

    int32_t height = 0;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    sptr<IStreamRepeat> output = nullptr;
    int32_t intResult = cameraService->CreateDeferredPreviewOutput(0, width, height, output);
    EXPECT_EQ(intResult, 2);

    width = 0;
    intResult = cameraService->CreateDeferredPreviewOutput(0, width, height, output);
    EXPECT_EQ(intResult, 2);

    Producer = nullptr;
    intResult = cameraService->CreatePreviewOutput(Producer, 0, width, height, output);
    EXPECT_EQ(intResult, 2);

    Producer = Surface->GetProducer();
    intResult = cameraService->CreatePreviewOutput(Producer, 0, width, height, output);
    EXPECT_EQ(intResult, 2);

    width = PREVIEW_DEFAULT_WIDTH;
    intResult = cameraService->CreatePreviewOutput(Producer, 0, width, height, output);
    EXPECT_EQ(intResult, 2);

    intResult = cameraService->CreateVideoOutput(Producer, 0, width, height, output);
    EXPECT_EQ(intResult, 2);

    width = 0;
    intResult = cameraService->CreateVideoOutput(Producer, 0, width, height, output);
    EXPECT_EQ(intResult, 2);

    Producer = nullptr;
    intResult = cameraService->CreateVideoOutput(Producer, 0, width, height, output);
    EXPECT_EQ(intResult, 2);

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraService with anomalous branch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_005, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> cameraService = (sptr<HCameraService> &)mockHCameraService;
    ASSERT_NE(cameraService, nullptr);

    int32_t width = 0;
    int32_t height = 0;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> Producer = Surface->GetProducer();

    sptr<IStreamCapture> output_1 = nullptr;
    int32_t intResult = cameraService->CreatePhotoOutput(Producer, 0, width, height, output_1);
    EXPECT_EQ(intResult, 2);

    width = PHOTO_DEFAULT_WIDTH;
    intResult = cameraService->CreatePhotoOutput(Producer, 0, width, height, output_1);
    EXPECT_EQ(intResult, 2);

    Producer = nullptr;
    intResult = cameraService->CreatePhotoOutput(Producer, 0, width, height, output_1);
    EXPECT_EQ(intResult, 2);

    sptr<IStreamMetadata> output_2 = nullptr;
    intResult = cameraService->CreateMetadataOutput(Producer, 0, {1}, output_2);
    EXPECT_EQ(intResult, 2);

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraService with anomalous branch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_006, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> cameraService = (sptr<HCameraService> &)mockHCameraService;
    ASSERT_NE(cameraService, nullptr);

    sptr<ICameraServiceCallback> callback = nullptr;
    int32_t intResult = cameraService->SetCameraCallback(callback);
    EXPECT_EQ(intResult, 2);

    callback = new(std::nothrow) CameraStatusServiceCallback(cameraManager);
    ASSERT_NE(callback, nullptr);
    intResult = cameraService->SetCameraCallback(callback);
    EXPECT_EQ(intResult, 0);

    sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    sptr<ICameraMuteServiceCallback> callback_2 = nullptr;
    intResult = cameraService->SetMuteCallback(callback_2);
    EXPECT_EQ(intResult, 2);

    callback_2 = new(std::nothrow) CameraMuteServiceCallback(cameraManager);
    ASSERT_NE(callback_2, nullptr);
    intResult = cameraService->SetMuteCallback(callback_2);
    EXPECT_EQ(intResult, 0);

    std::string cameraId = camInput->GetCameraId();
    int activeTime = 15;
    EffectParam effectParam = {0, 0, 0};
    intResult = cameraService->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::PERSISTENT_DEFAULT_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(intResult, 2);

    cameraId = "";
    intResult = cameraService->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::TRANSIENT_ACTIVE_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(intResult, 2);

    intResult = cameraService->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::PERSISTENT_DEFAULT_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(intResult, 2);

    cameraId = "";
    intResult = cameraService->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::PERSISTENT_DEFAULT_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(intResult, 2);

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test different status with OnReceive
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_007, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    cameraHostManager->Init();
    HDI::ServiceManager::V1_0::ServiceStatus status;

    status.serviceName = "1";
    cameraHostManager->GetRegisterServStatListener()->OnReceive(status);

    status.deviceClass = DEVICE_CLASS_CAMERA;
    status.serviceName = "distributed_camera_service";
    status.status = HDI::ServiceManager::V1_0::SERVIE_STATUS_START;
    cameraHostManager->GetRegisterServStatListener()->OnReceive(status);
    cameraHostManager->GetRegisterServStatListener()->OnReceive(status);

    status.status = HDI::ServiceManager::V1_0::SERVIE_STATUS_STOP;
    cameraHostManager->GetRegisterServStatListener()->OnReceive(status);

    status.status = 4;
    cameraHostManager->GetRegisterServStatListener()->OnReceive(status);
    cameraHostManager->DeInit();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CaptureSession with no static capability.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_008, TestSize.Level0)
{
    g_mockFlagWithoutAbt = true;
    sptr<CameraManager> camManager = new FakeCameraManager(new FakeHCameraService(mockCameraHostManager));

    std::vector<sptr<CameraDevice>> cameras = camManager->GetSupportedCameras();
    sptr<CaptureInput> input = camManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = camManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->BeginConfig(), 0);

    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(photo), 0);

    EXPECT_EQ(session->CommitConfig(), 0);

    sptr<CameraOutputCapability> cameraOutputCapability = camManager->GetSupportedOutputCapability(cameras[0]);

    std::vector<VideoStabilizationMode> videoStabilizationMode = session->GetSupportedStabilizationMode();
    EXPECT_EQ(videoStabilizationMode.empty(), true);

    int32_t intResult = session->GetSupportedStabilizationMode(videoStabilizationMode);
    EXPECT_EQ(intResult, 0);

    std::vector<ExposureMode> getSupportedExpModes = session->GetSupportedExposureModes();
    EXPECT_EQ(getSupportedExpModes.empty(), true);

    EXPECT_EQ(session->GetSupportedExposureModes(getSupportedExpModes), 0);

    EXPECT_EQ(session->GetExposureMode(), -1);
    EXPECT_EQ((session->GetMeteringPoint().x), 0);
    EXPECT_EQ((session->GetMeteringPoint().y), 0);

    float exposureValue = session->GetExposureValue();
    EXPECT_EQ(exposureValue, 0);
    int32_t exposureValueGet = session->GetExposureValue(exposureValue);
    EXPECT_EQ(exposureValueGet, 0);

    input->Close();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CaptureSession with no static capability.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_009, TestSize.Level0)
{
    g_mockFlagWithoutAbt = true;
    sptr<CameraManager> camManager = new FakeCameraManager(new FakeHCameraService(mockCameraHostManager));

    std::vector<sptr<CameraDevice>> cameras = camManager->GetSupportedCameras();

    sptr<CaptureInput> input = camManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = camManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->BeginConfig(), 0);

    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(photo), 0);

    EXPECT_EQ(session->CommitConfig(), 0);

    sptr<CameraOutputCapability> cameraOutputCapability = camManager->GetSupportedOutputCapability(cameras[0]);

    std::vector<FocusMode> supportedFocusModes = session->GetSupportedFocusModes();
    EXPECT_EQ(supportedFocusModes.empty(), true);
    EXPECT_EQ(session->GetSupportedFocusModes(supportedFocusModes), 0);

    EXPECT_EQ(session->GetFocusMode(), 0);

    float focalLength = session->GetFocalLength();
    EXPECT_EQ(focalLength, 0);
    EXPECT_EQ(session->GetFocalLength(focalLength), 0);

    std::vector<FlashMode> supportedFlashModes = session->GetSupportedFlashModes();
    EXPECT_EQ(supportedFlashModes.empty(), true);
    EXPECT_EQ(session->GetSupportedFlashModes(supportedFlashModes), 0);
    EXPECT_EQ(session->GetFlashMode(), 0);
    EXPECT_EQ(session->GetZoomRatio(), 0);

    bool isSupported;
    EXPECT_EQ(session->IsVideoStabilizationModeSupported(MIDDLE, isSupported), 0);
    if (isSupported) {
        EXPECT_EQ(session->SetVideoStabilizationMode(MIDDLE), 0);
    } else {
        EXPECT_EQ(session->SetVideoStabilizationMode(MIDDLE), 7400102);
    }
    EXPECT_EQ(session->IsFlashModeSupported(FLASH_MODE_AUTO), false);
    EXPECT_EQ(session->IsFlashModeSupported(FLASH_MODE_AUTO, isSupported), 0);

    sptr<PhotoOutput> photoOutput = (sptr<PhotoOutput> &)photo;
    EXPECT_EQ(photoOutput->IsMirrorSupported(), false);

    input->Close();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test dump with no static capability.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_010, TestSize.Level0)
{
    g_mockFlagWithoutAbt = true;
    sptr<CameraManager> camManager = new FakeCameraManager(new FakeHCameraService(mockCameraHostManager));
    std::vector<sptr<CameraDevice>> cameras = camManager->GetSupportedCameras();

    sptr<CaptureInput> input = camManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> camService = (sptr<HCameraService> &)mockHCameraService;
    ASSERT_NE(camService, nullptr);

    camService->OnStart();

    std::vector<std::string> cameraIds = {};
    std::vector<std::shared_ptr<OHOS::Camera::CameraMetadata>> cameraAbilityList = {};
    int32_t ret = camService->GetCameras(cameraIds, cameraAbilityList);

    int fd = 0;
    std::vector<std::u16string> args = {};
    ret = camService->Dump(fd, args);
    EXPECT_EQ(ret, 0);

    std::u16string cameraServiceInfo = u"";
    args.push_back(cameraServiceInfo);
    ret = camService->Dump(fd, args);
    EXPECT_EQ(ret, 0);

    input->Close();
}

/*
 * Feature: coverage
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamCapture SetRotation with no static capability.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_011, TestSize.Level0)
{
    g_mockFlagWithoutAbt = true;
    sptr<CameraManager> camManager = new FakeCameraManager(new FakeHCameraService(mockCameraHostManager));
    std::vector<sptr<CameraDevice>> cameras = camManager->GetSupportedCameras();

    sptr<CaptureInput> input = camManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = camManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);
    photoSetting->SetQuality(PhotoCaptureSetting::QUALITY_LEVEL_MEDIUM);
    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_90);
    EXPECT_EQ(photoSetting->GetQuality(), PhotoCaptureSetting::QUALITY_LEVEL_MEDIUM);

    EXPECT_CALL(*mockStreamOperator, Capture(_,
        matchCaptureSetting(photoSetting->GetCaptureMetadataSetting()), false));
    ret = ((sptr<PhotoOutput> &)photo)->Capture(photoSetting);
    EXPECT_EQ(ret, 0);

    input->Close();
    session->Release();
    input->Release();
}

/*
 * Feature: coverage
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat with no static capability.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_012, TestSize.Level0)
{
    g_mockFlagWithoutAbt = true;
    sptr<CameraManager> camManager = new FakeCameraManager(new FakeHCameraService(mockCameraHostManager));
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    std::vector<sptr<CameraDevice>> cameras = camManager->GetSupportedCameras();

    sptr<CaptureInput> input = camManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureOutput> metadatOutput = camManager->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    sptr<CaptureSession> session = camManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->BeginConfig(), 0);

    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(photo), 0);
    EXPECT_EQ(session->AddOutput(metadatOutput), 0);

    EXPECT_EQ(session->CommitConfig(), 0);

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer1 = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow) HStreamRepeat(producer1, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);

    std::shared_ptr<PhotoCaptureSetting> photoSetting = std::make_shared<PhotoCaptureSetting>();
    photoSetting->SetRotation(PhotoCaptureSetting::Rotation_90);
    photoSetting->SetQuality(PhotoCaptureSetting::QUALITY_LEVEL_MEDIUM);
    EXPECT_EQ(photoSetting->GetRotation(), PhotoCaptureSetting::Rotation_90);
    EXPECT_EQ(photoSetting->GetQuality(), PhotoCaptureSetting::QUALITY_LEVEL_MEDIUM);
    if (streamRepeat->LinkInput(mockStreamOperator, photoSetting->GetCaptureMetadataSetting()) != 0) {
        EXPECT_EQ(streamRepeat->Stop(), CAMERA_INVALID_STATE);
        EXPECT_EQ(streamRepeat->Start(), CAMERA_INVALID_STATE);

        EXPECT_EQ(streamRepeat->AddDeferredSurface(producer1), CAMERA_INVALID_STATE);

        EXPECT_EQ(streamRepeat->Start(), CAMERA_INVALID_STATE);
        EXPECT_EQ(streamRepeat->Stop(), CAMERA_INVALID_STATE);
    }
    input->Close();
    session->Release();
}

/*
 * Feature: coverage
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_014, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    camSession->BeginConfig();
    camSession->Start();

    sptr<ICameraDeviceService> cameraDevice = camInput->GetCameraDevice();
    EXPECT_EQ(camSession->AddInput(cameraDevice), 0);

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();

    auto streamRepeat = new (std::nothrow) HStreamRepeat(producer, 4, 1280, 960, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    auto streamRepeat1 = new (std::nothrow) HStreamRepeat(producer, 3, 640, 480, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat1, nullptr);
    sptr<HStreamCapture> streamCapture = new (std::nothrow) HStreamCapture(producer, 4, 1280, 960);
    ASSERT_NE(streamCapture, nullptr);

    EXPECT_EQ(camSession->AddOutput(StreamType::REPEAT, streamRepeat), 0);
    EXPECT_EQ(camSession->AddOutput(StreamType::REPEAT, streamRepeat1), 0);
    EXPECT_EQ(camSession->AddOutput(StreamType::CAPTURE, streamCapture), 0);

    CaptureErrorInfo it1;
    it1.streamId_ = 2;
    it1.error_ = BUFFER_LOST;
    CaptureErrorInfo it2;
    it2.streamId_ = 1;
    it2.error_ =  BUFFER_LOST;
    std::vector<CaptureErrorInfo> info = {};
    info.push_back(it1);
    info.push_back(it2);
    camSession->OnCaptureError(0, info);

    std::vector<int32_t> streamIds = {1, 2};
    camSession->OnFrameShutter(0, streamIds, 0);
    camSession->OnFrameShutterEnd(0, streamIds, 0);
    camSession->OnCaptureReady(0, streamIds, 0);
    input->Close();
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_015, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    camSession->BeginConfig();
    camSession->Start();

    sptr<ICameraDeviceService> cameraDevice = camInput->GetCameraDevice();
    EXPECT_EQ(camSession->AddInput(cameraDevice), 0);

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    sptr<HStreamCapture> streamCapture = new (std::nothrow) HStreamCapture(producer, 4, 1280, 960);
    sptr<HStreamCapture> streamCapture1 = new (std::nothrow) HStreamCapture(producer, 3, 640, 480);
    sptr<HStreamRepeat> streamRepeat =
        new (std::nothrow) HStreamRepeat(producer, 4, 1280, 960, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);

    EXPECT_EQ(camSession->AddOutput(StreamType::CAPTURE, streamCapture), 0);
    EXPECT_EQ(camSession->AddOutput(StreamType::CAPTURE, streamCapture1), 0);
    EXPECT_EQ(camSession->AddOutput(StreamType::REPEAT, streamRepeat), 0);

    CaptureErrorInfo it1;
    it1.streamId_ = 2;
    it1.error_ = BUFFER_LOST;
    CaptureErrorInfo it2;
    it2.streamId_ = 1;
    it2.error_ =  BUFFER_LOST;
    std::vector<CaptureErrorInfo> info = {};
    info.push_back(it1);
    info.push_back(it2);
    camSession->OnCaptureError(0, info);

    std::vector<int32_t> streamIds = {1, 2};
    camSession->OnFrameShutter(0, streamIds, 0);
    camSession->OnFrameShutterEnd(0, streamIds, 0);
    camSession->OnCaptureReady(0, streamIds, 0);

    input->Close();
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_016, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, PORTRAIT);
    ASSERT_NE(camSession, nullptr);

    EXPECT_EQ(camSession->CommitConfig(), CAMERA_INVALID_STATE);
    camSession->BeginConfig();
    camSession->Start();

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    sptr<HStreamCapture> streamCapture= new(std::nothrow) HStreamCapture(producer, 0, 0, 0);
    ASSERT_NE(streamCapture, nullptr);

    sptr<ICameraDeviceService> cameraDevice = camInput->GetCameraDevice();
    EXPECT_EQ(camSession->AddInput(cameraDevice), 0);
    EXPECT_EQ(camSession->AddOutput(StreamType::CAPTURE, streamCapture), 0);

    camSession->CommitConfig();

    CaptureErrorInfo it1;
    it1.streamId_ = 0;
    it1.error_ = BUFFER_LOST;
    CaptureErrorInfo it2;
    it2.streamId_ = 1;
    it2.error_ =  BUFFER_LOST;
    std::vector<CaptureErrorInfo> info = {};
    info.push_back(it1);
    info.push_back(it2);
    camSession->OnCaptureError(0, info);

    std::vector<int32_t> streamIds = {0, 1, 2};
    camSession->OnFrameShutter(0, streamIds, 0);
    camSession->OnFrameShutterEnd(0, streamIds, 0);
    camSession->OnCaptureReady(0, streamIds, 0);
    camSession->BeginConfig();

    input->Close();
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_017, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    sptr<StreamOperatorCallback> streamOperatorCb = nullptr;
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);
    EXPECT_EQ(camSession->Start(), CAMERA_INVALID_STATE);

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    sptr<HStreamRepeat> streamRepeat = new (std::nothrow) HStreamRepeat(producer, 0, 0, 0, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<HStreamMetadata> streamMetadata= new(std::nothrow) HStreamMetadata(producer, 0, {1});
    ASSERT_NE(streamMetadata, nullptr);
    EXPECT_EQ(camSession->AddOutput(StreamType::REPEAT, streamRepeat), CAMERA_INVALID_STATE);
    EXPECT_EQ(camSession->RemoveOutput(StreamType::REPEAT, streamRepeat), CAMERA_INVALID_STATE);

    sptr<ICameraDeviceService> cameraDevice = camInput->GetCameraDevice();
    camSession->BeginConfig();
    camSession->Start();
    camSession->AddOutput(StreamType::METADATA, streamMetadata);
    camSession->AddOutput(StreamType::METADATA, streamMetadata);
    camSession->RemoveOutput(StreamType::METADATA, streamMetadata);
    camSession->AddInput(cameraDevice);

    camSession->AddInput(cameraDevice);

    sptr<ICaptureSessionCallback> callback1 = nullptr;
    camSession->SetCallback(callback1);

    CameraInfoDumper infoDumper(0);
    camSession->DumpSessionInfo(infoDumper);
    camSession->DumpSessions(infoDumper);

    input->Close();
    camSession->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetExposureValue and SetExposureBias with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_018, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    session->LockForControl();

    Point exposurePoint = {1.0, 2.0};
    session->SetMeteringPoint(exposurePoint);
    session->SetMeteringPoint(exposurePoint);

    ExposureMode exposure = EXPOSURE_MODE_AUTO;
    bool exposureSupported = session->IsExposureModeSupported(exposure);
    if (exposureSupported) {
        session->SetExposureMode(exposure);
    }

    ret = session->GetExposureMode(exposure);
    EXPECT_EQ(ret, 0);

    ExposureMode exposureMode = session->GetExposureMode();
    exposureSupported = session->IsExposureModeSupported(exposureMode);
    if (exposureSupported) {
        int32_t setExposureMode = session->SetExposureMode(exposureMode);
        EXPECT_EQ(setExposureMode, 0);
    }
    session->UnlockForControl();
    input->Close();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetMeteringPoint and GetMeteringPoint with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_019, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    session->LockForControl();

    Point exposurePointGet = session->GetMeteringPoint();
    int32_t getMeteringPoint = session->GetMeteringPoint(exposurePointGet);
    EXPECT_EQ(getMeteringPoint, 0);

    float exposureValue = session->GetExposureValue();
    int32_t exposureValueGet = session->GetExposureValue(exposureValue);
    EXPECT_EQ(exposureValueGet, 0);

    int32_t setExposureBias = session->SetExposureBias(exposureValue);
    EXPECT_EQ(setExposureBias, 0);

    input->Close();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CaptureSession with anomalous branch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_020, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    std::shared_ptr<ExposureCallback> exposureCallback = std::make_shared<CallbackListener>();
    session->SetExposureCallback(exposureCallback);
    session->ProcessAutoExposureUpdates(metadata);

    std::shared_ptr<FocusCallback> focusCallback = std::make_shared<CallbackListener>();
    session->SetFocusCallback(focusCallback);
    session->ProcessAutoFocusUpdates(metadata);

    std::vector<FocusMode> getSupportedFocusModes = session->GetSupportedFocusModes();
    EXPECT_EQ(getSupportedFocusModes.empty(), false);
    int32_t supportedFocusModesGet = session->GetSupportedFocusModes(getSupportedFocusModes);
    EXPECT_EQ(supportedFocusModesGet, 0);

    input->Close();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CaptureSession with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_021, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    std::vector<FlashMode> getSupportedFlashModes = session->GetSupportedFlashModes();
    EXPECT_EQ(getSupportedFlashModes.empty(), false);
    int32_t getSupportedFlashMode = session->GetSupportedFlashModes(getSupportedFlashModes);
    EXPECT_EQ(getSupportedFlashMode, 0);

    FlashMode flashMode = session->GetFlashMode();
    int32_t flashModeGet = session->GetFlashMode(flashMode);
    EXPECT_EQ(flashModeGet, 0);
    int32_t setFlashMode = session->SetFlashMode(flashMode);
    EXPECT_EQ(setFlashMode, 0);

    input->Close();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetFocusPoint & GetFousPoint with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_022, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photo);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    Point FocusPoint = {1.0, 2.0};
    session->LockForControl();
    session->SetFocusPoint(FocusPoint);
    session->UnlockForControl();

    session->GetFocusPoint(FocusPoint);

    session->RemoveInput(input);
    session->RemoveOutput(photo);
    photo->Release();
    input->Close();
    input->Release();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test CaptureSession with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_023, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    EXPECT_EQ(session->BeginConfig(), 0);

    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(photo), 0);

    EXPECT_EQ(session->CommitConfig(), 0);

    EXPECT_EQ(session->GetFocusMode(), 2);

    session->LockForControl();

    FlashMode flash = FLASH_MODE_ALWAYS_OPEN;
    session->SetFlashMode(flash);
    session->SetFlashMode(flash);

    FocusMode focus = FOCUS_MODE_CONTINUOUS_AUTO;
    session->SetFocusMode(focus);
    session->SetFocusMode(focus);

    ExposureMode exposure = EXPOSURE_MODE_AUTO;
    session->SetExposureMode(exposure);

    float zoomRatioRange = session->GetZoomRatio();
    session->SetZoomRatio(zoomRatioRange);
    session->SetZoomRatio(zoomRatioRange);

    session->UnlockForControl();

    EXPECT_EQ(session->GetFocusMode(focus), 0);

    cameraManager->GetSupportedOutputCapability(cameras[0], 0);

    VideoStabilizationMode stabilizationMode = MIDDLE;
    session->GetActiveVideoStabilizationMode();
    session->GetActiveVideoStabilizationMode(stabilizationMode);
    session->SetVideoStabilizationMode(stabilizationMode);

    input->Close();
    input->Release();
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test IsCameraNeedClose & CheckPermission
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test IsCameraNeedClose & CheckPermission
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_024, TestSize.Level0)
{
    CamRetCode rc1 = HDI::Camera::V1_0::CAMERA_BUSY;
    EXPECT_EQ(HdiToServiceError(rc1), CAMERA_DEVICE_BUSY);
    CamRetCode rc2 = HDI::Camera::V1_0::INVALID_ARGUMENT;
    EXPECT_EQ(HdiToServiceError(rc2), CAMERA_INVALID_ARG);
    CamRetCode rc3 = HDI::Camera::V1_0::CAMERA_CLOSED;
    EXPECT_EQ(HdiToServiceError(rc3), CAMERA_DEVICE_CLOSED);
    CreateMsg(NULL);

    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    pid_t pid = IPCSkeleton::GetCallingPid();
    IsCameraNeedClose(callerToken, pid, pid);
    IsCameraNeedClose(123, pid, pid);
    CheckPermission("unittest", 0);
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_025, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow) HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);

    sptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator = nullptr;
    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<ICameraDeviceServiceCallback> callback1 = new(std::nothrow) CameraDeviceServiceCallback(input);
    ASSERT_NE(callback1, nullptr);

    camDevice->EnableResult(result);
    camDevice->DisableResult(result);

    int32_t ret = camDevice->Open();
    EXPECT_EQ(ret, 0);
    camDevice->UpdateSetting(nullptr);
    sptr<ICameraDeviceServiceCallback> callback = nullptr;
    camDevice->SetCallback(callback);
    camDevice->GetDeviceAbility();
    camDevice->SetCallback(callback1);
    camDevice->OnError(REQUEST_TIMEOUT, 0) ;
    camDevice->OnError(DEVICE_PREEMPT, 0) ;
    camDevice->OnError(DRIVER_ERROR, 0) ;

    EXPECT_EQ(camDevice->Close(), 0);
    EXPECT_EQ(camDevice->GetEnabledResults(result), 11);
    EXPECT_EQ(camDevice->Close(), 0);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeat & HStreamCommon
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat & HStreamCommon
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_026, TestSize.Level0)
{
    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    CameraInfoDumper infoDumper(0);
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<IStreamRepeatCallback> callback = nullptr;
    sptr<OHOS::IBufferProducer> producer = nullptr;
    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer1 = Surface->GetProducer();
    sptr<HStreamRepeat> streamRepeat =
        new (std::nothrow) HStreamRepeat(nullptr, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);

    EXPECT_EQ(streamRepeat->Start(), CAMERA_INVALID_STATE);
    EXPECT_EQ(streamRepeat->SetCallback(callback), CAMERA_INVALID_ARG);
    EXPECT_EQ(streamRepeat->OnFrameError(BUFFER_LOST), 0);
    EXPECT_EQ(streamRepeat->AddDeferredSurface(producer), CAMERA_INVALID_ARG);
    streamRepeat->DumpStreamInfo(infoDumper);
    EXPECT_EQ(streamRepeat->AddDeferredSurface(producer1), CAMERA_INVALID_STATE);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata1 = nullptr;
    EXPECT_EQ(streamRepeat->LinkInput(mockStreamOperator, metadata), CAMERA_OK);
    streamRepeat->LinkInput(mockStreamOperator, metadata1);
    mockStreamOperator = nullptr;
    streamRepeat->LinkInput(mockStreamOperator, metadata);
    streamRepeat->DumpStreamInfo(infoDumper);
    EXPECT_EQ(streamRepeat->Stop(), CAMERA_INVALID_STATE);
}

/*
 * Feature: Framework
 * Function: Test HStreamMetadata & HStreamCommon
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamMetadata & HStreamCommon
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_027, TestSize.Level0)
{
    int32_t format = 0;
    CameraInfoDumper infoDumper(0);
    std::string  dumpString ="HStreamMetadata";
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    sptr<HStreamMetadata> streamMetadata= new(std::nothrow) HStreamMetadata(producer, format, {1});
    ASSERT_NE(streamMetadata, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata1 = nullptr;
    streamMetadata->LinkInput(mockStreamOperator, metadata);
    streamMetadata->LinkInput(mockStreamOperator, metadata1);
    mockStreamOperator = nullptr;
    streamMetadata->LinkInput(mockStreamOperator, metadata);
    streamMetadata->Stop();
    streamMetadata->Start();
    streamMetadata->DumpStreamInfo(infoDumper);
    streamMetadata->Start();
    streamMetadata->Stop();
}

/*
 * Feature: Framework
 * Function: Test HStreamMetadata & HStreamCommon
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamMetadata & HStreamCommon
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_028, TestSize.Level0)
{
    int32_t format = 0;
    std::string  dumpString ="HStreamMetadata";
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    sptr<HStreamMetadata> streamMetadata= new(std::nothrow) HStreamMetadata(producer, format, {1});
    ASSERT_NE(streamMetadata, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    streamMetadata->Start();
    streamMetadata->LinkInput(mockStreamOperator, metadata);
    streamMetadata->Stop();
}

/*
 * Feature: Framework
 * Function: Test HStreamCapture & HStreamCommon
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamCapture & HStreamCommon
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_029, TestSize.Level0)
{
    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t captureId = 0;
    int32_t frameCount = 0;
    uint64_t timestamp = 0;
    CameraInfoDumper infoDumper(0);
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<IStreamCaptureCallback> callback = nullptr;
    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    sptr<OHOS::IBufferProducer> producer1 = nullptr;
    sptr<HStreamCapture> streamCapture= new(std::nothrow) HStreamCapture(producer, format, width, height);
    ASSERT_NE(streamCapture, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    EXPECT_EQ(streamCapture->SetThumbnail(false, producer1), CAMERA_OK);
    EXPECT_EQ(streamCapture->SetThumbnail(false, producer), CAMERA_OK);
    EXPECT_EQ(streamCapture->SetThumbnail(true, producer1), CAMERA_OK);
    streamCapture->DumpStreamInfo(infoDumper);
    EXPECT_EQ(streamCapture->SetThumbnail(true, producer), CAMERA_OK);
    streamCapture->SetRotation(metadata, captureId);
    mockStreamOperator = nullptr;
    streamCapture->LinkInput(mockStreamOperator, metadata);
    EXPECT_EQ(streamCapture->Capture(metadata),  CAMERA_INVALID_STATE);
    EXPECT_EQ(streamCapture->SetCallback(callback), CAMERA_INVALID_ARG);
    EXPECT_EQ(streamCapture->OnCaptureEnded(captureId, frameCount), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnCaptureError(captureId, frameCount), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnCaptureError(captureId, BUFFER_LOST), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnFrameShutter(captureId, timestamp), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnFrameShutterEnd(captureId, timestamp), CAMERA_OK);
    EXPECT_EQ(streamCapture->OnCaptureReady(captureId, timestamp), CAMERA_OK);
    streamCapture->DumpStreamInfo(infoDumper);

    EXPECT_EQ(streamCapture->Release(),  0);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_030, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    uint32_t callerToken1 = 3;
    sptr<HCameraDevice> camDevice = new(std::nothrow) HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    sptr<HCameraDevice> camDevice1 = new(std::nothrow) HCameraDevice(cameraHostManager, cameraId, callerToken1);
    ASSERT_NE(camDevice1, nullptr);

    sptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator = nullptr;
    sptr<CameraInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<ICameraDeviceServiceCallback> callback1 = new(std::nothrow) CameraDeviceServiceCallback(input);
    ASSERT_NE(callback1, nullptr);
    camDevice->EnableResult(result);
    camDevice->DisableResult(result);

    int32_t ret = camDevice->Open();
    EXPECT_EQ(ret, 0);
    camDevice->Open();
    g_getCameraAbilityerror = true;
    camDevice->GetDeviceAbility();
    g_openCameraDevicerror = true;
    EXPECT_EQ(camDevice->Open(), 0);

    camDevice->Close();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraHostManager with anomalous branch.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_031, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    std::vector<std::string> cameraIds = {};
    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    cameraHostManager->Init();
    ASSERT_NE(cameraHostManager, nullptr);

    std::string cameraId = cameras[0]->GetID();
    EXPECT_EQ(cameraHostManager->HCameraHostManager::GetCameras(cameraIds), 0);
    std::shared_ptr<OHOS::Camera::CameraMetadata> ability = cameras[0]->GetMetadata();
    EXPECT_EQ(cameraHostManager->HCameraHostManager::GetCameraAbility(cameraId, ability), 2);
    EXPECT_EQ(cameraHostManager->HCameraHostManager::SetFlashlight(cameraId, false), 2);

    cameraId = "HCameraHostManager";

    cameraHostManager->AddCameraDevice(cameraId, nullptr);
    EXPECT_EQ(cameraHostManager->HCameraHostManager::SetFlashlight(cameraId, true), 2);

    cameraHostManager->CloseCameraDevice(cameraId);

    ability = cameras[0]->GetMetadata();
    EXPECT_EQ(cameraHostManager->HCameraHostManager::GetCameraAbility(cameraId, ability), 2);

    EXPECT_EQ(cameraHostManager->HCameraHostManager::GetVersionByCamera(cameraId), 0);

    sptr<OHOS::HDI::Camera::V1_0::ICameraDevice> pDevice;
    cameraHostManager->HCameraHostManager::OpenCameraDevice(cameraId, nullptr, pDevice);

    cameraId = cameras[0]->GetID();
    HDI::ServiceManager::V1_0::ServiceStatus status;
    status.deviceClass = DEVICE_CLASS_CAMERA;
    status.serviceName = "distributed_camera_service";
    status.status = HDI::ServiceManager::V1_0::SERVIE_STATUS_START;
    cameraHostManager->GetRegisterServStatListener()->OnReceive(status);

    EXPECT_EQ(cameraHostManager->HCameraHostManager::SetFlashlight(cameraId, false), 2);

    std::shared_ptr<MockHCameraHostManager::MockStatusCallback> mockStatusCallback =
    std::make_shared<MockHCameraHostManager::MockStatusCallback>();
    sptr<MockHCameraHostManager> mockCameraHostManager_2 =
        new (std::nothrow) MockHCameraHostManager(mockStatusCallback);
    ASSERT_NE(mockCameraHostManager_2, nullptr);
    sptr<HCameraHostManager> cameraHostManager_2 = (sptr<HCameraHostManager> &)mockCameraHostManager_2;

    cameraHostManager_2->AddCameraDevice(cameraId, nullptr);
    cameraHostManager->DeInit();
    input->Close();
}


/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in AttachSketchSurface,
 *      IsSketchSupported, StartSketch, StopSketch, OnSketchStatusChanged,
 *      CreateSketchWrapper, GetSketchRatio
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged() in AttachSketchSurface,
 *          IsSketchSupported, StartSketch, StopSketch, OnSketchStatusChanged,
 *          CreateSketchWrapper, GetSketchRatio
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_032, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    Size previewSize = {.width = 1440, .height = 1080};

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    EXPECT_EQ(previewOutput->IsSketchSupported(), false);
    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(previewOutput->AttachSketchSurface(surface), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(previewOutput->StartSketch(), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(previewOutput->StopSketch(), CameraErrorCode::SESSION_NOT_CONFIG);

    EXPECT_EQ(previewOutput->OnSketchStatusChanged(SketchStatus::STOPED), CAMERA_INVALID_STATE);
    EXPECT_EQ(previewOutput->IsSketchSupported(), false);

    EXPECT_NE(previewOutput->CreateSketchWrapper(previewSize), 0);
    EXPECT_EQ(previewOutput->GetSketchRatio(), -1);

    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);
    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    EXPECT_EQ(session->RemoveOutput(preview), 0);
    EXPECT_EQ(session->RemoveInput(input), 0);
    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in IsSketchSupported,
 *              FindSketchSize, EnableSketch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged() in IsSketchSupported,
 *              FindSketchSize, EnableSketch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_033, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;
    EXPECT_EQ(previewOutput->IsSketchSupported(), false);
    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);
    ret = session->AddInput(input);
    EXPECT_EQ(ret, 0);
    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    previewOutput->SetSession(session);
    previewOutput->FindSketchSize();
    EXPECT_EQ(previewOutput->EnableSketch(true), 7400102);
    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in IsSketchSupported,
 *              OnNativeRegisterCallback, OnNativeUnregisterCallback, CameraServerDied, SetCallback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged() in IsSketchSupported,
 *              OnNativeRegisterCallback, OnNativeUnregisterCallback, CameraServerDied, SetCallback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_034, TestSize.Level0)
{
    InSequence s;
    Size previewSize = {.width = 1440, .height = 1080};
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    EXPECT_EQ(previewOutput->IsSketchSupported(), false);
    std::string eventString = "test";
    previewOutput->OnNativeRegisterCallback(eventString);
    previewOutput->OnNativeUnregisterCallback(eventString);
    eventString = "sketchStatusChanged";
    previewOutput->sketchWrapper_ = std::make_shared<SketchWrapper>(previewOutput->GetStream(), previewSize);
    previewOutput->OnNativeRegisterCallback(eventString);
    previewOutput->OnNativeUnregisterCallback(eventString);
    previewOutput->CameraServerDied(0);
    std::shared_ptr<PreviewStateCallback> previewStateCallback =
        std::make_shared<TestPreviewOutputCallback>("PreviewStateCallback");
    previewOutput->SetCallback(previewStateCallback);
    previewOutput->CameraServerDied(0);

    EXPECT_EQ(session->Release(), 0);
    EXPECT_EQ(input->Close(), 0);
}

/*
 * Feature: Framework
 * Function: Test Init, AttachSketchSurface, UpdateSketchRatio,
 *          SetPreviewStateCallback, OnSketchStatusChanged,
 *          UpdateSketchReferenceFovRatio in sketchWrapper
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test Init, AttachSketchSurface, UpdateSketchRatio,
 *          SetPreviewStateCallback, OnSketchStatusChanged,
 *          UpdateSketchReferenceFovRatio in sketchWrapper
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_035, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    Size previewSize = { .width = 1440, .height = 1080 };
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceMetadata = cameras[0]->GetMetadata();
    ASSERT_NE(deviceMetadata, nullptr);
    auto modeName = session->GetFeaturesMode();
    CameraFormat previewFormat = CAMERA_FORMAT_YCBCR_420_888;
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    SketchWrapper* sketchWrapper = new (std::nothrow) SketchWrapper(previewOutput->GetStream(), previewSize);
    ASSERT_NE(sketchWrapper, nullptr);
    std::shared_ptr<PreviewStateCallback> setCallback =
        std::make_shared<TestPreviewOutputCallback>("PreviewStateCallback");
    ASSERT_NE(setCallback, nullptr);
    camera_metadata_item_t item;
    OHOS::Camera::FindCameraMetadataItem(deviceMetadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    SceneFeaturesMode defaultSceneFeature(SceneMode::NORMAL, {});
    SceneFeaturesMode captureSceneFeature(SceneMode::CAPTURE, {});
    EXPECT_EQ(sketchWrapper->Init(deviceMetadata, modeName), 0);
    EXPECT_EQ(sketchWrapper->AttachSketchSurface(surface), 10);
    EXPECT_EQ(sketchWrapper->UpdateSketchRatio(1.0f), 0);
    sketchWrapper->SetPreviewStateCallback(setCallback);
    sketchWrapper->OnSketchStatusChanged(defaultSceneFeature);
    sketchWrapper->OnSketchStatusChanged(SketchStatus::STARTED, defaultSceneFeature);
    sketchWrapper->OnSketchStatusChanged(SketchStatus::STOPED, defaultSceneFeature);
    sketchWrapper->currentSketchStatusData_.sketchRatio = 1.0f;
    sketchWrapper->OnSketchStatusChanged(modeName);
    sketchWrapper->OnSketchStatusChanged(SketchStatus::STOPED, captureSceneFeature);
    sketchWrapper->UpdateSketchReferenceFovRatio(item);

    EXPECT_EQ(session->Release(), 0);
    EXPECT_EQ(input->Close(), 0);
}

/*
 * Feature: Framework
 * Function: Test AttachSketchSurface, StartSketch, StopSketch,
 *              OnNativeRegisterCallback in ~PreviewOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test AttachSketchSurface, StartSketch,
 *              StopSketch, OnNativeRegisterCallback in ~PreviewOutput
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_036, TestSize.Level0)
{
    InSequence s;
    Size previewSize = {.width = 1440, .height = 1080};
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    int32_t ret = session->BeginConfig();
    ret = session->AddInput(input);
    ret = session->AddOutput(preview);
    EXPECT_EQ(ret, 0);

    ret = session->CommitConfig();
    EXPECT_EQ(ret, 0);

    previewOutput->Release();
    previewOutput->SetSession(session);
    EXPECT_EQ(previewOutput->AttachSketchSurface(surface), INVALID_ARGUMENT);
    EXPECT_EQ(previewOutput->StartSketch(), 7400201);
    EXPECT_EQ(previewOutput->StopSketch(), 7400201);
    previewOutput->sketchWrapper_ = std::make_shared<SketchWrapper>(previewOutput->GetStream(), previewSize);
    previewOutput->OnNativeRegisterCallback("sketchStatusChanged");

    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in
 *              OnMetadataChanged, OnSketchStatusChanged
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged()
 *              in OnMetadataChanged, OnSketchStatusChanged
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_037, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    Size previewSize = {.width = 1440, .height = 1080};
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceMetadata = cameras[0]->GetMetadata();
    ASSERT_NE(deviceMetadata, nullptr);
    camera_metadata_item_t item;
    OHOS::Camera::FindCameraMetadataItem(deviceMetadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    EXPECT_EQ(session->BeginConfig(), 0);
    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);

    item.count = 0;
    EXPECT_EQ(previewOutput->OnControlMetadataChanged(OHOS_ABILITY_BEAUTY_FACE_SLENDER_VALUES, item),
        CAM_META_INVALID_PARAM);
    item.count = 1;
    EXPECT_EQ(previewOutput->OnControlMetadataChanged(OHOS_ABILITY_BEAUTY_FACE_SLENDER_VALUES, item),
        CAM_META_FAILURE);
    previewOutput->sketchWrapper_ = std::make_shared<SketchWrapper>(previewOutput->GetStream(), previewSize);

    previewOutput->SetSession(nullptr);
    EXPECT_EQ(previewOutput->OnSketchStatusChanged(SketchStatus::STOPED), CAMERA_INVALID_STATE);
    EXPECT_EQ(previewOutput->OnControlMetadataChanged(OHOS_ABILITY_BEAUTY_FACE_SLENDER_VALUES, item),
        CAM_META_FAILURE);
    previewOutput->SetSession(session);
    EXPECT_EQ(previewOutput->OnControlMetadataChanged(OHOS_ABILITY_BEAUTY_FACE_SLENDER_VALUES, item),
        CAM_META_SUCCESS);
    EXPECT_EQ(previewOutput->OnSketchStatusChanged(SketchStatus::STOPED), CAMERA_OK);

    EXPECT_EQ(session->CommitConfig(), 0);
    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test cameraFormat with CAMERA_FORMAT_YUV_420_SP and CAMERA_FORMAT_JPEG
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameraFormat with CAMERA_FORMAT_YUV_420_SP and CAMERA_FORMAT_JPEG
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_038, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    Size previewSize = {.width = 1440, .height = 1080};
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceMetadata = cameras[0]->GetMetadata();
    ASSERT_NE(deviceMetadata, nullptr);
    camera_metadata_item_t item;
    OHOS::Camera::FindCameraMetadataItem(deviceMetadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    CameraFormat previewFormat = CAMERA_FORMAT_YCBCR_420_888;
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;
    EXPECT_EQ(previewOutput->IsSketchSupported(), false);
    auto currentPreviewProfile = previewOutput->GetPreviewProfile();
    currentPreviewProfile->format_ = CAMERA_FORMAT_YUV_420_SP;
    EXPECT_EQ(previewOutput->IsSketchSupported(), false);
    currentPreviewProfile->format_ = CAMERA_FORMAT_JPEG;
    EXPECT_EQ(previewOutput->IsSketchSupported(), false);

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in
 *          AttachSketchSurface, StartSketch, StopSketch, OnNativeRegisterCallback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged()
 *          in AttachSketchSurface, StartSketch, StopSketch, OnNativeRegisterCallback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_039, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);
    Size previewSize = {.width = 1440, .height = 1080};

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);
    EXPECT_EQ(session->CommitConfig(), 0);
    previewOutput->SetSession(session);

    EXPECT_EQ(previewOutput->AttachSketchSurface(surface), INVALID_ARGUMENT);
    EXPECT_EQ(previewOutput->StartSketch(), 7400201);
    EXPECT_EQ(previewOutput->StopSketch(), 7400201);
    previewOutput->sketchWrapper_ = std::make_shared<SketchWrapper>(previewOutput->GetStream(), previewSize);
    EXPECT_EQ(previewOutput->AttachSketchSurface(surface), 7400201);
    EXPECT_EQ(previewOutput->AttachSketchSurface(nullptr), CameraErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(previewOutput->StartSketch(), 7400201);
    EXPECT_EQ(previewOutput->StopSketch(), 7400201);
    previewOutput->OnNativeRegisterCallback("sketchStatusChanged");

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test CameraServerDied
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test if innerCaptureSession_ == nullptr in CameraServerDied
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_040, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto appCallback = std::make_shared<AppSessionCallback>();
    ASSERT_NE(appCallback, nullptr);

    EXPECT_EQ(session->BeginConfig(), 0);
    session->CameraServerDied(0);
    session->appCallback_ = appCallback;
    session->CameraServerDied(0);
    session->innerCaptureSession_ = nullptr;
    session->CameraServerDied(0);
    session->appCallback_ = nullptr;

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in
 *              PrepareZoom, UnPrepareZoom, SetSmoothZoom
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged()
 *              in PrepareZoom, UnPrepareZoom, SetSmoothZoom
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_041, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    EXPECT_EQ(session->PrepareZoom(), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(session->UnPrepareZoom(), CameraErrorCode::SESSION_NOT_CONFIG);

    EXPECT_EQ(session->BeginConfig(), 0);
    EXPECT_EQ(session->PrepareZoom(), CameraErrorCode::SUCCESS);
    EXPECT_EQ(session->UnPrepareZoom(), CameraErrorCode::SUCCESS);

    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);

    session->SetSmoothZoom(0, 0);

    EXPECT_EQ(session->CommitConfig(), 0);
    EXPECT_EQ(session->PrepareZoom(), CameraErrorCode::SUCCESS);
    EXPECT_EQ(session->UnPrepareZoom(), CameraErrorCode::SUCCESS);
    EXPECT_EQ(session->SetSmoothZoom(0, 0), CameraErrorCode::SUCCESS);

    session->LockForControl();
    session->PrepareZoom();
    session->UnPrepareZoom();
    session->UnlockForControl();

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in
 *          PrepareZoom, UnPrepareZoom, SetSmoothZoom,
 *          SetBeauty of ~CaptureSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged()
 *          in PrepareZoom, UnPrepareZoom, SetSmoothZoom,
 *          SetBeauty of ~CaptureSession
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_042, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    EXPECT_EQ(session->BeginConfig(), 0);
    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);

    EXPECT_EQ(session->CommitConfig(), 0);
    session->Release();
    EXPECT_EQ(session->PrepareZoom(), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(session->UnPrepareZoom(), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(session->SetSmoothZoom(0, 0), CameraErrorCode::SESSION_NOT_CONFIG);
    session->SetBeauty(AUTO_TYPE, 0);

    EXPECT_EQ(input->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in
 *              SetBeautyValue, SetBeauty, CalculateExposureValue, SetFilter
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged()
 *              in SetBeautyValue, SetBeauty, CalculateExposureValue, SetFilter
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_043, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    EXPECT_EQ(session->SetBeautyValue(AUTO_TYPE, 0), false);

    session->SetBeauty(AUTO_TYPE, 0);
    EXPECT_EQ(session->BeginConfig(), 0);
    session->SetBeauty(AUTO_TYPE, 0);
    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);

    EXPECT_EQ(session->CommitConfig(), 0);
    session->SetBeauty(AUTO_TYPE, 0);

    EXPECT_EQ(session->CalculateExposureValue(100.0), 2147483647);

    session->LockForControl();
    session->SetBeauty(AUTO_TYPE, 0);
    session->SetFilter(NONE);
    session->UnlockForControl();

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test innerInputDevice_ in GetColorEffect,
 *          EnableMacro, IsMacroSupported,
 *          SetColorEffect, ProcessMacroStatusChange
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test innerInputDevice_ in GetColorEffect,
 *          EnableMacro, IsMacroSupported,
 *          SetColorEffect, ProcessMacroStatusChange
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_044, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto macroStatusCallback = std::make_shared<AppMacroStatusCallback>();
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();

    EXPECT_EQ(session->GetColorEffect(), COLOR_EFFECT_NORMAL);
    EXPECT_EQ(session->EnableMacro(true), OPERATION_NOT_ALLOWED);

    EXPECT_EQ(session->BeginConfig(), 0);

    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);

    EXPECT_EQ(session->CommitConfig(), 0);
    EXPECT_EQ(session->GetColorEffect(), COLOR_EFFECT_NORMAL);

    ((sptr<CameraInput>&)(session->innerInputDevice_))->cameraObj_ = nullptr;
    EXPECT_EQ(session->GetColorEffect(), COLOR_EFFECT_NORMAL);
    EXPECT_EQ(session->IsMacroSupported(), false);
    session->innerInputDevice_ = nullptr;
    EXPECT_EQ(session->GetColorEffect(), COLOR_EFFECT_NORMAL);
    EXPECT_EQ(session->IsMacroSupported(), false);
    EXPECT_EQ(session->EnableMacro(true), OPERATION_NOT_ALLOWED);

    session->LockForControl();
    session->SetColorEffect(COLOR_EFFECT_NORMAL);
    session->UnlockForControl();

    session->macroStatusCallback_ = macroStatusCallback;
    session->ProcessMacroStatusChange(metadata);

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test HCameraService in UpdateSkinSmoothSetting,
 *              UpdateFaceSlenderSetting, UpdateSkinToneSetting
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraService in UpdateSkinSmoothSetting,
 *              UpdateFaceSlenderSetting, UpdateSkinToneSetting
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_045, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    auto hCameraService = new HCameraService(0, true);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();

    EXPECT_EQ(hCameraService->UpdateSkinSmoothSetting(metadata, 0), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateSkinSmoothSetting(metadata, 1), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateSkinSmoothSetting(nullptr, 1), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateFaceSlenderSetting(metadata, 0), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateFaceSlenderSetting(metadata, 1), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateFaceSlenderSetting(nullptr, 1), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateSkinToneSetting(metadata, 0), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateSkinToneSetting(metadata, 1), CAMERA_OK);
    EXPECT_EQ(hCameraService->UpdateSkinToneSetting(nullptr, 1), CAMERA_OK);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionConfiged() || input == nullptr in CanAddInput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionConfiged() || input == nullptr in CanAddInput
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_046, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    sptr<CaptureInput> input1 = nullptr;
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    EXPECT_EQ(session->CanAddInput(input), false);
    EXPECT_EQ(session->BeginConfig(), 0);
    EXPECT_EQ(session->CanAddInput(input), true);
    EXPECT_EQ(session->CanAddInput(input1), false);
    session->innerCaptureSession_ = nullptr;
    EXPECT_EQ(session->CanAddInput(input), false);

    EXPECT_EQ(session->AddInput(input), OPERATION_NOT_ALLOWED);
    EXPECT_EQ(session->AddOutput(preview), OPERATION_NOT_ALLOWED);

    EXPECT_EQ(session->CommitConfig(), OPERATION_NOT_ALLOWED);

    EXPECT_EQ(session->RemoveOutput(preview), OPERATION_NOT_ALLOWED);
    EXPECT_EQ(session->RemoveInput(input), OPERATION_NOT_ALLOWED);
    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    session->Release();
}

/*
 * Feature: Framework
 * Function: Test !IsSessionConfiged() || output == nullptr in CanAddOutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionConfiged() || output == nullptr in CanAddOutput
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_047, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();

    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    sptr<CaptureOutput> output = nullptr;
    ASSERT_NE(preview, nullptr);

    EXPECT_EQ(session->CanAddOutput(preview), false);
    EXPECT_EQ(session->BeginConfig(), 0);
    EXPECT_EQ(session->CanAddOutput(output), false);
    preview->Release();
    EXPECT_EQ(session->CanAddOutput(preview), false);

    EXPECT_EQ(input->Close(), 0);
}

/*
 * Feature: Framework
 * Function: Test VerifyAbility, SetBeauty
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test VerifyAbility, SetBeauty
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_048, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    EXPECT_EQ(session->BeginConfig(), 0);

    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);

    EXPECT_EQ(session->VerifyAbility(static_cast<uint32_t>(OHOS_ABILITY_SCENE_FILTER_TYPES)), CAMERA_INVALID_ARG);
    session->LockForControl();
    session->SetBeauty(FACE_SLENDER, 3);
    session->UnlockForControl();

    EXPECT_EQ(session->CommitConfig(), 0);

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in
 *          AttachSketchSurface, CreateSketchWrapper，
 *          GetSketchRatio, GetDeviceMetadata, OnSketchStatusChanged, StartSketch, StopSketch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged()
 *          in AttachSketchSurface, CreateSketchWrapper， GetSketchRatio,
 *          GetDeviceMetadata, OnSketchStatusChanged, StartSketch, StopSketch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_049, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    Size previewSize = {.width = 1440, .height = 1080};

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    EXPECT_EQ(session->BeginConfig(), 0);
    EXPECT_EQ(session->AddInput(input), 0);
    EXPECT_EQ(session->AddOutput(preview), 0);

    previewOutput->SetSession(session);
    EXPECT_EQ(previewOutput->AttachSketchSurface(surface), CameraErrorCode::SESSION_NOT_CONFIG);
    EXPECT_EQ(previewOutput->CreateSketchWrapper(previewSize), 0);
    EXPECT_EQ(previewOutput->GetSketchRatio(), -1.0);
    EXPECT_NE(previewOutput->GetDeviceMetadata(), nullptr);
    previewOutput->FindSketchSize();
    previewOutput->OnSketchStatusChanged(SketchStatus::STOPED);
    EXPECT_EQ(previewOutput->StartSketch(), SESSION_NOT_CONFIG);
    EXPECT_EQ(previewOutput->StopSketch(), SESSION_NOT_CONFIG);

    EXPECT_EQ(session->CommitConfig(), 0);
    EXPECT_EQ(previewOutput->StartSketch(), SERVICE_FATL_ERROR);
    EXPECT_EQ(previewOutput->StopSketch(), SERVICE_FATL_ERROR);

    previewOutput->sketchWrapper_ = std::make_shared<SketchWrapper>(previewOutput->GetStream(), previewSize);
    EXPECT_EQ(previewOutput->StartSketch(), SERVICE_FATL_ERROR);
    EXPECT_EQ(previewOutput->StopSketch(), SERVICE_FATL_ERROR);
    EXPECT_EQ(previewOutput->AttachSketchSurface(surface), SERVICE_FATL_ERROR); // Need EnableSketch

    EXPECT_EQ(session->RemoveOutput(preview), OPERATION_NOT_ALLOWED);
    EXPECT_EQ(session->RemoveInput(input), OPERATION_NOT_ALLOWED);
    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test !IsSessionCommited() && !IsSessionConfiged() in OnNativeRegisterCallback,
 *          OnNativeUnregisterCallback, FindSketchSize
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test !IsSessionCommited() && !IsSessionConfiged() in
 *          OnNativeRegisterCallback, OnNativeUnregisterCallback, FindSketchSize
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_050, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    EXPECT_EQ(session->BeginConfig(), 0);
    EXPECT_EQ(session->AddInput(input), 0);

    EXPECT_EQ(session->AddOutput(preview), 0);

    previewOutput->SetSession(session);
    previewOutput->FindSketchSize();

    std::string eventString = "test";
    previewOutput->OnNativeRegisterCallback(eventString);
    previewOutput->OnNativeUnregisterCallback(eventString);
    eventString = "sketchStatusChanged";
    previewOutput->OnNativeRegisterCallback(eventString);
    previewOutput->OnNativeUnregisterCallback(eventString);

    EXPECT_EQ(session->CommitConfig(), 0);

    eventString = "test";
    previewOutput->OnNativeRegisterCallback(eventString);
    previewOutput->OnNativeUnregisterCallback(eventString);
    eventString = "sketchStatusChanged";
    previewOutput->OnNativeRegisterCallback(eventString);
    previewOutput->OnNativeUnregisterCallback(eventString);

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test GetSketchReferenceFovRatio, UpdateSketchReferenceFovRatio
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test GetSketchReferenceFovRatio, UpdateSketchReferenceFovRatio
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_051, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    Size previewSize = { .width = 1440, .height = 1080 };
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceMetadata = cameras[0]->GetMetadata();
    ASSERT_NE(deviceMetadata, nullptr);
    session->GetFeaturesMode();
    CameraFormat previewFormat = CAMERA_FORMAT_YCBCR_420_888;
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> preview = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(preview, nullptr);
    auto previewOutput = (sptr<PreviewOutput>&)preview;

    SketchWrapper* sketchWrapper = new (std::nothrow) SketchWrapper(previewOutput->GetStream(), previewSize);
    ASSERT_NE(sketchWrapper, nullptr);
    std::shared_ptr<PreviewStateCallback> setCallback =
        std::make_shared<TestPreviewOutputCallback>("PreviewStateCallback");
    ASSERT_NE(setCallback, nullptr);
    camera_metadata_item_t item;
    OHOS::Camera::FindCameraMetadataItem(deviceMetadata->get(), OHOS_CONTROL_ZOOM_RATIO, &item);

    auto sketchReferenceFovRangeVec = std::vector<SketchReferenceFovRange>(5);
    SketchReferenceFovRange sketchReferenceFovRange = { .zoomMin = -1.0f, .zoomMax = -1.0f, .referenceValue = -1.0f };
    sketchReferenceFovRangeVec[0] = sketchReferenceFovRange;
    sketchReferenceFovRange = { .zoomMin = -1.0f, .zoomMax = 100.0f, .referenceValue = -1.0f };
    SceneFeaturesMode illegalFeaturesMode(static_cast<SceneMode>(-1), {});
    SketchWrapper::g_sketchReferenceFovRatioMap_[illegalFeaturesMode] = sketchReferenceFovRangeVec;
    EXPECT_EQ(sketchWrapper->GetSketchReferenceFovRatio(illegalFeaturesMode, -1.0f), -1.0f);
    sketchReferenceFovRangeVec[1] = sketchReferenceFovRange;
    sketchReferenceFovRange = { .zoomMin = 100.0f, .zoomMax = 1.0f, .referenceValue = -1.0f };
    sketchReferenceFovRangeVec[2] = sketchReferenceFovRange;
    sketchReferenceFovRange = { .zoomMin = 100.0f, .zoomMax = 100.0f, .referenceValue = -1.0f };
    sketchReferenceFovRangeVec[3] = sketchReferenceFovRange;
    sketchReferenceFovRange = { .zoomMin = 100.0f, .zoomMax = 200.0f, .referenceValue = -1.0f };
    sketchReferenceFovRangeVec[4] = sketchReferenceFovRange;

    SceneFeaturesMode illegalFeaturesMode2(static_cast<SceneMode>(100), {});
    SketchWrapper::g_sketchReferenceFovRatioMap_[illegalFeaturesMode2] = sketchReferenceFovRangeVec;
    EXPECT_EQ(sketchWrapper->GetSketchReferenceFovRatio(illegalFeaturesMode2, -1.0f), -1.0f);
    EXPECT_EQ(sketchWrapper->GetSketchReferenceFovRatio(illegalFeaturesMode2, 200.0f), -1.0f);
    EXPECT_EQ(sketchWrapper->GetSketchReferenceFovRatio(illegalFeaturesMode2, 100.0f), -1.0f);
    sketchWrapper->UpdateSketchReferenceFovRatio(item);

    EXPECT_EQ(preview->Release(), 0);
    EXPECT_EQ(input->Release(), 0);
    EXPECT_EQ(session->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test cameramanager with updatetorchmode
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameramanager with updatetorchmode
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_052, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], 0);

    SceneMode mode = PORTRAIT;
    cameraManager->SetServiceProxy(nullptr);
    cameraManager->CreateCaptureSession(mode);
    cameraManager->ClearCameraDeviceListCache();

    TorchMode mode1 = TorchMode::TORCH_MODE_OFF;
    TorchMode mode2 = TorchMode::TORCH_MODE_ON;
    cameraManager->torchMode_ = mode1;
    cameraManager->UpdateTorchMode(mode1);
    cameraManager->UpdateTorchMode(mode2);
}

/*
 * Feature: Framework
 * Function: Test cameramanager with parsebasiccapability
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameramanager with parsebasiccapability
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_053, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], 0);

    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    camera_metadata_item_t item;
    OHOS::Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    CameraManager::ProfilesWrapper wrapper = {};
    cameraManager->ParseBasicCapability(wrapper, metadata, item);
}

/*
 * Feature: Framework
 * Function: Test cameramanager with no cameraid and cameraobjlist
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameramanager with no cameraid and cameraobjlist
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_054, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], 0);

    pid_t pid = 0;
    cameraManager->CameraServerDied(pid);
    sptr<ICameraServiceCallback> cameraServiceCallback = nullptr;
    cameraManager->SetCameraServiceCallback(cameraServiceCallback);
    sptr<ITorchServiceCallback> torchServiceCallback = nullptr;
    cameraManager->SetTorchServiceCallback(torchServiceCallback);
    sptr<ICameraMuteServiceCallback> cameraMuteServiceCallback = nullptr;
    cameraManager->SetCameraMuteServiceCallback(cameraMuteServiceCallback);
    float level = 9.2;
    int32_t ret = cameraManager->SetTorchLevel(level);
    EXPECT_EQ(ret, 7400201);
    string cameraId = "";
    int activeTime = 0;
    EffectParam effectParam = {0, 0, 0};
    ret = cameraManager->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::TRANSIENT_ACTIVE_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(ret, 7400201);
    cameraManager->cameraDeviceList_ = {};
    bool isTorchSupported = cameraManager->IsTorchSupported();
    EXPECT_EQ(isTorchSupported, false);
}

/*
 * Feature: Framework
 * Function: Test cameramanager with serviceProxy_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameramanager with serviceProxy_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_055, TestSize.Level0)
{
    sptr<ICameraServiceCallback> callback = new(std::nothrow) CameraStatusServiceCallback(cameraManager);
    ASSERT_NE(callback, nullptr);
    cameraManager->cameraSvcCallback_  = callback;

    pid_t pid = 0;
    cameraManager->SetServiceProxy(nullptr);
    cameraManager->CameraServerDied(pid);
    sptr<ICameraServiceCallback> cameraServiceCallback = nullptr;
    cameraManager->SetCameraServiceCallback(cameraServiceCallback);
    sptr<ITorchServiceCallback> torchServiceCallback = nullptr;
    cameraManager->SetTorchServiceCallback(torchServiceCallback);
    sptr<ICameraMuteServiceCallback> cameraMuteServiceCallback = nullptr;
    cameraManager->SetCameraMuteServiceCallback(cameraMuteServiceCallback);

    cameraManager->cameraDeviceList_ = {};
    string cameraId = "";
    cameraManager->GetCameraDeviceFromId(cameraId);
    bool isTorchSupported = cameraManager->IsTorchSupported();
    EXPECT_EQ(isTorchSupported, false);
}

/*
 * Feature: Framework
 * Function: Test cameramanager with preswitchcamera
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameramanager with preswitchcamera
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_056, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], 0);

    std::string cameraId = cameras[0]->GetID();
    int32_t ret = cameraManager->PreSwitchCamera(cameraId);
    EXPECT_EQ(ret, 7400201);

    cameraManager->SetServiceProxy(nullptr);
    cameraManager->PreSwitchCamera(cameraId);
    EXPECT_EQ(ret, 7400201);
}

/*
 * Feature: Framework
 * Function: Test cameradevice with position and zoomratiorange
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameradevice with position and zoomratiorange
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_057, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    cameras[0]->foldScreenType_ = CAMERA_FOLDSCREEN_INNER;
    cameras[0]->cameraPosition_ = CAMERA_POSITION_FRONT;
    cameras[0]->GetPosition();

    cameras[0]->zoomRatioRange_ = {1.1, 2.1};
    EXPECT_EQ(cameras[0]->GetZoomRatioRange(), cameras[0]->zoomRatioRange_);
}

/*
 * Feature: Framework
 * Function: Test camerainput with cameraserverdied
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test camerainput with cameraserverdied
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_058, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;

    pid_t pid = 0;
    camInput->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test camerainput with deviceObj_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test camerainput with deviceObj_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_059, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;

    pid_t pid = 0;
    std::shared_ptr<TestDeviceCallback> setCallback = std::make_shared<TestDeviceCallback>("InputCallback");
    camInput->deviceObj_ = nullptr;
    camInput->SetErrorCallback(setCallback);
    camInput->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test metadataoutput with cameraserverdied and stop
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput with cameraserverdied and stop
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_060, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureOutput> output = cameraManager->CreateMetadataOutput();
    ASSERT_NE(output, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)output;

    pid_t pid = 0;
    metadatOutput->CameraServerDied(pid);
    EXPECT_EQ(metadatOutput->Stop(), CameraErrorCode::SERVICE_FATL_ERROR);
}

/*
 * Feature: Framework
 * Function: Test metadataoutput with stream_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput with stream_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_061, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> output = cameraManager->CreateMetadataOutput();
    ASSERT_NE(output, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)output;

    pid_t pid = 0;
    metadatOutput->stream_ = nullptr;
    metadatOutput->CameraServerDied(pid);
    EXPECT_EQ(metadatOutput->Stop(), CameraErrorCode::SERVICE_FATL_ERROR);
    EXPECT_EQ(metadatOutput->Release(), CameraErrorCode::SERVICE_FATL_ERROR);
}

/*
 * Feature: Framework
 * Function: Test metadataoutput with surface_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput with surface_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_062, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> output = cameraManager->CreateMetadataOutput();
    ASSERT_NE(output, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)output;

    metadatOutput->surface_ = nullptr;
    EXPECT_EQ(metadatOutput->Release(), 0);
}

/*
 * Feature: Framework
 * Function: Test metadataoutput with start when session_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput with start when session_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_063, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)metadata;

    metadatOutput->session_ = nullptr;
    EXPECT_EQ(metadatOutput->Start(), CameraErrorCode::SUCCESS);
}

/*
 * Feature: Framework
 * Function: Test metadataoutput with start when not commit
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput with start when not commit
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_064, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)metadata;

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();
    session->AddInput(input);
    session->AddOutput(metadata);

    EXPECT_EQ(metadatOutput->Start(), CameraErrorCode::SUCCESS);

    session->CommitConfig();
    session->Start();

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test metadataoutput with start when stream_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput with start when stream_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_065, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)metadata;

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();

    session->AddInput(input);
    session->AddOutput(metadata);

    session->CommitConfig();
    session->Start();

    metadatOutput->stream_ = nullptr;
    EXPECT_EQ(metadatOutput->Start(), CameraErrorCode::SUCCESS);

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test metadataoutput with start
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput with start
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_066, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> metadata = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadata, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)metadata;

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    session->BeginConfig();

    session->AddInput(input);
    session->AddOutput(metadata);

    session->CommitConfig();
    session->Start();

    EXPECT_EQ(metadatOutput->Start(), CameraErrorCode::SUCCESS);
    EXPECT_EQ(metadatOutput->Stop(), CameraErrorCode::SUCCESS);
    metadatOutput->Release();

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test metadataoutput when destruction
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput when destruction
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_067, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> output = cameraManager->CreateMetadataOutput();
    ASSERT_NE(output, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)output;

    metadatOutput->Release();
    EXPECT_EQ(metadatOutput->Stop(), CameraErrorCode::SUCCESS);
}

/*
 * Feature: Framework
 * Function: Test photooutput with cameraserverdied
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photooutput with cameraserverdied
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_068, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    pid_t pid = 0;
    photoOutput->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test photooutput with cameraserverdied
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photooutput with cameraserverdied
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_069, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    pid_t pid = 0;
    std::shared_ptr<PhotoStateCallback> setCallback =
        std::make_shared<TestPhotoOutputCallback>("PhotoStateCallback");
    ((sptr<PhotoOutput>&)photoOutput)->SetCallback(setCallback);
    photoOutput->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test photooutput with cameraserverdied when stream_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photooutput with cameraserverdied when stream_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_070, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);

    pid_t pid = 0;
    sptr<PhotoOutput> phtOutput = (sptr<PhotoOutput>&)photoOutput;
    phtOutput->stream_ = nullptr;
    photoOutput->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test photooutput with confirmcapture when stream_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test photooutput with confirmcapture when stream_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_071, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CaptureOutput> photoOutput = CreatePhotoOutput();
    ASSERT_NE(photoOutput, nullptr);
    sptr<PhotoOutput> phtOutput = (sptr<PhotoOutput>&)photoOutput;

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);

    int32_t ret = session->BeginConfig();
    EXPECT_EQ(ret, 0);

    ret = session->AddOutput(photoOutput);
    session->CommitConfig();

    phtOutput->stream_ = nullptr;
    EXPECT_EQ(phtOutput->ConfirmCapture(), CameraErrorCode::SESSION_NOT_RUNNING);

    phtOutput->Release();
    EXPECT_EQ(phtOutput->ConfirmCapture(), CameraErrorCode::SESSION_NOT_RUNNING);

    session->Release();
}

/*
 * Feature: Framework
 * Function: Test previewoutput
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test previewoutput
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_072, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    auto previewOutput = (sptr<PreviewOutput>&)preview;
    previewOutput->stream_ = nullptr;
    previewOutput->session_ = nullptr;

    EXPECT_EQ(previewOutput->OnSketchStatusChanged(SketchStatus::STOPED), CAMERA_INVALID_STATE);
    EXPECT_EQ(previewOutput->IsSketchSupported(), false);

    Size previewSize = {};
    previewOutput->stream_ = nullptr;
    EXPECT_EQ(previewOutput->CreateSketchWrapper(previewSize), CameraErrorCode::SERVICE_FATL_ERROR);
}

/*
 * Feature: Framework
 * Function: Test previewoutput with callback and cameraserverdied
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test previewoutput with callback and cameraserverdied
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_073, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    auto previewOutput = (sptr<PreviewOutput>&)preview;

    std::string eventString = "sketchStatusChanged";
    previewOutput->sketchWrapper_ = nullptr;
    previewOutput->OnNativeRegisterCallback(eventString);
    previewOutput->OnNativeUnregisterCallback(eventString);

    pid_t pid = 0;
    previewOutput->stream_ = nullptr;
    previewOutput->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test sketchwrapper
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketchwrapper
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_074, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> cameraInput = (sptr<CameraInput>&)input;

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceMetadata = cameraInput->GetCameraDeviceInfo()->GetMetadata();
    ASSERT_NE(deviceMetadata, nullptr);

    int32_t width = 1440;
    int32_t height = 1080;
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = width;
    previewSize.height = height;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> previewOutput = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(previewOutput, nullptr);

    Size sketchSize;
    sketchSize.width = 640;
    sketchSize.height = 480;

    SketchWrapper* sketchWrapper = new (std::nothrow) SketchWrapper(previewOutput->GetStream(), sketchSize);
    ASSERT_NE(sketchWrapper, nullptr);
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    auto modeName = session->GetFeaturesMode();
    int ret = sketchWrapper->Init(deviceMetadata, modeName);
    EXPECT_EQ(ret, CAMERA_OK);

    sketchWrapper->sketchStream_ = nullptr;
    ret = sketchWrapper->AttachSketchSurface(nullptr);
    EXPECT_EQ(ret, CAMERA_INVALID_STATE);
    EXPECT_EQ(sketchWrapper->StartSketchStream(), CAMERA_UNKNOWN_ERROR);

    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata;
    sketchWrapper->UpdateSketchEnableRatio(metadata);
    sketchWrapper->UpdateSketchReferenceFovRatio(metadata);
    sketchWrapper->UpdateSketchConfigFromMoonCaptureBoostConfig(metadata);
}

/*
 * Feature: Framework
 * Function: Test sketchwrapper
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketchwrapper
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_075, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> cameraInput = (sptr<CameraInput>&)input;

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceMetadata =
        cameraInput->GetCameraDeviceInfo()->GetMetadata();
    ASSERT_NE(deviceMetadata, nullptr);

    int32_t width = 1440;
    int32_t height = 1080;
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = width;
    previewSize.height = height;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> previewOutput =
        cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(previewOutput, nullptr);

    Size sketchSize;
    sketchSize.width = 640;
    sketchSize.height = 480;

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    SketchWrapper *sketchWrapper = new (std::nothrow)
        SketchWrapper(previewOutput->GetStream(), sketchSize);
    ASSERT_NE(sketchWrapper, nullptr);

    auto modeName = session->GetFeaturesMode();
    int ret = sketchWrapper->Init(deviceMetadata, modeName);
    EXPECT_EQ(ret, CAMERA_OK);

    float sketchRatio = sketchWrapper->GetSketchEnableRatio(session->GetFeaturesMode());
    EXPECT_EQ(sketchRatio, -1.0);
    sketchWrapper->hostStream_ = nullptr;
    EXPECT_EQ(sketchWrapper->UpdateSketchRatio(sketchRatio), CAMERA_INVALID_STATE);
    EXPECT_EQ(sketchWrapper->Destroy(), CAMERA_INVALID_STATE);
}

/*
 * Feature: Framework
 * Function: Test sketchwrapper with different tag
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test sketchwrapper with different tag
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_076, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> cameraInput = (sptr<CameraInput>&)input;

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceMetadata =
        cameraInput->GetCameraDeviceInfo()->GetMetadata();
    ASSERT_NE(deviceMetadata, nullptr);

    int32_t width = 1440;
    int32_t height = 1080;
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = width;
    previewSize.height = height;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> previewOutput = cameraManager->CreatePreviewOutput(previewProfile, surface);
    ASSERT_NE(previewOutput, nullptr);

    Size sketchSize;
    sketchSize.width = 640;
    sketchSize.height = 480;

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
    ASSERT_NE(session, nullptr);
    SketchWrapper *sketchWrapper = new (std::nothrow)
        SketchWrapper(previewOutput->GetStream(), sketchSize);
    ASSERT_NE(sketchWrapper, nullptr);

    auto modeName = session->GetFeaturesMode();
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    camera_metadata_item_t item;
    OHOS::Camera::FindCameraMetadataItem(metadata->get(), OHOS_ABILITY_STREAM_AVAILABLE_BASIC_CONFIGURATIONS, &item);
    const camera_device_metadata_tag_t tag1 = OHOS_CONTROL_ZOOM_RATIO;
    EXPECT_EQ(sketchWrapper->OnMetadataDispatch(modeName, tag1, item), 0);
    const camera_device_metadata_tag_t tag2 = OHOS_CONTROL_CAMERA_MACRO;
    EXPECT_EQ(sketchWrapper->OnMetadataDispatch(modeName, tag2, item), CAM_META_SUCCESS);
}

/*
 * Feature: Framework
 * Function: Test videooutput with cameraserverdied
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test videooutput with cameraserverdied
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_077, TestSize.Level0)
{
    int32_t width = VIDEO_DEFAULT_WIDTH;
    int32_t height = VIDEO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = width;
    videoSize.height = height;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(videoProfile, surface);
    ASSERT_NE(video, nullptr);

    pid_t pid = 0;
    video->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test videooutput with cameraserverdied when stream_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test videooutput with cameraserverdied when stream_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_078, TestSize.Level0)
{
    int32_t width = VIDEO_DEFAULT_WIDTH;
    int32_t height = VIDEO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = width;
    videoSize.height = height;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(videoProfile, surface);
    ASSERT_NE(video, nullptr);

    pid_t pid = 0;
    video->stream_ = nullptr;
    std::shared_ptr<VideoStateCallback> setCallback =
        std::make_shared<TestVideoOutputCallback>("VideoStateCallback");
    video->SetCallback(setCallback);
    video->CameraServerDied(pid);
}

/*
 * Feature: Framework
 * Function: Test videooutput when destruction
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test videooutput when destruction
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_079, TestSize.Level0)
{
    int32_t width = VIDEO_DEFAULT_WIDTH;
    int32_t height = VIDEO_DEFAULT_HEIGHT;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = width;
    videoSize.height = height;
    std::vector<int32_t> videoFramerates = {30, 30};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> video = cameraManager->CreateVideoOutput(videoProfile, surface);
    ASSERT_NE(video, nullptr);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice when settings is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice when settings is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_080, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow) HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> settings;
    EXPECT_EQ(camDevice->UpdateSettingOnce(settings), CAMERA_INVALID_ARG);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_081, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow) HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> settings = cameras[0]->GetMetadata();
    EXPECT_EQ(camDevice->UpdateSettingOnce(settings), 0);

    camDevice->RegisterFoldStatusListener();
    camDevice->UnRegisterFoldStatusListener();
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_082, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow) HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    camDevice->UnRegisterFoldStatusListener();
    camDevice->RegisterFoldStatusListener();
    camDevice->UnRegisterFoldStatusListener();
    camDevice->cameraHostManager_ = nullptr;
    camDevice->RegisterFoldStatusListener();
    camDevice->UnRegisterFoldStatusListener();

    std::vector<int32_t> results = {};
    EXPECT_EQ(camDevice->EnableResult(results), CAMERA_INVALID_ARG);
    EXPECT_EQ(camDevice->DisableResult(results), CAMERA_INVALID_ARG);

    std::shared_ptr<OHOS::Camera::CameraMetadata> metaIn;
    std::shared_ptr<OHOS::Camera::CameraMetadata> metaOut;
    EXPECT_EQ(camDevice->GetStatus(metaIn, metaOut), CAMERA_INVALID_ARG);

    camDevice->Close();
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice when hdiCameraDevice_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice when hdiCameraDevice_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_083, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow)
        HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    camDevice->hdiCameraDevice_ = nullptr;
    EXPECT_EQ(camDevice->InitStreamOperator(), CAMERA_UNKNOWN_ERROR);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice when streamOperator_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice when streamOperator_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_084, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow)
        HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    std::vector<HDI::Camera::V1_1::StreamInfo_V1_1> streamInfos = {};
    camDevice->CreateStreams(streamInfos);

    std::shared_ptr<OHOS::Camera::CameraMetadata> deviceSettings = cameras[0]->GetMetadata();
    int32_t operationMode = 0;
    camDevice->streamOperator_ = nullptr;
    EXPECT_EQ(camDevice->CommitStreams(deviceSettings, operationMode), CAMERA_UNKNOWN_ERROR);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice when streamOperator is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice when streamOperator is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_085, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow)
        HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    std::vector<StreamInfo_V1_1> streamInfos = {};
    EXPECT_EQ(camDevice->UpdateStreams(streamInfos), CAMERA_UNKNOWN_ERROR);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice when streamOperatorCallback is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice when streamOperatorCallback is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_086, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow)
        HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    int32_t captureId = 0;
    const std::vector<int32_t> streamIds = {1, 2};
    EXPECT_EQ(camDevice->OnCaptureStarted(captureId, streamIds), CAMERA_INVALID_STATE);

    HDI::Camera::V1_2::CaptureStartedInfo it1;
    it1.streamId_ = 1;
    it1.exposureTime_ = 1;
    HDI::Camera::V1_2::CaptureStartedInfo it2;
    it2.streamId_ = 2;
    it2.exposureTime_ = 2;
    std::vector<OHOS::HDI::Camera::V1_2::CaptureStartedInfo> captureStartedInfo = {};
    captureStartedInfo.push_back(it1);
    captureStartedInfo.push_back(it2);
    EXPECT_EQ(camDevice->OnCaptureStarted_V1_2(captureId, captureStartedInfo), CAMERA_INVALID_STATE);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice when streamOperatorCallback is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice when streamOperatorCallback is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_087, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow)
        HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    int32_t captureId = 0;
    const std::vector<int32_t> streamIds = {1, 2};
    EXPECT_EQ(camDevice->OnCaptureStarted(captureId, streamIds), CAMERA_INVALID_STATE);
    const std::vector<OHOS::HDI::Camera::V1_2::CaptureStartedInfo> captureStartedInfo = {};
    EXPECT_EQ(camDevice->OnCaptureStarted_V1_2(captureId, captureStartedInfo), CAMERA_INVALID_STATE);

    CaptureEndedInfo it1;
    it1.streamId_ = 1;
    it1.frameCount_ = 1;
    CaptureEndedInfo it2;
    it2.streamId_ = 2;
    it2.frameCount_ = 2;
    std::vector<CaptureEndedInfo> captureEndedInfo = {};
    captureEndedInfo.push_back(it1);
    captureEndedInfo.push_back(it2);
    EXPECT_EQ(camDevice->OnCaptureEnded(captureId, captureEndedInfo), CAMERA_INVALID_STATE);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice when streamOperatorCallback is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice when streamOperatorCallback is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_088, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow)
        HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    int32_t captureId = 0;
    CaptureErrorInfo it1;
    it1.streamId_ = 2;
    it1.error_ = BUFFER_LOST;
    CaptureErrorInfo it2;
    it2.streamId_ = 1;
    it2.error_ =  BUFFER_LOST;
    std::vector<CaptureErrorInfo> captureErrorInfo = {};
    captureErrorInfo.push_back(it1);
    captureErrorInfo.push_back(it2);
    EXPECT_EQ(camDevice->OnCaptureError(captureId, captureErrorInfo), CAMERA_INVALID_STATE);

    const std::vector<int32_t> streamIds = {1, 2};
    uint64_t timestamp = 5;
    EXPECT_EQ(camDevice->OnFrameShutter(captureId, streamIds, timestamp), CAMERA_INVALID_STATE);
    EXPECT_EQ(camDevice->OnFrameShutterEnd(captureId, streamIds, timestamp), CAMERA_INVALID_STATE);
    EXPECT_EQ(camDevice->OnCaptureReady(captureId, streamIds, timestamp), CAMERA_INVALID_STATE);
}

/*
 * Feature: Framework
 * Function: Test HCameraDevice with CheckOnResultData
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraDevice with CheckOnResultData
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_089, TestSize.Level0)
{
    std::vector<int32_t> result;
    result.push_back(OHOS_SENSOR_EXPOSURE_TIME);
    result.push_back(OHOS_SENSOR_COLOR_CORRECTION_GAINS);

    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<HCameraHostManager> cameraHostManager = (sptr<HCameraHostManager> &)mockCameraHostManager;
    std::string cameraId = cameras[0]->GetID();
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    sptr<HCameraDevice> camDevice = new(std::nothrow)
        HCameraDevice(cameraHostManager, cameraId, callerToken);
    ASSERT_NE(camDevice, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraResult = cameras[0]->GetMetadata();
    camDevice->CheckOnResultData(nullptr);
    camDevice->CheckOnResultData(cameraResult);
}

/*
 * Feature: coverage
 * Function: Test HStreamRepeat
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_090, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow)
        HStreamRepeat(producer, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<HStreamRepeat> streamRepeat1 =
        new (std::nothrow) HStreamRepeat(producer, 4, 1280, 960, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat1, nullptr);
    SketchStatus status = SketchStatus::STARTED;
    streamRepeat->repeatStreamType_ = RepeatStreamType::SKETCH;
    streamRepeat->parentStreamRepeat_ = streamRepeat1;
    streamRepeat->UpdateSketchStatus(status);
}

/*
 * Feature: coverage
 * Function: Test HStreamRepeat when status is STARTED
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat when status is STARTED
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_091, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow)
        HStreamRepeat(producer, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    SketchStatus status = SketchStatus::STARTED;
    EXPECT_EQ(streamRepeat->OnSketchStatusChanged(status), 0);
}

/*
 * Feature: coverage
 * Function: Test HStreamRepeat when sketchStream is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat when sketchStream is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_092, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow)
        HStreamRepeat(producer, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<IStreamRepeat> sketchStream = nullptr;
    float sketchRatio = 0;
    EXPECT_EQ(streamRepeat->ForkSketchStreamRepeat(0, 1, sketchStream, sketchRatio), CAMERA_INVALID_ARG);
    EXPECT_EQ(streamRepeat->ForkSketchStreamRepeat(1, 0, sketchStream, sketchRatio), CAMERA_INVALID_ARG);
}

/*
 * Feature: coverage
 * Function: Test HStreamRepeat when sketchStreamRepeat_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat when sketchStreamRepeat_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_093, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow)
        HStreamRepeat(producer, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    float sketchRatio = 0;
    streamRepeat->sketchStreamRepeat_ = nullptr;
    EXPECT_EQ(streamRepeat->RemoveSketchStreamRepeat(), 0);
    EXPECT_EQ(streamRepeat->UpdateSketchRatio(sketchRatio), CAMERA_INVALID_STATE);
}

/*
 * Feature: coverage
 * Function: Test HStreamRepeat
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_094, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow)
        HStreamRepeat(producer, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    uint32_t interfaceCode = 5;
    EXPECT_EQ(streamRepeat->OperatePermissionCheck(interfaceCode), 0);
}

/*
 * Feature: coverage
 * Function: Test HCaptureSession when stream is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession when stream is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_095, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    std::vector<StreamInfo_V1_1> streamInfos = {};
    EXPECT_EQ(camSession->GetCurrentStreamInfos(streamInfos), 0);
    EXPECT_EQ(camSession->AddOutputStream(nullptr), CAMERA_INVALID_ARG);
    EXPECT_EQ(camSession->RemoveOutputStream(nullptr), CAMERA_INVALID_ARG);
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test HCaptureSession when cameraDevice_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession when cameraDevice_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_096, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    camSession->cameraDevice_ = nullptr;
    EXPECT_EQ(camSession->LinkInputAndOutputs(), CAMERA_INVALID_SESSION_CFG);
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test HCaptureSession with SetColorSpace
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession with SetColorSpace
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_097, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    bool isNeedUpdate = false;
    ColorSpace colorSpace = ColorSpace::SRGB;
    ColorSpace captureColorSpace = ColorSpace::SRGB;
    camSession->currColorSpace_ = ColorSpace::BT709;
    camSession->currCaptureColorSpace_ = ColorSpace::BT709;
    EXPECT_EQ(camSession->SetColorSpace(colorSpace, captureColorSpace, isNeedUpdate), CAMERA_INVALID_STATE);
    camSession->currColorSpace_ = ColorSpace::SRGB;
    EXPECT_EQ(camSession->SetColorSpace(colorSpace, captureColorSpace, isNeedUpdate), CAMERA_INVALID_STATE);
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test HCaptureSession with CheckIfColorSpaceMatchesFormat
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession with CheckIfColorSpaceMatchesFormat
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_098, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    camSession->RestartStreams();

    ColorSpace colorSpace = ColorSpace::SRGB;
    EXPECT_EQ(camSession->CheckIfColorSpaceMatchesFormat(colorSpace), 0);
    colorSpace = ColorSpace::BT2020_HLG ;
    EXPECT_EQ(camSession->CheckIfColorSpaceMatchesFormat(colorSpace), 0);
    colorSpace = ColorSpace::BT2020_PQ ;
    EXPECT_EQ(camSession->CheckIfColorSpaceMatchesFormat(colorSpace), 0);
    colorSpace = ColorSpace::BT2020_HLG_LIMIT ;
    EXPECT_EQ(camSession->CheckIfColorSpaceMatchesFormat(colorSpace), 0);
    colorSpace = ColorSpace::BT2020_PQ_LIMIT;
    EXPECT_EQ(camSession->CheckIfColorSpaceMatchesFormat(colorSpace), 0);
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test HCaptureSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_099, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test HCaptureSession when isSessionStarted_ is true
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession when isSessionStarted_ is true
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_100, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    camSession->SetColorSpaceForStreams();

    std::vector<StreamInfo_V1_1> streamInfos = {};
    camSession->CancelStreamsAndGetStreamInfos(streamInfos);

    camSession->isSessionStarted_ = true;
    camSession->RestartStreams();
    camSession->Release();
}

/*
 * Feature: coverage
 * Function: Test HCaptureSession when cameraDevice is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCaptureSession when cameraDevice is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_101, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode mode = PORTRAIT;
    sptr<HCaptureSession> camSession = new (std::nothrow) HCaptureSession(callerToken, mode);
    ASSERT_NE(camSession, nullptr);

    float currentFps = 0;
    float currentZoomRatio = 0;
    EXPECT_EQ(camSession->QueryFpsAndZoomRatio(currentFps, currentZoomRatio), false);
    std::vector<float> crossZoomAndTime = {0, 0};
    int32_t operationMode = 0;
    EXPECT_EQ(camSession->QueryZoomPerformance(crossZoomAndTime, operationMode), false);
    int32_t smoothZoomType = 0;
    float targetZoomRatio = 0;
    float duration = 0;
    EXPECT_EQ(camSession->SetSmoothZoom(smoothZoomType, operationMode,
        targetZoomRatio, duration), 11);
    camSession->Release();
}

/*
 * Feature: Framework
 * Function: Test metadataoutput when appStateCallback_ is not nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test metadataoutput when appStateCallback_ is not nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_102, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureOutput> output = cameraManager->CreateMetadataOutput();
    ASSERT_NE(output, nullptr);
    sptr<MetadataOutput> metadatOutput = (sptr<MetadataOutput>&)output;

    std::shared_ptr<MetadataStateCallback> metadataStateCallback =
        std::make_shared<AppMetadataCallback>();
    metadatOutput->SetCallback(metadataStateCallback);

    pid_t pid = 0;
    metadatOutput->CameraServerDied(pid);
}

/*
 * Feature: coverage
 * Function: Test HStreamRepeat when repeatStreamType_ is SKETCH
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat when repeatStreamType_ is SKETCH
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_103, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow)
        HStreamRepeat(producer, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    StreamInfo_V1_1 streamInfo;
    streamRepeat->repeatStreamType_ = RepeatStreamType::SKETCH;
    streamRepeat->SetStreamInfo(streamInfo);
}

/*
 * Feature: Framework
 * Function: Test PortraitSession when output is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test PortraitSession when output is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_104, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);
    sptr<CaptureOutput> output = nullptr;
    EXPECT_EQ(portraitSession->CanAddOutput(output), false);
    EXPECT_EQ(portraitSession->BeginConfig(), 0);
    EXPECT_EQ(portraitSession->AddInput(input), 0);
    EXPECT_EQ(portraitSession->AddOutput(preview), 0);
    EXPECT_EQ(portraitSession->AddOutput(photo), 0);
    EXPECT_EQ(portraitSession->CommitConfig(), 0);

    const float aperture = 0.1;
    EXPECT_EQ(portraitSession->CanAddOutput(output), false);
    std::vector<float> supportedVirtualApertures = {};
    std::vector<float> virtualApertures = {};
    portraitSession->GetSupportedVirtualApertures(virtualApertures);
    EXPECT_EQ(virtualApertures, supportedVirtualApertures);
    float virtualAperture = 0.0f;
    portraitSession->GetVirtualAperture(virtualAperture);
    EXPECT_EQ(virtualAperture, 0.0);
    portraitSession->SetVirtualAperture(aperture);
    std::vector<std::vector<float>> supportedPhysicalApertures;
    std::vector<std::vector<float>> physicalApertures;
    portraitSession->GetSupportedPhysicalApertures(physicalApertures);
    EXPECT_EQ(physicalApertures, supportedPhysicalApertures);
    float physicalAperture = 0.0f;
    portraitSession->GetPhysicalAperture(physicalAperture);
    EXPECT_EQ(physicalAperture, 0.0f);
    portraitSession->SetPhysicalAperture(aperture);

    portraitSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test PortraitSession without CommitConfig
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test PortraitSession without CommitConfig
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_105, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    EXPECT_EQ(portraitSession->BeginConfig(), 0);
    EXPECT_EQ(portraitSession->AddInput(input), 0);
    EXPECT_EQ(portraitSession->AddOutput(preview), 0);
    EXPECT_EQ(portraitSession->AddOutput(photo), 0);

    const float physicalAperture = 0.1;
    const float virtualAperture = 0.1;
    std::vector<float> supportedVirtualApertures = {};
    std::vector<float> virtualApertures = {};
    portraitSession->GetSupportedVirtualApertures(virtualApertures);
    EXPECT_EQ(virtualApertures, supportedVirtualApertures);
    float virtualApertureResult = 0.0f;
    portraitSession->GetVirtualAperture(virtualApertureResult);
    EXPECT_EQ(virtualApertureResult, 0.0);
    portraitSession->SetVirtualAperture(virtualAperture);
    std::vector<std::vector<float>> supportedPhysicalApertures;
    std::vector<std::vector<float>> physicalApertures;
    portraitSession->GetSupportedPhysicalApertures(physicalApertures);
    EXPECT_EQ(physicalApertures, supportedPhysicalApertures);
    float physicalApertureResult = 0.0f;
    portraitSession->GetPhysicalAperture(physicalApertureResult);
    EXPECT_EQ(physicalApertureResult, 0.0f);
    portraitSession->SetPhysicalAperture(physicalAperture);
    portraitSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test PortraitSession when innerInputDevice_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test PortraitSession when innerInputDevice_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_106, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<PortraitSession> portraitSession = nullptr;
    portraitSession = static_cast<PortraitSession *> (captureSession.GetRefPtr());
    ASSERT_NE(portraitSession, nullptr);

    EXPECT_EQ(portraitSession->BeginConfig(), 0);
    EXPECT_EQ(portraitSession->AddInput(input), 0);
    EXPECT_EQ(portraitSession->AddOutput(preview), 0);
    EXPECT_EQ(portraitSession->AddOutput(photo), 0);
    EXPECT_EQ(portraitSession->CommitConfig(), 0);

    portraitSession->innerInputDevice_ = nullptr;
    std::vector<PortraitEffect> supportedPortraitEffects = {};
    EXPECT_EQ(portraitSession->GetSupportedPortraitEffects(), supportedPortraitEffects);
    EXPECT_EQ(portraitSession->GetPortraitEffect(), PortraitEffect::OFF_EFFECT);
    std::vector<float> supportedVirtualApertures = {};
    std::vector<float> virtualApertures = {};
    portraitSession->GetSupportedVirtualApertures(virtualApertures);
    EXPECT_EQ(virtualApertures, supportedVirtualApertures);
    float virtualAperture = 0.0f;
    portraitSession->GetVirtualAperture(virtualAperture);
    EXPECT_EQ(virtualAperture, 0.0);
    std::vector<std::vector<float>> supportedPhysicalApertures;
    std::vector<std::vector<float>> physicalApertures;
    portraitSession->GetSupportedPhysicalApertures(physicalApertures);
    EXPECT_EQ(physicalApertures, supportedPhysicalApertures);
    float physicalAperture = 0.0f;
    portraitSession->GetPhysicalAperture(physicalAperture);
    EXPECT_EQ(physicalAperture, 0.0);

    portraitSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test NightSession about exposure
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test NightSession about exposure
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_107, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = NIGHT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);

    EXPECT_EQ(nightSession->BeginConfig(), 0);
    EXPECT_EQ(nightSession->AddInput(input), 0);
    EXPECT_EQ(nightSession->AddOutput(preview), 0);
    EXPECT_EQ(nightSession->AddOutput(photo), 0);
    EXPECT_EQ(nightSession->CommitConfig(), 0);

    std::vector<uint32_t> exposureRange = {};
    uint32_t exposureValue = 0;
    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), CameraErrorCode::INVALID_ARGUMENT);
    EXPECT_EQ(nightSession->GetExposure(exposureValue), CameraErrorCode::INVALID_ARGUMENT);

    nightSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test NightSession when innerInputDevice_ and output is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test NightSession when innerInputDevice_ and output is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_108, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = NIGHT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);
    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata = cameras[0]->GetMetadata();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);
    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);
    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<NightSession> nightSession = static_cast<NightSession*>(captureSession.GetRefPtr());
    ASSERT_NE(nightSession, nullptr);

    EXPECT_EQ(nightSession->BeginConfig(), 0);
    EXPECT_EQ(nightSession->AddInput(input), 0);
    EXPECT_EQ(nightSession->AddOutput(preview), 0);
    EXPECT_EQ(nightSession->AddOutput(photo), 0);
    EXPECT_EQ(nightSession->CommitConfig(), 0);

    nightSession->innerInputDevice_ = nullptr;
    std::vector<uint32_t> exposureRange = {};
    EXPECT_EQ(nightSession->GetExposureRange(exposureRange), CameraErrorCode::INVALID_ARGUMENT);
    sptr<CaptureOutput> output = nullptr;
    EXPECT_EQ(nightSession->CanAddOutput(output), false);

    nightSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test ScanSession when output is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ScanSession when output is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_109, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = SCAN;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ScanSession> scanSession = static_cast<ScanSession*>(captureSession.GetRefPtr());
    ASSERT_NE(scanSession, nullptr);

    EXPECT_EQ(scanSession->BeginConfig(), 0);
    EXPECT_EQ(scanSession->AddInput(input), 0);
    EXPECT_EQ(scanSession->AddOutput(preview), 0);
    EXPECT_EQ(scanSession->CommitConfig(), 0);

    sptr<CaptureOutput> output = nullptr;
    scanSession->CanAddOutput(output);

    scanSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test ScanSession when innerInputDevice_ is nullptr
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ScanSession when innerInputDevice_ is nullptr
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_110, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = SCAN;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ScanSession> scanSession = static_cast<ScanSession*>(captureSession.GetRefPtr());
    ASSERT_NE(scanSession, nullptr);

    EXPECT_EQ(scanSession->CanAddOutput(photo), false);
    EXPECT_EQ(scanSession->BeginConfig(), 0);
    EXPECT_EQ(scanSession->AddInput(input), 0);
    EXPECT_EQ(scanSession->AddOutput(preview), 0);

    scanSession->CanAddOutput(photo);

    scanSession->innerInputDevice_ = nullptr;
    int32_t ret = scanSession->AddOutput(preview);
    EXPECT_EQ(ret, SESSION_NOT_CONFIG);

    ret = scanSession->CommitConfig();
    EXPECT_EQ(ret, 0);

    scanSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test ScanSession
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ScanSession
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_111, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    SceneMode mode = SCAN;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureOutput> preview = CreatePreviewOutput();
    ASSERT_NE(preview, nullptr);

    sptr<CaptureOutput> photo = CreatePhotoOutput();
    ASSERT_NE(photo, nullptr);

    sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(mode);
    ASSERT_NE(captureSession, nullptr);
    sptr<ScanSession> scanSession = static_cast<ScanSession*>(captureSession.GetRefPtr());
    ASSERT_NE(scanSession, nullptr);

    EXPECT_EQ(scanSession->CanAddOutput(photo), false);
    EXPECT_EQ(scanSession->BeginConfig(), 0);
    EXPECT_EQ(scanSession->AddInput(input), 0);
    EXPECT_EQ(scanSession->AddOutput(preview), 0);

    scanSession->CanAddOutput(photo);

    scanSession->innerInputDevice_ = nullptr;
    int32_t ret = scanSession->AddOutput(preview);
    EXPECT_EQ(ret, SESSION_NOT_CONFIG);

    ret = scanSession->CommitConfig();
    EXPECT_EQ(ret, 0);

    scanSession->Release();
    input->Close();
}

/*
 * Feature: Framework
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HCameraService with anomalous branch
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_112, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> cameraService = (sptr<HCameraService> &)mockHCameraService;
    ASSERT_NE(cameraService, nullptr);

    sptr<ICameraServiceCallback> callback = nullptr;
    int32_t intResult = cameraService->SetCameraCallback(callback);
    EXPECT_EQ(intResult, 2);

    callback = new(std::nothrow) CameraStatusServiceCallback(cameraManager);
    ASSERT_NE(callback, nullptr);
    intResult = cameraService->SetCameraCallback(callback);
    EXPECT_EQ(intResult, 0);

    sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    sptr<ICameraMuteServiceCallback> callback_2 = nullptr;
    intResult = cameraService->SetMuteCallback(callback_2);
    EXPECT_EQ(intResult, 2);

    callback_2 = new(std::nothrow) CameraMuteServiceCallback(cameraManager);
    ASSERT_NE(callback_2, nullptr);
    intResult = cameraService->SetMuteCallback(callback_2);
    EXPECT_EQ(intResult, 0);

    std::string cameraId = camInput->GetCameraId();
    int activeTime = 0;
    EffectParam effectParam = {0, 0, 0};
    intResult = cameraService->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::TRANSIENT_ACTIVE_PARAM_OHOS,
        activeTime, effectParam);
    EXPECT_EQ(intResult, 2);

    intResult = cameraService->SetPrelaunchConfig(cameraId, RestoreParamTypeOhos::PERSISTENT_DEFAULT_PARAM_OHOS,
    activeTime, effectParam);
    EXPECT_EQ(intResult, 2);

    input->Close();
}

void CameraFrameworkUnitTest::TestPhotoSessionPreconfig(
    sptr<CaptureInput>& input, PreconfigType preconfigType, ProfileSizeRatio profileSizeRatio)
{
    sptr<CaptureSession> photoSession = cameraManager->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(photoSession, nullptr);
    if (photoSession->CanPreconfig(preconfigType, profileSizeRatio)) {
        int32_t intResult = photoSession->Preconfig(preconfigType, profileSizeRatio);
        EXPECT_EQ(intResult, 0);

        sptr<Surface> preivewSurface = Surface::CreateSurfaceAsConsumer();
        ASSERT_NE(preivewSurface, nullptr);

        sptr<PreviewOutput> previewOutput = nullptr;
        intResult = cameraManager->CreatePreviewOutputWithoutProfile(preivewSurface, &previewOutput);
        EXPECT_EQ(intResult, 0);
        ASSERT_NE(previewOutput, nullptr);

        sptr<IConsumerSurface> photoSurface = IConsumerSurface::Create();
        ASSERT_NE(photoSurface, nullptr);
        auto photoProducer = photoSurface->GetProducer();
        ASSERT_NE(photoProducer, nullptr);
        sptr<PhotoOutput> photoOutput = nullptr;
        intResult = cameraManager->CreatePhotoOutputWithoutProfile(photoProducer, &photoOutput);
        EXPECT_EQ(intResult, 0);
        ASSERT_NE(photoOutput, nullptr);

        intResult = photoSession->BeginConfig();
        EXPECT_EQ(intResult, 0);

        intResult = photoSession->AddInput(input);
        EXPECT_EQ(intResult, 0);

        sptr<CaptureOutput> previewOutputCaptureUpper = previewOutput;
        intResult = photoSession->AddOutput(previewOutputCaptureUpper);
        EXPECT_EQ(intResult, 0);
        sptr<CaptureOutput> photoOutputCaptureUpper = photoOutput;
        intResult = photoSession->AddOutput(photoOutputCaptureUpper);
        EXPECT_EQ(intResult, 0);

        intResult = photoSession->CommitConfig();
        EXPECT_EQ(intResult, 0);

        intResult = photoSession->Start();
        EXPECT_EQ(intResult, 0);

        intResult = photoSession->Stop();
        EXPECT_EQ(intResult, 0);

        intResult = photoSession->BeginConfig();
        EXPECT_EQ(intResult, 0);

        intResult = photoSession->RemoveInput(input);
        EXPECT_EQ(intResult, 0);

        intResult = photoSession->Release();
        EXPECT_EQ(intResult, 0);
    }
}

/*
 * Feature: Framework
 * Function: Test PhotoSession preconfig
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preconfig PhotoSession all config.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_preconfig_unittest_001, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> cameraService = (sptr<HCameraService>&)mockHCameraService;
    ASSERT_NE(cameraService, nullptr);

    sptr<ICameraServiceCallback> callback = new (std::nothrow) CameraStatusServiceCallback(cameraManager);
    ASSERT_NE(callback, nullptr);
    int32_t intResult = cameraService->SetCameraCallback(callback);
    EXPECT_EQ(intResult, 0);

    sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    std::vector<PreconfigType> preconfigTypes = { PRECONFIG_720P, PRECONFIG_1080P, PRECONFIG_4K,
        PRECONFIG_HIGH_QUALITY };
    std::vector<ProfileSizeRatio> profileSizeRatios = { UNSPECIFIED, RATIO_1_1, RATIO_4_3, RATIO_16_9 };
    for (auto& preconfigType : preconfigTypes) {
        for (auto& profileSizeRatio : profileSizeRatios) {
            TestPhotoSessionPreconfig(input, preconfigType, profileSizeRatio);
        }
    }

    input->Close();
}

void CameraFrameworkUnitTest::TestVideoSessionPreconfig(
    sptr<CaptureInput>& input, PreconfigType preconfigType, ProfileSizeRatio profileSizeRatio)
{
    sptr<CaptureSession> videoSession = cameraManager->CreateCaptureSession(SceneMode::VIDEO);
    ASSERT_NE(videoSession, nullptr);
    if (videoSession->CanPreconfig(preconfigType, profileSizeRatio)) {
        int32_t intResult = videoSession->Preconfig(preconfigType, profileSizeRatio);
        EXPECT_EQ(intResult, 0);

        sptr<PreviewOutput> previewOutput = nullptr;
        intResult =
            cameraManager->CreatePreviewOutputWithoutProfile(Surface::CreateSurfaceAsConsumer(), &previewOutput);
        EXPECT_EQ(intResult, 0);
        ASSERT_NE(previewOutput, nullptr);

        sptr<IConsumerSurface> photoSurface = IConsumerSurface::Create();
        ASSERT_NE(photoSurface, nullptr);
        auto photoProducer = photoSurface->GetProducer();
        ASSERT_NE(photoProducer, nullptr);
        sptr<PhotoOutput> photoOutput = nullptr;
        intResult = cameraManager->CreatePhotoOutputWithoutProfile(photoProducer, &photoOutput);
        EXPECT_EQ(intResult, 0);
        ASSERT_NE(photoOutput, nullptr);

        sptr<VideoOutput> videoOutput = nullptr;
        intResult = cameraManager->CreateVideoOutputWithoutProfile(Surface::CreateSurfaceAsConsumer(), &videoOutput);
        EXPECT_EQ(intResult, 0);
        ASSERT_NE(videoOutput, nullptr);

        intResult = videoSession->BeginConfig();
        EXPECT_EQ(intResult, 0);

        intResult = videoSession->AddInput(input);
        EXPECT_EQ(intResult, 0);

        sptr<CaptureOutput> previewOutputCaptureUpper = previewOutput;
        intResult = videoSession->AddOutput(previewOutputCaptureUpper);
        EXPECT_EQ(intResult, 0);
        sptr<CaptureOutput> photoOutputCaptureUpper = photoOutput;
        intResult = videoSession->AddOutput(photoOutputCaptureUpper);
        EXPECT_EQ(intResult, 0);
        sptr<CaptureOutput> videoOutputCaptureUpper = videoOutput;
        intResult = videoSession->AddOutput(videoOutputCaptureUpper);
        EXPECT_EQ(intResult, 0);

        intResult = videoSession->CommitConfig();
        EXPECT_EQ(intResult, 0);

        intResult = videoSession->Start();
        EXPECT_EQ(intResult, 0);

        intResult = videoSession->Stop();
        EXPECT_EQ(intResult, 0);

        intResult = videoSession->BeginConfig();
        EXPECT_EQ(intResult, 0);

        intResult = videoSession->RemoveInput(input);
        EXPECT_EQ(intResult, 0);

        intResult = videoSession->Release();
        EXPECT_EQ(intResult, 0);
    }
}

/*
 * Feature: Framework
 * Function: Test VideoSession preconfig
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test preconfig VideoSession all config.
 */
HWTEST_F(CameraFrameworkUnitTest, camera_preconfig_unittest_002, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
    EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
    sptr<CameraInput> camInput = (sptr<CameraInput>&)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<FakeHCameraService> mockHCameraService = new FakeHCameraService(mockCameraHostManager);
    sptr<HCameraService> cameraService = (sptr<HCameraService>&)mockHCameraService;
    ASSERT_NE(cameraService, nullptr);

    sptr<ICameraServiceCallback> callback = new (std::nothrow) CameraStatusServiceCallback(cameraManager);
    ASSERT_NE(callback, nullptr);
    int32_t intResult = cameraService->SetCameraCallback(callback);
    EXPECT_EQ(intResult, 0);

    sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
    ASSERT_NE(deviceObj, nullptr);

    std::vector<PreconfigType> preconfigTypes = { PRECONFIG_720P, PRECONFIG_1080P, PRECONFIG_4K,
        PRECONFIG_HIGH_QUALITY };
    std::vector<ProfileSizeRatio> profileSizeRatios = { UNSPECIFIED, RATIO_1_1, RATIO_4_3, RATIO_16_9 };
    for (auto& preconfigType : preconfigTypes) {
        for (auto& profileSizeRatio : profileSizeRatios) {
            TestVideoSessionPreconfig(input, preconfigType, profileSizeRatio);
        }
    }

    input->Close();
}

/*
 * Feature: Framework
 * Function: Test fuzz
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test opMode PORTRAIT fuzz test
 */
HWTEST_F(CameraFrameworkUnitTest, camera_fwcoverage_unittest_121, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode opMode = PORTRAIT;
    sptr<HCaptureSession> session = HCaptureSession::NewInstance(callerToken, opMode);
    ASSERT_NE(session, nullptr);
    session->Release();
}

/*
* Feature: Framework
* Function: Test normal branch that is support secure camera for open camera
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test normal branch for open camera
*/
HWTEST_F(CameraFrameworkUnitTest, camera_opencamera_unittest_001, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);

            uint64_t secureSeqId = 0;
            sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
            ASSERT_NE(deviceObj, nullptr);

            int intResult = camInput->Open(true, &secureSeqId);
            EXPECT_EQ(intResult, 0);
            EXPECT_NE(secureSeqId, 0);
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test anomalous branch that is not support secure camera for open camera
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test anomalous branch for open camera
*/
HWTEST_F(CameraFrameworkUnitTest, camera_opencamera_unittest_002, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) == modes.end()) {
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);

            uint64_t secureSeqId = 0;
            sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
            ASSERT_NE(deviceObj, nullptr);

            int intResult = camInput->Open(true, &secureSeqId);
            EXPECT_EQ(intResult, 0);
            EXPECT_EQ(secureSeqId, 0);
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test anomalous branch when open secure camera that seq is null
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test anomalous branch for open camera
*/
HWTEST_F(CameraFrameworkUnitTest, camera_opencamera_unittest_003, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);

            sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
            ASSERT_NE(deviceObj, nullptr);

            uint64_t* secureSeqId = nullptr;
            int intResult = camInput->Open(true, secureSeqId);
            EXPECT_EQ(intResult, CAMERA_INVALID_ARG);
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test normal branch that is open non-secure camera
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test normal branch that is open non-secure camera
*/
HWTEST_F(CameraFrameworkUnitTest, camera_opencamera_unittest_004, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) == modes.end()) {
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);

            sptr<ICameraDeviceService> deviceObj = camInput->GetCameraDevice();
            ASSERT_NE(deviceObj, nullptr);

            uint64_t secureSeqId = 0;
            int intResult = camInput->Open(false, &secureSeqId);
            EXPECT_EQ(intResult, CAMERA_OK);
            EXPECT_EQ(secureSeqId, 0);
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test normal branch that is add secure output flag
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test normal branch that is add secure output flag
*/
HWTEST_F(CameraFrameworkUnitTest, camera_securecamera_unittest_001, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = cameraManager->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);
            uint64_t secureSeqId = 0;
            int intResult = camInput->Open(true, &secureSeqId);
            EXPECT_EQ(intResult, CAMERA_OK);
            EXPECT_NE(secureSeqId, 0);

            sptr<CaptureOutput> preview = CreatePreviewOutput();
            ASSERT_NE(preview, nullptr);
            sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(input), 0);
            EXPECT_EQ(secureSession->AddOutput(preview), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview), 0);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            secureSession->Release();
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test normal branch that is add secure output flag
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test normal branch that is add secure output flag
*/
HWTEST_F(CameraFrameworkUnitTest, camera_securecamera_unittest_002, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = cameraManager->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);
            uint64_t secureSeqId = 0;
            int intResult = camInput->Open(false, &secureSeqId);
            EXPECT_EQ(intResult, CAMERA_OK);
            EXPECT_NE(secureSeqId, 0);
            sptr<CaptureOutput> preview = CreatePreviewOutput();
            ASSERT_NE(preview, nullptr);
            sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(input), 0);
            EXPECT_EQ(secureSession->AddOutput(preview), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview), 0);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            secureSession->Release();
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test abnormal branch that is add secure output flag twice
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test abnormal branch that is add secure output flag twice
*/
HWTEST_F(CameraFrameworkUnitTest, camera_securecamera_unittest_003, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = cameraManager->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);
            uint64_t secureSeqId = 0;
            int intResult = camInput->Open(false, &secureSeqId);
            EXPECT_EQ(intResult, CAMERA_OK);
            EXPECT_NE(secureSeqId, 0);

            sptr<CaptureOutput> preview1 = CreatePreviewOutput();
            ASSERT_NE(preview1, nullptr);
            sptr<CaptureOutput> preview2 = CreatePreviewOutput();
            ASSERT_NE(preview2, nullptr);
            sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(input), 0);
            EXPECT_EQ(secureSession->AddOutput(preview1), 0);
            EXPECT_EQ(secureSession->AddOutput(preview2), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview1), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview2), OPERATION_NOT_ALLOWED);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            secureSession->Release();
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test abnormal branch that is add secure output flag twice
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test abnormal branch that is add secure output flag twice
*/
HWTEST_F(CameraFrameworkUnitTest, camera_securecamera_unittest_004, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = cameraManager->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);
            uint64_t secureSeqId = 0;
            int intResult = camInput->Open(false, &secureSeqId);
            EXPECT_EQ(intResult, CAMERA_OK);
            EXPECT_NE(secureSeqId, 0);
            sptr<CaptureOutput> preview1 = CreatePreviewOutput();
            ASSERT_NE(preview1, nullptr);
            sptr<CaptureOutput> preview2 = CreatePreviewOutput();
            ASSERT_NE(preview2, nullptr);
            sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(input), 0);
            EXPECT_EQ(secureSession->AddOutput(preview1), 0);
            EXPECT_EQ(secureSession->AddOutput(preview2), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview1), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview2), OPERATION_NOT_ALLOWED);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            secureSession->Release();
            input->Close();
            break;
        }
    }
}

/*
* Feature: Framework
* Function: Test abnormal branch that secure output is added after commiting
* SubFunction: NA
* FunctionPoints: NA
* EnvConditions: NA
* CaseDescription: Test abnormal branch that secure output is added after commiting
*/
HWTEST_F(CameraFrameworkUnitTest, camera_securecamera_unittest_005, TestSize.Level0)
{
    InSequence s;
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        ASSERT_TRUE(modes.size() != 0);

        if (find(modes.begin(), modes.end(), SceneMode::SECURE) != modes.end()) {
            sptr<CameraOutputCapability> ability = cameraManager->
                GetSupportedOutputCapability(camDevice, SceneMode::SECURE);
            ASSERT_NE(ability, nullptr);
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);
            EXPECT_CALL(*mockCameraHostManager, OpenCameraDevice(_, _, _, _));
            EXPECT_CALL(*mockCameraDevice, SetResultMode(ON_CHANGED));
            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);
            uint64_t secureSeqId = 0;
            int intResult = camInput->Open(false, &secureSeqId);
            EXPECT_EQ(intResult, CAMERA_OK);
            EXPECT_NE(secureSeqId, 0);
            sptr<CaptureOutput> preview = CreatePreviewOutput();
            ASSERT_NE(preview, nullptr);
            sptr<CaptureSession> captureSession = cameraManager->CreateCaptureSession(SceneMode::SECURE);
            ASSERT_NE(captureSession, nullptr);
            sptr<SecureCameraSession> secureSession = nullptr;
            secureSession = static_cast<SecureCameraSession *> (captureSession.GetRefPtr());
            ASSERT_NE(secureSession, nullptr);

            EXPECT_EQ(secureSession->BeginConfig(), 0);
            EXPECT_EQ(secureSession->AddInput(input), 0);
            EXPECT_EQ(secureSession->AddOutput(preview), 0);
            EXPECT_EQ(secureSession->CommitConfig(), 0);
            EXPECT_EQ(secureSession->AddSecureOutput(preview), OPERATION_NOT_ALLOWED);
            secureSession->Release();
            input->Close();
            break;
        }
    }
}

HWTEST_F(CameraFrameworkUnitTest, test_createDeferredPhotoProcessingSession, TestSize.Level0)
{
    sptr<DeferredPhotoProcSession> deferredProcSession;
    deferredProcSession = cameraManager->CreateDeferredPhotoProcessingSession(g_userId_,
        std::make_shared<TestDeferredPhotoProcSessionCallback>());
    ASSERT_NE(deferredProcSession, nullptr);
}

/**
* @tc.name    : Test IsSlowMotionDetectionSupported API
* @tc.number  : IsSlowMotionDetectionSupported_001
* @tc.desc    : Test IsSlowMotionDetectionSupported interface, when session is not committed.
* @tc.require : slow motion only support videoStream & previewStrem & 1080p & 240fps
*/
HWTEST_F(CameraFrameworkUnitTest, IsSlowMotionDetectionSupported_001, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession(SceneMode::SLOW_MOTION);
    ASSERT_NE(session, nullptr);
    sptr<SlowMotionSession> slowSession = static_cast<SlowMotionSession *> (session.GetRefPtr());
    bool result = slowSession->IsSlowMotionDetectionSupported();
    EXPECT_EQ(false, result);
}

/**
* @tc.name    : Test IsSlowMotionDetectionSupported API
* @tc.number  : IsSlowMotionDetectionSupported_002
* @tc.desc    : Test IsSlowMotionDetectionSupported interface, when camera device is null.
* @tc.require : slow motion only support videoStream & previewStrem & 1080p & 240fps
*/
HWTEST_F(CameraFrameworkUnitTest, IsSlowMotionDetectionSupported_002, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession(SceneMode::SLOW_MOTION);
    ASSERT_NE(session, nullptr);

    sptr<Surface> videoSurface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = 1920;
    videoSize.height = 1080;
    std::vector<int32_t> videoFramerates = {0, 0};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> videoOutput = cameraManager->CreateVideoOutput(videoProfile, videoSurface);
    ASSERT_NE(videoOutput, nullptr);

    sptr<Surface> previewSurface = Surface::CreateSurfaceAsConsumer();
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = 1920;
    previewSize.height = 1080;
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> previewOutput = cameraManager->CreatePreviewOutput(previewProfile, previewSurface);
    ASSERT_NE(previewOutput, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutputCaptureUpper = previewOutput;
    intResult = session->AddOutput(previewOutputCaptureUpper);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutputCaptureUpper = videoOutput;
    intResult = session->AddOutput(videoOutputCaptureUpper);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    intResult = input->Release();
    EXPECT_EQ(intResult, 0);

    sptr<SlowMotionSession> slowSession = nullptr;
    slowSession = static_cast<SlowMotionSession *> (session.GetRefPtr());
    bool result = slowSession->IsSlowMotionDetectionSupported();
    EXPECT_EQ(false, result);
}

/**
* @tc.name    : Test IsSlowMotionDetectionSupported API
* @tc.number  : IsSlowMotionDetectionSupported_003
* @tc.desc    : Test IsSlowMotionDetectionSupported interface, when metadata item not found.
*               Test IsSlowMotionDetectionSupported interface, when metadata item data is 0.
*               Test IsSlowMotionDetectionSupported interface, when metadata item data is 1.
* @tc.require : slow motion only support videoStream & previewStrem & 1080p & 240fps
*/
HWTEST_F(CameraFrameworkUnitTest, IsSlowMotionDetectionSupported_003, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);
    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession(SceneMode::SLOW_MOTION);
    ASSERT_NE(session, nullptr);

    sptr<Surface> videoSurface = Surface::CreateSurfaceAsConsumer();
    CameraFormat videoFormat = CAMERA_FORMAT_YUV_420_SP;
    Size videoSize;
    videoSize.width = 1920;
    videoSize.height = 1080;
    std::vector<int32_t> videoFramerates = {0, 0};
    VideoProfile videoProfile = VideoProfile(videoFormat, videoSize, videoFramerates);
    sptr<VideoOutput> videoOutput = cameraManager->CreateVideoOutput(videoProfile, videoSurface);
    ASSERT_NE(videoOutput, nullptr);

    sptr<Surface> previewSurface = Surface::CreateSurfaceAsConsumer();
    CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
    Size previewSize;
    previewSize.width = 1920;
    previewSize.height = 1080;
    Profile previewProfile = Profile(previewFormat, previewSize);
    sptr<CaptureOutput> previewOutput = cameraManager->CreatePreviewOutput(previewProfile, previewSurface);
    ASSERT_NE(previewOutput, nullptr);

    int32_t intResult = session->BeginConfig();
    EXPECT_EQ(intResult, 0);

    intResult = session->AddInput(input);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> previewOutputCaptureUpper = previewOutput;
    intResult = session->AddOutput(previewOutputCaptureUpper);
    EXPECT_EQ(intResult, 0);

    sptr<CaptureOutput> videoOutputCaptureUpper = videoOutput;
    intResult = session->AddOutput(videoOutputCaptureUpper);
    EXPECT_EQ(intResult, 0);

    intResult = session->CommitConfig();
    EXPECT_EQ(intResult, 0);

    sptr<SlowMotionSession> slowSession = static_cast<SlowMotionSession *> (session.GetRefPtr());
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata =
        slowSession->GetInputDevice()->GetCameraDeviceInfo()->GetMetadata();
    uint8_t value_u8 = 0;
    bool result = false;
    // metadata item not found.
    result = slowSession->IsSlowMotionDetectionSupported();
    EXPECT_EQ(false, result);
    // metadata item data is 0.
    metadata->addEntry(OHOS_ABILITY_MOTION_DETECTION_SUPPORT, &value_u8, sizeof(uint8_t));
    result = slowSession->IsSlowMotionDetectionSupported();
    EXPECT_EQ(false, result);
    value_u8 = 1;
    // metadata item data is 1.
    metadata->updateEntry(OHOS_ABILITY_MOTION_DETECTION_SUPPORT, &value_u8, sizeof(uint8_t));
    result = slowSession->IsSlowMotionDetectionSupported();
    EXPECT_EQ(true, result);
    Rect rect;
    rect.topLeftX = 0.1;
    rect.topLeftY = 0.1;
    rect.width = 0.8;
    rect.height = 0.8;
    slowSession->SetSlowMotionDetectionArea(rect);

    std::shared_ptr<TestSlowMotionStateCallback> callback = std::make_shared<TestSlowMotionStateCallback>();
    slowSession->SetCallback(callback);
    EXPECT_EQ(slowSession->GetApplicationCallback(), callback);
}
/*
 * Feature: Framework
 * Function: Test PanoramaSession preview
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test PanoramaSession preview
 */
HWTEST_F(CameraFrameworkUnitTest, camera_panorama_unittest_001, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        if (find(modes.begin(), modes.end(), SceneMode::PANORAMA_PHOTO) != modes.end()) {
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);

            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);
            camInput->GetCameraDevice()->Open();

            sptr<CaptureSession> session = cameraManager->CreateCaptureSession(SceneMode::PANORAMA_PHOTO);
            sptr<PanoramaSession> panoramaSession = static_cast<PanoramaSession *> (session.GetRefPtr());
            ASSERT_NE(panoramaSession, nullptr);

            sptr<Surface> previewSurface = Surface::CreateSurfaceAsConsumer();
            CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
            Size previewSize;
            previewSize.width = 1920;
            previewSize.height = 1080;
            Profile previewProfile = Profile(previewFormat, previewSize);
            sptr<CaptureOutput> previewOutput = cameraManager->CreatePreviewOutput(previewProfile, previewSurface);
            ASSERT_NE(previewOutput, nullptr);

            int32_t intResult = panoramaSession->BeginConfig();
            EXPECT_EQ(intResult, 0);

            intResult = panoramaSession->AddInput(input);
            EXPECT_EQ(intResult, 0);

            sptr<CaptureOutput> previewOutputCaptureUpper = previewOutput;
            intResult = panoramaSession->AddOutput(previewOutputCaptureUpper);
            EXPECT_EQ(intResult, 0);

            intResult = panoramaSession->CommitConfig();
            EXPECT_EQ(intResult, 0);

            panoramaSession->Release();
            camInput->Close();
        }
    }
}

/*
 * Feature: Framework
 * Function: Test PanoramaSession set white balance lock
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test PanoramaSession set white balance lock
 */
HWTEST_F(CameraFrameworkUnitTest, camera_panorama_unittest_002, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    for (sptr<CameraDevice> camDevice : cameras) {
        std::vector<SceneMode> modes = cameraManager->GetSupportedModes(camDevice);
        if (find(modes.begin(), modes.end(), SceneMode::PANORAMA_PHOTO) != modes.end()) {
            sptr<CaptureInput> input = cameraManager->CreateCameraInput(camDevice);
            ASSERT_NE(input, nullptr);

            sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
            std::string cameraSettings = camInput->GetCameraSettings();
            camInput->SetCameraSettings(cameraSettings);
            camInput->GetCameraDevice()->Open();

            sptr<CaptureSession> session = cameraManager->CreateCaptureSession(SceneMode::PANORAMA_PHOTO);
            sptr<PanoramaSession> panoramaSession = static_cast<PanoramaSession *> (session.GetRefPtr());
            ASSERT_NE(panoramaSession, nullptr);

            sptr<Surface> previewSurface = Surface::CreateSurfaceAsConsumer();
            CameraFormat previewFormat = CAMERA_FORMAT_YUV_420_SP;
            Size previewSize;
            previewSize.width = 1920;
            previewSize.height = 1080;
            Profile previewProfile = Profile(previewFormat, previewSize);
            sptr<CaptureOutput> previewOutput = cameraManager->CreatePreviewOutput(previewProfile, previewSurface);
            ASSERT_NE(previewOutput, nullptr);

            int32_t intResult = panoramaSession->BeginConfig();
            EXPECT_EQ(intResult, 0);

            intResult = panoramaSession->AddInput(input);
            EXPECT_EQ(intResult, 0);

            sptr<CaptureOutput> previewOutputCaptureUpper = previewOutput;
            intResult = panoramaSession->AddOutput(previewOutputCaptureUpper);
            EXPECT_EQ(intResult, 0);

            intResult = panoramaSession->CommitConfig();
            EXPECT_EQ(intResult, 0);

            panoramaSession->SetWhiteBalanceMode(AWB_MODE_LOCKED);
            WhiteBalanceMode mode;
            panoramaSession->GetWhiteBalanceMode(mode);
            EXPECT_EQ(AWB_MODE_LOCKED, mode);

            panoramaSession->Release();
            camInput->Close();
        }
    }
}

HWTEST_F(CameraFrameworkUnitTest, test_CheckFrameRateRangeWithCurrentFps, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(session, nullptr);
    ASSERT_EQ(session->CheckFrameRateRangeWithCurrentFps(30, 30, 30, 60), false);
    ASSERT_EQ(session->CheckFrameRateRangeWithCurrentFps(30, 30, 30, 60), false);
    ASSERT_EQ(session->CheckFrameRateRangeWithCurrentFps(20, 40, 20, 40), true);
    ASSERT_EQ(session->CheckFrameRateRangeWithCurrentFps(20, 40, 30, 60), false);
}

HWTEST_F(CameraFrameworkUnitTest, test_CanPreconfig, TestSize.Level0)
{
    sptr<CaptureSession> session = cameraManager->CreateCaptureSession(SceneMode::CAPTURE);
    ASSERT_NE(session, nullptr);
    PreconfigType preconfigType = PreconfigType::PRECONFIG_720P;
    ProfileSizeRatio preconfigRatio = ProfileSizeRatio::RATIO_16_9;
    EXPECT_EQ(session->CanPreconfig(preconfigType, preconfigRatio), true);
    int32_t result = session->Preconfig(preconfigType, preconfigRatio);
    EXPECT_EQ(result, 0);
}

/*
 * Feature: Framework
 * Function: Test cameraManager GetSupportedOutputCapability with yuv photo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test cameraManager GetSupportedOutputCapability with yuv photo
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_can_get_yuv_photo_profile, TestSize.Level0)
{
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();

    SceneMode mode = PORTRAIT;
    std::vector<SceneMode> modes = cameraManager->GetSupportedModes(cameras[0]);
    ASSERT_TRUE(modes.size() != 0);

    sptr<CameraOutputCapability> ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    vector<Profile> photoProfiles = ability->GetPhotoProfiles();
    auto it = std::find_if(photoProfiles.begin(), photoProfiles.end(),
        [](const auto& profile){ return profile.format_ == CAMERA_FORMAT_YUV_420_SP;});

    EXPECT_NE(it, photoProfiles.end());

    mode = SceneMode::CAPTURE;
    ability = cameraManager->GetSupportedOutputCapability(cameras[0], mode);
    ASSERT_NE(ability, nullptr);

    it = std::find_if(photoProfiles.begin(), photoProfiles.end(),
        [](const auto& profile){ return profile.format_ == CAMERA_FORMAT_YUV_420_SP;});
    EXPECT_NE(it, photoProfiles.end());
}

HWTEST_F(CameraFrameworkUnitTest, test_CreateBurstDisplayName, TestSize.Level0)
{
    uint32_t callerToken = IPCSkeleton::GetCallingTokenID();
    SceneMode opMode = CAPTURE;
    sptr<HCaptureSession> session = HCaptureSession::NewInstance(callerToken, opMode);
    std::string displayName = session->CreateBurstDisplayName(1, 1);
    cout << "displayName: " << displayName <<endl;
    ASSERT_NE(displayName, "");
    ASSERT_THAT(displayName, testing::EndsWith("_COVER"));
    displayName = session->CreateBurstDisplayName(2, 2);
    cout << "displayName: " << displayName <<endl;
    ASSERT_THAT(displayName, Not(testing::EndsWith("_COVER")));
    displayName = session->CreateBurstDisplayName(-1, -1);
    cout << "displayName: " << displayName <<endl;
}

/*
 * Feature: Framework
 * Function: Verify that the method returns the correct list of cameras when the device is not foldable.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Verify that the method returns the correct list of cameras when the device is not foldable.
 */
HWTEST_F(CameraFrameworkUnitTest, get_supported_cameras_not_foldable, TestSize.Level0)
{
    EXPECT_CALL(*mockCameraManager, GetIsFoldable())
        .WillRepeatedly(Return(false));
    std::vector<sptr<CameraDevice>> expectedCameraList;
    auto changedMetadata = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    auto camera0 = new CameraDevice("device0", changedMetadata);
    auto camera1 = new CameraDevice("device1", changedMetadata);
    auto camera2 = new CameraDevice("device2", changedMetadata);
    expectedCameraList.emplace_back(camera0);
    expectedCameraList.emplace_back(camera1);
    expectedCameraList.emplace_back(camera2);
    mockCameraManager->cameraDeviceList_ = expectedCameraList;
    auto result = mockCameraManager->GetSupportedCameras();
    ASSERT_EQ(result.size(), expectedCameraList.size());
}

/*
 * Feature: Framework
 * Function: The goal is to verify that the method correctly returns the list of supported cameras when the device is
              collapsible and in the "expand" state
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: The goal is to verify that the method correctly returns the list of supported cameras when the
               device is collapsible and in the "expand" state
 */
HWTEST_F(CameraFrameworkUnitTest, get_supported_cameras_foldable_expand01, TestSize.Level0)
{
    EXPECT_CALL(*mockCameraManager, GetIsFoldable())
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mockCameraManager, GetFoldStatus())
        .WillRepeatedly(Return(FoldStatus::EXPAND));

    auto changedMetadata = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    int32_t cameraPosition = CAMERA_POSITION_BACK;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    int32_t foldStatus = OHOS_CAMERA_FOLD_STATUS_EXPANDED | OHOS_CAMERA_FOLD_STATUS_FOLDED;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera0 = new CameraDevice("device0", changedMetadata);

    auto changedMetadata1 = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    cameraPosition = CAMERA_POSITION_FRONT;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    foldStatus = OHOS_CAMERA_FOLD_STATUS_EXPANDED;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera1 = new CameraDevice("device1", changedMetadata1);

    std::vector<sptr<CameraDevice>> expectedCameraList;
    expectedCameraList.emplace_back(camera0);
    expectedCameraList.emplace_back(camera1);
    mockCameraManager->cameraDeviceList_ = expectedCameraList;
    auto result = mockCameraManager->GetSupportedCameras();

    ASSERT_EQ(result.size(), 2);
}

/*
 * Feature: Framework
 * Function: The goal is to verify that the method correctly returns the list of supported cameras when the device is
              collapsible and in the "expand" state
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: The goal is to verify that the method correctly returns the list of supported cameras when the
               device is collapsible and in the "expand" state
 */
HWTEST_F(CameraFrameworkUnitTest, get_supported_cameras_foldable_expand02, TestSize.Level0)
{
    EXPECT_CALL(*mockCameraManager, GetIsFoldable())
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mockCameraManager, GetFoldStatus())
        .WillRepeatedly(Return(FoldStatus::EXPAND));

    auto changedMetadata = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    int32_t cameraPosition = CAMERA_POSITION_BACK;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    int32_t foldStatus = OHOS_CAMERA_FOLD_STATUS_EXPANDED | OHOS_CAMERA_FOLD_STATUS_FOLDED;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera0 = new CameraDevice("device0", changedMetadata);

    auto changedMetadata1 = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    cameraPosition = CAMERA_POSITION_FRONT;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    foldStatus = OHOS_CAMERA_FOLD_STATUS_FOLDED;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera1 = new CameraDevice("device1", changedMetadata1);

    std::vector<sptr<CameraDevice>> expectedCameraList;
    expectedCameraList.emplace_back(camera0);
    expectedCameraList.emplace_back(camera1);
    mockCameraManager->cameraDeviceList_ = expectedCameraList;
    auto result = mockCameraManager->GetSupportedCameras();

    ASSERT_EQ(result.size(), 1);
}

/*
 * Feature: Framework
 * Function: In the scenario where the device is foldable and in a folded state, the goal is to ensure that the method
             correctly returns the list of cameras that support the current folded state.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: In the scenario where the device is foldable and in a folded state, the goal is to ensure that the
              method correctly returns the list of cameras that support the current folded state.
 */
HWTEST_F(CameraFrameworkUnitTest, get_supported_cameras_foldable_fold, TestSize.Level0)
{
    EXPECT_CALL(*mockCameraManager, GetIsFoldable())
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mockCameraManager, GetFoldStatus())
        .WillRepeatedly(Return(FoldStatus::FOLDED));

    auto changedMetadata = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    int32_t cameraPosition = CAMERA_POSITION_BACK;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    int32_t foldStatus = OHOS_CAMERA_FOLD_STATUS_EXPANDED | OHOS_CAMERA_FOLD_STATUS_FOLDED;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera0 = new CameraDevice("device0", changedMetadata);

    auto changedMetadata1 = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    cameraPosition = CAMERA_POSITION_FRONT;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    foldStatus = OHOS_CAMERA_FOLD_STATUS_FOLDED;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera1 = new CameraDevice("device1", changedMetadata1);

    std::vector<sptr<CameraDevice>> expectedCameraList;
    expectedCameraList.emplace_back(camera0);
    expectedCameraList.emplace_back(camera1);
    mockCameraManager->cameraDeviceList_ = expectedCameraList;
    auto result = mockCameraManager->GetSupportedCameras();

    ASSERT_EQ(result.size(), 2);
}

/*
 * Feature: Framework
 * Function: The unit test get_supported_cameras_foldable_half_fold checks the behavior of the
             CameraManager::GetSupportedCameras method when the device is foldable and in a half-folded state.
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: The unit test get_supported_cameras_foldable_half_fold checks the behavior of the
             CameraManager::GetSupportedCameras method when the device is foldable and in a half-folded state.
 */
HWTEST_F(CameraFrameworkUnitTest, get_supported_cameras_foldable_half_fold, TestSize.Level0)
{
    EXPECT_CALL(*mockCameraManager, GetIsFoldable())
        .WillRepeatedly(Return(true));

    EXPECT_CALL(*mockCameraManager, GetFoldStatus())
        .WillRepeatedly(Return(FoldStatus::HALF_FOLD));

    auto changedMetadata = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    int32_t cameraPosition = CAMERA_POSITION_BACK;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    int32_t foldStatus = OHOS_CAMERA_FOLD_STATUS_EXPANDED | OHOS_CAMERA_FOLD_STATUS_FOLDED;
    changedMetadata->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera0 = new CameraDevice("device0", changedMetadata);

    auto changedMetadata1 = std::make_shared<OHOS::Camera::CameraMetadata>(0, 0);
    cameraPosition = CAMERA_POSITION_FRONT;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_POSITION, &cameraPosition, 1);
    foldStatus = OHOS_CAMERA_FOLD_STATUS_FOLDED;
    changedMetadata1->addEntry(OHOS_ABILITY_CAMERA_FOLD_STATUS, &foldStatus, 1);
    auto camera1 = new CameraDevice("device1", changedMetadata1);

    std::vector<sptr<CameraDevice>> expectedCameraList;
    expectedCameraList.emplace_back(camera0);
    expectedCameraList.emplace_back(camera1);
    mockCameraManager->cameraDeviceList_ = expectedCameraList;
    auto result = mockCameraManager->GetSupportedCameras();
    ASSERT_EQ(result.size(), 1);
}

typedef struct {
    uv_loop_t* loop;
    uv_work_t* workReq;
} Param;

enum class PhotoOutputEventType : int32_t {
    CAPTURE_START = 1,
    CAPTURE_FRAME_SHUTTER = 2,
};

static bool g_semInit = false;
static uv_sem_t g_pause_sems;
std::vector<int32_t> g_eventOrder {};

static void WorkCb(uv_work_t* req)
{
    if (req == nullptr) {
        return;
    }
    int32_t ret = 0;
    PhotoOutputEventType data = *static_cast<PhotoOutputEventType*>(req->data);
    if (data == PhotoOutputEventType::CAPTURE_START) {
    } else if (data == PhotoOutputEventType::CAPTURE_FRAME_SHUTTER) {
        ret = fprintf(stdout, "WorkCb uv_sem_wait before data:%d\n", data);
        EXPECT_GE(ret, 0);
        uv_sem_wait(&g_pause_sems);
        ret = fprintf(stdout, "WorkCb uv_sem_wait end data:%d\n", data);
        EXPECT_GE(ret, 0);
    }
    ret = fprintf(stdout, "WorkCb data:%d, thread_id:%lu, end\n", data, uv_thread_self());
    EXPECT_GE(ret, 0);
    ret = fflush(stdout);
    EXPECT_EQ(ret, 0);
}

static void AfterWorkCb(uv_work_t* req, int status)
{
    if (req == nullptr) {
        return;
    }
    int32_t ret = 0;
    PhotoOutputEventType data = *static_cast<PhotoOutputEventType*>(req->data);
    if (data == PhotoOutputEventType::CAPTURE_START) {
        uv_sem_post(&g_pause_sems);
        ret = fprintf(stdout, "AfterWorkCb uv_sem_post data:%d\n", data);
        EXPECT_GE(ret, 0);
    } else if (data == PhotoOutputEventType::CAPTURE_FRAME_SHUTTER) {
        uv_sem_destroy(&g_pause_sems);
        g_semInit = false;
        ret = fprintf(stdout, "AfterWorkCb uv_sem_destroy data:%d\n", data);
        EXPECT_GE(ret, 0);
    }
    g_eventOrder.push_back(static_cast<int32_t>(data));
    ret = fprintf(stdout, "AfterWorkCb data:%d, thread_id:%lu, end\n", data, uv_thread_self());
    EXPECT_GE(ret, 0);
    ret = fflush(stdout);
    EXPECT_EQ(ret, 0);
}

void ThreadFunc(void* arg)
{
    if (arg == nullptr) {
        return;
    }
    Param* param = static_cast<Param*>(arg);
    if (!g_semInit) {
        uv_sem_init(&g_pause_sems, 0);
        g_semInit = true;
    }
    if (!param->workReq) {
        return;
    }
    int32_t ret = 0;
    PhotoOutputEventType eventType = *static_cast<PhotoOutputEventType*>(param->workReq->data);
    ret = fprintf(stdout, "ThreadFunc event:%d\n", eventType);
    EXPECT_GE(ret, 0);
    ret = uv_queue_work(param->loop, param->workReq, WorkCb, AfterWorkCb);
    EXPECT_EQ(ret, 0);
}

/*
 * Feature: Framework
 * Function: Test using libuv semaphores to sync after work callback
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test using libuv semaphores to sync after work callback
 */
HWTEST_F(CameraFrameworkUnitTest, camera_framework_unittest_libuv_sem_sync, TestSize.Level0)
{
    uv_loop_t* loop = uv_default_loop();
    PhotoOutputEventType captureStartEvent = PhotoOutputEventType::CAPTURE_START;
    PhotoOutputEventType frameShutterEvent = PhotoOutputEventType::CAPTURE_FRAME_SHUTTER;
    uv_work_t workReqCaptureStart { .data = &captureStartEvent, };
    uv_work_t workReqFrameShutter { .data = &frameShutterEvent, };
    Param paramCaptureStart {
        .loop = loop,
        .workReq = &workReqCaptureStart,
    };
    Param paramFrameShutter {
        .loop = loop,
        .workReq = &workReqFrameShutter,
    };
    uv_thread_t threadCaptureStart;
    uv_thread_t threadFrameShutter;
    uv_thread_create(&threadFrameShutter, ThreadFunc, &paramFrameShutter);
    uv_thread_create(&threadCaptureStart, ThreadFunc, &paramCaptureStart);
    uv_thread_join(&threadFrameShutter);
    uv_thread_join(&threadCaptureStart);
    EXPECT_EQ(uv_run(loop, UV_RUN_DEFAULT), 0);
    sleep(1);
    std::vector<int32_t> expectOrder {
        static_cast<int32_t>(PhotoOutputEventType::CAPTURE_START),
        static_cast<int32_t>(PhotoOutputEventType::CAPTURE_FRAME_SHUTTER)
    };
    EXPECT_EQ(expectOrder, g_eventOrder);
}
} // CameraStandard
} // OHOS
