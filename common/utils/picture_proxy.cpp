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

#include "picture_proxy.h"
#include "camera_log.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace CameraStandard {
// LCOV_EXCL_START
PictureProxy::PictureProxy(
    std::shared_ptr<Dynamiclib> pictureLib, std::shared_ptr<PictureIntf> pictureIntf)
    : pictureLib_(pictureLib), pictureIntf_(pictureIntf)
{
    CHECK_RETURN_ELOG(pictureLib_ == nullptr, "PictureProxy construct pictureLib is null");
    CHECK_RETURN_ELOG(pictureIntf_ == nullptr, "PictureProxy construct pictureIntf is null");
}

PictureProxy::~PictureProxy()
{
    MEDIA_INFO_LOG("PictureProxy dctor");
}

void PictureProxy::Create(sptr<SurfaceBuffer> &surfaceBuffer)
{
    MEDIA_INFO_LOG("PictureProxy ctor");
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_ELOG(!pictureIntf, "PictureProxy::Create pictureIntf_ is nullptr");
    pictureIntf->Create(surfaceBuffer);
}

typedef PictureIntf* (*CreatePictureIntf)();
std::shared_ptr<PictureProxy> PictureProxy::CreatePictureProxy()
{
    auto dynamiclib = CameraDynamicLoader::GetDynamiclib(PICTURE_SO);
    CHECK_RETURN_RET_ELOG(
        dynamiclib == nullptr, nullptr, "PictureProxy::CreatePictureProxy get dynamiclib fail");
    CreatePictureIntf createPictureIntf = (CreatePictureIntf)dynamiclib->GetFunction("createPictureAdapterIntf");
    CHECK_RETURN_RET_ELOG(
        createPictureIntf == nullptr, nullptr, "PictureProxy::CreatePictureProxy get createPictureIntf fail");
    PictureIntf* pictureIntf = createPictureIntf();
    CHECK_RETURN_RET_ELOG(
        pictureIntf == nullptr, nullptr, "PictureProxy::CreatePictureProxy get pictureIntf fail");
    std::shared_ptr<PictureProxy> pictureProxy =
        std::make_shared<PictureProxy>(dynamiclib, std::shared_ptr<PictureIntf>(pictureIntf));
    return pictureProxy;
}

std::shared_ptr<PictureIntf> PictureProxy::GetPictureIntf() const
{
    return pictureIntf_;
}
void PictureProxy::SetAuxiliaryPicture(sptr<SurfaceBuffer> &surfaceBuffer, CameraAuxiliaryPictureType type)
{
    MEDIA_INFO_LOG("PictureProxy::SetAuxiliaryPicture enter");
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_ELOG(!pictureIntf, "PictureProxy::SetAuxiliaryPicture pictureIntf_ is nullptr");
    pictureIntf->SetAuxiliaryPicture(surfaceBuffer, type);
}

void PictureProxy::CreateWithDeepCopySurfaceBuffer(sptr<SurfaceBuffer> &surfaceBuffer)
{
    MEDIA_INFO_LOG("PictureProxy::CreateWithDeepCopySurfaceBuffer enter");
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_ELOG(!pictureIntf, "PictureProxy::CreateWithDeepCopySurfaceBuffer pictureIntf_ is nullptr");
    pictureIntf->CreateWithDeepCopySurfaceBuffer(surfaceBuffer);
}

bool PictureProxy::Marshalling(Parcel &data) const
{
    MEDIA_INFO_LOG("PictureProxy::Marshalling enter");
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_RET_ELOG(pictureIntf == nullptr, false, "PictureProxy::Marshalling pictureIntf_ is nullptr");
    return pictureIntf->Marshalling(data);
}

void PictureProxy::UnmarshallingPicture(Parcel &data)
{
    MEDIA_INFO_LOG("PictureProxy::Unmarshalling enter");
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_ELOG(!pictureIntf, "PictureProxy::Unmarshalling failed! pictureIntf is nullptr");
    pictureIntf->UnmarshallingPicture(data);
}

int32_t PictureProxy::SetExifMetadata(sptr<SurfaceBuffer> &surfaceBuffer)
{
    MEDIA_INFO_LOG("PictureProxy::SetExifMetadata enter");
    int32_t retCode = -1;
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_RET_ELOG(!pictureIntf, retCode, "PictureProxy::SetExifMetadata pictureIntf is nullptr");
    retCode = pictureIntf->SetExifMetadata(surfaceBuffer);
    return retCode;
}

bool PictureProxy::SetMaintenanceData(sptr<SurfaceBuffer> &surfaceBuffer)
{
    bool retCode = false;
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_RET_ELOG(!pictureIntf, retCode, "PictureProxy::SetMaintenanceData pictureIntf is nullptr");
    retCode = pictureIntf->SetMaintenanceData(surfaceBuffer);
    return retCode;
}

void PictureProxy::RotatePicture()
{
    std::shared_ptr<PictureIntf> pictureIntf = GetPictureIntf();
    CHECK_RETURN_ELOG(!pictureIntf, "PictureProxy::RotatePicture pictureIntf is nullptr");
    pictureIntf->RotatePicture();
}
// LCOV_EXCL_STOP
}  // namespace CameraStandard
}  // namespace OHOS