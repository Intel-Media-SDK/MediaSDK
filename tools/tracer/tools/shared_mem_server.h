// Copyright (c) 2020 Intel Corporation
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

#include <string>
#include <sstream>
#include <vector>
#include <tchar.h>
#include "../loggers/log.h"

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

HANDLE g_RegScannerThread = NULL;
bool g_bQuit = false;
CRITICAL_SECTION g_threadAcc = {};

void stop_shared_memory_server()
{
    g_bQuit = true;
    if (g_RegScannerThread)
    {
        //EnterCriticalSection(&g_threadAcc);
        //WaitForSingleObject(g_RegScannerThread, 100);
        TerminateThread(g_RegScannerThread, 0);
        CloseHandle(g_RegScannerThread);
        g_RegScannerThread = NULL;
        //LeaveCriticalSection(&g_threadAcc);
        //DeleteCriticalSection(&g_threadAcc);
    }
}

void registry_scaner(LPVOID)
{
    DWORD start_flag = 0;
    DWORD size = sizeof(DWORD);
    HKEY key;
    for (;;)
    {
        
        RegGetValue(HKEY_LOCAL_MACHINE, ("Software\\Intel\\MediaSDK\\Dispatch\\tracer"), ("_start"), REG_DWORD, 0, (BYTE*)&start_flag, &size); 
        if ((start_flag == 0) && (Log::useGUI))
        {
            Log::clear();
        }
        if (ERROR_SUCCESS != RegOpenKey(HKEY_LOCAL_MACHINE,("Software\\Intel\\MediaSDK\\Dispatch\\tracer"),&key))
        {
            stop_shared_memory_server();
        }
        Sleep(100);
    }
}

void run_shared_memory_server()
{
     g_bQuit = false;
     if (!g_RegScannerThread)
     {
        //InitializeCriticalSection(&g_threadAcc);
        g_RegScannerThread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)registry_scaner,NULL,0,NULL);
        if (!g_RegScannerThread)
        {
            //DeleteCriticalSection(&g_threadAcc);
        }
     }
}
#endif
