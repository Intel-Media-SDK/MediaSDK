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

#include <vm_time.h>

// declare the static section of the file
namespace
{

const
int TaskPriorityRatio[MFX_PRIORITY_NUMBER] =
{
    // MFX_PRIORITY_LOW tasks utilize 100% of CPU power, which is left
    // unitilized by higher priority tasks.
    100,

    // MFX_PRIORITY_NORMAL tasks utilize 75% of CPU power, which is left
    // unitilized by MFX_PRIORITY_REALTIME priority tasks.
    75,

    // MFX_PRIORITY_HIGH tasks try to utilize 100% of CPU power,
    // if it has enough to do.
    100
};

enum
{
    // waiting tasks are examined every 1 msec
    MFX_WAIT_TIME_MS            = 1
};

} // namespace

int mfxSchedulerCore::GetTaskPriority(mfxTaskHandle task)
{
    int taskPriority = -1;

    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //
    const MFX_SCHEDULER_TASK *pTask = m_ppTaskLookUpTable.at(task.taskID);
    if ((pTask) &&
        (pTask->jobID == task.jobID) &&
        (MFX_TASK_WORKING == pTask->curStatus))
    {
        taskPriority = pTask->param.task.priority;
    }

    return taskPriority;

} // int mfxSchedulerCore::GetTaskPriority(mfxTaskHandle task)

void mfxSchedulerCore::GetTimeStat(mfxU64 timeSpent[MFX_PRIORITY_NUMBER],
                                   mfxU64 totalTimeSpent[MFX_PRIORITY_NUMBER])
{
    int priority;

    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //

    for (priority = MFX_PRIORITY_LOW; priority < MFX_PRIORITY_NUMBER; priority += 1)
    {
        mfxU32 i;

        totalTimeSpent[priority] = 0;
        timeSpent[priority] = 0;
        for (i = 0; i < MFX_TIME_STAT_PARTS; i += 1)
        {
            int other;

            // fill the time spent by threads with given priority
            timeSpent[priority] += m_workingTime[i].time[priority];
            // fill the time spend by threads with given or lowere priority
            for (other = MFX_PRIORITY_LOW; other <= priority; other += 1)
            {
                totalTimeSpent[priority] += m_workingTime[i].time[other];
            }
        }
    }

} // void mfxSchedulerCore::GetTimeStat(mfxU64 totalTimeSpent[MFX_PRIORITY_NUMBER],

enum
{
    // on the first run when the scheduler tries to get a task,
    // it tries to keep tasks prioirties.
    PRIORITY_RUN = 0,
    // on the second run the scheduler tries to get any 'ready' task,
    // ignoring priority.
    REGULAR_RUN = 1,

    NUMBER_OF_RUNS
};

mfxStatus mfxSchedulerCore::GetTask(MFX_CALL_INFO &callInfo,
                                    mfxTaskHandle previousTask,
                                    const mfxU32 threadNum)
{
    int prevTaskPriority = -1;
    mfxU32 run;
    mfxU64 totalTimeSpent[MFX_PRIORITY_NUMBER], timeSpent[MFX_PRIORITY_NUMBER];

    // get the current time stamp
    m_currentTimeStamp = GetHighPerformanceCounter();

    // get time spent statistic
    GetTimeStat(timeSpent, totalTimeSpent);

    // get the priority of the previous task
    prevTaskPriority = GetTaskPriority(previousTask);

    // there are three runs over the tasks lists. On the 1st run,
    // the scheduler keeping workload balance, which is described by
    // the TaskPriorityRatio table. On the 2nd run, the scheduler chooses
    // any ready task.
    for (run = 0; run < NUMBER_OF_RUNS; run += 1)
    {
        int priority;

        for (priority = MFX_PRIORITY_HIGH;
             priority >= MFX_PRIORITY_LOW;
             priority -= 1)
        {
            //
            // check the number of assigned task by the particular priority
            //

            // the second run always examines tasks
            if ((PRIORITY_RUN != run) ||
                // during the first run we subscribe tasks only,
                // if the kind of tasks does not utilize its share
                // of CPU performance.
                (TaskPriorityRatio[priority] * totalTimeSpent[priority] >=
                 100 * timeSpent[priority]))
            {
                int type;

                for (type = (threadNum) ? (MFX_TYPE_SOFTWARE) : (MFX_TYPE_HARDWARE);
                     type <= MFX_TYPE_SOFTWARE;
                     type += 1)
                {
                    MFX_SCHEDULER_TASK *pTask = m_pTasks[priority][type];

                    // try to continue the previous task
                    if (prevTaskPriority == priority)
                    {
                        mfxStatus mfxRes;

                        // try get the same task again
                        mfxRes = CanContinuePreviousTask(callInfo,
                                                         previousTask,
                                                         threadNum);
                        if (MFX_ERR_NONE == mfxRes)
                        {
                            return mfxRes;
                        }
                    }

                    // run over the tasks list
                    while (pTask)
                    {
                        mfxStatus mfxRes;

                        // is this task ready?
                        mfxRes = WrapUpTask(callInfo, pTask, threadNum);
                        if (MFX_ERR_NONE == mfxRes)
                        {
                            return mfxRes;
                        }

                        // get the next task
                        pTask = pTask->pNext;
                    }
                }
            }
        }
    }

    // print task parameters for DEBUG purposes
    PrintTaskInfoUnsafe();

    return MFX_ERR_NOT_FOUND;

} // mfxStatus mfxSchedulerCore::GetTask(MFX_CALL_INFO &callInfo,

mfxStatus mfxSchedulerCore::CanContinuePreviousTask(MFX_CALL_INFO &callInfo,
                                                    mfxTaskHandle previousTask,
                                                    const mfxU32 threadNum)
{
    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //

    // get the task object
    MFX_SCHEDULER_TASK *pTask = m_ppTaskLookUpTable.at(previousTask.taskID);
    if ((nullptr == pTask) ||
        (pTask->jobID != previousTask.jobID))
    {
        return MFX_ERR_NOT_FOUND;
    }

    return WrapUpTask(callInfo, pTask, threadNum);

} // mfxStatus mfxSchedulerCore::CanContinuePreviousTask(MFX_CALL_INFO &callInfo,

// static section of the file
namespace
{

enum
{
    MFX_INVALID_THREAD_NUMBER   = 0x7fffffff
};

inline
mfxU32 GetFreeThreadNumber(MFX_THREAD_ASSIGNMENT &occupancyInfo,
                           MFX_SCHEDULER_TASK *pTask)
{
    mfxU64 mask;
    mfxU32 numThreads;
    mfxU32 i;

    // get available thread mask and maximum allowed threads number
    if (MFX_TASK_INTER & occupancyInfo.threadingPolicy)
    {
        mask = pTask->param.threadMask;
    }
    else
    {
        mask = occupancyInfo.threadMask;
    }
    numThreads = pTask->param.task.entryPoint.requiredNumThreads;

    for (i = 0; i < numThreads; i += 1)
    {
        if (0 == (mask & (1LL << i)))
        {
            return i;
        }
    }

    return MFX_INVALID_THREAD_NUMBER;

} // mfxU32 GetFreeThreadNumber(MFX_THREAD_ASSIGNMENT &occupancyInfo,

} // namespace

bool mfxSchedulerCore::IsReadyToRun(MFX_SCHEDULER_TASK *pTask)
{
    // task is not ready to run (ro should not be run)
    // if task is already done
    if (MFX_TASK_NEED_CONTINUE != pTask->curStatus) {
        return false;
    }
    // or dependencies are not resolved
    if (false == pTask->IsDependenciesResolved()) {
        return false;
    }
    // or there is no proper thread number
    if (MFX_INVALID_THREAD_NUMBER == GetFreeThreadNumber(*(pTask->param.pThreadAssignment), pTask)) {
        return false;
    }
    // or task is still waiting
    if (pTask->param.bWaiting) {
        // prevent entering more than 1 thread in 'waiting' task,
        // let the thread inspect other tasks.
        if (pTask->param.occupancy) {
            return false;
        } else if (m_timeWaitPeriod) {
            // check the 'waiting' time period
            // calculate the period elapsed since the last 'need-waiting' call
            mfxU64 time = GetHighPerformanceCounter() - pTask->param.timing.timeLastEnter;
            if (m_timeWaitPeriod > time) {
                const mfxU64 hwCounter = GetHWEventCounter();

                if (hwCounter == pTask->param.timing.hwCounterLastEnter) {
                    return false;
                }
            }
        }
    }
    return true;
}

mfxStatus mfxSchedulerCore::WrapUpTask(MFX_CALL_INFO &callInfo,
                                       MFX_SCHEDULER_TASK *pTask,
                                       const mfxU32 threadNum)
{
    MFX_THREAD_ASSIGNMENT &occupancyInfo = *(pTask->param.pThreadAssignment);

    //
    // THE EXECUTION IS ALREADY IN SECURE SECTION.
    // Just do what need to do.
    //

    if (!IsReadyToRun(pTask)) {
        return MFX_ERR_NOT_FOUND;
    }

    // non-zero thread tries to get a dedicated (hardware) task...
    if (threadNum && (MFX_TASK_DEDICATED & occupancyInfo.threadingPolicy)) {
        return MFX_ERR_NOT_FOUND;
    }

    //
    // everything is OK.
    // Update the task and return the parameters
    //

    callInfo.threadNum = GetFreeThreadNumber(occupancyInfo, pTask);
    callInfo.callNum = pTask->param.numberOfCalls;

    // update the scheduler
    m_numAssignedTasks[pTask->param.task.priority] += 1;

    // update the number of assigned threads
    occupancyInfo.taskOccupancy += (0 == pTask->param.occupancy) ? (1) : (0);
    if (0 == (MFX_TASK_INTER & occupancyInfo.threadingPolicy))
    {
        occupancyInfo.occupancy += 1;
        occupancyInfo.threadMask |= (1LL << callInfo.threadNum);
    }
    pTask->param.occupancy += 1;
    pTask->param.threadMask |= (1LL << callInfo.threadNum);

    pTask->param.numberOfCalls += 1;

    // update the task's timing
    pTask->param.timing.timeLastEnter = m_currentTimeStamp;
    pTask->param.timing.timeLastCallIssued = m_currentTimeStamp;
    pTask->param.timing.hwCounterLastEnter = GetHWEventCounter();
    // create handle
    callInfo.taskHandle.taskID = pTask->taskID;
    callInfo.taskHandle.jobID = pTask->jobID;
    // set the task pointer
    callInfo.pTask = &pTask->param.task;
    // set the time stamp of task release
    callInfo.timeStamp = m_currentTimeStamp;


    return MFX_ERR_NONE;

} // mfxStatus mfxSchedulerCore::WrapUpTask(MFX_CALL_INFO &callInfo,

void mfxSchedulerCore::ResetWaitingTasks(const void *pOwner)
{
    ForEachTask(
        [pOwner](MFX_SCHEDULER_TASK* task)
        {
            // reset the 'waiting' flag
            if ((task->param.task.pOwner == pOwner) &&
                (MFX_TASK_NEED_CONTINUE == task->curStatus))
            {
                // resetting 'waiting' flag should help waking up permanent tasks
                task->param.bWaiting = false;

                // set new time of the last call processed to avoid overwriting
                // 'waiting' status flag.
                task->param.timing.timeLastCallProcessed = task->param.timing.timeLastCallIssued + 1;
            }
        }
    );
} // void mfxSchedulerCore::ResetWaitingTasks(const void *pOwner)

void mfxSchedulerCore::OnDependencyResolved(MFX_SCHEDULER_TASK *pTask)
{
    if (IsReadyToRun(pTask)) {
        if (MFX_TASK_DEDICATED & pTask->param.task.threadingPolicy) {
            m_DedicatedThreadsToWakeUp += pTask->param.task.entryPoint.requiredNumThreads;
        } else {
            m_RegularThreadsToWakeUp += pTask->param.task.entryPoint.requiredNumThreads;
        }
    }
}

void mfxSchedulerCore::MarkTaskCompleted(const MFX_CALL_INFO *pCallInfo,
                                         const mfxU32 threadNum)
{
    (void)pCallInfo;
    (void)threadNum;

    MFX_SCHEDULER_TASK *pTask = nullptr;
    pTask = m_ppTaskLookUpTable.at(pCallInfo->taskHandle.taskID);

    // check error(s)
    if (nullptr == pTask)
    {
        return;
    }

    bool taskReleased = false;
    mfxU32 nTraceTaskId = 0;
    mfxU32 curTime;

    MFX_THREAD_ASSIGNMENT &occupancyInfo = *(pTask->param.pThreadAssignment);

    // update working time
    curTime = GetLowResCurrentTime();
    if (m_workingTime[m_timeIdx].startTime + MFX_TIME_STAT_PERIOD / MFX_TIME_STAT_PARTS <
        curTime)
    {
        // advance the working time index. The current entry is out of time.
        m_timeIdx = (m_timeIdx + 1) % MFX_TIME_STAT_PARTS;
        memset(m_workingTime + m_timeIdx, 0, sizeof(m_workingTime[m_timeIdx]));
        m_workingTime[m_timeIdx].startTime = curTime;
    }
    m_workingTime[m_timeIdx].time[pTask->param.task.priority] += pCallInfo->timeSpend;

    // update the scheduler
    m_numAssignedTasks[pTask->param.task.priority] -= 1;

    // clean up the task object
    pTask->param.occupancy -= 1;
    pTask->param.threadMask &= ~(1LL << pCallInfo->threadNum);
    if (0 == (MFX_TASK_INTER & occupancyInfo.threadingPolicy))
    {
        occupancyInfo.occupancy -= 1;
        occupancyInfo.threadMask &= ~(1LL << pCallInfo->threadNum);
    }
    occupancyInfo.taskOccupancy -= (0 == pTask->param.occupancy) ? (1) : (0);

    // Below we will notify dependent tasks that this task is done and
    // scheduler will get notifications from dependent tasks that their
    // dependencies are resolved. Upon these notifications scheduler will
    // calculate how many threads we need to wakeup to handle dependent
    // tasks.
    m_DedicatedThreadsToWakeUp = 0;
    m_RegularThreadsToWakeUp = 0;

    // try to not overwrite the newest status from other thread
    if (pTask->param.timing.timeLastCallProcessed < pCallInfo->timeStamp)
    {
        pTask->param.timing.timeLastCallProcessed = pCallInfo->timeStamp;
    }
    // update the status of the current job
    if (isFailed(pCallInfo->res))
    {
        pTask->curStatus = pCallInfo->res;
    }
    // do not overwrite the failed status with successful one
    else if ((MFX_TASK_DONE == pCallInfo->res) &&
                (MFX_TASK_NEED_CONTINUE == pTask->curStatus))
    {
        pTask->curStatus = pCallInfo->res;

        // reset all waiting tasks with the given working object
        ResetWaitingTasks(pCallInfo->pTask->pOwner);
    }
    // update the task timing
    else if (MFX_TASK_BUSY == pCallInfo->res)
    {
        // try to not overwrite the newest status from other thread
        if (pTask->param.timing.timeLastCallProcessed <= pCallInfo->timeStamp)
        {
            pTask->param.bWaiting = true;
        }
        pTask->param.timing.timeOverhead += pCallInfo->timeSpend;
    }
    else
    {
        // reset all waiting tasks with the given working object
        ResetWaitingTasks(pCallInfo->pTask->pOwner);
    }
    pTask->param.timing.timeSpent += pCallInfo->timeSpend;

    //
    // update task status and tasks dependencies
    //

    if (0 == pTask->param.occupancy)
    {
        // complete the task if it is done
        if ((isFailed(pTask->curStatus)) ||
            (MFX_TASK_DONE == pTask->curStatus))
        {
            // store TaskId for tracing event
            nTraceTaskId = pCallInfo->pTask->nTaskId;

            // get entry point parameters to call FreeResources
            MFX_ENTRY_POINT &entryPoint = pTask->param.task.entryPoint;

            if (entryPoint.pCompleteProc)
            {
                mfxStatus mfxRes;

                // temporarily leave the protected code section
                m_guard.unlock();

                mfxRes = pTask->CompleteTask(pTask->curStatus);
                if ((isFailed(mfxRes)) &&
                    (MFX_ERR_NONE == pTask->curStatus)) {

                    pTask->curStatus = mfxRes;
                }

                // enter the protected code section
                m_guard.lock();
            }
        }

        // update the failed task status
        if (isFailed(pTask->curStatus))
        {
            //mfxU32 i;

            // save the status
            pTask->opRes = pTask->curStatus;

            pTask->done.notify_all();

            // update dependencies produced from the dependency table
            //for (i = 0; i < MFX_TASK_NUM_DEPENDENCIES; i += 1)
            //{
            //    if (pTask->param.task.pDst[i])
            //    {
            //        mfxU32 idx = pTask->param.dependencies.dstIdx[i];

            //        m_pDependencyTable[idx].mfxRes = pTask->curStatus;
            //    }
            //}
            ResolveDependencyTable(pTask);

            // mark all dependent task as 'failed'
            pTask->ResolveDependencies(pTask->curStatus);
            // release all allocated resources
            pTask->ReleaseResources();
        }
        // process task completed
        else if (MFX_TASK_DONE == pTask->curStatus)
        {
            mfxU32 i;


            // reset jobID to avoid false waiting on complete tasks, which were reused
            pTask->jobID = 0;
            // save the status
            pTask->opRes = MFX_ERR_NONE;

            pTask->done.notify_all();

            // remove dependencies produced from the dependency table
            for (i = 0; i < MFX_TASK_NUM_DEPENDENCIES; i += 1)
            {
                if (pTask->param.task.pDst[i])
                {
                    mfxU32 idx = pTask->param.dependencies.dstIdx[i];

                    m_pDependencyTable.at(idx).p = nullptr;
                }
            }

            // mark all dependent task as 'ready'
            pTask->ResolveDependencies(MFX_ERR_NONE);
            // release all allocated resources
            pTask->ReleaseResources();

            // task object becomes free
            taskReleased = true;
        }
    }


    // wake up additional threads for this task and tasks dependent
    if (m_DedicatedThreadsToWakeUp || m_RegularThreadsToWakeUp) {
        WakeUpThreads(m_DedicatedThreadsToWakeUp, m_RegularThreadsToWakeUp);
    }

    // wake up external threads waiting for a free task object
    if (taskReleased)
    {
        ++m_freeTasksCount;
        m_freeTasks.notify_one();
    }

    // send tracing event
    if (nTraceTaskId)
    {
        MFX_LTRACE_1(MFX_TRACE_LEVEL_SCHED, "^Completed^", "%d", nTraceTaskId);
    }

}

// update dependencies produced from the dependency table
void mfxSchedulerCore::ResolveDependencyTable(MFX_SCHEDULER_TASK *pTask)
{
    mfxU32 i;
    for (i = 0; i < MFX_TASK_NUM_DEPENDENCIES; i += 1)
    {
        if (pTask->param.task.pDst[i])
        {
            mfxU32 idx = pTask->param.dependencies.dstIdx[i];

            m_pDependencyTable.at(idx).mfxRes = pTask->curStatus;
        }
    }
}
