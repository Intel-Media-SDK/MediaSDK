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

#if !defined(__MFX_SCHEDULER_CORE_H)
#define __MFX_SCHEDULER_CORE_H

#include <mfx_interface_scheduler.h>

#include <mfx_scheduler_core_thread.h>
#include <mfx_scheduler_core_handle.h>
#include <mfx_scheduler_core_task.h>

#include <mfx_task.h>

// synchronization stuff
#include <vm_time.h>

#include <vector>

#include "mfx_common.h"

// set the following define to let the scheduler write log file
// with all its activities.
//#define MFX_SCHEDULER_LOG


enum
{
    // thread statistic is gathered within the following period of time (in msec).
    MFX_TIME_STAT_PERIOD        = 2000,
    // the following const specifies how many parts periods time devided to
    MFX_TIME_STAT_PARTS         = 4
};

enum
{
    MFX_INVALID_THREAD_ID       = -1
};

enum
{
    MFX_THREAD_TIME_TO_WAIT     = 1000
};


// forward declaration of the used classes
struct MFX_SCHEDULER_TASK;
struct MFX_THREAD_ASSIGNMENT;

typedef
struct MFX_DEPENDENCY_ITEM
{
    // Pointer to an item to sync
    void *p;

    // Result of the sync operation
    mfxStatus mfxRes;

    // Pointer to the task producing the output
    MFX_SCHEDULER_TASK *pTask;

} MFX_DEPENDENCY_ITEM;

typedef
struct MFX_THREADS_TIME
{
    // Starting time stamp of the item
    mfxU64 startTime;

    // Overall working time for every priority
    mfxU64 time[MFX_PRIORITY_NUMBER];

} MFX_THREAD_TIME;

typedef
struct MFX_CALL_INFO
{
    // Handle of the task
    mfxTaskHandle taskHandle;
    // Pointer to the entry point object
    const MFX_TASK *pTask;
    // Thread ordinal number for the current call
    mfxU32 threadNum;
    // Current call ordinal number
    mfxU32 callNum;

    // Status code of the task completion
    mfxStatus res;
    // Time stamp of task assignment
    mfxU64 timeStamp;
    // The time spent by the call
    mfxU64 timeSpend;

} MFX_CALL_INFO;

enum eWakeUpReason
{
    // wake up threads without a visible reason
    MFX_SCHEDULER_NO_WAKEUP_REASON = 0,
    // new task came
    MFX_SCHEDULER_NEW_TASK = 1,
    // hardware buffer is completed
    MFX_SCHEDULER_HW_BUFFER_COMPLETED = 2
};


class mfxSchedulerCore : public MFXIScheduler2
{
public:
    // Default constructor
    mfxSchedulerCore(void);

    //
    // MFXIScheduler interface
    //

    // Initialize the scheduler. Initialize the dependency tables and run threads.
    virtual
    mfxStatus Initialize(const MFX_SCHEDULER_PARAM *pParam = 0);

    virtual
    mfxStatus Initialize2(const MFX_SCHEDULER_PARAM2 *pParam = 0);

    // Add a new task to the scheduler. Threads start processing task immediately.
    virtual
    mfxStatus AddTask(const MFX_TASK &task, mfxSyncPoint *pSyncPoint);

    // Make synchronization, wait until task is done.
    virtual
    mfxStatus Synchronize(mfxSyncPoint syncPoint, mfxU32 timeToWait);

    // Wait until specified dependency become resolved
    virtual
    mfxStatus WaitForDependencyResolved(const void *pDependency);

    // Wait until all tasks of specified owner become complete or unattended
    virtual
    mfxStatus WaitForAllTasksCompletion(const void *pOwner);

    // Reset 'waiting' status for tasks of specified owner
    virtual
    mfxStatus ResetWaitingStatus(const void *pOwner);

    // Check the current status of the scheduler.
    virtual
    mfxStatus GetState(void);

    // Get the initialization parameters of the scheduler
    virtual
    mfxStatus GetParam(MFX_SCHEDULER_PARAM *pParam);

    // Recover from the failure.
    virtual
    mfxStatus Reset(void);

    // Send a performance message to the scheduler
    virtual
    mfxStatus AdjustPerformance(const mfxSchedulerMessage message);

    // Add a new task to the scheduler with extended source info.
    virtual
    mfxStatus AddTask(const MFX_TASK &task, mfxSyncPoint *pSyncPoint,
                      const char *pFileName, int lineNumber);

    //
    // MFXIUnknown interface
    //

    // Query another interface from the object. If the pointer returned is not NULL,
    // the reference counter is incremented automatically.
    virtual
    void *QueryInterface(const MFX_GUID &guid);

    // Increment reference counter of the object.
    virtual
    void AddRef(void);
    // Decrement reference counter of the object.
    // If the counter is equal to zero, destructor is called and
    // object is removed from the memory.
    virtual
    void Release(void);
    // Get the current reference counter value
    virtual
    mfxU32 GetNumRef(void) const;

    // Entry point for external threads
    virtual
    mfxStatus DoWork();

    //MFX_SCHEDULER_TASK is only one consumer
    void ResolveDependencyTable(MFX_SCHEDULER_TASK *pTask);

    // Notification to the scheduler that task got resolved dependencies
    void OnDependencyResolved(MFX_SCHEDULER_TASK *pTask);

    // WA for SINGLE THREAD MODE
    virtual
    mfxStatus GetTimeout(mfxU32 & maxTimeToRun);
protected:
    // Destructor is protected to avoid deletion the object by occasion.
    virtual
    ~mfxSchedulerCore(void);

    // Release the object
    void Close(void);

    // Wait until the scheduler got more work
    void Wait(const mfxU32 curThreadNum, std::unique_lock<std::mutex>& mutex);

    // Get high performance counter value. This counter is used to calculate
    // tasks duration and priority management.
    mfxU64 GetHighPerformanceCounter(void);
    // Get low-resolution counter. The timer is used for priority management
    // only.
    mfxU32 GetLowResCurrentTime(void);

    // Version of synchronize function, which uses handle
    mfxStatus Synchronize(mfxTaskHandle handle, mfxU32 timeToWait);

    //
    // WARNING: The functions below are not a thread-safe function,
    // external synchronization is required.
    //

    // Wake up requested number of dedicated and regular threads
    void WakeUpThreads(
        mfxU32 num_dedicated_threads = (mfxU32)-1,
        mfxU32 num_regular_threads = (mfxU32)-1);
    // Allocate the empty task
    mfxStatus AllocateEmptyTask(void);
    // Get the index in the occupancy table. The functions searches through
    // the table and return the index of the element tracking the same pState
    // as the task have.
    mfxStatus GetOccupancyTableIndex(mfxU32 &idx, const MFX_TASK *pTask);
    // Remove completed tasks to the 'free' queue
    void ScrubCompletedTasks(bool bComprehensive = false);
    // Register task outputs as dependencies.
    void RegisterTaskDependencies(MFX_SCHEDULER_TASK *pTask);
    // Recover the dependency table after failure
    void RecoverDependencyTable(const void *pDependency);

    // Get the priority of the task
    int GetTaskPriority(mfxTaskHandle task);

    // Get time statistic for the moment
    void GetTimeStat(mfxU64 timeSpent[MFX_PRIORITY_NUMBER],
                     mfxU64 totalTimeSpent[MFX_PRIORITY_NUMBER]);

    // Check if the thread can continue the previous task.
    mfxStatus CanContinuePreviousTask(MFX_CALL_INFO &callInfo,
                                      mfxTaskHandle previousTask,
                                      const mfxU32 threadNum);

    // Check whether task is ready to run
    bool IsReadyToRun(MFX_SCHEDULER_TASK *pTask);

    // Wrap up the task into thread-friendly object,
    // check all dependencies and conditions.
    mfxStatus WrapUpTask(MFX_CALL_INFO &callInfo,
                         MFX_SCHEDULER_TASK *pTask,
                         const mfxU32 threadNum);

    inline void call_pRoutine(MFX_CALL_INFO& call);

    //
    // End of thread-unsafe functions declarations.
    //

    // Provide a task for an internal thread
    mfxStatus GetTask(MFX_CALL_INFO &callInfo,
                      mfxTaskHandle previousTask,
                      const mfxU32 threadNum);
    // Mark a piece of job completed by the thread
    void MarkTaskCompleted(const MFX_CALL_INFO *pCallInfo,
                           const mfxU32 threadNum);
    // Reset 'waiting' state for tasks with given owner
    void ResetWaitingTasks(const void *pOwner);
    // Managing HW event counter functions
    inline
    void IncrementHWEventCounter(void);
    inline
    mfxU64 GetHWEventCounter(void) const;

    void PrintTaskInfo(void);
    void PrintTaskInfoUnsafe(void);

    // Sets scheduling for the specified thread
    bool SetScheduling(std::thread& handle);

    // Assign socket affinity for every thread
    void SetThreadsAffinityToSockets(void);

    inline MFX_SCHEDULER_THREAD_CONTEXT* GetThreadCtx(mfxU32 thread_id)
    { return &m_pThreadCtx[thread_id]; }

    // Invokes functor 'bool F(MFX_SCHEDULER_TASK*)' for every valid task that returns 'true' to continue iteration or 'false' to stop it.
    template <typename F>
    void ForEachTaskWhile(F&& f)
    {
        MFX_SCHEDULER_TASK *task;
        for (int priority = MFX_PRIORITY_HIGH; priority >= MFX_PRIORITY_LOW; priority -= 1)
        {
            for (int type = MFX_TYPE_HARDWARE; type <= MFX_TYPE_SOFTWARE; type += 1)
            {
                task = m_pTasks[priority][type];

                // run over the tasks with particular priority
                while (task)
                {
                    if (false == std::forward<F>(f)(task))
                        return;
                    // advance the task pointer
                    task = task->pNext;
                }
            }
        }
    }

    // Invokes functor 'void F(MFX_SCHEDULER_TASK*)' for every valid task
    template <typename F>
    void ForEachTask(F&& f) {

        ForEachTaskWhile(
            [&f] (MFX_SCHEDULER_TASK *task)
            { std::forward<F>(f)(task); return true; }
        );
    }

    // Scheduler's initialization parameters
    MFX_SCHEDULER_PARAM2 m_param;
    // Reference counters
    mfxU32 m_refCounter;
    // Current time stamp
    mfxU64 m_currentTimeStamp;
    // Time wait period for 'waiting' tasks
    const
    mfxU64 m_timeWaitPeriod;
    // HW 'buffer done' event counter
    volatile
    mfxU64 m_hwEventCounter;
    // Frequency for vm_tick to get msec
    vm_tick m_vmtick_msec_frequency;

    // Working time statistic array
    MFX_THREADS_TIME m_workingTime[MFX_TIME_STAT_PARTS];
    // Current time statistic index
    mfxU32 m_timeIdx;

    //
    // THREADING STUFF
    //

    // Initialize and start a wake up thread
    mfxStatus StartWakeUpThread(void);
    // Stop and terminate the wake up thread
    mfxStatus StopWakeUpThread(void);

    // 'quit' flag for threads
    volatile
    bool m_bQuit;
    volatile
    bool m_bQuitWakeUpThread;

    // Threads contexts
    MFX_SCHEDULER_THREAD_CONTEXT *m_pThreadCtx;



    // Condition variable to wait free task objects
    mfxU16 m_freeTasksCount;
    std::condition_variable m_freeTasks;
    // Handle to the wakeup thread
    std::thread m_hwWakeUpThread;

    // declare thread working routine
    void ThreadProc(MFX_SCHEDULER_THREAD_CONTEXT *pContext);
    void WakeupThreadProc();

    //
    // TASKING STUFF
    //

    // Guard for task queues
    std::mutex m_guard;
    // array of task queues
    MFX_SCHEDULER_TASK *m_pTasks[MFX_PRIORITY_NUMBER][MFX_TYPE_NUMBER];
    // Number of assigned tasks for each kind of tasks
    mfxU32 m_numAssignedTasks[MFX_PRIORITY_NUMBER];
    // Queue of failed tasks
    MFX_SCHEDULER_TASK *m_pFailedTasks;
    // Number of tasks for the dedicated thread
    mfxU32 m_DedicatedThreadsToWakeUp;
    // Number of tasks for non-dedicated threads
    mfxU32 m_RegularThreadsToWakeUp;

    // these members are used only from the main thread,
    // so synchronization is not necessary to access them.

    // Table to get a task by handle value
    std::vector<MFX_SCHEDULER_TASK *> m_ppTaskLookUpTable;
    // Queue of available tasks
    MFX_SCHEDULER_TASK *m_pFreeTasks;

    //
    // DEPENDENCY STUFF
    //

    // Dependency table.
    // There are an index: it holds the number of values valid in the array.
    // The array may have larger size.
    std::vector<MFX_DEPENDENCY_ITEM> m_pDependencyTable;
    volatile
    mfxU32 m_numDependencies;

    // Threads assignment table.
    std::vector<MFX_THREAD_ASSIGNMENT> m_occupancyTable;
    // Number of valid entries in the table.
    volatile
    mfxU32 m_numOccupancies;

    // Number of allocated task objects
    mfxU32 m_taskCounter;
    // Number of job submitted
    mfxU32 m_jobCounter;

    mfxU32 m_timer_hw_event;


private:
    // declare a assignment operator to avoid warnings
    mfxSchedulerCore & operator = (mfxSchedulerCore &)
    {
        return *this;
    }

    mfxSchedulerCore(const mfxSchedulerCore &s);        // No copy CTR

};

inline
void mfxSchedulerCore::IncrementHWEventCounter(void)
{
    m_hwEventCounter += 1;

} // void mfxSchedulerCore::IncrementHWEventCounter(void)

inline
mfxU64 mfxSchedulerCore::GetHWEventCounter(void) const
{
    return m_hwEventCounter;

} // mfxU64 mfxSchedulerCore::GetHWEventCounter(void) const

inline
void mfxSchedulerCore::call_pRoutine(MFX_CALL_INFO& call)
{
    const char *pRoutineName = (call.pTask->entryPoint.pRoutineName)?
        call.pTask->entryPoint.pRoutineName:
        "MFX Async Task";
    mfxU64 start;

    (void)pRoutineName;
    MFX_AUTO_LTRACE(MFX_TRACE_LEVEL_SCHED, pRoutineName);
    MFX_LTRACE_1(MFX_TRACE_LEVEL_SCHED, "^Child^of", "%d", call.pTask->nParentId);

    // mark beginning of working period
    start = GetHighPerformanceCounter();
    try {
        if (call.pTask->bObsoleteTask) {
            // NOTE: it is legacy task call, it should be eliminated soon
            call.res = call.pTask->entryPoint.pRoutine(
                call.pTask->entryPoint.pState,
                (void *) &call.pTask->obsolete_params,
                call.threadNum,
                call.callNum);
        } else {
            // NOTE: this is the only legal task calling process.
            // Should survive only this one.
            call.res = call.pTask->entryPoint.pRoutine(
                call.pTask->entryPoint.pState,
                call.pTask->entryPoint.pParam,
                call.threadNum,
                call.callNum);
        }
    } catch(...) {
        call.res = MFX_ERR_UNKNOWN;
    }

    call.timeSpend = (GetHighPerformanceCounter() - start);

    MFX_LTRACE_1(MFX_TRACE_LEVEL_SCHED, "mfxRes = ", "%d", call.res);
}

#endif // !defined(__MFX_SCHEDULER_CORE_H)
