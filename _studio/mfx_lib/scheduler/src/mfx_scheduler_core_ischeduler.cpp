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

#include <mfx_scheduler_core.h>

#include <mfx_scheduler_core_task.h>
#include <mfx_scheduler_core_handle.h>

#include <vm_time.h>
#include <vm_sys_info.h>
#include <umc_automatic_mutex.h>
#include <mfx_trace.h>
#include <cassert>





enum
{
    MFX_TIME_INFINITE           = 0x7fffffff,
    MFX_TIME_TO_WAIT            = 5
};

mfxStatus mfxSchedulerCore::Initialize(const MFX_SCHEDULER_PARAM *pParam)
{
    MFX_SCHEDULER_PARAM2 param2;
    memset(&param2, 0, sizeof(param2));
    if (pParam) {
        MFX_SCHEDULER_PARAM* casted_param2 = &param2;
        *casted_param2 = *pParam;
    }
    if (!param2.numberOfThreads) {
        // that's just in case: core, which calls scheduler, is doing
        // exactly the same what we are doing below
        param2.numberOfThreads = vm_sys_info_get_cpu_num();
    }
    return Initialize2(&param2);
}

mfxStatus mfxSchedulerCore::Initialize2(const MFX_SCHEDULER_PARAM2 *pParam)
{
    UMC::Status umcRes;
    mfxU32 i;
    mfxU32 iRes;

    // release the object before initialization
    Close();


    // save the parameters
    if (pParam)
    {
        m_param = *pParam;
    }


    if (VM_OK != vm_mutex_init(&m_guard)) {
        return MFX_ERR_MEMORY_ALLOC;
    }

    // clean up the task look up table
    umcRes = m_ppTaskLookUpTable.Alloc(MFX_MAX_NUMBER_TASK);
    if (UMC::UMC_OK != umcRes)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    // allocate the dependency table
    m_pDependencyTable.Alloc(MFX_MAX_NUMBER_TASK * 2);
    if (UMC::UMC_OK != umcRes)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    // allocate the thread assignment object table.
    // its size should be equal to the number of task,
    // larger table is not required.
    m_occupancyTable.Alloc(MFX_MAX_NUMBER_TASK);
    if (UMC::UMC_OK != umcRes)
    {
        return MFX_ERR_MEMORY_ALLOC;
    }

    if (MFX_SINGLE_THREAD != m_param.flags)
    {
        if (m_param.numberOfThreads && m_param.params.NumThread) {
            // use user-overwritten number of threads
            m_param.numberOfThreads = m_param.params.NumThread;
        }
        if (!m_param.numberOfThreads) {
            return MFX_ERR_UNSUPPORTED;
        }
        if (m_param.numberOfThreads == 1) {
            // we need at least 2 threads to avoid dead locks
            return MFX_ERR_UNSUPPORTED;
        }


        try
        {
            // allocate 'free task' event
            umcRes = m_freeTasks.Init(MFX_MAX_NUMBER_TASK, MFX_MAX_NUMBER_TASK);
            if (UMC::UMC_OK != umcRes)
            {
                return MFX_ERR_UNKNOWN;
            }

            // allocate thread contexts
            m_pThreadCtx = new MFX_SCHEDULER_THREAD_CONTEXT[m_param.numberOfThreads];

            // start threads
            for (i = 0; i < m_param.numberOfThreads; i += 1)
            {
                // prepare context
                m_pThreadCtx[i].threadNum = i;
                m_pThreadCtx[i].pSchedulerCore = this;
                vm_status vmRes = vm_cond_init(&m_pThreadCtx[i].taskAdded);
                if (VM_OK != vmRes) {
                    return MFX_ERR_UNKNOWN;
                }
                // spawn a thread
                iRes = vm_thread_create(
                    &(m_pThreadCtx[i].threadHandle),
                    scheduler_thread_proc,
                    m_pThreadCtx + i);
                if (0 == iRes)
                {
                    return MFX_ERR_UNKNOWN;
                }
                if (!SetScheduling(m_pThreadCtx[i].threadHandle)) {
                    return MFX_ERR_UNKNOWN;
                }
            }
        }
        catch (...)
        {
            return MFX_ERR_MEMORY_ALLOC;
        }


        SetThreadsAffinityToSockets();
    }
    else
    {
        // to run HW listen thread. Will be enabled if tests are OK

    }

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::Initialize(mfxSchedulerFlags flags, mfxU32 numberOfThreads)

mfxStatus mfxSchedulerCore::AddTask(const MFX_TASK &task, mfxSyncPoint *pSyncPoint)
{
    return AddTask(task, pSyncPoint, NULL, 0);

} // mfxStatus mfxSchedulerCore::AddTask(const MFX_TASK &task, mfxSyncPoint *pSyncPoint)

mfxStatus mfxSchedulerCore::Synchronize(mfxSyncPoint syncPoint, mfxU32 timeToWait)
{
    mfxTaskHandle handle;
    mfxStatus mfxRes;

    // check error(s)
    if (NULL == syncPoint)
    {
        return MFX_ERR_NULL_PTR;
    }

    // cast the pointer to handle and make syncing
    handle.handle = (size_t) syncPoint;


    // waiting on task's handle
    mfxRes = Synchronize(handle, timeToWait);


    return mfxRes;

} // mfxStatus mfxSchedulerCore::Synchronize(mfxSyncPoint syncPoint, mfxU32 timeToWait)

mfxStatus mfxSchedulerCore::Synchronize(mfxTaskHandle handle, mfxU32 timeToWait)
{
    MFX_SCHEDULER_TASK *pTask;

    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    // look up the task
    pTask = m_ppTaskLookUpTable[handle.taskID];
    if (NULL == pTask)
    {
        return MFX_ERR_NULL_PTR;
    }

    if (MFX_SINGLE_THREAD == m_param.flags)
    {
        //let really run task to
        MFX_CALL_INFO call = {};
        mfxTaskHandle previousTaskHandle = {};

        mfxStatus task_sts = MFX_ERR_NONE;
        mfxU64 start = GetHighPerformanceCounter();
        mfxU64 frequency = vm_time_get_frequency();
        while (MFX_WRN_IN_EXECUTION == pTask->opRes)
        {
            UMC::AutomaticMutex guard(m_guard);
            task_sts = GetTask(call, previousTaskHandle, 0);

            if (task_sts != MFX_ERR_NONE)
                continue;

            guard.Unlock();

            call.res = call.pTask->entryPoint.pRoutine(call.pTask->entryPoint.pState,
                                                       call.pTask->entryPoint.pParam,
                                                       call.threadNum,
                                                       call.callNum);

            guard.Lock();

            // save the previous task's handle
            previousTaskHandle = call.taskHandle;

            MarkTaskCompleted(&call, 0);

            if ((mfxU32)((GetHighPerformanceCounter() - start)/frequency) > timeToWait)
                break;

            if (MFX_TASK_DONE!= call.res)
            {
                vm_status vmRes;

                guard.Unlock();
                vmRes = vm_event_timed_wait(&m_hwTaskDone, 15 /*ms*/);
                guard.Lock();

                if (VM_OK == vmRes|| VM_TIMEOUT == vmRes)
                {
                    vmRes = vm_event_reset(&m_hwTaskDone);
                    IncrementHWEventCounter();
                }
            }
        }
        //
        // inspect the task
        //

        // the handle is outdated,
        // the previous task job is over and completed with successful status
        // NOTE: it make sense to read task result and job ID the following order.
        if ((MFX_ERR_NONE == pTask->opRes) ||
            (pTask->jobID != handle.jobID))
        {
            return MFX_ERR_NONE;
        }

        // wait the result on the event
        if (MFX_WRN_IN_EXECUTION == pTask->opRes)
        {
            return MFX_WRN_IN_EXECUTION;
        }

        // check error status
        if ((MFX_ERR_NONE != pTask->opRes) &&
            (pTask->jobID == handle.jobID))
        {
            return pTask->opRes;
        }

        // in all other cases task is complete
        return MFX_ERR_NONE;
    }
    else
    {
        UMC::AutomaticMutex guard(m_guard);
        vm_status res;
        vm_tick start_tick = vm_time_get_tick(), end_tick;
        mfxU32 spent_ms;

        while ((pTask->jobID == handle.jobID) &&
               (MFX_WRN_IN_EXECUTION == pTask->opRes)) {
            MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_PRIVATE, "Scheduler::Wait");
            MFX_LTRACE_1(MFX_TRACE_LEVEL_SCHED, "^Depends^on", "%d", pTask->param.task.nParentId);
            MFX_LTRACE_I(MFX_TRACE_LEVEL_SCHED, timeToWait);

            res = vm_cond_timedwait(&pTask->done, &m_guard, timeToWait);
            if ((VM_OK == res) || (VM_TIMEOUT == res)) {
                end_tick = vm_time_get_tick();

                spent_ms = (mfxU32)((end_tick - start_tick)/m_vmtick_msec_frequency);
                if (spent_ms >= timeToWait) {
                  break;
                }
                timeToWait -= spent_ms;
                start_tick = end_tick;
            } else {
                return MFX_ERR_UNKNOWN;
            }
        }

        if (pTask->jobID == handle.jobID) {
            return pTask->opRes;
        } else {
            /* Notes:
             *  - task executes next job already, we _lost_ task status and can only assume that
             *  everything was OK or FAILED, we will assume that task succeeded
             */
            return MFX_ERR_NONE;
        }
    }
}

mfxStatus mfxSchedulerCore::GetTimeout(mfxU32& maxTimeToRun)
{
    (void)maxTimeToRun;

    return MFX_ERR_UNSUPPORTED;
}

mfxStatus mfxSchedulerCore::WaitForDependencyResolved(const void *pDependency)
{
    mfxTaskHandle waitHandle = {};
    bool bFind = false;

    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }
    if (NULL == pDependency)
    {
        return MFX_ERR_NONE;
    }

    // find a handle to wait
    {
        UMC::AutomaticMutex guard(m_guard);
        mfxU32 curIdx;

        for (curIdx = 0; curIdx < m_numDependencies; curIdx += 1)
        {
            if (m_pDependencyTable[curIdx].p == pDependency)
            {
                // get the handle before leaving protected section
                waitHandle.taskID = m_pDependencyTable[curIdx].pTask->taskID;
                waitHandle.jobID = m_pDependencyTable[curIdx].pTask->jobID;

                // handle is found, go to wait
                bFind = true;
                break;
            }
        }
        // leave the protected section
    }

    // the dependency is still in the table,
    // wait until it leaves the table.
    if (bFind)
    {
        return Synchronize(waitHandle, MFX_TIME_INFINITE);
    }

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::WaitForDependencyResolved(const void *pDependency)

mfxStatus mfxSchedulerCore::WaitForTaskCompletion(const void *pOwner)
{
    mfxTaskHandle waitHandle;

    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }
    if (NULL == pOwner)
    {
        return MFX_ERR_NULL_PTR;
    }

    // make sure that threads are running
    {
        UMC::AutomaticMutex guard(m_guard);

        ResetWaitingTasks(pOwner);

        WakeUpThreads();
    }

    do
    {
        // reset waiting handle
        waitHandle.handle = 0;

        // enter protected section.
        // make searching in a separate code block,
        // to avoid dead-locks with other threads.
        {
            UMC::AutomaticMutex guard(m_guard);
            int priority;

            priority = MFX_PRIORITY_HIGH;
            do
            {
                int type;

                for (type = MFX_TYPE_HARDWARE; type <= MFX_TYPE_SOFTWARE; type += 1)
                {
                    const MFX_SCHEDULER_TASK *pTask = m_pTasks[priority][type];

                    // look through tasks list. Find the specified object.
                    while (pTask)
                    {
                        // the task with equal ID has been found.
                        if ((pTask->param.task.pOwner == pOwner) &&
                            (MFX_WRN_IN_EXECUTION == pTask->opRes))
                        {
                            // create wait handle to make waiting more precise
                            waitHandle.taskID = pTask->taskID;
                            waitHandle.jobID = pTask->jobID;
                            break;
                        }

                        // get the next task
                        pTask = pTask->pNext;
                    }
                }

            } while ((0 == waitHandle.handle) && (--priority >= MFX_PRIORITY_LOW));

            // leave the protected section
        }

        // wait for a while :-)
        // for a while and infinite - diffrent things )
        if (waitHandle.handle)
        {
            Synchronize(waitHandle, MFX_TIME_TO_WAIT);
        }

    } while (waitHandle.handle);

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::WaitForTaskCompletion(const void *pOwner)

mfxStatus mfxSchedulerCore::ResetWaitingStatus(const void *pOwner)
{
    // reset 'waiting' tasks belong to the given state
    ResetWaitingTasks(pOwner);

    // wake up sleeping threads
    WakeUpThreads();

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::ResetWaitingStatus(const void *pOwner)

mfxStatus mfxSchedulerCore::GetState(void)
{
    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::GetState(void)

mfxStatus mfxSchedulerCore::GetParam(MFX_SCHEDULER_PARAM *pParam)
{
    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }
    if (NULL == pParam)
    {
        return MFX_ERR_NULL_PTR;
    }

    // copy the parameters
    *pParam = m_param;

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::GetParam(MFX_SCHEDULER_PARAM *pParam)

mfxStatus mfxSchedulerCore::Reset(void)
{
    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    if (NULL == m_pFailedTasks)
    {
        return MFX_ERR_NONE;
    }

    // enter guarded section
    {
        UMC::AutomaticMutex guard(m_guard);

        // clean up the working queue
        ScrubCompletedTasks(true);
    }

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::Reset(void)

mfxStatus mfxSchedulerCore::AdjustPerformance(const mfxSchedulerMessage message)
{
    mfxStatus mfxRes = MFX_ERR_NONE;

    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }

    switch(message)
    {
        // reset the scheduler to the performance default state
    case MFX_SCHEDULER_RESET_TO_DEFAULTS:
        break;

    case MFX_SCHEDULER_START_HW_LISTENING:
        if (m_param.flags != MFX_SINGLE_THREAD)
        {
            if (0 == vm_thread_is_valid(&m_hwWakeUpThread))
            {
                mfxRes = StartWakeUpThread();
            }
        }
        break;

    case MFX_SCHEDULER_STOP_HW_LISTENING:
        if (m_param.flags != MFX_SINGLE_THREAD)
        {
            if (vm_thread_is_valid(&m_hwWakeUpThread))
            {
                mfxRes = StopWakeUpThread();
            }
        }
        break;

        // unknown message
    default:
        mfxRes = MFX_ERR_UNKNOWN;
        break;
    }

    return mfxRes;

} // mfxStatus mfxSchedulerCore::AdjustPerformance(const mfxSchedulerMessage message)


mfxStatus mfxSchedulerCore::AddTask(const MFX_TASK &task, mfxSyncPoint *pSyncPoint,
                                    const char *pFileName, int lineNumber)
{
#ifdef MFX_TRACE_ENABLE
    MFX_LTRACE_1(MFX_TRACE_LEVEL_SCHED, "^Enqueue^", "%d", task.nTaskId);
#endif


    // check error(s)
    if (0 == m_param.numberOfThreads)
    {
        return MFX_ERR_NOT_INITIALIZED;
    }
    if ((NULL == task.entryPoint.pRoutine) ||
        (NULL == pSyncPoint))
    {
        return MFX_ERR_NULL_PTR;
    }

    // make sure that there is enough free task objects
    m_freeTasks.Wait();

    // enter protected section
    {
        UMC::AutomaticMutex guard(m_guard);
        mfxStatus mfxRes;
        MFX_SCHEDULER_TASK *pTask, **ppTemp;
        mfxTaskHandle handle;
        MFX_THREAD_ASSIGNMENT *pAssignment = NULL;
        mfxU32 occupancyIdx;
        int type;

        // Make sure that there is an empty task object

        mfxRes = AllocateEmptyTask();
        if (MFX_ERR_NONE != mfxRes)
        {
            // better to return error instead of WRN  (two-tasks per component scheme)
            return MFX_ERR_MEMORY_ALLOC;
        }

        // initialize the task
        m_pFreeTasks->ResetDependency();
        mfxRes = m_pFreeTasks->Reset();
        if (MFX_ERR_NONE != mfxRes)
        {
            return mfxRes;
        }
        m_pFreeTasks->param.task = task;
        mfxRes = GetOccupancyTableIndex(occupancyIdx, &task);
        if (MFX_ERR_NONE != mfxRes)
        {
            return mfxRes;
        }
        pAssignment = &(m_occupancyTable[occupancyIdx]);

        // update the thread assignment parameters
        if (MFX_TASK_INTRA & task.threadingPolicy)
        {
            // last entries in the dependency arrays must be empty
            if ((m_pFreeTasks->param.task.pSrc[MFX_TASK_NUM_DEPENDENCIES - 1]) ||
                (m_pFreeTasks->param.task.pDst[MFX_TASK_NUM_DEPENDENCIES - 1]))
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            // fill INTRA task dependencies
            m_pFreeTasks->param.task.pSrc[MFX_TASK_NUM_DEPENDENCIES - 1] = pAssignment->pLastTask;
            m_pFreeTasks->param.task.pDst[MFX_TASK_NUM_DEPENDENCIES - 1] = m_pFreeTasks;
            // update the last intra task pointer
            pAssignment->pLastTask = m_pFreeTasks;
        }
        // do not save the pointer to thread assigment instance
        // until all checking have been done
        m_pFreeTasks->param.pThreadAssignment = pAssignment;
        pAssignment->m_numRefs += 1;

        // saturate the number of available threads
        uint32_t numThreads = m_pFreeTasks->param.task.entryPoint.requiredNumThreads;
        numThreads = (0 == numThreads) ? (m_param.numberOfThreads) : (numThreads);
        numThreads = UMC::get_min(m_param.numberOfThreads, numThreads);
        numThreads = (uint32_t) UMC::get_min(sizeof(pAssignment->threadMask) * 8, numThreads);
        m_pFreeTasks->param.task.entryPoint.requiredNumThreads = numThreads;

        // set the advanced task's info
        m_pFreeTasks->param.sourceInfo.pFileName = pFileName;
        m_pFreeTasks->param.sourceInfo.lineNumber = lineNumber;
        // set the sync point for the task
        handle.taskID = m_pFreeTasks->taskID;
        handle.jobID = m_pFreeTasks->jobID;
        *pSyncPoint = (mfxSyncPoint) handle.handle;

        // Register task dependencies
        RegisterTaskDependencies(m_pFreeTasks);


        //
        // move task to the corresponding task
        //

        // remove the task from the 'free' queue
        pTask = m_pFreeTasks;
        m_pFreeTasks = m_pFreeTasks->pNext;
        pTask->pNext = NULL;

        // find the end of the corresponding queue
        type = (task.threadingPolicy & MFX_TASK_DEDICATED) ? (MFX_TYPE_HARDWARE) : (MFX_TYPE_SOFTWARE);
        ppTemp = m_pTasks[task.priority] + type;
        while (*ppTemp)
        {
            ppTemp = &((*ppTemp)->pNext);
        }

        // add the task to the end of the corresponding queue
        *ppTemp = pTask;

        // reset all 'waiting' tasks to prevent freezing
        // so called 'permanent' tasks.
        ResetWaitingTasks(pTask->param.task.pOwner);

        mfxU32 num_hw_threads = 0, num_sw_threads = 0;

        // increment the number of available tasks
        if (MFX_TASK_DEDICATED & task.threadingPolicy) {
            num_hw_threads = numThreads;
        } else {
            num_sw_threads = numThreads;
        }

        // wake up working threads if task has resolved dependencies
        if (IsReadyToRun(pTask)) {
            WakeUpThreads(num_hw_threads, num_sw_threads);
        }

        // leave the protected section
    }

    return MFX_ERR_NONE;

}

mfxStatus mfxSchedulerCore::DoWork()
{
    return MFX_ERR_UNSUPPORTED;
} // mfxStatus mfxSchedulerCore::DoWork()




