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
#include <mfx_scheduler_core_handle.h>
#include <mfx_trace.h>

#include <vm_time.h>
#include <vm_sys_info.h>
#include <algorithm>


mfxSchedulerCore::mfxSchedulerCore(void)
    :  m_currentTimeStamp(0)
    // since on Linux we have blocking synchronization which means an absence of polling,
    // there is no need to use 'waiting' time period.
    , m_timeWaitPeriod(0)
    , m_hwWakeUpThread()
    , m_DedicatedThreadsToWakeUp(0)
    , m_RegularThreadsToWakeUp(0)
{
    memset(&m_param, 0, sizeof(m_param));
    m_refCounter = 1;

    memset(m_workingTime, 0, sizeof(m_workingTime));
    m_timeIdx = 0;

    m_bQuit = false;

    m_pThreadCtx = NULL;
    m_vmtick_msec_frequency = vm_time_get_frequency()/1000;

    // reset task variables
    memset(m_pTasks, 0, sizeof(m_pTasks));
    memset(m_numAssignedTasks, 0, sizeof(m_numAssignedTasks));
    m_pFailedTasks = NULL;

    m_pFreeTasks = NULL;

    // reset dependency table variables
    m_numDependencies = 0;

    // reset busy objects table
    m_numOccupancies = 0;

    // reset task counters
    m_taskCounter = 0;
    m_jobCounter = 0;

    m_hwEventCounter = 0;

    m_timer_hw_event = MFX_THREAD_TIME_TO_WAIT;

    // set number of free tasks
    m_freeTasksCount = MFX_MAX_NUMBER_TASK;

} // mfxSchedulerCore::mfxSchedulerCore(void)

mfxSchedulerCore::~mfxSchedulerCore(void)
{
    Close();

} // mfxSchedulerCore::~mfxSchedulerCore(void)

bool mfxSchedulerCore::SetScheduling(std::thread& handle)
{
    (void)handle;
    if (m_param.params.SchedulingType || m_param.params.Priority) {
        if (handle.joinable()) {
            struct sched_param param{};

            param.sched_priority = m_param.params.Priority;
            return !pthread_setschedparam(handle.native_handle(), m_param.params.SchedulingType, &param);
        }
    }
    return true;
}

void mfxSchedulerCore::SetThreadsAffinityToSockets(void)
{
}

void mfxSchedulerCore::Close(void)
{
    StopWakeUpThread();

    // stop threads
    if (m_pThreadCtx)
    {
        mfxU32 i;

        // set the 'quit' flag for threads
        m_bQuit = true;

        {
            // set the events to wake up sleeping threads
            std::lock_guard<std::mutex> guard(m_guard);
            WakeUpThreads();
        }

        for (i = 0; i < m_param.numberOfThreads; i += 1)
        {
            // wait for particular thread
            if (m_pThreadCtx[i].threadHandle.joinable())
                m_pThreadCtx[i].threadHandle.join();
        }

        delete[] m_pThreadCtx;
    }

    // run over the task lists and abort the existing tasks
    ForEachTask(
        [](MFX_SCHEDULER_TASK* task)
        {
            if (MFX_TASK_WORKING == task->curStatus)
            {
                task->CompleteTask(MFX_ERR_ABORTED);
            }
        }
    );

    // delete task objects
    for (auto & it : m_ppTaskLookUpTable)
    {
        delete it;
        it = nullptr;
    }


    memset(&m_param, 0, sizeof(m_param));

    memset(m_workingTime, 0, sizeof(m_workingTime));
    m_timeIdx = 0;

    // reset variables
    m_bQuit = false;
    m_pThreadCtx = NULL;
    // reset task variables
    memset(m_pTasks, 0, sizeof(m_pTasks));
    memset(m_numAssignedTasks, 0, sizeof(m_numAssignedTasks));
    m_pFailedTasks = NULL;

    m_pFreeTasks = NULL;

    // reset dependency table variables
    m_numDependencies = 0;

    // reset busy objects table
    m_numOccupancies = 0;

    // reset task counters
    m_taskCounter = 0;
    m_jobCounter = 0;
}

void mfxSchedulerCore::WakeUpThreads(mfxU32 num_dedicated_threads, mfxU32 num_regular_threads)
{
    if (m_param.flags == MFX_SINGLE_THREAD)
        return;

    MFX_SCHEDULER_THREAD_CONTEXT* thctx;

    if (num_dedicated_threads) {
        // we have single dedicated thread, thus no loop here
        thctx = GetThreadCtx(0);
        if (thctx->state == MFX_SCHEDULER_THREAD_CONTEXT::Waiting) {
            thctx->taskAdded.notify_one();
        }
    }
    // if we have woken up dedicated thread, we exclude it from the loop below
    for (mfxU32 i = (num_dedicated_threads)? 1: 0; (i < m_param.numberOfThreads) && num_regular_threads; ++i) {
        thctx = GetThreadCtx(i);
        if (thctx->state == MFX_SCHEDULER_THREAD_CONTEXT::Waiting) {
            thctx->taskAdded.notify_one();
            --num_regular_threads;
        }
    }
}

void mfxSchedulerCore::Wait(const mfxU32 curThreadNum, std::unique_lock<std::mutex>& mutex)
{
    MFX_SCHEDULER_THREAD_CONTEXT* thctx = GetThreadCtx(curThreadNum);

    if (thctx) {
        thctx->taskAdded.wait(mutex);
    }
}

mfxU64 mfxSchedulerCore::GetHighPerformanceCounter(void)
{
    return (mfxU64) vm_time_get_tick();

} // mfxU64 mfxSchedulerCore::GetHighPerformanceCounter(void)

mfxU32 mfxSchedulerCore::GetLowResCurrentTime(void)
{
    return vm_time_get_current_time();

} // mfxU32 mfxSchedulerCore::GetCurrentTime(void)
mfxStatus mfxSchedulerCore::AllocateEmptyTask(void)
{
    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //

    // Clean up task queues
    ScrubCompletedTasks();

    // allocate one new task
    if (nullptr == m_pFreeTasks)
    {


        // the maximum allowed number of tasks is reached
        if (MFX_MAX_NUMBER_TASK <= m_taskCounter)
        {
            return MFX_WRN_DEVICE_BUSY;
        }

        // allocate one more task
        try
        {
            m_pFreeTasks = new MFX_SCHEDULER_TASK(m_taskCounter++, this);
        }
        catch(...)
        {
            return MFX_WRN_DEVICE_BUSY;
        }
        // register the task in the look up table
        m_ppTaskLookUpTable[m_pFreeTasks->taskID] = m_pFreeTasks;
    }
    memset(&(m_pFreeTasks->param), 0, sizeof(m_pFreeTasks->param));
    // increment job number. This number must grow evenly.
    // make job number 0 an invalid value to avoid problem with
    // task number 0 with job number 0, which are NULL when being combined.
    m_jobCounter += 1;
    if (MFX_MAX_NUMBER_JOB <= m_jobCounter)
    {
        m_jobCounter = 1;
    }
    m_pFreeTasks->jobID = m_jobCounter;

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::AllocateEmptyTask(void)

mfxStatus mfxSchedulerCore::GetOccupancyTableIndex(mfxU32 &idx,
                                                   const MFX_TASK *pTask)
{
    mfxU32 i = 0;
    MFX_THREAD_ASSIGNMENT *pAssignment = NULL;

    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //

    // check the table, decrement the number of used entries
    while ((m_numOccupancies) &&
           (0 == m_occupancyTable[m_numOccupancies - 1].m_numRefs))
    {
        m_numOccupancies -= 1;
    }

    // find the existing element with the given pState and pRoutine
    for (i = 0; i < m_numOccupancies; i += 1)
    {
        if ((m_occupancyTable[i].pState == pTask->entryPoint.pState) &&
            (m_occupancyTable[i].pRoutine == pTask->entryPoint.pRoutine))
        {
            // check the type of other tasks using this table entry
            if (m_occupancyTable[i].threadingPolicy != pTask->threadingPolicy)
            {
                return MFX_ERR_INVALID_VIDEO_PARAM;
            }

            pAssignment = &(m_occupancyTable[i]);
            break;
        }
    }

    // if the element exist, check the parameters for compatibility
    if (pAssignment)
    {
        // actually, there is nothing to do
    }
    // allocate one more element in the array
    else
    {
        for (i = 0; i < m_numOccupancies; i += 1)
        {
            if (0 == m_occupancyTable[i].m_numRefs)
            {
                break;
            }
        }
        // we can't reallocate the table
        if (m_occupancyTable.size() == i)
        {
            return MFX_WRN_DEVICE_BUSY;
        }

        pAssignment = &(m_occupancyTable[i]);

        // fill the parameters
        memset(pAssignment, 0, sizeof(MFX_THREAD_ASSIGNMENT));
        pAssignment->pState = pTask->entryPoint.pState;
        pAssignment->pRoutine = pTask->entryPoint.pRoutine;
        pAssignment->threadingPolicy = pTask->threadingPolicy;
    }

    // update the number of allocated objects
    m_numOccupancies = std::max(mfxU32(m_numOccupancies), i + 1);

    // save the index to return
    idx = i;

    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::GetOccupancyTableIndex(mfxU32 &idx,

void mfxSchedulerCore::ScrubCompletedTasks(bool bComprehensive)
{
    int priority;

    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //

    for (priority = MFX_PRIORITY_HIGH;
         priority >= MFX_PRIORITY_LOW;
         priority -= 1)
    {
        int type;

        for (type = MFX_TYPE_HARDWARE; type <= MFX_TYPE_SOFTWARE; type += 1)
        {
            MFX_SCHEDULER_TASK **ppCur;

            // if there is an empty task, immediately return
            if ((false == bComprehensive) &&
                (m_pFreeTasks))
            {
                return;
            }

            ppCur = m_pTasks[priority] + type;
            while (*ppCur)
            {
                // move task completed to the 'free' queue.
                if (MFX_ERR_NONE == (*ppCur)->opRes)
                {
                    MFX_SCHEDULER_TASK *pTemp;

                    // cut the task from the queue
                    pTemp = *ppCur;
                    *ppCur = pTemp->pNext;
                    // add it to the 'free' queue
                    pTemp->pNext = m_pFreeTasks;
                    m_pFreeTasks = pTemp;
                }
                // move task failed to the 'failed' queue.
                else if ((MFX_ERR_NONE != (*ppCur)->opRes) &&
                         (MFX_WRN_IN_EXECUTION != (*ppCur)->opRes))
                {
                    MFX_SCHEDULER_TASK *pTemp;

                    // cut the task from the queue
                    pTemp = *ppCur;
                    *ppCur = pTemp->pNext;
                    // add it to the 'failed' queue
                    pTemp->pNext = m_pFailedTasks;
                    m_pFailedTasks = pTemp;
                }
                else
                {
                    // set the next task
                    ppCur = &((*ppCur)->pNext);
                }
            }
        }
    }

} // void mfxSchedulerCore::ScrubCompletedTasks(bool bComprehensive)

void mfxSchedulerCore::RegisterTaskDependencies(MFX_SCHEDULER_TASK  *pTask)
{
    mfxU32 i, tableIdx, remainInputs;
    const void *pSrcCopy[MFX_TASK_NUM_DEPENDENCIES];
    mfxStatus taskRes = MFX_WRN_IN_EXECUTION;

    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //

    // check if the table have empty position(s),
    // If so decrement the index of the last table entry.
    if (m_pDependencyTable.size() > m_numDependencies)
    {
        auto it = std::find_if(m_pDependencyTable.rend() - m_numDependencies, m_pDependencyTable.rend(),
                               [](const MFX_DEPENDENCY_ITEM & item){ return item.p != nullptr; });

        m_numDependencies = m_pDependencyTable.rend() - it;
    }

    // get the number of source dependencies
    remainInputs = 0;
    for (i = 0; i < MFX_TASK_NUM_DEPENDENCIES; i += 1)
    {
        // make a copy of source dependencies.
        // source dependencies have to be swept, because of duplication in
        // the dependency table. task will sync on the first matching entry.
        pSrcCopy[i] = pTask->param.task.pSrc[i];
        if (pSrcCopy[i])
        {
            remainInputs += 1;
        }
    }

    // run over the table and save the handles of incomplete inputs
    for (tableIdx = 0; tableIdx < m_numDependencies; tableIdx += 1)
    {
        // compare only filled table entries
        if (m_pDependencyTable[tableIdx].p)
        {
            for (i = 0; i < MFX_TASK_NUM_DEPENDENCIES; i += 1)
            {
                // we found the source dependency,
                // save the handle
                if (m_pDependencyTable[tableIdx].p == pSrcCopy[i])
                {
                    // dependency is fail. The dependency resolved, but failed.
                    if (MFX_WRN_IN_EXECUTION != m_pDependencyTable[tableIdx].mfxRes)
                    {
                        // waiting task inherits status from the parent task
                        // need to propogate error status to all dependent tasks.
                        //if (MFX_TASK_WAIT & pTask->param.task.threadingPolicy)
                        {
                            taskRes = m_pDependencyTable[tableIdx].mfxRes;
                        }
                        //// all other tasks are aborted
                        //else
                        //{
                        //    taskRes = MFX_ERR_ABORTED;
                        //}
                    }
                    // link dependency
                    else
                    {
                        m_pDependencyTable[tableIdx].pTask->SetDependentItem(pTask, i);
                    }
                    // sweep already used dependency
                    pSrcCopy[i] = NULL;
                    remainInputs -= 1;
                    break;
                }
            }

            // is there more source dependencies?
            if (0 == remainInputs)
            {
                break;
            }
        }
    }

    // run over the table and register generated outputs
    tableIdx = 0;
    for (i = 0; i < MFX_TASK_NUM_DEPENDENCIES; i += 1)
    {
        if (pTask->param.task.pDst[i])
        {
            // find empty table entry
            while (m_pDependencyTable.at(tableIdx).p)
            {
                tableIdx += 1;
            }

            // save the generated dependency
            m_pDependencyTable[tableIdx].p = pTask->param.task.pDst[i];
            m_pDependencyTable[tableIdx].mfxRes = taskRes;
            m_pDependencyTable[tableIdx].pTask = pTask;

            // save the index of the output
            pTask->param.dependencies.dstIdx[i] = tableIdx;
            tableIdx += 1;
        }
    }

    // update the dependency table max index
    if (tableIdx >= m_numDependencies)
    {
        m_numDependencies = tableIdx;
    }

    // if dependency were failed,
    // set the task into the 'aborted' state
    if (MFX_WRN_IN_EXECUTION != taskRes)
    {
        // save the status
        m_pFreeTasks->curStatus = taskRes;
        m_pFreeTasks->opRes = taskRes;
        m_pFreeTasks->done.notify_all();
    }

} // void mfxSchedulerCore::RegisterTaskDependencies(MFX_SCHEDULER_TASK  *pTask)



//#define ENABLE_TASK_DEBUG


void mfxSchedulerCore::PrintTaskInfo(void)
{

} // void mfxSchedulerCore::PrintTaskInfo(void)

void mfxSchedulerCore::PrintTaskInfoUnsafe(void)
{

} // void mfxSchedulerCore::PrintTaskInfoUnsafe(void)
