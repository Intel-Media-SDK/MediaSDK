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

#ifndef __VM_MUTEX_H__
#define __VM_MUTEX_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Invalidate a mutex */
void vm_mutex_set_invalid(vm_mutex *mutex);

/* Verify if a mutex is valid */
int32_t  vm_mutex_is_valid(vm_mutex *mutex);

/* Init a mutex, return VM_OK if success */
vm_status vm_mutex_init(vm_mutex *mutex);

/* Lock the mutex with blocking. */
vm_status vm_mutex_lock(vm_mutex *mutex);

/* Unlock the mutex. */
vm_status vm_mutex_unlock(vm_mutex *mutex);

/* Lock the mutex without blocking, return VM_OK if success */
vm_status vm_mutex_try_lock(vm_mutex *mutex);

/* Destroy a mutex */
void vm_mutex_destroy(vm_mutex *mutex);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_MUTEX_H__ */
