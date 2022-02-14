/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include <string.h>
#include <openssl/sha.h>

#include "parameter_hal.h"
#include "sysparam_errno.h"
#include "sysversion.h"

#define OS_FULL_NAME_LEN 128
#define VERSION_ID_MAX_LEN 256

static const char OHOS_OS_NAME[] = { "OpenHarmony" };
static const int OHOS_SDK_API_LEVEL = 8;
static const char OHOS_BUILD_ROOT_HASH[] = { "default" };
static const char OHOS_SECURITY_PATCH_TAG[] = { "2020-09-01" };
static const char OHOS_RELEASE_TYPE[] = { "Canary1" };
static const int DEV_BUF_LENGTH = 3;
static const int DEV_BUF_MAX_LENGTH = 1024;
static const int DEV_UUID_LENGTH = 65;

static const char EMPTY_STR[] = { "" };

int GetParameter(const char *key, const char *def, char *value, unsigned int len)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_INVALID;
    }
    int ret = HalGetParameter(key, def, value, len);
    return (ret < 0) ? ret : strlen(value);
}

int SetParameter(const char *key, const char *value)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_INVALID;
    }
    return HalSetParameter(key, value);
}

int WaitParameter(const char *key, const char *value, int timeout)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_INVALID;
    }
    return HalWaitParameter(key, value, timeout);
}

const char *GetDeviceType(void)
{
    return HalGetDeviceType();
}

const char *GetProductModel(void)
{
    return HalGetProductModel();
}

const char *GetManufacture(void)
{
    return HalGetManufacture();
}

const char *GetBrand(void)
{
    return HalGetBrand();
}

const char *GetMarketName(void)
{
    return HalGetMarketName();
}

const char *GetProductSeries(void)
{
    return HalGetProductSeries();
}

const char *GetSoftwareModel(void)
{
    return HalGetSoftwareModel();
}

const char *GetHardwareModel(void)
{
    return HalGetHardwareModel();
}

const char *GetHardwareProfile(void)
{
    return HalGetHardwareProfile();
}

const char *GetSerial(void)
{
    return HalGetSerial();
}

const char *GetSecurityPatchTag(void)
{
    return OHOS_SECURITY_PATCH_TAG;
}

const char *GetAbiList(void)
{
    return HalGetAbiList();
}

const char *GetBootloaderVersion(void)
{
    return HalGetBootloaderVersion();
}

static const char *BuildOSFullName(void)
{
    const char release[] = "Release";
    char value[OS_FULL_NAME_LEN];
    const char *releaseType = GetOsReleaseType();
    int length;
    if (strncmp(releaseType, release, sizeof(release) - 1) == 0) {
        length = sprintf_s(value, OS_FULL_NAME_LEN, "%s-%d.%d.%d.%d",
            OHOS_OS_NAME, GetMajorVersion(), GetSeniorVersion(), GetFeatureVersion(), GetBuildVersion());
    } else {
        length = sprintf_s(value, OS_FULL_NAME_LEN, "%s-%d.%d.%d.%d(%s)",
            OHOS_OS_NAME, GetMajorVersion(), GetSeniorVersion(), GetFeatureVersion(), GetBuildVersion(), releaseType);
    }
    if (length < 0) {
        return EMPTY_STR;
    }
    const char *osFullName = strdup(value);
    return osFullName;
}

const char *GetOSFullName(void)
{
    static const char *osFullName = NULL;
    if (osFullName != NULL) {
        return osFullName;
    }
    osFullName = BuildOSFullName();
    if (osFullName == NULL) {
        return EMPTY_STR;
    }
    return osFullName;
}

int GetSdkApiVersion(void)
{
    return OHOS_SDK_API_LEVEL;
}

int GetFirstApiVersion(void)
{
    return HalGetFirstApiVersion();
}

const char *GetDisplayVersion(void)
{
    return HalGetDisplayVersion();
}

const char *GetIncrementalVersion(void)
{
    return HalGetIncrementalVersion();
}

static const char *BuildVersionId(void)
{
    char value[VERSION_ID_MAX_LEN];
    int len = sprintf_s(value, VERSION_ID_MAX_LEN, "%s/%s/%s/%s/%s/%s/%s/%d/%s/%s",
        GetDeviceType(), GetManufacture(), GetBrand(), GetProductSeries(),
        GetOSFullName(), GetProductModel(), GetSoftwareModel(),
        OHOS_SDK_API_LEVEL, GetIncrementalVersion(), GetBuildType());
    if (len < 0) {
        return EMPTY_STR;
    }
    const char *versionId = strdup(value);
    return versionId;
}

const char *GetVersionId(void)
{
    static const char *ohosVersionId = NULL;
    if (ohosVersionId != NULL) {
        return ohosVersionId;
    }
    ohosVersionId = BuildVersionId();
    if (ohosVersionId == NULL) {
        return EMPTY_STR;
    }
    return ohosVersionId;
}

const char *GetBuildType(void)
{
    return HalGetBuildType();
}

const char *GetBuildUser(void)
{
    return HalGetBuildUser();
}

const char *GetBuildHost(void)
{
    return HalGetBuildHost();
}

const char *GetBuildTime(void)
{
    return HalGetBuildTime();
}

const char *GetBuildRootHash(void)
{
    return OHOS_BUILD_ROOT_HASH;
}

const char *GetOsReleaseType(void)
{
    return OHOS_RELEASE_TYPE;
}

int GetSha256Value(const char *input, char *udid, int udidSize)
{
    if (input == NULL || udid == NULL) {
        return EC_FAILURE;
    }
    char buf[DEV_BUF_LENGTH] = { 0 };
    unsigned char hash[SHA256_DIGEST_LENGTH] = { 0 };
    SHA256_CTX sha256;
    if ((SHA256_Init(&sha256) == 0) || (SHA256_Update(&sha256, input, strlen(input)) == 0) ||
        (SHA256_Final(hash, &sha256) == 0)) {
        return EC_FAILURE;
    }

    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        unsigned char value = hash[i];
        memset_s(buf, DEV_BUF_LENGTH, 0, DEV_BUF_LENGTH);
        sprintf_s(buf, sizeof(buf), "%02X", value);
        if (strcat_s(udid, udidSize, buf) != 0) {
            return EC_FAILURE;
        }
    }
    return EC_SUCCESS;
}

int GetDevUdid(char *udid, int size)
{
    if (size < DEV_UUID_LENGTH) {
        return EC_INVALID;
    }

    const char *manufacture = GetManufacture();
    const char *model = GetHardwareModel();
    const char *sn = GetSerial();
    if (manufacture == NULL || model == NULL || sn == NULL) {
        return EC_INVALID;
    }
    int tmpSize = strlen(manufacture) + strlen(model) + strlen(sn) + 1;
    if (tmpSize <= 0 || tmpSize > DEV_BUF_MAX_LENGTH) {
        return EC_INVALID;
    }
    char *tmp = malloc(tmpSize);
    if (tmp == NULL) {
        return EC_SYSTEM_ERR;
    }

    memset_s(tmp, tmpSize, 0, tmpSize);
    if ((strcat_s(tmp, tmpSize, manufacture) != 0) || (strcat_s(tmp, tmpSize, model) != 0) ||
        (strcat_s(tmp, tmpSize, sn) != 0)) {
        free(tmp);
        return EC_SYSTEM_ERR;
    }

    int ret = GetSha256Value(tmp, udid, size);
    free(tmp);
    return ret;
}

unsigned int FindParameter(const char *name)
{
    if (name == NULL) {
        return EC_INVALID;
    }
    return HalFindParameter(name);
}

unsigned int GetParameterCommitId(unsigned int handle)
{
    return HalGetParameterCommitId(handle);
}

int GetParameterName(unsigned int handle, char *name, unsigned int len)
{
    if (name == NULL) {
        return EC_INVALID;
    }
    int ret = HalGetParameterName(handle, name, len);
    return (ret != 0) ? EC_FAILURE : strlen(name);
}

int GetParameterValue(unsigned int handle, char *value, unsigned int len)
{
    if (value == NULL) {
        return EC_INVALID;
    }
    int ret = HalGetParameterValue(handle, value, len);
    return (ret != 0) ? EC_FAILURE : strlen(value);
}
