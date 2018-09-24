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
        string timestamp = "";
#if defined(_WIN32) || defined(_WIN64)
        SYSTEMTIME st;
        GetLocalTime(&st);
        timestamp = ToString(st.wYear) + "-" + ToString(st.wMonth) + "-" + ToString(st.wDay) + " " +
            ToString(st.wHour) + ":" + ToString(st.wMinute) + ":" + ToString(st.wSecond) + ":" + ToString(st.wMilliseconds);

#else
        time_t t = time(0);
        struct tm * now = localtime(&t);
        timestamp = ToString((now->tm_year + 1900)) + '-' + ToString(now->tm_mon + 1) + '-' + ToString(now->tm_mday) + " " +
                       ToString(now->tm_hour) + ":" + ToString(now->tm_min) + ":" + ToString(now->tm_sec);
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
