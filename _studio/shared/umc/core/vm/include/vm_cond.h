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

#ifndef __VM_COND_H__
#define __VM_COND_H__

#include "vm_types.h"
#include "vm_mutex.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Invalidate a condvar */
void vm_cond_set_invalid(vm_cond *cond);

/* Verify if a condvar is valid */
int32_t  vm_cond_is_valid(vm_cond *cond);

/* Init a condvar, return VM_OK if success */
vm_status vm_cond_init(vm_cond *cond);

/* Sleeps on the specified condition variable and releases the specified critical section as an atomic operation */
vm_status vm_cond_wait(vm_cond *cond, vm_mutex *mutex);

/* Sleeps on the specified condition variable and releases the specified critical section as an atomic operation */
vm_status vm_cond_timedwait(vm_cond *cond, vm_mutex *mutex, uint32_t msec);

/* Wake a single thread waiting on the specified condition variable */
vm_status vm_cond_signal(vm_cond *cond);

/* Wake all threads waiting on the specified condition variable */
vm_status vm_cond_broadcast(vm_cond *cond);

/* Destroy the condvar */
void vm_cond_destroy(vm_cond *cond);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_COND_H__ */
