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

#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

#include "vm_event.h"

static void vm_event_set_invalid_internal(vm_event *event)
{
    memset(event, 0, sizeof(vm_event));
    event->state = -1;
}

/* Invalidate an event */
void vm_event_set_invalid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    vm_event_set_invalid_internal(event);

} /* void vm_event_set_invalid(vm_event *event) */

/* Verify if an event is valid */
int32_t vm_event_is_valid(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return 0;

    return event->state >= 0;

} /* int32_t vm_event_is_valid(vm_event *event) */

/* Init an event. Event is created unset. return 1 if success */
vm_status vm_event_init(vm_event *event, int32_t manual, int32_t state)
{
    int res = 0;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    event->manual = manual;
    event->state = state ? 1 : 0;
    res = pthread_cond_init(&event->cond, 0);
    if (!res)
    {
        res = pthread_mutex_init(&event->mutex, 0);
        if (res)
        {
            int cres = pthread_cond_destroy(&event->cond);
            assert(!cres); // we experienced undefined behavior
            vm_event_set_invalid_internal(event);
        }
    }

    return (res)? VM_OPERATION_FAILED: VM_OK;

} /* vm_status vm_event_init(vm_event *event, int32_t manual, int32_t state) */

vm_status vm_event_named_init(vm_event * event,
                              int32_t manual, int32_t state, const char * pcName)
{
    (void)event;
    (void)manual;
    (void)state;
    (void)pcName;

    /* linux version of named events is not supported by now */
    return VM_OPERATION_FAILED;

} /* vm_status vm_event_named_init(vm_event *event, */

/* Set the event to either HIGH (1) or LOW (0) state */
vm_status vm_event_signal(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;
    int res = 0;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        res = pthread_mutex_lock(&event->mutex);
        if (!res)
        {
            umc_status = VM_OK;
            if (0 == event->state)
            {
                event->state = 1;
                if (event->manual)
                {
                    res = pthread_cond_broadcast(&event->cond);
                    if (res)
                    {
                        umc_status = VM_OPERATION_FAILED;
                    }
                }
                else
                {
                    res = pthread_cond_signal(&event->cond);
                    if (res)
                    {
                        umc_status = VM_OPERATION_FAILED;
                    }
                }
            }

            if (0 != pthread_mutex_unlock(&event->mutex))
            {
                umc_status = VM_OPERATION_FAILED;
            }
        }
        else
        {
            umc_status = VM_OPERATION_FAILED;
        }
    }
    return umc_status;

} /* vm_status vm_event_signal(vm_event *event) */

vm_status vm_event_reset(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;
    int res = 0;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        res = pthread_mutex_lock(&event->mutex);
        if (!res)
        {
            umc_status = VM_OK;
            if (1 == event->state)
                event->state = 0;

            if (0 != pthread_mutex_unlock(&event->mutex))
            {
                umc_status = VM_OPERATION_FAILED;
            }
        }
        else
        {
            umc_status = VM_OPERATION_FAILED;
        }
    }
    return umc_status;

} /* vm_status vm_event_reset(vm_event *event) */

/* Pulse the event 0 -> 1 -> 0 */
vm_status vm_event_pulse(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;
    int res = 0;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        res = pthread_mutex_lock(&event->mutex);
        if (!res)
        {
            umc_status = VM_OK;

            if (event->manual)
            {
                res = pthread_cond_broadcast(&event->cond);
                if (res)
                {
                    umc_status = VM_OPERATION_FAILED;
                }
            }
            else
            {
                res = pthread_cond_signal(&event->cond);
                if (res)
                {
                    umc_status = VM_OPERATION_FAILED;
                }
            }

            event->state = 0;

            if (0 != pthread_mutex_unlock(&event->mutex))
            {
                umc_status = VM_OPERATION_FAILED;
            }
        }
        else
        {
            umc_status = VM_OPERATION_FAILED;
        }
    }
    return umc_status;

} /* vm_status vm_event_pulse(vm_event *event) */

/* Wait for event to be high with blocking */
vm_status vm_event_wait(vm_event *event)
{
    vm_status umc_status = VM_NOT_INITIALIZED;
    int res;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        res = pthread_mutex_lock(&event->mutex);
        if (!res)
        {
            umc_status = VM_OK;
            if (!event->state)
            {
                while (!res && !event->state)
                {
                    res = pthread_cond_wait(&event->cond,&event->mutex);
                }
                if (res)
                {
                    umc_status = VM_OPERATION_FAILED;
                }
            }

            if (!event->manual)
                event->state = 0;

            if (0 != pthread_mutex_unlock(&event->mutex))
            {
                umc_status = VM_OPERATION_FAILED;
            }
        }
        else
        {
            umc_status = VM_OPERATION_FAILED;
        }
    }
    return umc_status;

} /* vm_status vm_event_wait(vm_event *event) */

/* Wait for event to be high without blocking, return 1 if successful */
vm_status vm_event_timed_wait(vm_event *event, uint32_t msec)
{
    vm_status umc_status = VM_NOT_INITIALIZED;
    int res = 0;

    /* check error(s) */
    if (NULL == event)
        return VM_NULL_PTR;

    if (0 <= event->state)
    {
        res = pthread_mutex_lock(&event->mutex);
        if (!res)
        {
            if (0 == event->state)
            {
                if (0 == msec)
                {
                    umc_status = VM_TIMEOUT;
                }
                else
                {
                    struct timeval tval;
                    struct timespec tspec;
                    int32_t i_res;
                    unsigned long long micro_sec;

                    gettimeofday(&tval, NULL);
                    // NOTE: micro_sec _should_ be unsigned long long, not uint32_t to avoid overflow
                    micro_sec = 1000 * msec + tval.tv_usec;
                    tspec.tv_sec = tval.tv_sec + (uint32_t)(micro_sec / 1000000);
                    tspec.tv_nsec = (uint32_t)(micro_sec % 1000000) * 1000;
                    i_res = 0;

                    while (!i_res && !event->state)
                    {
                        i_res = pthread_cond_timedwait(&event->cond,
                            &event->mutex,
                            &tspec);
                    }

                    if (0 == i_res)
                        umc_status = VM_OK;
                    else if (ETIMEDOUT == i_res)
                        umc_status = VM_TIMEOUT;
                    else
                        umc_status = VM_OPERATION_FAILED;
                }
            }
            else
                umc_status = VM_OK;

            if (!event->manual)
                event->state = 0;
        }
        else
        {
            umc_status = VM_OPERATION_FAILED;
        }

        if(pthread_mutex_unlock(&event->mutex))
        {
            umc_status = VM_OPERATION_FAILED;
        }
    }
    return umc_status;

} /* vm_status vm_event_timed_wait(vm_event *event, uint32_t msec) */

/* Destory the event */
void vm_event_destroy(vm_event *event)
{
    /* check error(s) */
    if (NULL == event)
        return;

    if (event->state >= 0)
    {
        int res = pthread_cond_destroy(&event->cond);
        assert(!res); // we experienced undefined behavior
        res = pthread_mutex_destroy(&event->mutex);
        assert(!res); // we experienced undefined behavior

        vm_event_set_invalid_internal(event);
    }
} /* void vm_event_destroy(vm_event *event) */
#else
# pragma warning( disable: 4206 )
#endif /* LINUX32 */
