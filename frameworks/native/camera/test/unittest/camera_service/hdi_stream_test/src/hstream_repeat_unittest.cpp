/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "camera_log.h"
#include "camera_util.h"
#include "gmock/gmock.h"
#include "hstream_repeat_unittest.h"
#include "stream_repeat_callback_stub.h"
#include "stream_repeat_stub.h"
#include "surface_type.h"
#include "test_common.h"
#include "test_token.h"

using namespace testing::ext;
using ::testing::A;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Return;
using ::testing::_;
namespace OHOS {
namespace CameraStandard {
constexpr static uint32_t CAMERA_STREAM_REPEAT_ON_DEFAULT = 1;
const uint32_t CONST_1 = 1;
const uint32_t CONST_0 = 0;
const uint32_t PHOTO_MODE = 1;
const uint32_t NUM_ONE = 1;
const uint32_t METADATA_ITEM_SIZE = 20;
const uint32_t METADATA_DATA_SIZE = 200;
using namespace OHOS::HDI::Camera::V1_1;
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
    virtual ~MockStreamOperator() {}
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

class MockHStreamRepeatStub : public StreamRepeatStub {
public:
    MOCK_METHOD0(Start, int32_t());
    MOCK_METHOD0(Stop, int32_t());
    MOCK_METHOD1(SetCallback, int32_t(const sptr<IStreamRepeatCallback>& callback));
    MOCK_METHOD0(UnSetCallback, int32_t());
    MOCK_METHOD0(Release, int32_t());
    MOCK_METHOD1(AddDeferredSurface, int32_t(const sptr<OHOS::IBufferProducer>& producer));
    MOCK_METHOD4(ForkSketchStreamRepeat, int32_t(
        int32_t width, int32_t height, sptr<IRemoteObject>& sketchStream, float sketchRatio));
    MOCK_METHOD1(UpdateSketchRatio, int32_t(float sketchRatio));
    MOCK_METHOD0(RemoveSketchStreamRepeat, int32_t());
    MOCK_METHOD2(SetFrameRate, int32_t(int32_t minFrameRate, int32_t maxFrameRate));
    MOCK_METHOD1(EnableSecure, int32_t(bool isEnable));
    MOCK_METHOD1(SetMirror, int32_t(bool isEnable));
    MOCK_METHOD2(AttachMetaSurface, int32_t(const sptr<OHOS::IBufferProducer>& producer, int32_t videoMetaType));
    MOCK_METHOD2(SetCameraRotation, int32_t(bool isEnable, int32_t rotation));
    MOCK_METHOD1(OperatePermissionCheck, int32_t(uint32_t interfaceCode));
    MOCK_METHOD1(CallbackEnter, int32_t(uint32_t code));
    MOCK_METHOD2(CallbackExit, int32_t(uint32_t code, int32_t result));
    MOCK_METHOD1(GetMirror, int32_t(bool& isEnable));
    MOCK_METHOD1(SetCameraApi, int32_t(uint32_t apiCompatibleVersion));
    virtual ~MockHStreamRepeatStub() {}
};

class MockHStreamRepeatCallbackStub : public StreamRepeatCallbackStub {
public:
    MOCK_METHOD0(OnFrameStarted, int32_t());
    MOCK_METHOD1(OnFrameEnded, int32_t(int32_t frameCount));
    MOCK_METHOD1(OnFrameError, int32_t(int32_t errorCode));
    MOCK_METHOD1(OnSketchStatusChanged, int32_t(SketchStatus status));
    MOCK_METHOD1(OnDeferredVideoEnhancementInfo, int32_t(const CaptureEndedInfoExt& captureEndedInfo));
    MockHStreamRepeatCallbackStub()
    {
        ON_CALL(*this, OnFrameStarted()).WillByDefault(Return(CAMERA_OK));
        ON_CALL(*this, OnDeferredVideoEnhancementInfo(_)).WillByDefault(Return(CAMERA_OK));
        ON_CALL(*this, OnSketchStatusChanged(_)).WillByDefault(Return(CAMERA_OK));
    }
    virtual ~MockHStreamRepeatCallbackStub() {}
};

void HStreamRepeatUnit::SetUpTestCase(void)
{
    ASSERT_TRUE(TestToken().GetAllCameraPermission());
}

void HStreamRepeatUnit::TearDownTestCase(void) {}

void HStreamRepeatUnit::TearDown(void) {}

void HStreamRepeatUnit::SetUp(void) {}

HStreamRepeat* HStreamRepeatUnit::CreateHStreamRepeat(RepeatStreamType type)
{
    sptr<OHOS::IConsumerSurface> cSurface = IConsumerSurface::Create();
    sptr<OHOS::IBufferProducer> producer = cSurface->GetProducer();
    int32_t format = 1;
    int32_t width = 1920;
    int32_t height = 1080;
    auto streamRepeat = new (std::nothrow) HStreamRepeat(producer, format, width, height, type);
    return streamRepeat;
}

sptr<CaptureOutput> HStreamRepeatUnit::CreatePhotoOutput(Profile& photoProfile)
{
    sptr<IConsumerSurface> surface = IConsumerSurface::Create();
    if (surface == nullptr) {
        return nullptr;
    }
    sptr<IBufferProducer> surfaceProducer = surface->GetProducer();
    return CameraManager::GetInstance()->CreatePhotoOutput(photoProfile, surfaceProducer);
}

/*
 * Feature: Framework
 * Function: Test SetStreamInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the SetStreamInfo function when mEnableSecure is true.
 *    The extendedStreamInfos should not be empty after setting stream info.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    StreamInfo_V1_1 streamInfo;
    streamRepeat->repeatStreamType_ = RepeatStreamType::PREVIEW;
    streamRepeat->mEnableSecure = true;
    streamRepeat->SetStreamInfo(streamInfo);
    EXPECT_FALSE(streamInfo.extendedStreamInfos.empty());
}

/*
 * Feature: Framework
 * Function: Test SetStreamInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the SetStreamInfo function when mEnableSecure is false.
 *    The extendedStreamInfos should be empty after setting stream info.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    StreamInfo_V1_1 streamInfo;
    streamRepeat->repeatStreamType_ = RepeatStreamType::PREVIEW;
    streamRepeat->mEnableSecure = false;
    streamRepeat->SetStreamInfo(streamInfo);
    EXPECT_TRUE(streamInfo.extendedStreamInfos.empty());
}

/*
 * Feature: Framework
 * Function: Test UpdateSketchStatus
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the UpdateSketchStatus function. When the sketch status is updated to STOPED,
 *    the parentStreamRepeat should remain unchanged.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_003, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    SketchStatus status = SketchStatus::STOPED;
    streamRepeat->repeatStreamType_ = RepeatStreamType::SKETCH;
    streamRepeat->sketchStatus_ = SketchStatus::STARTED;
    streamRepeat->mEnableSecure = false;
    streamRepeat->UpdateSketchStatus(status);
    EXPECT_EQ(streamRepeat->parentStreamRepeat_.promote(), nullptr);
}

/*
 * Feature: Framework
 * Function: Test Start
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the Start function. When the stream is not in a valid state,
 *    the expected return value is CAMERA_INVALID_STATE.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_004, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> settings;
    bool isUpdateSeetings = false;
    int32_t ret = streamRepeat->Start(settings, isUpdateSeetings);
    EXPECT_EQ(ret, CAMERA_INVALID_STATE);
}

/*
 * Feature: Framework
 * Function: Test ReleaseStream
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the ReleaseStream function.
 *    When the stream is released without delay, the expected return value is CAMERA_OK.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_005, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    bool isDelay = false;
    int32_t ret = streamRepeat->ReleaseStream(isDelay);
    EXPECT_EQ(ret, CAMERA_OK);
}

/*
 * Feature: Framework
 * Function: Test OnDeferredVideoEnhancementInfo
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the OnDeferredVideoEnhancementInfo function.
 * When the deferred video enhancement info is received,
 *    the expected return value is CAMERA_OK.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_006, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    CaptureEndedInfoExt captureEndedInfo = {1, 100, true, "video123"};
    streamRepeat->repeatStreamType_ = RepeatStreamType::PREVIEW;
    int32_t ret = streamRepeat->OnDeferredVideoEnhancementInfo(captureEndedInfo);
    EXPECT_EQ(ret, CAMERA_OK);
}

/*
 * Feature: Framework
 * Function: Test OnFrameError
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test the OnFrameError function. When a frame error of type HIGH_TEMPERATURE_ERROR is reported,
 *    the expected return value is CAMERA_OK.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_007, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t errorType = HDI::Camera::V1_3::HIGH_TEMPERATURE_ERROR;
    int32_t ret = streamRepeat->OnFrameError(errorType);
    EXPECT_EQ(ret, CAMERA_OK);
}

/*
 * Feature: Framework
 * Function: Test SetMirrorForLivePhoto
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetMirrorForLivePhoto when the mirror mode is supported.
 *    The expected return value is CAM_META_SUCCESS.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_008, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    uint8_t dfxSwitch = true;
    bool isEnable = true;
    camera_metadata_item_t item;
    int32_t mode = 110;
    int32_t res;
    streamRepeat->cameraAbility_->addEntry(OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &dfxSwitch, NUM_ONE);
    streamRepeat->SetMirrorForLivePhoto(isEnable, mode);
    res = OHOS::Camera::FindCameraMetadataItem(streamRepeat->cameraAbility_->get(),
        OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
    EXPECT_EQ(res, CAM_META_SUCCESS);
}

/*
 * Feature: Framework
 * Function: Test SetMirrorForLivePhoto
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetMirrorForLivePhoto when the mirror mode is not supported.
 *    The expected return value is CAM_META_ITEM_NOT_FOUND.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_009, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    bool isEnable = true;
    camera_metadata_item_t item;
    int32_t mode = 110;
    int32_t res;
    streamRepeat->SetMirrorForLivePhoto(isEnable, mode);
    res = OHOS::Camera::FindCameraMetadataItem(streamRepeat->cameraAbility_->get(),
        OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
    EXPECT_EQ(res, CAM_META_ITEM_NOT_FOUND);
}

/*
 * Feature: Framework
 * Function: Test SetStreamTransform
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test SetStreamTransform for front camera.
 *    The expected camera position remains OHOS_CAMERA_POSITION_FRONT.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_010, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    streamRepeat->cameraUsedAsPosition_ = OHOS_CAMERA_POSITION_FRONT;
    int disPlayRotation = -1;
    streamRepeat->SetStreamTransform(disPlayRotation);
    EXPECT_TRUE(streamRepeat->cameraUsedAsPosition_ == OHOS_CAMERA_POSITION_FRONT);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraSetRotation
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraSetRotation for front camera with 180-degree sensor orientation.
 *    The expected sensor orientation is 0.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_011, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->setCameraRotation_ = STREAM_ROTATE_180;
    int32_t sensorOrientation = STREAM_ROTATE_180;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessCameraSetRotation(sensorOrientation, cameraPosition);
    EXPECT_EQ(sensorOrientation, STREAM_ROTATE_0);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraSetRotation
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraSetRotation for back camera with 90-degree sensor orientation.
 *    The expected sensor orientation is 270.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_012, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_90;
    streamRepeat->setCameraRotation_ = STREAM_ROTATE_90;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessCameraSetRotation(sensorOrientation, cameraPosition);
    EXPECT_EQ(sensorOrientation, STREAM_ROTATE_270);
}

/*
 * Feature: Framework
 * Function: Test ProcessVerticalCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessVerticalCameraPosition for front camera with 90-degree sensor orientation.
 *    The expected transform is GRAPHIC_FLIP_H_ROT90.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_013, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_90;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H_ROT90;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessVerticalCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessVerticalCameraPosition for front camera with 180-degree sensor orientation.
 *    The expected transform is GRAPHIC_FLIP_H_ROT180.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_014, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_180;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H_ROT180;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessVerticalCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessVerticalCameraPosition for front camera with 360-degree sensor orientation.
 *    The expected transform is GRAPHIC_FLIP_H_ROT90.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_015, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_360;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H_ROT90;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/**
 * @tc.name  : Test ProcessVerticalCameraPosition API
 * @tc.number: ProcessVerticalCameraPosition_001
 * @tc.desc  : Test ProcessVerticalCameraPosition API, when streamRotation is STREAM_ROTATE_0 and cameraPosition is
 * OHOS_CAMERA_POSITION_FRONT
 */
HWTEST_F(HStreamRepeatUnit, ProcessVerticalCameraPosition_001, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/**
 * @tc.name  : Test ProcessVerticalCameraPosition API
 * @tc.number: ProcessVerticalCameraPosition_002
 * @tc.desc  : Test ProcessVerticalCameraPosition API, when streamRotation is STREAM_ROTATE_270 and cameraPosition
 * is OHOS_CAMERA_POSITION_FRONT
 */
HWTEST_F(HStreamRepeatUnit, ProcessVerticalCameraPosition_002, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_270;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H_ROT270;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/**
 * @tc.name  : Test ProcessVerticalCameraPosition API
 * @tc.number: ProcessVerticalCameraPosition_003
 * @tc.desc  : Test ProcessVerticalCameraPosition API, when streamRotation is STREAM_ROTATE_270 and cameraPosition
 * is't OHOS_CAMERA_POSITION_FRONT
 */
HWTEST_F(HStreamRepeatUnit, ProcessVerticalCameraPosition_003, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_270;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_270;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessVerticalCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessVerticalCameraPosition for back camera with 360-degree sensor orientation.
 *    The expected transform is GRAPHIC_ROTATE_NONE.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_016, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_360;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessVerticalCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessVerticalCameraPosition for back camera with 270-degree sensor orientation.
 *    The expected transform is GRAPHIC_ROTATE_90.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_017, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_270;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_90;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessVerticalCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessVerticalCameraPosition for back camera with 180-degree sensor orientation.
 *    The expected transform is GRAPHIC_ROTATE_180.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_018, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_180;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_180;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessVerticalCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessVerticalCameraPosition for back camera with 0-degree sensor orientation.
 *    The expected transform is GRAPHIC_ROTATE_180.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_019, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessVerticalCameraPosition(sensorOrientation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_180;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for front camera with 0-degree stream rotation.
 *    The expected transform is GRAPHIC_FLIP_H.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_020, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for front camera with 90-degree stream rotation.
 *    The expected transform is GRAPHIC_FLIP_H_ROT90.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_021, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_90;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H_ROT90;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for front camera with 180-degree stream rotation.
 *    The expected transform is GRAPHIC_FLIP_H_ROT180.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_022, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_180;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H_ROT180;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for front camera with 270-degree stream rotation.
 *    The expected transform is GRAPHIC_FLIP_H_ROT270.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_023, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_270;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_FLIP_H_ROT270;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for front camera with 360-degree stream rotation.
 *    The expected transform is GRAPHIC_ROTATE_NONE.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_024, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_360;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for back camera with 0-degree stream rotation.
 *    The expected transform is GRAPHIC_ROTATE_NONE.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_025, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for back camera with 90-degree stream rotation.
 *    The expected transform is GRAPHIC_ROTATE_90.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_026, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_90;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_90;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for back camera with 180-degree stream rotation.
 *    The expected transform is GRAPHIC_ROTATE_180.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_027, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_180;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_180;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for back camera with 270-degree stream rotation.
 *    The expected transform is GRAPHIC_ROTATE_270.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_028, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_270;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_270;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test ProcessCameraPosition
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test ProcessCameraPosition for back camera with 360-degree stream rotation.
 *    The expected transform is GRAPHIC_ROTATE_NONE.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_029, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t streamRotation = STREAM_ROTATE_360;
    camera_position_enum_t cameraPosition = OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessCameraPosition(streamRotation, cameraPosition);
    GraphicTransformType transform = GraphicTransformType::GRAPHIC_ROTATE_NONE;
    EXPECT_EQ(streamRepeat->producer_->GetTransform(transform), GSERROR_OK);
}

/*
 * Feature: Framework
 * Function: Test OperatePermissionCheck
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OperatePermissionCheck for an unauthorized caller token.
 *    The expected return value is CAMERA_OPERATION_NOT_ALLOWED.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_030, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    uint32_t interfaceCode = static_cast<uint32_t>(IStreamRepeatIpcCode::COMMAND_FORK_SKETCH_STREAM_REPEAT);
    streamRepeat->callerToken_ = 111;
    int32_t ret = streamRepeat->OperatePermissionCheck(interfaceCode);
    EXPECT_EQ(ret, CAMERA_OPERATION_NOT_ALLOWED);
}

/**
 * @tc.name  : Test OperatePermissionCheck API
 * @tc.number: OperatePermissionCheck_001
 * @tc.desc  : Test OperatePermissionCheck API, when interfaceCode is invalid
 */
HWTEST_F(HStreamRepeatUnit, OperatePermissionCheck_001, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    uint32_t interfaceCode = static_cast<uint32_t>(IStreamRepeatIpcCode::COMMAND_FORK_SKETCH_STREAM_REPEAT) + 1;
    int32_t ret = streamRepeat->OperatePermissionCheck(interfaceCode);
    EXPECT_EQ(ret, CAMERA_OK);
}

/*
 * Feature: Framework
 * Function: Test OperatePermissionCheck
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OpenVideoDfxSwitch to ensure the debug switch is set correctly in camera ability metadata.
 *    The expected return value is CAMERA_OK.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_031, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    streamRepeat->OpenVideoDfxSwitch(cameraAbility);
    streamRepeat->OpenVideoDfxSwitch(cameraAbility);
    camera_metadata_item_t item;
    int32_t ret = OHOS::Camera::FindCameraMetadataItem(cameraAbility->get(),
        OHOS_CONTROL_VIDEO_DEBUG_SWITCH, &item);
    EXPECT_EQ(ret, CAMERA_OK);
}

/*
 * Feature: coverage
 * Function: Test anomalous branch
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat with no static capability.
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_032, TestSize.Level1)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<CameraManager> cameraManager = CameraManager::GetInstance();
    ASSERT_NE(cameraManager, nullptr);
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    ASSERT_FALSE(cameras.empty());

    sptr<CaptureInput> input = cameraManager->CreateCameraInput(cameras[0]);
    ASSERT_NE(input, nullptr);

    sptr<CameraInput> camInput = (sptr<CameraInput> &)input;
    std::string cameraSettings = camInput->GetCameraSettings();
    camInput->SetCameraSettings(cameraSettings);
    camInput->GetCameraDevice()->Open();
    sptr<CameraOutputCapability> outputCapability =
        cameraManager->GetSupportedOutputCapability(cameras[CONST_0], PHOTO_MODE);
    ASSERT_NE(outputCapability, nullptr);
    auto photoProfiles = outputCapability->GetPhotoProfiles();
    ASSERT_FALSE(photoProfiles.empty());
    sptr<CaptureOutput> photo = CreatePhotoOutput(photoProfiles[CONST_0]);
    ASSERT_NE(photo, nullptr);

    sptr<CaptureOutput> metadatOutput = cameraManager->CreateMetadataOutput();
    ASSERT_NE(metadatOutput, nullptr);

    sptr<CaptureSession> session = cameraManager->CreateCaptureSession();
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
    sptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator;
    if (streamRepeat->LinkInput(streamOperator, photoSetting->GetCaptureMetadataSetting()) != 0) {
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
 * Feature: Framework
 * Function: Test HStreamRepeat & HStreamCommon
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat & HStreamCommon
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_033, TestSize.Level1)
{
    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    CameraInfoDumper infoDumper(0);
    sptr<CameraManager> cameraManager = CameraManager::GetInstance();
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
    EXPECT_EQ(streamRepeat->AddDeferredSurface(producer), CAMERA_INVALID_ARG);
    streamRepeat->DumpStreamInfo(infoDumper);
    EXPECT_EQ(streamRepeat->AddDeferredSurface(producer1), CAMERA_INVALID_STATE);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata =
        std::make_shared<OHOS::Camera::CameraMetadata>(METADATA_ITEM_SIZE, METADATA_DATA_SIZE);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata1 = nullptr;
    sptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator = new MockStreamOperator();
    EXPECT_EQ(streamRepeat->LinkInput(streamOperator, metadata), CAMERA_OK);
    streamRepeat->LinkInput(streamOperator, metadata1);
    streamOperator = nullptr;
    streamRepeat->LinkInput(streamOperator, metadata);
    streamRepeat->DumpStreamInfo(infoDumper);
    EXPECT_EQ(streamRepeat->Stop(), CAMERA_INVALID_STATE);
}

/**
 * @tc.name  : Test LinkInput API
 * @tc.number: LinkInput_001
 * @tc.desc  : Test LinkInput API, when repeatStreamType_ is VIDEO
 */
HWTEST_F(HStreamRepeatUnit, LinkInput_001, TestSize.Level0)
{
    int32_t format = 0;
    int32_t width = 0;
    int32_t height = 0;
    CameraInfoDumper infoDumper(0);
    sptr<CameraManager> cameraManager = CameraManager::GetInstance();
    std::vector<sptr<CameraDevice>> cameras = cameraManager->GetSupportedCameras();
    sptr<IStreamRepeatCallback> callback = nullptr;
    sptr<OHOS::IBufferProducer> producer = nullptr;
    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer1 = Surface->GetProducer();
    sptr<HStreamRepeat> streamRepeat =
        new (std::nothrow) HStreamRepeat(nullptr, format, width, height, RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);

    EXPECT_EQ(streamRepeat->Start(), CAMERA_INVALID_STATE);
    EXPECT_EQ(streamRepeat->SetCallback(callback), CAMERA_INVALID_ARG);
    EXPECT_EQ(streamRepeat->AddDeferredSurface(producer), CAMERA_INVALID_ARG);
    streamRepeat->DumpStreamInfo(infoDumper);
    EXPECT_EQ(streamRepeat->AddDeferredSurface(producer1), CAMERA_INVALID_STATE);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata =
        std::make_shared<OHOS::Camera::CameraMetadata>(METADATA_ITEM_SIZE, METADATA_DATA_SIZE);
    std::shared_ptr<OHOS::Camera::CameraMetadata> metadata1 = nullptr;
    sptr<OHOS::HDI::Camera::V1_0::IStreamOperator> streamOperator = new MockStreamOperator();
    EXPECT_EQ(streamRepeat->LinkInput(streamOperator, metadata), CAMERA_OK);
    streamRepeat->LinkInput(streamOperator, metadata1);
    streamOperator = nullptr;
    streamRepeat->LinkInput(streamOperator, metadata);
    streamRepeat->DumpStreamInfo(infoDumper);
    EXPECT_EQ(streamRepeat->Stop(), CAMERA_INVALID_STATE);
}

/*
 * Feature: coverage
 * Function: Test HStreamRepeat
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_034, TestSize.Level1)
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
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_035, TestSize.Level1)
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
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_036, TestSize.Level1)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow)
        HStreamRepeat(producer, format, width, height, RepeatStreamType::PREVIEW);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<IRemoteObject> sketchStream = nullptr;
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
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_037, TestSize.Level1)
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
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_038, TestSize.Level1)
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
 * Function: Test HStreamRepeat when repeatStreamType_ is SKETCH
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test HStreamRepeat when repeatStreamType_ is SKETCH
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_039, TestSize.Level1)
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

/**
 * @tc.name  : Test NotifyDeviceStateChangeInfo API
 * @tc.number: NotifyDeviceStateChangeInfo_001
 * @tc.desc  : Test NotifyDeviceStateChangeInfo API, when repeatStreamType_ is LIVEPHOTO
 */
HWTEST_F(HStreamRepeatUnit, SetStreamInfo_001, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow) HStreamRepeat(producer, format, width, height, RepeatStreamType::LIVEPHOTO);
    ASSERT_NE(streamRepeat, nullptr);
    StreamInfo_V1_1 streamInfo;
    streamRepeat->SetStreamInfo(streamInfo);
}

/**
 * @tc.name  : Test NotifyDeviceStateChangeInfo API
 * @tc.number: NotifyDeviceStateChangeInfo_002
 * @tc.desc  : Test NotifyDeviceStateChangeInfo API, when repeatStreamType_ is invalid
 */
HWTEST_F(HStreamRepeatUnit, SetStreamInfo_002, TestSize.Level0)
{
    int32_t format = CAMERA_FORMAT_YUV_420_SP;
    int32_t width = PHOTO_DEFAULT_WIDTH;
    int32_t height = PHOTO_DEFAULT_HEIGHT;

    sptr<IConsumerSurface> Surface = IConsumerSurface::Create();
    sptr<IBufferProducer> producer = Surface->GetProducer();
    auto streamRepeat = new (std::nothrow) HStreamRepeat(producer, format, width, height,
        static_cast<RepeatStreamType>(static_cast<int32_t>(RepeatStreamType::LIVEPHOTO) + 1));
    ASSERT_NE(streamRepeat, nullptr);
    StreamInfo_V1_1 streamInfo;
    streamRepeat->SetStreamInfo(streamInfo);
}

/**
 * @tc.name  : Test StartSketchStream API
 * @tc.number: StartSketchStream_001
 * @tc.desc  : Test StartSketchStream API, when sketchStreamRepeat_ is nullptr
 */
HWTEST_F(HStreamRepeatUnit, StartSketchStream_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->sketchStreamRepeat_ = nullptr;
    auto settings = std::make_shared<CameraMetadata>(10, 100);
    streamRepeat->StartSketchStream(settings);
}

/**
 * @tc.name  : Test StartSketchStream API
 * @tc.number: StartSketchStream_002
 * @tc.desc  : Test StartSketchStream API, when OHOS_CONTROL_ZOOM_RATIO is't found
 */
HWTEST_F(HStreamRepeatUnit, StartSketchStream_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    auto streamRepeatSketch = CreateHStreamRepeat(RepeatStreamType::SKETCH);
    streamRepeat->sketchStreamRepeat_ = streamRepeatSketch;
    auto settings = std::make_shared<CameraMetadata>(10, 100);
    streamRepeat->StartSketchStream(settings);
}

/**
 * @tc.name  : Test StartSketchStream API
 * @tc.number: StartSketchStream_003
 * @tc.desc  : Test StartSketchStream API, when tagRatio is less than sketchStreamRepeat
 */
HWTEST_F(HStreamRepeatUnit, StartSketchStream_003, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    auto streamRepeatSketch = CreateHStreamRepeat(RepeatStreamType::SKETCH);
    streamRepeatSketch->sketchRatio_ = 2;
    streamRepeat->sketchStreamRepeat_ = streamRepeatSketch;
    auto settings = std::make_shared<CameraMetadata>(10, 100);
    float zoomRatio = 1;
    settings->addEntry(OHOS_CONTROL_ZOOM_RATIO, &zoomRatio, CONST_1);
    ASSERT_FALSE(streamRepeatSketch->sketchRatio_ > 0 &&
        zoomRatio - streamRepeatSketch->sketchRatio_ >= -std::numeric_limits<float>::epsilon());
    streamRepeat->StartSketchStream(settings);
}

/*
 * Feature: Framework
 * Function: Test HStreamMetadataCallbackStub with OnRemoteRequest
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OnRemoteRequest for switch of COMMAND_ON_FRAME_ERROR
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_040, TestSize.Level1)
{
    MockHStreamRepeatCallbackStub stub;
    MessageParcel data;
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    uint32_t code = static_cast<uint32_t>(IStreamRepeatCallbackIpcCode::COMMAND_ON_FRAME_ERROR);
    EXPECT_CALL(stub, OnFrameError(_))
        .WillOnce(Return(0));
    int errCode = stub.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(errCode, 0);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeatStub with OnRemoteRequest
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OnRemoteRequest for switch of CAMERA_ENABLE_STREAM_MIRROR
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_041, TestSize.Level1)
{
    MockHStreamRepeatStub stub;
    MessageParcel data;
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    uint32_t code = static_cast<uint32_t>(IStreamRepeatIpcCode::COMMAND_SET_MIRROR);
    EXPECT_CALL(stub, SetMirror(_))
        .WillOnce(Return(ERR_NONE));
    int errCode = stub.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(errCode, ERR_NONE);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeatStub with OnRemoteRequest
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OnRemoteRequest for switch of CAMERA_ENABLE_SECURE_STREAM
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_042, TestSize.Level1)
{
    MockHStreamRepeatStub stub;
    MessageParcel data;
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    uint32_t code = static_cast<uint32_t>(IStreamRepeatIpcCode::COMMAND_ENABLE_SECURE);
    EXPECT_CALL(stub, EnableSecure(_))
        .WillOnce(Return(ERR_NONE));
    int errCode = stub.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(errCode, ERR_NONE);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeatStub with OnRemoteRequest
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OnRemoteRequest for switch of CAMERA_STREAM_FRAME_RANGE_SET
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_043, TestSize.Level1)
{
    MockHStreamRepeatStub stub;
    MessageParcel data;
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    uint32_t code = static_cast<uint32_t>(IStreamRepeatIpcCode::COMMAND_SET_FRAME_RATE);
    EXPECT_CALL(stub, SetFrameRate(_, _))
        .WillOnce(Return(ERR_NONE));
    int errCode = stub.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(errCode, ERR_NONE);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeatStub with OnRemoteRequest
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OnRemoteRequest for switch of COMMAND_UPDATE_SKETCH_RATIO
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_044, TestSize.Level1)
{
    MockHStreamRepeatStub stub;
    MessageParcel data;
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    uint32_t code = static_cast<uint32_t>(IStreamRepeatIpcCode::COMMAND_UPDATE_SKETCH_RATIO);
    EXPECT_CALL(stub, UpdateSketchRatio(_))
        .WillOnce(Return(ERR_NONE));
    int errCode = stub.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(errCode, ERR_NONE);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeatStub with OnRemoteRequest
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OnRemoteRequest for switch of COMMAND_GET_MIRROR
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_046, TestSize.Level1)
{
    MockHStreamRepeatStub stub;
    MessageParcel data;
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    uint32_t code = static_cast<uint32_t>(IStreamRepeatIpcCode::COMMAND_GET_MIRROR);
    EXPECT_CALL(stub, GetMirror(_))
        .WillOnce(Return(ERR_NONE));
    int errCode = stub.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(errCode, ERR_NONE);
}

/*
 * Feature: Framework
 * Function: Test HStreamRepeatStub with OnRemoteRequest
 * SubFunction: NA
 * FunctionPoints: NA
 * EnvConditions: NA
 * CaseDescription: Test OnRemoteRequest for switch of default
 */
HWTEST_F(HStreamRepeatUnit, hstream_repeat_unittest_048, TestSize.Level1)
{
    MockHStreamRepeatStub stub;
    MessageParcel data;
    data.WriteInterfaceToken(stub.GetDescriptor());
    data.RewindRead(0);
    MessageParcel reply;
    MessageOption option;
    uint32_t code = CAMERA_STREAM_REPEAT_ON_DEFAULT;
    int errCode = stub.OnRemoteRequest(code, data, reply, option);
    EXPECT_EQ(errCode, ERR_NONE);
}

/**
 * @tc.name  : Test ReleaseStream API
 * @tc.number: ReleaseStream_001
 * @tc.desc  : Test ReleaseStream API, when sketchStreamRepeat_ is't nullptr
 */
HWTEST_F(HStreamRepeatUnit, ReleaseStream_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->sketchStreamRepeat_ = CreateHStreamRepeat(RepeatStreamType::SKETCH);
    auto ret = streamRepeat->ReleaseStream(false);
    EXPECT_EQ(ret, CAMERA_OK);
}

/**
 * @tc.name  : Test OnFrameStarted API
 * @tc.number: OnFrameStarted_001
 * @tc.desc  : Test OnFrameStarted API, when streamRepeatCallback_ is nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnFrameStarted_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->streamRepeatCallback_ = nullptr;
    auto ret = streamRepeat->OnFrameStarted();
    EXPECT_EQ(ret, CAMERA_OK);
}

/**
 * @tc.name  : Test OnFrameStarted API
 * @tc.number: OnFrameStarted_002
 * @tc.desc  : Test OnFrameStarted API, when streamRepeatCallback_ is't nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnFrameStarted_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    sptr<MockHStreamRepeatCallbackStub> callback = new MockHStreamRepeatCallbackStub();
    ASSERT_NE(callback, nullptr);
    streamRepeat->streamRepeatCallback_ = callback;
    auto ret = streamRepeat->OnFrameStarted();
    EXPECT_EQ(ret, CAMERA_OK);
    testing::Mock::AllowLeak(callback.GetRefPtr());
}

/**
 * @tc.name  : Test OnFrameStarted API
 * @tc.number: OnFrameStarted_003
 * @tc.desc  : Test OnFrameStarted API, when repeatStreamType_ is VIDEO
 */
HWTEST_F(HStreamRepeatUnit, OnFrameStarted_003, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<MockHStreamRepeatCallbackStub> callback = new MockHStreamRepeatCallbackStub();
    ASSERT_NE(callback, nullptr);
    streamRepeat->streamRepeatCallback_ = callback;
    auto ret = streamRepeat->OnFrameStarted();
    EXPECT_EQ(ret, CAMERA_OK);
    testing::Mock::AllowLeak(callback.GetRefPtr());
}

/**
 * @tc.name  : Test OnFrameEnded API
 * @tc.number: OnFrameEnded_001
 * @tc.desc  : Test OnFrameEnded API, when streamRepeatCallback_ is nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnFrameEnded_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->streamRepeatCallback_ = nullptr;
    auto ret = streamRepeat->OnFrameStarted();
    EXPECT_EQ(ret, CAMERA_OK);
    ret = streamRepeat->OnFrameEnded(CONST_0);
    EXPECT_EQ(ret, CAMERA_OK);
}

/**
 * @tc.name  : Test OnFrameEnded API
 * @tc.number: OnFrameEnded_002
 * @tc.desc  : Test OnFrameEnded API, when streamRepeatCallback_ is't nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnFrameEnded_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    sptr<MockHStreamRepeatCallbackStub> callback = new MockHStreamRepeatCallbackStub();
    ASSERT_NE(callback, nullptr);
    streamRepeat->streamRepeatCallback_ = callback;
    auto ret = streamRepeat->OnFrameStarted();
    EXPECT_EQ(ret, CAMERA_OK);
    ret = streamRepeat->OnFrameEnded(CONST_0);
    EXPECT_EQ(ret, CAMERA_OK);
    testing::Mock::AllowLeak(callback.GetRefPtr());
}

/**
 * @tc.name  : Test OnFrameEnded API
 * @tc.number: OnFrameEnded_003
 * @tc.desc  : Test OnFrameEnded API, when repeatStreamType_ is VIDEO
 */
HWTEST_F(HStreamRepeatUnit, OnFrameEnded_003, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<MockHStreamRepeatCallbackStub> callback = new MockHStreamRepeatCallbackStub();
    ASSERT_NE(callback, nullptr);
    streamRepeat->streamRepeatCallback_ = callback;
    auto ret = streamRepeat->OnFrameStarted();
    EXPECT_EQ(ret, CAMERA_OK);
    ret = streamRepeat->OnFrameEnded(CONST_0);
    EXPECT_EQ(ret, CAMERA_OK);
    testing::Mock::AllowLeak(callback.GetRefPtr());
}

/**
 * @tc.name  : Test OnDeferredVideoEnhancementInfo API
 * @tc.number: OnDeferredVideoEnhancementInfo_001
 * @tc.desc  : Test OnDeferredVideoEnhancementInfo API, when repeatStreamType_ is VIDEO and streamRepeatCallback_ is
 * nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnDeferredVideoEnhancementInfo_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->streamRepeatCallback_ = nullptr;
    auto ret = streamRepeat->OnDeferredVideoEnhancementInfo(CaptureEndedInfoExt());
    EXPECT_EQ(ret, CAMERA_OK);
}

/**
 * @tc.name  : Test OnDeferredVideoEnhancementInfo API
 * @tc.number: OnDeferredVideoEnhancementInfo_002
 * @tc.desc  : Test OnDeferredVideoEnhancementInfo API, when repeatStreamType_ is VIDEO and streamRepeatCallback_
 * is't nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnDeferredVideoEnhancementInfo_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<MockHStreamRepeatCallbackStub> callback = new MockHStreamRepeatCallbackStub();
    ASSERT_NE(callback, nullptr);
    streamRepeat->streamRepeatCallback_ = callback;
    auto ret = streamRepeat->OnDeferredVideoEnhancementInfo(CaptureEndedInfoExt());
    EXPECT_EQ(ret, CAMERA_OK);
    testing::Mock::AllowLeak(callback.GetRefPtr());
}

/**
 * @tc.name  : Test OnSketchStatusChanged API
 * @tc.number: OnSketchStatusChanged_001
 * @tc.desc  : Test OnSketchStatusChanged API, when streamRepeatCallback_ is nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnSketchStatusChanged_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->streamRepeatCallback_ = nullptr;
    auto ret = streamRepeat->OnSketchStatusChanged(SketchStatus());
    EXPECT_EQ(ret, CAMERA_OK);
}

/**
 * @tc.name  : Test OnSketchStatusChanged API
 * @tc.number: OnSketchStatusChanged_002
 * @tc.desc  : Test OnSketchStatusChanged API, when streamRepeatCallback_ is't nullptr
 */
HWTEST_F(HStreamRepeatUnit, OnSketchStatusChanged_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    sptr<MockHStreamRepeatCallbackStub> callback = new MockHStreamRepeatCallbackStub();
    ASSERT_NE(callback, nullptr);
    streamRepeat->streamRepeatCallback_ = callback;
    auto ret = streamRepeat->OnSketchStatusChanged(SketchStatus());
    EXPECT_EQ(ret, CAMERA_OK);
    testing::Mock::AllowLeak(callback.GetRefPtr());
}

/**
 * @tc.name  : Test OnSketchStatusChanged API
 * @tc.number: OnSketchStatusChanged_001
 * @tc.desc  : Test OnSketchStatusChanged API, when streamRepeatCallback_ is't nullptr
 */
HWTEST_F(HStreamRepeatUnit, ForkSketchStreamRepeat_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->sketchStreamRepeat_ = CreateHStreamRepeat(RepeatStreamType::SKETCH);
    sptr<IRemoteObject> sketchStream;
    auto ret = streamRepeat->ForkSketchStreamRepeat(
        streamRepeat->width_, streamRepeat->height_, sketchStream, streamRepeat->sketchRatio_);
    EXPECT_EQ(ret, CAMERA_OK);
}

/**
 * @tc.name  : Test OnSketchStatusChanged API
 * @tc.number: OnSketchStatusChanged_002
 * @tc.desc  : Test OnSketchStatusChanged API, when streamRepeatCallback_ is nullptr
 */
HWTEST_F(HStreamRepeatUnit, ForkSketchStreamRepeat_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat(RepeatStreamType::VIDEO);
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->sketchStreamRepeat_ = nullptr;
    sptr<IRemoteObject> sketchStream;
    auto ret = streamRepeat->ForkSketchStreamRepeat(
        streamRepeat->width_, streamRepeat->height_, sketchStream, streamRepeat->sketchRatio_);
    EXPECT_EQ(ret, CAMERA_OK);
}

/**
 * @tc.name  : Test SetFrameRate API
 * @tc.number: SetFrameRate_001
 * @tc.desc  : Test SetFrameRate API, when cameraAbility_ is nullptr
 */
HWTEST_F(HStreamRepeatUnit, SetFrameRate_001, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->cameraAbility_ = nullptr;
    auto rc = streamRepeat->SetFrameRate(30, 60);
    EXPECT_EQ(rc, CAMERA_OK);
}

/**
 * @tc.name  : Test SetFrameRate API
 * @tc.number: SetFrameRate_002
 * @tc.desc  : Test SetFrameRate API, when cameraAbility_ is't nullptr
 */
HWTEST_F(HStreamRepeatUnit, SetFrameRate_002, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    auto rc = streamRepeat->SetFrameRate(30, 60);
    EXPECT_EQ(rc, CAMERA_OK);
}

/**
 * @tc.name  : Test SetFrameRate API
 * @tc.number: SetFrameRate_003
 * @tc.desc  : Test SetFrameRate API, when streamOperator is nullptr
 */
HWTEST_F(HStreamRepeatUnit, SetFrameRate_003, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    streamRepeat->streamOperator_ = nullptr;
    auto rc = streamRepeat->SetFrameRate(30, 60);
    EXPECT_EQ(rc, CAMERA_OK);
}

/**
 * @tc.name  : Test SetFrameRate API
 * @tc.number: SetFrameRate_004
 * @tc.desc  : Test SetFrameRate API, when streamOperator is't nullptr and repeatStreamStatus_ is STOPED
 */
HWTEST_F(HStreamRepeatUnit, SetFrameRate_004, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    streamRepeat->streamOperator_ = new MockStreamOperator();
    streamRepeat->repeatStreamStatus_ = RepeatStreamStatus::STOPED;
    auto rc = streamRepeat->SetFrameRate(30, 60);
    EXPECT_EQ(rc, CAMERA_OK);
}

/**
 * @tc.name  : Test SetFrameRate API
 * @tc.number: SetFrameRate_005
 * @tc.desc  : Test SetFrameRate API, when streamOperator is't nullptr and repeatStreamStatus_ is STARTED
 */
HWTEST_F(HStreamRepeatUnit, SetFrameRate_005, TestSize.Level1)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    streamRepeat->streamOperator_ = new MockStreamOperator();
    streamRepeat->repeatStreamStatus_ = RepeatStreamStatus::STARTED;
    auto rc = streamRepeat->SetFrameRate(30, 60);
    EXPECT_EQ(rc, CAMERA_OK);
}

/**
 * @tc.name  : Test SetMirrorForLivePhoto API
 * @tc.number: SetMirrorForLivePhoto_001
 * @tc.desc  : Test SetMirrorForLivePhoto API, when isMirrorSupported
 */
HWTEST_F(HStreamRepeatUnit, SetMirrorForLivePhoto_001, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    bool isEnable = true;
    camera_metadata_item_t item;
    int32_t mode = 110;
    std::vector<uint8_t> data { 110, 2 };
    int32_t res;
    streamRepeat->cameraAbility_->addEntry(OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, data.data(), data.size());
    streamRepeat->SetMirrorForLivePhoto(isEnable, mode);
    res = OHOS::Camera::FindCameraMetadataItem(
        streamRepeat->cameraAbility_->get(), OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
    EXPECT_EQ(res, CAM_META_SUCCESS);
    EXPECT_EQ(streamRepeat->enableMirror_, true);
}

/**
 * @tc.name  : Test SetMirrorForLivePhoto API
 * @tc.number: SetMirrorForLivePhoto_002
 * @tc.desc  : Test SetMirrorForLivePhoto API, when isMirrorSupported is false
 */
HWTEST_F(HStreamRepeatUnit, SetMirrorForLivePhoto_002, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    std::shared_ptr<OHOS::Camera::CameraMetadata> cameraAbility =
        std::make_shared<OHOS::Camera::CameraMetadata>(1, 1024);
    ASSERT_NE(cameraAbility, nullptr);
    streamRepeat->cameraAbility_ = cameraAbility;
    std::vector<uint8_t> data { 0, 0 };
    bool isEnable = true;
    camera_metadata_item_t item;
    int32_t mode = 110;
    int32_t res;
    streamRepeat->cameraAbility_->addEntry(OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, data.data(), data.size());
    streamRepeat->SetMirrorForLivePhoto(isEnable, mode);
    res = OHOS::Camera::FindCameraMetadataItem(
        streamRepeat->cameraAbility_->get(), OHOS_CONTROL_CAPTURE_MIRROR_SUPPORTED, &item);
    EXPECT_EQ(res, CAM_META_SUCCESS);
    EXPECT_EQ(streamRepeat->enableMirror_, false);
}

/**
 * @tc.name  : Test ProcessFixedTransform API
 * @tc.number: ProcessFixedTransform_001
 * @tc.desc  : Test ProcessFixedTransform API, when enableCameraRotation_
 */
HWTEST_F(HStreamRepeatUnit, ProcessFixedTransform_001, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->enableCameraRotation_ = true;
    int32_t sensorOrientation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = camera_position_enum_t::OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessFixedTransform(sensorOrientation, cameraPosition);
}

/**
 * @tc.name  : Test ProcessFixedTransform API
 * @tc.number: ProcessFixedTransform_002
 * @tc.desc  : Test ProcessFixedTransform API, when enableCameraRotation_ is false
 */
HWTEST_F(HStreamRepeatUnit, ProcessFixedTransform_002, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    streamRepeat->enableCameraRotation_ = false;
    int32_t sensorOrientation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = camera_position_enum_t::OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessFixedTransform(sensorOrientation, cameraPosition);
}

/**
 * @tc.name  : Test ProcessFixedDiffDeviceTransform API
 * @tc.number: ProcessFixedDiffDeviceTransform_001
 * @tc.desc  : Test ProcessFixedDiffDeviceTransform API, when cameraPosition is OHOS_CAMERA_POSITION_FRONT
 */
HWTEST_F(HStreamRepeatUnit, ProcessFixedDiffDeviceTransform_001, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = camera_position_enum_t::OHOS_CAMERA_POSITION_FRONT;
    streamRepeat->ProcessFixedDiffDeviceTransform(sensorOrientation, cameraPosition);
}

/**
 * @tc.name  : Test ProcessFixedDiffDeviceTransform API
 * @tc.number: ProcessFixedDiffDeviceTransform_002
 * @tc.desc  : Test ProcessFixedDiffDeviceTransform API, when cameraPosition is't OHOS_CAMERA_POSITION_FRONT
 */
HWTEST_F(HStreamRepeatUnit, ProcessFixedDiffDeviceTransform_002, TestSize.Level0)
{
    auto streamRepeat = CreateHStreamRepeat();
    ASSERT_NE(streamRepeat, nullptr);
    int32_t sensorOrientation = STREAM_ROTATE_0;
    camera_position_enum_t cameraPosition = camera_position_enum_t::OHOS_CAMERA_POSITION_BACK;
    streamRepeat->ProcessFixedDiffDeviceTransform(sensorOrientation, cameraPosition);
}
}
}