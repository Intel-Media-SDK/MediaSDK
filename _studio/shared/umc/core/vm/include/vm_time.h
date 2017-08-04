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

#ifndef __VM_TIME_H__
#define __VM_TIME_H__

#include "vm_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

typedef long long vm_tick;
typedef int32_t vm_time_handle;

/* Yield the execution of current thread for msec miliseconds */
void vm_time_sleep(uint32_t msec);

/* Obtain the time since the machine started in milliseconds on Windows
 * and time elapsed since the Epoch on Linux
 */
uint32_t vm_time_get_current_time(void);

/* Obtain the clock tick of an uninterrupted master clock */
vm_tick vm_time_get_tick(void);

/* Obtain the clock resolution */
vm_tick vm_time_get_frequency(void);

/* Create the object of time measure */
vm_status vm_time_open(vm_time_handle *handle);

/* Initialize the object of time measure */
vm_status vm_time_init(vm_time *m);

/* Start the process of time measure */
vm_status vm_time_start(vm_time_handle handle, vm_time *m);

/* Stop the process of time measure and obtain the sampling time in seconds */
double vm_time_stop(vm_time_handle handle, vm_time *m);

/* Close the object of time measure */
vm_status vm_time_close(vm_time_handle *handle);

/* Unix style time related functions */
/* TZP must be NULL or VM_NOT_INITIALIZED status will returned */
/* !!! Time zone not supported, !!! vm_status instead of int will returned */
vm_status vm_time_gettimeofday( struct vm_timeval *TP, struct vm_timezone *TZP );

void vm_time_timeradd(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2);
void vm_time_timersub(struct vm_timeval* destination,  struct vm_timeval* src1, struct vm_timeval* src2);
int vm_time_timercmp(struct vm_timeval* src1, struct vm_timeval* src2, struct vm_timeval* threshold);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __VM_TIME_H__ */
