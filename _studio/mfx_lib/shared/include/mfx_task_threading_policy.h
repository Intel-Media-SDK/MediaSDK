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

#ifndef __MFX_TASK_THREADING_POLICY_H
#define __MFX_TASK_THREADING_POLICY_H

// Declare task execution flag's
enum
{
    // Task can't share processing unit with other tasks
    MFX_TASK_INTRA = 1,
    // Task can share processing unit with other tasks
    MFX_TASK_INTER = 2,
    // Task can be managed by thread 0 only
    MFX_TASK_DEDICATED = 4,
    // Task share executing threads with other tasks
    MFX_TASK_SHARED = 8

};

typedef
enum mfxTaskThreadingPolicy
{
    // The plugin doesn't support parallel task execution.
    // Tasks need to be processed one by one.
    MFX_TASK_THREADING_INTRA = MFX_TASK_INTRA,

    // The plugin supports parallel task execution.
    // Tasks can be processed independently.
    MFX_TASK_THREADING_INTER = MFX_TASK_INTER,

    // All task marked 'dedicated' is executed by thread #0 only.
    MFX_TASK_THREADING_DEDICATED = MFX_TASK_DEDICATED | MFX_TASK_INTRA,

    // As inter, but the plugin has limited processing resources.
    // The total number of threads is limited.
    MFX_TASK_THREADING_SHARED = MFX_TASK_SHARED,

    MFX_TASK_THREADING_DEFAULT = MFX_TASK_THREADING_DEDICATED

} mfxTaskThreadingPolicy;

#endif // __MFX_TASK_THREADING_POLICY_H
