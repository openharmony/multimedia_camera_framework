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

#include "dynamic_loader/camera_napi_ex_proxy.h"

#include "camera_log.h"


namespace OHOS {
namespace CameraStandard {
std::shared_ptr<CameraNapiExProxy> CameraNapiExProxy::GetCameraNapiExProxy()
{
    auto dynamiclib = CameraDynamicLoader::GetDynamiclib(NAPI_EXT_SO);
    CHECK_RETURN_RET_ELOG(
        dynamiclib == nullptr, nullptr, "get extension lib fail");
    std::shared_ptr<CameraNapiExProxy> sessionForSysProxy = std::make_shared<CameraNapiExProxy>(dynamiclib);
    return sessionForSysProxy;
}
CameraNapiExProxy::CameraNapiExProxy(std::shared_ptr<Dynamiclib> napiExLib) : napiExLib_(
    napiExLib)
{
    MEDIA_DEBUG_LOG("CameraNapiExProxy construct");
    CHECK_RETURN_ELOG(napiExLib_ == nullptr, "napiExLib is null");
}

CameraNapiExProxy::~CameraNapiExProxy()
{
    MEDIA_DEBUG_LOG("CameraNapiExProxy destruct");
    CameraDynamicLoader::FreeDynamicLibDelayed(NAPI_EXT_SO);
    napiExLib_ = nullptr;
}

typedef napi_value (*CreateSessionInstanceForSys)(napi_env, int32_t);
napi_value CameraNapiExProxy::CreateSessionForSys(napi_env env, int32_t jsModeName)
{
    MEDIA_DEBUG_LOG("CameraNapiExProxy::CreateSessionForSys is called");
    napi_value result = nullptr;
    CHECK_RETURN_RET_ELOG(napiExLib_ == nullptr, result, "napiExLib_ is null");
    CreateSessionInstanceForSys createSessionInstanceForSys =
        (CreateSessionInstanceForSys)napiExLib_->GetFunction("createSessionInstanceForSys");
    CHECK_RETURN_RET_ELOG(
        createSessionInstanceForSys == nullptr, result, "get function createSessionInstanceForSys fail");
    result = createSessionInstanceForSys(env, jsModeName);
    CHECK_RETURN_RET_ELOG(
        result == nullptr, result, "createSessionInstanceForSys fail");
    return result;
}

typedef napi_value (*CreateDeprecatedSessionInstanceForSys)(napi_env);
napi_value CameraNapiExProxy::CreateDeprecatedSessionForSys(napi_env env)
{
    MEDIA_DEBUG_LOG("CameraNapiExProxy::CreateDeprecatedSessionForSys is called");
    napi_value result = nullptr;
    CHECK_RETURN_RET_ELOG(napiExLib_ == nullptr, result, "napiExLib_ is null");
    CreateDeprecatedSessionInstanceForSys createDeprecatedSessionInstanceForSys =
        (CreateDeprecatedSessionInstanceForSys)napiExLib_->GetFunction("createDeprecatedSessionInstanceForSys");
    CHECK_RETURN_RET_ELOG(createDeprecatedSessionInstanceForSys == nullptr, result,
        "get function createDeprecatedSessionInstanceForSys fail");
    result = createDeprecatedSessionInstanceForSys(env);
    CHECK_RETURN_RET_ELOG(
        result == nullptr, result, "createDeprecatedSessionInstanceForSys fail");
    return result;
}

typedef napi_value (*CreateModeManagerInstance)(napi_env);
napi_value CameraNapiExProxy::CreateModeManager(napi_env env)
{
    MEDIA_DEBUG_LOG("CameraNapiExProxy::CreateModeManager is called");
    napi_value result = nullptr;
    CHECK_RETURN_RET_ELOG(napiExLib_ == nullptr, result, "napiExLib_ is null");
    CreateModeManagerInstance createModeManagerInstance =
        (CreateModeManagerInstance)napiExLib_->GetFunction("createModeManagerInstance");
    CHECK_RETURN_RET_ELOG(createModeManagerInstance == nullptr, result,
        "get function createModeManagerInstance fail");
    result = createModeManagerInstance(env);
    CHECK_RETURN_RET_ELOG(
        result == nullptr, result, "createModeManagerInstance fail");
    return result;
}

typedef napi_value (*CreateDepthDataOutputInstance)(napi_env, DepthProfile&);
napi_value CameraNapiExProxy::CreateDepthDataOutput(napi_env env, DepthProfile& depthProfile)
{
    MEDIA_DEBUG_LOG("CameraNapiExProxy::CreateDepthDataOutput is called");
    napi_value result = nullptr;
    CHECK_RETURN_RET_ELOG(napiExLib_ == nullptr, result, "napiExLib_ is null");
    CreateDepthDataOutputInstance createDepthDataOutputInstance =
        (CreateDepthDataOutputInstance)napiExLib_->GetFunction("createDepthDataOutputInstance");
    CHECK_RETURN_RET_ELOG(
        createDepthDataOutputInstance == nullptr, result, "get function createDepthDataOutputInstance fail");
    result = createDepthDataOutputInstance(env, depthProfile);
    CHECK_RETURN_RET_ELOG(
        result == nullptr, result, "createDepthDataOutputInstance fail");
    return result;
}

typedef bool (*CheckAndGetSysOutput)(napi_env, napi_value, sptr<CaptureOutput>&);
bool CameraNapiExProxy::CheckAndGetOutput(napi_env env, napi_value obj, sptr<CaptureOutput> &output)
{
    MEDIA_DEBUG_LOG("CameraNapiExProxy::CheckAndGetOutput is called");
    bool result = false;
    CHECK_RETURN_RET_ELOG(napiExLib_ == nullptr, result, "napiExLib_ is null");
    CheckAndGetSysOutput checkAndGetOutput =
        (CheckAndGetSysOutput)napiExLib_->GetFunction("checkAndGetOutput");
    CHECK_RETURN_RET_ELOG(
        checkAndGetOutput == nullptr, result, "get function checkAndGetOutput fail");
    result = checkAndGetOutput(env, obj, output);
    return result;
}
} // namespace CameraStandard
} // namespace OHOS