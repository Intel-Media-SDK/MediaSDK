// Copyright (c) 2017-2020 Intel Corporation
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

#if !defined (__MFX_SCHEDULER_CORE_TASK_H)
#define __MFX_SCHEDULER_CORE_TASK_H

#include <mfxdefs.h>

#include <mfx_dependency_item.h>
#include <mfx_task.h>
#include <mfx_scheduler_core_handle.h>

#include <condition_variable>

// forward declaration of used types
struct MFX_SCHEDULER_TASK;


class mfxSchedulerCore;
struct MFX_THREAD_ASSIGNMENT
{
    // Pointer to the object being shared among the threads/tasks
    void *pState;
    // Pointer to the routine being shared among the threads/tasks
    mfxTaskRoutine pRoutine;
    // Current thread assignment type
    mfxTaskThreadingPolicy threadingPolicy;
    // Number of tasks using this element of table
    mfxU32 m_numRefs;

    // Number of task being processed semultaneously
    mfxU32 taskOccupancy;
    // Current occupancy of the task(s) (number of threads entered inside)
    mfxU32 occupancy;
    // Occupied thread numbers mask
    mfxU64 threadMask;

    // Pointer to the last task using this pState.
    MFX_SCHEDULER_TASK *pLastTask;

};

struct MFX_SCHEDULER_TASK : public mfxDependencyItem<MFX_TASK_NUM_DEPENDENCIES>
{
    // The constructor is disabled for precise resource control.
    // Declare a class what can create instances of tasks.
    friend class mfxSchedulerCore;

    // Reset the task into the initial state
    mfxStatus Reset(void);

    // Update the dependent object with the status of the parent object
    virtual
    void OnDependencyResolved(mfxStatus result);

    // wraps an optional call to pCompleteProc
    mfxStatus CompleteTask(mfxStatus res);

    // Release all allocated resources and decrement reference counters
    void ReleaseResources(void);

    // Ordinal task number. It doesn't grow during task lifetime.
    // This ID is used to access task tracking tables and lists.
    const
    mfxU32 taskID;
    // Job number. It is set to a new value, which is steadly growing, everytime
    // when task is assigned for a new piece of job. Sometimes jobID is accessed
    // without synchronization, so it is required not to cache it.
    volatile
    mfxU32 jobID;

    // task state variables

    // Waiting 'until task is done' object
    std::condition_variable done;
    // Final status of the current job
    volatile
    mfxStatus opRes;
    // Status of the last error/success processing. This status is copyied into
    // opRes variable, when the task is done and the last thread leaves the task.
    volatile
    mfxStatus curStatus;

    // make all task's parameters as a separate object
    // to make initialization easier.
    struct
    {

        // task describing parameters

        // Task's parameters
        MFX_TASK task;
        // Pointer to the thread occupancy table's entity
        MFX_THREAD_ASSIGNMENT *pThreadAssignment;
        // Current occupancy of the task (number of threads entered inside)
        mfxU32 occupancy;
        // Occupied threads bit mask
        mfxU64 threadMask;
        // Number of call of the task
        mfxU32 numberOfCalls;

        // task timing parameters
        bool bWaiting;                                              // (bool) task needs some waiting
        struct
        {
            // Time in msec of the last 'entering' to the task
            mfxU64 timeLastEnter;
            // Time stamp of the last call issued
            mfxU64 timeLastCallIssued;
            // Time stamp of the last processed call result
            mfxU64 timeLastCallProcessed;
            // Integral time spend for the task
            mfxU64 timeSpent;
            // Time spend for false thread entering
            mfxU64 timeOverhead;
            // HW counter value of the last 'entering' to the task
            mfxU64 hwCounterLastEnter;
        } timing;

        // source file info
        struct
        {
            const char *pFileName;                                      // (const char *) source file name, where task was spawn
            int lineNumber;                                             // (int) source source file line number, where task was spawn
        } sourceInfo;

        // dependencies members
        struct
        {
            // Indicies in the dependency table of the generated outputs
            mfxU32 dstIdx[MFX_TASK_NUM_DEPENDENCIES];
        } dependencies;

    } param;

    // Pointer to the next task
    MFX_SCHEDULER_TASK *pNext;

    // bad practice to get cross links, but this is how our scheduler designed 
    mfxSchedulerCore *m_pSchedulerCore;

protected:
    // Destructor is protected to avoid deletion the object by occasion.
    virtual
    ~MFX_SCHEDULER_TASK(void);

private:
    // Constructor. Because scheduler's tasks is a limited resource,
    // prevent task creation on the stack.
    MFX_SCHEDULER_TASK(mfxU32 taskID, mfxSchedulerCore *m_pSchedulerCore);

    // Declare assignment operator to avoid
    // assignment of object by accident.
    MFX_SCHEDULER_TASK & operator = (MFX_SCHEDULER_TASK &);
};

// Get the number of task allocated
mfxU32 GetNumTaskAllocated(void);

#endif // !defined (__MFX_SCHEDULER_CORE_TASK_H)
