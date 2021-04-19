/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
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

#include "parameter.h"
#include <securec.h>
#include "hal_sys_param.h"
#include "ohos_errno.h"
#include "param_adaptor.h"

#define FILE_RO "ro."
#define VERSION_ID_LEN 256
#define PROPERTY_MAX_LENGTH 2048

static const char OHOS_OS_NAME[] = {"OpenHarmony"};
static const int  OHOS_SDK_API_VERSION = 3;
static const char OHOS_SECURITY_PATCH_TAG[] = {"2020-09-01"};
static const char OHOS_RELEASE_TYPE[] = {"Beta"};

static const int MAJOR_VERSION = 1;
static const int SENIOR_VERSION = 0;
static const int FEATURE_VERSION = 1;
static const int BUILD_VERSION = 0;

static const char EMPTY_STR[] = {""};

static boolean IsValidValue(const char* value, unsigned int len)
{
    if ((value == NULL) || !strlen(value) || (strlen(value) + 1 > len)) {
        return FALSE;
    }
    return TRUE;
}

int GetParameter(const char* key, const char* def, char* value, unsigned int len)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_INVALID;
    }
    if (!CheckPermission()) {
        return EC_FAILURE;
    }
    int ret = GetSysParam(key, value, len);
    if (ret == EC_INVALID) {
        return EC_INVALID;
    }
    if ((ret < 0) && IsValidValue(def, len)) {
        if (strcpy_s(value, len, def) != 0) {
            return EC_FAILURE;
        }
        ret = strlen(def);
    }
    return ret;
}

int SetParameter(const char* key, const char* value)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_INVALID;
    }
    if (!CheckPermission()) {
        return EC_FAILURE;
    }
    if (strncmp(key, FILE_RO, strlen(FILE_RO)) == 0) {
        return EC_INVALID;
    }

    return SetSysParam(key, value);
}

const char* GetDeviceType(void)
{
    return HalGetDeviceType();
}

const char* GetManufacture(void)
{
    return HalGetManufacture();
}

const char* GetBrand(void)
{
    return HalGetBrand();
}

const char* GetMarketName(void)
{
    return HalGetMarketName();
}

const char* GetProductSeries(void)
{
    return HalGetProductSeries();
}

const char* GetProductModel(void)
{
    return HalGetProductModel();
}

const char* GetSoftwareModel(void)
{
    return HalGetSoftwareModel();
}

const char* GetHardwareModel(void)
{
    return HalGetHardwareModel();
}

const char* GetHardwareProfile(void)
{
    return HalGetHardwareProfile();
}

const char* GetSerial(void)
{
    return HalGetSerial();
}

const char* GetBootloaderVersion(void)
{
    return HalGetBootloaderVersion();
}

const char* GetSecurityPatchTag(void)
{
    return OHOS_SECURITY_PATCH_TAG;
}

const char* GetAbiList(void)
{
    return HalGetAbiList();
}

const char* GetOSFullName(void)
{
    static const char* osFullName = NULL;
    static const char release[] = "Release";

    if (osFullName != NULL) {
        return osFullName;
    }
    char* value = (char*)malloc(VERSION_ID_LEN);
    if (value == NULL) {
        return EMPTY_STR;
    }
    if (memset_s(value, VERSION_ID_LEN, 0, VERSION_ID_LEN) != 0) {
        free(value);
        value = NULL;
        return EMPTY_STR;
    }
    const char* releaseType = GetOsReleaseType();
    int length;
    if (strncmp(releaseType, release, sizeof(release) - 1) == 0) {
        length = sprintf_s(value, VERSION_ID_LEN, "%s-%d.%d.%d.%d",
            OHOS_OS_NAME, MAJOR_VERSION, SENIOR_VERSION, FEATURE_VERSION, BUILD_VERSION);
    } else {
        length = sprintf_s(value, VERSION_ID_LEN, "%s-%d.%d.%d.%d(%s)",
            OHOS_OS_NAME, MAJOR_VERSION, SENIOR_VERSION, FEATURE_VERSION, BUILD_VERSION, releaseType);
    }
    if (length < 0) {
        free(value);
        value = NULL;
        return EMPTY_STR;
    }
    osFullName = strdup(value);
    free(value);
    value = NULL;
    if (osFullName == NULL) {
        return EMPTY_STR;
    }
    return osFullName;
}

const char* GetDisplayVersion(void)
{
    return HalGetDisplayVersion();
}

int GetSdkApiVersion(void)
{
    return OHOS_SDK_API_VERSION;
}

int GetFirstApiVersion(void)
{
    return HalGetFirstApiVersion();
}

const char* GetIncrementalVersion(void)
{
    return HalGetIncrementalVersion();
}

const char* GetVersionId(void)
{
    static const char* versionId = NULL;
    if (versionId != NULL) {
        return versionId;
    }
    char* value = (char*)malloc(VERSION_ID_LEN);
    if (value == NULL) {
        return EMPTY_STR;
    }
    if (memset_s(value, VERSION_ID_LEN, 0, VERSION_ID_LEN) != 0) {
        free(value);
        value = NULL;
        return EMPTY_STR;
    }
    const char* productType = GetDeviceType();
    const char* manufacture = GetManufacture();
    const char* brand = GetBrand();
    const char* productSerial = GetProductSeries();
    const char* osFullName = GetOSFullName();
    const char* productModel = GetProductModel();
    const char* softwareModel = GetSoftwareModel();
    const char* incrementalVersion = GetIncrementalVersion();
    const char* buildType = GetBuildType();
    
    int len = sprintf_s(value, VERSION_ID_LEN, "%s/%s/%s/%s/%s/%s/%s/%d/%s/%s",
        productType, manufacture, brand, productSerial, osFullName, productModel,
        softwareModel, OHOS_SDK_API_VERSION, incrementalVersion, buildType);
    if (len < 0) {
        free(value);
        value = NULL;
        return EMPTY_STR;
    }
    versionId = strdup(value);
    free(value);
    value = NULL;
    if (versionId == NULL) {
        return EMPTY_STR;
    }
    return versionId;
}

const char* GetBuildType(void)
{
    return HalGetBuildType();
}

const char* GetBuildUser(void)
{
    return HalGetBuildUser();
}

const char* GetBuildHost(void)
{
    return HalGetBuildHost();
}

const char* GetBuildTime(void)
{
    return HalGetBuildTime();
}

static const char* GetProperty(const char* propertyInfo, const size_t propertySize, const char** propertyHolder)
{
    if (*propertyHolder != NULL) {
        return *propertyHolder;
    }
    if ((propertySize == 0) || (propertySize > PROPERTY_MAX_LENGTH)) {
        return EMPTY_STR;
    }
    char* prop = (char*)malloc(propertySize);
    if (prop == NULL) {
        return EMPTY_STR;
    }
    if (strcpy_s(prop, propertySize, propertyInfo) != 0) {
        free(prop);
        prop = NULL;
        return EMPTY_STR;
    }
    *propertyHolder = prop;
    return *propertyHolder;
}

const char* GetBuildRootHash(void)
{
    static const char* buildRootHash = NULL;
    return GetProperty(BUILD_ROOTHASH, strlen(BUILD_ROOTHASH) + 1, &buildRootHash);
}

const char* GetOsReleaseType(void)
{
    return OHOS_RELEASE_TYPE;
}

int GetMajorVersion(void)
{
    return MAJOR_VERSION;
}

int GetSeniorVersion(void)
{
    return SENIOR_VERSION;
}

int GetFeatureVersion(void)
{
    return FEATURE_VERSION;
}

int GetBuildVersion(void)
{
    return BUILD_VERSION;
}