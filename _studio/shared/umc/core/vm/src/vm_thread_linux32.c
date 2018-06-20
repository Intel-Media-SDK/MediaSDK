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

#define _GNU_SOURCE /* required by CPU_ZERO/CPU_SET to be able to link apps */
#include <sched.h>
#include <unistd.h>
#include <sys/time.h>
#include "vm_thread.h"
#include "vm_event.h"
#include "vm_mutex.h"

static
void *vm_thread_proc(void *pv_params)
{
    vm_thread *p_thread = (vm_thread *) pv_params;

    /* check error(s) */
    if (NULL == pv_params)
        return ((void *) -1);

    p_thread->p_thread_func(p_thread->p_arg);
    vm_event_signal(&p_thread->exit_event);

    return ((void *) 1);

} /* void *vm_thread_proc(void *pv_params) */

/* set the thread handler an invalid value */
void vm_thread_set_invalid(vm_thread *thread)
{
    /* check error(s) */
    if (NULL == thread)
        return;

    thread->is_valid = 0;
    thread->i_wait_count = 0;
    vm_event_set_invalid(&thread->exit_event);
    vm_mutex_set_invalid(&thread->access_mut);

} /* void vm_thread_set_invalid(vm_thread *thread) */

/* verify if the thread handler is valid */
int32_t vm_thread_is_valid(vm_thread *thread)
{
    /* check error(s) */
    if (NULL == thread)
        return 0;

    if (thread->is_valid)
    {
        vm_mutex_lock(&thread->access_mut);
        if (VM_OK == vm_event_timed_wait(&thread->exit_event, 0))
        {
            vm_mutex_unlock(&thread->access_mut);
            vm_thread_wait(thread);
        }
        else
            vm_mutex_unlock(&thread->access_mut);
    }
    return thread->is_valid;

} /* int32_t vm_thread_is_valid(vm_thread *thread) */

/* create a thread. return 1 if success */
int32_t vm_thread_create(vm_thread *thread,
                        uint32_t (*vm_thread_func)(void *),
                        void *arg)
{
    int32_t i_res = 1;
//    pthread_attr_t attr;

    /* check error(s) */
    if ((NULL == thread) ||
        (NULL == vm_thread_func))
        return 0;

    if (0 != i_res)
    {
        if (VM_OK != vm_event_init(&thread->exit_event, 1, 0))
            i_res = 0;
    }

    if ((0 != i_res) &&
        (VM_OK != vm_mutex_init(&thread->access_mut)))
        i_res = 0;

    if (0 != i_res)
    {
        vm_mutex_lock(&thread->access_mut);
        thread->p_thread_func = vm_thread_func;
        thread->p_arg = arg;

        thread->is_valid =! pthread_create(&thread->handle,
                                           NULL,
                                           vm_thread_proc,
                                           (void*)thread);

        i_res = (thread->is_valid) ? 1 : 0;
        vm_mutex_unlock(&thread->access_mut);
//        pthread_attr_destroy(&attr);
    }
    return i_res;

} /* int32_t vm_thread_create(vm_thread *thread, */


/* attach to current thread. return 1 if successful  */
int32_t vm_thread_attach(vm_thread *thread, vm_thread_callback func, void *arg)
{
    (void)arg;

    int32_t i_res = 1;
    pthread_attr_t attr;

    /* check error(s) */
    if (NULL == thread)
        return 0;

    if (0 != i_res)
    {
        if (VM_OK != vm_event_init(&thread->exit_event, 1, 0))
            i_res = 0;
    }

    if ((0 != i_res) &&
        (VM_OK != vm_mutex_init(&thread->access_mut)))
        i_res = 0;

    thread->is_valid = 1;
    thread->p_thread_func = func;
    thread->p_arg = 0;
    thread->i_wait_count = 1; // vm_thread_wait should not try to join this thread
    thread->handle = pthread_self();

    return i_res;
}

int32_t vm_thread_set_scheduling(vm_thread* thread, void* params)
{
    int32_t i_res = 1;
    struct sched_param param;
    vm_thread_linux_schedparams* linux_params = (vm_thread_linux_schedparams*)params;

    /* check error(s) */
    if (!thread || !linux_params)
        return 0;

    if (thread->is_valid)
    {
        vm_mutex_lock(&thread->access_mut);
        param.sched_priority = linux_params->priority;
        i_res = !pthread_setschedparam(thread->handle, linux_params->schedtype, &param);
        vm_mutex_unlock(&thread->access_mut);
    }
    return i_res;
}

/* set thread priority, return 1 if successful */
int32_t vm_thread_set_priority(vm_thread *thread, vm_thread_priority priority)
{
    int32_t i_res = 1;
    int32_t policy, pmin, pmax, pmean;
    struct sched_param param;

    /* check error(s) */
    if (NULL == thread)
        return 0;

    if (thread->is_valid)
    {
        vm_mutex_lock(&thread->access_mut);
        pthread_getschedparam(thread->handle,&policy,&param);

        pmin = sched_get_priority_min(policy);
        pmax = sched_get_priority_max(policy);
        pmean = (pmin + pmax) / 2;

        switch (priority)
        {
        case VM_THREAD_PRIORITY_HIGHEST:
            param.sched_priority = pmax;
            break;

        case VM_THREAD_PRIORITY_LOWEST:
            param.sched_priority = pmin;
            break;

        case VM_THREAD_PRIORITY_NORMAL:
            param.sched_priority = pmean;
            break;

        case VM_THREAD_PRIORITY_HIGH:
            param.sched_priority = (pmax + pmean) / 2;
            break;

        case VM_THREAD_PRIORITY_LOW:
            param.sched_priority = (pmin + pmean) / 2;
            break;

        default:
            i_res = 0;
            break;
        }

        if (i_res)
            i_res = !pthread_setschedparam(thread->handle, policy, &param);
        vm_mutex_unlock(&thread->access_mut);
    }
    return i_res;

} /* int32_t vm_thread_set_priority(vm_thread *thread, vm_thread_priority priority) */

/* wait until a thread exists */
void vm_thread_wait(vm_thread *thread)
{
    /* check error(s) */
    if (NULL == thread)
        return;

    if (thread->is_valid)
    {
        vm_mutex_lock(&thread->access_mut);
        thread->i_wait_count++;
        vm_mutex_unlock(&thread->access_mut);

        vm_event_wait(&thread->exit_event);

        vm_mutex_lock(&thread->access_mut);
        thread->i_wait_count--;
        if (0 == thread->i_wait_count)
        {
            pthread_join(thread->handle, NULL);
            thread->is_valid = 0;
        }
        vm_mutex_unlock(&thread->access_mut);
    }
} /* void vm_thread_wait(vm_thread *thread) */

/* close thread after all */
void vm_thread_close(vm_thread *thread)
{
    /* check error(s) */
    if (NULL == thread)
        return;

    vm_thread_wait(thread);
    vm_event_destroy(&thread->exit_event);
    vm_mutex_destroy(&thread->access_mut);

} /* void vm_thread_close(vm_thread *thread) */

vm_thread_priority vm_get_current_thread_priority()
{
    return VM_THREAD_PRIORITY_NORMAL;
}

void vm_set_current_thread_priority(vm_thread_priority priority)
{
    (void)priority;
}

void vm_set_thread_affinity_mask(vm_thread *thread, unsigned long long mask)
{
#if !defined(__ANDROID__)
    int cpu = -1;
    cpu_set_t cpuset;

    /* check error(s) */
    if (NULL == thread)
        return;

    if (!mask)
        return;

    CPU_ZERO(&cpuset);
    while(mask) {
        cpu ++;
        if (mask & 1) {
            CPU_SET(cpu, &cpuset);
        }
        mask >>= 1;
    }

    pthread_setaffinity_np(thread->handle, sizeof(cpu_set_t), &cpuset);
#else
    (void)thread;
    (void)mask;
#endif
}

#else
# pragma warning( disable: 4206 )
#endif /* LINUX32 */
