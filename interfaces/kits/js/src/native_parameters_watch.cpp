/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <functional>
#include <vector>
#include "native_parameters_js.h"
static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, 0, "StartupParametersJs" };
using namespace OHOS::HiviewDFX;
using namespace OHOS::system;
static constexpr int ARGC_NUMBER = 2;
static constexpr int BUF_LENGTH = 256;

static napi_ref g_paramWatchRef;

using ParamAsyncContext = struct {
    napi_env env = nullptr;
    napi_async_work work = nullptr;

    char key[BUF_LENGTH] = { 0 };
    size_t keyLen = 0;
    char value[BUF_LENGTH] = { 0 };
    size_t valueLen = 0;
    int32_t timeout;
    napi_deferred deferred = nullptr;
    napi_ref callbackRef = nullptr;

    int status = -1;
    std::string getValue;
};

using ParamWatcher = struct {
    napi_env env = nullptr;
    napi_ref thisVarRef = nullptr;
    char keyPrefix[BUF_LENGTH] = { 0 };
    size_t keyLen = 0;
    bool notifySwitch = false;
    bool startWatch = false;
    void ProcessParamChange(const char *key, const char *value);

    std::mutex mutex {};
    std::map<uint32_t, napi_ref> callbackReferences {};
};

static napi_value NapiGetNull(napi_env env)
{
    napi_value result = 0;
    napi_get_null(env, &result);
    return result;
}

static napi_value GetNapiValue(napi_env env, int val)
{
    napi_value result = nullptr;
    napi_create_int32(env, val, &result);
    return result;
}

static int GetParamValue(napi_env env, napi_value arg, napi_valuetype valueType, char *buffer, size_t &buffLen)
{
    napi_valuetype type = napi_null;
    napi_typeof(env, arg, &type);
    PARAM_JS_CHECK(type == valueType, return -1, "Invalid type %d %d", type, valueType);
    napi_status status = napi_ok;
    if (valueType == napi_string) {
        status = napi_get_value_string_utf8(env, arg, buffer, buffLen, &buffLen);
    } else if (valueType == napi_number) {
        status = napi_get_value_int32(env, arg, (int *)buffer);
    }
    return status;
}

static void WaitCallbackWork(napi_env env, ParamAsyncContext *asyncContext)
{
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "JSStartupGet", NAPI_AUTO_LENGTH, &resource);
    napi_create_async_work(
        env, nullptr, resource,
        [](napi_env env, void *data) {
            ParamAsyncContext *asyncContext = (ParamAsyncContext *)data;
            asyncContext->status = WaitParameter(asyncContext->key, asyncContext->value, asyncContext->timeout);
            HiLog::Debug(LABEL, "JSApp Wait status: %{public}d, key: %{public}s",
                asyncContext->status, asyncContext->key);
        },
        [](napi_env env, napi_status status, void *data) {
            ParamAsyncContext *asyncContext = (ParamAsyncContext *)data;
            napi_value result[ARGC_NUMBER] = { 0 };
            napi_value message = nullptr;
            napi_create_object(env, &result[0]);
            napi_create_int32(env, asyncContext->status, &message);
            napi_set_named_property(env, result[0], "code", message);
            napi_get_undefined(env, &result[1]); // only one param

            if (asyncContext->deferred) {
                if (asyncContext->status == 0) {
                    napi_resolve_deferred(env, asyncContext->deferred, result[1]);
                } else {
                    napi_reject_deferred(env, asyncContext->deferred, result[0]);
                }
            } else {
                napi_value callbackRef = nullptr;
                napi_value callResult = nullptr;
                napi_get_reference_value(env, asyncContext->callbackRef, &callbackRef);
                napi_value undefined;
                napi_get_undefined(env, &undefined);
                napi_call_function(env, undefined, callbackRef, ARGC_NUMBER, result, &callResult);
                napi_delete_reference(env, asyncContext->callbackRef);
            }
            napi_delete_async_work(env, asyncContext->work);
            delete asyncContext;
        },
        (void *)asyncContext, &asyncContext->work);
    napi_queue_async_work(env, asyncContext->work);
}

napi_value ParamWait(napi_env env, napi_callback_info info)
{
    constexpr int PARAM_TIMEOUT_INDEX = 2;
    constexpr int ARGC_THREE_NUMBER = 3;
    size_t argc = ARGC_THREE_NUMBER + 1;
    napi_value argv[ARGC_THREE_NUMBER + 1];
    napi_value thisVar = nullptr;
    napi_status status = napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr);
    PARAM_JS_CHECK(status == napi_ok, return GetNapiValue(env, status), "Failed to get cb info");
    PARAM_JS_CHECK(argc >= ARGC_THREE_NUMBER, return GetNapiValue(env, status), "Failed to get argc");

    auto *asyncContext = new ParamAsyncContext();
    PARAM_JS_CHECK(asyncContext != nullptr, return GetNapiValue(env, status), "Failed to create context");
    asyncContext->env = env;

    // get param key
    asyncContext->keyLen = BUF_LENGTH - 1;
    asyncContext->valueLen = BUF_LENGTH - 1;
    size_t len = sizeof(asyncContext->timeout);
    int ret = GetParamValue(env, argv[0], napi_string, asyncContext->key, asyncContext->keyLen);
    PARAM_JS_CHECK(ret == 0, delete asyncContext;
        return GetNapiValue(env, ret), "Invalid param for wait");
    ret = GetParamValue(env, argv[1], napi_string, asyncContext->value, asyncContext->valueLen);
    PARAM_JS_CHECK(ret == 0, delete asyncContext;
        return GetNapiValue(env, ret), "Invalid param for wait");
    ret = GetParamValue(env, argv[PARAM_TIMEOUT_INDEX], napi_number, (char *)&asyncContext->timeout, len);
    PARAM_JS_CHECK(ret == 0, delete asyncContext;
        return GetNapiValue(env, ret), "Invalid param for wait");
    if (argc > ARGC_THREE_NUMBER) {
        napi_valuetype valueType = napi_null;
        napi_typeof(env, argv[ARGC_THREE_NUMBER], &valueType);
        PARAM_JS_CHECK(valueType == napi_function, delete asyncContext;
            return GetNapiValue(env, ret), "Invalid param for wait callbackRef");
        napi_create_reference(env, argv[ARGC_THREE_NUMBER], 1, &asyncContext->callbackRef);
    }
    HiLog::Debug(LABEL, "JSApp Wait key: %{public}s, value: %{public}s callbackRef %p.",
        asyncContext->key, asyncContext->value, asyncContext->callbackRef);

    napi_value result = nullptr;
    if (asyncContext->callbackRef == nullptr) {
        napi_create_promise(env, &asyncContext->deferred, &result);
    } else {
        napi_get_undefined(env, &result);
    }
    WaitCallbackWork(env, asyncContext);
    return GetNapiValue(env, 0);
}

static bool GetFristRefence(ParamWatcher *watcher, uint32_t &next)
{
    std::lock_guard<std::mutex> lock(watcher->mutex);
    auto iter = watcher->callbackReferences.begin();
    if (iter != watcher->callbackReferences.end()) {
        next = watcher->callbackReferences.begin()->first;
        return true;
    }
    return false;
}

static napi_ref GetWatcherReference(ParamWatcher *watcher, uint32_t next)
{
    std::lock_guard<std::mutex> lock(watcher->mutex);
    auto iter = watcher->callbackReferences.find(next);
    if (iter != watcher->callbackReferences.end()) {
        return iter->second;
    }
    return nullptr;
}

static uint32_t GetNextRefence(ParamWatcher *watcher, uint32_t &next)
{
    std::lock_guard<std::mutex> lock(watcher->mutex);
    auto iter = watcher->callbackReferences.upper_bound(next);
    if (iter == watcher->callbackReferences.end()) {
        return false;
    }
    next = iter->first;
    return true;
}

static void DelCallback(napi_env env, napi_value callback, ParamWatcher *watcher)
{
    bool isEquals = false;
    uint32_t next = 0;
    bool ret = GetFristRefence(watcher, next);
    while (ret) {
        napi_ref callbackRef = GetWatcherReference(watcher, next);
        if (callbackRef == nullptr) {
            ret = GetNextRefence(watcher, next);
            continue;
        }
        if (callback != nullptr) {
            napi_value handler = nullptr;
            napi_get_reference_value(env, callbackRef, &handler);
            napi_strict_equals(env, handler, callback, &isEquals);
            if (isEquals) {
                HiLog::Debug(LABEL, "JSApp watcher delete key %{public}s %{public}u.", watcher->keyPrefix, next);
                napi_delete_reference(env, callbackRef);
                watcher->callbackReferences.erase(next);
                return;
            }
        } else {
            napi_delete_reference(env, callbackRef);
            watcher->callbackReferences.erase(next);
        }
        ret = GetNextRefence(watcher, next);
    }
    HiLog::Debug(LABEL, "JSApp watcher delete key %{public}s all.", watcher->keyPrefix);
    watcher->callbackReferences.clear();
}

static bool CheckCallbackEqual(napi_env env, napi_value callback, ParamWatcher *watcher)
{
    bool isEquals = false;
    uint32_t next = 0;
    bool ret = GetFristRefence(watcher, next);
    while (ret) {
        napi_ref callbackRef = GetWatcherReference(watcher, next);
        if (callbackRef != nullptr) {
            napi_value handler = nullptr;
            napi_get_reference_value(env, callbackRef, &handler);
            napi_strict_equals(env, handler, callback, &isEquals);
            if (isEquals) {
                return true;
            }
        }
        ret = GetNextRefence(watcher, next);
    }
    return false;
}

static napi_value ParamWatchConstructor(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, nullptr));
    ParamWatcher *watcher = new ParamWatcher();
    PARAM_JS_CHECK(watcher != nullptr, return NapiGetNull(env), "Failed to create param watcher");
    napi_status status = napi_create_reference(env, thisVar, 1, &watcher->thisVarRef);
    PARAM_JS_CHECK(status == 0, delete watcher;
        return NapiGetNull(env), "Failed to create reference %d", status);
    HiLog::Debug(LABEL, "JSApp watcher this = %{public}p ", watcher);

    napi_wrap(
        env, thisVar, watcher,
        [](napi_env env, void *data, void *hint) {
            ParamWatcher *watcher = (ParamWatcher *)data;
            HiLog::Debug(LABEL, "JSApp watcher this = %{public}p, destruct", watcher);
            if (watcher) {
                DelCallback(env, nullptr, watcher);
                WatchParameter(watcher->keyPrefix, nullptr, nullptr);
                delete watcher;
                watcher = nullptr;
            }
        },
        nullptr, nullptr);
    return thisVar;
}

napi_value GetWatcher(napi_env env, napi_callback_info info)
{
    constexpr int MAX_LENGTH = 128;
    size_t argc = 1;
    napi_value argv[1];
    napi_value thisVar = nullptr;
    void *data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, &thisVar, &data));

    napi_value obj = thisVar;
    ParamWatcher *watcher = nullptr;
    napi_unwrap(env, thisVar, (void **)&watcher);
    if (watcher == nullptr) { // check if watcher exist
        napi_value constructor = nullptr;
        int status = napi_get_reference_value(env, g_paramWatchRef, &constructor);
        status = napi_new_instance(env, constructor, 0, nullptr, &obj);
        PARAM_JS_CHECK(status == 0, return NapiGetNull(env), "Failed to create instance for watcher");
        napi_unwrap(env, obj, (void **)&watcher);
    }
    if (watcher != nullptr) {
        watcher->keyLen = MAX_LENGTH;
        int ret = GetParamValue(env, argv[0], napi_string, watcher->keyPrefix, watcher->keyLen);
        PARAM_JS_CHECK(ret == 0, return NapiGetNull(env), "Failed to get key prefix");
        HiLog::Debug(LABEL, "JSApp watcher keyPrefix = %{public}s %{public}p.", watcher->keyPrefix, watcher);
    }
    return obj;
}

static ParamWatcher *GetWatcherInfo(napi_env env, napi_callback_info info, napi_value *callback)
{
    size_t argc = 2;
    napi_value argv[2];
    napi_value thisVar = nullptr;
    void *data = nullptr;
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data);

    size_t typeLen = 32;
    std::vector<char> eventType(typeLen);
    int ret = GetParamValue(env, argv[0], napi_string, eventType.data(), typeLen);
    PARAM_JS_CHECK(ret == 0, return nullptr, "Failed to get event type");
    if (strncmp(eventType.data(), "valueChange", typeLen) != 0) {
        return nullptr;
    }
    // argv[1]:callbackRef
    if (argc > 1) {
        napi_valuetype valuetype;
        napi_status status = napi_typeof(env, argv[1], &valuetype);
        PARAM_JS_CHECK(status == 0, return nullptr, "Failed to get type");
        PARAM_JS_CHECK(valuetype == napi_function, return nullptr, "Invalid type %d", valuetype);
        *callback = argv[1];
    }
    ParamWatcher *watcher = nullptr;
    napi_unwrap(env, thisVar, (void **)&watcher);
    return watcher;
}

static void ProcessParamChange(const char *key, const char *value, void *context)
{
    ParamWatcher *watcher = (ParamWatcher *)context;
    PARAM_JS_CHECK(watcher != nullptr, return, "Invalid param");
    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(watcher->env, &scope);
    napi_value result[ARGC_NUMBER] = { 0 };
    napi_create_string_utf8(watcher->env, key, strlen(key), &result[0]);
    napi_create_string_utf8(watcher->env, value, strlen(value), &result[1]);
    napi_value thisVar = nullptr;
    napi_get_reference_value(watcher->env, watcher->thisVarRef, &thisVar);

    uint32_t next = 0;
    bool ret = GetFristRefence(watcher, next);
    while (ret) {
        napi_ref callbackRef = GetWatcherReference(watcher, next);
        if (callbackRef != nullptr) {
            napi_value callbackFunc = nullptr;
            napi_get_reference_value(watcher->env, callbackRef, &callbackFunc);
            napi_value callbackResult = nullptr;
            napi_call_function(watcher->env, thisVar, callbackFunc, ARGC_NUMBER, result, &callbackResult);
        }
        ret = GetNextRefence(watcher, next);
    }
    napi_close_handle_scope(watcher->env, scope);
    HiLog::Debug(LABEL, "JSApp watcher notify param key:%{public}s key:%{public}s.", key, value);
}

static napi_value SwithWatchOn(napi_env env, napi_callback_info info)
{
    napi_value callback = nullptr;
    ParamWatcher *watcher = GetWatcherInfo(env, info, &callback);
    PARAM_JS_CHECK(watcher != nullptr, return GetNapiValue(env, -1), "Failed to get watcher swith param");

    if (!watcher->startWatch) {
        int ret = WatchParameter(watcher->keyPrefix, ProcessParamChange, watcher);
        PARAM_JS_CHECK(ret == 0, return GetNapiValue(env, ret), "Failed to start watcher ret %{public}d", ret);
        watcher->startWatch = true;
    }

    if (CheckCallbackEqual(env, callback, watcher)) {
        HiLog::Warn(LABEL, "JSApp watcher repeater switch on %{public}s", watcher->keyPrefix);
        return 0;
    }

    // save callback
    napi_ref callbackRef;
    napi_create_reference(env, callback, 1, &callbackRef);
    {
        static uint32_t watcherId = 0;
        std::lock_guard<std::mutex> lock(watcher->mutex);
        watcherId++;
        HiLog::Debug(LABEL, "JSApp watcher add key %{public}s %{public}u all.", watcher->keyPrefix, watcherId);
        watcher->callbackReferences[watcherId] = callbackRef;
    }
    watcher->env = env;
    return GetNapiValue(env, 0);
}

static napi_value SwithWatchOff(napi_env env, napi_callback_info info)
{
    napi_value callback = nullptr;
    ParamWatcher *watcher = GetWatcherInfo(env, info, &callback);
    PARAM_JS_CHECK(watcher != nullptr, return GetNapiValue(env, -1), "Failed to get watcher");

    if (callback != nullptr) {
        if (!CheckCallbackEqual(env, callback, watcher)) {
            HiLog::Error(LABEL, "JSApp watcher switch off callback is not exist %{public}s", watcher->keyPrefix);
            return GetNapiValue(env, -1);
        }
    }
    DelCallback(env, callback, watcher);
    return GetNapiValue(env, 0);
}

napi_value RegisterWatcher(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        DECLARE_NAPI_FUNCTION("on", SwithWatchOn),
        DECLARE_NAPI_FUNCTION("off", SwithWatchOff),
    };

    napi_value result = nullptr;
    NAPI_CALL(env,
        napi_define_class(env,
            "paramWatcher",
            NAPI_AUTO_LENGTH,
            ParamWatchConstructor,
            nullptr,
            sizeof(properties) / sizeof(*properties),
            properties,
            &result));
    napi_set_named_property(env, exports, "paramWatcher", result);
    napi_create_reference(env, result, 1, &g_paramWatchRef);
    return exports;
}