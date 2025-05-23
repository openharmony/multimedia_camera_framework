/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef CAMERA_NAPI_PARAM_PARSER_H
#define CAMERA_NAPI_PARAM_PARSER_H

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>

#include "camera_error_code.h"
#include "camera_napi_const.h"
#include "camera_napi_object.h"
#include "js_native_api.h"
#include "js_native_api_types.h"
#include "napi/native_api.h"

namespace OHOS {
namespace CameraStandard {
enum CameraNapiAsyncFunctionType : int32_t { ASYNC_FUN_TYPE_NONE, ASYNC_FUN_TYPE_CALLBACK, ASYNC_FUN_TYPE_PROMISE };
struct CameraNapiAsyncFunction {
public:
    CameraNapiAsyncFunction(napi_env env, const char* resourceName, napi_ref& callbackRef, napi_deferred& deferred)
    {
        env_ = env;
        status_ = napi_create_string_utf8(env, resourceName, NAPI_AUTO_LENGTH, &(resourceName_));
        if (status_ != napi_ok) {
            return;
        }
        callbackRefPtr_ = &callbackRef;
        deferred_ = &deferred;
    }

    inline napi_value GetResourceName()
    {
        return resourceName_;
    }

    inline bool IsStatusOk()
    {
        return status_ == napi_ok;
    }

    inline napi_value GetPromise()
    {
        if (status_ != napi_ok) {
            return nullptr;
        }
        return promise_;
    }

    inline CameraNapiAsyncFunctionType GetAsyncFunctionType()
    {
        return asyncFunctionType_;
    }

    inline bool AssertStatus(CameraErrorCode errorCode, const char* message)
    {
        if (status_ != napi_ok) {
            napi_throw_error(env_, std::to_string(errorCode).c_str(), message);
        }
        return status_ == napi_ok;
    }

    void Reset()
    {
        if (callbackRefPtr_ != nullptr && *callbackRefPtr_ != nullptr) {
            napi_delete_reference(env_, *callbackRefPtr_);
            *callbackRefPtr_ = nullptr;
        }
        if (deferred_ != nullptr && *deferred_ != nullptr) {
            napi_value rejection = nullptr;
            napi_get_undefined(env_, &rejection);
            napi_reject_deferred(env_, *deferred_, rejection);
            *deferred_ = nullptr;
        }
    }

private:
    napi_status CreatePromise()
    {
        if (status_ != napi_ok) {
            return status_;
        }
        status_ = napi_create_promise(env_, deferred_, &promise_);
        if (status_ == napi_ok) {
            asyncFunctionType_ = ASYNC_FUN_TYPE_PROMISE;
        }
        return status_;
    }

    inline napi_status CreateCallback(napi_value callback)
    {
        if (status_ != napi_ok) {
            return status_;
        }
        status_ = napi_create_reference(env_, callback, 1, callbackRefPtr_);
        if (status_ == napi_ok) {
            asyncFunctionType_ = ASYNC_FUN_TYPE_CALLBACK;
        }
        return status_;
    }

    napi_env env_ = nullptr;
    napi_status status_ = napi_invalid_arg;
    napi_value resourceName_ = nullptr;
    napi_ref* callbackRefPtr_ = nullptr;
    napi_deferred* deferred_ = nullptr;
    napi_value promise_ = nullptr;
    CameraNapiAsyncFunctionType asyncFunctionType_ = ASYNC_FUN_TYPE_NONE;

    friend class CameraNapiParamParser;
};

class CameraNapiParamParser {
public:
    template<typename T>
    explicit CameraNapiParamParser(napi_env env, napi_callback_info info, T*& nativeObjPointer)
        : CameraNapiParamParser(env, info, 0, nativeObjPointer, nullptr)
    {}

    template<typename T>
    explicit CameraNapiParamParser(napi_env env, napi_callback_info info, T*& nativeObjPointer,
        std::shared_ptr<CameraNapiAsyncFunction> asyncFunction)
        : CameraNapiParamParser(env, info, 0, nativeObjPointer, asyncFunction)
    {}

    template<typename T, typename... Args>
    explicit CameraNapiParamParser(napi_env env, napi_callback_info info, T*& nativeObjPointer, Args&... args)
        : CameraNapiParamParser(env, info, sizeof...(args), nativeObjPointer, nullptr)
    {
        if (napiError != napi_ok) {
            return;
        }
        if (paramSize_ > 0) {
            Next(args...);
        }
    }

    template<typename T, typename... Args>
    explicit CameraNapiParamParser(napi_env env, napi_callback_info info, T*& nativeObjPointer,
        std::shared_ptr<CameraNapiAsyncFunction> asyncFunction, Args&... args)
        : CameraNapiParamParser(env, info, sizeof...(args), nativeObjPointer, asyncFunction)
    {
        if (napiError != napi_ok) {
            return;
        }
        if (paramSize_ > 0) {
            Next(args...);
        }
    }

    template<typename... Args>
    explicit CameraNapiParamParser(napi_env env, std::vector<napi_value> paramValue, Args&... args)
        : env_(env), paramSize_(sizeof...(args)), paramValue_(paramValue)
    {
        if (paramSize_ != paramValue_.size()) {
            napiError = napi_status::napi_invalid_arg;
            return;
        }
        napiError = napi_ok;
        if (paramSize_ > 0) {
            Next(args...);
        }
    }

    inline bool AssertStatus(CameraErrorCode errorCode, const char* message)
    {
        if (napiError != napi_ok) {
            napi_throw_error(env_, std::to_string(errorCode).c_str(), message);
        }
        return napiError == napi_ok;
    }

    inline bool IsStatusOk()
    {
        return napiError == napi_ok;
    }

    inline napi_value GetThisVar()
    {
        return thisVar_;
    }

private:
    template<typename T>
    explicit CameraNapiParamParser(napi_env env, napi_callback_info info, size_t napiParamSize, T*& nativeObjPointer,
        std::shared_ptr<CameraNapiAsyncFunction> asyncFunction)
        : env_(env), asyncFunction_(asyncFunction)
    {
        size_t paramSizeIncludeAsyncFun = napiParamSize + (asyncFunction_ == nullptr ? 0 : 1);
        paramSize_ = paramSizeIncludeAsyncFun;
        paramValue_.resize(paramSize_, nullptr);
        napiError = napi_get_cb_info(env_, info, &paramSize_, paramValue_.data(), &thisVar_, nullptr);
        if (napiError != napi_ok) {
            return;
        }
        if (asyncFunction_ != nullptr) {
            asyncFunction->Reset();
            // Check callback function
            if (paramSize_ > 0 && paramSize_ == paramSizeIncludeAsyncFun) {
                napi_valuetype napiType = napi_undefined;
                napi_typeof(env, paramValue_[paramSize_ - 1], &napiType);
                if (napiType == napi_function) {
                    napiError = asyncFunction_->CreateCallback(paramValue_[paramSize_ - 1]);
                } else {
                    napiError = napi_status::napi_invalid_arg;
                }
            } else if (paramSizeIncludeAsyncFun > 0 && paramSize_ == paramSizeIncludeAsyncFun - 1) {
                napiError = asyncFunction_->CreatePromise();
            } else {
                napiError = napi_status::napi_invalid_arg;
            }
            if (napiError != napi_ok) {
                return;
            }
        } else if (paramSize_ != napiParamSize) {
            napiError = napi_status::napi_invalid_arg;
            return;
        }
        UnwrapThisVarToAddr(thisVar_, nativeObjPointer);
    }

    template<typename T>
    void UnwrapThisVarToAddr(napi_value thisVar, T*& dataPointAddr)
    {
        if (napiError != napi_ok) {
            return;
        }
        if (thisVar == nullptr) {
            napiError = napi_invalid_arg;
            return;
        }
        napiError = napi_unwrap(env_, thisVar, reinterpret_cast<void**>(&dataPointAddr));
        if (napiError == napi_ok && dataPointAddr == nullptr) {
            napiError = napi_invalid_arg;
        }
    }

    CameraNapiParamParser& Next(CameraNapiObject& cameraNapiOjbect)
    {
        if (napiError != napi_status::napi_ok) {
            return *this;
        }
        if (paraIndex_ >= paramSize_) {
            napiError = napi_status::napi_invalid_arg;
            return *this;
        }
        napiError = cameraNapiOjbect.ParseNapiObjectToMap(env_, paramValue_[paraIndex_]);
        paraIndex_++;
        return *this;
    }

    template<typename T>
    CameraNapiParamParser& Next(T*& outData)
    {
        if (napiError != napi_status::napi_ok) {
            return *this;
        }
        if (paraIndex_ >= paramSize_) {
            napiError = napi_status::napi_invalid_arg;
            return *this;
        }
        napi_valuetype valueNapiType = napi_undefined;
        napi_typeof(env_, paramValue_[paraIndex_], &valueNapiType);
        if (valueNapiType == napi_object) {
            napiError = napi_unwrap(env_, paramValue_[paraIndex_], reinterpret_cast<void**>(&outData));
            if (napiError == napi_ok && outData == nullptr) {
                napiError = napi_invalid_arg;
            }
        } else {
            napiError = napi_status::napi_invalid_arg;
        }
        paraIndex_++;
        return *this;
    }

    template<typename T, typename = std::enable_if_t<std::is_same_v<T, bool> || std::is_same_v<T, int32_t> ||
                                                     std::is_same_v<T, int64_t> || std::is_same_v<T, uint32_t> ||
                                                     std::is_same_v<T, double> || std::is_same_v<T, std::string>>>
    CameraNapiParamParser& Next(T& outData)
    {
        if (napiError != napi_status::napi_ok) {
            return *this;
        }
        if (paraIndex_ >= paramSize_) {
            napiError = napi_status::napi_invalid_arg;
            return *this;
        }
        napi_valuetype valueNapiType = napi_undefined;
        napi_typeof(env_, paramValue_[paraIndex_], &valueNapiType);
        if (std::is_same_v<T, bool> && valueNapiType == napi_boolean) {
            napiError = napi_get_value_bool(env_, paramValue_[paraIndex_], (bool*)(&outData));
        } else if (std::is_same_v<T, int32_t> && valueNapiType == napi_number) {
            napiError = napi_get_value_int32(env_, paramValue_[paraIndex_], (int32_t*)(&outData));
        } else if (std::is_same_v<T, int64_t> && valueNapiType == napi_number) {
            napiError = napi_get_value_int64(env_, paramValue_[paraIndex_], (int64_t*)(&outData));
        } else if (std::is_same_v<T, uint32_t> && valueNapiType == napi_number) {
            napiError = napi_get_value_uint32(env_, paramValue_[paraIndex_], (uint32_t*)(&outData));
        } else if (std::is_same_v<T, double> && valueNapiType == napi_number) {
            napiError = napi_get_value_double(env_, paramValue_[paraIndex_], (double*)(&outData));
        } else if (std::is_same_v<T, std::string> && valueNapiType == napi_string) {
            size_t stringSize = 0;
            napiError = napi_get_value_string_utf8(env_, paramValue_[paraIndex_], nullptr, 0, &stringSize);
            if (napiError != napi_ok) {
                paraIndex_++;
                return *this;
            }
            std::string* stringPtr = (std::string*)(&outData);
            stringPtr->resize(stringSize);
            napiError = napi_get_value_string_utf8(
                env_, paramValue_[paraIndex_], stringPtr->data(), stringSize + 1, &stringSize);
            if (napiError != napi_ok) {
                paraIndex_++;
                return *this;
            }
        } else {
            napiError = napi_status::napi_invalid_arg;
        }
        paraIndex_++;
        return *this;
    }

    template<typename T, typename... Args>
    CameraNapiParamParser& Next(T& outData, Args&... args)
    {
        Next(outData);
        if (sizeof...(args) > 0) {
            Next(args...);
        }
        return *this;
    }

    napi_env env_ = nullptr;
    napi_value thisVar_ = nullptr;
    size_t paramSize_ = 0;

    size_t paraIndex_ = 0;
    std::vector<napi_value> paramValue_ {};
    std::shared_ptr<CameraNapiAsyncFunction> asyncFunction_ = nullptr;
    napi_status napiError = napi_status::napi_invalid_arg;
};
} // namespace CameraStandard
} // namespace OHOS
#endif