// Copyright (c) 2018-2020 Intel Corporation
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

#include <mfx_scheduler_core.h>
#include <mfx_scheduler_core_task.h>

#include <mfx_trace.h>
#include <stdio.h>
#include <vm_time.h>


mfxStatus mfxSchedulerCore::StartWakeUpThread(void)
{
    // stop the thread before creating it again
    // don't try to check thread status, it might lead to interesting effects.
    if (m_hwWakeUpThread.joinable())
        StopWakeUpThread();

    m_timer_hw_event = MFX_THREAD_TIME_TO_WAIT;



    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::StartWakeUpThread(void)

mfxStatus mfxSchedulerCore::StopWakeUpThread(void)
{

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::StopWakeUpThread(void)

void mfxSchedulerCore::ThreadProc(MFX_SCHEDULER_THREAD_CONTEXT *pContext)
{
    std::unique_lock<std::mutex> guard(m_guard);
    mfxTaskHandle previousTaskHandle = {};
    const uint32_t threadNum = pContext->threadNum;

    {
        char thread_name[30] = {};
        snprintf(thread_name, sizeof(thread_name)-1, "ThreadName=MSDK#%d", threadNum);
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, thread_name);
    }

    // main working cycle for threads
    while (false == m_bQuit)
    {
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_HOTSPOTS, "thread_proc");

        MFX_CALL_INFO call = {};
        mfxStatus mfxRes;

        pContext->state = MFX_SCHEDULER_THREAD_CONTEXT::Waiting;

        mfxRes = GetTask(call, previousTaskHandle, threadNum);
        if (MFX_ERR_NONE == mfxRes)
        {
            pContext->state = MFX_SCHEDULER_THREAD_CONTEXT::Running;
            guard.unlock();
            {
                // perform asynchronous operation
                call_pRoutine(call);
            }
            guard.lock();

            pContext->workTime += call.timeSpend;
            // save the previous task's handle
            previousTaskHandle = call.taskHandle;

            // mark the task completed,
            // set the sync point into the high state if any.
            MarkTaskCompleted(&call, threadNum);
            //timer1.Stop(0);
        }
        else
        {
            mfxU64 start, stop;


            // mark beginning of sleep period
            start = GetHighPerformanceCounter();

            // there is no any task.
            // sleep for a while until the event is signaled.
            Wait(threadNum, guard);

            // mark end of sleep period
            stop = GetHighPerformanceCounter();

            // update thread statistic
            pContext->sleepTime += (stop - start);

        }
    }
}

void mfxSchedulerCore::WakeupThreadProc()
{
    {
        const char thread_name[30] = "ThreadName=MSDKHWL#0";
        (void)thread_name;
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, thread_name);
    }

    // main working cycle for threads
    while (false == m_bQuitWakeUpThread)
    {
        IncrementHWEventCounter();
        {
            std::lock_guard<std::mutex> guard(m_guard);
            WakeUpThreads(1,1);
        }
    }
}
