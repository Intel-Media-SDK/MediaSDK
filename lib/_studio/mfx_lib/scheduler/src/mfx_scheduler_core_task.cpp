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

#include <mfx_scheduler_core_task.h>
#include <mfx_scheduler_core.h>


#include <memory.h>

MFX_SCHEDULER_TASK::MFX_SCHEDULER_TASK(mfxU32 taskID, mfxSchedulerCore *pSchedulerCore) :
    taskID(taskID),
    jobID(0),
    pNext(NULL),
    m_pSchedulerCore(pSchedulerCore)
{
    vm_cond_set_invalid(&done);

    // reset task parameters
    memset(&param, 0, sizeof(param));
}

MFX_SCHEDULER_TASK::~MFX_SCHEDULER_TASK(void)
{
    vm_cond_destroy(&done);
}

mfxStatus MFX_SCHEDULER_TASK::Reset(void)
{
    // reset task parameters
    memset(&param, 0, sizeof(param));

    opRes = MFX_WRN_IN_EXECUTION;
    curStatus = MFX_TASK_WORKING;

    return MFX_ERR_NONE;

} // mfxStatus MFX_SCHEDULER_TASK::Reset(void)

void MFX_SCHEDULER_TASK::OnDependencyResolved(mfxStatus result)
{
    if (isFailed(result))
    {

        // waiting task inherits status from the parent task
        // need to propogate error status to all dependent tasks.
        //if (MFX_TASK_WAIT & param.task.threadingPolicy)
        {
            opRes = result;
            curStatus = result;
        }
        // all other tasks are aborted
        //else
        //{
        //    opRes = MFX_ERR_ABORTED;
        //    curStatus = MFX_ERR_ABORTED;
        //}

        // need to update dependency table for all tasks dependent from failed 
        m_pSchedulerCore->ResolveDependencyTable(this);
        vm_cond_broadcast(&done);

        // release the current task resources
        ReleaseResources();

        // be aware of external call
        try
        {
            MFX_ENTRY_POINT &entryPoint = param.task.entryPoint;

            if (entryPoint.pCompleteProc)
            {
                // release the component's resources
                entryPoint.pCompleteProc(entryPoint.pState,
                                         entryPoint.pParam,
                                         MFX_ERR_ABORTED);
            }
        }
        catch(...)
        {
        }
    }

    // call the parent's method
    mfxDependencyItem<MFX_TASK_NUM_DEPENDENCIES>::OnDependencyResolved(result);

} // void MFX_SCHEDULER_TASK::OnDependencyResolved(mfxStatus result)

void MFX_SCHEDULER_TASK::ReleaseResources(void)
{
    if (param.pThreadAssignment)
    {
        param.pThreadAssignment->m_numRefs -= 1;
        if (param.pThreadAssignment->pLastTask == this)
        {
            param.pThreadAssignment->pLastTask = NULL;
        }
    }

    // thread assignment info is not required for the task any more
    param.pThreadAssignment = NULL;

} // void MFX_SCHEDULER_TASK::ReleaseResources(void)
