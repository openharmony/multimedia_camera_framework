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

#ifndef METADATA_OUTPUT_NAPI_H_
#define METADATA_OUTPUT_NAPI_H_

#include "camera_napi_event_emitter.h"
#include "camera_napi_utils.h"
#include "input/camera_info.h"
#include "input/camera_manager.h"
#include "listener_base.h"

namespace OHOS {
namespace CameraStandard {
static const char CAMERA_METADATA_OUTPUT_NAPI_CLASS_NAME[] = "MetadataOutput";

class MetadataOutputCallback : public MetadataObjectCallback, public ListenerBase,
    public std::enable_shared_from_this<MetadataOutputCallback> {
public:
    explicit MetadataOutputCallback(napi_env env);
    virtual ~MetadataOutputCallback() = default;

    void OnMetadataObjectsAvailable(std::vector<sptr<MetadataObject>> metaObjects) const override;
private:
    void OnMetadataObjectsAvailableCallback(const std::vector<sptr<MetadataObject>> metadataObjList) const;
    napi_value CreateMetadataObjJSArray(napi_env env, const std::vector<sptr<MetadataObject>> metadataObjList) const;
    void AddMetadataObjExtending(napi_env env, sptr<MetadataObject> metadataObj, napi_value &metadataNapiObj) const;
    void CreateHumanFaceMetaData(napi_env env, sptr<MetadataObject> metadataObj, napi_value &metadataNapiObj) const;
    void CreateCatFaceMetaData(napi_env env, sptr<MetadataObject> metadataObj, napi_value &metadataNapiObj) const;
    void CreateDogFaceMetaData(napi_env env, sptr<MetadataObject> metadataObj, napi_value &metadataNapiObj) const;
};

class MetadataStateCallbackNapi : public MetadataStateCallback, public ListenerBase,
    public std::enable_shared_from_this<MetadataStateCallbackNapi> {
public:
    explicit MetadataStateCallbackNapi(napi_env env);
    virtual ~MetadataStateCallbackNapi() = default;
    void OnError(const int32_t errorType) const override;

private:
    void OnErrorCallback(const int32_t errorType) const;
    void OnErrorCallbackAsync(const int32_t errorType) const;
};

struct MetadataOutputCallbackInfo {
    const std::vector<sptr<MetadataObject>> info_;
    weak_ptr<const MetadataOutputCallback> listener_;
    MetadataOutputCallbackInfo(std::vector<sptr<MetadataObject>> metadataObjList,
        shared_ptr<const MetadataOutputCallback> listener)
        : info_(metadataObjList), listener_(listener) {}
};

struct MetadataStateCallbackInfo {
    int32_t errorType_;
    weak_ptr<const MetadataStateCallbackNapi> listener_;
    MetadataStateCallbackInfo(int32_t errorType, shared_ptr<const MetadataStateCallbackNapi> listener)
        : errorType_(errorType), listener_(listener) {}
};

class MetadataOutputNapi : public CameraNapiEventEmitter<MetadataOutputNapi> {
public:
    static napi_value Init(napi_env env, napi_value exports);
    static napi_value CreateMetadataOutput(napi_env env, std::vector<MetadataObjectType> metadataObjectTypes);
    static napi_value AddMetadataObjectTypes(napi_env env, napi_callback_info info);
    static napi_value RemoveMetadataObjectTypes(napi_env env, napi_callback_info info);
    MetadataOutputNapi();
    ~MetadataOutputNapi() override;
    sptr<MetadataOutput> GetMetadataOutput();
    static bool IsMetadataOutput(napi_env env, napi_value obj);
    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value Once(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);

    const EmitterFunctions& GetEmitterFunctions() override;

private:
    static void MetadataOutputNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value MetadataOutputNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value Start(napi_env env, napi_callback_info info);
    static napi_value Stop(napi_env env, napi_callback_info info);
    static napi_value Release(napi_env env, napi_callback_info info);

    void RegisterMetadataObjectsAvailableCallbackListener(const std::string& eventName, napi_env env,
        napi_value callback, const std::vector<napi_value>& args, bool isOnce);
    void UnregisterMetadataObjectsAvailableCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);
    void RegisterErrorCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce);
    void UnregisterErrorCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args);

    static thread_local napi_ref sConstructor_;
    static thread_local sptr<MetadataOutput> sMetadataOutput_;
    static thread_local uint32_t metadataOutputTaskId;

    napi_env env_;
    sptr<MetadataOutput> metadataOutput_;
    std::shared_ptr<MetadataOutputCallback> metadataOutputCallback_;
    std::shared_ptr<MetadataStateCallbackNapi> metadataStateCallback_;
};

struct MetadataOutputAsyncContext : public AsyncContext {
    MetadataOutputAsyncContext(std::string funcName, int32_t taskId) : AsyncContext(funcName, taskId) {};
    MetadataOutputNapi* objectInfo = nullptr;
    std::string errorMsg;
    std::vector<MetadataObjectType> SupportedMetadataObjectTypes;
    std::vector<MetadataObjectType> setSupportedMetadataObjectTypes;
};
} // namespace CameraStandard
} // namespace OHOS
#endif /* METADATA_OUTPUT_NAPI_H_ */
