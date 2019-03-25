// Copyright (c) 2013-2019 Intel Corporation
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "mfx_plugin_hive.h"
#include "mfx_library_iterator.h"
#include "mfx_dispatcher.h"
#include "mfx_dispatcher_log.h"
#include "mfx_load_dll.h"

#define TRACE_HIVE_ERROR(str, ...) DISPATCHER_LOG_ERROR((("[HIVE]: " str), __VA_ARGS__))
#define TRACE_HIVE_INFO(str, ...) DISPATCHER_LOG_INFO((("[HIVE]: " str), __VA_ARGS__))
#define TRACE_HIVE_WRN(str, ...) DISPATCHER_LOG_WRN((("[HIVE]: " str), __VA_ARGS__))

namespace
{
    const wchar_t rootPluginPath[] = L"Software\\Intel\\MediaSDK\\Plugin";
    const wchar_t rootDispatchPath[] = L"Software\\Intel\\MediaSDK\\Dispatch";
    const wchar_t pluginSubkey[] = L"Plugin";
    const wchar_t TypeKeyName[] = L"Type";
    const wchar_t CodecIDKeyName[] = L"CodecID";
    const wchar_t GUIDKeyName[] = L"GUID";
    const wchar_t PathKeyName[] = L"Path";
    const wchar_t DefaultKeyName[] = L"Default";
    const wchar_t PlgVerKeyName[] = L"PluginVersion";
    const wchar_t APIVerKeyName[] = L"APIVersion";
}

namespace
{
#ifdef _WIN64
    const wchar_t pluginFileName[] = L"FileName64";
#else
    const wchar_t pluginFileName[] = L"FileName32";
#endif // _WIN64

    //do not allow store plugin in different hierarchy
    const wchar_t pluginFileNameRestrictedCharacters[] = L"\\/";
    const wchar_t pluginCfgFileName[] = L"plugin.cfg";
    const wchar_t pluginSearchPattern[] = L"????????????????????????????????";
    const mfxU32 pluginCfgFileNameLen = 10;
    const mfxU32 pluginDirNameLen = 32;
    const mfxU32 defaultPluginNameLen = 25;
    const mfxU32 charsPermfxU8 = 2;
    const mfxU32 slashLen = 1;
    enum
    {
        MAX_PLUGIN_FILE_LINE = 4096
    };

    #define alignStr() "%-14S"
}

MFX::MFXPluginsInHive::MFXPluginsInHive(int mfxStorageID, const wchar_t *msdkLibSubKey, mfxVersion currentAPIVersion)
    : MFXPluginStorageBase(currentAPIVersion)
{
    HKEY rootHKey;
    bool bRes;
    WinRegKey regKey;

    if (MFX_LOCAL_MACHINE_KEY != mfxStorageID && MFX_CURRENT_USER_KEY != mfxStorageID)
        return;

    // open required registry key
    rootHKey = (MFX_LOCAL_MACHINE_KEY == mfxStorageID) ? (HKEY_LOCAL_MACHINE) : (HKEY_CURRENT_USER);
    if (msdkLibSubKey) {
        //dispatch/subkey/plugin
        bRes = regKey.Open(rootHKey, rootDispatchPath, KEY_READ);
        if (bRes)
        {
            bRes = regKey.Open(regKey, msdkLibSubKey, KEY_READ);
        }
        if (bRes)
        {
            bRes = regKey.Open(regKey, pluginSubkey, KEY_READ);
        }
    }
    else
    {
        bRes = regKey.Open(rootHKey, rootPluginPath, KEY_READ);
    }

    if (false == bRes) {
        return;
    }
    DWORD index = 0;
    if (!regKey.QueryInfo(&index)) {
        return;
    }
    try
    {
        resize(index);
    }
    catch (...) {
        TRACE_HIVE_ERROR("new PluginDescriptionRecord[%d] threw an exception: \n", index);
        return;
    }

    for(index = 0; ; index++)
    {
        wchar_t   subKeyName[MFX_MAX_REGISTRY_KEY_NAME];
        DWORD     subKeyNameSize = sizeof(subKeyName) / sizeof(subKeyName[0]);
        WinRegKey subKey;

        // query next value name
        bool enumRes = regKey.EnumKey(index, subKeyName, &subKeyNameSize);
        if (!enumRes) {
            break;
        }

        // open the sub key
        bRes = subKey.Open(regKey, subKeyName, KEY_READ);
        if (!bRes) {
            continue;
        }

        if (msdkLibSubKey)
        {
            TRACE_HIVE_INFO("Found Plugin: %s\\%S\\%S\\%S\\%S\n", (MFX_LOCAL_MACHINE_KEY == mfxStorageID) ? ("HKEY_LOCAL_MACHINE") : ("HKEY_CURRENT_USER"),
                rootDispatchPath, msdkLibSubKey, pluginSubkey, subKeyName);
        }
        else
        {
            TRACE_HIVE_INFO("Found Plugin: %s\\%S\\%S\n", (MFX_LOCAL_MACHINE_KEY == mfxStorageID) ? ("HKEY_LOCAL_MACHINE") : ("HKEY_CURRENT_USER"),
                rootPluginPath, subKeyName);
        }

        PluginDescriptionRecord descriptionRecord;

        if (!QueryKey(subKey, TypeKeyName, descriptionRecord.Type))
        {
            continue;
        }
        TRACE_HIVE_INFO(alignStr()" : %d\n", TypeKeyName, descriptionRecord.Type);

        if (QueryKey(subKey, CodecIDKeyName, descriptionRecord.CodecId))
        {
            TRACE_HIVE_INFO(alignStr()" : " MFXFOURCCTYPE()" \n", CodecIDKeyName, MFXU32TOFOURCC(descriptionRecord.CodecId));
        }
        else
        {
                TRACE_HIVE_INFO(alignStr()" : \n", CodecIDKeyName, "NOT REGISTERED");
        }

        if (!QueryKey(subKey, GUIDKeyName, descriptionRecord.PluginUID))
        {
            continue;
        }
        TRACE_HIVE_INFO(alignStr()" : " MFXGUIDTYPE()"\n", GUIDKeyName, MFXGUIDTOHEX(&descriptionRecord.PluginUID));

        mfxU32 nSize = sizeof(descriptionRecord.sPath)/sizeof(*descriptionRecord.sPath);
        if (!subKey.Query(PathKeyName, descriptionRecord.sPath, nSize))
        {
            TRACE_HIVE_WRN("no value for : %S\n", PathKeyName);
            continue;
        }
        TRACE_HIVE_INFO(alignStr()" : %S\n", PathKeyName, descriptionRecord.sPath);

        if (!QueryKey(subKey, DefaultKeyName, descriptionRecord.Default))
        {
            continue;
        }
        TRACE_HIVE_INFO(alignStr()" : %s\n", DefaultKeyName, descriptionRecord.Default ? "true" : "false");

        mfxU32 version = 0;
        if (!QueryKey(subKey, PlgVerKeyName, version))
        {
            continue;
        }
        descriptionRecord.PluginVersion = static_cast<mfxU16>(version);
        if (0 == version)
        {
            TRACE_HIVE_ERROR(alignStr()" : %d, which is invalid\n", PlgVerKeyName, descriptionRecord.PluginVersion);
            continue;
        }
        else
        {
            TRACE_HIVE_INFO(alignStr()" : %d\n", PlgVerKeyName, descriptionRecord.PluginVersion);
        }

        mfxU32 APIVersion = 0;
        if (!QueryKey(subKey, APIVerKeyName, APIVersion))
        {
            continue;
        }
        ConvertAPIVersion(APIVersion, descriptionRecord);
        TRACE_HIVE_INFO(alignStr()" : %d.%d \n", APIVerKeyName, descriptionRecord.APIVersion.Major, descriptionRecord.APIVersion.Minor);

        try
        {
            operator[](index) = descriptionRecord;
        }
        catch (...) {
            TRACE_HIVE_ERROR("operator[](%d) = descriptionRecord; - threw exception \n", index);
        }
    }
}

MFX::MFXPluginsInFS::MFXPluginsInFS( mfxVersion currentAPIVersion )
    : MFXPluginStorageBase(currentAPIVersion)
    , mIsVersionParsed()
    , mIsAPIVersionParsed()
{
    WIN32_FIND_DATAW find_data;
    wchar_t currentModuleName[MAX_PLUGIN_PATH];

    GetModuleFileNameW(NULL, currentModuleName, MAX_PLUGIN_PATH);
    if (GetLastError() != 0)
    {
        TRACE_HIVE_ERROR("GetModuleFileName() reported an error: %d\n", GetLastError());
        return;
    }
    wchar_t *lastSlashPos = wcsrchr(currentModuleName, L'\\');
    if (!lastSlashPos) {
        lastSlashPos = currentModuleName;
    }
    mfxU32 executableDirLen = (mfxU32)(lastSlashPos - currentModuleName) + slashLen;
    if (executableDirLen + pluginDirNameLen + pluginCfgFileNameLen >= MAX_PLUGIN_PATH)
    {
        TRACE_HIVE_ERROR("MAX_PLUGIN_PATH which is %d, not enough to locate plugin path\n", MAX_PLUGIN_PATH);
        return;
    }
    wcscpy_s(lastSlashPos + slashLen
        , MAX_PLUGIN_PATH - executableDirLen, pluginSearchPattern);

    HANDLE fileFirst = FindFirstFileW(currentModuleName, &find_data);
    if (INVALID_HANDLE_VALUE == fileFirst)
    {
        TRACE_HIVE_ERROR("FindFirstFileW() unable to locate any plugins folders\n", 0);
        return;
    }
    do
    {
        if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            continue;
        }
        if (pluginDirNameLen != wcslen(find_data.cFileName))
        {
            continue;
        }
        //converting dirname into guid
        PluginDescriptionRecord descriptionRecord;
        descriptionRecord.APIVersion = currentAPIVersion;
        descriptionRecord.onlyVersionRegistered = true;

        mfxU32 i = 0;
        for(i = 0; i != pluginDirNameLen / charsPermfxU8; i++)
        {
            mfxU32 hexNum = 0;
            if (1 != swscanf_s(find_data.cFileName + charsPermfxU8 * i, L"%2x", &hexNum))
            {
                // it is ok to have non-plugin subdirs with length 32
                //TRACE_HIVE_INFO("folder name \"%S\" is not a valid GUID string\n", find_data.cFileName);
                break;
            }
            if (hexNum == 0 && find_data.cFileName + charsPermfxU8 * i != wcsstr(find_data.cFileName + 2*i, L"00"))
            {
                // it is ok to have non-plugin subdirs with length 32
                //TRACE_HIVE_INFO("folder name \"%S\" is not a valid GUID string\n", find_data.cFileName);
                break;
            }
            descriptionRecord.PluginUID.Data[i] = (mfxU8)hexNum;
        }
        if (i != pluginDirNameLen / charsPermfxU8) {
            continue;
        }

        wcscpy_s(currentModuleName + executableDirLen
            , MAX_PLUGIN_PATH - executableDirLen, find_data.cFileName);

        wcscpy_s(currentModuleName + executableDirLen + pluginDirNameLen
            , MAX_PLUGIN_PATH - executableDirLen - pluginDirNameLen, L"\\");

        //this is path to plugin directory
        wcscpy_s(descriptionRecord.sPath
            , sizeof(descriptionRecord.sPath) / sizeof(*descriptionRecord.sPath), currentModuleName);

        wcscpy_s(currentModuleName + executableDirLen + pluginDirNameLen + slashLen
            , MAX_PLUGIN_PATH - executableDirLen - pluginDirNameLen - slashLen, pluginCfgFileName);

        FILE *pluginCfgFile = 0;
        _wfopen_s(&pluginCfgFile, currentModuleName, L"r");
        if (!pluginCfgFile)
        {
            TRACE_HIVE_INFO("in directory \"%S\" no mandatory \"%S\"\n"
                , find_data.cFileName, pluginCfgFileName);
            continue;
        }

        if (ParseFile(pluginCfgFile, descriptionRecord))
        {
            try
            {
                push_back(descriptionRecord);
            }
            catch (...) {
                TRACE_HIVE_ERROR("mRecords.push_back(descriptionRecord); - threw exception \n", 0);
            }
        }

        fclose(pluginCfgFile);
    }while (FindNextFileW(fileFirst, &find_data));
    FindClose(fileFirst);
}

bool MFX::MFXPluginsInFS::ParseFile(FILE * f, PluginDescriptionRecord & descriptionRecord)
{
    wchar_t line[MAX_PLUGIN_FILE_LINE];

    while(NULL != fgetws(line, sizeof(line) / sizeof(*line), f))
    {
        wchar_t *delimiter = wcschr(line, L'=');
        if (0 == delimiter)
        {
            TRACE_HIVE_INFO("plugin.cfg contains line \"%S\" which is not in K=V format, skipping \n", line);
            continue;
        }
        *delimiter = 0;
        if (!ParseKVPair(line, delimiter + 1, descriptionRecord))
        {
            return false;
        }
    }

    if (!mIsVersionParsed)
    {
        TRACE_HIVE_ERROR("%S : Mandatory  key %S not found\n", pluginCfgFileName, PlgVerKeyName);
        return false;
    }

    if (!mIsAPIVersionParsed)
    {
        TRACE_HIVE_ERROR("%S : Mandatory  key %S not found\n", pluginCfgFileName, APIVerKeyName);
        return false;
    }

    if (!wcslen(descriptionRecord.sPath))
    {
        TRACE_HIVE_ERROR("%S : Mandatory  key %S not found\n", pluginCfgFileName, pluginFileName);
        return false;
    }

    return true;
}

bool MFX::MFXPluginsInFS::ParseKVPair( wchar_t * key, wchar_t* value, PluginDescriptionRecord & descriptionRecord)
{
    if (0 != wcsstr(key, PlgVerKeyName))
    {
        mfxU32 version ;
        if (0 == swscanf_s(value, L"%d", &version))
        {
            return false;
        }
        descriptionRecord.PluginVersion = (mfxU16)version;

        if (0 == descriptionRecord.PluginVersion)
        {
            TRACE_HIVE_ERROR("%S: %S = %d,  which is invalid\n", pluginCfgFileName, PlgVerKeyName, descriptionRecord.PluginVersion);
            return false;
        }

        TRACE_HIVE_INFO("%S: %S = %d \n", pluginCfgFileName, PlgVerKeyName, descriptionRecord.PluginVersion);
        mIsVersionParsed = true;
        return true;
    }

    if (0 != wcsstr(key, APIVerKeyName))
    {
        mfxU32 APIversion;
        if (0 == swscanf_s(value, L"%d", &APIversion))
        {
            return false;
        }

        ConvertAPIVersion(APIversion, descriptionRecord);
        TRACE_HIVE_INFO("%S: %S = %d.%d \n", pluginCfgFileName, APIVerKeyName, descriptionRecord.APIVersion.Major, descriptionRecord.APIVersion.Minor);

        mIsAPIVersionParsed = true;
        return true;
    }


    if (0!=wcsstr(key, pluginFileName))
    {
        wchar_t *startQuoteMark = wcschr(value, L'\"');
        if (!startQuoteMark)
        {
            TRACE_HIVE_ERROR("plugin filename not in quotes : %S\n", value);
            return false;
        }
        wchar_t *endQuoteMark = wcschr(startQuoteMark + 1, L'\"');

        if (!endQuoteMark)
        {
            TRACE_HIVE_ERROR("plugin filename not in quotes : %S\n", value);
            return false;
        }
        *endQuoteMark = 0;

        mfxU32 currentPathLen = (mfxU32)wcslen(descriptionRecord.sPath);
        if (currentPathLen + wcslen(startQuoteMark + 1) > sizeof(descriptionRecord.sPath) / sizeof(*descriptionRecord.sPath))
        {
            TRACE_HIVE_ERROR("buffer of MAX_PLUGIN_PATH characters which is %d, not enough lo store plugin path: %S%S\n"
                , MAX_PLUGIN_PATH, descriptionRecord.sPath, startQuoteMark + 1);
            return false;
        }

        size_t restrictedCharIdx = wcscspn(startQuoteMark + 1, pluginFileNameRestrictedCharacters);
        if (restrictedCharIdx != wcslen(startQuoteMark + 1))
        {
            TRACE_HIVE_ERROR("plugin filename :%S, contains one of restricted characters: %S\n", startQuoteMark + 1, pluginFileNameRestrictedCharacters);
            return false;
        }

        wcscpy_s(descriptionRecord.sPath + currentPathLen
            , sizeof(descriptionRecord.sPath) / sizeof(*descriptionRecord.sPath) - currentPathLen, startQuoteMark + 1);

        TRACE_HIVE_INFO("%S: %S = \"%S\" \n", pluginCfgFileName, pluginFileName, startQuoteMark + 1);

        return true;
    }


    return true;
}

MFX::MFXDefaultPlugins::MFXDefaultPlugins(mfxVersion currentAPIVersion, MFX_DISP_HANDLE * hdl, int implType)
    : MFXPluginStorageBase(currentAPIVersion)
{
    wchar_t libModuleName[MAX_PLUGIN_PATH];

    GetModuleFileNameW((HMODULE)hdl->hModule, libModuleName, MAX_PLUGIN_PATH);
    if (GetLastError() != 0)
    {
        TRACE_HIVE_ERROR("GetModuleFileName() reported an error: %d\n", GetLastError());
        return;
    }
    wchar_t *lastSlashPos = wcsrchr(libModuleName, L'\\');
    if (!lastSlashPos) {
        lastSlashPos = libModuleName;
    }
    mfxU32 executableDirLen = (mfxU32)(lastSlashPos - libModuleName) + slashLen;
    if (executableDirLen + defaultPluginNameLen >= MAX_PLUGIN_PATH)
    {
        TRACE_HIVE_ERROR("MAX_PLUGIN_PATH which is %d, not enough to locate default plugin path\n", MAX_PLUGIN_PATH);
        return;
    }

    mfx_get_default_plugin_name(lastSlashPos + slashLen, MAX_PLUGIN_PATH - executableDirLen, (eMfxImplType)implType);

    if (-1 != GetFileAttributesW(libModuleName))
    {
        // add single default plugin description
        PluginDescriptionRecord descriptionRecord;
        descriptionRecord.APIVersion = currentAPIVersion;
        descriptionRecord.Default = true;

        wcscpy_s(descriptionRecord.sPath
            , sizeof(descriptionRecord.sPath) / sizeof(*descriptionRecord.sPath), libModuleName);

        push_back(descriptionRecord);
    }
    else
    {
        TRACE_HIVE_INFO("GetFileAttributesW() unable to locate default plugin dll named %S\n", libModuleName);
    }
}


