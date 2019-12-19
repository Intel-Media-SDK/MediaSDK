/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
\**********************************************************************************/

#ifndef THREAD11_H_
#define THREAD11_H_

#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>


class CThread11
{
public:
    const int WAIT_FOREVER = -1;
    CThread11(int timeoutMS=10);
    ~CThread11();

    void WakeUp();

    virtual void Run();
    virtual bool RunOnce() { return true; }
    virtual bool OnStart() { return true; }
    virtual void  OnClose() {}
    bool Start();
    bool Stop();
    bool IsRunning()
    {
        return isRunning;
    }

protected:
    std::chrono::milliseconds chronoTimeoutMS;
    const std::chrono::milliseconds CHRONO_FOREVER;
    bool isSignalled = false;

    std::atomic<bool> shouldStop;
    std::atomic<bool> isRunning;

    std::thread thread;
    std::mutex mtxSleep;
    std::condition_variable cvWakeUp;
};

#endif
