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

#ifndef __VM_THREAD_H__
#define __VM_THREAD_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* IMPORTANT:
 *    The thread return value is always saved for the calling thread to
 *    collect. To avoid memory leak, always vm_thread_wait() for the
 *    child thread to terminate in the calling thread.
 */

#define VM_THREAD_CALLCONVENTION

typedef uint32_t (VM_THREAD_CALLCONVENTION * vm_thread_callback)(void *);

typedef enum
{
    VM_THREAD_PRIORITY_HIGHEST,
    VM_THREAD_PRIORITY_HIGH,
    VM_THREAD_PRIORITY_NORMAL,
    VM_THREAD_PRIORITY_LOW,
    VM_THREAD_PRIORITY_LOWEST
} vm_thread_priority;

/* Set the thread handler to an invalid value */
void vm_thread_set_invalid(vm_thread *thread);

/* Verify if the thread handler is valid */
int32_t vm_thread_is_valid(vm_thread *thread);

/* Create a thread. The thread is set to run at the call.
   Return 1 if successful */
int32_t vm_thread_create(vm_thread *thread, vm_thread_callback func, void *arg);


/* Attach to externally created thread. Should be called from the thread in question.
Return 1 if successful */
int32_t vm_thread_attach(vm_thread *thread, vm_thread_callback func, void *arg);

/* Wait until a thread exists */
void vm_thread_wait(vm_thread *thread);

/* Set thread priority. Return 1 if successful */
int32_t vm_thread_set_priority(vm_thread *thread, vm_thread_priority priority);

/* Close thread after all */
void vm_thread_close(vm_thread *thread);

/* Get current thread priority */
vm_thread_priority vm_get_current_thread_priority(void);

/* Set current thread priority */
void vm_set_current_thread_priority(vm_thread_priority priority);

typedef struct
{
  int32_t schedtype;
  int32_t priority;
} vm_thread_linux_schedparams;

/* Set thread scheduling type and corresponding parameters. */
int32_t vm_thread_set_scheduling(vm_thread* thread, void* params);

/* Set thread's affinity mask */
void vm_set_thread_affinity_mask(vm_thread *thread, unsigned long long mask);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_THREAD_H__ */
