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

#include "pch.h"
#include "Thread11.h"

using namespace std;

CThread11::CThread11(int timeoutMS)
    : chronoTimeoutMS(timeoutMS), WAIT_FOREVER(-1), CHRONO_FOREVER(-1)
{
    shouldStop = false;
    isRunning = false;
}


CThread11::~CThread11()
{
    Stop();
}

void CThread11::WakeUp()
{
    unique_lock<mutex> lock(mtxSleep);
    isSignalled = true;
    cvWakeUp.notify_all();
}

void CThread11::Run()
{
    isRunning = true;
	isSignalled = false;

    if (OnStart())
    {
        while (!shouldStop)
        {
            if (!RunOnce())
            {
                break;
            }

            unique_lock<mutex> lock(mtxSleep);
            
            if (!isSignalled)
            {
                if (chronoTimeoutMS == CHRONO_FOREVER)
                {
                    cvWakeUp.wait(lock);
                }
                else
                {
                    cvWakeUp.wait_for(lock, chronoTimeoutMS);
                }
            }
            isSignalled = false;
        }
    }
    OnClose();

    // Detaching is required in case of "natural" closing, otherwise next call to start won't succeed
	if (!shouldStop)
	{
		thread.detach();
	}

    isRunning = false;
}

bool CThread11::Start()
{
    if (!isRunning)
    {
        try
        {
            shouldStop = false;
            thread = std::thread(&CThread11::Run, this);
			isRunning = true;
        }
        catch (...)
        {
            return false;
        }
    }
    return true;
}

bool CThread11::Stop()
{
    if (isRunning && thread.native_handle())
    {
        // Sending stop signal
        shouldStop = true;

        // Wake up, just in case if thread is waiting for cv
        WakeUp();

        // Waiting for thread to be killed
		if (isRunning && thread.joinable())
		{
			thread.join();
		}
        return true;
    }
    return false;
}