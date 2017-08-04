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

#ifndef __VM_SEMAPHORE_H__
#define __VM_SEMAPHORE_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Invalidate a semaphore */
void vm_semaphore_set_invalid(vm_semaphore *sem);

/* Verify if a semaphore is valid */
int32_t vm_semaphore_is_valid(vm_semaphore *sem);

/* Init a semaphore with value, return VM_OK if successful */
vm_status vm_semaphore_init(vm_semaphore *sem, int32_t init_count);
vm_status vm_semaphore_init_max(vm_semaphore *sem, int32_t init_count, int32_t max_count);

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_timedwait(vm_semaphore *sem, uint32_t msec);

/* Decrease the semaphore value with blocking. */
vm_status vm_semaphore_wait(vm_semaphore *sem);

/* Decrease the semaphore value without blocking, return VM_OK if success */
vm_status vm_semaphore_try_wait(vm_semaphore *sem);

/* Increase the semaphore value */
vm_status vm_semaphore_post(vm_semaphore *sem);

/* Increase the semaphore value */
vm_status vm_semaphore_post_many(vm_semaphore *sem, int32_t post_count);

/* Destroy a semaphore */
void vm_semaphore_destroy(vm_semaphore *sem);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_SEMAPHORE_H__ */
