/******************************************************************************\
Copyright (c) 2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "fei_worker.h"

void Worker::Start()
{
    m_working_thread = std::thread(std::bind(&Worker::Process, this));
}

void Worker::Stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_data.push(ThreadTask()); // nullptr command is a stop thread command
        m_condition.notify_one();
    }
    {
        // mutexed code section not to have exceptions in join
        // if already joined in another thread or not started
        std::lock_guard<std::mutex> lock(m_shutdown_mutex);
        if(m_working_thread.joinable())
        {
            m_working_thread.join();
        }
    }
}

void Worker::Push(ThreadTask task)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.push(task);
    m_condition.notify_one();
}

void Worker::WaitingPop(ThreadTask* command)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return !m_data.empty(); });
    *command = m_data.front();
    m_data.pop();
}

void Worker::Process()
{
    ThreadTask cmd;
    for(;;)
    {
        WaitingPop(&cmd);
        if(cmd == nullptr)
        {
            break;
        }
        else
        {
            cmd();
        }
    }
}
