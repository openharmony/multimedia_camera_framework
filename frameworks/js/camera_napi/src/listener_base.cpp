/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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
#include "listener_base.h"

#include <cstdint>
#include <memory>
#include <uv.h>

#include "camera_log.h"
#include "js_native_api.h"
#include "js_native_api_types.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace CameraStandard {
ListenerBase::ListenerBase(napi_env env) : env_(env)
{
    MEDIA_DEBUG_LOG("ListenerBase is called.");
    // Record the JS thread that owns the env.
    jsThreadId_ = std::this_thread::get_id();

    // Allocate a decoupled context so that the cleanup hook's `data` pointer
    // remains valid even if `this` is destroyed on another thread.
    hookCtx_ = new (std::nothrow) HookContext();
    if (hookCtx_ == nullptr) {
        MEDIA_ERR_LOG("ListenerBase alloc HookContext fail");
        return;
    }
    hookCtx_->listener = this;
    hookCtx_->env = env;

    auto ret = napi_add_env_cleanup_hook(env_, ListenerBase::CleanUp, hookCtx_);
    if (ret != napi_status::napi_ok) {
        MEDIA_ERR_LOG("add env hook error: %{public}d", ret);
    }
}

ListenerBase::~ListenerBase()
{
    MEDIA_DEBUG_LOG("~ListenerBase is called.");
    if (hookCtx_ == nullptr) {
        env_ = nullptr;
        return;
    }

    HookContext* ctx = hookCtx_;
    hookCtx_ = nullptr;

    // Detach ourselves from the hook context first so that, no matter when
    // CleanUp fires, it will never touch a dangling ListenerBase.
    bool alreadyFired = false;
    {
        std::lock_guard<std::mutex> lock(ctx->mutex);
        ctx->listener = nullptr;
        alreadyFired = ctx->hookFired;
    }

    if (alreadyFired) {
        // Env has already been destroyed; hook already ran. Just free ctx.
        delete ctx;
        env_ = nullptr;
        return;
    }

    if (std::this_thread::get_id() == jsThreadId_) {
        // Same thread as the env owner: safe to remove the hook directly.
        auto ret = napi_remove_env_cleanup_hook(ctx->env, ListenerBase::CleanUp, ctx);
        if (ret != napi_status::napi_ok) {
            MEDIA_ERR_LOG("remove env hook error: %{public}d", ret);
        }
        {
            std::lock_guard<std::mutex> lock(ctx->mutex);
            ctx->hookRemoved = true;
        }
        delete ctx;
    } else {
        // Destructor is running on a non-JS thread. Marshal the hook-removal
        // back to the JS thread via napi_send_event. The task takes ownership
        // of ctx and deletes it after execution.
        MEDIA_WARNING_LOG("~ListenerBase on non-js thread, post remove-hook to js thread");
        napi_env env = ctx->env;
        auto task = [ctx]() {
            {
                std::lock_guard<std::mutex> lock(ctx->mutex);
                if (!ctx->hookFired && !ctx->hookRemoved) {
                    napi_remove_env_cleanup_hook(ctx->env, ListenerBase::CleanUp, ctx);
                    ctx->hookRemoved = true;
                }
            }
            delete ctx;
        };
        auto ret = napi_send_event(env, task, napi_eprio_immediate);
        if (ret != napi_status::napi_ok) {
            // Fallback: we cannot safely call napi APIs from here. Since the
            // listener pointer inside ctx has been cleared, any later hook
            // firing will be a harmless no-op, but ctx itself will leak.
            // This is preferable to crashing.
            MEDIA_ERR_LOG("napi_send_event for remove hook failed: %{public}d", ret);
        }
    }
    env_ = nullptr;
}

ListenerBase::ExecuteCallbackData::ExecuteCallbackData(napi_env env, napi_value errCode, napi_value returnData)
    : env_(env), errCode_(errCode), returnData_(returnData) {};

void ListenerBase::SaveCallbackReference(const std::string eventName, napi_value callback, bool isOnce)
{
    if (callback == nullptr) {
        MEDIA_ERR_LOG("SaveCallbackReference:%s js callback is nullptr, save nothing", eventName.c_str());
        return;
    }
    napi_valuetype valueType = napi_undefined;
    napi_typeof(env_, callback, &valueType);
    if (valueType != napi_function) {
        MEDIA_ERR_LOG("SaveCallbackReference:%s js callback valueType is not function", eventName.c_str());
        return;
    }
    auto& callbackList = GetCallbackList(eventName);
    std::lock_guard<std::mutex> lock(callbackList.listMutex);
    for (auto it = callbackList.refList.begin(); it != callbackList.refList.end(); ++it) {
        bool isSameCallback = CameraNapiUtils::IsSameNapiValue(env_, callback, it->GetCallbackFunction());
        CHECK_RETURN_ELOG(isSameCallback, "SaveCallbackReference: has same callback, nothing to do");
    }
    callbackList.refList.emplace_back(AutoRef(env_, callback, isOnce));
    MEDIA_DEBUG_LOG("Save callback reference success, %s callback list size [%{public}zu]", eventName.c_str(),
        callbackList.refList.size());
}

void ListenerBase::RemoveCallbackRef(const std::string eventName, napi_value callback)
{
    if (callback == nullptr) {
        MEDIA_DEBUG_LOG("RemoveCallbackReference: js callback is nullptr, remove all callback reference");
        RemoveAllCallbacks(eventName);
        return;
    }
    auto& callbackList = GetCallbackList(eventName);
    std::lock_guard<std::mutex> lock(callbackList.listMutex);
    for (auto it = callbackList.refList.begin(); it != callbackList.refList.end(); ++it) {
        bool isSameCallback = CameraNapiUtils::IsSameNapiValue(env_, callback, it->GetCallbackFunction());
        if (isSameCallback) {
            MEDIA_INFO_LOG("RemoveCallbackReference: find %s callback, delete it", eventName.c_str());
            callbackList.refList.erase(it);
            return;
        }
    }
    MEDIA_INFO_LOG("RemoveCallbackReference: %s callback not find", eventName.c_str());
}

int32_t ListenerBase::GetCallbackCount(const std::string eventName)
{
    auto& callbackList = GetCallbackList(eventName);
    std::lock_guard<std::mutex> lock(callbackList.listMutex);
    return callbackList.refList.size();
}

void ListenerBase::ExecuteCallback(const std::string eventName, const ExecuteCallbackNapiPara& callbackPara) const
{
    MEDIA_DEBUG_LOG("ListenerBase::ExecuteCallback is called");
    auto& callbackList = GetCallbackList(eventName);
    std::lock_guard<std::mutex> lock(callbackList.listMutex);
    for (auto it = callbackList.refList.begin(); it != callbackList.refList.end();) {
        napi_call_function(env_, callbackPara.recv, it->GetCallbackFunction(), callbackPara.argc, callbackPara.argv,
            callbackPara.result);
        if (it->isOnce_) {
            MEDIA_DEBUG_LOG("ListenerBase::ExecuteCallback, once to del %s", eventName.c_str());
            it = callbackList.refList.erase(it);
        } else {
            it++;
        }
    }
    MEDIA_DEBUG_LOG("ListenerBase::ExecuteCallback, %s callback list size [%{public}zu]", eventName.c_str(),
        callbackList.refList.size());
}

void ListenerBase::ExecuteCallbackScopeSafe(
    const std::string eventName, const std::function<ExecuteCallbackData()> fun, bool isAsync) const
{
    napi_handle_scope scope_ = nullptr;
    if (!env_) {
        MEDIA_ERR_LOG("ListenerBase::ExecuteCallbackScopeSafe env is nullptr");
        return;
    }
    napi_status status = napi_open_handle_scope(env_, &scope_);
    CHECK_RETURN_ELOG(
        status != napi_ok || scope_ == nullptr, "ListenerBase::ExecuteCallbackScopeSafe napi_open_handle_scope fail");

    MEDIA_DEBUG_LOG("ListenerBase::ExecuteCallbackScopeSafe %{public}s is called", eventName.c_str());
    auto& callbackList = GetCallbackList(eventName);
    std::lock_guard<std::mutex> lock(callbackList.listMutex);
    for (auto it = callbackList.refList.begin(); it != callbackList.refList.end();) {
        // Do not move this call out of loop.
        ExecuteCallbackData callbackData = fun();
        if (callbackData.env_ == nullptr) {
            MEDIA_ERR_LOG("ExecuteCallbackScopeSafe %{public}s env is null ", eventName.c_str());
            continue;
        }

        napi_value retVal;
        if (isAsync) {
            napi_value result[ARGS_TWO] = { nullptr, nullptr };
            result[0] = callbackData.errCode_;
            result[1] = callbackData.returnData_;

            napi_call_function(callbackData.env_, nullptr, it->GetCallbackFunction(), ARGS_TWO, result, &retVal);
        } else {
            napi_value argv[ARGS_ONE] = { nullptr };
            argv[0] = callbackData.returnData_;
            napi_call_function(callbackData.env_, nullptr, it->GetCallbackFunction(), ARGS_ONE, argv, &retVal);
        }
        if (it->isOnce_) {
            it = callbackList.refList.erase(it);
        } else {
            it++;
        }
    }
    MEDIA_DEBUG_LOG("ListenerBase::ExecuteCallbackScopeSafe, %s callback list size [%{public}zu]", eventName.c_str(),
        callbackList.refList.size());

    status = napi_close_handle_scope(env_, scope_);
    CHECK_RETURN_ELOG(status != napi_ok, "ListenerBase::ExecuteCallbackScopeSafe napi_close_handle_scope fail");
}

void ListenerBase::RemoveAllCallbacks(const std::string eventName)
{
    auto& callbackList = GetCallbackList(eventName);
    std::lock_guard<std::mutex> lock(callbackList.listMutex);
    callbackList.refList.clear();
    MEDIA_DEBUG_LOG("RemoveAllCallbacks: remove all js callbacks success");
}

bool ListenerBase::IsEmpty(const std::string eventName) const
{
    auto& callbackList = GetCallbackList(eventName);
    std::lock_guard<std::mutex> lock(callbackList.listMutex);
    return callbackList.refList.empty();
}

void ListenerBase::CleanUp(void* data)
{
    MEDIA_INFO_LOG("ListenerBase::CleanUp enter");
    auto* ctx = reinterpret_cast<HookContext*>(data);
    if (ctx == nullptr) {
        return;
    }

    ListenerBase* listener = nullptr;
    {
        std::lock_guard<std::mutex> lock(ctx->mutex);
        ctx->hookFired = true;
        listener = ctx->listener; // may be nullptr if destructor already ran
    }
    if (listener != nullptr) {
        listener->CleanUpImpl();
    }
    // ctx is freed by the destructor path (either the in-thread branch or
    // the napi_send_event task). Never free it here to avoid a race with
    // a destructor that might run concurrently on another thread.
}

void ListenerBase::CleanUpImpl()
{
    MEDIA_INFO_LOG("ListenerBase::CleanUpImpl enter");
    ClearNamedCallbackMap();
    env_ = nullptr;
}

} // namespace CameraStandard
} // namespace OHOS
