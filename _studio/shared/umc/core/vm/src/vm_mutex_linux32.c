// Copyright (c) 2017-2018 Intel Corporation
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

#if defined(LINUX32)

#define _GNU_SOURCE /* may need on some OS to support PTHREAD_MUTEX_RECURSIVE */

#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "vm_mutex.h"

static void vm_mutex_set_invalid_internal(vm_mutex *mutex)
{
    memset(mutex, 0, sizeof(vm_mutex));
}

/* Invalidate a mutex */
void vm_mutex_set_invalid(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return;

    vm_mutex_set_invalid_internal(mutex);

} /* void vm_mutex_set_invalid(vm_mutex *mutex) */

/* Verify if a mutex is valid */
int32_t vm_mutex_is_valid(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return 0;

    return mutex->is_valid;

} /* int32_t vm_mutex_is_valid(vm_mutex *mutex) */

/* Init a mutex, return 1 if success */
vm_status vm_mutex_init(vm_mutex *mutex)
{
    pthread_mutexattr_t mutex_attr;
    int res = 0;

    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    vm_mutex_destroy(mutex);

    res = pthread_mutexattr_init(&mutex_attr);
    if (!res)
    {
        res = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        /* in Media SDK we definetely need recursive mutex */
        if (!res)
        {
            res = pthread_mutex_init(&mutex->handle, &mutex_attr);
            mutex->is_valid = !res;
            if (res)
            {
                vm_mutex_set_invalid_internal(mutex);
            }
        }
        pthread_mutexattr_destroy(&mutex_attr);
    }
    return (res)? VM_OPERATION_FAILED: VM_OK;

} /* vm_status vm_mutex_init(vm_mutex *mutex) */

/* Lock the mutex with blocking. */
vm_status vm_mutex_lock(vm_mutex *mutex)
{
    vm_status umc_res = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    if (mutex->is_valid)
    {
        if (0 == pthread_mutex_lock(&mutex->handle))
            umc_res = VM_OK;
        else
            umc_res = VM_OPERATION_FAILED;
    }
    return umc_res;

} /* vm_status vm_mutex_lock(vm_mutex *mutex) */

/* Unlock the mutex. */
vm_status vm_mutex_unlock(vm_mutex *mutex)
{
    vm_status umc_res = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    if (mutex->is_valid)
    {
        if (0 == pthread_mutex_unlock(&mutex->handle))
            umc_res = VM_OK;
        else
            umc_res = VM_OPERATION_FAILED;
    }
    return umc_res;

} /* vm_status vm_mutex_unlock(vm_mutex *mutex) */

/* Lock the mutex without blocking, return 1 if success */
vm_status vm_mutex_try_lock(vm_mutex *mutex)
{
    vm_status umc_res = VM_NOT_INITIALIZED;

    /* check error(s) */
    if (NULL == mutex)
        return VM_NULL_PTR;

    if (mutex->is_valid)
    {
        int32_t i_res = pthread_mutex_trylock(&mutex->handle);
        switch (i_res)
        {
        case 0:
            umc_res = VM_OK;
            break;

        case EBUSY:
            umc_res = VM_TIMEOUT;
            break;

        default:
            umc_res = VM_OPERATION_FAILED;
            break;
        }
    }
    return umc_res;

} /* vm_status vm_mutex_try_lock(vm_mutex *mutex) */

/* Destroy the mutex */
void vm_mutex_destroy(vm_mutex *mutex)
{
    /* check error(s) */
    if (NULL == mutex)
        return;

    if (mutex->is_valid)
    {
        int res = pthread_mutex_destroy(&mutex->handle);
        assert(!res); // we experienced undefined behavior
        (void)res;
        vm_mutex_set_invalid_internal(mutex);
    }
} /* void vm_mutex_destroy(vm_mutex *mutex) */
#else
# pragma warning( disable: 4206 )
#endif /* LINUX32 */
