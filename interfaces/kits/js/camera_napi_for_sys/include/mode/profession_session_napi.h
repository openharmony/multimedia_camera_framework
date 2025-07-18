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

#ifndef PROFESSION_SESSION_NAPI_H
#define PROFESSION_SESSION_NAPI_H

#include "session/camera_session_for_sys_napi.h"
#include "session/profession_session.h"

namespace OHOS {
namespace CameraStandard {
class ExposureInfoCallbackListener : public ExposureInfoCallback, public ListenerBase,
    public std::enable_shared_from_this<ExposureInfoCallbackListener> {
public:
    ExposureInfoCallbackListener(napi_env env) : ListenerBase(env) {}
    ~ExposureInfoCallbackListener() = default;
    void OnExposureInfoChanged(ExposureInfo info) override;

private:
    void OnExposureInfoChangedCallback(ExposureInfo info) const;
    void OnExposureInfoChangedCallbackAsync(ExposureInfo info) const;
};

struct ExposureInfoChangedCallback {
    ExposureInfo info_;
    weak_ptr<const ExposureInfoCallbackListener> listener_;
    ExposureInfoChangedCallback(ExposureInfo info, shared_ptr<const ExposureInfoCallbackListener> listener)
        : info_(info), listener_(listener) {}
};

class IsoInfoCallbackListener : public IsoInfoCallback, public ListenerBase,
    public std::enable_shared_from_this<IsoInfoCallbackListener> {
public:
    IsoInfoCallbackListener(napi_env env) : ListenerBase(env) {}
    ~IsoInfoCallbackListener() = default;
    void OnIsoInfoChanged(IsoInfo info) override;

private:
    void OnIsoInfoChangedCallback(IsoInfo info) const;
    void OnIsoInfoChangedCallbackAsync(IsoInfo info) const;
};

struct IsoInfoChangedCallback {
    IsoInfo info_;
    weak_ptr<const IsoInfoCallbackListener> listener_;
    IsoInfoChangedCallback(IsoInfo info, shared_ptr<const IsoInfoCallbackListener> listener)
        : info_(info), listener_(listener) {}
};

class ApertureInfoCallbackListener : public ApertureInfoCallback, public ListenerBase,
    public std::enable_shared_from_this<ApertureInfoCallbackListener> {
public:
    ApertureInfoCallbackListener(napi_env env) : ListenerBase(env) {}
    ~ApertureInfoCallbackListener() = default;
    void OnApertureInfoChanged(ApertureInfo info) override;

private:
    void OnApertureInfoChangedCallback(ApertureInfo info) const;
    void OnApertureInfoChangedCallbackAsync(ApertureInfo info) const;
};

struct ApertureInfoChangedCallback {
    ApertureInfo info_;
    weak_ptr<const ApertureInfoCallbackListener> listener_;
    ApertureInfoChangedCallback(ApertureInfo info, shared_ptr<const ApertureInfoCallbackListener> listener)
        : info_(info), listener_(listener) {}
};

class LuminationInfoCallbackListener : public LuminationInfoCallback, public ListenerBase,
    public std::enable_shared_from_this<LuminationInfoCallbackListener> {
public:
    LuminationInfoCallbackListener(napi_env env) : ListenerBase(env) {}
    ~LuminationInfoCallbackListener() = default;
    void OnLuminationInfoChanged(LuminationInfo info) override;

private:
    void OnLuminationInfoChangedCallback(LuminationInfo info) const;
    void OnLuminationInfoChangedCallbackAsync(LuminationInfo info) const;
};

struct LuminationInfoChangedCallback {
    LuminationInfo info_;
    weak_ptr<const LuminationInfoCallbackListener> listener_;
    LuminationInfoChangedCallback(LuminationInfo info, shared_ptr<const LuminationInfoCallbackListener> listener)
        : info_(info), listener_(listener) {}
};

static const char PROFESSIONAL_SESSION_NAPI_CLASS_NAME[] = "ProfessionSession";
class ProfessionSessionNapi : public CameraSessionForSysNapi {
public:
    static void Init(napi_env env);
    static napi_value CreateCameraSession(napi_env env, SceneMode mode);
    ProfessionSessionNapi();
    ~ProfessionSessionNapi();

    static void ProfessionSessionNapiDestructor(napi_env env, void* nativeObject, void* finalize_hint);
    static napi_value ProfessionSessionNapiConstructor(napi_env env, napi_callback_info info);

    static napi_value GetSupportedMeteringModes(napi_env env, napi_callback_info info);
    static napi_value IsMeteringModeSupported(napi_env env, napi_callback_info info);
    static napi_value GetMeteringMode(napi_env env, napi_callback_info info);
    static napi_value SetMeteringMode(napi_env env, napi_callback_info info);

    static napi_value IsManualIsoSupported(napi_env env, napi_callback_info info);
    static napi_value GetIsoRange(napi_env env, napi_callback_info info);
    static napi_value GetISO(napi_env env, napi_callback_info info);
    static napi_value SetISO(napi_env env, napi_callback_info info);

    static napi_value GetSupportedVirtualApertures(napi_env env, napi_callback_info info);
    static napi_value GetVirtualAperture(napi_env env, napi_callback_info info);
    static napi_value SetVirtualAperture(napi_env env, napi_callback_info info);

    static napi_value GetExposureDurationRange(napi_env env, napi_callback_info info);
    static napi_value GetExposureDuration(napi_env env, napi_callback_info info);
    static napi_value SetExposureDuration(napi_env env, napi_callback_info info);

    static napi_value GetSupportedExposureHintModes(napi_env env, napi_callback_info info);
    static napi_value GetExposureHintMode(napi_env env, napi_callback_info info);
    static napi_value SetExposureHintMode(napi_env env, napi_callback_info info);

    static napi_value GetSupportedFocusAssistFlashModes(napi_env env, napi_callback_info info);
    static napi_value IsFocusAssistFlashModeSupported(napi_env env, napi_callback_info info);
    static napi_value GetFocusAssistFlashMode(napi_env env, napi_callback_info info);
    static napi_value SetFocusAssistFlashMode(napi_env env, napi_callback_info info);

    static napi_value On(napi_env env, napi_callback_info info);
    static napi_value Once(napi_env env, napi_callback_info info);
    static napi_value Off(napi_env env, napi_callback_info info);

    static const std::vector<napi_property_descriptor> manual_exposure_funcs;
    static const std::vector<napi_property_descriptor> manual_focus_funcs;
    static const std::vector<napi_property_descriptor> manual_iso_props;
    static const std::vector<napi_property_descriptor> pro_session_props;

    std::shared_ptr<ExposureInfoCallbackListener> exposureInfoCallback_ = nullptr;
    std::shared_ptr<IsoInfoCallbackListener> isoInfoCallback_ = nullptr;
    std::shared_ptr<ApertureInfoCallbackListener> apertureInfoCallback_ = nullptr;
    std::shared_ptr<LuminationInfoCallbackListener> luminationInfoCallback_ = nullptr;
    std::shared_ptr<AbilityCallbackListener> abilityCallback_ = nullptr;

    napi_env env_;
    sptr<ProfessionSession> professionSession_;

    static thread_local napi_ref sConstructor_;

protected:
    void RegisterExposureInfoCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce) override;
    void UnregisterExposureInfoCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args) override;
    void RegisterAbilityChangeCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce) override;
    void UnregisterAbilityChangeCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args) override;
    void RegisterIsoInfoCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce) override;
    void UnregisterIsoInfoCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args) override;
    void RegisterApertureInfoCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce) override;
    void UnregisterApertureInfoCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args) override;
    void RegisterLuminationInfoCallbackListener(const std::string& eventName, napi_env env, napi_value callback,
        const std::vector<napi_value>& args, bool isOnce) override;
    void UnregisterLuminationInfoCallbackListener(
        const std::string& eventName, napi_env env, napi_value callback, const std::vector<napi_value>& args) override;
};
}
}
#endif /* PROFESSION_SESSION_NAPI_H */
