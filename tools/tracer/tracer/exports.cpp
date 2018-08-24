// Copyright (c) 2018 Intel Corporation
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

#if defined(_WIN32) || defined(_WIN64)

#include <mfx_dxva2_device.h>
#include <tchar.h>
#include <string.h>

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")



#define SDK_ANALYZER_EXPORT(type) extern "C" __declspec(dllexport) type  __stdcall

#ifdef _WIN64
#define FUNCTION(name) name
#else
#define FUNCTION(name) name
#endif
extern int main(int argc,  char **, bool bUsePrefix);

static const size_t MAX_FIXED_KEY_LENGTH = 32;
static const size_t MAX_FIXED_DIR_LENGTH = 256;


SDK_ANALYZER_EXPORT(void) msdk_analyzer_dll()
{
    // flag function to distinguish from native mediasdk DLL
    // could print a version info here
}

SDK_ANALYZER_EXPORT(void) convert_etl_to_text(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
//
    hwnd; hinst; lpszCmdLine; nCmdShow;
}


typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;



struct KeyData
{
    bool   bCreateNew;
    TCHAR  valueName[MAX_FIXED_KEY_LENGTH];
    DWORD  dwType;
    DWORD  value;
    TCHAR* pStr;//not conforming C99 standard limitation, union can't be used for static initialization
};

void SetValues(HKEY key, KeyData *values, int nValues)
{
    for (int nKey = 0; nKey < nValues; nKey++)
    {
        BYTE *pValue = (BYTE *)&values[nKey].value;
        DWORD nSize = 0;
        if (values[nKey].dwType == REG_SZ)
        {
            pValue = (BYTE *)values[nKey].pStr;
            nSize = sizeof(TCHAR)*(DWORD)(1+strnlen_s(values[nKey].pStr, MAX_PATH));
        }
        else if (values[nKey].dwType == REG_DWORD)
        {
            nSize = sizeof(DWORD);
        }

        bool bKeyMissed = true;

        if (!values[nKey].bCreateNew)
        {
            bKeyMissed = (ERROR_SUCCESS != RegQueryValueEx(key, values[nKey].valueName, 0, NULL, NULL, NULL ));
        }

        if (bKeyMissed)
        {
            RegSetValueEx(key,
                values[nKey].valueName,
                0,
                values[nKey].dwType,
                pValue,
                nSize);
        }
    }
}

DWORD Reg(HKEY key, TCHAR* dll_name, TCHAR* analyzer_key, TCHAR* install_dir, TCHAR* conf_path, bool is_64from32)
{
    MFX::DXVA2Device device;

    //1 required to add software device
    bool bSoftwareDevice = false;

   if (!device.InitDXGI1(0))
   {
       bSoftwareDevice = true;
   }

   mfxU32 deviceID = bSoftwareDevice ? 0 : device.GetDeviceID();
   mfxU32 vendorID = bSoftwareDevice ? 0 : device.GetVendorID();

   TCHAR adapter_number[6];


   HKEY key2;


        //we need to create keys
        if (is_64from32)
        {
            if (ERROR_SUCCESS != RegCreateKeyEx(key, analyzer_key, 0, NULL, 0, KEY_CREATE_SUB_KEY | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY, NULL, &key2, NULL))
            {
                   RegCloseKey(key);
                   return 0;
            }
        }
        else
        {
            if (ERROR_SUCCESS != RegCreateKeyEx(key, analyzer_key, 0, NULL, 0, KEY_CREATE_SUB_KEY | KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &key2, NULL))
            {
                   RegCloseKey(key);
                   return 0;
            }
        }


   size_t length = strnlen_s(install_dir, MAX_FIXED_DIR_LENGTH) + strnlen_s(dll_name, MAX_FIXED_KEY_LENGTH);
   TCHAR *sdk_analyzer_path = new TCHAR[length + 3];
   strcpy_s(sdk_analyzer_path, (length + 3), install_dir);
   strcat_s(sdk_analyzer_path, (length + 3), dll_name);

   KeyData keysToAdd[] =
   {
      {true, _T("APIVersion"), REG_DWORD, 0x100,0},
      {true, _T("Merit"), REG_DWORD, 0x7ffffffe, 0},
      {true, _T("Path"), REG_SZ, 0,sdk_analyzer_path},
   };

   SetValues(key2, keysToAdd, sizeof(keysToAdd)/sizeof(keysToAdd[0]));

   DWORD start = 0;
   length = strnlen_s(conf_path, MAX_PATH);
   TCHAR *sdk_config_path = new TCHAR[length + 2];
   strcpy_s(sdk_config_path, (length + 2), conf_path);

   RegSetValueEx(key2, _T("DeviceId"),0,REG_DWORD,(BYTE*)&deviceID,sizeof(deviceID));
   RegSetValueEx(key2, _T("VendorId"),0,REG_DWORD,(BYTE*)&vendorID,sizeof(vendorID));
   RegSetValueEx(key2, _T("_start"),0,REG_DWORD,(BYTE*)&start,sizeof(DWORD));
   RegSetValueEx(key2, _T("_conf"),0,REG_SZ,(BYTE*)sdk_config_path,(sizeof(TCHAR)*(DWORD)(1+strnlen_s(sdk_config_path, length + 2))));

   RegCloseKey(key2);

   delete[] sdk_analyzer_path;
   delete[] sdk_config_path;

   return 1;
}

SDK_ANALYZER_EXPORT(UINT) FUNCTION(uninstall)();
SDK_ANALYZER_EXPORT(UINT) FUNCTION(install)(TCHAR *installDir,
                                  TCHAR *appdata,
                                  TCHAR *confPath)
{
    appdata;
    FUNCTION(uninstall)();


    TCHAR msdk_analyzers_key[MAX_FIXED_KEY_LENGTH] = _T("tracer");
#ifdef _DEBUG
    TCHAR msdk_analyzers_dll[MAX_FIXED_KEY_LENGTH] = _T("\\mfx-tracer_64_d.dll");
#else
    TCHAR msdk_analyzers_dll[MAX_FIXED_KEY_LENGTH] = _T("\\mfx-tracer_64.dll");
#endif

#ifdef _DEBUG
    TCHAR msdk_analyzers_dll_32[MAX_FIXED_KEY_LENGTH] = _T("\\mfx-tracer_32_d.dll");
#else
    TCHAR msdk_analyzers_dll_32[MAX_FIXED_KEY_LENGTH] = _T("\\mfx-tracer_32.dll");
#endif

   TCHAR *current_msdk_analyzer;
#ifdef _WIN64
        current_msdk_analyzer = msdk_analyzers_dll;
#else
        current_msdk_analyzer = msdk_analyzers_dll_32;
#endif

#ifndef _WIN64
   HKEY key_;
   if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        ,_T("Software\\Intel\\MediaSDK\\Dispatch\\")
        ,0
        ,NULL
        ,0
        ,KEY_CREATE_SUB_KEY | KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_64KEY
        ,NULL
        ,&key_
        ,NULL))
    {
        return 0;
    }

    if (!Reg(key_, msdk_analyzers_dll, msdk_analyzers_key, installDir, installDir, true))
    {
        RegCloseKey(key_);
        return 0;
    }

    RegCloseKey(key_);
#endif

    HKEY key;
    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        ,_T("Software\\Intel\\MediaSDK\\Dispatch\\")
        ,0
        ,NULL
        ,REG_OPTION_NON_VOLATILE
        ,KEY_CREATE_SUB_KEY | KEY_QUERY_VALUE | KEY_SET_VALUE
        ,NULL
        ,&key
        ,NULL))
    {
        return 0;
    }

    if (Reg(key, current_msdk_analyzer, msdk_analyzers_key, installDir, confPath, false) == 0)
    {
        RegCloseKey(key);
        return 0;
    }

    RegCloseKey(key);


    //run_shared_memory_server();*/
    return 1;
}

SDK_ANALYZER_EXPORT(UINT) FUNCTION(uninstall)()
{

#ifndef _WIN64
    HKEY key_;
    if (ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Intel\\MediaSDK\\Dispatch\\"), 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key_))
    {
        return 0;
    }

    SHDeleteKey(key_,_T("tracer"));
    RegCloseKey(key_);
#endif
    HKEY key;
    if (ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Intel\\MediaSDK\\Dispatch\\"), 0, KEY_ALL_ACCESS, &key))
    {
        return 0;
    }

    SHDeleteKey(key, _T("tracer"));
    RegCloseKey(key);
   //stop_shared_memory_server();

    return 1;
}

SDK_ANALYZER_EXPORT(VOID) FUNCTION(start)()
{
    DWORD start_flag = 1;
#ifndef _WIN64
    HKEY key_;
    if (ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Intel\\MediaSDK\\Dispatch\\tracer"), 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key_))
    {
        return;
    }

    RegSetValueEx(key_, _T("_start"),0,REG_DWORD,(BYTE*)&start_flag,sizeof(DWORD));
    RegCloseKey(key_);
#endif
    HKEY key;
    if (ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Intel\\MediaSDK\\Dispatch\\tracer"), 0, KEY_ALL_ACCESS, &key))
    {
        return;
    }

    RegSetValueEx(key, _T("_start"),0,REG_DWORD,(BYTE*)&start_flag,sizeof(DWORD));
    RegCloseKey(key);
}

SDK_ANALYZER_EXPORT(VOID) FUNCTION(stop)()
{
    DWORD start_flag = 0;
#ifndef _WIN64
    HKEY key_;
    if (ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Intel\\MediaSDK\\Dispatch\\tracer"), 0, KEY_ALL_ACCESS | KEY_WOW64_64KEY, &key_))
    {
        return;
    }

    RegSetValueEx(key_, _T("_start"),0,REG_DWORD,(BYTE*)&start_flag,sizeof(DWORD));
    RegCloseKey(key_);
#endif
    HKEY key;
    if (ERROR_SUCCESS!=RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\Intel\\MediaSDK\\Dispatch\\tracer"), 0, KEY_ALL_ACCESS, &key))
    {
        return;
    }

    RegSetValueEx(key, _T("_start"),0,REG_DWORD,(BYTE*)&start_flag,sizeof(DWORD));
    RegCloseKey(key);

}

#endif