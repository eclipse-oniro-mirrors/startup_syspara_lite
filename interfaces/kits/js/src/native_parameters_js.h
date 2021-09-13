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
#ifndef NATIVE_PARAMETERS_JS_H
#define NATIVE_PARAMETERS_JS_H

#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <map>
#include <fcntl.h>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <string>
#include <unistd.h>

#include "hilog/log.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "param_wrapper.h"
#include "parameter.h"

namespace {
const static int MAX_LENGTH = 128;
const static int BUF_LENGTH = 256;
const static int ARGC_NUMBER = 2;
const static int ARGC_THREE_NUMBER = 3;

#define PARAM_JS_CHECK(retCode, exper, ...) \
    if (!(retCode)) {                       \
        HiLog::Error(LABEL, __VA_ARGS__);   \
        exper;                              \
    }

using StorageAsyncContext = struct {
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
    std::map<uint32_t, napi_ref> callbackReferences{};
} ;
} // namespace

EXTERN_C_START
napi_value GetWatcher(napi_env env, napi_callback_info info);
napi_value ParamWait(napi_env env, napi_callback_info info);
napi_value RegisterWatcher(napi_env env, napi_value exports);
EXTERN_C_END

#endif // NATIVE_PARAMETERS_JS_H