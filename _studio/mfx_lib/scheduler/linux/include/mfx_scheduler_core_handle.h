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

#if !defined(__MFX_SCHEDULER_CORE_HANDLE_H)
#define __MFX_SCHEDULER_CORE_HANDLE_H

#include "vm_types.h"

enum
{
    MFX_BITS_FOR_HANDLE = 32,

    // declare constans for task objects
    MFX_BITS_FOR_TASK_NUM = 10,
    MFX_MAX_NUMBER_TASK = 1 << MFX_BITS_FOR_TASK_NUM,

    // declare constans for job objects
    MFX_BITS_FOR_JOB_NUM = MFX_BITS_FOR_HANDLE - MFX_BITS_FOR_TASK_NUM,
    MFX_MAX_NUMBER_JOB = 1 << MFX_BITS_FOR_JOB_NUM
};

// Type mfxTaskHandle is a composite type,
// which structure is close to a handle structure.
// Few LSBs are used for internal task object indentification.
// Rest bits are used to identify job.
typedef
union mfxTaskHandle
{
    struct
    {
    unsigned int taskID : MFX_BITS_FOR_TASK_NUM;
    unsigned int jobID : MFX_BITS_FOR_JOB_NUM;
    };

    size_t handle;

} mfxTaskHandle;

#endif // !defined(__MFX_SCHEDULER_CORE_HANDLE_H)
