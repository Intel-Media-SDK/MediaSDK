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

#include <mfx_scheduler_core.h>
#include <mfx_scheduler_core_task.h>

#include <mfx_trace.h>
#include <stdio.h>
#include <vm_time.h>


mfxStatus mfxSchedulerCore::StartWakeUpThread(void)
{
    // stop the thread before creating it again
    // don't try to check thread status, it might lead to interesting effects.
    if (m_hwWakeUpThread.handle)
        StopWakeUpThread();

    m_timer_hw_event = MFX_THREAD_TIME_TO_WAIT;



    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::StartWakeUpThread(void)

mfxStatus mfxSchedulerCore::StopWakeUpThread(void)
{

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::StopWakeUpThread(void)

uint32_t mfxSchedulerCore::scheduler_thread_proc(void *pParam)
{
    MFX_SCHEDULER_THREAD_CONTEXT *pContext = (MFX_SCHEDULER_THREAD_CONTEXT *) pParam;

    {
        char thread_name[30] = {};
        snprintf(thread_name, sizeof(thread_name)-1, "ThreadName=MSDK#%d", pContext->threadNum);
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, thread_name);
    }

    pContext->pSchedulerCore->ThreadProc(pContext);
    return (0x0cced00 + pContext->threadNum);
}

void mfxSchedulerCore::ThreadProc(MFX_SCHEDULER_THREAD_CONTEXT *pContext)
{
    mfxTaskHandle previousTaskHandle = {};
    const uint32_t threadNum = pContext->threadNum;

    // main working cycle for threads
    while (false == m_bQuit)
    {
        MFX_CALL_INFO call = {};
        mfxStatus mfxRes;

        mfxRes = GetTask(call, previousTaskHandle, threadNum);
        if (MFX_ERR_NONE == mfxRes)
        {
            mfxU64 start, stop;

            // perform asynchronous operation
            try
            {
                const char *pRoutineName = call.pTask->entryPoint.pRoutineName;
                if (!pRoutineName) pRoutineName = "MFX Async Task";
                MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, pRoutineName);
                MFX_LTRACE_1(MFX_TRACE_LEVEL_SCHED, "^Child^of", "%d", call.pTask->nParentId);

                // mark beginning of working period
                start = GetHighPerformanceCounter();

                // NOTE: it is legacy task call,
                // it should be eliminated soon
                if (call.pTask->bObsoleteTask)
                {
                    call.res = call.pTask->entryPoint.pRoutine(call.pTask->entryPoint.pState,
                                                               (void *) &call.pTask->obsolete_params,
                                                               call.threadNum,
                                                               call.callNum);
                }
                // the only legal task calling process.
                // Should survive only this one :-).
                else
                {
                    call.res = call.pTask->entryPoint.pRoutine(call.pTask->entryPoint.pState,
                                                               call.pTask->entryPoint.pParam,
                                                               call.threadNum,
                                                               call.callNum);
                }

                // mark end of working period
                stop = GetHighPerformanceCounter();

                // update thread statistic
                call.timeSpend = (stop - start);
                pContext->workTime += call.timeSpend;
                // save the previous task's handle
                previousTaskHandle = call.taskHandle;
                MFX_LTRACE_1(MFX_TRACE_LEVEL_SCHED, "mfxRes = ", "%d", call.res);
            }
            catch(...)
            {
                call.res = MFX_ERR_UNKNOWN;
            }

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
            Wait(threadNum);

            // mark end of sleep period
            stop = GetHighPerformanceCounter();

            // update thread statistic
            pContext->sleepTime += (stop - start);

        }
    }
}

uint32_t mfxSchedulerCore::scheduler_wakeup_thread_proc(void *pParam)
{
    mfxSchedulerCore * const pSchedulerCore = (mfxSchedulerCore *) pParam;

    {
        const char thread_name[30] = "ThreadName=MSDKHWL#0";
        MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, thread_name);
    }

    pSchedulerCore->WakeupThreadProc();
    return 0x0ccedff;
}

void mfxSchedulerCore::WakeupThreadProc()
{
    // main working cycle for threads
    while (false == m_bQuitWakeUpThread)
    {
        vm_status vmRes;

        vmRes = vm_event_timed_wait(&m_hwTaskDone, m_timer_hw_event);

        // HW event is signaled. Reset all HW waiting tasks.
        if (VM_OK == vmRes||
            VM_TIMEOUT == vmRes)
        {
            vmRes = vm_event_reset(&m_hwTaskDone);

            //MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, "HW Event");
            IncrementHWEventCounter();
            WakeUpThreads((mfxU32) MFX_INVALID_THREAD_ID, MFX_SCHEDULER_HW_BUFFER_COMPLETED);
        }
    }
}
