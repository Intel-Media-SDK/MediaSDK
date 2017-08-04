// Copyright (c) 2017 Intel Corporation
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

#ifndef __MFX_MSG_H
#define __MFX_MSG_H

#include <umc_defs.h>

#include <stdio.h>
#include <windows.h>

extern "C"
unsigned long long freq;

inline
void Msg(uint32_t threadNum, const char *pMsg, unsigned long long time, unsigned long long lasting)
{
    char cStr[256];
    int32_t timeSec, timeMSec, us;

    timeSec = (uint32_t) ((time / freq) % 60);
    timeMSec = (uint32_t) (((time * 1000) / freq) % 1000);
    us = (uint32_t) ((lasting * 1000000) / freq);
    sprintf_s(cStr, sizeof(cStr),
              "[% 4u] %3u.%03u % 30s % 6u us\n",
              threadNum, timeSec, timeMSec, pMsg, us);
    OutputDebugStringA(cStr);

} // void Msg(uint32_t threadNum, const char *pMsg, unsigned long long time)

#endif // __MFX_MSG_H
