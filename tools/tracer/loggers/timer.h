/* ****************************************************************************** *\

Copyright (C) 2012-2020 Intel Corporation.  All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
- Neither the name of Intel Corporation nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL INTEL CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

File Name: timer.h

\* ****************************************************************************** */

#ifndef TIMER_H__
#define TIMER_H__

#include <string>
#include "../dumps/dump.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <sys/time.h>
#include <ctime>
#endif

using namespace std;

class Timer
{
protected:
#if defined(_WIN32) || defined(_WIN64)
    LARGE_INTEGER start;
#else
    struct timeval start;
#endif

public:
    Timer()
    {
        Restart();
    }

    static string GetTimeStamp(){
        string timestamp = "<time unknown>";
#if defined(_WIN32) || defined(_WIN64)
        SYSTEMTIME st;
        GetLocalTime(&st);
        timestamp = ToString(st.wYear) + "-" + ToString(st.wMonth) + "-" + ToString(st.wDay) + " " +
            ToString(st.wHour) + ":" + ToString(st.wMinute) + ":" + ToString(st.wSecond) + ":" + ToString(st.wMilliseconds);

#else
        time_t t = time(0);
        struct tm * now = localtime(&t);
        if (now)
        {
            timestamp = ToString((now->tm_year + 1900)) + '-' + ToString(now->tm_mon + 1) + '-' + ToString(now->tm_mday) + " " +
                           ToString(now->tm_hour) + ":" + ToString(now->tm_min) + ":" + ToString(now->tm_sec);
        }
#endif

        return timestamp;
    };

    double GetTime()
    {
#if defined(_WIN32) || defined(_WIN64)
        LARGE_INTEGER tick, ticksPerSecond;
        QueryPerformanceFrequency(&ticksPerSecond);
        QueryPerformanceCounter(&tick);
        return (((double)tick.QuadPart - (double)start.QuadPart) / (double)ticksPerSecond.QuadPart) * 1000;
#else
        struct timeval now;
        gettimeofday(&now, NULL);
        return ((now.tv_sec - start.tv_sec) + (now.tv_usec - start.tv_usec)/1000000.0) * 1000;
#endif
    }

    void Restart()
    {
#if defined(_WIN32) || defined(_WIN64)
        QueryPerformanceCounter(&start);
#else
        gettimeofday(&start, NULL);
#endif
    }
};
#endif //TIMER_H__
